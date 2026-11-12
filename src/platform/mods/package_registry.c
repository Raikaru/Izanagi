#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64
#include "p4pc/packages.h"
#include "p4pc/capabilities.h"
#include "p4pc/os.h"
#include "../files/vfs.h"
#include "compendium.h"
#include "file_index.h"
#include "package_memory_internal.h"
#include "package_sort.h"
#include "package_boot_internal.h"
#include "package_identifier.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define CLAIM_BUDGET (8u * 1024u * 1024u)
#define MANIFEST_LIMIT (64u * 1024u)
#define FIELD_LIMIT 2048u
#define HANDLER_LIMIT 16u
struct P4PackageProvider {
    P4PackageRegistry *registry;
    const P4PackageProvider *backing; /* immutable original data, storage pin only */
    P4PackageInfo info;
    unsigned preference; /* explicit selector position, zero when unspecified */
    _Atomic uint32_t claims;
    _Atomic int refused;
    char files[1200];
    unsigned file_flags;
    P4PackageFiles *index;
    P4PackageFileView view;
    char *manifest;
    P4PackageField *fields;
    size_t field_count, field_capacity, manifest_bytes;
    int require_cosmetic;
    const char *requires; /* mod.ini `requires=` capability list, inside manifest */
    const char *diagnostic;
    unsigned diagnostic_line;
};
struct P4PackageRegistry {
    const P4PackageClient *client;
    P4PackageRegistry *next, *previous;
    P4PackageRegistry **output;
    size_t count, conflicts, ordered_count, allocation_bytes;
    P4PackageProvider **ordered;
    _Atomic unsigned references, retired;
    int prepared, preflight_prepared;
    P4PackageProvider packages[];
};
struct P4PackageFilePlan {
    const P4PackageProvider *provider;
    const char *relative;
    uint64_t maximum;
    P4OSFileIdentity identity;
    _Atomic unsigned references;
};
struct P4PackageFile {
    FILE *stream;
    const P4PackageProvider *provider;
    P4PackageFileOrigin origin;
    P4PackageFilePlan *plan;
    uint64_t maximum;
    P4OSFileIdentity identity; /* Used only when no issued plan owns the identity. */
};
typedef struct FileClaim { const char *key; P4PackageProvider *provider; } FileClaim;
typedef struct Discovery {
    const char *root;
    P4PackageRegistry *registry;
    char ids[P4_PACKAGE_MAX][64];
    size_t requested;
    int bundled; /* scanning the read-only root of packages shipped with the game */
    unsigned char found[P4_PACKAGE_MAX], player[P4_PACKAGE_MAX];
    const char *reasons[P4_PACKAGE_MAX]; /* parser diagnostics are static literals */
    int failed;
} Discovery;
int p4_package_startup_blocked;
static P4PackageRegistry *first, *last;
static P4PackageProvider **catalog;
static size_t catalog_count, catalog_capacity, admission_serial;
static uint32_t generation;
static _Atomic int phase; /* 0 staging, 1 preparing, 2 published, 3 failed/retired */
static _Atomic unsigned source_tracking; /* bit0 explicit demand, bit1 sealed */
static const P4PackageHandler *handlers[HANDLER_LIMIT];
static size_t handler_count;
static unsigned char handler_prepared[HANDLER_LIMIT];
static size_t handler_order[HANDLER_LIMIT], handler_order_count;
static unsigned char handler_completed[HANDLER_LIMIT], handler_product_set[HANDLER_LIMIT];
static const void *handler_products[HANDLER_LIMIT];
static unsigned char handler_selected[HANDLER_LIMIT];
static unsigned char runtime_completed[HANDLER_LIMIT], runtime_product_set[HANDLER_LIMIT];
static const void *runtime_products[HANDLER_LIMIT];
static const P4PackageProvider *runtime_failure_provider;
static char runtime_failure_reason[1024];
static int runtime_pass, runtime_state;
static size_t preparing_handler = HANDLER_LIMIT;
static int boot_error;
static int native_dirty;
static P4PackageReadObserver read_observer;
static void *read_observer_user;

int p4_packages_source_tracking_require(void)
{
    unsigned state = atomic_load(&source_tracking);
    do {
        if (state & 2u) {
            fprintf(stderr, "p4pc: verified source tracking requested after activation/retirement\n");
            return -1;
        }
    } while (!atomic_compare_exchange_weak(&source_tracking, &state, state | 1u));
    return 0;
}
int p4_packages_source_tracking_required(void) { return (atomic_load(&source_tracking) & 1u) != 0; }
int p4_packages_source_tracking_seal(void) { return (atomic_fetch_or(&source_tracking, 2u) & 1u) != 0; }

static int join(char *dst, size_t cap, const char *dir, const char *name)
{
    int n = snprintf(dst, cap, "%s/%s", dir, name);
    return n >= 0 && (size_t)n < cap ? 0 : -1;
}
_Static_assert(sizeof ((P4PackageInfo *)0)->id == P4_PACKAGE_ID_SIZE, "package_identifier.h id bound");
static int identifier(const char *s) { return p4_package_identifier(s); }
static char *trim(char *s)
{
    while (isspace((unsigned char)*s)) ++s;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) *--end = 0;
    return s;
}
static int number(const char *s, int *out)
{
    char *end;
    errno = 0;
    long value = strtol(s, &end, 10);
    if (errno || !s[0] || *end || value < INT_MIN || value > INT_MAX) return -1;
    *out = (int)value;
    return 0;
}
static int version(const char *s, unsigned *major, unsigned *minor)
{
    if (*s < '0' || *s > '9') return -1;
    char *end;
    errno = 0;
    unsigned long a = strtoul(s, &end, 10);
    if (errno || *end != '.' || a != P4_PACKAGE_API_MAJOR) return -1;
    s = end + 1;
    if (*s < '0' || *s > '9') return -1;
    unsigned long b = strtoul(s, &end, 10);
    if (errno || *end || b > P4_PACKAGE_API_MINOR) return -1;
    *major = (unsigned)a; *minor = (unsigned)b;
    return 0;
}

/* Generated from include/p4pc/p4.d/<key>.luau (cmake/sources/052-capabilities.cmake). */
static const struct { const char *key; unsigned revision; } capabilities[] = {
#define P4_CAPABILITY(key, revision) {key, revision},
#include "p4_capabilities.inc"
#undef P4_CAPABILITY
};
unsigned p4_capability_revision(const char *key)
{
    for (size_t i = 0; key && i < sizeof capabilities / sizeof *capabilities; ++i)
        if (!strcmp(capabilities[i].key, key)) return capabilities[i].revision;
    return 0;
}
size_t p4_capability_count(void) { return sizeof capabilities / sizeof *capabilities; }
const char *p4_capability_at(size_t index, unsigned *revision)
{
    if (index >= p4_capability_count()) return NULL;
    if (revision) *revision = capabilities[index].revision;
    return capabilities[index].key;
}
static int capability_key_char(char c, int first)
{
    return (c >= 'a' && c <= 'z') || (!first && ((c >= '0' && c <= '9') || c == '_' || c == '-'));
}
int p4_capability_parse(const char *text, size_t size, char *key, unsigned *revision)
{
    size_t length = 0;
    while (length < size && text[length] != '@') ++length;
    if (!length || length >= P4_CAPABILITY_KEY_MAX) return -1;
    for (size_t i = 0, first = 1; i < length; ++i) {
        if (text[i] == '.' && !first && i + 1 < length) { first = 1; continue; }
        if (!capability_key_char(text[i], (int)first)) return -1;
        first = 0;
    }
    unsigned long value = 1;
    if (length < size) {
        const char *digits = text + length + 1;
        size_t count = size - length - 1;
        if (!count || count > 9 || *digits == '0') return -1;
        value = 0;
        for (size_t i = 0; i < count; ++i) {
            if (digits[i] < '0' || digits[i] > '9') return -1;
            value = value * 10u + (unsigned)(digits[i] - '0');
        }
    }
    memcpy(key, text, length);
    key[length] = 0;
    *revision = (unsigned)value;
    return 0;
}
/* Visits each trimmed comma-separated entry; -1 on an empty or malformed entry. */
static int requires_each(const char *list, int (*visit)(const char *key, unsigned revision, void *user), void *user)
{
    for (const char *s = list;;) {
        while (*s == ' ' || *s == '\t') ++s;
        const char *end = s;
        while (*end && *end != ',') ++end;
        const char *last = end;
        while (last > s && (last[-1] == ' ' || last[-1] == '\t')) --last;
        char key[P4_CAPABILITY_KEY_MAX];
        unsigned revision;
        if (p4_capability_parse(s, (size_t)(last - s), key, &revision)) return -1;
        if (visit && visit(key, revision, user)) return 1;
        if (!*end) return 0;
        s = end + 1;
    }
}
int p4_capability_requires_valid(const char *list) { return list && requires_each(list, NULL, NULL) == 0 ? 0 : -1; }
typedef struct RequiresReason { char *text; size_t capacity; } RequiresReason;
static int requires_missing(const char *key, unsigned revision, void *user)
{
    RequiresReason *reason = user;
    unsigned provided = p4_capability_revision(key);
    if (provided >= revision) return 0;
    if (provided)
        snprintf(reason->text, reason->capacity,
                 "Requires capability %s@%u, but this build provides %s@%u. Update Persona4-PC or use an older version of the mod.",
                 key, revision, key, provided);
    else
        snprintf(reason->text, reason->capacity,
                 "Requires capability %s@%u, which this build does not provide. Update Persona4-PC or check the key's spelling.",
                 key, revision);
    return 1;
}
int p4_capability_requires_check(const char *list, char *reason, size_t capacity)
{
    RequiresReason state = {reason, capacity};
    if (capacity) reason[0] = 0;
    if (!list) return 0;
    int result = requires_each(list, requires_missing, &state);
    if (result < 0 && capacity) snprintf(reason, capacity, "Invalid requires= list");
    return result ? -1 : 0;
}
static int copy_value(char *dst, size_t cap, const char *value)
{
    size_t size = strlen(value);
    if (!size || size >= cap) return -1;
    for (size_t i = 0; i < size; ++i)
        if ((unsigned char)value[i] < 32 || (unsigned char)value[i] == 127) return -1;
    memcpy(dst, value, size + 1);
    return 0;
}
static void registry_release(P4PackageRegistry *registry);
static int registry_retain_storage(P4PackageRegistry *registry)
{
    unsigned count = atomic_load(&registry->references);
    do {
        if (!count || count == UINT_MAX) return -1;
    } while (!atomic_compare_exchange_weak(&registry->references, &count, count + 1));
    return 0;
}
static const P4PackageProvider *provider_data(const P4PackageProvider *provider)
{
    return provider->backing ? provider->backing : provider;
}
static void provider_destroy(P4PackageProvider *provider)
{
    if (provider->backing) { registry_release(provider->backing->registry); return; }
    p4_package_files_destroy(provider->index);
    p4_package_metadata_free(provider->fields, provider->field_capacity * sizeof *provider->fields);
    p4_package_metadata_free(provider->manifest, provider->manifest_bytes);
}
static void registry_release(P4PackageRegistry *registry)
{
    if (atomic_fetch_sub(&registry->references, 1) != 1) return;
    for (size_t i = 0; i < registry->count; ++i) provider_destroy(&registry->packages[i]);
    p4_package_metadata_free(registry, registry->allocation_bytes);
}
static void rollback_native(void)
{
    if (!native_dirty) return;
    native_dirty = 0;
    preparing_handler = HANDLER_LIMIT;
    for (size_t next = handler_order_count; next != 0; --next) {
        size_t i = handler_order[next - 1];
        if (handler_prepared[i] && handlers[i]->rollback)
            handlers[i]->rollback(handlers[i]->user);
        handler_prepared[i] = handler_completed[i] = handler_product_set[i] = 0;
        handler_products[i] = NULL;
    }
    for (P4PackageRegistry *registry = last; registry; registry = registry->previous) {
        if (registry->prepared && registry->client->rollback)
            registry->client->rollback(registry->client->user);
        registry->prepared = 0;
        for (size_t i = 0; i < registry->count; ++i) {
            P4PackageProvider *provider = &registry->packages[i];
            atomic_fetch_and(&provider->claims, P4_PACKAGE_CLAIM_FILES | P4_PACKAGE_CLAIM_SCRIPT);
            provider->require_cosmetic = 0;
        }
    }
    atomic_store(&source_tracking, 0);
}
static void abort_batch(void)
{
    rollback_native();
    p4_package_registry_retire();
}
static int field_key_compare(const void *left, const void *right)
{
    const P4PackageField *a = left, *b = right;
    int order = strcmp(a->section, b->section);
    if (!order) order = strcmp(a->argument, b->argument);
    return order ? order : strcmp(a->key, b->key);
}
static int field_compare(const void *left, const void *right)
{
    int order = field_key_compare(left, right);
    if (order) return order;
    const P4PackageField *a = left, *b = right;
    return (a->line > b->line) - (a->line < b->line);
}
static int append_field(P4PackageProvider *provider, const char *section, const char *argument,
                        const char *key, const char *value, unsigned line)
{
    if (provider->field_count == FIELD_LIMIT) return -1;
    if (provider->field_count == provider->field_capacity) {
        size_t capacity = provider->field_capacity ? provider->field_capacity * 2 : 16;
        P4PackageField *fields = p4_package_metadata_resize(provider->fields,
            provider->field_capacity * sizeof *fields, capacity * sizeof *fields);
        if (!fields) return -1;
        provider->fields = fields;
        provider->field_capacity = capacity;
    }
    provider->fields[provider->field_count++] = (P4PackageField){section, argument, key, value, line};
    return 0;
}
static int manifest(const P4PackageClient *client, const char *directory, P4PackageProvider *provider)
{
    char path[1200];
    provider->diagnostic = "Unreadable/nonregular manifest, over64KiB, or embedded NUL";
    provider->diagnostic_line = 0;
    if (join(path, sizeof path, directory, client->manifest_name) || !p4_os_is_file_nofollow(path)) return -1;
    FILE *file = p4_os_fopen_rb(path);
    if (!file) return -1;
    int failed = fseeko(file, 0, SEEK_END) != 0;
    off_t size = failed ? -1 : ftello(file);
    if (size < 0 || size > MANIFEST_LIMIT || fseeko(file, 0, SEEK_SET)) failed = 1;
    if (!failed) {
        provider->manifest_bytes = (size_t)size + 1;
        provider->manifest = p4_package_metadata_alloc(provider->manifest_bytes, 0);
        if (!provider->manifest || fread(provider->manifest, 1, (size_t)size, file) != (size_t)size ||
            ferror(file) || memchr(provider->manifest, 0, (size_t)size)) failed = 1;
        else provider->manifest[size] = 0;
    }
    if (fclose(file)) failed = 1;
    if (failed) goto fail;
    unsigned seen = 0;
    provider->diagnostic = "Invalid manifest section, key or structure";
    int metadata = 0, metadata_seen = 0, section_fields = 0;
    char *section = NULL, *argument = NULL;
    for (char *line = provider->manifest; *line;) {
        ++provider->diagnostic_line;
        char *next = strchr(line, '\n');
        if (next) *next++ = 0;
        size_t line_size = strlen(line);
        if (line_size >= 512) { provider->diagnostic = "Manifest line exceeds511 bytes"; goto fail; }
        char *s = trim(line);
        if (!*s || *s == '#' || *s == ';') goto next_line;
        for (const unsigned char *p = (const unsigned char *)s; *p; ++p)
            if ((*p < 32 && *p != '\t') || *p == 127) goto fail;
        if (*s == '[') {
            if (section && !metadata && !section_fields) {
                provider->diagnostic = "Empty extension section refused";
                goto fail;
            }
            section_fields = 0;
            size_t length = strlen(s);
            if (length < 3 || s[length - 1] != ']') goto fail;
            metadata = !strcmp(s, client->section);
            if (metadata && metadata_seen++) goto fail;
            s[length - 1] = 0;
            section = trim(s + 1);
            char *space = section;
            while (*space && !isspace((unsigned char)*space)) ++space;
            if (*space) { *space++ = 0; argument = trim(space); }
            else argument = space;
            if (!identifier(section)) goto fail;
            goto next_line;
        }
        char *eq = strchr(s, '=');
        if (!section || !eq) goto fail;
        *eq = 0;
        char *key = trim(s), *value = trim(eq + 1);
        if (!identifier(key)) goto fail;
        if (!metadata) {
            if (append_field(provider, section, argument, key, value, provider->diagnostic_line)) {
                provider->diagnostic = "Manifest exceeds2048 extension fields or allocation failed";
                goto fail;
            }
            section_fields = 1;
            goto next_line;
        }
        unsigned bit = 0;
        int result = -1;
        P4PackageInfo *info = &provider->info;
        if (!strcmp(key, "id")) { bit = 1; result = copy_value(info->id, sizeof info->id, value); }
        else if (!strcmp(key, "name")) { bit = 2; result = copy_value(info->name, sizeof info->name, value); }
        else if (!strcmp(key, "version")) { bit = 4; result = copy_value(info->version, sizeof info->version, value); }
        else if (!strcmp(key, "api")) { bit = 8; result = version(value, &info->api_major, &info->api_minor); }
        else if (!strcmp(key, "priority")) { bit = 16; result = number(value, &info->priority); }
        else if (!strcmp(key, "order")) { bit = 32; result = number(value, &info->order); }
        else if (!strcmp(key, "requires")) {
            bit = 64;
            if (!(seen & bit) && p4_capability_requires_valid(value)) {
                provider->diagnostic = "Invalid requires= list: use comma-separated capability keys such as events.ailment@1";
                goto fail;
            }
            provider->requires = value;
            result = 0;
        }
        if (seen & bit) { provider->diagnostic = "Duplicate metadata key"; goto fail; }
        if (result) {
            provider->diagnostic = !strcmp(key, "api") ? "Unsupported required API version (runtime" P4_PACKAGE_API_TEXT ")" :
                "Unknown metadata key or invalid bounded value";
            goto fail;
        }
        seen |= bit;
next_line:
        if (!next) break;
        line = next;
    }
    if (section && !metadata && !section_fields) {
        provider->diagnostic = "Empty extension section refused";
        goto fail;
    }
    if ((seen & 63) != 63 || !identifier(provider->info.id) || strlen(directory) >= sizeof provider->info.directory) {
        provider->diagnostic = "Require valid id/name/version/api/priority/order exactly once";
        goto fail;
    }
    strcpy(provider->info.directory, directory);
    if (provider->field_count > 1) p4_package_sort(provider->fields, provider->field_count, sizeof *provider->fields, field_compare);
    for (size_t i = 1; i < provider->field_count; ++i) {
        if (!field_key_compare(&provider->fields[i - 1], &provider->fields[i])) {
            provider->diagnostic = "Duplicate extension section/argument/key";
            provider->diagnostic_line = provider->fields[i].line;
            goto fail;
        }
    }
    provider->diagnostic = NULL;
    provider->diagnostic_line = 0;
    return 0;
fail:
    fprintf(stderr, "p4pc: %s: %s line%u: %s\n", client->diagnostic_name,
            path, provider->diagnostic_line, provider->diagnostic);
    return -1;
}
typedef struct ManifestProbe { const char *name; int present; } ManifestProbe;
typedef struct InstallationScan {
    const P4PackageClient *client;
    const char *root;
    P4PackageInstallationVisitor visitor;
    void *user;
    size_t count;
    int failed;
    enum { SCAN_SHIPPED, SCAN_PLAYER, SCAN_BUNDLED } pass;
    /* Ids (and the shipped versions) of the bundled census and of the valid
     * player installations: a player copy hides, and reports, its bundled id. */
    char (*shipped)[2][64], (*seen)[64];
    size_t shipped_count, shipped_capacity, seen_count, seen_capacity;
} InstallationScan;
static void probe_manifest(const char *name, void *user)
{
    ManifestProbe *probe = user;
    const char *expected = probe->name;
    while (*name && *expected && tolower((unsigned char)*name) == (unsigned char)*expected) { ++name; ++expected; }
    if (!*name && !*expected) probe->present = 1;
}
static void scan_installation(const char *name, void *user)
{
    InstallationScan *scan = user;
    char directory[1200];
    if (scan->failed || !strcmp(name, ".") || !strcmp(name, "..")) return;
    if (join(directory, sizeof directory, scan->root, name)) { scan->failed = 1; return; }
    if (!p4_os_is_dir_nofollow(directory)) return;
    if (++scan->count > 4096) {
        fprintf(stderr, "p4pc: packages: installed catalog exceeds4096 directories\n");
        scan->failed = 1;
        return;
    }
    P4PackageProvider provider = {0};
    ManifestProbe probe = {.name = scan->client->manifest_name};
    P4PackageInstallation installation = {.folder = name, .directory = directory};
    int valid = 0;
    if (p4_os_list_dir(directory, probe_manifest, &probe)) {
        installation.diagnostic = "Cannot enumerate installation directory";
    } else if (probe.present) {
        valid = manifest(scan->client, directory, &provider) == 0;
        if (!valid) {
            installation.diagnostic = provider.diagnostic;
            installation.line = provider.diagnostic_line;
        }
    } else if (scan->client->compendium) {
        valid = p4_compendium_manifest(directory, name, &provider.info, provider.files, sizeof provider.files) == 0;
        if (!valid) installation.diagnostic = "Unrecognized disc tree or invalid Mod.xml metadata";
        else { provider.info.api_major = 1; provider.info.api_minor = 0; }
    } else {
        installation.diagnostic = "No supported native package manifest";
    }
    char shadows[192] = {0};
    if (valid && scan->pass == SCAN_SHIPPED) {
        if (scan->shipped_count == scan->shipped_capacity) {
            size_t capacity = scan->shipped_capacity ? scan->shipped_capacity * 2u : 16u;
            char (*shipped)[2][64] = realloc(scan->shipped, capacity * sizeof *shipped);
            if (!shipped) { scan->failed = 1; provider_destroy(&provider); return; }
            scan->shipped = shipped; scan->shipped_capacity = capacity;
        }
        snprintf(scan->shipped[scan->shipped_count][0], 64, "%s", provider.info.id);
        snprintf(scan->shipped[scan->shipped_count++][1], 64, "%s", provider.info.version);
        provider_destroy(&provider);
        return;
    }
    if (!valid && scan->pass == SCAN_SHIPPED) { provider_destroy(&provider); return; }
    if (valid && scan->pass == SCAN_BUNDLED) {
        for (size_t i = 0; i < scan->seen_count; ++i)
            if (!strcmp(scan->seen[i], provider.info.id)) { provider_destroy(&provider); return; }
    }
    if (valid && scan->pass == SCAN_PLAYER) {
        if (scan->seen_count == scan->seen_capacity) {
            size_t capacity = scan->seen_capacity ? scan->seen_capacity * 2u : 16u;
            char (*seen)[64] = realloc(scan->seen, capacity * sizeof *seen);
            if (!seen) { scan->failed = 1; provider_destroy(&provider); return; }
            scan->seen = seen; scan->seen_capacity = capacity;
        }
        strcpy(scan->seen[scan->seen_count++], provider.info.id);
        for (size_t i = 0; i < scan->shipped_count; ++i) {
            if (strcmp(scan->shipped[i][0], provider.info.id)) continue;
            snprintf(shadows, sizeof shadows, "player copy shadows bundled %s %s", scan->shipped[i][0], scan->shipped[i][1]);
            fprintf(stderr, "p4pc: %s: %s\n", scan->client->diagnostic_name, shadows);
            installation.shadows = shadows;
        }
    }
    if (valid) {
        provider.info.kind = scan->client->kind;
        provider.info.ordinal = SIZE_MAX;
        installation.info = &provider.info;
        installation.fields = provider.fields;
        installation.field_count = provider.field_count;
    }
    installation.bundled = scan->pass == SCAN_BUNDLED;
    if (scan->visitor(&installation, scan->user)) scan->failed = 1;
    provider_destroy(&provider);
}
int p4_package_installations_scan_bundled(const P4PackageClient *client, const char *root, const char *bundled_root,
                                          P4PackageInstallationVisitor visitor, void *user)
{
    if (!client || !client->diagnostic_name || !client->manifest_name || !client->section ||
        (client->kind != P4_PACKAGE_MOD && client->kind != P4_PACKAGE_ENGINE_PACK) || !visitor) return -1;
    InstallationScan scan = {.client = client, .visitor = visitor, .user = user};
    int failed = 0;
    /* The bundled root is optional: a missing one shows nothing. */
    const int shipped = bundled_root && *bundled_root && p4_os_is_dir_nofollow(bundled_root);
    if (shipped) {
        scan.root = bundled_root;
        scan.pass = SCAN_SHIPPED;
        failed = p4_os_list_dir(bundled_root, scan_installation, &scan) || scan.failed;
        scan.count = 0;
    }
    if (!failed && root && *root) {
        if (!p4_os_is_dir_nofollow(root)) failed = 1;
        else {
            scan.root = root;
            scan.pass = SCAN_PLAYER;
            failed = p4_os_list_dir(root, scan_installation, &scan) || scan.failed;
        }
    }
    if (!failed && shipped) {
        scan.root = bundled_root;
        scan.pass = SCAN_BUNDLED;
        failed = p4_os_list_dir(bundled_root, scan_installation, &scan) || scan.failed;
    }
    free(scan.shipped);
    free(scan.seen);
    return failed ? -1 : 0;
}
int p4_package_installations_scan(const P4PackageClient *client, const char *root,
                                  P4PackageInstallationVisitor visitor, void *user)
{
    return p4_package_installations_scan_bundled(client, root, NULL, visitor, user);
}

/* Admission refusals of this process, so later messages can say why an enabled
 * id is absent. One slot per selector id; the newest reason wins. */
typedef struct Refusal { P4PackageKind kind; char id[64], reason[512]; } Refusal;
static Refusal refusals[2 * P4_PACKAGE_MAX];
static size_t refusal_count;
static void remember_refusal(P4PackageKind kind, const char *id, const char *reason)
{
    size_t i = 0;
    while (i < refusal_count && (refusals[i].kind != kind || strcmp(refusals[i].id, id))) ++i;
    if (i == refusal_count) {
        if (refusal_count == sizeof refusals / sizeof refusals[0]) return;
        ++refusal_count;
    }
    refusals[i].kind = kind;
    snprintf(refusals[i].id, sizeof refusals[i].id, "%s", id);
    snprintf(refusals[i].reason, sizeof refusals[i].reason, "%s", reason);
}
const char *p4_package_refusal(P4PackageKind kind, const char *id)
{
    for (size_t i = 0; id && i < refusal_count; ++i)
        if (refusals[i].kind == kind && !strcmp(refusals[i].id, id)) return refusals[i].reason;
    return NULL;
}
static int reject_id(const P4PackageClient *client, const char *id, const char *reason)
{
    fprintf(stderr, "p4pc: %s: refused %s: %s\n", client->diagnostic_name, id, reason);
    remember_refusal(client->kind, id, reason);
    return client->refuse ? client->refuse(id, reason, client->user) : -1;
}
static int reject_provider(P4PackageProvider *provider, const char *reason)
{
    if (atomic_load(&provider->refused)) return 0;
    if (!reason || !reason[0] || reject_id(provider->registry->client, provider->info.id, reason)) return -1;
    atomic_store(&provider->refused, 1);
    ++admission_serial;
    return 0;
}

static int requirement_fail(char *reason, size_t capacity, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    if (reason && capacity) vsnprintf(reason, capacity, format, args);
    va_end(args);
    return -1;
}
static int decimal(const char **cursor, unsigned *out)
{
    const char *s = *cursor;
    if (*s < '0' || *s > '9') return -1;
    unsigned long value = 0;
    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (unsigned long)(*s++ - '0');
        if (value > 0xffffu) return -1;
    }
    *out = (unsigned)value;
    *cursor = s;
    return 0;
}
int p4_package_version_satisfies(const char *version, unsigned major, unsigned minor)
{
    unsigned parts[3] = {0, 0, 0};
    const char *s = version;
    if (!s || decimal(&s, &parts[0])) return -1;
    for (int i = 1; i < 3 && *s == '.'; ++i) {
        ++s;
        if (decimal(&s, &parts[i])) return -1;
    }
    if (*s && *s != '-' && *s != '+') return -1;
    return parts[0] == major && parts[1] >= minor;
}
int p4_package_requirements(const P4PackageInfo *self, const P4PackageField *fields, size_t count,
                            P4PackageRequirement out[P4_PACKAGE_REQUIRES_MAX], size_t *found,
                            char *reason, size_t capacity)
{
    if (!self || !out || !found || (count && !fields)) return requirement_fail(reason, capacity, "Invalid requirement query");
    *found = 0;
    for (size_t first = 0; first < count;) {
        size_t last = first + 1;
        while (last < count && !strcmp(fields[first].section, fields[last].section) &&
               !strcmp(fields[first].argument, fields[last].argument)) ++last;
        const char *section = fields[first].section;
        int companion = !strcmp(section, "companion");
        if (!companion && strcmp(section, "require")) { first = last; continue; }
        const char *id = fields[first].argument, *what = companion ? "companion" : "requirement";
        unsigned line = fields[first].line;
        if (self->api_major == 1 && self->api_minor < 6u)
            return requirement_fail(reason, capacity, "Line%u: [%s] sections require api=1.6", line, section);
        if (!identifier(id))
            return requirement_fail(reason, capacity, "Line%u: invalid %s id [%s %.70s]", line, what, section, id);
        if (!strcmp(id, self->id))
            return requirement_fail(reason, capacity, companion ? "Line%u: %s cannot be its own companion"
                                                                : "Line%u: %s cannot require itself", line, id);
        for (size_t i = 0; i < *found; ++i)
            if (!strcmp(out[i].id, id))
                return requirement_fail(reason, capacity, "Line%u: %s is both required and a companion", line, id);
        if (*found == P4_PACKAGE_REQUIRES_MAX)
            return requirement_fail(reason, capacity, "More than %u requirements and companions", P4_PACKAGE_REQUIRES_MAX);
        P4PackageRequirement *need = &out[(*found)++];
        memset(need, 0, sizeof *need);
        strcpy(need->id, id);
        need->companion = companion;
        int versioned = 0;
        for (size_t i = first; i < last; ++i) {
            const P4PackageField *field = &fields[i];
            if (field->line < line) line = field->line;
            if (!strcmp(field->key, "version")) {
                const char *s = field->value;
                if (decimal(&s, &need->major) || *s++ != '.' || decimal(&s, &need->minor) || *s)
                    return requirement_fail(reason, capacity, "Line%u: %s %s version must be <major>.<minor>",
                                            field->line, what, id);
                versioned = 1;
            } else if (!companion && !strncmp(field->key, "setting.", 8)) {
                const char *key = field->key + 8;
                size_t key_size = strlen(key), value_size = strlen(field->value);
                if (!key_size || key_size >= sizeof need->settings[0].key ||
                    !value_size || value_size >= sizeof need->settings[0].value)
                    return requirement_fail(reason, capacity, "Line%u: requirement %s setting row is out of bounds",
                                            field->line, id);
                if (need->setting_count == P4_PACKAGE_REQUIRE_SETTINGS)
                    return requirement_fail(reason, capacity, "Line%u: requirement %s names more than %u settings",
                                            field->line, id, P4_PACKAGE_REQUIRE_SETTINGS);
                memcpy(need->settings[need->setting_count].key, key, key_size + 1);
                memcpy(need->settings[need->setting_count++].value, field->value, value_size + 1);
            } else {
                return requirement_fail(reason, capacity, "Line%u: %s %s has unknown field %.40s",
                                        field->line, what, id, field->key);
            }
        }
        if (!versioned)
            return requirement_fail(reason, capacity, "Line%u: %s %s needs version=<major>.<minor>", line, what, id);
        need->line = line;
        first = last;
    }
    return 0;
}
static int declares_setting(const P4PackageProvider *provider, const char *key)
{
    for (size_t i = 0; i < provider->field_count; ++i)
        if (!strcmp(provider->fields[i].section, "setting") && !strcmp(provider->fields[i].argument, key)) return 1;
    return 0;
}
/* Whether `dependent`'s manifest requires `id` (and names `setting`, when given). */
static int field_requires(const P4PackageProvider *dependent, const char *id, const char *setting)
{
    for (size_t i = 0; i < dependent->field_count; ++i) {
        const P4PackageField *field = &dependent->fields[i];
        if (strcmp(field->section, "require") || strcmp(field->argument, id)) continue;
        if (!setting || (!strncmp(field->key, "setting.", 8) && !strcmp(field->key + 8, setting))) return 1;
    }
    return 0;
}

static void discover(const char *name, void *user)
{
    Discovery *discovery = user;
    P4PackageRegistry *registry = discovery->registry;
    const P4PackageClient *client = registry->client;
    char path[1200];
    P4PackageProvider provider = {0};
    ManifestProbe probe = {.name = client->manifest_name};
    if (discovery->failed || !strcmp(name, ".") || !strcmp(name, "..")) return;
    if (join(path, sizeof path, discovery->root, name)) { discovery->failed = 1; return; }
    if (!p4_os_is_dir_nofollow(path)) return;
    if (p4_os_list_dir(path, probe_manifest, &probe)) return;
    if (probe.present) {
        if (manifest(client, path, &provider)) {
            for (size_t i = 0; i < discovery->requested; ++i)
                if (!strcmp(discovery->ids[i], provider.info.id)) discovery->reasons[i] = provider.diagnostic;
            goto discard;
        }
        if (client->mount_files && join(provider.files, sizeof provider.files, path, "files")) goto discard;
    } else {
        if (!client->compendium || p4_compendium_manifest(path, name, &provider.info,
                provider.files, sizeof provider.files)) goto discard;
        provider.file_flags = P4_PACKAGE_FILES_CASEFOLD | P4_PACKAGE_FILES_DISC_TREE;
        provider.info.api_major = 1;
        provider.info.api_minor = 0;
    }
    for (size_t i = 0; i < discovery->requested; ++i) {
        if (strcmp(discovery->ids[i], provider.info.id)) continue;
        /* A player installation hides, and reports, the bundled copy of the same id. */
        if (discovery->bundled && discovery->player[i]) {
            fprintf(stderr, "p4pc: %s: player copy shadows bundled %s %s\n", client->diagnostic_name,
                provider.info.id, provider.info.version);
            goto discard;
        }
        if (discovery->found[i]) {
            for (size_t entry = 0; entry < registry->count; ++entry) {
                P4PackageProvider *previous = &registry->packages[entry];
                if (strcmp(previous->info.id, provider.info.id)) continue;
                if (reject_provider(previous, "Duplicate installed packages declare this enabled id"))
                    discovery->failed = 1;
                break;
            }
            goto discard;
        }
        discovery->found[i] = 1;
        discovery->player[i] = !discovery->bundled;
        provider.registry = registry;
        provider.info.kind = client->kind;
        provider.info.ordinal = SIZE_MAX;
        provider.info.generation = generation;
        registry->packages[registry->count] = provider;
        atomic_init(&registry->packages[registry->count].claims, 0);
        ++registry->count;
        atomic_init(&registry->packages[registry->count - 1].refused, 0);
        return;
    }
discard:
    provider_destroy(&provider);
}
static int provider_compare(const void *left, const void *right)
{
    const P4PackageInfo *a = &((const P4PackageProvider *)left)->info;
    const P4PackageInfo *b = &((const P4PackageProvider *)right)->info;
    if (a->priority != b->priority) return a->priority < b->priority ? -1 : 1;
    if (a->order != b->order) return a->order < b->order ? -1 : 1;
    return strcmp(a->id, b->id);
}
static int catalog_compare(const void *left, const void *right)
{
    return provider_compare(*(const P4PackageProvider *const *)left, *(const P4PackageProvider *const *)right);
}
static int catalog_id_compare(const void *left, const void *right)
{
    const P4PackageProvider *a = *(const P4PackageProvider *const *)left;
    const P4PackageProvider *b = *(const P4PackageProvider *const *)right;
    return strcmp(a->info.id, b->info.id);
}
static int index_provider(P4PackageProvider *provider)
{
    P4PackageMemoryUsage memory = p4_packages_memory_usage();
    provider->index = p4_package_files_create(provider->info.directory, provider->info.id,
        P4_PACKAGE_FILES_CASEFOLD | P4_PACKAGE_FILES_PROVIDER, memory.limit);
    if (!provider->index) return -1;
    if (!provider->registry->client->mount_files) return 0;
    size_t root_size = strlen(provider->info.directory);
    const char *prefix;
    if (!strcmp(provider->files, provider->info.directory)) prefix = "";
    else if (!strncmp(provider->files, provider->info.directory, root_size) && provider->files[root_size] == '/')
        prefix = provider->files + root_size + 1;
    else return -1;
    int status = p4_package_file_view_init(&provider->view, provider->index, prefix, provider->file_flags);
    if (status < 0) return -1;
    if (status == 1) {
        /* Native code/content-only packages need no dummy files/ directory.
         * A present non-directory payload is not treated as absence. */
        if (p4_package_files_resolve(prefix, provider->index)) return -1;
        provider->files[0] = 0;
    }
    return 0;
}
int p4_package_registry_load(P4PackageRegistry **out, const P4PackageClient *client,
                             const char *root, const char *enabled)
{
    return p4_package_registry_load_bundled(out, client, root, NULL, enabled);
}
int p4_package_registry_load_bundled(P4PackageRegistry **out, const P4PackageClient *client,
                                     const char *root, const char *bundled_root, const char *enabled)
{
    if (phase || !out || *out || !client || !client->diagnostic_name || !client->manifest_name ||
        !client->section || (client->kind != P4_PACKAGE_MOD && client->kind != P4_PACKAGE_ENGINE_PACK)) return -1;
    if (!first) refusal_count = 0; /* a new staging batch */
    if (p4_package_startup_blocked || !enabled || !enabled[0]) return 0;
    for (P4PackageRegistry *existing = first; existing; existing = existing->next)
        if (existing->client->kind == client->kind) return -1;
    Discovery discovery = {.root = root};
    P4PackageRegistry *registry = NULL;
    while (*enabled) {
        while (isspace((unsigned char)*enabled)) ++enabled;
        const char *end = strchr(enabled, ',');
        if (!end) end = enabled + strlen(enabled);
        const char *trimmed = end;
        while (trimmed > enabled && isspace((unsigned char)trimmed[-1])) --trimmed;
        size_t size = (size_t)(trimmed - enabled);
        if (discovery.requested == P4_PACKAGE_MAX || !size || size >= sizeof discovery.ids[0]) goto fail;
        memcpy(discovery.ids[discovery.requested], enabled, size);
        discovery.ids[discovery.requested][size] = 0;
        if (!identifier(discovery.ids[discovery.requested])) goto fail;
        for (size_t i = 0; i < discovery.requested; ++i)
            if (!strcmp(discovery.ids[i], discovery.ids[discovery.requested])) goto fail;
        ++discovery.requested;
        if (!*end) break;
        enabled = end + 1;
        if (!*enabled) goto fail;
    }
    const int player_root = root && root[0] && p4_os_is_dir_nofollow(root);
    const int shipped_root = bundled_root && bundled_root[0] && p4_os_is_dir_nofollow(bundled_root);
    if ((!player_root && !shipped_root) || generation == UINT32_MAX) {
        for (size_t i = 0; i < discovery.requested; ++i)
            if (reject_id(client, discovery.ids[i], "The configured package root is unavailable")) goto fail;
        return 0;
    }
    size_t allocation_bytes = sizeof *registry + discovery.requested *
        (sizeof registry->packages[0] + sizeof registry->ordered[0]);
    registry = p4_package_metadata_alloc(allocation_bytes, 1);
    if (!registry) goto fail;
    registry->allocation_bytes = allocation_bytes;
    registry->ordered = (P4PackageProvider **)(registry->packages + discovery.requested);
    registry->client = client;
    registry->output = out;
    atomic_init(&registry->references, 1);
    atomic_init(&registry->retired, 0);
    ++generation;
    discovery.registry = registry;
    if (player_root && (p4_os_list_dir(root, discover, &discovery) || discovery.failed)) goto fail;
    if (shipped_root) {
        discovery.root = bundled_root;
        discovery.bundled = 1;
        if (p4_os_list_dir(bundled_root, discover, &discovery) || discovery.failed) goto fail;
    }
    for (size_t i = 0; i < discovery.requested; ++i) {
        if (!discovery.found[i]) {
            const char *reason = discovery.reasons[i] ? discovery.reasons[i] : "Enabled id not found, or its manifest was refused";
            if (reject_id(client, discovery.ids[i], reason)) goto fail;
        }
    }
    for (size_t i = 0; i < registry->count; ++i) {
        P4PackageProvider *provider = &registry->packages[i];
        if (atomic_load(&provider->refused)) continue;
        if (index_provider(provider)) {
            if (reject_provider(provider, "Unsafe, unreadable or over-budget package file inventory")) goto fail;
            p4_package_files_destroy(provider->index);
            provider->index = NULL;
            provider->view = (P4PackageFileView){0};
            continue;
        }
        registry->ordered[registry->ordered_count++] = provider;
    }
    registry->previous = last;
    if (last) last->next = registry; else first = registry;
    last = registry;
    *out = registry;
    return 0;
fail:
    fprintf(stderr, "p4pc: %s: package staging refused\n", client->diagnostic_name);
    if (registry) registry_release(registry);
    abort_batch();
    return -1;
}
int p4_package_registry_set_order(P4PackageRegistry *registry, const char *ids)
{
    char names[P4_PACKAGE_MAX][64];
    size_t count = 0;
    if (phase || !registry || !ids) return -1;
    while (*ids) {
        while (isspace((unsigned char)*ids)) ++ids;
        const char *end = strchr(ids, ',');
        if (!end) end = ids + strlen(ids);
        const char *trimmed = end;
        while (trimmed > ids && isspace((unsigned char)trimmed[-1])) --trimmed;
        size_t length = (size_t)(trimmed - ids);
        if (count == P4_PACKAGE_MAX || !length || length >= sizeof names[0]) return -1;
        memcpy(names[count], ids, length);
        names[count][length] = 0;
        if (!identifier(names[count])) return -1;
        for (size_t i = 0; i < count; ++i) if (!strcmp(names[i], names[count])) return -1;
        ++count;
        if (!*end) break;
        ids = end + 1;
        if (!*ids) return -1;
    }
    for (size_t i = 0; i < registry->count; ++i) {
        P4PackageProvider *provider = &registry->packages[i];
        provider->preference = 0;
        for (size_t j = 0; j < count; ++j)
            if (!strcmp(provider->info.id, names[j])) { provider->preference = (unsigned)j + 1; break; }
    }
    return 0;
}

int p4_package_handler_register(const P4PackageHandler *handler)
{
    if (phase || !handler || !handler->name || !identifier(handler->name) ||
        (!handler->prepare && !handler->finalize_prepare && !handler->runtime_prepare && !handler->runtime_finalize_prepare)) return -1;
    if (handler->boot_phase != P4_PACKAGE_BOOT_SCHEMA && handler->boot_phase != P4_PACKAGE_BOOT_TABLES &&
        handler->boot_phase != P4_PACKAGE_BOOT_RESOURCES && handler->boot_phase != P4_PACKAGE_BOOT_UI) return -1;
    if (!handler->sections && !handler->boot_phase) return -1;
    for (size_t i = 0; i < handler_count; ++i) {
        if (handlers[i] == handler) return 0;
        if (!strcmp(handlers[i]->name, handler->name)) return -1;
        if (handler->sections && handlers[i]->sections)
            for (const char *const *a = handler->sections; *a; ++a)
                for (const char *const *b = handlers[i]->sections; *b; ++b)
                    if (!strcmp(*a, *b)) return -1;
    }
    size_t sections = 0;
    if (handler->sections)
        for (const char *const *section = handler->sections; *section; ++section)
            if (++sections > 16 || !identifier(*section) || !strcmp(*section, "require") ||
                !strcmp(*section, "companion")) return -1;
    if (sections && !handler->prepare) return -1; /* payload checks cannot be deferred */
    if ((!sections && !handler->boot_phase) || handler_count == HANDLER_LIMIT) return -1;
    if (handler->dependencies) {
        size_t count = 0;
        for (const char *const *dependency = handler->dependencies; *dependency; ++dependency)
            if (++count > HANDLER_LIMIT || !identifier(*dependency)) return -1;
    }
    handlers[handler_count++] = handler;
    return 0;
}
static size_t handler_named(const char *name)
{
    for (size_t i = 0; i < handler_count; ++i)
        if (!strcmp(handlers[i]->name, name)) return i;
    return HANDLER_LIMIT;
}
static int order_handlers(void)
{
    unsigned char ordered[HANDLER_LIMIT] = {0};
    handler_order_count = 0;
    for (size_t i = 0; i < handler_count; ++i) {
        if (!handlers[i]->dependencies) continue;
        for (const char *const *name = handlers[i]->dependencies; *name; ++name) {
            size_t dependency = handler_named(*name);
            if (dependency == HANDLER_LIMIT || handlers[dependency]->boot_phase > handlers[i]->boot_phase) {
                fprintf(stderr, "p4pc: native boot provider %s has missing/backwards dependency %s\n", handlers[i]->name, *name);
                return -1;
            }
        }
    }
    while (handler_order_count != handler_count) {
        size_t best = HANDLER_LIMIT;
        for (size_t i = 0; i < handler_count; ++i) {
            if (ordered[i]) continue;
            int ready = 1;
            if (handlers[i]->dependencies)
                for (const char *const *name = handlers[i]->dependencies; *name; ++name) {
                    size_t dependency = handler_named(*name);
                    if (dependency >= HANDLER_LIMIT) return -1;
                    if (!ordered[dependency]) { ready = 0; break; }
                }
            if (ready && (best == HANDLER_LIMIT || handlers[i]->boot_phase < handlers[best]->boot_phase ||
                (handlers[i]->boot_phase == handlers[best]->boot_phase && strcmp(handlers[i]->name, handlers[best]->name) < 0)))
                best = i;
        }
        if (best == HANDLER_LIMIT) {
            fprintf(stderr, "p4pc: native boot provider dependency cycle\n");
            return -1;
        }
        ordered[best] = 1;
        handler_order[handler_order_count++] = best;
    }
    return 0;
}
static int product_error(void)
{
    if (preparing_handler != HANDLER_LIMIT) boot_error = 1;
    return -1;
}
int p4_package_boot_provide(const void *product)
{
    unsigned char *set = runtime_pass ? runtime_product_set : handler_product_set;
    const void **products = runtime_pass ? runtime_products : handler_products;
    if (preparing_handler == HANDLER_LIMIT || set[preparing_handler]) return product_error();
    set[preparing_handler] = 1;
    products[preparing_handler] = product;
    return 0;
}
int p4_package_boot_product(const char *dependency, const void **out)
{
    if (out) *out = NULL;
    if (preparing_handler == HANDLER_LIMIT || !dependency || !out) return product_error();
    const char *const *names = handlers[preparing_handler]->dependencies;
    const unsigned char *completed = runtime_pass ? runtime_completed : handler_completed;
    const unsigned char *set = runtime_pass ? runtime_product_set : handler_product_set;
    const void *const *products = runtime_pass ? runtime_products : handler_products;
    if (names) for (; *names; ++names) {
        if (strcmp(*names, dependency)) continue;
        size_t index = handler_named(dependency);
        if (index == HANDLER_LIMIT || !completed[index] || !set[index]) return product_error();
        *out = products[index];
        return 0;
    }
    return product_error();
}
static int claim_compare(const void *left, const void *right)
{
    const FileClaim *a = left, *b = right;
    int order = p4_package_key_compare(a->key, b->key);
    if (order) return order;
    return a->provider->info.ordinal < b->provider->info.ordinal ? -1 :
        a->provider->info.ordinal != b->provider->info.ordinal;
}
static int raw_conflicts(void)
{
    size_t capacity = 0, used = 0;
    for (size_t i = 0; i < catalog_count; ++i) {
        size_t count = catalog[i]->view.count;
        if (count > CLAIM_BUDGET / sizeof(FileClaim) - capacity) return -1;
        capacity += count;
    }
    FileClaim *claims = capacity ? p4_package_metadata_alloc(capacity * sizeof *claims, 0) : NULL;
    if (capacity && !claims) return -1;
    for (size_t i = 0; i < catalog_count; ++i) {
        P4PackageProvider *provider = catalog[i];
        if (!provider->view.files) continue;
        for (size_t entry = 0; entry < p4_package_files_count(provider->index); ++entry) {
            const char *key;
            int directory;
            int result = p4_package_file_view_entry(&provider->view, entry, &key, &directory);
            if (result < 0) { p4_package_metadata_free(claims, capacity * sizeof *claims); return -1; }
            if (!result && !directory) {
                atomic_fetch_or(&provider->claims, P4_PACKAGE_CLAIM_FILES);
                claims[used++] = (FileClaim){key, provider};
            }
        }
    }
    if (used > 1) p4_package_sort(claims, used, sizeof *claims, claim_compare);
    for (size_t start = 0; start < used;) {
        size_t end = start + 1;
        while (end < used && !p4_package_key_compare(claims[start].key, claims[end].key)) ++end;
        if (end > start + 1) {
            char canonical[512];
            size_t size = strlen(claims[start].key);
            for (size_t i = 0; i <= size; ++i) {
                unsigned char c = (unsigned char)claims[start].key[i];
                canonical[i] = (char)(c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c);
            }
            for (size_t a = start; a < end; ++a) for (size_t b = a + 1; b < end; ++b) {
                P4PackageRegistry *x = claims[a].provider->registry, *y = claims[b].provider->registry;
                ++x->conflicts;
                if (x != y) ++y->conflicts;
                fprintf(stderr, "p4pc: packages: file conflict %s: %s < %s (winner)\n", canonical,
                        claims[a].provider->info.id, claims[b].provider->info.id);
            }
        }
        start = end;
    }
    p4_package_metadata_free(claims, capacity * sizeof *claims);
    return 0;
}
static int renew_candidate(void)
{
    if (generation == UINT32_MAX) return -1;
    P4PackageRegistry *next_first = NULL, *next_last = NULL;
    for (P4PackageRegistry *source = first; source; source = source->next) {
        size_t survivors = 0;
        for (size_t i = 0; i < source->count; ++i)
            survivors += !atomic_load(&source->packages[i].refused);
        size_t bytes = sizeof(P4PackageRegistry) + survivors *
            (sizeof(P4PackageProvider) + sizeof(P4PackageProvider *));
        P4PackageRegistry *replacement = p4_package_metadata_alloc(bytes, 1);
        if (!replacement) goto fail;
        replacement->allocation_bytes = bytes;
        replacement->client = source->client;
        replacement->ordered = (P4PackageProvider **)(replacement->packages + survivors);
        atomic_init(&replacement->references, 1);
        atomic_init(&replacement->retired, 0);
        replacement->previous = next_last;
        if (next_last) next_last->next = replacement; else next_first = replacement;
        next_last = replacement;
        for (size_t i = 0; i < source->count; ++i) {
            const P4PackageProvider *original = &source->packages[i];
            if (atomic_load(&original->refused)) continue;
            const P4PackageProvider *data = provider_data(original);
            /* Existing candidate ownership authorizes a new immutable storage
             * view. This is not a new I/O/work retain on a retired provider. */
            if (registry_retain_storage(data->registry)) goto fail;
            P4PackageProvider *copy = &replacement->packages[replacement->count++];
            copy->registry = replacement;
            copy->backing = data;
            copy->info = original->info;
            copy->preference = original->preference;
            copy->info.ordinal = SIZE_MAX;
            copy->info.generation = generation + 1;
            atomic_init(&copy->claims, atomic_load(&original->claims) &
                (P4_PACKAGE_CLAIM_FILES | P4_PACKAGE_CLAIM_SCRIPT));
            atomic_init(&copy->refused, 0);
            copy->file_flags = data->file_flags;
            copy->index = data->index;
            copy->view = data->view;
            copy->manifest = data->manifest;
            copy->fields = data->fields;
            copy->field_count = data->field_count;
            copy->field_capacity = data->field_capacity;
            copy->manifest_bytes = data->manifest_bytes;
            copy->requires = data->requires;
        }
    }
    P4PackageRegistry *old = first, *replacement = next_first;
    while (old) {
        P4PackageRegistry *next = old->next;
        replacement->output = old->output;
        replacement->preflight_prepared = old->preflight_prepared;
        old->preflight_prepared = 0; /* compiled client state transfers, not reruns */
        old->output = NULL;
        if (replacement->output) *replacement->output = replacement;
        atomic_store(&old->retired, 1);
        registry_release(old); /* independent jobs/pins keep this snapshot intact */
        old = next;
        replacement = replacement->next;
    }
    first = next_first;
    last = next_last;
    ++generation;
    return 0;
fail:
    while (next_first) {
        P4PackageRegistry *next = next_first->next;
        registry_release(next_first);
        next_first = next;
    }
    return -1;
}
/* Requirements are admission metadata. Each requirement of a catalog package
 * must be a selected package of the same kind whose version satisfies the
 * minimum and which declares the named settings (their values belong to the
 * mod-settings handler). Requirements then move before their dependents inside
 * that kind's catalog slots, otherwise keeping the selector/manifest order, and
 * a cycle refuses its members. Refusals cascade through later rebuilds.
 * 0 admitted, 1 someone was refused (rebuild), -1 strict failure. */
static int admit_requirements(void)
{
    enum { LIMIT = 2 * P4_PACKAGE_MAX };
    if (catalog_count > LIMIT) return -1;
    unsigned char edges[LIMIT][P4_PACKAGE_REQUIRES_MAX], edge_count[LIMIT] = {0};
    int refused = 0;
    for (size_t i = 0; i < catalog_count; ++i) {
        P4PackageProvider *provider = catalog[i];
        P4PackageRequirement requires[P4_PACKAGE_REQUIRES_MAX];
        size_t count = 0;
        char reason[512] = "";
        if (p4_package_requirements(&provider->info, provider->fields, provider->field_count,
                                    requires, &count, reason, sizeof reason) && !reason[0])
            strcpy(reason, "Invalid requirements");
        for (size_t r = 0; r < count && !reason[0]; ++r) {
            const P4PackageRequirement *need = &requires[r];
            size_t j = 0;
            while (j < catalog_count &&
                   (catalog[j]->info.kind != provider->info.kind || strcmp(catalog[j]->info.id, need->id))) ++j;
            if (j == catalog_count && need->companion) continue; /* an absent companion is fine */
            if (j == catalog_count) {
                const char *why = p4_package_refusal(provider->info.kind, need->id);
                if (why)
                    snprintf(reason, sizeof reason, "%s needs %s %u.%u+, which was refused: %s",
                             provider->info.name, need->id, need->major, need->minor, why);
                else
                    snprintf(reason, sizeof reason, "%s needs %s %u.%u+, which is not enabled",
                             provider->info.name, need->id, need->major, need->minor);
                break;
            }
            const P4PackageInfo *have = &catalog[j]->info;
            const char *role = need->companion ? "companion " : "";
            int satisfied = p4_package_version_satisfies(have->version, need->major, need->minor);
            if (satisfied <= 0) {
                snprintf(reason, sizeof reason, satisfied ? "%s needs %s%s %u.%u+, but its version %s is not comparable"
                                                          : "%s needs %s%s %u.%u+ (enabled: %s)",
                         provider->info.name, role, have->name, need->major, need->minor, have->version);
                break;
            }
            for (size_t s = 0; s < need->setting_count && !reason[0]; ++s)
                if (!declares_setting(catalog[j], need->settings[s].key))
                    snprintf(reason, sizeof reason, "%s needs %s setting %s, which it does not declare",
                             provider->info.name, have->name, need->settings[s].key);
            if (!reason[0]) edges[i][edge_count[i]++] = (unsigned char)j;
        }
        if (reason[0]) {
            if (reject_provider(provider, reason)) return -1;
            refused = 1;
        }
    }
    if (refused) return 1;
    /* Kahn's algorithm, always taking the earliest ready catalog position. */
    unsigned char emitted[LIMIT] = {0};
    P4PackageProvider *sorted[LIMIT];
    size_t used = 0;
    for (;;) {
        size_t pick = catalog_count;
        for (size_t i = 0; i < catalog_count && pick == catalog_count; ++i) {
            if (emitted[i]) continue;
            int ready = 1;
            for (unsigned e = 0; e < edge_count[i]; ++e)
                if (!emitted[edges[i][e]]) ready = 0;
            if (ready) pick = i;
        }
        if (pick == catalog_count) break;
        emitted[pick] = 1;
        sorted[used++] = catalog[pick];
    }
    if (used < catalog_count) {
        /* Stuck: every remaining package waits on another remaining one, so some
         * sit on cycles. Refuse exactly those; their dependents cascade. */
        for (size_t i = 0; i < catalog_count; ++i) {
            if (emitted[i]) continue;
            unsigned char seen[LIMIT] = {0};
            size_t stack[LIMIT], depth = 0, through = catalog_count;
            for (unsigned e = 0; e < edge_count[i]; ++e)
                if (!emitted[edges[i][e]] && !seen[edges[i][e]]) { seen[edges[i][e]] = 1; stack[depth++] = edges[i][e]; }
            while (depth && !seen[i]) {
                size_t node = stack[--depth];
                for (unsigned e = 0; e < edge_count[node]; ++e) {
                    size_t next = edges[node][e];
                    if (emitted[next] || seen[next]) continue;
                    seen[next] = 1;
                    stack[depth++] = next;
                }
            }
            if (!seen[i]) continue;
            for (unsigned e = 0; e < edge_count[i] && through == catalog_count; ++e)
                if (!emitted[edges[i][e]]) through = edges[i][e];
            char reason[512];
            snprintf(reason, sizeof reason, "%s is in a requirement cycle through %s",
                     catalog[i]->info.name, catalog[through]->info.name);
            if (reject_provider(catalog[i], reason)) return -1;
            refused = 1;
        }
        return refused ? 1 : -1;
    }
    /* Each kind keeps its own catalog slots; only its members are reordered. */
    for (int kind = P4_PACKAGE_MOD; kind <= P4_PACKAGE_ENGINE_PACK; ++kind) {
        size_t next = 0;
        for (size_t slot = 0; slot < catalog_count; ++slot) {
            if ((int)catalog[slot]->info.kind != kind) continue;
            while ((int)sorted[next]->info.kind != kind) ++next;
            catalog[slot] = sorted[next++];
        }
    }
    return 0;
}
const P4PackageProvider *p4_packages_requirer(const P4PackageProvider *provider, const char *setting)
{
    int state = atomic_load(&phase);
    if (!provider || (state != 1 && state != 2)) return NULL;
    for (size_t i = 0; i < catalog_count; ++i) {
        const P4PackageProvider *other = catalog[i];
        if (other != provider && other->info.kind == provider->info.kind &&
            field_requires(other, provider->info.id, setting)) return other;
    }
    return NULL;
}
int p4_packages_companion_admitted(const P4PackageProvider *provider, const char *id)
{
    int state = atomic_load(&phase);
    if (!provider || !id || (state != 1 && state != 2)) return -1;
    int declared = 0;
    for (size_t i = 0; i < provider->field_count && !declared; ++i)
        declared = !strcmp(provider->fields[i].section, "companion") && !strcmp(provider->fields[i].argument, id);
    if (!declared) return -1;
    for (size_t i = 0; i < catalog_count; ++i)
        if (catalog[i]->info.kind == provider->info.kind && !strcmp(catalog[i]->info.id, id)) return 1;
    return 0;
}

int p4_package_registry_publish(void)
{
    if (phase) return -1;
    phase = 1;
    if (!first && !handler_count) { phase = 2; return 0; }
    unsigned char used_handlers[HANDLER_LIMIT];
    size_t attempt_serial, candidate_serial = admission_serial;
    int order_valid = order_handlers() == 0;
rebuild:
    if (!order_valid) goto fail;
    if (admission_serial != candidate_serial) {
        rollback_native();
        if (renew_candidate()) goto fail;
        candidate_serial = admission_serial;
    }
    catalog_count = 0;
    for (P4PackageRegistry *registry = first; registry; registry = registry->next) {
        registry->ordered_count = registry->conflicts = 0;
        for (size_t i = 0; i < registry->count; ++i) {
            P4PackageProvider *provider = &registry->packages[i];
            if (!atomic_load(&provider->refused)) { ++catalog_count; continue; }
        }
    }
    if (catalog_count && !catalog) {
        catalog_capacity = catalog_count;
        catalog = p4_package_metadata_alloc(catalog_capacity * sizeof *catalog, 0);
        if (!catalog) { catalog_capacity = 0; goto fail; }
    }
    size_t next = 0;
    for (P4PackageRegistry *registry = first; registry; registry = registry->next)
        for (size_t i = 0; i < registry->count; ++i)
            if (!atomic_load(&registry->packages[i].refused)) catalog[next++] = &registry->packages[i];
    p4_package_sort(catalog, catalog_count, sizeof *catalog, catalog_id_compare);
    for (size_t i = 1; i < catalog_count; ++i) {
        if (strcmp(catalog[i - 1]->info.id, catalog[i]->info.id)) continue;
        P4PackageProvider *provider = catalog[i - 1]->info.kind == P4_PACKAGE_MOD ? catalog[i - 1] : catalog[i];
        if (reject_provider(provider, "Package id conflicts with another configured selector")) goto fail;
        goto rebuild;
    }
    p4_package_sort(catalog, catalog_count, sizeof *catalog, catalog_compare);
    /* Explicit selector order permutes only that selector's positions in the
     * shared default catalog, keeping other selectors in their original slots. */
    for (size_t i = 0; i < catalog_count; ++i) {
        P4PackageProvider *item = catalog[i];
        if (!item->preference) continue;
        size_t destination = i;
        for (size_t j = 0; j < i; ++j) {
            P4PackageProvider *other = catalog[j];
            if (other->info.kind == item->info.kind &&
                (!other->preference || other->preference > item->preference)) {
                destination = j;
                break;
            }
        }
        if (destination == i) continue;
        for (size_t j = i; j > destination; --j) {
            size_t previous = j;
            while (previous > destination && catalog[previous - 1]->info.kind != item->info.kind) --previous;
            if (previous > destination) {
                catalog[j] = catalog[previous - 1];
                j = previous;
            }
        }
        catalog[destination] = item;
    }
    int requirements = admit_requirements();
    if (requirements < 0) goto fail;
    if (requirements) goto rebuild;
    for (size_t i = 0; i < handler_count; ++i)
        used_handlers[i] = handlers[i]->boot_phase != P4_PACKAGE_BOOT_SCHEMA;
    for (size_t i = 0; i < catalog_count; ++i) {
        P4PackageProvider *provider = catalog[i];
        provider->info.ordinal = i;
        provider->registry->ordered[provider->registry->ordered_count++] = provider;
    }
    for (size_t i = 0; i < catalog_count; ++i) {
        P4PackageProvider *provider = catalog[i];
        char missing[256];
        if (p4_capability_requires_check(provider->requires, missing, sizeof missing)) {
            if (reject_provider(provider, missing)) goto fail;
            goto rebuild;
        }
        if (p4_package_provider_has_file(provider, "main.luau")) {
            if (provider->info.kind == P4_PACKAGE_ENGINE_PACK) {
                if (reject_provider(provider, "Engine packs cannot contain a player main.luau")) goto fail;
                goto rebuild;
            }
            atomic_fetch_or(&provider->claims, P4_PACKAGE_CLAIM_SCRIPT);
        }
        if (provider->field_count && provider->info.api_minor < 1) {
            if (reject_provider(provider, "Native extension fields require api=1.1")) goto fail;
            goto rebuild;
        }
        for (size_t entry = 0; entry < provider->field_count; ++entry) {
            const P4PackageField *field = &provider->fields[entry];
            int found = !strcmp(field->section, "require") || !strcmp(field->section, "companion"); /* the core's own */
            for (size_t h = 0; h < handler_count; ++h) {
                if (!handlers[h]->sections) continue;
                for (const char *const *section = handlers[h]->sections; *section; ++section)
                    if (!strcmp(field->section, *section)) { used_handlers[h] = 1; found = 1; }
            }
            if (!found) {
                char reason[512];
                snprintf(reason, sizeof reason, "Line%u: unavailable native capability [%s %s]",
                         field->line, field->section, field->argument);
                if (reject_provider(provider, reason)) goto fail;
                goto rebuild;
            }
        }
    }
    for (size_t next = handler_order_count; next != 0; --next) {
        size_t i = handler_order[next - 1];
        if (!used_handlers[i] || !handlers[i]->dependencies) continue;
        for (const char *const *name = handlers[i]->dependencies; *name; ++name) {
            size_t dependency = handler_named(*name);
            if (dependency >= HANDLER_LIMIT) goto fail;
            used_handlers[dependency] = 1;
        }
    }
    if (raw_conflicts()) goto fail;
    attempt_serial = admission_serial;
    for (size_t next = 0; next < handler_order_count; ++next) {
        size_t i = handler_order[next];
        if (!used_handlers[i] || !handlers[i]->preflight) continue;
        native_dirty = 1;
        handler_prepared[i] = 1; /* rollback also owns partial descriptor staging */
        if (handlers[i]->preflight(handlers[i]->user)) {
            if (admission_serial != attempt_serial) goto rebuild;
            goto fail;
        }
    }
    if (admission_serial != attempt_serial) goto rebuild;
    for (P4PackageRegistry *registry = first; registry; registry = registry->next) {
        if (!registry->client->preflight) continue;
        registry->preflight_prepared = 1;
        if (registry->client->preflight(registry, registry->client->user)) {
            if (admission_serial != attempt_serial) goto rebuild;
            goto fail;
        }
    }
    if (admission_serial != attempt_serial) goto rebuild;
    for (P4PackageRegistry *registry = first; registry; registry = registry->next) {
        if (!registry->client->prepare) continue;
        registry->prepared = 1;
        native_dirty = 1;
        if (registry->client->prepare(registry, registry->client->user)) goto prepared_fail;
        if (admission_serial != attempt_serial) goto prepared_fail;
    }
    for (size_t next = 0; next < handler_order_count; ++next) {
        size_t i = handler_order[next];
        if (!used_handlers[i]) continue;
        handler_prepared[i] = 1;
        native_dirty = 1;
        preparing_handler = i;
        boot_error = 0;
        int result = handlers[i]->prepare ? handlers[i]->prepare(handlers[i]->user) : 0;
        preparing_handler = HANDLER_LIMIT;
        if (result || boot_error || admission_serial != attempt_serial) goto prepared_fail;
        handler_completed[i] = 1;
    }
    for (size_t next = 0; next < handler_order_count; ++next) {
        size_t i = handler_order[next];
        if (!used_handlers[i] || !handlers[i]->finalize_prepare) continue;
        preparing_handler = i;
        boot_error = 0;
        int result = handlers[i]->finalize_prepare(handlers[i]->user);
        preparing_handler = HANDLER_LIMIT;
        if (result || boot_error || admission_serial != attempt_serial) goto prepared_fail;
    }
    const uint32_t gameplay = P4_PACKAGE_CLAIM_FILES | P4_PACKAGE_CLAIM_SCRIPT |
        P4_PACKAGE_CLAIM_SEMANTIC | P4_PACKAGE_CLAIM_TABLES | P4_PACKAGE_CLAIM_NATIVE;
    for (size_t i = 0; i < catalog_count; ++i) {
        P4PackageProvider *provider = catalog[i];
        uint32_t claims = p4_package_provider_claims(provider);
        if (provider->require_cosmetic && ((claims & gameplay) || !(claims & P4_PACKAGE_CLAIM_COSMETIC))) {
            (void)reject_provider(provider, "Cosmetic-only declaration conflicts with actual gameplay claims");
            goto prepared_fail;
        }
    }
    /* No mod code has run. Only this final globally consistent admission pass
     * may expose raw files or decoded native generations. */
    for (size_t i = 0; i < catalog_count; ++i) {
        P4PackageProvider *provider = catalog[i];
        if (!provider->view.files) continue;
        if (vfs_override_register(provider_data(provider)->files, provider->info.id, &p4_package_file_view_vfs, &provider->view)) {
            vfs_override_clear();
            goto fail;
        }
    }
    for (P4PackageRegistry *registry = first; registry; registry = registry->next)
        if (registry->client->publish) registry->client->publish(registry, registry->client->user);
    for (size_t next = 0; next < handler_order_count; ++next) {
        size_t i = handler_order[next];
        if (handler_prepared[i] && handlers[i]->publish) handlers[i]->publish(handlers[i]->user);
    }
    for (size_t i = 0; i < catalog_count; ++i) {
        const P4PackageInfo *info = &catalog[i]->info;
        fprintf(stderr, "p4pc: %s: enabled %s %s (priority %d, order %d)\n",
                catalog[i]->registry->client->diagnostic_name, info->id, info->version, info->priority, info->order);
    }
    memcpy(handler_selected, used_handlers, handler_count);
    phase = 2;
    return 0;
prepared_fail:
    if (admission_serial != attempt_serial) {
        goto rebuild;
    }
fail:
    rollback_native();
    int quarantined = 0;
    for (P4PackageRegistry *registry = first; registry; registry = registry->next) {
        if (registry->client->kind != P4_PACKAGE_MOD) continue;
        for (size_t i = 0; i < registry->count; ++i) {
            P4PackageProvider *provider = &registry->packages[i];
            if (atomic_load(&provider->refused)) continue;
            if (!reject_provider(provider, "The selected mod transaction could not be prepared; no mod code has run"))
                quarantined = 1;
        }
    }
    if (quarantined) goto rebuild;
    fprintf(stderr, "p4pc: packages: activation refused; no partial package batch published\n");
    p4_package_registry_retire();
    return -1;
}
int p4_package_boot_fail(const P4PackageProvider *provider, const char *reason)
{
    if (!runtime_pass || preparing_handler == HANDLER_LIMIT) return product_error();
    if (provider && !p4_package_provider_selected(provider)) return product_error();
    if (!runtime_failure_reason[0]) {
        runtime_failure_provider = provider;
        snprintf(runtime_failure_reason, sizeof runtime_failure_reason, "%s",
                 reason && reason[0] ? reason : "Native runtime preparation failed");
    }
    boot_error = 1;
    return -1;
}
int p4_package_registry_runtime_materialize(const P4PackageProvider **failed, const char **reason)
{
    if (failed) *failed = NULL;
    if (reason) *reason = NULL;
    if (runtime_state == 2) return 1;
    if (phase != 2 || runtime_state) {
        if (failed) *failed = runtime_failure_provider;
        if (reason) *reason = runtime_failure_reason[0] ? runtime_failure_reason : "Native package runtime is not available";
        return -1;
    }
    runtime_state = 1;
    runtime_pass = 1;
    for (unsigned finalizing = 0; finalizing < 2; ++finalizing) {
        for (size_t next = 0; next < handler_order_count; ++next) {
            size_t i = handler_order[next];
            if (!handler_selected[i]) continue;
            int (*callback)(void *) = finalizing ? handlers[i]->runtime_finalize_prepare : handlers[i]->runtime_prepare;
            if (!callback) {
                if (!finalizing) runtime_completed[i] = 1;
                continue;
            }
            preparing_handler = i;
            boot_error = 0;
            int result = callback(handlers[i]->user);
            preparing_handler = HANDLER_LIMIT;
            if (result || boot_error) {
                if (!runtime_failure_reason[0])
                    snprintf(runtime_failure_reason, sizeof runtime_failure_reason,
                             "Native provider %s failed during runtime %s (%d)",
                             handlers[i]->name, finalizing ? "finalization" : "preparation", result);
                runtime_pass = 0;
                runtime_state = 3;
                if (failed) *failed = runtime_failure_provider;
                if (reason) *reason = runtime_failure_reason;
                /* Raw files were exposed. Never run early rollback, candidate
                 * renewal, VM destruction or aggregate-handle release here. */
                return -1;
            }
            if (!finalizing) runtime_completed[i] = 1;
        }
    }
    runtime_pass = 0;
    for (size_t next = 0; next < handler_order_count; ++next) {
        size_t i = handler_order[next];
        if (handler_selected[i] && handlers[i]->runtime_publish)
            handlers[i]->runtime_publish(handlers[i]->user);
    }
    runtime_state = 2;
    return 0;
}
void p4_package_registry_retire(void)
{
    phase = 3;
    atomic_store(&source_tracking, 2u);
    for (P4PackageRegistry *entry = last; entry; entry = entry->previous) {
        if (entry->preflight_prepared && entry->client->preflight_rollback)
            entry->client->preflight_rollback(entry->client->user);
        entry->preflight_prepared = 0;
    }
    p4_package_metadata_free(catalog, catalog_capacity * sizeof *catalog);
    catalog = NULL;
    catalog_count = catalog_capacity = 0;
    P4PackageRegistry *registry = first;
    first = last = NULL;
    while (registry) {
        P4PackageRegistry *next = registry->next;
        if (registry->output) *registry->output = NULL;
        atomic_store(&registry->retired, 1);
        registry_release(registry);
        registry = next;
    }
}
size_t p4_package_registry_count(const P4PackageRegistry *registry) { return registry ? registry->ordered_count : 0; }
const P4PackageInfo *p4_package_registry_info(const P4PackageRegistry *registry, size_t index)
{
    const P4PackageProvider *provider = p4_package_registry_provider(registry, index);
    return provider ? &provider->info : NULL;
}
size_t p4_package_registry_file_conflicts(const P4PackageRegistry *registry) { return registry ? registry->conflicts : 0; }
size_t p4_packages_count(void) { return catalog_count; }
int p4_mod_witness_enabled(void)
{
    const char *value = getenv("P4PC_MOD_WITNESS");
    return value && !strcmp(value, "1");
}
int p4_packages_automation_requested(void)
{
    return getenv("P4PC_INPUT")||getenv("P4PC_LOCKSTEP_DIR")||getenv("P4PC_STATE_DUMP")||
        getenv("P4PC_CAPTURE")||getenv("P4PC_OFFSCREEN")||getenv("P4PC_HIDDEN_WINDOW");
}
int p4_mod_witness_validate(void)
{
    const char *value = getenv("P4PC_MOD_WITNESS");
    if (!value) return 0;
    const char *input = getenv("P4PC_INPUT");
    if (strcmp(value, "1") || !input || strcmp(input, "script")) {
        fprintf(stderr, "p4pc: P4PC_MOD_WITNESS accepts only1 with P4PC_INPUT=script; this is not a retail/fidelity run\n");
        return -1;
    }
    return 0;
}
void p4_packages_assert_disabled(void)
{
    if (getenv("P4PC_MOD_WITNESS") != NULL) {
        fprintf(stderr, "p4pc: fidelity harness refuses P4PC_MOD_WITNESS, including with both selectors disabled\n");
        abort();
    }
    for (const P4PackageRegistry *registry = first; registry; registry = registry->next) {
        if (!registry->count) continue;
        fprintf(stderr, "p4pc: fidelity requires P4PC_MODS=0 and P4PC_PACKS=0; %s selector still contains %zu package(s), first=%s\n",
            registry->client->diagnostic_name, registry->count, registry->packages[0].info.id);
        abort();
    }
}
void p4_mod_fidelity_check_start(void)
{
    if (p4_mod_witness_validate()) abort();
    if (!p4_mod_witness_enabled()) p4_packages_assert_disabled();
}
const P4PackageInfo *p4_packages_info(size_t ordinal) { return ordinal < catalog_count ? &catalog[ordinal]->info : NULL; }
const P4PackageProvider *p4_packages_provider(size_t ordinal) { return ordinal < catalog_count ? catalog[ordinal] : NULL; }
const P4PackageProvider *p4_package_registry_provider(const P4PackageRegistry *registry, size_t index)
{
    return registry && index < registry->ordered_count ? registry->ordered[index] : NULL;
}
const P4PackageField *p4_package_provider_fields(const P4PackageProvider *provider, size_t *count)
{
    if (!count) return NULL;
    *count = provider ? provider->field_count : 0;
    return provider ? provider->fields : NULL;
}
const P4PackageInfo *p4_package_provider_info(const P4PackageProvider *provider) { return provider ? &provider->info : NULL; }
int p4_package_provider_selected(const P4PackageProvider *provider)
{
    int state = atomic_load(&phase);
    return (state == 1 || state == 2) && provider && provider->info.ordinal < catalog_count &&
        catalog[provider->info.ordinal] == provider && !atomic_load(&provider->refused) &&
        !atomic_load(&provider->registry->retired);
}
size_t p4_package_provider_files_count(const P4PackageProvider *provider)
{
    return provider ? p4_package_files_count(provider->index) : 0;
}
int p4_package_provider_file(const P4PackageProvider *provider, size_t position,
                              const char **relative, int *directory)
{
    return provider ? p4_package_files_entry(provider->index, position, relative, directory) : -1;
}
int p4_package_provider_replacement(const P4PackageProvider *provider, size_t position,
                                     const char **key, int *directory)
{
    if (!provider || position >= p4_package_files_count(provider->index) || !key || !directory) return -1;
    if (!provider->view.files) return 1;
    return p4_package_file_view_entry(&provider->view, position, key, directory);
}
static P4PackageProvider *preparing_provider(const P4PackageProvider *provider)
{
    if (phase != 1 || !provider || provider->info.ordinal >= catalog_count ||
        catalog[provider->info.ordinal] != provider) return NULL;
    return (P4PackageProvider *)provider;
}
int p4_package_provider_refuse(const P4PackageProvider *provider, const char *reason)
{
    P4PackageProvider *actual = preparing_provider(provider);
    return actual ? reject_provider(actual, reason) : -1;
}
int p4_package_provider_claim(const P4PackageProvider *provider, uint32_t claims)
{
    const uint32_t allowed = P4_PACKAGE_CLAIM_FILES | P4_PACKAGE_CLAIM_SCRIPT |
        P4_PACKAGE_CLAIM_SEMANTIC | P4_PACKAGE_CLAIM_COSMETIC |
        P4_PACKAGE_CLAIM_NATIVE | P4_PACKAGE_CLAIM_TABLES;
    P4PackageProvider *actual = preparing_provider(provider);
    if (!actual || !claims || (claims & ~allowed)) return -1;
    atomic_fetch_or(&actual->claims, claims);
    return 0;
}
uint32_t p4_package_provider_claims(const P4PackageProvider *provider)
{
    return provider ? atomic_load(&provider->claims) : 0;
}
int p4_package_provider_require_cosmetic(const P4PackageProvider *provider)
{
    P4PackageProvider *actual = preparing_provider(provider);
    if (!actual) return -1;
    actual->require_cosmetic = 1;
    return 0;
}


int p4_package_provider_retain(const P4PackageProvider *provider)
{
    int current_phase = atomic_load(&phase);
    if ((current_phase != 1 && current_phase != 2) || !provider || atomic_load(&provider->refused) ||
        atomic_load(&provider->registry->retired)) return -1;
    P4PackageRegistry *registry = provider->registry;
    if (registry_retain_storage(registry)) return -1;
    if (atomic_load(&registry->retired)) { registry_release(registry); return -1; }
    return 0;
}
void p4_package_provider_release(const P4PackageProvider *provider) { if (provider) registry_release(provider->registry); }
int p4_package_provider_same_source(const P4PackageProvider *left, const P4PackageProvider *right)
{
    return left && right && provider_data(left) == provider_data(right);
}
int p4_package_provider_has_file(const P4PackageProvider *provider, const char *relative)
{
    const char *actual;
    int directory;
    return provider && !atomic_load(&provider->registry->retired) &&
        !p4_package_files_find(provider->index, relative, &actual, &directory) && !directory;
}
static int contained_path(char *path, size_t capacity, const P4PackageProvider *provider, const char *relative)
{
    if (join(path, capacity, provider->info.directory, relative) ||
        !p4_os_is_dir_nofollow(provider->info.directory)) return -1;
    char *component = path + strlen(provider->info.directory) + 1;
    for (char *slash; (slash = strchr(component, '/')) != NULL; component = slash + 1) {
        *slash = 0;
        int directory = p4_os_is_dir_nofollow(path);
        *slash = '/';
        if (!directory) return -1;
    }
    return p4_os_is_file_nofollow(path) ? 0 : -1;
}
static int open_file(const P4PackageProvider *provider, const char *relative,
                     uint64_t maximum, P4PackageFilePlan *plan, P4PackageFile **out)
{
    if (plan ? p4_package_file_plan_retain(plan) : p4_package_provider_retain(provider)) return -1;
    char path[1200];
    FILE *stream = NULL;
    if (contained_path(path, sizeof path, provider, relative)) goto fail;
    stream = p4_os_fopen_rb(path);
    P4OSFileIdentity identity;
    if (!stream || p4_os_file_identity(stream, &identity) || identity.size > maximum) goto fail;
    if (plan && (identity.size != plan->identity.size ||
        memcmp(identity.tokens, plan->identity.tokens, sizeof identity.tokens))) goto fail;
    P4PackageFile *file = p4_package_metadata_alloc(sizeof *file, 0);
    if (!file) goto fail;
    file->stream = stream;
    file->provider = provider;
    file->origin = (P4PackageFileOrigin){&provider->info, relative};
    file->plan = plan;
    file->maximum = maximum;
    if (!plan) file->identity = identity;
    *out = file;
    return 0;
fail:
    if (stream) fclose(stream);
    if (plan) p4_package_file_plan_release(plan);
    else p4_package_provider_release(provider);
    return -1;
}
int p4_package_file_open(const P4PackageProvider *provider, const char *relative,
                         uint64_t maximum, P4PackageFile **out)
{
    if (!out || *out || !provider || !relative) return -1;
    const char *actual;
    int directory;
    int found = p4_package_files_find(provider->index, relative, &actual, &directory);
    if (found) return found;
    return directory ? -1 : open_file(provider, actual, maximum, NULL, out);
}
int p4_package_file_plan_acquire(P4PackageFile *file, P4PackageFilePlan **out)
{
    if (!file || !out || *out || p4_package_provider_retain(file->provider)) return -1;
    if (file->plan) {
        int result = p4_package_file_plan_retain(file->plan);
        if (!result) *out = file->plan;
        p4_package_provider_release(file->provider);
        return result;
    }
    P4PackageFilePlan *plan = p4_package_metadata_alloc(sizeof *plan, 0);
    if (!plan) { p4_package_provider_release(file->provider); return -1; }
    plan->provider = file->provider;
    plan->relative = file->origin.relative;
    plan->maximum = file->maximum;
    plan->identity = file->identity;
    atomic_init(&plan->references, 2); /* One cached by the file, one returned. */
    file->plan = plan;
    p4_package_provider_release(file->provider); /* File now owns its plan instead. */
    *out = plan;
    return 0;
}
int p4_package_file_plan_retain(P4PackageFilePlan *plan)
{
    if (!plan) return -1;
    unsigned value = atomic_load(&plan->references);
    while (value && value != UINT_MAX) {
        if (atomic_compare_exchange_weak(&plan->references, &value, value + 1)) return 0;
    }
    return -1;
}
void p4_package_file_plan_release(P4PackageFilePlan *plan)
{
    if (!plan || atomic_fetch_sub(&plan->references, 1) != 1) return;
    const P4PackageProvider *provider = plan->provider;
    p4_package_metadata_free(plan, sizeof *plan);
    p4_package_provider_release(provider);
}
int p4_package_file_plan_open(P4PackageFilePlan *plan, P4PackageFile **out)
{
    if (!plan || !out || *out) return -1;
    return open_file(plan->provider, plan->relative, plan->maximum, plan, out);
}
static const P4OSFileIdentity *file_identity(const P4PackageFile *file)
{
    return file->plan ? &file->plan->identity : &file->identity;
}
uint64_t p4_package_file_size(const P4PackageFile *file) { return file ? file_identity(file)->size : 0; }
int p4_package_file_read_at(P4PackageFile *file, uint64_t offset, void *bytes, size_t size)
{
    if (!file || (!bytes && size) || offset > INT64_MAX) return -1;
    uint64_t length = file_identity(file)->size;
    if (offset > length || size > length - offset) return -1;
    if (!size) return 0;
    if (fseeko(file->stream, (off_t)offset, SEEK_SET)) return -1;
    size_t got = fread(bytes, 1, size, file->stream);
    if (got && read_observer) read_observer(&file->origin, bytes, got, offset, read_observer_user);
    return got == size && !ferror(file->stream) ? 0 : -1;
}
int p4_package_file_close(P4PackageFile *file)
{
    if (!file) return -1;
    int result = fclose(file->stream);
    const P4PackageProvider *provider = file->provider;
    P4PackageFilePlan *plan = file->plan;
    p4_package_metadata_free(file, sizeof *file);
    if (plan) p4_package_file_plan_release(plan);
    else p4_package_provider_release(provider);
    return result ? -1 : 0;
}
int p4_package_reads_observe(P4PackageReadObserver observer, void *user)
{
    if (!observer || read_observer) return -1;
    read_observer = observer;
    read_observer_user = user;
    return 0;
}
int p4_package_reads_unobserve(P4PackageReadObserver observer, void *user)
{
    if (!observer || read_observer != observer || read_observer_user != user) return -1;
    read_observer = NULL;
    read_observer_user = NULL;
    return 0;
}
