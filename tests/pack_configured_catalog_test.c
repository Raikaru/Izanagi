/* Real pack client, configuration, package registry and VFS; synthetic files only. */
#include "p4pc/packs.h"
#include "p4pc/mods.h"
#include "vfs.h"
#include <stdio.h>
#include <string.h>

static int seen, failed;
static int installation(const P4PackageInstallation *entry, void *user)
{
    if (user != &seen || !entry || !entry->info || entry->diagnostic ||
        entry->info->kind != P4_PACKAGE_ENGINE_PACK ||
        strcmp(entry->info->id, "catalog.pack") || entry->info->generation ||
        entry->info->ordinal != SIZE_MAX || p4_packs_count() || p4_packages_count()) failed = 1;
    ++seen;
    return 0;
}
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"pack frontend:%d: %s\n",__LINE__,#x); return 1; } } while (0)
int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    const char *mode = argv[1];
    if (!strcmp(mode, "scan") || !strcmp(mode, "empty") || !strcmp(mode, "error")) {
        int result = p4_packs_configured_installations_scan(installation, &seen);
        CHECK(result == (!strcmp(mode, "error") ? -1 : 0));
        CHECK(seen == (!strcmp(mode, "scan") ? 1 : 0) && !failed);
        CHECK(!p4_packs_count() && !p4_packages_count());
        CHECK(!p4_packages_memory_usage().resident && !p4_packages_memory_usage().reserved);
        CHECK(!p4_mods_init() && !p4_packs_init() && !p4_package_registry_publish());
        p4_mod_fidelity_check_start();
        p4_package_registry_retire();
    } else if (!strcmp(mode, "active")) {
        CHECK(!p4_mods_init() && !p4_packs_init() && !p4_package_registry_publish());
        CHECK(!p4_mods_count() && p4_packs_count() == 1 && p4_packages_count() == 1);
        const P4PackageInfo *info = p4_packs_info(0);
        CHECK(info && info->kind == P4_PACKAGE_ENGINE_PACK && !strcmp(info->id, "catalog.pack"));
        CHECK(!vfs_mount(NULL, NULL));
        int fd = vfs_open("DATA/HELLO.BIN", 0);
        CHECK(fd >= 0);
        P4VfsOrigin origin;
        CHECK(!vfs_origin(fd, &origin) && origin.kind == P4_VFS_MOD && !strcmp(origin.provider, "catalog.pack"));
        char bytes[5];
        CHECK(vfs_read(fd, bytes, sizeof bytes) == (int)sizeof bytes && !memcmp(bytes, "hello", sizeof bytes));
        CHECK(!vfs_close(fd));
        p4_package_registry_retire();
    } else if (!strcmp(mode, "disabled")) {
        CHECK(!p4_mods_init() && !p4_packs_init() && !p4_package_registry_publish());
        CHECK(!p4_packs_count() && !p4_packages_count());
        p4_mod_fidelity_check_start();
        p4_package_registry_retire();
    } else if (!strcmp(mode, "invalid-switch") || !strcmp(mode, "automation") ||
               !strcmp(mode, "duplicate")) {
        CHECK(!p4_mods_init());
        CHECK(p4_packs_init() == -1 && !p4_packs_count() && !p4_packages_count());
    } else return 2;
    puts("pack frontend: PASS");
    return 0;
}
