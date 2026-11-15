/* p4mod_package.h over the game's file_index.c, with its flags and budget. */
#include "p4mod_package.h"
#include "file_index.h"
#include "package_memory_internal.h"
#include <stdlib.h>

struct P4ModPackage { P4PackageFiles *files; };

P4ModPackage *p4mod_package_open(const char *root, const char *owner)
{
    P4ModPackage *package = calloc(1, sizeof *package);
    if (!package) return NULL;
    /* package_registry.c index_provider: these flags, the configured package budget. */
    package->files = p4_package_files_create(root, owner, P4_PACKAGE_FILES_CASEFOLD | P4_PACKAGE_FILES_PROVIDER,
        p4_packages_memory_usage().limit);
    if (!package->files) { free(package); return NULL; }
    return package;
}
void p4mod_package_close(P4ModPackage *package)
{
    if (!package) return;
    p4_package_files_destroy(package->files);
    free(package);
}
size_t p4mod_package_count(const P4ModPackage *package) { return p4_package_files_count(package->files); }
int p4mod_package_entry(const P4ModPackage *package, size_t index, const char **relative, int *directory)
{
    return p4_package_files_entry(package->files, index, relative, directory);
}
int p4mod_package_find(const P4ModPackage *package, const char *name, const char **relative, int *directory)
{
    return p4_package_files_find(package->files, name, relative, directory) ? 1 : 0;
}
