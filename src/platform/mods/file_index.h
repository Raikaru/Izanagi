#ifndef P4PC_PACKAGE_FILE_INDEX_H
#define P4PC_PACKAGE_FILE_INDEX_H
#include <stddef.h>
#include "../files/vfs.h"

typedef struct P4PackageFiles P4PackageFiles;
enum P4PackageFileFlags {
    P4_PACKAGE_FILES_CASEFOLD = 1u,
    P4_PACKAGE_FILES_DISC_TREE = 2u, /* root exposes only the five CVM labels */
    P4_PACKAGE_FILES_PROVIDER = 4u /* complete package; payload rules belong to its view */
};
/* Scans/validates once. Names are stored in one bounded arena, not copied per
 * lookup. The caller owns this immutable index until every registered layer
 * using it is retired. All allocation capacities count against memory_limit. */
P4PackageFiles *p4_package_files_create(const char *root, const char *owner,
    unsigned flags, size_t memory_limit);
void p4_package_files_destroy(P4PackageFiles *files);
size_t p4_package_files_count(const P4PackageFiles *files); /* files + directories */
size_t p4_package_files_memory(const P4PackageFiles *files);
/* Sorted in canonical ASCII case-insensitive order. Borrowed original-cased
 * relative name; directory flag includes empty directories. Returns 0 or -1. */
int p4_package_files_entry(const P4PackageFiles *files, size_t index,
    const char **relative, int *directory);
/* 0 found, 1 absent, -1 invalid arguments. Root "" is a directory. */
int p4_package_files_find(const P4PackageFiles *files, const char *canonical,
                           const char **relative, int *directory);
/* Direct VFS resolver callback: returns a borrowed original-cased relative path,
 * or NULL for an unclaimed key. No allocation, directory walk or stat on misses. */
const char *p4_package_files_resolve(const char *canonical, void *files);
int p4_package_files_list(const char *canonical_directory, P4VfsPathVisitor visitor,
                          void *visitor_user, void *files);
extern const P4VfsPathIndex p4_package_files_vfs;
int p4_package_key_compare(const char *left, const char *right);
/* A borrowed projection over the same names/arena: no second filesystem walk,
 * name copies or independently indexed provider. root_position is the source
 * index position, not an exposed ordinal; entry returns 1 for an unexposed row. */
typedef struct P4PackageFileView {
    P4PackageFiles *files;
    const char *prefix;
    size_t offset, count;
    unsigned flags;
} P4PackageFileView;
int p4_package_file_view_init(P4PackageFileView *view, P4PackageFiles *files,
                              const char *prefix, unsigned flags);
int p4_package_file_view_entry(const P4PackageFileView *view, size_t root_position,
                               const char **relative, int *directory);
extern const P4VfsPathIndex p4_package_file_view_vfs;
#endif
