/*
 * Virtual file tree behind the PS2 file API (vfs.h).
 *
 * Path rules: a device prefix ("cdrom0:", "host0:") is dropped, '\' becomes '/',
 * repeated and leading separators are removed, a ";N" version suffix is cut, and the
 * rest is upper-cased. ".." components are refused.
 *
 * Lookup order: the override directory first (a host file at the same upper-case
 * relative path), then the disc. The disc's five containers are searched in the order
 * DATA, BTL, BGM, ENV, ZZZZZ, then the disc's own root files (SLUS_217.82, SYSTEM.CNF,
 * and the LIB31 IRX modules). A path whose first component is a container label ("BTL/BATTLE/X")
 * searches only that container.
 */
#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "iso9660.h"
#include "vfs.h"
#include "p4pc/os.h"
#include "p4pc/vfs_source.h"
#include <limits.h>
#include "p4pc/profile_trace.h"

#define CONTAINERS 5
#define CVM_HEADER_BYTES 0x1800
#define CVM_MAGIC "CVMH"
#define OPEN_MAX 64
#define KEY_MAX 512
#define HOST_PATH_MAX 1200

static const char *const labels[CONTAINERS] = {"DATA", "BTL", "BGM", "ENV", "ZZZZZ"};
static const char *const files[CONTAINERS] = {"DATA.CVM", "BTL.CVM", "BGM.CVM", "ENV.CVM", "ZZZZZ.BIN"};
static char upper_ascii(unsigned char c) { return (char)(c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c); }

typedef struct {
    char *path;       /* canonical key inside the container: upper case, '/'-separated */
    int container;    /* index into labels[] */
    long long offset; /* absolute byte offset of the data in the disc image */
    long long size;
} entry_t;

typedef struct {
    int used;
    FILE *fp;         /* disc image (shared, not owned) or host override file (owned) */
    int owned;
    int entry;        /* immutable disc extent, including an override's original backing file */
    int layer;        /* selected host provider, or -1 for the disc */
    char origin_key[KEY_MAX + 6]; /* lazily qualified for indexed disc files */
    long long base;   /* absolute offset of byte 0 of the file within fp */
    long long size;
    long long pos;
} slot_t;

static FILE *disc;
static entry_t *entries;
static int n_entries;
static int cap_entries;
#define OVERRIDE_MAX 65 /* 64 enabled mods and the developer's loose-file layer */
typedef struct OverrideRoot {
    char path[HOST_PATH_MAX], provider[64];
    P4VfsOriginKind kind;
    P4VfsPathIndex index;
    void *index_user;
} OverrideRoot;
static OverrideRoot override_roots[OVERRIDE_MAX];
static int override_count;
static int mounted;
static slot_t slots[OPEN_MAX];
static P4VfsReadObserver read_observer;
static void *read_observer_user;
static const P4VfsOrigin disc_origin = {P4_VFS_DISC, 0, "disc", NULL};
static int full_key(const entry_t *entry, char *buffer, size_t capacity);
/* A demand-only boot view, NOT a mount or an exposed replacement layer. */
static int source_prepared;
static char source_iso[HOST_PATH_MAX];
static OverrideRoot source_developer;
static P4OSFileIdentity source_disc_identity;
static unsigned source_plans;
struct P4VfsSourcePlan {
    slot_t slot;
    P4VfsSourceInfo info;
    unsigned references;
};
static int same_identity(FILE *stream, const P4OSFileIdentity *identity)
{
    P4OSFileIdentity current;
    return !p4_os_file_identity(stream, &current) && current.size == identity->size &&
        !memcmp(current.tokens, identity->tokens, sizeof current.tokens);
}

typedef struct {
    int found;
    long long offset[CONTAINERS];
    long long size[CONTAINERS];
} root_t;

typedef struct {
    int container;
    int failed;
} walk_t;

static void root_visit(void *ctx, const char *path, long long offset, long long size)
{
    root_t *root = ctx;
    for (int i = 0; i < CONTAINERS; i++) {
        if (strcmp(path, files[i]) == 0) {
            root->offset[i] = offset;
            root->size[i] = size;
            root->found |= 1 << i;
        }
    }
}

static void add_visit(void *ctx, const char *path, long long offset, long long size)
{
    walk_t *w = ctx;
    char *copy;

    if (w->failed) {
        return;
    }
    if (n_entries == cap_entries) {
        int cap = cap_entries ? cap_entries * 2 : 4096;
        entry_t *grown = realloc(entries, (size_t)cap * sizeof *entries);
        if (!grown) {
            w->failed = 1;
            return;
        }
        entries = grown;
        cap_entries = cap;
    }
    copy = strdup(path);
    if (!copy) {
        w->failed = 1;
        return;
    }
    entries[n_entries].path = copy;
    entries[n_entries].container = w->container;
    entries[n_entries].offset = offset;
    entries[n_entries].size = size;
    n_entries++;
}

static int cmp_entry(const void *a, const void *b)
{
    const entry_t *x = a;
    const entry_t *y = b;
    int c = strcmp(x->path, y->path);
    if (c) {
        return c;
    }
    return x->container - y->container;
}

static void clear_index(void)
{
    for (int i = 0; i < n_entries; i++) {
        free(entries[i].path);
    }
    n_entries = 0;
}

static int check_cvm_header(FILE *fp, long long offset)
{
    char magic[4];
    if (iso_read_at(fp, offset, magic, sizeof magic) != 0) {
        return -1;
    }
    return memcmp(magic, CVM_MAGIC, sizeof magic) == 0 ? 0 : -1;
}

static int index_disc(const char *iso_path, int pin_identity)
{
    root_t root;
    walk_t w;

    memset(&root, 0, sizeof root);
    disc = p4_os_fopen_rb(iso_path);
    if (!disc) {
        fprintf(stderr, "p4pc: vfs: cannot open disc image %s\n", iso_path);
        return -1;
    }
    if (pin_identity && p4_os_file_identity(disc, &source_disc_identity)) goto fail;
    if (iso_walk(disc, 0, root_visit, &root) != 0 || root.found != (1 << CONTAINERS) - 1) {
        fprintf(stderr, "p4pc: vfs: %s is not a complete disc (missing CVM containers)\n", iso_path);
        goto fail;
    }
    for (int i = 0; i < CONTAINERS; i++) {
        if (check_cvm_header(disc, root.offset[i]) != 0) {
            fprintf(stderr, "p4pc: vfs: %s has no CVMH header\n", files[i]);
            goto fail;
        }
    }
    for (int i = 0; i < CONTAINERS; i++) {
        w.container = i;
        w.failed = 0;
        if (iso_walk(disc, root.offset[i] + CVM_HEADER_BYTES, add_visit, &w) != 0 || w.failed) {
            fprintf(stderr, "p4pc: vfs: cannot index %s\n", files[i]);
            goto fail;
        }
    }
    w.container = CONTAINERS;
    w.failed = 0;
    if (iso_walk(disc, 0, add_visit, &w) != 0 || w.failed) {
        fprintf(stderr, "p4pc: vfs: cannot index the disc root\n");
        goto fail;
    }
    qsort(entries, (size_t)n_entries, sizeof *entries, cmp_entry);
    if (pin_identity && !same_identity(disc, &source_disc_identity)) goto fail;
    return 0;

fail:
    clear_index();
    fclose(disc);
    disc = NULL;
    return -1;
}

static int register_root(const char *root, const char *provider, P4VfsOriginKind kind,
                         const P4VfsPathIndex *index, void *user)
{
    size_t len, provider_len;
    if (index && (!index->resolve || !index->list)) return -1;
    if (mounted || !root || !root[0] || !provider || !provider[0] || override_count == OVERRIDE_MAX)
        return -1;
    len = strlen(root);
    provider_len = strlen(provider);
    while (len > 1 && (root[len - 1] == '/' || root[len - 1] == '\\'))
        len--;
    if (len >= HOST_PATH_MAX || provider_len >= sizeof override_roots[0].provider)
        return -1;
    OverrideRoot *entry = &override_roots[override_count];
    memcpy(entry->path, root, len);
    entry->path[len] = '\0';
    memcpy(entry->provider, provider, provider_len + 1);
    entry->kind = kind;
    entry->index = index ? *index : (P4VfsPathIndex){0};
    entry->index_user = user;
    override_count++;
    return 0;
}

int vfs_override_register(const char *root, const char *provider,
                          const P4VfsPathIndex *index, void *user)
{
    return register_root(root, provider, P4_VFS_MOD, index, user);
}

void vfs_override_clear(void)
{
    if (!mounted)
        override_count = 0;
}

int vfs_mount(const char *iso_path, const char *override_dir)
{
    int before = override_count;

    if (mounted) {
        return 0;
    }
    if (source_prepared && (strcmp(iso_path ? iso_path : "", source_iso) ||
        strcmp(override_dir ? override_dir : "", source_developer.path) ||
        (disc && !same_identity(disc, &source_disc_identity)))) return -1;
    if (override_dir && override_dir[0] && register_root(override_dir, "developer", P4_VFS_DEVELOPER, NULL, NULL) != 0)
        return -1;
    if (iso_path && !source_prepared && index_disc(iso_path, 0) != 0) {
        override_count = before;
        return -1;
    }
    mounted = 1;
    {
        const char *trace = getenv("P4PC_PROFILE_SPANS");
        if (trace && trace[0] && trace[0] != '0') {
            /* The immutable index already exists. Raw sector spans are joined
             * to all intersecting extents offline, never guessed as one file. */
            char key[KEY_MAX + 6];
            for (int i = 0; i < n_entries; i++)
                if (!full_key(&entries[i], key, sizeof key))
                    fprintf(stderr, "PROF-EXTENT 0x%llx 0x%llx %s\n",
                        (unsigned long long)entries[i].offset,
                        (unsigned long long)entries[i].size, key);
        }
    }
    return 0;
}

int vfs_source_prepare_selected(void)
{
    if (source_prepared) return disc && !same_identity(disc, &source_disc_identity) ? -1 : 0;
    if (mounted) return -1;
    const char *iso = getenv("P4PC_ISO"), *developer = getenv("P4PC_OVERRIDE_DIR");
    if ((iso && strlen(iso) >= sizeof source_iso) ||
        (developer && strlen(developer) >= sizeof source_developer.path)) return -1;
    if (iso && index_disc(iso, 1)) return -1;
    if (iso) strcpy(source_iso, iso);
    if (developer) strcpy(source_developer.path, developer);
    strcpy(source_developer.provider, "developer"); source_developer.kind = P4_VFS_DEVELOPER;
    source_prepared = 1;
    return 0;
}
const char *vfs_source_selected_iso(void)
{
    return source_prepared ? (*source_iso ? source_iso : NULL) : getenv("P4PC_ISO");
}
int vfs_source_mount_selected(void)
{
    return vfs_mount(vfs_source_selected_iso(), source_prepared ?
        (*source_developer.path ? source_developer.path : NULL) : getenv("P4PC_OVERRIDE_DIR"));
}
int vfs_source_disc_identity(P4OSFileIdentity *identity)
{
    if (!identity || vfs_source_prepare_selected()) return -1;
    if (!disc) return 1;
    *identity = source_disc_identity;
    return 0;
}

/* Turns a PS2 path into a canonical key. Returns 0, or -1 when the path is too long
 * or names a parent directory. */
static int normalise(const char *in, char *key, size_t cap)
{
    const char *colon = strchr(in, ':');
    const char *comp;
    size_t n = 0;

    if (colon) {
        in = colon + 1;
    }
    for (const char *p = in; *p; p++) {
        char c = *p;
        if (c == '\\') {
            c = '/';
        }
        if (c == ';') {
            break;
        }
        if (c == '/' && (n == 0 || key[n - 1] == '/')) {
            continue;
        }
        if (n + 1 >= cap) {
            return -1;
        }
        key[n++] = upper_ascii((unsigned char)c);
    }
    if (n > 0 && key[n - 1] == '/') {
        n--;
    }
    key[n] = '\0';

    comp = key;
    for (;;) {
        const char *end = strchr(comp, '/');
        size_t len = end ? (size_t)(end - comp) : strlen(comp);
        if (len == 2 && comp[0] == '.' && comp[1] == '.') {
            return -1;
        }
        if (!end) {
            break;
        }
        comp = end + 1;
    }
    return 0;
}

/* If key starts with a container label and '/', returns that container and points *rest
 * past the label. Otherwise returns -1 and *rest is key. */
static int split_label(const char *key, const char **rest)
{
    for (int i = 0; i < CONTAINERS; i++) {
        size_t len = strlen(labels[i]);
        if (strncmp(key, labels[i], len) == 0 && key[len] == '/') {
            *rest = key + len + 1;
            return i;
        }
    }
    *rest = key;
    return -1;
}

/* Index of the first entry with this key in the requested container (or any, when hint
 * is -1). Ties sort by container, so the first match is the earliest in lookup order. */
static int find_entry(const char *key, int hint)
{
    int lo = 0;
    int hi = n_entries;

    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (strcmp(entries[mid].path, key) < 0) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    for (int i = lo; i < n_entries && strcmp(entries[i].path, key) == 0; i++) {
        if (hint < 0 || entries[i].container == hint) {
            return i;
        }
    }
    return -1;
}

static int alloc_slot(void)
{
    for (int i = 0; i < OPEN_MAX; i++) {
        if (!slots[i].used) {
            return i;
        }
    }
    return -1;
}

/* Indexed layers do not stat misses, and an indexed winner remains a winner
 * when its backing file becomes unreadable: never silently expose lower data. */
static int resolve_root(const OverrideRoot *root, const char *key, char *out, size_t cap)
{
    const char *relative = root->index.resolve ? root->index.resolve(key, root->index_user) : key;
    struct stat st;
    if (!relative) return -1;
    int n = relative[0] ? snprintf(out, cap, "%s/%s", root->path, relative)
                        : snprintf(out, cap, "%s", root->path);
    if (n < 0 || (size_t)n >= cap) return -1;
    return root->index.resolve || stat(out, &st) == 0 ? 0 : -1;
}

static int host_path(const char *key, int index, int container, char *out, size_t cap)
{
    char qualified[KEY_MAX + 6];
    for (int i = override_count - 1; i >= 0; i--) {
        if (resolve_root(&override_roots[i], key, out, cap) == 0)
            return i;
        /* Unqualified retail opens still address the same canonical file.
         * Search its qualified replacement within each layer, not afterwards:
         * otherwise a lower-priority loose path could beat a higher mod. */
        if (container < 0 && index >= 0 && entries[index].container < CONTAINERS) {
            int n = snprintf(qualified, sizeof qualified, "%s/%s",
                             labels[entries[index].container], entries[index].path);
            if (n >= 0 && (size_t)n < sizeof qualified && resolve_root(&override_roots[i], qualified, out, cap) == 0)
                return i;
        }
    }
    return -1;
}

/* Size of a host file. Port code is built without -malign-double, so struct stat matches
 * glibc's layout and st_size is read directly. */
static int host_size(const char *path, long long *size)
{
    struct stat st;

    if (stat(path, &st) != 0) {
        return -1;
    }
    *size = (long long)st.st_size;
    return 0;
}

int vfs_open(const char *path, int writable)
{
    char key[KEY_MAX];
    char host[HOST_PATH_MAX];
    struct stat st;
    long long size;
    const char *rest;
    FILE *fp;
    int fd;
    int container;
    int idx;

    if (writable || !mounted) {
        return -1;
    }
    if (normalise(path, key, sizeof key) != 0 || key[0] == '\0') {
        return -1;
    }
    fd = alloc_slot();
    if (fd < 0) {
        return -1;
    }
    container = split_label(key, &rest);
    idx = find_entry(rest, container);

    int layer = host_path(key, idx, container, host, sizeof host);
    if (layer >= 0 && (!override_roots[layer].index.resolve || p4_os_is_file_nofollow(host)) &&
        stat(host, &st) == 0 && S_ISREG(st.st_mode) && host_size(host, &size) == 0) {
        fp = fopen(host, "rb");
        if (fp) {
            slots[fd].used = 1;
            slots[fd].fp = fp;
            slots[fd].owned = 1;
            slots[fd].entry = idx;
            slots[fd].base = 0;
            slots[fd].size = size;
            slots[fd].pos = 0;
            slots[fd].layer = layer;
            slots[fd].origin_key[0] = '\0';
            if (idx < 0) memcpy(slots[fd].origin_key, key, strlen(key) + 1);
            return fd;
        }
    }
    if (layer >= 0) return -1;

    if (idx < 0) {
        return -1;
    }
    slots[fd].used = 1;
    slots[fd].fp = disc;
    slots[fd].owned = 0;
    slots[fd].entry = idx;
    slots[fd].base = entries[idx].offset;
    slots[fd].size = entries[idx].size;
    slots[fd].pos = 0;
    slots[fd].layer = -1;
    slots[fd].origin_key[0] = '\0';
    return fd;
}

static int slot_origin(slot_t *slot, P4VfsOrigin *out)
{
    if (!slot->origin_key[0] && (slot->entry < 0 ||
        full_key(&entries[slot->entry], slot->origin_key, sizeof slot->origin_key)))
        return -1;
    *out = disc_origin;
    if (slot->layer >= 0) {
        out->kind = override_roots[slot->layer].kind;
        out->rank = (uint32_t)slot->layer + 1;
        out->provider = override_roots[slot->layer].provider;
    } else if (slot->layer == -2) {
        out->kind = P4_VFS_DEVELOPER;
        out->rank = (uint32_t)override_count + (mounted ? 0u : 1u);
        out->provider = source_developer.provider;
    }
    out->key = slot->origin_key;
    return 0;
}
int vfs_origin(int fd, P4VfsOrigin *out)
{
    if (!out || fd < 0 || fd >= OPEN_MAX || !slots[fd].used) return -1;
    return slot_origin(&slots[fd], out);
}

int vfs_read_observer_register(P4VfsReadObserver observer, void *user)
{
    if (!observer || (read_observer && (read_observer != observer || read_observer_user != user))) return -1;
    read_observer = observer;
    read_observer_user = user;
    return 0;
}

int vfs_read_observer_remove(P4VfsReadObserver observer, void *user)
{
    if (!observer || read_observer != observer || read_observer_user != user) return -1;
    read_observer = NULL;
    read_observer_user = NULL;
    return 0;
}

__attribute__((noinline))
static void observe_slot(slot_t *slot, const void *bytes, size_t size)
{
    P4VfsOrigin origin;
    if (!slot_origin(slot, &origin)) {
        int64_t offset = slot->pos - (long long)size;
        read_observer(&origin, bytes, size, offset, slot->owned ? -1 : slot->base + offset, read_observer_user);
    }
}

/* Deferred logging keeps provenance callbacks outside physical read timing. */
static void trace_read(slot_t *slot, uint64_t start, uint64_t end, uint64_t requested,
                       int64_t result, uint64_t offset)
{
    P4VfsOrigin origin;
    if (start && !slot_origin(slot, &origin)) {
        const char *operation = origin.kind == P4_VFS_DISC ? "vfs.read.disc"
            : origin.kind == P4_VFS_MOD ? "vfs.read.mod" : "vfs.read.developer";
        p4pc_prof_span_record(start, end, operation, requested, result, offset, origin.key);
    }
}

/* A read inside the slot's known extent is complete or an error: a short host transfer is
 * continued, never reported as the end of the file. */
static int read_slot(slot_t *s, void *dst, int n)
{
    long long left;
    uint64_t trace_start, trace_end;
    off_t offset;
    left = s->size - s->pos;
    if (left <= 0) {
        return 0;
    }
    if ((long long)n > left) {
        n = (int)left;
    }
    offset = (off_t)(s->base + s->pos);
    trace_start = p4pc_prof_span_begin();
    if (iso_read_at(s->fp, (long long)offset, dst, (size_t)n) != 0) {
        trace_read(s, trace_start, p4pc_prof_span_stop(trace_start), (uint64_t)n, -1, (uint64_t)offset);
        return -1;
    }
    trace_end = p4pc_prof_span_stop(trace_start);
    s->pos += (long long)n;
    if (read_observer && n) observe_slot(s, dst, (size_t)n);
    trace_read(s, trace_start, trace_end, (uint64_t)n, (int64_t)n, (uint64_t)offset);
    return n;
}
int vfs_read(int fd, void *dst, int n)
{
    if (fd < 0 || fd >= OPEN_MAX || !slots[fd].used || n < 0) return -1;
    return read_slot(&slots[fd], dst, n);
}

int vfs_source_plan_acquire(P4VfsOriginKind layer, const char *path, P4VfsSourcePlan **out)
{
    if (!path || !out || *out || mounted || source_plans == OPEN_MAX ||
        (layer != P4_VFS_DISC && layer != P4_VFS_DEVELOPER) || vfs_source_prepare_selected()) return -1;
    char key[KEY_MAX], host[HOST_PATH_MAX];
    const char *rest;
    if (normalise(path, key, sizeof key) || !*key) return -1;
    int container = split_label(key, &rest), index = find_entry(rest, container);
    FILE *stream;
    P4OSFileIdentity identity;
    if (layer == P4_VFS_DISC) {
        if (!disc || index < 0) return 1;
        stream = disc; identity = source_disc_identity;
        if (entries[index].offset < 0 || entries[index].size < 0 ||
            (uint64_t)entries[index].offset > identity.size ||
            (uint64_t)entries[index].size > identity.size - (uint64_t)entries[index].offset) return -1;
    } else {
        if (!*source_developer.path) return 1;
        int found = resolve_root(&source_developer, key, host, sizeof host) == 0;
        if (!found && container < 0 && index >= 0 && entries[index].container < CONTAINERS) {
            char qualified[KEY_MAX + 6];
            if (full_key(&entries[index], qualified, sizeof qualified)) return -1;
            found = resolve_root(&source_developer, qualified, host, sizeof host) == 0;
        }
        if (!found) return 1;
        stream = p4_os_fopen_rb(host);
        if (!stream) return -1;
        if (p4_os_file_identity(stream, &identity) || identity.size > INT64_MAX) { fclose(stream); return -1; }
    }
    P4VfsSourcePlan *plan = malloc(sizeof *plan);
    if (!plan) { if (layer == P4_VFS_DEVELOPER) fclose(stream); return -1; }
    plan->slot.used = 1; plan->slot.fp = stream; plan->slot.owned = layer == P4_VFS_DEVELOPER;
    plan->slot.entry = index; plan->slot.layer = layer == P4_VFS_DEVELOPER ? -2 : -1;
    plan->slot.base = layer == P4_VFS_DISC ? entries[index].offset : 0;
    plan->slot.size = layer == P4_VFS_DISC ? entries[index].size : (long long)identity.size;
    plan->slot.pos = 0;
    if (index >= 0) {
        if (full_key(&entries[index], plan->slot.origin_key, sizeof plan->slot.origin_key)) {
            if (plan->slot.owned) fclose(stream);
            free(plan); return -1;
        }
    } else strcpy(plan->slot.origin_key, key);
    plan->info = (P4VfsSourceInfo){layer, plan->slot.origin_key, (uint64_t)plan->slot.base,
        (uint64_t)plan->slot.size, identity};
    plan->references = 1;
    ++source_plans; *out = plan;
    return 0;
}
const P4VfsSourceInfo *vfs_source_plan_info(const P4VfsSourcePlan *plan) { return plan ? &plan->info : NULL; }
int vfs_source_plan_retain(P4VfsSourcePlan *plan)
{
    if (!plan || !plan->references || plan->references == UINT_MAX) return -1;
    ++plan->references;
    return 0;
}
void vfs_source_plan_release(P4VfsSourcePlan *plan)
{
    if (!plan || --plan->references) return;
    if (plan->slot.owned) fclose(plan->slot.fp);
    --source_plans; free(plan);
}
int vfs_source_plan_read_at(P4VfsSourcePlan *plan, uint64_t offset, void *bytes, size_t size)
{
    if (!plan || !plan->references || (!bytes && size) || offset > plan->info.size ||
        (uint64_t)size > plan->info.size - offset) return -1;
    if (!size) return 0;
    if (!same_identity(plan->slot.fp, &plan->info.file)) return -1;
    plan->slot.pos = (long long)offset;
    uint8_t *output = bytes;
    size_t remaining = size;
    while (remaining) {
        int count = remaining > INT_MAX ? INT_MAX : (int)remaining;
        if (read_slot(&plan->slot, output, count) != count) return -1;
        remaining -= (size_t)count; output += count;
    }
    return same_identity(plan->slot.fp, &plan->info.file) ? 0 : -1;
}

long long vfs_seek(int fd, long long offset, int whence)
{
    slot_t *s;
    long long base;
    long long next;

    if (fd < 0 || fd >= OPEN_MAX || !slots[fd].used) {
        return -1;
    }
    s = &slots[fd];
    switch (whence) {
    case 0:
        base = 0;
        break;
    case 1:
        base = s->pos;
        break;
    case 2:
        base = s->size;
        break;
    default:
        return -1;
    }
    next = base + offset;
    if (next < 0) {
        return -1;
    }
    s->pos = next;
    return next;
}

int vfs_close(int fd)
{
    slot_t *s;

    if (fd < 0 || fd >= OPEN_MAX || !slots[fd].used) {
        return -1;
    }
    s = &slots[fd];
    if (s->owned) {
        fclose(s->fp);
    }
    memset(s, 0, sizeof *s);
    return 0;
}

/* Disc-only lookup: the path rules of vfs_open, but override files are never considered,
 * so the CD drive model sees exactly what the disc holds. */
int vfs_disc_locate(const char *path, long long *offset, long long *size)
{
    char key[KEY_MAX];
    const char *rest;
    int container;
    int idx;

    if (!mounted || normalise(path, key, sizeof key) != 0 || key[0] == '\0') {
        return -1;
    }
    container = split_label(key, &rest);
    idx = find_entry(rest, container);
    if (idx < 0) {
        return -1;
    }
    *offset = entries[idx].offset;
    *size = entries[idx].size;
    return 0;
}

int vfs_disc_extent(int fd, long long *offset, int *container)
{
    const slot_t *s;
    int source_container;

    if (fd < 0 || fd >= OPEN_MAX || !slots[fd].used)
        return -1;
    s = &slots[fd];
    if (s->entry < 0)
        return -1;
    source_container = entries[s->entry].container;
    *offset = entries[s->entry].offset;
    *container = source_container == CONTAINERS ? -1 : source_container;
    return 0;
}

int vfs_is_override(int fd)
{
    return fd >= 0 && fd < OPEN_MAX && slots[fd].used && slots[fd].owned;
}

int vfs_disc_read_sectors(unsigned int lsn, unsigned int sectors, void *dst)
{
    size_t want = (size_t)sectors * CD_SECTOR;
    uint64_t trace_start, trace_end;

    if (!mounted || !disc) {
        return -1;
    }
    trace_start = p4pc_prof_span_begin();
    if (iso_read_at(disc, (long long)lsn * CD_SECTOR, dst, want) != 0) {
        trace_end = p4pc_prof_span_stop(trace_start);
        p4pc_prof_span_record(trace_start, trace_end, "vfs.sectors.short", want, -1, (uint64_t)lsn * CD_SECTOR, "disc");
        return -1;
    }
    trace_end = p4pc_prof_span_stop(trace_start);
    if (read_observer && want)
        read_observer(&disc_origin, dst, want, -1, (int64_t)lsn * CD_SECTOR, read_observer_user);
    p4pc_prof_span_record(trace_start, trace_end, "vfs.sectors", want, (int64_t)want, (uint64_t)lsn * CD_SECTOR, "disc");
    return 0;
}

/* Directory listings and stat. A child of directory D is the next path component of an
 * entry's full key (container label included) below D, merged with the names in the
 * override directory's D. Indexed packages retain original host casing while
 * exposing canonical names; developer loose files must already be uppercase. */
#define NAME_CAP 256

typedef struct {
    char name[NAME_CAP];
    int is_dir;
    int from_override;
    long long size;
} item_t;

typedef struct {
    item_t *items;
    int n;
    int cap;
    int any;
} listing_t;

typedef struct {
    int used;
    item_t *items;
    int n;
    int pos;
} dir_slot_t;

static dir_slot_t dirs[OPEN_MAX];

/* Writes the full key of entry e: its container label (if any), then its path. */
static int full_key(const entry_t *e, char *buf, size_t cap)
{
    int n;

    if (e->container < CONTAINERS) {
        n = snprintf(buf, cap, "%s/%s", labels[e->container], e->path);
    } else {
        n = snprintf(buf, cap, "%s", e->path);
    }
    return (n < 0 || (size_t)n >= cap) ? -1 : 0;
}

/* If `full` lies strictly below directory `dir` (empty for the top), sets *name and *len to
 * its next path component and *more to whether it is a directory. Returns -1 otherwise. */
static int child_of(const char *dir, const char *full, const char **name, size_t *len, int *more)
{
    size_t dl = strlen(dir);
    const char *rest = full;
    const char *end;

    if (dl > 0) {
        if (strncmp(full, dir, dl) != 0 || full[dl] != '/') {
            return -1;
        }
        rest = full + dl + 1;
    }
    if (*rest == '\0') {
        return -1;
    }
    end = strchr(rest, '/');
    *name = rest;
    *len = end ? (size_t)(end - rest) : strlen(rest);
    *more = end != NULL;
    return 0;
}

static int add_item(listing_t *L, const char *name, size_t len, int is_dir, int from_override,
                    long long size)
{
    item_t *it;

    if (len == 0 || len >= NAME_CAP) {
        return 0;
    }
    if (L->n == L->cap) {
        int cap = L->cap ? L->cap * 2 : 256;
        item_t *grown = realloc(L->items, (size_t)cap * sizeof *grown);
        if (!grown) {
            return -1;
        }
        L->items = grown;
        L->cap = cap;
    }
    it = &L->items[L->n++];
    memcpy(it->name, name, len);
    it->name[len] = '\0';
    it->is_dir = is_dir;
    it->from_override = from_override;
    it->size = size;
    return 0;
}

/* Name order; for one name the override comes first, so it wins the dedupe. */
static int cmp_item(const void *a, const void *b)
{
    const item_t *x = a;
    const item_t *y = b;
    int c = strcmp(x->name, y->name);
    if (c) {
        return c;
    }
    return y->from_override - x->from_override;
}

typedef struct IndexedListing { listing_t *listing; int layer; } IndexedListing;
static int gather_indexed(const char *relative, int directory, void *user)
{
    IndexedListing *context = user;
    char canonical[KEY_MAX], host[HOST_PATH_MAX], name[NAME_CAP];
    const char *base = strrchr(relative, '/');
    size_t length = strlen(relative);
    if (length >= sizeof canonical) return -1;
    for (size_t i = 0; i <= length; i++) canonical[i] = upper_ascii((unsigned char)relative[i]);
    /* Do not stat or expose a shadowed lower-layer entry. */
    if (host_path(canonical, -1, -1, host, sizeof host) != context->layer) return 0;
    base = base ? base + 1 : relative;
    size_t name_length = strlen(base);
    if (name_length >= sizeof name) return -1;
    for (size_t i = 0; i <= name_length; i++) name[i] = upper_ascii((unsigned char)base[i]);
    long long size = 0;
    if (directory ? !p4_os_is_dir_nofollow(host)
                  : !p4_os_is_file_nofollow(host) || host_size(host, &size) != 0) return -1;
    return add_item(context->listing, name, name_length, directory, context->layer + 1, size);
}

/* Gathers the children of directory `dir` from both layers, one per name, sorted. Sets
 * L->any when `dir` exists in either layer (it has a child, or an override directory). */
static int gather(const char *dir, listing_t *L)
{
    char full[KEY_MAX + 64];
    char host_dir[HOST_PATH_MAX];
    const char *name;
    size_t len;
    int more;
    int w = 0;
    int n;
    DIR *d;
    struct dirent *de;

    if (dir[0]) {
        struct stat st;
        int winner = host_path(dir, -1, -1, host_dir, sizeof host_dir);
        if (winner >= 0 && (stat(host_dir, &st) != 0 || !S_ISDIR(st.st_mode) ||
            (override_roots[winner].index.resolve && !p4_os_is_dir_nofollow(host_dir)))) return -1;
    }
    for (int i = 0; i < n_entries; i++) {
        if (full_key(&entries[i], full, sizeof full) != 0 || child_of(dir, full, &name, &len, &more) != 0) {
            continue;
        }
        L->any = 1;
        if (add_item(L, name, len, more, 0, more ? 0 : entries[i].size) != 0) {
            return -1;
        }
    }

    for (int layer = 0; layer < override_count; layer++) {
        const OverrideRoot *root = &override_roots[layer];
        if (root->index.list) {
            IndexedListing context = {L, layer};
            int result = root->index.list(dir, gather_indexed, &context, root->index_user);
            if (result < 0) return -1;
            if (!result) L->any = 1;
            continue;
        }
        if (resolve_root(root, dir, host_dir, sizeof host_dir) != 0) continue;
        d = opendir(host_dir);
        if (d) {
            L->any = 1;
            while ((de = readdir(d)) != NULL) {
                char host[HOST_PATH_MAX];
                char upper[NAME_CAP];
                struct stat st;
                long long sz = 0;
                size_t nl = strlen(de->d_name);

                if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0 || nl >= NAME_CAP) {
                    continue;
                }
                for (size_t k = 0; k <= nl; k++) {
                    upper[k] = upper_ascii((unsigned char)de->d_name[k]);
                }
                if (strcmp(upper, de->d_name) != 0) continue;
                if (snprintf(host, sizeof host, "%s/%s", host_dir, de->d_name) >= (int)sizeof host ||
                    stat(host, &st) != 0) {
                    continue;
                }
                if (S_ISDIR(st.st_mode)) {
                    if (add_item(L, upper, nl, 1, layer + 1, 0) != 0) {
                        closedir(d);
                        return -1;
                    }
                } else if (S_ISREG(st.st_mode)) {
                    if (host_size(host, &sz) != 0) {
                        continue;
                    }
                    if (add_item(L, upper, nl, 0, layer + 1, sz) != 0) {
                        closedir(d);
                        return -1;
                    }
                }
            }
            closedir(d);
        }
    }

    if (L->n > 1) {
        qsort(L->items, (size_t)L->n, sizeof *L->items, cmp_item);
    }
    for (int i = 0; i < L->n; i++) {
        if (w > 0 && strcmp(L->items[w - 1].name, L->items[i].name) == 0) {
            continue;
        }
        L->items[w++] = L->items[i];
    }
    L->n = w;
    return 0;
}

int vfs_dir_open(const char *path)
{
    char key[KEY_MAX];
    listing_t L;
    int slot = -1;

    if (!mounted || normalise(path, key, sizeof key) != 0) {
        return -1;
    }
    for (int i = 0; i < OPEN_MAX; i++) {
        if (!dirs[i].used) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        return -1;
    }
    memset(&L, 0, sizeof L);
    if (gather(key, &L) != 0 || (!L.any && key[0] != '\0')) {
        free(L.items);
        return -1;
    }
    dirs[slot].used = 1;
    dirs[slot].items = L.items;
    dirs[slot].n = L.n;
    dirs[slot].pos = 0;
    return slot;
}

int vfs_dir_next(int dh, char *name, size_t cap, int *is_dir, long long *size)
{
    dir_slot_t *s;
    const item_t *it;

    if (dh < 0 || dh >= OPEN_MAX || !dirs[dh].used) {
        return -1;
    }
    s = &dirs[dh];
    if (s->pos >= s->n) {
        return 0;
    }
    it = &s->items[s->pos++];
    if (strlen(it->name) >= cap) {
        return -1;
    }
    memcpy(name, it->name, strlen(it->name) + 1);
    *is_dir = it->is_dir;
    *size = it->size;
    return 1;
}

int vfs_dir_close(int dh)
{
    if (dh < 0 || dh >= OPEN_MAX || !dirs[dh].used) {
        return -1;
    }
    free(dirs[dh].items);
    memset(&dirs[dh], 0, sizeof dirs[dh]);
    return 0;
}

int vfs_stat(const char *path, int *is_dir, long long *size)
{
    char key[KEY_MAX];
    char host[HOST_PATH_MAX];
    struct stat st;
    const char *rest;
    listing_t L;
    int container;
    int idx;
    int found;

    if (!mounted || normalise(path, key, sizeof key) != 0) {
        return -1;
    }
    if (key[0] == '\0') {
        *is_dir = 1;
        *size = 0;
        return 0;
    }
    container = split_label(key, &rest);
    idx = find_entry(rest, container);
    int layer = host_path(key, idx, container, host, sizeof host);
    if (layer >= 0) {
        if (stat(host, &st) != 0 || (override_roots[layer].index.resolve &&
            !p4_os_is_file_nofollow(host) && !p4_os_is_dir_nofollow(host))) return -1;
        if (S_ISREG(st.st_mode)) {
            *is_dir = 0;
            if (host_size(host, size) != 0) {
                return -1;
            }
            return 0;
        }
        if (S_ISDIR(st.st_mode)) {
            *is_dir = 1;
            *size = 0;
            return 0;
        }
        return -1;
    }
    if (idx >= 0) {
        *is_dir = 0;
        *size = entries[idx].size;
        return 0;
    }
    memset(&L, 0, sizeof L);
    found = gather(key, &L) == 0 && L.any;
    free(L.items);
    if (!found) {
        return -1;
    }
    *is_dir = 1;
    *size = 0;
    return 0;
}
