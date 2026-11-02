#include "p4pc/mods.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef P4_MOD_LUAU
#include "script_prepare.h"
#endif

static P4PackageRegistry *registry;
static int loaded;
static P4ModRefusalFn refusal;
static void *refusal_user;

int p4_mods_set_refusal_handler(P4ModRefusalFn callback, void *user)
{
    if (loaded) return -1;
    refusal = callback;
    refusal_user = user;
    return 0;
}
static int refuse_mod(const char *id, const char *reason, void *user)
{
    (void)user;
    return refusal ? refusal(id, reason, refusal_user) : -1;
}

#ifndef P4_MOD_LUAU
static int preflight_mods(const P4PackageRegistry *packages, void *user)
{
    (void)user;
    for (size_t i = 0; i < p4_package_registry_count(packages); ++i) {
        const P4PackageProvider *provider = p4_package_registry_provider(packages, i);
        if (p4_package_provider_claims(provider) & P4_PACKAGE_CLAIM_SCRIPT) {
            if (p4_package_provider_refuse(provider,
                    "Requires Luau, but this build has no game scripting capability")) return -1;
        }
    }
    return 0;
}
#endif
static const P4PackageClient client = {
    .diagnostic_name = "mods", .manifest_name = "mod.ini", .section = "[mod]",
    .kind = P4_PACKAGE_MOD,
    .compendium = 1, .mount_files = 1,
    .refuse = refuse_mod,
#ifdef P4_MOD_LUAU
    .preflight = p4_mod_scripts_prepare,
    .preflight_rollback = p4_mod_scripts_discard_preflight
#else
    .preflight = preflight_mods
#endif
};
int p4_mods_stage(const char *root, const char *enabled)
{
    if (loaded) return -1;
    int status = p4_package_registry_load(&registry, &client, root, enabled);
    if (status == 0) loaded = 1;
    return status;
}
int p4_mods_set_load_order(const char *ids)
{
    if (!loaded || !ids) return -1;
    return registry ? p4_package_registry_set_order(registry, ids) : 0;
}
int p4_mods_installations_scan(const char *root, P4PackageInstallationVisitor visitor, void *user)
{
    return p4_package_installations_scan(&client, root, visitor, user);
}
size_t p4_mods_count(void) { return p4_package_registry_count(registry); }
const P4PackageInfo *p4_mods_info(size_t index) { return p4_package_registry_info(registry, index); }
size_t p4_mods_file_conflicts(void) { return p4_package_registry_file_conflicts(registry); }
void p4_mods_assert_disabled(void)
{
    size_t count = p4_mods_count();
    if (count) {
        fprintf(stderr, "p4pc: mods: harness invariant violated: %zu active mods\n", count);
        abort();
    }
}
