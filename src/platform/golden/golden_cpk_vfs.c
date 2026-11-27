/* Read-only Golden archive source; no game bytes are distributed with the port. */
#define _POSIX_C_SOURCE 200809L
#include "golden_cpk_vfs.h"
#include "p4pc/golden_source.h"
#include "p4pc/content_digest.h"
#include "p4pc/os.h"
#include "../files/vfs.h"
#include "../config/p4cfg.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARCHIVES 4
#define CACHE_SLOTS 8
#define CACHE_BYTES (64u << 20)
#define KEY_CAP 1024

typedef struct Archive { P4GoldenCpk cpk; P4OSFileIdentity identity; const char *name; } Archive;
typedef struct Cached {
    Archive *archive;
    P4GoldenCpkEntry entry;
    unsigned char *bytes;
    size_t size, refs;
    unsigned long long age;
} Cached;
typedef struct GoldenVfs { Archive archives[ARCHIVES]; Cached cache[CACHE_SLOTS]; size_t cache_bytes; unsigned long long clock; } GoldenVfs;
typedef struct OpenFile {
    Archive *archive;
    P4GoldenCpkEntry entry;
    Cached *cache;
    unsigned char *owned;
    const unsigned char *data;
    size_t size;
} OpenFile;
static GoldenVfs source;

static int same_file(Archive *archive)
{
    P4OSFileIdentity now;
    return !p4_os_file_identity(archive->cpk.file, &now) && now.size == archive->identity.size &&
           !memcmp(now.tokens, archive->identity.tokens, sizeof now.tokens);
}
static int read_raw(Archive *archive, uint64_t offset, void *bytes, size_t size)
{
    if (!same_file(archive) || offset > archive->identity.size || size > archive->identity.size - offset)
        return -1;
#if defined(_WIN32)
    if (offset > INT64_MAX || _fseeki64(archive->cpk.file, (__int64)offset, SEEK_SET)) return -1;
#else
    if (offset > INT64_MAX || fseeko(archive->cpk.file, (off_t)offset, SEEK_SET)) return -1;
#endif
    if (size && fread(bytes, 1, size, archive->cpk.file) != size) return -1;
    return same_file(archive) ? 0 : -1;
}
static void evict(Cached *item)
{
    source.cache_bytes -= item->size;
    free(item->bytes);
    memset(item, 0, sizeof *item);
}
static Cached *cache_get(Archive *archive, const P4GoldenCpkEntry *entry)
{
    for (size_t i = 0; i < CACHE_SLOTS; ++i) {
        Cached *item = &source.cache[i];
        if (item->bytes && item->archive == archive && item->entry.offset == entry->offset &&
            item->entry.size == entry->size && item->entry.extract == entry->extract) {
            item->refs++; item->age = ++source.clock; return item;
        }
    }
    return NULL;
}
static Cached *cache_put(Archive *archive, const P4GoldenCpkEntry *entry, unsigned char *bytes, size_t size)
{
    if (size > CACHE_BYTES) return NULL;
    while (1) {
        Cached *empty = NULL, *oldest = NULL;
        for (size_t i = 0; i < CACHE_SLOTS; ++i) {
            Cached *item = &source.cache[i];
            if (!item->bytes) empty = item;
            else if (!item->refs && (!oldest || item->age < oldest->age)) oldest = item;
        }
        if (empty && size <= CACHE_BYTES - source.cache_bytes) {
            *empty = (Cached){archive, *entry, bytes, size, 1, ++source.clock};
            source.cache_bytes += size;
            return empty;
        }
        if (!oldest) return NULL;
        evict(oldest);
    }
}
static int load(Archive *archive, const P4GoldenCpkEntry *entry, OpenFile *file)
{
    char reason[160];
    size_t size;
    unsigned char *bytes;
    if (!same_file(archive) || entry->offset > archive->identity.size ||
        entry->size > archive->identity.size - entry->offset) return -1;
    file->archive = archive; file->entry = *entry; file->size = entry->extract;
    if (entry->size == entry->extract) return 0;
    file->cache = cache_get(archive, entry);
    if (file->cache) { file->data = file->cache->bytes; return 0; }
    if (p4_golden_cpk_read(&archive->cpk, entry, &bytes, &size, reason, sizeof reason)) return -1;
    if (!same_file(archive)) { free(bytes); return -1; }
    file->cache = cache_put(archive, entry, bytes, size);
    if (!file->cache) file->owned = bytes;
    file->data = bytes;
    return 0;
}
static void release_file(void *user, void *handle)
{
    OpenFile *file = handle; (void)user;
    if (file->cache) --file->cache->refs;
    free(file->owned); free(file);
}
static int read_file(void *user, void *handle, size_t offset, void *bytes, size_t size)
{
    OpenFile *file = handle; (void)user;
    if (offset > file->size || size > file->size - offset || !same_file(file->archive)) return -1;
    if (file->data) { if (size) memcpy(bytes, file->data + offset, size); return 0; }
    return read_raw(file->archive, file->entry.offset + offset, bytes, size);
}
static int digest_file(OpenFile *file, char hex[65])
{
    unsigned char buffer[65536];
    P4ContentDigest digest;
    p4_content_digest_begin(&digest);
    for (size_t offset = 0; offset < file->size;) {
        size_t size = file->size - offset;
        if (size > sizeof buffer) size = sizeof buffer;
        if (read_file(NULL, file, offset, buffer, size)) return -1;
        p4_content_digest_update(&digest, buffer, size); offset += size;
    }
    p4_content_digest_end(&digest, hex);
    return 0;
}
static const char *strip_label(const char *path)
{
    static const char *const labels[] = {"DATA/", "BTL/", "BGM/", "ENV/", "ZZZZZ/"};
    for (size_t i = 0; i < sizeof labels / sizeof labels[0]; ++i) {
        size_t n = strlen(labels[i]);
        if (!strncmp(path, labels[i], n)) return path + n;
    }
    return path;
}
static int find(Archive *archive, const char *key, P4GoldenCpkEntry *entry, size_t *prefix)
{
    char path[KEY_CAP];
    size_t n = strlen(key);
    if (n >= sizeof path) return -1;
    memcpy(path, key, n + 1);
    for (;;) {
        int result = p4_golden_cpk_find(&archive->cpk, path, entry);
        if (result <= 0) { *prefix = strlen(path); return result; }
        char *slash = strrchr(path, '/');
        if (!slash) return 1;
        *slash = '\0';
    }
}
static int nested(OpenFile *file, const char *path)
{
    if (!file->data) {
        if (file->size > SIZE_MAX - 1) return -1;
        file->owned = malloc(file->size ? file->size : 1);
        if (!file->owned || read_raw(file->archive, file->entry.offset, file->owned, file->size)) return -1;
        file->data = file->owned;
    }
    while (*path) {
        const uint8_t *child = NULL;
        size_t child_size = 0, consumed;
        char candidate[KEY_CAP];
        if (strlen(path) >= sizeof candidate) return -1;
        strcpy(candidate, path);
        for (;;) {
            int a = p4_golden_bin_find(file->data, file->size, candidate, &child, &child_size);
            int b = a == 0 ? 1 : p4_golden_pak2_find(file->data, file->size, candidate, &child, &child_size);
            if (a == 0 || b == 0) break;
            char *slash = strrchr(candidate, '/');
            if (!slash) return -1;
            *slash = '\0';
        }
        consumed = strlen(candidate);
        file->data = child; file->size = child_size;
        if (child_size >= 8 && !memcmp(child, "CRILAYLA", 8)) {
            size_t plain_size = p4_golden_crilayla_size(child, child_size);
            unsigned char *plain = plain_size ? malloc(plain_size) : NULL;
            if (!plain || p4_golden_crilayla(child, child_size, plain, plain_size)) { free(plain); return -1; }
            free(file->owned); file->owned = plain;
            if (file->cache) { --file->cache->refs; file->cache = NULL; }
            file->data = plain; file->size = plain_size;
        }
        path += consumed;
        if (*path == '/') ++path;
    }
    return 0;
}
static int open_file(void *user, const char *key, size_t *size, void **handle,
                     char *chain, size_t chain_capacity, char sha256[65])
{
    GoldenVfs *vfs = user;
    const char *keys[2] = {key, strip_label(key)};
    for (size_t k = 0; k < 2; ++k) {
        if (k && keys[0] == keys[1]) break;
        for (size_t i = 0; i < ARCHIVES; ++i) {
            Archive *archive = &vfs->archives[i];
            P4GoldenCpkEntry entry;
            size_t prefix = 0;
            int found = find(archive, keys[k], &entry, &prefix);
            if (found < 0) return -1;
            if (found) continue;
            OpenFile *file = calloc(1, sizeof *file);
            if (!file) return -1;
            if (load(archive, &entry, file) || (keys[k][prefix] && nested(file, keys[k] + prefix + 1)) ||
                digest_file(file, sha256) || snprintf(chain, chain_capacity, "%s::%s", archive->name, keys[k]) >= (int)chain_capacity) {
                release_file(user, file); return -1;
            }
            *size = file->size; *handle = file;
            fprintf(stderr, "Golden VFS: %s -> %s size=%llu sha256=%s\n", key, chain, (unsigned long long)*size, sha256);
            return 0;
        }
    }
    fprintf(stderr, "Golden VFS: refused %s\n", key);
    return 1;
}
static int list_files(void *user, const char *directory, P4VfsVirtualVisitor visitor, void *visitor_user)
{
    GoldenVfs *vfs = user;
    static const char *const labels[] = {"DATA", "BTL", "BGM", "ENV", "ZZZZZ"};
    const char *dir = strip_label(directory);
    if (!*directory) {
        for (size_t i = 0; i < sizeof labels / sizeof labels[0]; ++i)
            if (visitor(labels[i], 1, 0, visitor_user)) return -1;
    } else {
        for (size_t i = 0; i < sizeof labels / sizeof labels[0]; ++i)
            if (!strcmp(directory, labels[i])) { dir = ""; break; }
    }
    size_t dir_len = strlen(dir);
    int any = !*dir;
    for (size_t a = 0; a < ARCHIVES; ++a) {
        P4GoldenCpk *cpk = &vfs->archives[a].cpk;
        if (!same_file(&vfs->archives[a])) return -1;
        for (uint32_t row = 0; row < cpk->utf.row_count; ++row) {
            const char *dir_name, *file_name;
            char path[KEY_CAP];
            uint64_t size;
            if (p4_golden_utf_str(&cpk->utf, row, "DirName", &dir_name) ||
                p4_golden_utf_str(&cpk->utf, row, "FileName", &file_name) ||
                p4_golden_utf_u64(&cpk->utf, row, "ExtractSize", &size) || size > LLONG_MAX) return -1;
            int n = snprintf(path, sizeof path, "%s%s%s", dir_name, *dir_name ? "/" : "", file_name);
            if (n < 0 || (size_t)n >= sizeof path) return -1;
            for (int i = 0; i < n; ++i)
                if (path[i] >= 'a' && path[i] <= 'z') path[i] = (char)(path[i] - 'a' + 'A');
            if (dir_len && (strncmp(path, dir, dir_len) || path[dir_len] != '/')) continue;
            char *child = path + dir_len + !!dir_len;
            char *slash = strchr(child, '/');
            if (slash) *slash = '\0';
            if (!*child) continue;
            any = 1;
            if (visitor(child, slash != NULL, slash ? 0 : (long long)size, visitor_user)) return -1;
        }
    }
    return any ? 0 : 1;
}
static int register_source(const char *directory, int hybrid, int dormant)
{
    static const char *const names[ARCHIVES] = {"data_e.cpk", "data.cpk", "data_movie.cpk", "sysdat.cpk"};
    char path[1200], reason[160];
    P4VfsVirtualSource api = {open_file, read_file, release_file, list_files, &source, hybrid};
    if (!directory || !*directory) return -1;
#ifdef NDEBUG
    if (hybrid) return -1;
#endif
    if (source.archives[0].cpk.file) return -1;
    memset(&source, 0, sizeof source);
    for (size_t i = 0; i < ARCHIVES; ++i) {
        source.archives[i].name = names[i];
        int n = snprintf(path, sizeof path, "%s/%s", directory, names[i]);
        if (n < 0 || (size_t)n >= sizeof path || p4_golden_cpk_open(&source.archives[i].cpk, path, reason, sizeof reason) ||
            p4_os_file_identity(source.archives[i].cpk.file, &source.archives[i].identity)) goto fail;
    }
    if (dormant ? vfs_virtual_register_dormant(&api) : vfs_virtual_register(&api)) goto fail;
    return 0;
fail:
    for (size_t i = 0; i < ARCHIVES; ++i) p4_golden_cpk_close(&source.archives[i].cpk);
    return -1;
}

int p4_golden_cpk_vfs_register(const char *directory, int hybrid)
{
    return register_source(directory, hybrid, 0);
}

int p4_golden_cpk_vfs_register_dormant(const char *directory)
{
    int result = register_source(directory, 0, 1);
    if (!result) fprintf(stderr, "Golden VFS: dormant witness source registered; PS2 lookup unchanged\n");
    return result;
}

int p4_golden_cpk_vfs_start(void)
{
    char directory[1024];
    const char *setting = getenv("P4PC_GOLDEN_HYBRID");
    int hybrid = setting && *setting && strcmp(setting, "0");
#ifdef NDEBUG
    if (hybrid) {
        fprintf(stderr, "Golden VFS: hybrid fallback is unavailable in release builds\n");
        return -1;
    }
#endif
    if (p4_golden_locate(p4cfg_get("golden.install"), directory, sizeof directory)) return -1;
    if (hybrid) fprintf(stderr, "Golden VFS: HYBRID development disc fallback enabled\n");
    return p4_golden_cpk_vfs_register(directory, hybrid);
}

int p4_golden_cpk_vfs_dormant_requested(void)
{
    return getenv("P4PC_GOLDEN_VFS_DORMANT") != NULL;
}

int p4_golden_cpk_vfs_start_dormant(void)
{
    char directory[1024];
    if (p4_golden_locate(p4cfg_get("golden.install"), directory, sizeof directory)) return -1;
    return p4_golden_cpk_vfs_register_dormant(directory);
}
