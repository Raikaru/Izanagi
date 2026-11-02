#include "p4pc/packs.h"
#include "p4pc/os.h"
#include "../config/p4cfg.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The same precedence is used for on-demand discovery and startup. */
static const char *configured_root(char path[1200], int *explicit_root)
{
    const char *root = getenv("P4PC_PACK_DIR");
    if (!root || !root[0]) root = p4cfg_get("packs.directory");
    *explicit_root = root && root[0];
    if (*explicit_root) return root;
    const char *config = p4cfg_path();
    if (!config || strlen(config) >= 1200) {
        fprintf(stderr, "p4pc: packs: configured root needs a settings path or P4PC_PACK_DIR\n");
        return NULL;
    }
    strcpy(path, config);
    char *slash = strrchr(path, '/'), *back = strrchr(path, '\\');
    if (back && (!slash || back > slash)) slash = back;
    size_t base = slash ? (size_t)(slash - path + 1) : 0;
    if (base + sizeof "packs" > 1200) return NULL;
    memcpy(path + base, "packs", sizeof "packs");
    return path;
}
int p4_packs_configured_installations_scan(P4PackageInstallationVisitor visitor, void *user)
{
    p4cfg_load();
    char path[1200];
    int explicit_root;
    const char *root = configured_root(path, &explicit_root);
    if (!root) return -1;
    if (!explicit_root && !p4_os_is_dir_nofollow(root)) {
        char resolved[1200];
        if (p4_os_resolved_path(root, resolved, sizeof resolved) && errno == ENOENT)
            return 0;
    }
    return p4_packs_installations_scan(root, visitor, user);
}
int p4_packs_init(void)
{
    if (p4_mod_witness_validate()) return -1;
    const char *switch_value = getenv("P4PC_PACKS");
    if (switch_value && !strcmp(switch_value, "0")) return p4_packs_stage(NULL, NULL);
    if (switch_value && strcmp(switch_value, "1")) {
        fprintf(stderr, "p4pc: packs: P4PC_PACKS must be 0 or 1\n");
        return -1;
    }
    p4cfg_load();
    const char *enabled = p4cfg_get("packs.enabled");
    if (!enabled || !enabled[0]) return p4_packs_stage(NULL, NULL);
    if (!p4_mod_witness_enabled() && p4_packages_automation_requested()) {
        fprintf(stderr, "p4pc: packs: automation requires P4PC_PACKS=0\n");
        return -1;
    }
    char path[1200];
    int explicit_root;
    const char *root = configured_root(path, &explicit_root);
    return root ? p4_packs_stage(root, enabled) : -1;
}
