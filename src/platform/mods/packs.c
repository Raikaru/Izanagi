#include "p4pc/packs.h"

static P4PackageRegistry *registry;
static int loaded;
static P4PackRefusalFn refusal;
static void *refusal_user;

int p4_packs_set_refusal_handler(P4PackRefusalFn callback, void *user)
{
    if (loaded) return -1;
    refusal = callback;
    refusal_user = user;
    return 0;
}
static int refuse_pack(const char *id, const char *reason, void *user)
{
    (void)user;
    return refusal ? refusal(id, reason, refusal_user) : -1;
}
static const P4PackageClient client = {
    .diagnostic_name = "packs", .manifest_name = "pack.ini", .section = "[pack]",
    .kind = P4_PACKAGE_ENGINE_PACK, .mount_files = 1, .refuse = refuse_pack
};
int p4_packs_stage(const char *root, const char *enabled)
{
    if (loaded) return -1;
    int status = p4_package_registry_load(&registry, &client, root, enabled);
    if (!status) loaded = 1;
    return status;
}
int p4_packs_installations_scan(const char *root, P4PackageInstallationVisitor visitor, void *user)
{
    return p4_package_installations_scan(&client, root, visitor, user);
}
size_t p4_packs_count(void) { return p4_package_registry_count(registry); }
const P4PackageInfo *p4_packs_info(size_t index) { return p4_package_registry_info(registry, index); }
size_t p4_packs_file_conflicts(void) { return p4_package_registry_file_conflicts(registry); }
