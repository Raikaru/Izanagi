#include "p4pc/mods.h"
#include "p4pc/mod_setting_store.h"
#include "p4pc/mod_lifecycle.h"
#include "../config/p4cfg.h"
#include "p4pc/os.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *configured_root(char path[1200])
{
    const char *root = getenv("P4PC_MOD_DIR");
    if (!root || !root[0]) root = p4cfg_get("mods.directory");
    if (root && root[0]) return root;
    const char *config = p4cfg_path();
    if (!config || strlen(config) >= 1200) {
        fprintf(stderr, "p4pc: mods: configured root needs a settings path or P4PC_MOD_DIR\n");
        return NULL;
    }
    strcpy(path, config);
    char *slash = strrchr(path, '/'), *back = strrchr(path, '\\');
    if (back && (!slash || back > slash)) slash = back;
    size_t base = slash ? (size_t)(slash - path + 1) : 0;
    if (base + sizeof "mods" > 1200) return NULL;
    memcpy(path + base, "mods", sizeof "mods");
    return path;
}
/* Per-mod settings live beside p4pc.ini; without a settings file (P4PC_CONFIG=none)
 * they stay at their declared defaults and writes are refused. */
static void settings_root(void)
{
    const char *config = p4cfg_path();
    char directory[1200];
    if (!config || strlen(config) >= sizeof directory) {
        p4_mod_setting_store_root(NULL);
        p4_mod_lifecycle_root(NULL);
        return;
    }
    strcpy(directory, config);
    char *slash = strrchr(directory, '/'), *back = strrchr(directory, '\\');
    if (back && (!slash || back > slash)) slash = back;
    if (slash) *slash = 0;
    else strcpy(directory, ".");
    if (p4_mod_setting_store_root(directory[0] ? directory : "/"))
        fprintf(stderr, "p4pc: mods: cannot use the settings directory for mod settings\n");
    /* The lifecycle session lives beside them; nothing is written until a mod asks. */
    if (p4_mod_lifecycle_root(directory[0] ? directory : "/"))
        fprintf(stderr, "p4pc: mods: cannot use the settings directory for mod lifecycle records\n");
}
int p4_mods_configured_installations_scan(P4PackageInstallationVisitor visitor, void *user)
{
    p4cfg_load();
    settings_root();
    char path[1200];
    const char *root = configured_root(path);
    if (!root) return -1;
    /* A fresh settings directory need not contain a mods folder. Only the
     * implicit default may be absent; explicit roots must still be valid. */
    const char *env_root = getenv("P4PC_MOD_DIR");
    const char *ini_root = p4cfg_get("mods.directory");
    if ((!env_root || !env_root[0]) && (!ini_root || !ini_root[0]) &&
            !p4_os_is_dir_nofollow(root)) {
        char resolved[1200];
        if (p4_os_resolved_path(root, resolved, sizeof resolved) && errno == ENOENT)
            return 0;
    }
    return p4_mods_installations_scan(root, visitor, user);
}
int p4_mods_init(void)
{
    if (p4_mod_witness_validate()) return -1;
    const char *switch_value = getenv("P4PC_MODS");
    if (switch_value && !strcmp(switch_value, "0")) {
        int rc = p4_mods_stage(NULL, NULL);
        p4_mods_assert_disabled();
        return rc;
    }
    if (switch_value && strcmp(switch_value, "1")) {
        fprintf(stderr, "p4pc: mods: P4PC_MODS must be 0 or 1\n");
        return -1;
    }
    p4cfg_load();
    const char *enabled = p4cfg_get("mods.enabled");
    if (!enabled || !enabled[0]) return p4_mods_stage(NULL, NULL);
    /* Ordinary automation never consumes player settings. Only the explicitly
     * labelled, non-fidelity mod-test lane may exercise the real player loader. */
    if (!p4_mod_witness_enabled() && p4_packages_automation_requested()) {
        fprintf(stderr, "p4pc: mods: automation requires P4PC_MODS=0\n");
        return -1;
    }
    settings_root();
    char path[1200];
    const char *root = configured_root(path);
    if (!root || p4_mods_stage(root, enabled)) return -1;
    const char *order = p4cfg_get("mods.load_order");
    if (order && order[0] && p4_mods_set_load_order(order)) {
        fprintf(stderr, "p4pc: mods: invalid load_order selector\n");
        return -1;
    }
    return 0;
}
