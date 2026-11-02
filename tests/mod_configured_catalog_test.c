/* Real configuration frontend/common metadata scanner; no payload/VM service. */
#include "p4pc/mods.h"
#include <stdio.h>
#include <string.h>

static int seen, failed;
static int installation(const P4PackageInstallation *entry, void *user)
{
    if (user != &seen || !entry || !entry->info || entry->diagnostic ||
        (strcmp(entry->info->id, "catalog.first") && strcmp(entry->info->id, "catalog.second")) ||
        entry->info->generation || entry->info->ordinal != SIZE_MAX || entry->field_count != 1 ||
        p4_mods_count() || p4_packages_count()) failed = 1;
    ++seen;
    return 0;
}
int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    int empty = !strcmp(argv[1], "empty");
    int expected = (!strcmp(argv[1], "ok") || empty) ? 0 : -1;
    int status = p4_mods_configured_installations_scan(installation, &seen);
    if (status != expected || seen != (expected || empty ? 0 : 2) || failed ||
        p4_mods_count() || p4_packages_count() || p4_packages_memory_usage().resident ||
        p4_packages_memory_usage().reserved) return 1;
    /* The kill switch still disables activation even when the INI selects an
     * unavailable id; an on-demand scan must not change startup semantics. */
    if (p4_mods_init() || p4_package_registry_publish()) return 1;
    p4_mod_fidelity_check_start();
    p4_package_registry_retire();
    printf("configured catalog: PASS seen=%d activation=0\n", seen);
    return 0;
}
