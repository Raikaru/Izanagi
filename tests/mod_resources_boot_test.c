/* Real package plans, PNG decode and guest RW resource materialization. */
#include "p4pc/mod_resources_boot.h"
#include "p4pc/image.h"
#include "p4pc/game_items_ui.h"
#include "p4pc/game_table_boot.h"
#include "p4pc/platform.h"
#include "p4pc/os.h"
#include "p4gl.h"
#include "guest_test.h"
#include "package_boot_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/mod_setting_store.h"
extern const P4PackageHandler p4_game_tables_sources_boot_provider;
extern const P4PackageHandler p4_game_tables_items_ui_boot_provider;
static const P4GameItemsUi *item_ui;
static int item_ui_early, item_ui_late;
static const char *const item_ui_dependencies[] = {"game-tables.item-ui", NULL};
static int item_ui_prepare(void *unused)
{
    const void *product = NULL;
    (void)unused;
    assert(!p4_package_boot_product("game-tables.item-ui", &product) && !product);
    item_ui_early = 1;
    return p4_package_boot_provide(NULL);
}
static int item_ui_runtime(void *unused)
{
    const void *product = NULL;
    (void)unused;
    assert(!p4_package_boot_product("game-tables.item-ui", &product) && product);
    item_ui = product;
    assert(p4_game_tables_items_ui_item(item_ui, 1536) != 0);
    item_ui_late = 1;
    return p4_package_boot_provide(product);
}
static const P4PackageHandler item_ui_observer = {
    .name = "fixture.item-ui", .prepare = item_ui_prepare, .boot_phase = P4_PACKAGE_BOOT_UI,
    .dependencies = item_ui_dependencies, .runtime_prepare = item_ui_runtime
};
extern void *kwlnProbeHeapBudget(void);
extern void kwlnInitEngine(void);
static const char *mode;
static const char *fixture_root;
static const P4Resources *runtime;
static int early_seen, late_seen;
static const char *const dependencies[] = {P4_RESOURCES_IMAGES_HANDLER, NULL};

static int early(void *unused)
{
    (void)unused;
    const void *product = NULL;
    assert(!p4_package_boot_product(P4_RESOURCES_IMAGES_HANDLER, &product));
    if (!strcmp(mode, "empty") || !strcmp(mode, "unrelated")) assert(!product);
    else {
        const P4ResourceImagesPrepared *view = product;
        assert(view && p4_resources_images_prepared_count(view) == 1);
        assert(p4_resources_images_prepared_find(view, "boot.a", "boot.a:icon", 7) == 1);
        assert(p4_resources_images_prepared_find(view, "boot.a", "boot.a:icon", 8) == 0);
        assert(p4_resources_images_prepared_find(view, "boot.b", "boot.a:icon", 7) == -1);
        assert(p4_resources_images_prepared_find(view, NULL, "boot.a:icon", 7) == -1);
        if (!strcmp(mode, "changed")) {
            char path[2048];
            assert(snprintf(path, sizeof path, "%s/boot.a/icon.png", fixture_root) > 0);
            FILE *file = fopen(path, "wb"); assert(file);
            assert(fwrite("changed", 1, 7, file) == 7 && !fclose(file));
        }
    }
    early_seen = 1;
    return p4_package_boot_provide(product);
}
static int late(void *unused)
{
    (void)unused;
    const void *product = NULL;
    assert(!p4_package_boot_product(P4_RESOURCES_IMAGES_HANDLER, &product));
    runtime = product;
    if (!strcmp(mode, "empty") || !strcmp(mode, "unrelated")) assert(!runtime);
    else assert(runtime && p4_resources_count(runtime) == 1 && p4_resources_revision(runtime));
    late_seen = 1;
    return p4_package_boot_provide(product);
}
static const P4PackageHandler observer = {
    .name = "fixture.resource-ui", .prepare = early, .boot_phase = P4_PACKAGE_BOOT_UI,
    .dependencies = dependencies, .runtime_prepare = late
};

static int run(int argc, char **argv)
{
    assert(argc == 4); mode = argv[1]; fixture_root = argv[2];
    const char *elf = getenv("P4PC_ELF");
    if (!elf || !*elf) { fprintf(stderr, "mod_resources_boot: P4PC_ELF unset\n"); return 77; }
    p4_os_headless_env();
    p4_os_setenv("P4PC_INPUT", "script", 1);
    p4_os_setenv("P4PC_MODS", "0", 1);
    p4_os_setenv("P4PC_PACKS", "0", 1);
    p4_os_setenv("P4PC_CONFIG", "none", 1);
    p4_os_setenv("P4PC_TRAP_CONTINUE", "0", 1);
    p4pc_trap_configure();
    /* Initialize the ordinary mods-off fixture before staging synthetic test
     * providers; early-only also exercises admission before RW/GL startup. */
    p4pc_load_image(elf); p4pc_apply_patches(); p4pc_kernel_pre_init();
    int item_mode = !strncmp(mode, "item-ui", 7);
    int item_invalid = !strcmp(mode, "item-ui-missing") || !strcmp(mode, "item-ui-revision");
    int early_only = !strcmp(mode, "early-only") || !strcmp(mode, "override-early") || !strcmp(mode, "late-fail") ||
        !strcmp(mode, "bad-crc") || !strcmp(mode, "bad-zlib") ||
        !strcmp(mode, "bad-revision") || !strcmp(mode, "missing") || !strcmp(mode, "changed") || item_invalid;
    if (!early_only) {
        kwlnProbeHeapBudget(); kwlnInitEngine();
        assert(p4gl_context_is_current());
    } else assert(!p4gl_context_is_current());
    assert(!p4_package_handler_register(&p4_mod_settings_boot_provider)); /* resource-image `when` */
    assert(!p4_package_handler_register(&p4_resources_images_boot_provider));
    assert(!p4_package_handler_register(&observer));
    if (item_mode) {
        assert(!p4_package_handler_register(&p4_game_tables_sources_boot_provider));
        assert(!p4_package_handler_register(&p4_game_tables_items_ui_boot_provider));
        assert(!p4_package_handler_register(&item_ui_observer));
    }
    P4PackageClient client = {.diagnostic_name = "resource boot fixture", .manifest_name = "mod.ini",
        .section = "[mod]", .kind = P4_PACKAGE_MOD, .mount_files = 1};
    P4PackageRegistry *registry = NULL;
    assert(!p4_package_registry_load(&registry, &client, argv[2], argv[3]));
    int result = p4_package_registry_publish();
    if (item_mode) {
        if (item_invalid) {
            assert(result < 0 && !item_ui_early && !item_ui_late && !item_ui);
            assert(p4_resources_images_session_end() == P4_RESOURCE_OK);
            puts("item UI boot: unresolved authored reference refused before publication");
            return 0;
        }
        assert(!result && item_ui_early && !item_ui_late);
        const P4PackageProvider *failed = NULL; const char *reason = NULL;
        result = p4_package_registry_runtime_materialize(&failed, &reason);
        if (result) fprintf(stderr, "item UI boot runtime failed: %s\n", reason ? reason : "unknown");
        assert(!result && !failed && item_ui_late && item_ui);
        assert(p4_game_tables_items_ui_boot_session_end() == P4_TABLE_CONFLICT);
        /* This fixture has no live game task, queue or graphics users. */
        assert(p4_game_tables_items_session_end(1) == P4_TABLE_OK);
        assert(p4_game_tables_items_ui_boot_session_end() == P4_TABLE_OK);
        assert(p4_resources_images_session_end() == P4_RESOURCE_OK);
        p4_package_registry_retire();
        puts("item UI boot: real table and resource products resolved and drained");
        return 0;
    }
    if (!strcmp(mode, "bad-crc") || !strcmp(mode, "bad-zlib") ||
        !strcmp(mode, "bad-revision") || !strcmp(mode, "missing") || !strcmp(mode, "changed")) {
        assert(result < 0 && early_seen == !strcmp(mode, "changed") && !late_seen && !runtime);
        assert(p4_resources_images_session_end() == P4_RESOURCE_OK);
        puts("resource boot: invalid authored input refused before publication");
        return 0;
    }
    assert(!result && early_seen && !late_seen);
    if (!strcmp(mode, "early-only") || !strcmp(mode, "override-early")) {
        assert(!p4gl_context_is_current());
        assert(p4_resources_images_session_end() == P4_RESOURCE_OK);
        p4_package_registry_retire();
        puts("resource boot: early decode required no graphics context");
        return 0;
    }
    if (!strcmp(mode, "late-fail")) {
        const P4PackageProvider *failed = NULL; const char *reason = NULL;
        assert(p4_package_registry_runtime_materialize(&failed, &reason) < 0);
        assert(failed && p4_package_provider_selected(failed) && reason && *reason && !late_seen);
        assert(!strcmp(p4_package_provider_info(failed)->id, "boot.a"));
        /* The isolated fixture has no game tasks or consumers to drain. */
        assert(p4_resources_images_session_end() == P4_RESOURCE_OK);
        p4_package_registry_retire();
        puts("resource boot: late failure retained its selected provider");
        return 0;
    }
    const P4PackageProvider *failed = NULL; const char *reason = NULL;
    result = p4_package_registry_runtime_materialize(&failed, &reason);
    if (result) fprintf(stderr, "resource boot runtime failed: %s\n", reason ? reason : "unknown");
    assert(!result && late_seen && !failed);
    if (runtime) {
        const P4ResourceImage *image = p4_resources_find(runtime, "boot.a:icon");
        assert(image && image->revision == 7 && image->width == 2 && image->height == 2);
        const char *provider = !strcmp(mode, "override") ? "boot.b" : "boot.a";
        assert(!strcmp(image->owner, "boot.a") && !strcmp(image->source.provider, provider));
        assert(!strcmp(image->source.key, "icon.png") && image->source.rank == P4_CONTENT_RANK_PACKAGE_PLAN);
        assert(p4_resources_conflicts(runtime) == (!strcmp(mode, "override") ? 1u : 0u));
        const uint8_t a[] = {255,0,0,255, 0,255,0,128, 0,0,255,64, 255,255,0,0};
        const uint8_t b[] = {1,2,3,4, 5,6,7,8, 9,10,11,12, 13,14,15,16};
        const uint8_t *expected = !strcmp(mode, "override") ? b : a;
        const uint8_t *pixels = P4_G2H(image->pixels);
        assert(!memcmp(pixels, expected, 8) && !memcmp(pixels + image->stride, expected + 8, 8));
        uint32_t token = image->token;
        assert(p4_resources_pin(token) == P4_RESOURCE_OK);
        assert(p4_resources_images_session_end() == P4_RESOURCE_CONFLICT);
        assert(p4_resources_query(token));
        assert(p4_resources_release(token) == P4_RESOURCE_OK);
        assert(p4_resources_images_session_end() == P4_RESOURCE_OK);
        assert(!p4_resources_query(token));
    } else assert(p4_resources_images_session_end() == P4_RESOURCE_OK);
    p4_package_registry_retire();
    p4gl_context_close();
    puts("resource boot: plan decode, phase products, raster ownership and pinned drain passed");
    return 0;
}
int main(int argc, char **argv) { return p4t_on_guest_stack(run, argc, argv); }
