/* Port session identity and crash markers for mods (include/p4pc/mod_lifecycle.h). */
#include "p4pc/mod_lifecycle.h"
#include "p4pc/os.h"
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_CAP 1200
#define HEADER "P4PC-CRASH 1 session="

typedef struct {
    char id[65];
    char tag[P4_MOD_LIFECYCLE_TAG_MAX + 1];
} Tag;

static char *root;
static uint64_t session;
static Tag tags[P4_MOD_LIFECYCLE_TAGGED];
static unsigned tag_count;

static int fail(char *reason, size_t capacity, const char *format, ...)
{
    if (reason && capacity) {
        va_list args;
        va_start(args, format);
        vsnprintf(reason, capacity, format, args);
        va_end(args);
    }
    return -1;
}
static int path_of(char out[PATH_CAP], const char *leaf)
{
    int n = snprintf(out, PATH_CAP, "%s/mod-lifecycle%s%s", root, leaf ? "/" : "", leaf ? leaf : "");
    return n > 0 && n < PATH_CAP ? 0 : -1;
}
static int marker_path(uint64_t id, char out[PATH_CAP])
{
    char leaf[48];
    snprintf(leaf, sizeof leaf, "crash-%" PRIu64 ".marker", id);
    return path_of(out, leaf);
}

int p4_mod_lifecycle_root(const char *directory)
{
    char *copy = NULL;
    if (directory) {
        size_t size = strlen(directory);
        if (!size || size >= PATH_CAP - 64 || !(copy = malloc(size + 1))) return -1;
        memcpy(copy, directory, size + 1);
    }
    p4_os_crash_marker_close();
    free(root);
    root = copy;
    session = 0;
    tag_count = 0;
    return 0;
}

/* Reads a whole small file: its size, or -1 (missing: errno ENOENT). */
static long read_small(const char *path, char *out, size_t capacity)
{
    FILE *stream = fopen(path, "rb");
    if (!stream) return -1;
    size_t size = fread(out, 1, capacity, stream);
    int bad = ferror(stream);
    fclose(stream);
    if (bad || size >= capacity) {
        errno = EIO;
        return -1;
    }
    out[size] = 0;
    return (long)size;
}
static int write_replace(const char *path, const char *bytes, size_t length)
{
    char temporary[PATH_CAP];
    int n = snprintf(temporary, sizeof temporary, "%s.tmp", path);
    if (n <= 0 || n >= PATH_CAP) return -1;
    FILE *stream = fopen(temporary, "wb");
    if (!stream) return -1;
    int bad = fwrite(bytes, 1, length, stream) != length;
    if (p4_os_sync_file(stream)) bad = 1;
    if (fclose(stream)) bad = 1;
    if (bad || p4_os_replace_file(temporary, path)) {
        remove(temporary);
        return -1;
    }
    return 0;
}

/* The bytes the crash handler writes: the session, then each tagged mod. */
static int publish(void)
{
    char bytes[P4_OS_CRASH_MARKER_CAP];
    int length = snprintf(bytes, sizeof bytes, HEADER "%" PRIu64 "\n", session);
    for (unsigned i = 0; i < tag_count; ++i)
        length += snprintf(bytes + length, sizeof bytes - (size_t)length, "%s\t%s\n", tags[i].id, tags[i].tag);
    return p4_os_crash_marker_set(bytes, (size_t)length);
}

typedef struct {
    uint64_t kept[P4_MOD_LIFECYCLE_KEPT + 1];
    unsigned count;
} Prune;
static void prune_visit(const char *name, void *user)
{
    Prune *prune = user;
    char *end;
    if (strncmp(name, "crash-", 6) || name[6] < '0' || name[6] > '9') return;
    uint64_t id = strtoull(name + 6, &end, 10);
    if (strcmp(end, ".marker") || id == session) return;
    char path[PATH_CAP], bytes[8];
    if (marker_path(id, path)) return;
    FILE *stream = fopen(path, "rb");
    if (!stream) return;
    size_t size = fread(bytes, 1, sizeof bytes, stream);
    fclose(stream);
    /* Empty: that session ended without a crash record. */
    if (!size) {
        remove(path);
        return;
    }
    /* Keep the newest recorded crashes; the oldest beyond the bound goes. */
    unsigned at = prune->count;
    while (at && prune->kept[at - 1] > id) {
        prune->kept[at] = prune->kept[at - 1];
        --at;
    }
    prune->kept[at] = id;
    if (++prune->count > P4_MOD_LIFECYCLE_KEPT) {
        if (!marker_path(prune->kept[0], path)) remove(path);
        memmove(prune->kept, prune->kept + 1, P4_MOD_LIFECYCLE_KEPT * sizeof prune->kept[0]);
        --prune->count;
    }
}

uint64_t p4_mod_lifecycle_session(char *reason, size_t capacity)
{
    if (session) return session;
    if (!root) return (uint64_t)(fail(reason, capacity, "No settings directory: this run has no lifecycle session"), 0);
    char directory[PATH_CAP], counter[PATH_CAP], marker[PATH_CAP], text[32];
    if (path_of(directory, NULL) || path_of(counter, "session"))
        return (uint64_t)(fail(reason, capacity, "Lifecycle path is too long"), 0);
    if (p4_os_mkdir(directory) && errno != EEXIST)
        return (uint64_t)(fail(reason, capacity, "Cannot create the mod-lifecycle folder"), 0);
    uint64_t previous = 0;
    long size = read_small(counter, text, sizeof text);
    if (size < 0 && errno != ENOENT)
        return (uint64_t)(fail(reason, capacity, "Cannot read the lifecycle session counter"), 0);
    if (size >= 0) {
        char *end;
        previous = strtoull(text, &end, 10);
        if (end == text || strcmp(end, "\n") || previous == UINT64_MAX)
            return (uint64_t)(fail(reason, capacity, "The lifecycle session counter is corrupt"), 0);
    }
    uint64_t next = previous + 1;
    int length = snprintf(text, sizeof text, "%" PRIu64 "\n", next);
    /* The counter is durable before the session is used anywhere. */
    if (write_replace(counter, text, (size_t)length))
        return (uint64_t)(fail(reason, capacity, "Cannot write the lifecycle session counter"), 0);
    if (marker_path(next, marker) || p4_os_crash_marker_open(marker))
        return (uint64_t)(fail(reason, capacity, "Cannot create the crash marker of this session"), 0);
    session = next;
    tag_count = 0;
    if (publish()) {
        p4_os_crash_marker_close();
        session = 0;
        return (uint64_t)(fail(reason, capacity, "Cannot arm the crash marker of this session"), 0);
    }
    Prune prune = {0};
    p4_os_list_dir(directory, prune_visit, &prune);
    return session;
}

static int tag_valid(const char *tag)
{
    size_t size = strlen(tag);
    if (!size || size > P4_MOD_LIFECYCLE_TAG_MAX) return 0;
    for (size_t i = 0; i < size; ++i)
        if (tag[i] < 0x20 || tag[i] > 0x7e) return 0;
    return 1;
}
int p4_mod_lifecycle_tag(const char *mod_id, const char *tag, char *reason, size_t capacity)
{
    if (!mod_id || strlen(mod_id) >= sizeof tags[0].id) return fail(reason, capacity, "Invalid mod id");
    if (tag && !tag_valid(tag)) return fail(reason, capacity, "Lifecycle tags are 1..63 printable ASCII bytes");
    if (!p4_mod_lifecycle_session(reason, capacity)) return -1;
    unsigned i = 0;
    while (i < tag_count && strcmp(tags[i].id, mod_id)) ++i;
    Tag saved[P4_MOD_LIFECYCLE_TAGGED];
    unsigned saved_count = tag_count;
    memcpy(saved, tags, sizeof tags);
    if (!tag) {
        if (i == tag_count) return 0;
        memmove(tags + i, tags + i + 1, (tag_count - i - 1) * sizeof tags[0]);
        --tag_count;
    } else {
        if (i == tag_count) {
            if (tag_count == P4_MOD_LIFECYCLE_TAGGED) return fail(reason, capacity, "Too many mods hold a lifecycle tag");
            strcpy(tags[tag_count++].id, mod_id);
        }
        strcpy(tags[i].tag, tag);
    }
    if (publish()) {
        memcpy(tags, saved, sizeof tags);
        tag_count = saved_count;
        return fail(reason, capacity, "Cannot arm the crash marker of this session");
    }
    return 0;
}

/* Another session's marker: 1 recorded (bytes hold it), 0 none, -1 bad. */
static int read_marker(uint64_t id, char *bytes, size_t capacity, char *reason, size_t reason_capacity)
{
    char path[PATH_CAP], header[48];
    if (!root) return fail(reason, reason_capacity, "No settings directory: there are no lifecycle records");
    if (!id || id == session) return 0;
    if (marker_path(id, path)) return fail(reason, reason_capacity, "Lifecycle path is too long");
    long size = read_small(path, bytes, capacity);
    if (size < 0) return errno == ENOENT ? 0 : fail(reason, reason_capacity, "Cannot read crash marker %" PRIu64, id);
    if (!size) return 0;
    int length = snprintf(header, sizeof header, HEADER "%" PRIu64 "\n", id);
    if (strncmp(bytes, header, (size_t)length) || bytes[size - 1] != '\n')
        return fail(reason, reason_capacity, "Crash marker %" PRIu64 " is corrupt or belongs to another session", id);
    return 1;
}
static char *tag_line(char *bytes, const char *mod_id)
{
    size_t id_length = strlen(mod_id);
    for (char *line = strchr(bytes, '\n') + 1; *line; line = strchr(line, '\n') + 1)
        if (!strncmp(line, mod_id, id_length) && line[id_length] == '\t') return line;
    return NULL;
}
int p4_mod_lifecycle_crashed(uint64_t id, const char *mod_id, char *tag, size_t tag_capacity,
                             char *reason, size_t capacity)
{
    char bytes[P4_OS_CRASH_MARKER_CAP + 1];
    int found = read_marker(id, bytes, sizeof bytes, reason, capacity);
    if (found <= 0) return found;
    if (tag && tag_capacity) {
        char *line = tag_line(bytes, mod_id);
        size_t length = 0;
        if (line) {
            line += strlen(mod_id) + 1;
            length = (size_t)(strchr(line, '\n') - line);
        }
        if (length >= tag_capacity) return fail(reason, capacity, "Lifecycle tag exceeds buffer");
        memcpy(tag, line ? line : "", length);
        tag[length] = 0;
    }
    return 1;
}
int p4_mod_lifecycle_consume(uint64_t id, const char *mod_id, char *reason, size_t capacity)
{
    char bytes[P4_OS_CRASH_MARKER_CAP + 1], path[PATH_CAP];
    int found = read_marker(id, bytes, sizeof bytes, reason, capacity);
    if (found <= 0) return found;
    char *line = tag_line(bytes, mod_id);
    if (line) {
        char *next = strchr(line, '\n') + 1;
        memmove(line, next, strlen(next) + 1);
    }
    if (marker_path(id, path)) return fail(reason, capacity, "Lifecycle path is too long");
    /* The last mod's record consumed: the marker goes. */
    if (!strchr(strchr(bytes, '\n') + 1, '\n')) {
        if (remove(path) && errno != ENOENT) return fail(reason, capacity, "Cannot delete crash marker %" PRIu64, id);
        return 0;
    }
    if (line && write_replace(path, bytes, strlen(bytes))) return fail(reason, capacity, "Cannot rewrite crash marker %" PRIu64, id);
    return 0;
}
