/* p4mod's view of a mod folder through the game's own package file index
 * (src/platform/mods/file_index.c), built as package_registry.c index_provider
 * builds it: ASCII case-insensitive names, canonical-path collisions, links,
 * special entries, executables and noncanonical names refused. */
#ifndef P4MOD_PACKAGE_H
#define P4MOD_PACKAGE_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct P4ModPackage P4ModPackage;
/* NULL when the game would refuse the folder; the game's own "p4pc: mods:"
 * line naming the entry and the reason is already on stderr. root is UTF-8. */
P4ModPackage *p4mod_package_open(const char *root, const char *owner);
void p4mod_package_close(P4ModPackage *package);
/* Sorted in the game's case-insensitive order; relative is the name as stored
 * on disk, '/'-separated. 0, or -1 past the end. */
size_t p4mod_package_count(const P4ModPackage *package);
int p4mod_package_entry(const P4ModPackage *package, size_t index, const char **relative, int *directory);
/* The game's lookup: 0 found (relative as stored on disk), 1 absent. */
int p4mod_package_find(const P4ModPackage *package, const char *name, const char **relative, int *directory);
#ifdef __cplusplus
}
#endif
#endif
