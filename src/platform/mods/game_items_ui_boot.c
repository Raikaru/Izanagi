#include "p4pc/game_items_ui.h"
#include "p4pc/game_table_boot.h"
#include "p4pc/mod_resources_boot.h"
#include "p4pc/packages.h"

/* UI owns this product and its pins; native items borrow it only after stage. */
static P4GameItemsUi *owned_ui;

static int prepare(void *user)
{
    (void)user;
    const void *tables_product = NULL, *images_product = NULL;
    if (p4_package_boot_product(P4_GAME_TABLES_BOOT_PROVIDER, &tables_product) ||
        p4_package_boot_product(P4_RESOURCES_IMAGES_HANDLER, &images_product))
        return p4_package_boot_fail(NULL, "Item UI CPU dependencies are unavailable");
    const P4GameTablesBootProduct *tables = tables_product;
    const P4GameItems *items = tables ? tables->items : NULL;
    const P4ResourceImagesPrepared *images = images_product;
    for (size_t i = 0; items && i < p4_game_tables_items_count(items); ++i) {
        const P4GameItemView *row = p4_game_tables_items_at(items, i);
        if (!row || (!row->icon_anchor_plus_one && (!images || p4_resources_images_prepared_find(images,
                row->icon.owner, row->icon.key, row->icon.revision) != 1)))
            return p4_package_boot_fail(NULL, "Item UI image reference is unavailable");
    }
    for (size_t i = 0; items && i < p4_game_tables_items_category_count(items); ++i) {
        const P4GameItemCategoryView *row = p4_game_tables_items_category_at(items, i);
        if (!row || !images || p4_resources_images_prepared_find(images,
                row->icon.owner, row->icon.key, row->icon.revision) != 1)
            return p4_package_boot_fail(NULL, "Item UI category image reference is unavailable");
    }
    /* Resource tokens do not exist during the CPU-only pass. */
    return p4_package_boot_provide(NULL);
}

static int runtime_prepare(void *user)
{
    (void)user;
    const void *tables_product = NULL, *resources_product = NULL;
    if (p4_package_boot_product(P4_GAME_TABLES_BOOT_PROVIDER, &tables_product) ||
        p4_package_boot_product(P4_RESOURCES_IMAGES_HANDLER, &resources_product))
        return p4_package_boot_fail(NULL, "Item UI dependencies are unavailable");
    const P4GameTablesBootProduct *tables = tables_product;
    const P4GameItems *items = tables ? tables->items : NULL;
    if (!items || (!p4_game_tables_items_count(items) && !p4_game_tables_items_category_count(items)))
        return p4_package_boot_provide(NULL);
    const P4Resources *resources = resources_product;
    if (owned_ui)
        return p4_package_boot_fail(NULL, "Item UI product already exists");
    P4GameItemsUi *ui = NULL;
    int result = p4_game_tables_items_ui_prepare(items, resources, &ui);
    if (result) return p4_package_boot_fail(NULL, "Item UI could not resolve its exact authored resources");
    result = p4_game_tables_items_ui_stage(ui);
    if (result) {
        p4_game_tables_items_ui_destroy(ui);
        return p4_package_boot_fail(NULL, "Item UI could not attach to its native item owner");
    }
    /* If product publication itself fails, retain the staged borrow for the
     * controlled restart; early rollback cannot run after native exposure. */
    owned_ui = ui;
    return p4_package_boot_provide(ui);
}

int p4_game_tables_items_ui_boot_session_end(void)
{
    /* Caller must already prove all task/queue and graphics users quiescent.
     * This exact-pointer check proves only that the item owner detached. */
    if (!p4_game_tables_items_ui_detached(owned_ui)) return P4_TABLE_CONFLICT;
    p4_game_tables_items_ui_destroy(owned_ui);
    owned_ui = NULL;
    return P4_TABLE_OK;
}

static const char *const dependencies[] = {
    P4_GAME_TABLES_BOOT_PROVIDER, P4_RESOURCES_IMAGES_HANDLER, NULL
};
const P4PackageHandler p4_game_tables_items_ui_boot_provider = {
    .name = "game-tables.item-ui", .prepare = prepare,
    .boot_phase = P4_PACKAGE_BOOT_UI, .dependencies = dependencies,
    .runtime_prepare = runtime_prepare
};
