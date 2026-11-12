/* Mod-owned artifact files beside p4pc.ini (p4pc/mod_artifacts.h). */
#include "p4pc/mod_artifacts.h"
#include "p4pc/os.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_CAP 1400

static char *root;

static int fail(char *reason, size_t capacity, const char *format, ...)
{
    if (reason && capacity) {
        va_list arguments;
        va_start(arguments, format);
        vsnprintf(reason, capacity, format, arguments);
        va_end(arguments);
    }
    return -1;
}
int p4_mod_artifact_root(const char *directory)
{
    char *copy = NULL;
    if (directory) {
        size_t size = strlen(directory);
        if (!size || size >= PATH_CAP - 256 || !(copy = malloc(size + 1))) return -1;
        memcpy(copy, directory, size + 1);
    }
    free(root);
    root = copy;
    return 0;
}
static int id_valid(const char *id)
{
    size_t size = id ? strlen(id) : 0;
    if (!size || size > 63 || id[0] == '.') return 0;
    for (size_t i = 0; i < size; ++i) {
        char c = id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) return 0;
    }
    return 1;
}
static char lower(char c) { return c >= 'A' && c <= 'Z' ? (char)(c - 'A' + 'a') : c; }
/* Windows device names stay devices with any extension ("nul.txt"). */
static int reserved(const char *segment, size_t size)
{
    static const char *const names[] = {"con", "prn", "aux", "nul"};
    size_t base = 0;
    while (base < size && segment[base] != '.') ++base;
    char stem[5] = {0};
    if (base != 3 && base != 4) return 0;
    for (size_t i = 0; i < base; ++i) stem[i] = lower(segment[i]);
    if (base == 3) {
        for (size_t i = 0; i < sizeof names / sizeof *names; ++i)
            if (!strcmp(stem, names[i])) return 1;
        return 0;
    }
    return (!strncmp(stem, "com", 3) || !strncmp(stem, "lpt", 3)) && stem[3] >= '1' && stem[3] <= '9';
}
static int segment_valid(const char *segment, size_t size)
{
    if (!size || size > 64 || segment[0] == '.' || segment[size - 1] == '.' || reserved(segment, size)) return 0;
    for (size_t i = 0; i < size; ++i) {
        char c = segment[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == '.' || c == '_' || c == '-'))
            return 0;
    }
    return 1;
}
int p4_mod_artifact_path_valid(const char *path)
{
    size_t size = path ? strlen(path) : 0, segments = 0;
    if (!size || size > P4_MOD_ARTIFACT_PATH_MAX) return 0;
    for (size_t start = 0; start <= size;) {
        size_t end = start;
        while (end < size && path[end] != '/') ++end;
        if (!segment_valid(path + start, end - start) || ++segments > P4_MOD_ARTIFACT_SEGMENTS) return 0;
        start = end + 1;
    }
    return 1;
}

typedef struct CaseCheck { const char *name; size_t size; int exact, clash; } CaseCheck;
static void case_visit(const char *name, void *user)
{
    CaseCheck *check = user;
    if (strlen(name) != check->size) return;
    int same = 1, exact = 1;
    for (size_t i = 0; i < check->size; ++i) {
        if (lower(name[i]) != lower(check->name[i])) same = 0;
        if (name[i] != check->name[i]) exact = 0;
    }
    if (same && exact) check->exact = 1;
    else if (same) check->clash = 1;
}
/* 0 when directory has no entry differing from name only by case; -1 otherwise. */
static int case_unique(const char *directory, const char *name, size_t size)
{
    CaseCheck check = {name, size, 0, 0};
    if (p4_os_list_dir(directory, case_visit, &check)) return errno == ENOENT ? 0 : -1;
    return check.clash ? -1 : 0;
}
static int join(char out[PATH_CAP], const char *base, const char *leaf, size_t size)
{
    int n = snprintf(out, PATH_CAP, "%s/%.*s", base, (int)size, leaf);
    return n > 0 && n < PATH_CAP ? 0 : -1;
}
/* A real directory (never a link/reparse point), created when absent. */
static int plain_directory(const char *path, int create)
{
    if (p4_os_is_dir_nofollow(path)) return 0;
    if (!create) return -1;
    if (p4_os_mkdir(path) == 0) return p4_os_is_dir_nofollow(path) ? 0 : -1;
    return -1;
}
/* The mod folder, and the file's parent directory walked segment by segment. */
static int resolve(const char *mod_id, const char *path, int create, char folder[PATH_CAP],
                   char file[PATH_CAP], char *reason, size_t capacity)
{
    char base[PATH_CAP];
    if (!root) return fail(reason, capacity, "Mod artifacts are not persistent in this run");
    if (!id_valid(mod_id)) return fail(reason, capacity, "Invalid mod id");
    if (!p4_mod_artifact_path_valid(path)) return fail(reason, capacity, "Invalid artifact path %s", path ? path : "");
    if (snprintf(base, sizeof base, "%s/mod-artifacts", root) >= PATH_CAP ||
        join(folder, base, mod_id, strlen(mod_id)))
        return fail(reason, capacity, "Artifact path is too long");
    if (plain_directory(root, 0)) return fail(reason, capacity, "The settings folder is not a plain directory");
    if (plain_directory(base, create) || plain_directory(folder, create))
        return fail(reason, capacity, create ? "Cannot create the mod-artifacts folder" : "No artifacts");
    char directory[PATH_CAP];
    strcpy(directory, folder);
    size_t size = strlen(path);
    for (size_t start = 0;;) {
        size_t end = start;
        while (end < size && path[end] != '/') ++end;
        if (case_unique(directory, path + start, end - start))
            return fail(reason, capacity, "Artifact %s differs only by case from an existing name", path);
        char next[PATH_CAP];
        if (join(next, directory, path + start, end - start)) return fail(reason, capacity, "Artifact path is too long");
        if (end == size) {
            strcpy(file, next);
            return 0;
        }
        if (plain_directory(next, create))
            return fail(reason, capacity, create ? "Cannot create the artifact folder for %s" : "No artifact %s", path);
        strcpy(directory, next);
        start = end + 1;
    }
}

static long file_size(const char *path)
{
    FILE *stream = fopen(path, "rb");
    if (!stream) return -1;
    long size = fseek(stream, 0, SEEK_END) ? -1 : ftell(stream);
    fclose(stream);
    return size;
}
typedef struct Usage { char directory[PATH_CAP]; uint64_t bytes; unsigned files, depth; int bad; } Usage;
static void usage_visit(const char *name, void *user)
{
    Usage *usage = user;
    if (!strcmp(name, ".") || !strcmp(name, "..") || usage->bad) return;
    char path[PATH_CAP];
    if (join(path, usage->directory, name, strlen(name))) { usage->bad = 1; return; }
    if (p4_os_is_file_nofollow(path)) {
        long size = file_size(path);
        if (size < 0) { usage->bad = 1; return; }
        usage->bytes += (uint64_t)size;
        ++usage->files;
    } else if (p4_os_is_dir_nofollow(path) && usage->depth < P4_MOD_ARTIFACT_SEGMENTS) {
        char saved[PATH_CAP];
        strcpy(saved, usage->directory);
        strcpy(usage->directory, path);
        ++usage->depth;
        if (p4_os_list_dir(path, usage_visit, usage)) usage->bad = 1;
        --usage->depth;
        strcpy(usage->directory, saved);
    }
}

int p4_mod_artifact_write(const char *mod_id, const char *path, const void *bytes, size_t size,
                          char *reason, size_t reason_capacity)
{
    char folder[PATH_CAP], file[PATH_CAP], temporary[PATH_CAP];
    if (size > P4_MOD_ARTIFACT_FILE_MAX) return fail(reason, reason_capacity, "Artifacts are at most 4 MiB");
    if (size && !bytes) return fail(reason, reason_capacity, "Missing artifact bytes");
    if (resolve(mod_id, path, 1, folder, file, reason, reason_capacity)) return -1;
    int exists = p4_os_is_file_nofollow(file);
    long old_size = exists ? file_size(file) : 0;
    if (!exists && (p4_os_is_dir_nofollow(file) || file_size(file) >= 0))
        return fail(reason, reason_capacity, "Artifact %s is not a plain file", path);
    if (old_size < 0) return fail(reason, reason_capacity, "Cannot read artifact %s", path);
    Usage usage = {.directory = {0}};
    strcpy(usage.directory, folder);
    if (p4_os_list_dir(folder, usage_visit, &usage) || usage.bad)
        return fail(reason, reason_capacity, "Cannot measure the artifacts of %s", mod_id);
    uint64_t total = usage.bytes - (uint64_t)old_size + size;
    if (!exists && usage.files >= P4_MOD_ARTIFACT_FILES_MAX)
        return fail(reason, reason_capacity, "A mod keeps at most 256 artifacts");
    if (total > P4_MOD_ARTIFACT_TOTAL_MAX)
        return fail(reason, reason_capacity, "A mod keeps at most 64 MiB of artifacts");
    /* The temporary name starts with a dot, which no artifact path can. */
    const char *leaf = strrchr(file, '/') + 1;
    int n = snprintf(temporary, sizeof temporary, "%.*s.%s.tmp", (int)(leaf - file), file, leaf);
    if (n <= 0 || n >= PATH_CAP) return fail(reason, reason_capacity, "Artifact path is too long");
    if (file_size(temporary) >= 0 && !p4_os_is_file_nofollow(temporary))
        return fail(reason, reason_capacity, "Artifact %s has a foreign temporary file", path);
    FILE *stream = fopen(temporary, "wb");
    if (!stream) return fail(reason, reason_capacity, "Cannot write artifact %s", path);
    int bad = size && fwrite(bytes, 1, size, stream) != size;
    if (fflush(stream) || p4_os_sync_file(stream)) bad = 1;
    if (fclose(stream)) bad = 1;
    if (bad || p4_os_replace_file(temporary, file)) {
        remove(temporary);
        return fail(reason, reason_capacity, "Cannot write artifact %s", path);
    }
    return 0;
}
int64_t p4_mod_artifact_read(const char *mod_id, const char *path, void *out, size_t capacity,
                             char *reason, size_t reason_capacity)
{
    char folder[PATH_CAP], file[PATH_CAP];
    if (!root || !id_valid(mod_id) || !p4_mod_artifact_path_valid(path)) {
        fail(reason, reason_capacity, "Invalid or unavailable artifact");
        return root ? -2 : -1;
    }
    char ignored[96];
    if (resolve(mod_id, path, 0, folder, file, ignored, sizeof ignored)) return -1;
    if (!p4_os_is_file_nofollow(file)) return -1;
    FILE *stream = fopen(file, "rb");
    if (!stream) return -1;
    long size = fseek(stream, 0, SEEK_END) ? -1 : ftell(stream);
    if (size < 0 || (uint64_t)size > capacity || fseek(stream, 0, SEEK_SET)) {
        fclose(stream);
        fail(reason, reason_capacity, "Artifact %s is unreadable or larger than requested", path);
        return -2;
    }
    if (!out) {
        fclose(stream);
        return size;
    }
    size_t got = size ? fread(out, 1, (size_t)size, stream) : 0;
    int bad = ferror(stream) || got != (size_t)size;
    fclose(stream);
    if (bad) {
        fail(reason, reason_capacity, "Cannot read artifact %s", path);
        return -2;
    }
    return size;
}
int p4_mod_artifact_folder(const char *mod_id, char *out, size_t capacity)
{
    if (!root || !id_valid(mod_id) || !out) return -1;
    int n = snprintf(out, capacity, "%s/mod-artifacts/%s", root, mod_id);
    return n > 0 && (size_t)n < capacity ? 0 : -1;
}
