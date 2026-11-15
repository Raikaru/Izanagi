/* [resource-model] boot handler (src/platform/mods/mod_models_boot.c) on real selected provider plans,
 * the real cooker and the frozen override table. A developer tree (P4PC_OVERRIDE_DIR) stands in for the
 * disc, so no retail bytes are needed; tests/mod_models_boot_test.py writes the synthetic packages.
 *
 *   mod_models_boot <mode> <packages root> <enabled ids> <replaced key>
 *
 * Prints "publish <rc>", then for a published admission "runtime <rc>", "active <0|1>" and
 * "override <key> <name> <provider> <triangles> schema <n> animations <n>" or "override none". */
#include "p4pc/mod_models.h"
#include "p4pc/mod_setting_store.h"
#include "p4pc/os.h"
#include "p4pc/packages.h"
#include "../src/platform/models/p4model.h"
#include "package_boot_internal.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "usage: mod_models_boot <mode> <root> <enabled> <key>\n");
        return 2;
    }
    const char *mode = argv[1];
    p4_os_setenv("P4PC_CONFIG", "none", 1);
    p4_os_setenv("P4PC_MODS", "0", 1);
    p4_os_setenv("P4PC_PACKS", "0", 1);
    assert(!p4_package_handler_register(&p4_mod_settings_boot_provider)); /* resource-model `when` */
    assert(!p4_package_handler_register(&p4_resources_models_boot_provider));
    /* The retail path's only cost: with nothing declared the table is inactive before and after. */
    assert(!p4_model_overrides_active() && !p4_model_override_find(argv[4]));
    P4PackageClient client = {.diagnostic_name = "model boot fixture", .manifest_name = "mod.ini",
        .section = "[mod]", .kind = P4_PACKAGE_MOD, .mount_files = 1};
    P4PackageRegistry *registry = NULL;
    assert(!p4_package_registry_load(&registry, &client, argv[2], argv[3]));
    int result = p4_package_registry_publish();
    printf("publish %d\n", result);
    if (result) {
        assert(!p4_model_overrides_active());
        fflush(stdout);
        return 0;
    }
    const P4PackageProvider *failed = NULL;
    const char *reason = NULL;
    result = p4_package_registry_runtime_materialize(&failed, &reason);
    printf("runtime %d %s\n", result, reason ? reason : "");
    printf("active %d\n", p4_model_overrides_active());
    const P4ModelOverride *override = p4_model_override_find(argv[4]);
    if (override) {
        size_t size = 0;
        const uint8_t *blob = p4_model_override_blob(override, &size);
        assert(blob && !p4_model_cooked_check(blob, size));
        printf("override %s %s %s %u schema %u animations %u\n", argv[4], p4_model_override_name(override),
               p4_model_override_provider(override), (unsigned)p4_model_header(blob)->triangles,
               (unsigned)p4_model_override_schema(override), (unsigned)p4_model_header(blob)->animations);
        /* Keys are case-insensitive, as VFS keys are. */
        char lower[256];
        snprintf(lower, sizeof lower, "%s", argv[4]);
        for (char *p = lower; *p; ++p)
            if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
        assert(p4_model_override_find(lower) == override);
    } else {
        printf("override none\n");
    }
    if (!strcmp(mode, "empty")) assert(!p4_model_overrides_active());
    p4_model_overrides_session_end();
    assert(!p4_model_overrides_active());
    p4_package_registry_retire();
    fflush(stdout);
    return 0;
}
