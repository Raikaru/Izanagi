#include "file_index.h"
#include "compendium.h"
#include "package_memory_internal.h"
#include "package_sort.h"
#include "p4pc/os.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEY_MAX 512
#define HOST_MAX 1200
#define RELATIVE_MAX 1024

typedef union Name { uint32_t offset; const char *text; } Name;
typedef struct Entry { Name name; unsigned directory; } Entry;
struct P4PackageFiles {
    Entry *entries;
    char *strings;
    size_t count, capacity, used, string_capacity, limit;
    int tracked;
};
static unsigned char upper(unsigned char c) { return c >= 'a' && c <= 'z' ? (unsigned char)(c - 'a' + 'A') : c; }
int p4_package_key_compare(const char *left, const char *right)
{
    while (*left && upper((unsigned char)*left) == upper((unsigned char)*right)) { left++; right++; }
    return (int)upper((unsigned char)*left) - (int)upper((unsigned char)*right);
}
size_t p4_package_files_memory(const P4PackageFiles *files)
{
    return files ? sizeof *files + files->capacity * sizeof *files->entries + files->string_capacity : 0;
}
static int reserve_entries(P4PackageFiles *files)
{
    if (files->count < files->capacity) return 0;
    size_t capacity = files->capacity ? files->capacity * 2 : 8;
    if (capacity < files->capacity || capacity > SIZE_MAX / sizeof *files->entries) return -1;
    size_t bytes = capacity * sizeof *files->entries;
    if (bytes > files->limit - sizeof *files || files->string_capacity > files->limit - sizeof *files - bytes) return -1;
    Entry *entries = files->tracked ? p4_package_metadata_resize(files->entries,
        files->capacity * sizeof *entries, bytes) : realloc(files->entries, bytes);
    if (!entries) return -1;
    files->entries = entries;
    files->capacity = capacity;
    return 0;
}
static int append(P4PackageFiles *files, const char *relative, int directory)
{
    size_t size = strlen(relative) + 1;
    if (size > UINT32_MAX - files->used || reserve_entries(files)) return -1;
    size_t needed = files->used + size;
    if (needed > files->string_capacity) {
        size_t capacity = files->string_capacity ? files->string_capacity : 256;
        while (capacity < needed) {
            if (capacity > SIZE_MAX / 2) return -1;
            capacity *= 2;
        }
        size_t overhead = sizeof *files + files->capacity * sizeof *files->entries;
        if (capacity > files->limit - overhead) return -1;
        char *strings = files->tracked ? p4_package_metadata_resize(files->strings,
            files->string_capacity, capacity) : realloc(files->strings, capacity);
        if (!strings) return -1;
        files->strings = strings;
        files->string_capacity = capacity;
    }
    Entry *entry = &files->entries[files->count++];
    entry->name.offset = (uint32_t)files->used;
    entry->directory = !!directory;
    memcpy(files->strings + files->used, relative, size);
    files->used = needed;
    return 0;
}
static int container(const char *name)
{
    static const char *const labels[] = {"DATA", "BTL", "BGM", "ENV", "ZZZZZ"};
    for (size_t i = 0; i < sizeof labels / sizeof labels[0]; i++) if (!p4_package_key_compare(name, labels[i])) return 1;
    return 0;
}
static int valid_component(const char *name, unsigned flags)
{
    for (const unsigned char *p = (const unsigned char *)name; *p; p++) {
        if (*p < 32 || *p == 127 || strchr(":;/\\", *p) ||
            (*p >= 128 && !(flags & P4_PACKAGE_FILES_PROVIDER))) return 0;
        if (!(flags & P4_PACKAGE_FILES_CASEFOLD) && *p >= 'a' && *p <= 'z') return 0;
    }
    return name[0] != 0;
}
typedef struct Scan {
    P4PackageFiles *files;
    const char *root, *owner, *relative;
    unsigned flags, depth;
    int exposed;
    int *failed;
} Scan;
static void failure(Scan *scan, const char *path, const char *reason)
{
    fprintf(stderr, "p4pc: mods: %s: %s: %s\n", scan->owner, path, reason);
    *scan->failed = 1;
}
static int empty_prebuild(const char *path, const char *name)
{
    if (p4_package_key_compare(name, "prebuild.bat") || !p4_os_is_file_nofollow(path)) return 0;
    FILE *file = p4_os_fopen_rb(path);
    if (!file) return 0;
    int empty = fgetc(file) == EOF && !ferror(file);
    fclose(file);
    return empty;
}
static void scan_entry(const char *name, void *user)
{
    Scan *scan = user;
    char relative[RELATIVE_MAX], host[HOST_MAX];
    if (*scan->failed || !strcmp(name, ".") || !strcmp(name, "..")) return;
    int n = scan->relative[0] ? snprintf(relative, sizeof relative, "%s/%s", scan->relative, name)
                              : snprintf(relative, sizeof relative, "%s", name);
    if (n < 0 || (size_t)n >= sizeof relative) { failure(scan, name, "package relative path exceeds bound"); return; }
    n = snprintf(host, sizeof host, "%s/%s", scan->root, relative);
    if (n < 0 || (size_t)n >= sizeof host) { failure(scan, relative, "package host path exceeds bound"); return; }
    if (p4_mod_code_name(name)) {
        if (!scan->depth && (scan->flags & (P4_PACKAGE_FILES_DISC_TREE | P4_PACKAGE_FILES_PROVIDER)) &&
            empty_prebuild(host, name)) return;
        failure(scan, host, "unsupported executable/HostFS/cheat input; rewrite this feature as a Luau mod");
        return;
    }
    int exposed = scan->exposed && (scan->depth || !(scan->flags & P4_PACKAGE_FILES_DISC_TREE) || container(name));
    size_t key_limit = scan->flags & P4_PACKAGE_FILES_PROVIDER ? RELATIVE_MAX : KEY_MAX;
    if (exposed && (strlen(relative) >= key_limit || !valid_component(name, scan->flags))) {
        failure(scan, host, "noncanonical or overlong replacement path"); return;
    }
    int directory = p4_os_is_dir_nofollow(host);
    if (!directory && !p4_os_is_file_nofollow(host)) {
        failure(scan, host, "symlink/reparse-point or special package entry refused"); return;
    }
    if (!scan->depth && (scan->flags & P4_PACKAGE_FILES_DISC_TREE) && container(name) && !directory) {
        failure(scan, host, "disc-tree container label must name a directory"); return;
    }
    if (!directory && p4_mod_code_file(host) != 0) {
        failure(scan, host, "unreadable or executable replacement; rewrite this feature as a Luau mod"); return;
    }
    if (exposed && append(scan->files, relative, directory)) {
        failure(scan, host, "package file index allocation/memory budget exceeded"); return;
    }
    if (directory) {
        if (scan->depth == 32) { failure(scan, host, "package directory depth exceeds 32"); return; }
        Scan child = {scan->files, scan->root, scan->owner, relative, scan->flags, scan->depth + 1, exposed, scan->failed};
        if (p4_os_list_dir(host, scan_entry, &child)) failure(scan, host, "cannot enumerate package directory");
    }
}
static int compare_entry(const void *left, const void *right)
{
    return p4_package_key_compare(((const Entry *)left)->name.text, ((const Entry *)right)->name.text);
}
P4PackageFiles *p4_package_files_create(const char *root, const char *owner, unsigned flags, size_t memory_limit)
{
    if (!root || !owner || flags & ~(P4_PACKAGE_FILES_CASEFOLD | P4_PACKAGE_FILES_DISC_TREE | P4_PACKAGE_FILES_PROVIDER) ||
        memory_limit < sizeof(P4PackageFiles) || !p4_os_is_dir_nofollow(root)) return NULL;
    int tracked = (flags & P4_PACKAGE_FILES_PROVIDER) != 0;
    P4PackageFiles *files = tracked ? p4_package_metadata_alloc(sizeof *files, 1) : calloc(1, sizeof *files);
    if (!files) return NULL;
    files->limit = memory_limit;
    files->tracked = tracked;
    int failed = 0;
    Scan scan = {files, root, owner, "", flags, 0, 1, &failed};
    if (p4_os_list_dir(root, scan_entry, &scan) || failed) goto fail;
    /* The arena is now immutable. Switching the construction offsets to pointers
     * avoids per-name allocations and keeps the comparator context-free. */
    for (size_t i = 0; i < files->count; i++) {
        uint32_t offset = files->entries[i].name.offset;
        files->entries[i].name.text = files->strings + offset;
    }
    if (files->count > 1) p4_package_sort(files->entries, files->count, sizeof *files->entries, compare_entry);
    for (size_t i = 1; i < files->count; i++) {
        if (!compare_entry(&files->entries[i - 1], &files->entries[i])) {
            fprintf(stderr, "p4pc: mods: %s: canonical path collision: %s / %s\n", owner,
                    files->entries[i - 1].name.text, files->entries[i].name.text);
            goto fail;
        }
    }
    return files;
fail:
    p4_package_files_destroy(files);
    return NULL;
}
void p4_package_files_destroy(P4PackageFiles *files)
{
    if (!files) return;
    if (files->tracked) {
        p4_package_metadata_free(files->entries, files->capacity * sizeof *files->entries);
        p4_package_metadata_free(files->strings, files->string_capacity);
        p4_package_metadata_free(files, sizeof *files);
    } else {
        free(files->entries);
        free(files->strings);
        free(files);
    }
}
size_t p4_package_files_count(const P4PackageFiles *files) { return files ? files->count : 0; }
int p4_package_files_entry(const P4PackageFiles *files, size_t index, const char **relative, int *directory)
{
    if (!files || index >= files->count || !relative || !directory) return -1;
    *relative = files->entries[index].name.text;
    *directory = (int)files->entries[index].directory;
    return 0;
}
static size_t lower_bound(const P4PackageFiles *files, const char *canonical)
{
    size_t low = 0, high = files->count;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        int order = p4_package_key_compare(files->entries[middle].name.text, canonical);
        if (order < 0) low = middle + 1; else high = middle;
    }
    return low;
}
const char *p4_package_files_resolve(const char *canonical, void *user)
{
    const P4PackageFiles *files = user;
    if (!files || !canonical) return NULL;
    if (!canonical[0]) return "";
    size_t low = lower_bound(files, canonical);
    return low < files->count && !p4_package_key_compare(files->entries[low].name.text, canonical)
        ? files->entries[low].name.text : NULL;
}
int p4_package_files_list(const char *canonical, P4VfsPathVisitor visitor,
                          void *visitor_user, void *user)
{
    const P4PackageFiles *files = user;
    char prefix[RELATIVE_MAX], after[RELATIVE_MAX];
    if (!files || !canonical || !visitor) return -1;
    size_t length = strlen(canonical);
    if (length + 1 >= sizeof prefix) return -1;
    if (length) {
        size_t dir = lower_bound(files, canonical);
        if (dir == files->count || p4_package_key_compare(files->entries[dir].name.text, canonical) ||
            !files->entries[dir].directory) return 1;
    }
    memcpy(prefix, canonical, length);
    if (length) prefix[length++] = '/';
    prefix[length] = 0;
    for (size_t i = lower_bound(files, prefix); i < files->count;) {
        const Entry *entry = &files->entries[i];
        const char *name = entry->name.text;
        size_t p = 0;
        while (p < length && name[p] && upper((unsigned char)name[p]) == upper((unsigned char)prefix[p])) p++;
        if (p != length) break;
        const char *slash = strchr(name + length, '/');
        if (slash) {
            /* Skip this child's entire subtree, but preserve siblings such as
             * A.TXT and A0: all A/... keys are below the exclusive bound A0. */
            size_t end = (size_t)(slash - name);
            memcpy(after, name, end);
            after[end] = '/' + 1;
            after[end + 1] = 0;
            i = lower_bound(files, after);
            continue;
        }
        if (visitor(name, (int)entry->directory, visitor_user)) return -1;
        i++;
    }
    return 0;
}
const P4VfsPathIndex p4_package_files_vfs = {
    p4_package_files_resolve, p4_package_files_list
};

static int view_container(const char *relative)
{
    size_t size = strcspn(relative, "/");
    static const char *const labels[] = {"DATA", "BTL", "BGM", "ENV", "ZZZZZ"};
    for (size_t i = 0; i < sizeof labels / sizeof labels[0]; ++i) {
        if (strlen(labels[i]) != size) continue;
        size_t j = 0;
        while (j < size && upper((unsigned char)relative[j]) == (unsigned char)labels[i][j]) ++j;
        if (j == size) return 1;
    }
    return 0;
}
static const char *view_relative(const P4PackageFileView *view, const char *name)
{
    if (view->offset) {
        size_t length = view->offset - 1;
        size_t i = 0;
        while (i < length && name[i] && upper((unsigned char)name[i]) == upper((unsigned char)view->prefix[i])) ++i;
        if (i != length || name[i] != '/') return NULL;
        name += view->offset;
    }
    if ((view->flags & P4_PACKAGE_FILES_DISC_TREE) && !view_container(name)) return NULL;
    return name;
}
int p4_package_file_view_entry(const P4PackageFileView *view, size_t position,
                               const char **relative, int *directory)
{
    const char *name;
    if (!view || !relative || !directory ||
        p4_package_files_entry(view->files, position, &name, directory)) return -1;
    *relative = view_relative(view, name);
    return *relative ? 0 : 1;
}
int p4_package_file_view_init(P4PackageFileView *view, P4PackageFiles *files,
                              const char *prefix, unsigned flags)
{
    if (!view || !files || !prefix ||
        (flags & ~(P4_PACKAGE_FILES_CASEFOLD | P4_PACKAGE_FILES_DISC_TREE))) return -1;
    const char *actual = "";
    if (*prefix) {
        size_t position = lower_bound(files, prefix);
        if (position == files->count || p4_package_key_compare(files->entries[position].name.text, prefix) ||
            !files->entries[position].directory) return 1;
        actual = files->entries[position].name.text;
    }
    P4PackageFileView candidate = {.files = files, .prefix = actual,
        .offset = *actual ? strlen(actual) + 1 : 0, .flags = flags};
    for (size_t i = 0; i < files->count; ++i) {
        const Entry *entry = &files->entries[i];
        const char *relative = view_relative(&candidate, entry->name.text);
        if (!relative) continue;
        if (strlen(relative) >= KEY_MAX) return -1;
        if ((flags & P4_PACKAGE_FILES_DISC_TREE) && !strchr(relative, '/') && !entry->directory) return -1;
        const char *component = relative;
        for (const char *p = relative;; ++p) {
            if (*p != '/' && *p != '\0') {
                unsigned char c = (unsigned char)*p;
                if (c < 32 || c >= 127 || strchr(":;\\\\", c) ||
                    (!(flags & P4_PACKAGE_FILES_CASEFOLD) && c >= 'a' && c <= 'z')) return -1;
                continue;
            }
            if (p == component) return -1;
            if (!*p) break;
            component = p + 1;
        }
        ++candidate.count;
    }
    *view = candidate;
    return 0;
}
static int view_key(const P4PackageFileView *view, const char *canonical, char key[RELATIVE_MAX])
{
    int n = view->offset ? snprintf(key, RELATIVE_MAX, "%s%s%s", view->prefix, *canonical ? "/" : "", canonical)
                         : snprintf(key, RELATIVE_MAX, "%s", canonical);
    return n >= 0 && n < RELATIVE_MAX ? 0 : -1;
}
static const char *view_resolve(const char *canonical, void *user)
{
    const P4PackageFileView *view = user;
    char key[RELATIVE_MAX];
    if (!view || !canonical || view_key(view, canonical, key)) return NULL;
    if (*canonical && (view->flags & P4_PACKAGE_FILES_DISC_TREE) && !view_container(canonical)) return NULL;
    const char *relative = p4_package_files_resolve(key, view->files);
    if (!relative) return NULL;
    return *canonical ? relative + view->offset : "";
}
typedef struct ViewListing {
    const P4PackageFileView *view;
    P4VfsPathVisitor visitor;
    void *user;
} ViewListing;
static int view_visit(const char *name, int directory, void *user)
{
    ViewListing *listing = user;
    const char *relative = view_relative(listing->view, name);
    return relative ? listing->visitor(relative, directory, listing->user) : 0;
}
static int view_list(const char *canonical, P4VfsPathVisitor visitor, void *visitor_user, void *user)
{
    const P4PackageFileView *view = user;
    char key[RELATIVE_MAX];
    if (!view || !canonical || !visitor || view_key(view, canonical, key)) return -1;
    if (*canonical && (view->flags & P4_PACKAGE_FILES_DISC_TREE) && !view_container(canonical)) return 1;
    ViewListing listing = {view, visitor, visitor_user};
    return p4_package_files_list(key, view_visit, &listing, view->files);
}
const P4VfsPathIndex p4_package_file_view_vfs = {view_resolve, view_list};

int p4_package_files_find(const P4PackageFiles *files, const char *canonical,
                           const char **relative, int *directory)
{
    if (!files || !canonical || !relative || !directory) return -1;
    if (!*canonical) { *relative = ""; *directory = 1; return 0; }
    size_t position = lower_bound(files, canonical);
    if (position == files->count || p4_package_key_compare(files->entries[position].name.text, canonical))
        return 1;
    *relative = files->entries[position].name.text;
    *directory = (int)files->entries[position].directory;
    return 0;
}
