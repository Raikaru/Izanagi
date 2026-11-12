/* [resource-model owner:key]: glTF models that replace a disc model's look (docs/modding/files.md,
 * docs/modding/models.md). Same admission pattern as resource-image (mod_resources_boot.c): bounded
 * lowercase owner:key, explicit nonzero revision, contained indexed files read through issued file
 * plans, a cosmetic claim, refusals that name the file and the exceeded bound, later selected
 * package wins a shared key or target. Early CPU preparation cooks (or loads the cached cook of)
 * each glTF; the runtime phase freezes the override table the model loader consults. */
#include "p4pc/mod_models.h"
#include "p4pc/mod_setting_gate.h"
#include "p4pc/vfs_source.h"
#include "p4pc/os.h"
#include "../config/p4cfg.h"
#include "../models/p4model.h"
#include "file_index.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODEL_KEY_MAX 256
#define MODEL_PLANS_MAX 32

typedef struct ModelInput {
    char owner[64], key[96];
    uint32_t revision, schema;
    char replaces[MODEL_KEY_MAX];   /* canonical disc key, pack member name as stored in the pack */
    const P4PackageProvider *provider;
    char file[MODEL_KEY_MAX];       /* actual contained glTF key */
    P4PackageFilePlan *plans[MODEL_PLANS_MAX];
    size_t plan_count;
    uint8_t *blob;
    size_t blob_size;
    int cache_hit;
} ModelInput;

struct P4ModelOverride {
    char replaces[MODEL_KEY_MAX];
    char name[160];
    char provider[64];
    uint8_t *blob;
    size_t size;
    uint32_t schema;
};

static ModelInput *prepared;
static size_t prepared_count, prepared_capacity;
static P4ModelOverride *table;
static size_t table_count;
static int table_active;

/* ------------------------------------------------------------------------------------------------
 * Table */

static int key_compare(const char *a, const char *b)
{
    for (;; ++a, ++b) {
        int x = (unsigned char)*a, y = (unsigned char)*b;
        if (x >= 'a' && x <= 'z') x -= 32;
        if (y >= 'a' && y <= 'z') y -= 32;
        if (x != y || !x) return x - y;
    }
}

int p4_model_overrides_active(void)
{
    return table_active;
}

const P4ModelOverride *p4_model_override_find(const char *key)
{
    if (!table_active || !key) return NULL;
    for (size_t i = 0; i < table_count; ++i)
        if (!key_compare(table[i].replaces, key)) return &table[i];
    return NULL;
}

const uint8_t *p4_model_override_blob(const P4ModelOverride *override, size_t *size)
{
    if (size) *size = override ? override->size : 0;
    return override ? override->blob : NULL;
}

const char *p4_model_override_name(const P4ModelOverride *override)
{
    return override ? override->name : "";
}

const char *p4_model_override_provider(const P4ModelOverride *override)
{
    return override ? override->provider : "";
}

uint32_t p4_model_override_schema(const P4ModelOverride *override)
{
    return override ? override->schema : 0u;
}

static int table_add(const char *key, const char *name, const char *provider, uint8_t *blob, size_t size, uint32_t schema)
{
    for (size_t i = 0; i < table_count; ++i)
        if (!key_compare(table[i].replaces, key)) {
            free(table[i].blob);
            table[i].blob = blob;
            table[i].size = size;
            table[i].schema = schema;
            snprintf(table[i].name, sizeof table[i].name, "%s", name);
            snprintf(table[i].provider, sizeof table[i].provider, "%s", provider);
            return 0;
        }
    P4ModelOverride *grown = realloc(table, (table_count + 1) * sizeof *grown);
    if (!grown) return -1;
    table = grown;
    P4ModelOverride *o = &table[table_count++];
    memset(o, 0, sizeof *o);
    snprintf(o->replaces, sizeof o->replaces, "%s", key);
    snprintf(o->name, sizeof o->name, "%s", name);
    snprintf(o->provider, sizeof o->provider, "%s", provider);
    o->blob = blob;
    o->size = size;
    o->schema = schema;
    return 0;
}

void p4_model_overrides_session_end(void)
{
    for (size_t i = 0; i < table_count; ++i) free(table[i].blob);
    free(table);
    table = NULL;
    table_count = 0;
    table_active = 0;
}

/* ------------------------------------------------------------------------------------------------
 * p4pc_viewer's P4PC_VIEWER_MODEL seam: a host glTF, its files beside it, cooked with the budget of
 * the model it overrides and installed without a package. */

static char viewer_dir[1024];

static int viewer_read_host(const char *path, uint8_t **data, size_t *size)
{
    FILE *f = fopen(path, "rb");
    long n = -1;
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END) == 0) n = ftell(f);
    if (n <= 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return -1;
    }
    *data = malloc((size_t)n);
    if (!*data || fread(*data, 1, (size_t)n, f) != (size_t)n) {
        free(*data);
        *data = NULL;
        fclose(f);
        return -1;
    }
    fclose(f);
    *size = (size_t)n;
    return 0;
}

static int viewer_read_beside(void *user, const char *uri, uint8_t **data, size_t *size)
{
    char path[2048];
    (void)user;
    if (strstr(uri, "..")) return -1;
    snprintf(path, sizeof path, "%s%s", viewer_dir, uri);
    return viewer_read_host(path, data, size);
}

int p4_model_viewer_override(const char *rmd_path)
{
    const char *gltf_path = getenv("P4PC_VIEWER_MODEL");
    char key[600], error[512];
    uint8_t *gltf = NULL, *blob = NULL;
    size_t size = 0, blob_size = 0;

    if (!gltf_path || !*gltf_path) return 0;
    const char *slash = strrchr(gltf_path, '/'), *back = strrchr(gltf_path, '\\');
    if (back && (!slash || back > slash)) slash = back;
    snprintf(viewer_dir, sizeof viewer_dir, "%.*s", slash ? (int)(slash - gltf_path + 1) : 0, gltf_path);
    if (p4_model_disc_key(rmd_path, key, 512) || viewer_read_host(gltf_path, &gltf, &size)) {
        fprintf(stderr, "p4pc_viewer: cannot read %s or resolve %s\n", gltf_path, rmd_path);
        return -1;
    }
    int failed = p4_model_cook(gltf_path, gltf, size, viewer_read_beside, NULL, p4_model_budget_for(key), &blob, &blob_size,
                               error, sizeof error);
    free(gltf);
    if (failed) {
        fprintf(stderr, "p4pc_viewer: model refused: %s\n", error);
        return -1;
    }
    if (table_add(key, "viewer:model", "viewer", blob, blob_size, 3u)) {
        free(blob);
        return -1;
    }
    table_active = 1;
    fprintf(stderr, "p4pc_viewer: %s overrides %s\n", gltf_path, key);
    return 0;
}


/* ------------------------------------------------------------------------------------------------
 * Declarations */

static int identifier(const char *text, size_t size)
{
    if (!text || !size || text[0] == '.' || text[size - 1] == '.') return 0;
    for (size_t i = 0; i < size; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) return 0;
    }
    return 1;
}

static int qualified(const char *key, char owner[64])
{
    if (!key) return 0;
    const char *colon = strchr(key, ':');
    if (!colon || colon == key || !colon[1] || strchr(colon + 1, ':')) return 0;
    size_t owner_size = (size_t)(colon - key), key_size = strlen(key);
    if (owner_size >= 64 || key_size >= 96 || !identifier(key, owner_size) ||
        !identifier(colon + 1, key_size - owner_size - 1))
        return 0;
    memcpy(owner, key, owner_size);
    owner[owner_size] = 0;
    return 1;
}

static int decimal(const char *text, uint32_t *out)
{
    if (!text || !*text) return 0;
    uint32_t value = 0;
    do {
        unsigned char c = (unsigned char)*text++;
        if (c < '0' || c > '9' || value > (UINT32_MAX - (c - '0')) / 10u) return 0;
        value = value * 10u + c - '0';
    } while (*text);
    if (!value) return 0;
    *out = value;
    return 1;
}

static int refuse(const P4PackageProvider *provider, const char *key, const char *file, const char *why)
{
    char message[640];
    if (file)
        snprintf(message, sizeof message, "resource-model %.95s (%.160s): %.360s", key ? key : "?", file, why);
    else
        snprintf(message, sizeof message, "resource-model %.95s: %.360s", key ? key : "?", why);
    p4_package_provider_refuse(provider, message);
    return -1;
}

static const char *actual_file(const P4PackageProvider *provider, const char *path)
{
    size_t first = 0, last = p4_package_provider_files_count(provider);
    while (first < last) {
        size_t middle = first + (last - first) / 2;
        const char *actual;
        int directory;
        if (p4_package_provider_file(provider, middle, &actual, &directory)) return NULL;
        int order = p4_package_key_compare(actual, path);
        if (!order) return directory ? NULL : actual;
        if (order < 0) first = middle + 1;
        else last = middle;
    }
    return NULL;
}

static void input_release(ModelInput *input)
{
    for (size_t i = 0; i < input->plan_count; ++i) p4_package_file_plan_release(input->plans[i]);
    free(input->blob);
    memset(input, 0, sizeof *input);
}

static void prepared_clear(void)
{
    for (size_t i = 0; i < prepared_count; ++i) input_release(&prepared[i]);
    free(prepared);
    prepared = NULL;
    prepared_count = prepared_capacity = 0;
}

/* The target as the disc stores it: container-qualified key, and for a pack member the member name
 * spelled as the pack records it. 0, or -1 with a reason. */
static int resolve_target(const char *text, char out[MODEL_KEY_MAX], char *why, size_t why_size)
{
    char path[MODEL_KEY_MAX];
    const char *hash = strchr(text, '#');
    size_t length = hash ? (size_t)(hash - text) : strlen(text);
    if (!length || length >= sizeof path) {
        snprintf(why, why_size, "replaces must name a disc RMD or PAC#member key");
        return -1;
    }
    memcpy(path, text, length);
    path[length] = 0;
    P4VfsSourcePlan *plan = NULL;
    /* The disc, else the developer tree (P4PC_OVERRIDE_DIR), which stands in for it in fixtures. */
    int found = vfs_source_plan_acquire(P4_VFS_DISC, path, &plan);
    if (found > 0) found = vfs_source_plan_acquire(P4_VFS_DEVELOPER, path, &plan);
    if (found) {
        snprintf(why, why_size, found > 0 ? "replaces %s: not on the disc" : "replaces %s: the disc cannot be read", path);
        return -1;
    }
    const P4VfsSourceInfo *info = vfs_source_plan_info(plan);
    snprintf(out, MODEL_KEY_MAX, "%s", info->key);
    int result = 0;
    if (hash) {
        /* A flat pack: 252-byte name, u32 size, payload, next entry on a 64-byte boundary. */
        const char *member = hash + 1;
        uint64_t offset = 0;
        int matched = 0;
        while (!matched && offset + 256 <= info->size) {
            uint8_t head[256];
            if (vfs_source_plan_read_at(plan, offset, head, sizeof head)) break;
            head[251] = 0;
            if (!head[0]) break;
            uint32_t size = (uint32_t)head[252] | (uint32_t)head[253] << 8 | (uint32_t)head[254] << 16 | (uint32_t)head[255] << 24;
            if (!key_compare((const char *)head, member)) {
                size_t used = strlen(out);
                snprintf(out + used, MODEL_KEY_MAX - used, "#%s", (const char *)head);
                matched = 1;
                break;
            }
            offset = (offset + 256 + size + 63) / 64 * 64;
        }
        if (!matched) {
            snprintf(why, why_size, "replaces %s: the pack has no member %s", path, member);
            result = -1;
        }
    } else {
        size_t n = strlen(out);
        if (n < 4 || key_compare(out + n - 4, ".RMD")) {
            snprintf(why, why_size, "replaces %s: a model is an .RMD file or a PAC#member", out);
            result = -1;
        }
    }
    vfs_source_plan_release(plan);
    return result;
}

typedef struct ReadContext {
    const P4PackageProvider *provider;
    ModelInput *input;
    char directory[MODEL_KEY_MAX];  /* the glTF's directory inside the package, with trailing '/' */
    uint64_t limit;
    int failed;
} ReadContext;

static int read_whole(const P4PackageProvider *provider, const char *actual, uint64_t limit, ModelInput *input,
                      uint8_t **data, size_t *size)
{
    P4PackageFile *stream = NULL;
    if (p4_package_file_open(provider, actual, limit, &stream)) return -1;
    uint64_t length = p4_package_file_size(stream);
    P4PackageFilePlan *plan = NULL;
    uint8_t *bytes = length && length <= SIZE_MAX ? malloc((size_t)length) : NULL;
    int failed = !bytes || input->plan_count >= MODEL_PLANS_MAX || p4_package_file_plan_acquire(stream, &plan) ||
                 p4_package_file_read_at(stream, 0, bytes, (size_t)length);
    if (p4_package_file_close(stream)) failed = 1;
    if (failed) {
        if (plan) p4_package_file_plan_release(plan);
        free(bytes);
        return -1;
    }
    input->plans[input->plan_count++] = plan;
    *data = bytes;
    *size = (size_t)length;
    return 0;
}

static int read_package_file(void *user, const char *uri, uint8_t **data, size_t *size)
{
    ReadContext *c = user;
    char path[MODEL_KEY_MAX * 2];
    if (!uri || !*uri || uri[0] == '/' || strchr(uri, ':') || strchr(uri, '\\') || strstr(uri, "..")) return -1;
    int n = snprintf(path, sizeof path, "%s%s", c->directory, uri);
    if (n < 0 || (size_t)n >= sizeof path) return -1;
    /* The hash and a cook each read the files they name; every read keeps its own issued plan, which
     * finalize_prepare re-checks before publication. */
    const char *actual = actual_file(c->provider, path);
    if (!actual) return -1;
    return read_whole(c->provider, actual, c->limit, c->input, data, size);
}

static const char *cache_directory(char out[1200])
{
    const char *env = getenv("P4PC_MODEL_CACHE");
    if (env && env[0]) {
        if (!strcmp(env, "none") || strlen(env) >= 1200) return NULL;
        strcpy(out, env);
        return out;
    }
    const char *config = p4cfg_path();
    if (!config || strlen(config) + sizeof "/model-cache" >= 1200) return NULL;
    strcpy(out, config);
    char *slash = strrchr(out, '/'), *back = strrchr(out, '\\');
    if (back && (!slash || back > slash)) slash = back;
    if (slash) strcpy(slash, "/model-cache");
    else strcpy(out, "model-cache");
    return out;
}

static int prepare_model(const P4PackageProvider *provider, const P4PackageField *fields, size_t count)
{
    const P4PackageInfo *info = p4_package_provider_info(provider);
    const char *key = fields[0].argument, *file = NULL, *format = NULL, *schema = NULL, *revision = NULL, *replaces = NULL;
    ModelInput input;
    memset(&input, 0, sizeof input);
    if (!info || !p4_package_provider_selected(provider) || !qualified(key, input.owner))
        return refuse(provider, key, NULL, "expected a bounded lowercase owner:key");
    size_t gates = 0;
    for (size_t i = 0; i < count; ++i) {
        if (!strcmp(fields[i].key, "file")) file = fields[i].value;
        else if (!strcmp(fields[i].key, "format")) format = fields[i].value;
        else if (!strcmp(fields[i].key, "schema")) schema = fields[i].value;
        else if (!strcmp(fields[i].key, "revision")) revision = fields[i].value;
        else if (!strcmp(fields[i].key, "replaces")) replaces = fields[i].value;
        else if (!strcmp(fields[i].key, P4_MOD_SETTING_GATE_FIELD)) ++gates; /* evaluated by prepare() */
        else return refuse(provider, key, file, "unknown declaration field");
    }
    /* schema=1: static meshes; schema=2 adds skinned meshes; schema=3 adds slot animations (a port without
     * them refuses the declaration). */
    uint32_t schema_number;
    if (gates > 1 || count - gates != 5 || !file || !*file || !format || strcmp(format, "gltf") ||
        !decimal(schema, &schema_number) || schema_number < 1 || schema_number > 3 ||
        !decimal(revision, &input.revision) || !replaces)
        return refuse(provider, key, file, "requires file, format=gltf, schema=1, 2 or 3, nonzero revision and replaces");
    char why[320];
    input.schema = schema_number;
    if (resolve_target(replaces, input.replaces, why, sizeof why)) return refuse(provider, key, file, why);
    const P4ModelBudget *budget = p4_model_budget_for(input.replaces);
    const char *actual = actual_file(provider, file);
    if (!actual || strlen(actual) >= sizeof input.file)
        return refuse(provider, key, file, "contained indexed source is missing or its key is too long");
    snprintf(input.file, sizeof input.file, "%s", actual);
    ReadContext context;
    memset(&context, 0, sizeof context);
    context.provider = provider;
    context.input = &input;
    context.limit = budget->sourceBytes;
    const char *slash = strrchr(actual, '/');
    if (slash) snprintf(context.directory, sizeof context.directory, "%.*s/", (int)(slash - actual), actual);
    uint8_t *gltf = NULL;
    size_t size = 0;
    if (read_whole(provider, actual, budget->sourceBytes, &input, &gltf, &size)) {
        input_release(&input);
        char bound[96];
        snprintf(bound, sizeof bound, "cannot read it within %u MiB (the %s budget)", (unsigned)(budget->sourceBytes >> 20),
                 budget->category);
        return refuse(provider, key, file, bound);
    }
    char error[512] = {0}, cache_dir[1200], source[68];
    const char *cache = cache_directory(cache_dir);
    int failed = p4_model_source_hash(file, gltf, size, read_package_file, &context, source, error, sizeof error);
    if (!failed && cache && p4_model_cache_load(cache, source, &input.blob, &input.blob_size) == 0) {
        input.cache_hit = 1;
    } else if (!failed) {
        failed = p4_model_cook(file, gltf, size, read_package_file, &context, budget, &input.blob, &input.blob_size,
                               error, sizeof error);
        if (!failed && cache && p4_model_cache_store(cache, input.blob, input.blob_size))
            fprintf(stderr, "p4pc: resource-model %s: cannot write the model cache in %s\n", key, cache);
    }
    free(gltf);
    if (failed) {
        const char *why = error[0] ? error : "cannot cook the glTF";
        size_t named = strlen(file);
        input_release(&input);
        /* The cooker's reasons already start with "<file>: ". */
        return refuse(provider, key, !strncmp(why, file, named) && why[named] == ':' ? NULL : file, why);
    }
    for (uint32_t i = 0; schema_number == 1 && i < p4_model_header(input.blob)->meshes; ++i)
        if (p4_model_mesh(input.blob, i)->skinOffset) {
            input_release(&input);
            snprintf(why, sizeof why, "mesh %u is skinned; skinned meshes need schema=2", (unsigned)i);
            return refuse(provider, key, file, why);
        }
    if (p4_package_provider_claim(provider, P4_PACKAGE_CLAIM_COSMETIC)) {
        input_release(&input);
        return refuse(provider, key, file, "cannot claim the selected cosmetic source");
    }
    snprintf(input.key, sizeof input.key, "%s", key);
    input.provider = provider;
    /* rw_to_gltf exports carry every slot's animation: below schema 3 they are cooked but not applied. */
    const uint32_t slot_animations = p4_model_header(input.blob)->animations;
    char animations[96] = "";
    if (slot_animations)
        snprintf(animations, sizeof animations, schema_number >= 3 ? ", %u slot animations" : ", %u slot animations left retail (schema=3 applies them)",
                 (unsigned)slot_animations);
    fprintf(stderr, "p4pc: resource-model %s: %s replaces %s (%u triangles%s, %s)\n", key, file, input.replaces,
            (unsigned)p4_model_header(input.blob)->triangles, animations, input.cache_hit ? "cached cook" : "cooked");
    for (size_t i = 0; i < prepared_count; ++i) {
        ModelInput *other = &prepared[i];
        if (!strcmp(other->key, input.key) || !key_compare(other->replaces, input.replaces)) {
            const P4PackageInfo *left = p4_package_provider_info(other->provider);
            fprintf(stderr, "p4pc: resource-model %s (%s) from provider %s overrides %s (%s) from provider %s\n", input.key,
                    input.replaces, info->id, other->key, other->replaces, left ? left->id : "?");
            input_release(other);
            *other = input; /* catalog order: the later selected package wins */
            return 0;
        }
    }
    if (prepared_count == prepared_capacity) {
        size_t next = prepared_capacity ? prepared_capacity * 2 : 8;
        ModelInput *grown = realloc(prepared, next * sizeof *grown);
        if (!grown) {
            input_release(&input);
            return refuse(provider, key, file, "resource metadata budget exhausted");
        }
        prepared = grown;
        prepared_capacity = next;
    }
    prepared[prepared_count++] = input;
    return 0;
}

static int prepare(void *unused)
{
    (void)unused;
    if (table || prepared_count) return -1;
    for (size_t i = 0, n = p4_packages_count(); i < n; ++i) {
        const P4PackageProvider *provider = p4_packages_provider(i);
        size_t count = 0;
        const P4PackageField *fields = p4_package_provider_fields(provider, &count);
        for (size_t first = 0; first < count;) {
            size_t last = first + 1;
            while (last < count && !strcmp(fields[first].section, fields[last].section) &&
                   !strcmp(fields[first].argument, fields[last].argument))
                ++last;
            if (!strcmp(fields[first].section, "resource-model")) {
                char reason[192];
                const int gate = p4_mod_setting_section_gate(provider, fields + first, last - first, reason, sizeof reason);
                if (gate < 0) return refuse(provider, fields[first].argument, NULL, reason);
                if (gate && prepare_model(provider, fields + first, last - first)) return -1;
            }
            first = last;
        }
    }
    return p4_package_boot_provide(prepared_count ? prepared : NULL);
}

static int finalize_prepare(void *unused)
{
    (void)unused;
    for (size_t i = 0; i < prepared_count; ++i) {
        ModelInput *input = &prepared[i];
        for (size_t p = 0; p < input->plan_count; ++p) {
            P4PackageFile *check = NULL;
            if (p4_package_file_plan_open(input->plans[p], &check))
                return refuse(input->provider, input->key, input->file, "selected source changed before publication");
            if (p4_package_file_close(check))
                return refuse(input->provider, input->key, input->file, "selected source failed final identity check");
        }
    }
    return 0;
}

static void rollback(void *unused)
{
    (void)unused;
    prepared_clear(); /* early pass only, before raw file exposure */
}

/* A raw files/ replacement of the same RMD or PAC from another selected package: the model section is
 * the more specific declaration and wins for the model's look (the loader still reads the winning
 * file's other chunks). Reported with both providers. */
static void report_raw_replacements(const ModelInput *input)
{
    char file_key[MODEL_KEY_MAX];
    snprintf(file_key, sizeof file_key, "%s", input->replaces);
    char *hash = strchr(file_key, '#');
    if (hash) *hash = 0;
    const P4PackageInfo *mine = p4_package_provider_info(input->provider);
    for (size_t i = 0, n = p4_packages_count(); i < n; ++i) {
        const P4PackageProvider *provider = p4_packages_provider(i);
        if (!p4_package_provider_selected(provider)) continue;
        for (size_t f = 0, files = p4_package_provider_files_count(provider); f < files; ++f) {
            const char *replaced;
            int directory;
            if (p4_package_provider_replacement(provider, f, &replaced, &directory) || directory) continue;
            if (!key_compare(replaced, file_key)) {
                const P4PackageInfo *theirs = p4_package_provider_info(provider);
                fprintf(stderr, "p4pc: resource-model %s from provider %s wins over provider %s's raw replacement of %s "
                        "for the model's geometry, materials and textures\n", input->key, mine ? mine->id : "?",
                        theirs ? theirs->id : "?", file_key);
            }
        }
    }
}

static int runtime_prepare(void *unused)
{
    (void)unused;
    if (table_active) return p4_package_boot_fail(NULL, "Model override table still active");
    if (!prepared_count) return p4_package_boot_provide(NULL);
    for (size_t i = 0; i < prepared_count; ++i) {
        ModelInput *input = &prepared[i];
        if (!p4_package_provider_selected(input->provider))
            return p4_package_boot_fail(NULL, "Selected model provider is no longer active");
        const P4PackageInfo *info = p4_package_provider_info(input->provider);
        char name[160];
        snprintf(name, sizeof name, "%s", input->key);
        report_raw_replacements(input);
        if (table_add(input->replaces, name, info ? info->id : "?", input->blob, input->blob_size, input->schema))
            return p4_package_boot_fail(input->provider, "Cannot publish the selected model override");
        input->blob = NULL; /* the table owns it now */
        input->blob_size = 0;
    }
    table_active = table_count != 0;
    return p4_package_boot_provide(table_active ? table : NULL);
}

static const char *const sections[] = {"resource-model", NULL};
static const char *const dependencies[] = {P4_MOD_SETTINGS_HANDLER, NULL};
const P4PackageHandler p4_resources_models_boot_provider = {
    .name = P4_RESOURCES_MODELS_HANDLER,
    .sections = sections,
    .prepare = prepare,
    .rollback = rollback,
    .boot_phase = P4_PACKAGE_BOOT_RESOURCES,
    .dependencies = dependencies,
    .finalize_prepare = finalize_prepare,
    .runtime_prepare = runtime_prepare
};
