/* In-memory item compilation/boot bridge. File lookup, queue, skill eligibility,
 * raster and source-transport probes are isolated; real provenance is covered
 * by source/use fixtures, live menu/battle/sidecar qualification is separate. */
#include "p4pc/game_items.h"
#include "p4pc/game_items_ui.h"
#include "p4pc/game_tables_native.h"
#include "p4pc/game_table_overlay.h"
#include "../src/platform/mods/mod_resources_native.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned allocations, releases, live_arenas, rasters, destroyed;
static unsigned prepared, icons_drawn;
static unsigned item_uses, category_uses, text_uses;
static int item_probe_status;
_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{ fprintf(stderr, "Unexpected item fixture recovery: %s\n", reason); abort(); }
static uint8_t *icon_node, *icon_queue;
static uint16_t quantity_ids[800];
static unsigned quantity_calls;
int32_t datItemCount(int16_t id)
{
    assert(quantity_calls < sizeof quantity_ids / sizeof quantity_ids[0]);
    quantity_ids[quantity_calls++] = (uint16_t)id;
    return id >= (int32_t)P4_ITEM_RETAIL_COUNT ? p4_game_tables_items_native_quantity((uint32_t)id) :
        P4_G8(0x0079757au + (int32_t)id);
}
/* Kind-3 list fixture uses the original count increment contract. Other list
 * branches must fail rather than silently pretending to exercise retail code. */
int32_t fclAddClamped(int32_t a, int32_t b, int32_t c, int32_t d, int8_t e)
{ assert(b == 1 && !c && !d && !e && a >= 0 && a < 32767); return a + b; }
uint32_t datEntryGetFlags(int16_t id) { (void)id; abort(); }
uint32_t datEntryCategoryMask(int16_t id) { (void)id; abort(); }
uint32_t datEntryGetKind(int16_t id)
{
    assert(id >= (int32_t)P4_ITEM_RETAIL_COUNT);
    return ((const P4ItemRow *)p4_game_tables_items_native_metadata((uint16_t)id))->kind;
}
int8_t datItemAvailableMonth(int16_t id) { (void)id; abort(); }
int8_t datItemAvailableDay(int16_t id) { (void)id; abort(); }
int32_t fclCalendarMonthOrder(int32_t month) { (void)month; abort(); }
uint32_t datItemMoonGateFirst(int16_t id) { (void)id; abort(); }
uint32_t datItemMoonGateSecond(int16_t id) { (void)id; abort(); }
int32_t datEntryEvaluateFlags(int16_t id) { (void)id; abort(); }
uint32_t datGetFlag(int32_t id) { (void)id; abort(); }
int32_t fclMoonPhaseAtLeast(int32_t id, int32_t amount) { (void)id; (void)amount; abort(); }
int32_t fclItemIsDateAvailable(int16_t id) { (void)id; abort(); }
uint8_t clndGetMoonPhase(int32_t id) { (void)id; abort(); }
void datSetStateTableEntry(int32_t id, uint8_t value) { (void)id; (void)value; abort(); }
int32_t datItemPriceCompare(const void *a, const void *b) { (void)a; (void)b; abort(); }
void func_00440bb8(char *base, uint32_t count, uint32_t size, int (*compare)(const void *, const void *))
{ (void)base; (void)count; (void)size; (void)compare; abort(); }
/* Isolate row enumeration from the separately owned skill/battle pipeline. */
int32_t datCalcIsFlaggedPersonaSkill(int32_t id)
{
    return id < (int32_t)P4_ITEM_RETAIL_COUNT ? id > 0x300 && id < 0x400 :
        p4_game_tables_items_native_new_consumable((uint32_t)id);
}
static uint32_t expected_icon;
static const char *const *expected_model_paths;
static unsigned model_calls, model_count;
int p4pc_image_loaded(void) { return 1; }
int vfs_disc_locate(const char *path, long long *offset, long long *size)
{
    if (expected_model_paths) {
        assert(model_calls < model_count && !strcmp(path, expected_model_paths[model_calls]));
        if (++model_calls == model_count) return -1;
    } else assert(!strcmp(path, "model/weapon/wp001.RMD"));
    *offset = 123; *size = 456; return 0;
}
int p4_resources_draw(uint32_t token, float x, float y, float depth, uint32_t rgba, float scale_x, float scale_y)
{
    assert(token == expected_icon && p4_resources_query(token));
    assert(x == 21 && y == -4 && depth == 69 && rgba == 0x782b7fa6u);
    assert(scale_x == 1 && scale_y == 1);
    ++icons_drawn; return P4_RESOURCE_OK;
}
uint8_t *PrimAllocQueueNode(void)
{
    icon_node = p4t_galloc(0x30, 16); assert(icon_node);
    memset(icon_node, 0xa5, 0x30); return icon_node;
}
void PrimAppendNode(uint8_t *queue, uint8_t *node)
{
    assert(queue == icon_queue && node == icon_node);
    assert(!P4_G32(P4_H2G(node) + 12u) && !P4_G32(P4_H2G(node) + 24u));
    assert(P4_G32(P4_H2G(node) + 16u) == P4_H2G(node) + 28u);
}
static uint32_t expected_source_hash;
static uint32_t hash(const uint8_t *bytes, size_t size);
static int prepare(void *user, const char *table, const char *source, uint32_t rows,
    const uint8_t *bytes, size_t size)
{
    assert(!user && !strcmp(table, "item") && !strcmp(source, "init/itemtbl.bin"));
    assert(rows == 1536 && bytes[1] == 6 && size == P4_ITEM_RETAIL_FILE_SIZE);
    assert(p4_game_tables_items_session_end(1) == P4_TABLE_CONFLICT);
    assert(p4_game_tables_items_native_load(source, bytes, (uint32_t)size) == P4_TABLE_CONFLICT);
    assert(p4_game_tables_items_session_end(1) == P4_TABLE_CONFLICT);
    if (hash(bytes, size) != expected_source_hash) return P4_TABLE_CONFLICT;
    ++prepared; return P4_TABLE_OK;
}
static int action_mode = P4_ITEM_ACTION_DEFERRED;
static int card_action(void *user, uint32_t id, uint32_t category)
{
    assert(user == &action_mode && id == 1550 && category == 6);
    if (action_mode == P4_ITEM_ACTION_HANDLED) {
        P4GameItems *items = p4_game_tables_items_active();
        P4GameItemState state;
        assert(p4_game_tables_items_state_get(items, id, &state) == P4_TABLE_OK && state.quantity);
        assert(p4_game_tables_items_state_set(items, id, state.quantity - 1, state.availability) == P4_TABLE_OK);
    }
    return action_mode;
}
static int fail_allocate;
static void *allocate(void *user, uint32_t size, uint32_t *guest)
{
    (void)user; ++allocations;
    if (fail_allocate) return NULL;
    void *host = p4t_galloc(size, 16);
    if (host) { *guest = P4_H2G(host); ++live_arenas; }
    return host;
}
static void release(void *user, uint32_t guest, void *host, uint32_t size)
{
    (void)user; assert(size && guest == P4_H2G(host)); ++releases; --live_arenas;
}
/* Only the graphics backend is mocked; real resource token/pin ownership runs. */
int p4_resources_raster_create(const P4ContentSource *source, const uint8_t *rgba,
    uint32_t width, uint32_t height, uint32_t *raster, uint32_t *pixels, uint32_t *stride, uint32_t *rows)
{
    assert(source->origin == P4_TEXTURE_OVERRIDE && width == 4 && height == 4);
    uint8_t *copy = p4t_galloc(64, 16); memcpy(copy, rgba, 64);
    *pixels = P4_H2G(copy); *raster = P4_H2G(p4t_galloc(64, 16)); *stride = 16; *rows = 4;
    ++rasters; return P4_RESOURCE_OK;
}
void p4_resources_raster_destroy(uint32_t raster) { assert(raster); ++destroyed; }
static uint32_t hash(const uint8_t *bytes, size_t size)
{
    uint32_t result = 2166136261u;
    for (size_t i = 0; i < size; ++i) result = (result ^ bytes[i]) * 16777619u;
    return result;
}
/* Dispatch probes, explicitly NOT package/native-origin attestations. */
static int item_source_use(const P4GameItems *items, uint32_t table, uint32_t id, void *context)
{
    assert(context == items);
    if (table == P4_ITEM_USE_ROW) {
        P4GameItemDefinition definition;
        assert(p4_game_tables_items_definition_find(items, id, &definition) == P4_TABLE_OK);
        ++item_uses;
    } else {
        assert(table == P4_ITEM_USE_CATEGORY && p4_game_tables_items_category_find(items, id));
        ++category_uses;
    }
    return item_probe_status;
}
static int text_source_use(const P4GameText *text, uint32_t table, uint32_t id, uint32_t role, void *context)
{
    assert(context == text);
    P4GameTextDefinition definition;
    assert(p4_game_tables_text_definition_find(text, table, id, role, &definition) == P4_TABLE_OK);
    ++text_uses;
    return P4_TABLE_OK;
}
static void text(P4GameText *snapshot, uint32_t table, uint32_t id, uint32_t role, const char *bytes)
{
    assert(p4_game_tables_text_bind_runtime_use(snapshot, text_source_use, snapshot) == P4_TABLE_OK);
    assert(p4_game_tables_text_register(snapshot, "gear", table, id, role, 1,
        (const uint8_t *)bytes, strlen(bytes) + 1) == P4_TABLE_OK);
}
static void list_rebuild(int expanded)
{
    const uint32_t bytes = p4_game_tables_items_native_work_bytes();
    uint8_t *work = malloc(bytes + 16u); assert(work);
    memset(work, 0x5a, bytes + 16u);
    p4_game_tables_items_native_rebuild(work, 0, 0, 3, 0);
    int16_t count;
    memcpy(&count, work + 2, sizeof count);
    assert(count == (expanded ? 1539 : 1536));
    for (int16_t i = 0; i < 1536; ++i) {
        int16_t id; memcpy(&id, work + 0x0e + (size_t)i * 2u, sizeof id);
        assert(id == i);
    }
    if (expanded) {
        const int16_t expected[] = {1536, 1550, 1551};
        for (unsigned i = 0; i < 3; ++i) {
            int16_t id; memcpy(&id, work + 0x0e + (1536u + i) * 2u, sizeof id);
            assert(id == expected[i]);
        }
    }
    for (unsigned i = 0; i < 16; ++i) assert(work[bytes + i] == 0x5a);
    free(work);
}
static void shop_layout(int expanded)
{
    uint32_t size = p4_game_tables_items_native_shop_work_bytes();
    uint32_t count = p4_game_tables_items_native_gear_scan_count();
    assert(size == (expanded ? 0x1174u + count * 2u + 6u : 0x1174u));
    uint8_t *work = malloc(size + 16u); assert(work);
    memset(work, 0x5a, size + 16u);
    assert((uint8_t *)p4_game_tables_items_native_shop_unlocked(work) ==
        work + (expanded ? 0x1174u : 0x16u));
    p4_game_tables_items_native_shop_save_cursor(work, 1, expanded ? 257 : 57);
    assert(p4_game_tables_items_native_shop_cursor(work, 1) == (expanded ? 257 : 57));
    if (expanded) {
        assert(work[0x1169] == 0x5a);
        assert(p4_game_tables_items_native_gear_scan_id(0x2ffu) == 1536);
        assert(p4_game_tables_items_native_gear_scan_id(0x300u) == -1);
        assert(p4_game_tables_items_native_gear_scan_id(0x301u) == -1);
    }
    for (unsigned i = 0; i < 16; ++i) assert(work[size + i] == 0x5a);
    free(work);
    size = p4_game_tables_items_native_material_work_bytes();
    assert(size == (expanded ? 0x180cu + 1539u * 4u : 0x180cu));
    work = malloc(size + 16u); assert(work);
    memset(work, 0x5a, size + 16u);
    assert((uint8_t *)p4_game_tables_items_native_material_rows(work) ==
        work + (expanded ? 0x180cu : 4u));
    for (unsigned i = 0; i < 16; ++i) assert(work[size + i] == 0x5a);
    free(work);
}
static void legacy_inventory_rows(void)
{
    assert(p4_game_tables_items_native_camp_work_bytes() == 0x9740);
    assert(p4_game_tables_items_native_inventory_capacity() == 768);
    uint8_t *camp = p4t_galloc(0x9740, 16), *expected = malloc(0x9740);
    assert(camp && expected); memset(camp, 0x5a, 0x9740); memcpy(expected, camp, 0x9740);
    assert((uint8_t *)p4_game_tables_items_native_inventory_rows(camp + 0x2bc) == camp + 0x2fa);
    uint16_t count = 0;
    P4GameItemInventoryRow *rows = (P4GameItemInventoryRow *)(expected + 0x2fa);
    for (uint32_t id = 0x300; id < 0x600; ++id) {
        int16_t quantity = (int16_t)(P4_G8(0x0079757au + id) & 255);
        if (quantity) rows[count++] = (P4GameItemInventoryRow){(int16_t)id, quantity};
    }
    quantity_calls = 0;
    assert(p4_game_tables_items_native_inventory_refresh(camp + 0x2bc) == count && count == 768);
    assert(quantity_calls == 768 && !memcmp(camp, expected, 0x9740));
    for (unsigned i = 0; i < quantity_calls; ++i) assert(quantity_ids[i] == 0x300 + i);
    free(expected);
}
static void battle_rows(const P4GameItemBattleRow *added, unsigned added_count)
{
    uint32_t bytes = p4_game_tables_items_native_battle_work_bytes();
    assert(bytes == (added_count ? 0xa20u + (256u + added_count) * 4u : 0xa20u));
    uint8_t *work = p4t_galloc(bytes, 16), *expected = malloc(bytes);
    assert(work && expected);
    memset(work, 0x5a, bytes); memcpy(expected, work, bytes);
    uint32_t offset = added_count ? 0xa20u : 0x9cu + 0x1a6u;
    assert((uint8_t *)p4_game_tables_items_native_battle_rows(work + 0x9c) == work + offset);
    P4GameItemBattleRow *rows = (P4GameItemBattleRow *)(expected + offset);
    uint16_t count = 0;
    for (uint32_t id = 0x301; id < 0x400; ++id) {
        uint8_t quantity = P4_G8(0x0079757au + id);
        if (quantity) {
            rows[count].id = (int16_t)id;
            rows[count++].quantity = quantity;
        }
    }
    for (unsigned i = 0; i < added_count; ++i) {
        rows[count].id = added[i].id;
        rows[count++].quantity = added[i].quantity;
    }
    quantity_calls = 0;
    assert(p4_game_tables_items_native_battle_refresh(work + 0x9c) == count);
    assert(count == 255u + added_count && !memcmp(work, expected, bytes));
    unsigned call = 0;
    for (uint32_t id = 0x300; id < 0x400; ++id) {
        assert(quantity_ids[call++] == id);
        if (id > 0x300 && P4_G8(0x0079757au + id)) assert(quantity_ids[call++] == id);
    }
    for (unsigned i = 0; i < added_count; ++i) {
        assert(quantity_ids[call++] == (uint16_t)added[i].id);
        assert(quantity_ids[call++] == (uint16_t)added[i].id);
    }
    assert(quantity_calls == call);
    free(expected);
}
/* These source probes check ordering only; actual immutable origin/file
 * authority is exercised by game_table_sources/game_table_boot fixtures. */
static unsigned source_prepares, source_commits;
static int source_prepare(void *user)
{
    P4GameItems *items = user;
    assert(p4_game_tables_items_revision(items));
    assert(!p4_game_tables_items_active() || p4_game_tables_items_active() == items);
    uint32_t expected = p4_game_tables_items_active() ? p4_game_tables_items_base(items) : 0x0079bf10u;
    assert(P4_G32(0x007642a0u) == expected);
    ++source_prepares;
    return P4_TABLE_OK;
}
static void source_commit(void *user)
{
    assert(p4_game_tables_items_active() == user && source_prepares > source_commits);
    ++source_commits;
}
/* Standalone in-memory cases explicitly exercise both real phases. Native
 * coordinator cases below deliberately stop at seal until prepare-all. */
static int compile_items(P4GameItems *items, const uint8_t *file, size_t size,
    const P4GameText *names, const P4GameText *descriptions, uint32_t revision)
{
    if (!p4_game_tables_items_semantic_revision(items))
        assert(p4_game_tables_items_bind_runtime_use(items, item_source_use, items) == P4_TABLE_OK);
    int result = p4_game_tables_items_seal(items, file, size, names, descriptions, revision);
    return result ? result : p4_game_tables_items_materialize(items);
}
/* The table service's session overlay of the retail item table re-applies after
 * every publication of the built-in copy (game_table_overlay.c, API 1.11). */
static unsigned retail_publications;
void p4_game_tables_overlay_static_published(uint32_t address, uint32_t bytes)
{
    assert(address == 0x0079bf10u && bytes == P4_ITEM_RETAIL_COUNT * P4_ITEM_ROW_SIZE);
    assert(P4_G32(0x007642a0u) == address);
    ++retail_publications;
}
static void native_read(const uint8_t *file)
{
    assert(p4_game_tables_items_native_load("init/itemtbl.bin", file, P4_ITEM_RETAIL_FILE_SIZE) == P4_TABLE_OK);
    memcpy(P4_G2H(0x0079bf00u), file, P4_ITEM_RETAIL_FILE_SIZE);
    const unsigned before = retail_publications;
    p4_game_tables_items_native_use_builtin();
    /* Only the built-in retail table is announced; a catalog's own base is not. */
    assert(retail_publications == before + (P4_G32(0x007642a0u) == 0x0079bf10u));
}
static P4GameItemsUi *runtime_prepare(P4GameItems *items, const uint8_t *file,
    const P4GameText *names, const P4GameText *descriptions, const P4Resources *resources)
{
    assert(p4_game_tables_prepare_register(prepare, NULL) == P4_TABLE_OK);
    if (!p4_game_tables_items_semantic_revision(items))
        assert(p4_game_tables_items_bind_runtime_use(items, item_source_use, items) == P4_TABLE_OK);
    assert(p4_game_tables_items_seal(items, file, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 9) == P4_TABLE_OK);
    assert(p4_game_tables_items_boot_stage(items) == P4_TABLE_NOT_READY);
    assert(p4_game_tables_items_source_stage(items) == P4_TABLE_OK);
    assert(p4_game_tables_items_boot_stage(items) == P4_TABLE_OK);
    native_read(file); /* Original copy/builtin publication, never expanded publication. */
    assert(!p4_game_tables_items_active() && P4_G32(0x007642a0u) == 0x0079bf10u);
    P4GameItemsUi *ui = NULL;
    assert(p4_game_tables_items_ui_prepare(items, resources, &ui) == P4_TABLE_OK && ui);
    assert(p4_game_tables_items_ui_detached(ui));
    assert(p4_game_tables_items_ui_stage(ui) == P4_TABLE_OK);
    assert(!p4_game_tables_items_ui_detached(ui));
    assert(p4_game_tables_battle_complete_register("table-source", source_prepare, source_commit, items) == P4_TABLE_OK);
    assert(!p4_game_tables_items_active() && P4_G32(0x007642a0u) == 0x0079bf10u);
    return ui;
}
static void runtime_detach(P4GameItemsUi *ui)
{
    int result = p4_game_tables_battle_abort();
    assert(result == P4_TABLE_OK || result == P4_TABLE_FROZEN);
    assert(p4_game_tables_items_session_end(1) == P4_TABLE_OK);
    assert(p4_game_tables_items_ui_detached(ui));
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    p4_game_tables_items_ui_destroy(ui);
}

static void model_closure(const P4GameTableAllocator *allocator, const uint8_t *file,
    const P4GameText *names, const P4GameText *descriptions, const P4Resources *resources)
{
    static const struct {
        uint32_t mask, flags, right, left, count;
        const char *paths[3];
    } cases[] = {
        {0x40, 1, 1, 1002, 2, {"model/weapon/wp001_R.RMD", "model/weapon/wp002_L.RMD"}},
        {0x60, 0, 1, 1002, 3, {"model/weapon/wp001.RMD", "model/weapon/wp001_R.RMD", "model/weapon/wp002_L.RMD"}},
        {0x80, 2, 0x70, P4_ITEM_MODEL_NONE, 2, {"model/weapon/wp070.RMD", "model/weapon/wp070_c1.RMD"}},
        {0x20, 2, 1, P4_ITEM_MODEL_NONE, 1, {"model/weapon/wp001.RMD"}},
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        P4GameItems *items = p4_game_tables_items_create(allocator); assert(items);
        P4GameItemDesc row = {.id = 1536, .revision = 1, .key = "gear:model", .icon = {"art", "art:square", 1},
            .metadata = {.kind = 0, .flags = cases[i].flags, .category_mask = cases[i].mask}};
        assert(p4_game_tables_items_register(items, "gear", &row) == P4_TABLE_OK);
        if (cases[i].mask & 0x40u)
            assert(p4_game_tables_items_models_register(items, "gear", 1536, cases[i].right, P4_ITEM_MODEL_NONE) == P4_TABLE_INVALID);
        if (cases[i].mask & 0x80u) {
            assert(p4_game_tables_items_models_register(items, "gear", 1536, 0x6f, P4_ITEM_MODEL_NONE) == P4_TABLE_INVALID);
            assert(p4_game_tables_items_models_register(items, "gear", 1536, 0x96, P4_ITEM_MODEL_NONE) == P4_TABLE_INVALID);
        }
        assert(p4_game_tables_items_models_register(items, "gear", 1536, cases[i].right, cases[i].left) == P4_TABLE_OK);
        P4GameItemsUi *ui = runtime_prepare(items, file, names, descriptions, resources);
        expected_model_paths = cases[i].paths; model_calls = 0; model_count = cases[i].count;
        unsigned before_allocations = allocations;
        uint32_t before_base = P4_G32(0x007642a0u);
        assert(p4_game_tables_battle_prepare() == P4_TABLE_NOT_FOUND);
        assert(model_calls == model_count && allocations == before_allocations);
        assert(!p4_game_tables_items_active() && !p4_game_tables_items_revision(items) &&
            p4_game_tables_items_semantic_revision(items) && P4_G32(0x007642a0u) == before_base);
        runtime_detach(ui);
        expected_model_paths = NULL;
        p4_game_tables_items_destroy(items);
    }
}
static void empty_boot(const P4GameTableAllocator *allocator, const uint8_t *file)
{
    P4GameItems *items = p4_game_tables_items_create(allocator); assert(items);
    unsigned before_allocations = allocations, before_prepared = prepared;
    assert(p4_game_tables_items_seal(items, NULL, 0, NULL, NULL, 1) == P4_TABLE_OK);
    assert(p4_game_tables_items_boot_stage(items) == P4_TABLE_OK);
    P4GameItemsUi *ui = NULL;
    assert(p4_game_tables_items_ui_prepare(items, NULL, &ui) == P4_TABLE_OK && !ui);
    assert(p4_game_tables_items_ui_stage(ui) == P4_TABLE_OK);
    native_read(file);
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK);
    assert(!p4_game_tables_items_active() && !p4_game_tables_items_native_expanded());
    assert(P4_G32(0x007642a0u) == 0x0079bf10u && allocations == before_allocations && prepared == before_prepared);
    for (uint32_t id = 0; id < P4_ITEM_RETAIL_COUNT; ++id)
        assert(p4_game_tables_items_native_metadata(id) == (const uint8_t *)P4_G2H(0x0079bf10u + id * P4_ITEM_ROW_SIZE));
    assert(allocations == before_allocations); /* Retail queries do not clone or mutate the table. */
    assert(p4_game_tables_items_native_list_capacity() == 1536 && p4_game_tables_items_native_work_bytes() == 0x1810);
    assert(p4_game_tables_items_native_camp_work_bytes() == 0x9740 && p4_game_tables_items_native_inventory_capacity() == 768);
    battle_rows(NULL, 0);
    native_read(file);
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK);
    assert(P4_G32(0x007642a0u) == 0x0079bf10u && allocations == before_allocations && prepared == before_prepared);
    runtime_detach(NULL);
    p4_game_tables_items_destroy(items);
}
static void prefix_source_commit(void *user)
{
    assert(!p4_game_tables_items_active() && p4_game_tables_items_revision(user) && source_prepares > source_commits);
    ++source_commits;
}
static P4GameItems *prefix_view(const uint8_t *file)
{
    P4GameItems *items = p4_game_tables_items_create(NULL); assert(items);
    assert(!p4_game_tables_items_bind_runtime_use(items, item_source_use, items));
    assert(!p4_game_tables_items_seal(items, file, P4_ITEM_RETAIL_FILE_SIZE, NULL, NULL, 7));
    assert(!p4_game_tables_items_native_admit(items));
    return items;
}
static void original_prefix_boot(const uint8_t *file)
{
    const unsigned before_allocations = allocations;
    P4GameItems *items = prefix_view(file), *wrong = prefix_view(file);
    assert(!p4_game_tables_prepare_register(prepare, NULL));
    assert(p4_game_tables_items_boot_stage(items) == P4_TABLE_NOT_READY);
    /* A refused coordinator registration does not grant ownership of the
     * existing participant; source-only rollback must leave it intact. */
    assert(!p4_game_tables_items_source_stage(items));
    assert(!p4_game_tables_battle_complete_register("item", source_prepare, prefix_source_commit, items));
    assert(p4_game_tables_items_boot_stage(items) == P4_TABLE_CONFLICT);
    assert(!p4_game_tables_items_boot_unstage(items));
    assert(p4_game_tables_battle_complete_register("item", source_prepare, prefix_source_commit, wrong) == P4_TABLE_CONFLICT);
    assert(!p4_game_tables_battle_complete_unregister("item", source_prepare, prefix_source_commit, items));
    assert(!p4_game_tables_items_source_stage(items) && !p4_game_tables_items_boot_stage(items));
    assert(p4_game_tables_items_boot_unstage(wrong) == P4_TABLE_CONFLICT);
    assert(!p4_game_tables_items_boot_unstage(items) && !p4_game_tables_items_boot_unstage(items));
    assert(!p4_game_tables_items_source_stage(items) && !p4_game_tables_items_boot_stage(items));
    assert(!p4_game_tables_battle_complete_register("table-source", source_prepare, prefix_source_commit, items));
    assert(p4_game_tables_battle_complete() == P4_TABLE_NOT_READY); /* No actual precopy/copy hook yet. */
    assert(!p4_game_tables_items_revision(items) && !p4_game_tables_items_base(items));
    assert(p4_game_tables_items_boot_unstage(items) == P4_TABLE_FROZEN); /* Native graph already locked. */
    runtime_detach(NULL);
    p4_game_tables_items_destroy(items); p4_game_tables_items_destroy(wrong);
    items = prefix_view(file);
    unsigned before_prepared = prepared, before_uses = item_uses;
    assert(!p4_game_tables_items_source_stage(items) && !p4_game_tables_items_boot_stage(items));
    assert(!p4_game_tables_battle_complete_register("table-source", source_prepare, prefix_source_commit, items));
    native_read(file); /* Controlled source/copy probes, NOT loaded-range authority. */
    assert(prepared == before_prepared + 1 && item_uses == before_uses);
    assert(p4_game_tables_items_boot_unstage(items) == P4_TABLE_FROZEN);
    assert(!p4_game_tables_battle_complete() && p4_game_tables_items_revision(items) == 7);
    assert(!p4_game_tables_items_active() && !p4_game_tables_items_native_expanded() &&
        !p4_game_tables_items_base(items) && P4_G32(0x007642a0u) == 0x0079bf10u && allocations == before_allocations);
    assert(!p4_game_tables_items_runtime_use(items, 42) && item_uses == before_uses + 1);
    assert(p4_game_tables_items_native_metadata(42) == P4_G2H(0x0079bf10u + 42u * P4_ITEM_ROW_SIZE));
    runtime_detach(NULL);
    p4_game_tables_items_destroy(items);
    assert(allocations == before_allocations);
}
static void allocator_declaration(const uint8_t *file, const P4GameText *names, const P4GameText *descriptions)
{
    P4GameItems *items = p4_game_tables_items_create(NULL); assert(items);
    P4GameItemDesc row = {.id = 1536, .revision = 1, .key = "gear:cpu-only",
        .icon = {"art", "art:square", 1}, .metadata = {.kind = 1}};
    const unsigned before = allocations;
    assert(!p4_game_tables_items_register(items, "gear", &row));
    assert(!p4_game_tables_items_seal(items, file, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 7));
    assert(p4_game_tables_items_native_admit(items) == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_items_source_stage(items) == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_items_boot_stage(items) == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_items_materialize(items) == P4_TABLE_NOT_FOUND);
    assert(!p4_game_tables_items_revision(items) && !p4_game_tables_items_base(items) && allocations == before);
    assert(!p4_game_tables_items_boot_unstage(items));
    p4_game_tables_items_destroy(items);
}
static void accessory_overlay(const P4GameTableAllocator *allocator, const uint8_t *file,
    const P4GameText *names, const P4GameText *descriptions, const P4Resources *resources)
{
    P4GameItems *items = p4_game_tables_items_create(allocator); assert(items);
    P4GameItemDesc row = {.id = 1536, .revision = 1, .key = "gear:accessory", .icon = {"art", "art:square", 1},
        .metadata = {.kind = 2, .flags = 0x8000, .category_mask = 0x20, .stat_bonus = {17, -9, 0, 5, -11}}};
    assert(p4_game_tables_items_register(items, "gear", &row) == P4_TABLE_OK);
    P4GameItemsUi *ui = runtime_prepare(items, file, names, descriptions, resources);
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK);
    assert(p4_game_tables_items_native_set_equipment(1, 2, 1536) == 1);
    assert(p4_game_tables_items_native_equipment(1, 2) == 1536);
    assert(p4_game_tables_items_equipment_set(items, 2, 2, 1536) == P4_TABLE_INVALID);
    const P4ItemRow *metadata = (const P4ItemRow *)p4_game_tables_items_native_metadata(1536);
    assert(!memcmp(metadata->stat_bonus, row.metadata.stat_bonus, sizeof metadata->stat_bonus));
    uint32_t saved_reference = (uint32_t)p4_game_tables_items_native_equipment(1, 2);
    p4_game_tables_items_state_clear(items);
    assert(p4_game_tables_items_native_equipment(1, 2) == -1);
    assert(p4_game_tables_items_equipment_set(items, 1, 2, saved_reference) == P4_TABLE_OK);
    assert(p4_game_tables_items_native_equipment(1, 2) == (int32_t)saved_reference);
    runtime_detach(ui); assert(!p4_game_tables_items_active());
    p4_game_tables_items_destroy(items);
}
static void battle_expansion(const P4GameTableAllocator *allocator, uint8_t *file,
    const P4GameText *names, const P4GameText *descriptions, const P4Resources *resources)
{
    const P4GameItemBattleRow added[] = {{1536, 3, 0}, {1550, 4, 0}};
    P4GameItems *items = p4_game_tables_items_create(allocator); assert(items);
    for (unsigned i = 0; i < 2; ++i) {
        P4GameItemDesc row = {.id = (uint32_t)added[i].id, .revision = 1,
            .key = i ? "gear:battle_second" : "gear:battle_first", .icon = {"art", "art:square", 1},
            .metadata = {.kind = 3, .flags = 0x10000, .granted_skill = 1}};
        assert(p4_game_tables_items_register(items, "gear", &row) == P4_TABLE_OK);
    }
    P4GameItemsUi *ui = runtime_prepare(items, file, names, descriptions, resources);
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK);
    for (unsigned i = 0; i < 2; ++i)
        assert(p4_game_tables_items_state_set(items, (uint32_t)added[i].id, added[i].quantity, 0) == P4_TABLE_OK);
    battle_rows(added, 2); /*257 selectable rows, beyond the retail256-row inline storage. */
    uint32_t frozen_base = p4_game_tables_items_base(items);
    unsigned before_allocations = allocations, before_prepared = prepared;
    native_read(file);
    assert(p4_game_tables_items_active() == items);
    assert(allocations == before_allocations && prepared == before_prepared + 1);
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK);
    assert(P4_G32(0x007642a0u) == frozen_base);
    for (unsigned i = 0; i < 2; ++i)
        assert(p4_game_tables_items_native_quantity((uint32_t)added[i].id) == added[i].quantity);
    file[P4_ITEM_FILE_HEADER_SIZE + P4_ITEM_ROW_SIZE - 1u] ^= 1;
    assert(p4_game_tables_items_native_load("init/itemtbl.bin", file, P4_ITEM_RETAIL_FILE_SIZE) == P4_TABLE_CONFLICT);
    assert(P4_G32(0x007642a0u) == frozen_base);
    assert(p4_game_tables_items_active() == items && allocations == before_allocations);
    file[P4_ITEM_FILE_HEADER_SIZE + P4_ITEM_ROW_SIZE - 1u] ^= 1;
    runtime_detach(ui);
    p4_game_tables_items_destroy(items);
}
static void semantic_phase(const P4GameTableAllocator *allocator, const uint8_t *file)
{
    unsigned before = allocations, live = live_arenas;
    unsigned used_items = item_uses, used_categories = category_uses, used_text = text_uses;
    P4TextProfile raw = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4TextProfile message = {P4_TEXT_ENCODING_US_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4GameText *names = p4_game_tables_text_create(allocator, &raw);
    P4GameText *descriptions = p4_game_tables_text_create(allocator, &message);
    assert(names && descriptions);
    text(names, P4_TEXT_ITEM, 1550, P4_TEXT_NAME, "CPU card");
    text(descriptions, P4_TEXT_ITEM, 1550, P4_TEXT_DESCRIPTION, "CPU description");
    text(names, P4_TEXT_ITEM_CATEGORY, 6, P4_TEXT_NAME, "Cards");
    text(descriptions, P4_TEXT_ITEM_CATEGORY, 6, P4_TEXT_DESCRIPTION, "Card category");
    assert(p4_game_tables_text_seal(names, 3) == P4_TABLE_OK &&
        p4_game_tables_text_seal(descriptions, 4) == P4_TABLE_OK && allocations == before);
    P4GameItemDesc card = {.id = 1550, .revision = 5, .key = "gear:cpu_card",
        .icon = {"art", "art:square", 1}, .metadata = {.kind = 6, .granted_skill = 65536}};
    P4GameItemCategoryDesc category = {.id = 6, .revision = 2, .key = "gear:cards",
        .stock_policy = P4_ITEM_STOCK_NONE, .icon = {"art", "art:category", 1}};
    P4GameItems *unbound = p4_game_tables_items_create(allocator); assert(unbound);
    strcpy(category.action.owner, "script"); strcpy(category.action.key, "script:use-card");
    assert(p4_game_tables_items_category_register(unbound, "gear", &category) == P4_TABLE_INVALID);
    category.action.revision = 7;
    assert(p4_game_tables_items_register(unbound, "gear", &card) == P4_TABLE_OK);
    assert(p4_game_tables_items_category_register(unbound, "gear", &category) == P4_TABLE_OK);
    assert(p4_game_tables_items_action_register(unbound, "gear", 6, NULL, card_action, &action_mode) == P4_TABLE_CONFLICT);
    assert(p4_game_tables_items_seal(unbound, file, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 8) == P4_TABLE_NOT_FOUND);
    assert(!p4_game_tables_items_semantic_revision(unbound) && allocations == before);
    p4_game_tables_items_destroy(unbound);
    memset(&category.action, 0, sizeof category.action); /* Explicit compiled-native fixture action. */
    P4GameItems *items = p4_game_tables_items_create(allocator); assert(items);
    assert(p4_game_tables_items_register(items, "gear", &card) == P4_TABLE_OK);
    assert(p4_game_tables_items_category_register(items, "gear", &category) == P4_TABLE_OK);
    assert(p4_game_tables_items_action_register(items, "gear", 6, NULL, card_action, &action_mode) == P4_TABLE_OK);
    P4GameItemDefinition definition = {.id = UINT32_MAX};
    assert(p4_game_tables_items_definition_find(items, 1550, &definition) == P4_TABLE_NOT_READY && definition.id == UINT32_MAX);
    assert(p4_game_tables_items_materialize(items) == P4_TABLE_NOT_READY);
    assert(p4_game_tables_items_seal(items, file, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 8) == P4_TABLE_OK);
    assert(allocations == before && !p4_game_tables_items_revision(items) &&
        p4_game_tables_items_semantic_revision(items) == 8 && !p4_game_tables_items_base(items));
    assert(p4_game_tables_items_seal(items, NULL, 0, NULL, NULL, 8) == P4_TABLE_OK);
    assert(p4_game_tables_items_seal(items, file, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 9) == P4_TABLE_CONFLICT);
    assert(p4_game_tables_items_definition_find(items, 1550, &definition) == P4_TABLE_OK &&
        definition.metadata->granted_skill == 65536 && !strcmp(definition.owner, "gear"));
    const P4ItemRow *owned = definition.metadata;
    assert(p4_game_tables_items_definition_find(items, 1535, &definition) == P4_TABLE_OK && !definition.owner &&
        (const uint8_t *)definition.metadata == file + P4_ITEM_FILE_HEADER_SIZE + 1535u * P4_ITEM_ROW_SIZE);
    assert(p4_game_tables_items_definition_find(items, 1537, &definition) == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_items_action_register(items, "gear", 6, NULL, card_action, &action_mode) == P4_TABLE_FROZEN);
    assert(p4_game_tables_items_register(items, "gear", &card) == P4_TABLE_FROZEN);
    assert(p4_game_tables_items_materialize(items) == P4_TABLE_NOT_READY && allocations == before);
    assert(p4_game_tables_text_materialize(names) == P4_TABLE_OK && p4_game_tables_text_materialize(descriptions) == P4_TABLE_OK);
    fail_allocate = 1;
    assert(p4_game_tables_items_materialize(items) == P4_TABLE_NOMEM && !p4_game_tables_items_revision(items));
    fail_allocate = 0;
    assert(p4_game_tables_items_definition_find(items, 1550, &definition) == P4_TABLE_OK && definition.metadata == owned);
    assert(p4_game_tables_items_materialize(items) == P4_TABLE_OK && p4_game_tables_items_revision(items) == 8);
    assert(p4_game_tables_items_materialize(items) == P4_TABLE_FROZEN);
    assert(!memcmp(p4_game_tables_items_metadata(items, 1550), owned, sizeof *owned));
    assert(p4_game_tables_items_validate_runtime_sources(items) == P4_TABLE_NOT_READY &&
        p4_game_tables_items_runtime_use(items, 1550) == P4_TABLE_NOT_READY);
    assert(item_uses == used_items && category_uses == used_categories && text_uses == used_text);
    P4GameItemState state;
    assert(p4_game_tables_items_state_get(items, 1550, &state) == P4_TABLE_OK && !state.quantity && !state.availability);
    p4_game_tables_items_destroy(items);
    P4GameItems *transport = p4_game_tables_items_create(allocator); assert(transport);
    assert(p4_game_tables_items_register(transport, "gear", &card) == P4_TABLE_OK);
    assert(p4_game_tables_items_category_register(transport, "gear", &category) == P4_TABLE_OK);
    assert(p4_game_tables_items_action_register(transport, "gear", 6, NULL, card_action, &action_mode) == P4_TABLE_OK);
    assert(p4_game_tables_items_bind_runtime_use(transport, NULL, transport) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_bind_runtime_use(transport, item_source_use, NULL) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_bind_runtime_use(transport, item_source_use, transport) == P4_TABLE_OK);
    assert(p4_game_tables_items_bind_runtime_use(transport, item_source_use, transport) == P4_TABLE_OK);
    assert(p4_game_tables_items_bind_runtime_use(transport, item_source_use, &transport) == P4_TABLE_CONFLICT);
    assert(p4_game_tables_items_seal(transport, file, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 8) == P4_TABLE_OK);
    assert(p4_game_tables_items_validate_runtime_sources(transport) == P4_TABLE_OK);
    assert(p4_game_tables_items_bind_runtime_use(transport, item_source_use, transport) == P4_TABLE_FROZEN);
    assert(p4_game_tables_items_runtime_use(transport, 1550) == P4_TABLE_NOT_READY);
    assert(p4_game_tables_items_materialize(transport) == P4_TABLE_OK);
    assert(item_uses == used_items && category_uses == used_categories && text_uses == used_text);
    assert(p4_game_tables_items_runtime_use(transport, 1537) == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_items_category_runtime_use(transport, 7) == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_items_runtime_use(transport, 1550) == P4_TABLE_OK && item_uses == used_items + 1);
    assert(p4_game_tables_items_category_runtime_use(transport, 6) == P4_TABLE_OK && category_uses == used_categories + 1);
    item_probe_status = 1;
    assert(p4_game_tables_items_runtime_use(transport, 1550) == P4_TABLE_INVALID);
    item_probe_status = P4_TABLE_CONFLICT;
    assert(p4_game_tables_items_category_runtime_use(transport, 6) == P4_TABLE_CONFLICT);
    item_probe_status = 0;
    p4_game_tables_items_destroy(transport);
    p4_game_tables_text_destroy(names); p4_game_tables_text_destroy(descriptions);
    assert(live_arenas == live);
}
/* Derived rows: an anchor's retail row, copied at seal, with named overrides;
 * weapons without models bind the anchor's own native keys. */
static void anchor_row(uint8_t *file, uint32_t id, const P4ItemRow *row)
{
    memcpy(file + P4_ITEM_FILE_HEADER_SIZE + id * P4_ITEM_ROW_SIZE, row, sizeof *row);
}
static int derived_seal(const P4GameTableAllocator *allocator, const uint8_t *file,
    const P4GameText *names, const P4GameText *descriptions, uint32_t anchor, uint32_t right, uint32_t left)
{
    P4GameItems *items = p4_game_tables_items_create(allocator); assert(items);
    const P4GameItemDesc row = {.id = 1610, .revision = 1, .key = "gear:derived-case", .icon = {"art", "art:square", 1}};
    const P4GameItemDerive derive = {.anchor = anchor};
    assert(p4_game_tables_items_register_derived(items, "gear", &row, &derive) == P4_TABLE_OK);
    if (right != P4_ITEM_MODEL_NONE)
        assert(p4_game_tables_items_models_register(items, "gear", 1610, right, left) == P4_TABLE_OK);
    const int result = p4_game_tables_items_seal(items, file, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 4);
    p4_game_tables_items_destroy(items);
    return result;
}
static void derived_rows(const P4GameTableAllocator *allocator, const uint8_t *retail)
{
    uint8_t *file = malloc(P4_ITEM_RETAIL_FILE_SIZE); assert(file);
    memcpy(file, retail, P4_ITEM_RETAIL_FILE_SIZE);
    const P4ItemRow sword = {.kind = 0, .flags = 0x11, .primary_value = 30, .secondary_value = 90, .property = 5,
        .stat_bonus = {1, 0, 0, 0, 0}, .value = 0, .element = 0, .category_mask = 0x20, .price = 500,
        .available_month = 4, .available_day = 20, .first_moon_gate = 7};
    P4ItemRow knives = sword, shoes = sword, ward = {.kind = 2, .flags = 0x22, .stat_bonus = {0, 3, 0, 0, 0},
        .category_mask = 0x3E0, .price = 1000};
    knives.category_mask = 0x40; shoes.category_mask = 0x80;
    anchor_row(file, 4, &sword); anchor_row(file, 48, &knives); anchor_row(file, 0x70, &shoes);
    anchor_row(file, 0x204, &ward); anchor_row(file, 0x300, &sword);
    P4TextProfile raw = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4TextProfile message = {P4_TEXT_ENCODING_US_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4GameText *names = p4_game_tables_text_create(allocator, &raw);
    P4GameText *descriptions = p4_game_tables_text_create(allocator, &message);
    assert(names && descriptions);
    for (uint32_t id = 1600; id <= 1610; ++id) {
        text(names, P4_TEXT_ITEM, id, P4_TEXT_NAME, "Forged");
        text(descriptions, P4_TEXT_ITEM, id, P4_TEXT_DESCRIPTION, "Forged description");
    }
    assert(!p4_game_tables_text_seal(names, 3) && !p4_game_tables_text_seal(descriptions, 4));
    P4GameItems *items = p4_game_tables_items_create(allocator); assert(items);
    P4GameItemDesc row = {.id = 1600, .revision = 1, .key = "gear:forged", .icon = {"art", "art:square", 1},
        .metadata = {.kind = 9, .price = 1}}; /* A derived row's own metadata is ignored. */
    P4GameItemDerive derive = {.anchor = 4, .mask = P4_ITEM_DERIVE_PRICE | P4_ITEM_DERIVE_ELEMENT |
        P4_ITEM_DERIVE_VALUE | P4_ITEM_DERIVE_PROPERTY,
        .overrides = {.price = 1200, .element = 2, .value = 0x24, .property = 9, .primary_value = 999}};
    assert(p4_game_tables_items_register_derived(items, "gear", &row, &derive) == P4_TABLE_OK);
    row.id = 1601; row.key = "gear:forged-knives"; derive = (P4GameItemDerive){.anchor = 48};
    assert(p4_game_tables_items_register_derived(items, "gear", &row, &derive) == P4_TABLE_OK);
    row.id = 1602; row.key = "gear:forged-ward";
    derive = (P4GameItemDerive){.anchor = 0x204, .mask = P4_ITEM_DERIVE_STATS | P4_ITEM_DERIVE_AVAILABLE |
        P4_ITEM_DERIVE_MOON, .overrides = {.stat_bonus = {-1, 2, 0, 0, 5}, .available_month = -1,
            .available_day = 3, .first_moon_gate = 0, .second_moon_gate = 0x10}};
    assert(p4_game_tables_items_register_derived(items, "gear", &row, &derive) == P4_TABLE_OK);
    row.id = 1603; row.key = "gear:forged-shoes"; derive = (P4GameItemDerive){.anchor = 0x70};
    assert(p4_game_tables_items_register_derived(items, "gear", &row, &derive) == P4_TABLE_OK);
    /* An explicit model on a derived weapon is checked against its anchor at seal. */
    row.id = 1604; row.key = "gear:forged-model"; derive = (P4GameItemDerive){.anchor = 4};
    assert(p4_game_tables_items_register_derived(items, "gear", &row, &derive) == P4_TABLE_OK);
    assert(p4_game_tables_items_models_register(items, "gear", 1604, 9, P4_ITEM_MODEL_NONE) == P4_TABLE_OK);
    row.id = 1605; row.key = "gear:bad-anchor"; derive = (P4GameItemDerive){.anchor = P4_ITEM_RETAIL_COUNT};
    assert(p4_game_tables_items_register_derived(items, "gear", &row, &derive) == P4_TABLE_INVALID);
    derive = (P4GameItemDerive){.anchor = 4, .mask = P4_ITEM_DERIVE_FIELDS + 1};
    assert(p4_game_tables_items_register_derived(items, "gear", &row, &derive) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_seal(items, file, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 4) == P4_TABLE_OK);
    const P4GameItemView *view = p4_game_tables_items_find(items, 1600);
    P4ItemRow expected = sword;
    expected.price = 1200; expected.element = 2; expected.value = 0x24; expected.property = 9; /* Only the named fields. */
    assert(view && !memcmp(&view->metadata, &expected, sizeof expected));
    assert(view->models_registered && view->right_model_id == 4 && view->left_model_id == P4_ITEM_MODEL_NONE);
    view = p4_game_tables_items_find(items, 1601);
    assert(view && !memcmp(&view->metadata, &knives, sizeof knives) &&
        view->right_model_id == 48 && view->left_model_id == 1048); /* Unit 2's left key. */
    view = p4_game_tables_items_find(items, 1602);
    expected = ward; expected.stat_bonus[0] = -1; expected.stat_bonus[1] = 2; expected.stat_bonus[4] = 5;
    expected.available_month = -1; expected.available_day = 3; expected.second_moon_gate = 0x10;
    assert(view && !memcmp(&view->metadata, &expected, sizeof expected) && !view->models_registered);
    view = p4_game_tables_items_find(items, 1603);
    assert(view && view->right_model_id == 0x70 && view->left_model_id == P4_ITEM_MODEL_NONE);
    view = p4_game_tables_items_find(items, 1604);
    assert(view && view->right_model_id == 9 && !memcmp(&view->metadata, &sword, sizeof sword));
    p4_game_tables_items_destroy(items);
    /* Seal refusals: models on a non-weapon, unit 2 without a left key, unit 3
     * outside 0x70..0x95, and a weapon anchor with no native key of its own. */
    assert(derived_seal(allocator, file, names, descriptions, 0x204, 4, P4_ITEM_MODEL_NONE) == P4_TABLE_INVALID);
    assert(derived_seal(allocator, file, names, descriptions, 48, 48, P4_ITEM_MODEL_NONE) == P4_TABLE_INVALID);
    assert(derived_seal(allocator, file, names, descriptions, 0x70, 0x60, P4_ITEM_MODEL_NONE) == P4_TABLE_INVALID);
    assert(derived_seal(allocator, file, names, descriptions, 0x300, P4_ITEM_MODEL_NONE, P4_ITEM_MODEL_NONE) == P4_TABLE_INVALID);
    assert(derived_seal(allocator, file, names, descriptions, 4, P4_ITEM_MODEL_NONE, P4_ITEM_MODEL_NONE) == P4_TABLE_OK);
    p4_game_tables_text_destroy(names); p4_game_tables_text_destroy(descriptions);
    free(file);
}

int main(void)
{
    static const uint32_t outside_new_domain[] = {0, 1535, 32766, 32767, 32768, 65535, UINT32_MAX};
    for (size_t i = 0; i < sizeof outside_new_domain / sizeof outside_new_domain[0]; ++i)
        assert(!p4_game_tables_items_native_id_is_new(outside_new_domain[i]));
    assert(p4_game_tables_items_native_id_is_new(1536) && p4_game_tables_items_native_id_is_new(32765));
    for (uint32_t opacity = 0; opacity < 256; ++opacity)
        assert(p4_game_tables_items_native_inverse_alpha_rgba((opacity << 24) | 0x2b7fa6u) ==
            ((255u - opacity) << 24 | 0x2b7fa6u));
    assert(p4_game_tables_items_native_gear_detail_code(0) == 20);
    assert(p4_game_tables_items_native_gear_detail_code(1) == 27);
    assert(p4_game_tables_items_native_gear_detail_code(2) == 28);
    assert(p4_game_tables_items_native_gear_detail_code(3) == -1);
    assert(p4_game_tables_items_native_gear_detail_code(6) == -1);
    assert(p4_game_tables_items_native_gear_detail_code(UINT32_MAX) == -1);
    if (p4t_guest_init() || p4t_guest_map(0x00790000u, 0x30000u) ||
        p4t_guest_map(0x00760000u, 0x10000u)) return 77;
    strcpy(P4_G2H(0x007636e0u), "model/");
    uint8_t *retail_guard = P4_G2H(0x00790000u);
    memset(retail_guard, 0xa5, 0x30000u);
    uint32_t guard = hash(retail_guard, 0x30000u);
    P4GameTableAllocator allocator = {NULL, allocate, release};
    legacy_inventory_rows();
    list_rebuild(0);
    shop_layout(0);
    P4GameItems *empty = p4_game_tables_items_create(&allocator); assert(empty);
    assert(compile_items(empty, NULL, 0, NULL, NULL, 1) == P4_TABLE_OK);
    assert(!allocations && !p4_game_tables_items_count(empty) && !p4_game_tables_items_base(empty));
    assert(p4_game_tables_items_limit(empty) == P4_ITEM_RETAIL_COUNT);
    p4_game_tables_items_destroy(empty);
    assert(hash(retail_guard, 0x30000u) == guard);

    P4TextProfile raw = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4TextProfile message = {P4_TEXT_ENCODING_US_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4GameText *names = p4_game_tables_text_create(&allocator, &raw);
    P4GameText *descriptions = p4_game_tables_text_create(&allocator, &message);
    assert(names && descriptions);
    const uint32_t ids[] = {1536, 1550, 1551, P4_ITEM_MAX_ID};
    for (size_t i = 0; i < sizeof ids / sizeof ids[0]; ++i) {
        text(names, P4_TEXT_ITEM, ids[i], P4_TEXT_NAME, "Synthetic item");
        text(descriptions, P4_TEXT_ITEM, ids[i], P4_TEXT_DESCRIPTION, "Synthetic\nitem description");
    }
    text(names, P4_TEXT_ITEM_CATEGORY, 6, P4_TEXT_NAME, "Skill Cards");
    text(descriptions, P4_TEXT_ITEM_CATEGORY, 6, P4_TEXT_DESCRIPTION, "Synthetic card category");
    assert(p4_game_tables_text_freeze(names, 9) == P4_TABLE_OK);
    assert(p4_game_tables_text_freeze(descriptions, 9) == P4_TABLE_OK);
    uint8_t rgba[64]; memset(rgba, 0x7f, sizeof rgba);
    P4ContentSource source = {.origin = P4_TEXTURE_OVERRIDE, .rank = 1,
        .provider = "art", .key = "icons/synthetic.rgba"};
    P4Resources *resources = p4_resources_create(256); assert(resources);
    uint32_t icon = 0, category_icon = 0;
    assert(p4_resources_image(resources, "art", "art:square", &source, rgba, sizeof rgba, 4, 4, 1, &icon) == P4_RESOURCE_OK);
    memset(rgba, 0x43, sizeof rgba); strcpy(source.key, "icons/category.rgba");
    assert(p4_resources_image(resources, "art", "art:category", &source, rgba, sizeof rgba, 4, 4, 1, &category_icon) == P4_RESOURCE_OK);
    assert(category_icon != icon);
    assert(p4_resources_freeze(resources, 9) == P4_RESOURCE_OK);

    uint8_t *file = malloc(P4_ITEM_RETAIL_FILE_SIZE); assert(file);
    for (size_t i = 0; i < P4_ITEM_RETAIL_FILE_SIZE; ++i) file[i] = (uint8_t)(i * 31u + i / 68u);
    file[0] = 0; file[1] = 6; file[2] = file[3] = 0;
    uint32_t prefix = hash(file + 4, P4_ITEM_RETAIL_FILE_SIZE - 4);
    expected_source_hash = hash(file, P4_ITEM_RETAIL_FILE_SIZE);
    memcpy(P4_G2H(0x0079bf00u), file, P4_ITEM_RETAIL_FILE_SIZE);
    guard = hash(retail_guard, 0x30000u); /* Exact original retail copy is the baseline. */
    empty_boot(&allocator, file);
    original_prefix_boot(file);
    allocator_declaration(file, names, descriptions);
    P4GameItems *items = p4_game_tables_items_create(&allocator); assert(items);
    char key[] = "gear:sword";
    P4GameItemDesc sword = {.id = 1536, .revision = 3, .key = key, .icon = {"art", "art:square", 1},
        .metadata = {.kind = 0, .flags = 1, .primary_value = 321, .element = 19,
            .granted_skill = 0x456, .price = 123456, .category_mask = 0x4020}};
    P4GameItemDesc card = {.id = 1550, .revision = 4, .key = "gear:card", .icon = {"art", "art:square", 1},
        .metadata = {.kind = P4_ITEM_CATEGORY_SKILL_CARD, .category_mask = 0x2000, .granted_skill = 0x457}};
    P4GameItemDesc heal = {.id = 1551, .revision = 1, .key = "gear:heal", .icon = {"art", "art:square", 1},
        .metadata = {.kind = 3, .flags = 0x10000, .granted_skill = 1}};
    P4GameItemDesc invalid = sword; invalid.id = 1535;
    assert(p4_game_tables_items_register(items, "gear", &invalid) == P4_TABLE_INVALID);
    invalid.id = P4_ITEM_MAX_ID + 1;
    assert(p4_game_tables_items_register(items, "gear", &invalid) == P4_TABLE_INVALID);
    invalid = sword; invalid.icon.revision = 0;
    assert(p4_game_tables_items_register(items, "gear", &invalid) == P4_TABLE_INVALID);
    invalid = sword; strcpy(invalid.icon.owner, "other");
    assert(p4_game_tables_items_register(items, "gear", &invalid) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_register(items, "gear", &sword) == P4_TABLE_OK);
    assert(p4_game_tables_items_register(items, "gear", &sword) == P4_TABLE_CONFLICT);
    key[5] = 'x'; sword.icon.key[4] = 'x'; sword.metadata.price = 0;
    assert(p4_game_tables_items_register(items, "gear", &card) == P4_TABLE_OK);
    assert(p4_game_tables_items_register(items, "gear", &heal) == P4_TABLE_OK);
    assert(p4_game_tables_items_models_register(items, "other", 1536, 1, P4_ITEM_MODEL_NONE) == P4_TABLE_CONFLICT);
    assert(p4_game_tables_items_models_register(items, "gear", 1536, 256, P4_ITEM_MODEL_NONE) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_models_register(items, "gear", 1536, 1, 1001) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_models_register(items, "gear", 1550, 1, P4_ITEM_MODEL_NONE) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_models_register(items, "gear", 1536, 1, P4_ITEM_MODEL_NONE) == P4_TABLE_OK);
    assert(p4_game_tables_items_models_register(items, "gear", 1536, 1, P4_ITEM_MODEL_NONE) == P4_TABLE_CONFLICT);
    assert(p4_game_tables_items_requirement_register(items, "gear", 1537, 0, 1, 1) == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_items_requirement_register(items, "other", 1536, 0, 1550, 3) == P4_TABLE_CONFLICT);
    assert(p4_game_tables_items_requirement_register(items, "gear", 1536, 2, 1550, 3) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_requirement_register(items, "gear", 1536, 0, P4_ITEM_MAX_ID + 1, 3) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_requirement_register(items, "gear", 1536, 0, 1550, 256) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_requirement_register(items, "gear", 1536, 0, 1550, 3) == P4_TABLE_OK);
    assert(p4_game_tables_items_requirement_register(items, "gear", 1536, 0, 1, 0) == P4_TABLE_CONFLICT);
    assert(!p4_game_tables_items_at(items, 0) && !p4_game_tables_items_base(items));
    unsigned before = allocations;
    assert(compile_items(items, file, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 9) == P4_TABLE_NOT_FOUND);
    assert(allocations == before && !p4_game_tables_items_revision(items));
    P4GameItemCategoryDesc category = {.id = 6, .revision = 2, .filter_b = 0x2000,
        .stock_policy = P4_ITEM_STOCK_NONE, .key = "gear:skill_cards", .icon = {"art", "art:category", 1}};
    P4GameItemCategoryDesc unset = category; unset.stock_policy = P4_ITEM_STOCK_UNSET;
    assert(p4_game_tables_items_category_register(items, "gear", &unset) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_category_register(items, "gear", &category) == P4_TABLE_OK);
    assert(p4_game_tables_items_category_register(items, "gear", &category) == P4_TABLE_CONFLICT);
    assert(compile_items(items, file, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 9) == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_items_action_register(items, "other", 6, NULL, card_action, &action_mode) == P4_TABLE_CONFLICT);
    assert(p4_game_tables_items_action_register(items, "gear", 7, NULL, card_action, &action_mode) == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_items_action_register(items, "gear", 6, NULL, card_action, &action_mode) == P4_TABLE_OK);
    assert(p4_game_tables_items_action_register(items, "gear", 6, NULL, card_action, &action_mode) == P4_TABLE_OK);
    assert(p4_game_tables_items_action_register(items, "gear", 6, NULL, card_action, NULL) == P4_TABLE_CONFLICT);
    assert(compile_items(items, file, P4_ITEM_RETAIL_FILE_SIZE - 1, names, descriptions, 9) == P4_TABLE_INVALID);
    file[0] = 1;
    assert(compile_items(items, file, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 9) == P4_TABLE_INVALID);
    file[0] = 0;
    fail_allocate = 1;
    assert(compile_items(items, file, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 9) == P4_TABLE_NOMEM);
    assert(!p4_game_tables_items_revision(items) && p4_game_tables_items_semantic_revision(items) == 9 &&
        p4_game_tables_items_find(items, 1536) && !p4_game_tables_items_base(items));
    fail_allocate = 0;
    model_closure(&allocator, file, names, descriptions, resources);
    /* Stable semantic references freeze without Resources. A real UI mismatch
     * rejects only the unpublished UI product and unwinds its prior pins. */
    P4GameItems *bad_icons = p4_game_tables_items_create(&allocator); assert(bad_icons);
    P4GameItemCategoryDesc wrong_icon = category; wrong_icon.icon.revision = 2;
    assert(p4_game_tables_items_register(bad_icons, "gear", &card) == P4_TABLE_OK);
    assert(p4_game_tables_items_category_register(bad_icons, "gear", &wrong_icon) == P4_TABLE_OK);
    assert(p4_game_tables_items_action_register(bad_icons, "gear", 6, NULL, card_action, &action_mode) == P4_TABLE_OK);
    assert(compile_items(bad_icons, file, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 9) == P4_TABLE_OK);
    P4GameItemsUi *refused_ui = NULL;
    assert(p4_game_tables_items_ui_prepare(bad_icons, NULL, &refused_ui) == P4_TABLE_NOT_READY && !refused_ui);
    assert(p4_game_tables_items_ui_prepare(bad_icons, resources, &refused_ui) == P4_TABLE_CONFLICT && !refused_ui);
    p4_game_tables_items_destroy(bad_icons);
    unsigned before_prepared = prepared;
    P4GameItemsUi *ui = runtime_prepare(items, file, names, descriptions, resources);
    assert(prepared == before_prepared + 1 && !p4_game_tables_items_base(items) && !p4_game_tables_items_active());
    assert(p4_game_tables_battle_prepare() == P4_TABLE_OK);
    uint32_t relocated = p4_game_tables_items_base(items); assert(relocated);
    assert(!p4_game_tables_items_active() && P4_G32(0x007642a0u) == 0x0079bf10u);
    p4_game_tables_battle_commit();
    assert(p4_game_tables_items_active() == items && P4_G32(0x007642a0u) == relocated);
    assert(p4_game_tables_items_native_metadata(0) == (const uint8_t *)p4_game_tables_items_metadata(items, 0));
    assert(p4_game_tables_items_native_metadata(1535) == (const uint8_t *)p4_game_tables_items_metadata(items, 1535));
    assert(p4_game_tables_items_native_metadata(1536) == (const uint8_t *)p4_game_tables_items_metadata(items, 1536));
    assert(p4_game_tables_items_native_row_count() == 1539);
    assert(p4_game_tables_items_native_id_at(1536) == 1536 && p4_game_tables_items_native_id_at(1537) == 1550 &&
        p4_game_tables_items_native_id_at(1538) == 1551);
    assert(p4_game_tables_items_native_list_capacity() == 1541);
    expected_icon = icon; icon_queue = p4t_galloc(0x30, 16);
    p4_game_tables_items_native_queue_icon(1536, 21, -4, 69, 0xa67f2b, 0x78, P4_H2G(icon_queue));
    assert(!icons_drawn);
    void (*draw_icon)(uint8_t *, int32_t) = (void (*)(uint8_t *, int32_t))P4_FN_G2H(P4_G32(P4_H2G(icon_node) + 8u));
    draw_icon(icon_node, (int32_t)P4_G32(P4_H2G(icon_node) + 16u));
    assert(icons_drawn == 1 && p4_game_tables_items_native_model(1536, 0) == 1);
    assert(!p4_game_tables_items_native_draw_category_icon(1536, 21, -4, 69, 0x782b7fa6u) && icons_drawn == 1);
    expected_icon = category_icon;
    assert(p4_game_tables_items_native_draw_category_icon(1550, 21, -4, 69, 0x782b7fa6u) && icons_drawn == 2);
    assert(p4_game_tables_items_native_stock_policy(1536) == P4_ITEM_STOCK_REQUIREMENTS);
    assert(p4_game_tables_items_native_stock_policy(1550) == P4_ITEM_STOCK_NONE);
    assert(p4_game_tables_items_native_stock_policy(1551) == P4_ITEM_STOCK_CALENDAR);
    assert(p4_game_tables_items_native_requirement_scan_id(767) == 1536);
    assert(p4_game_tables_items_native_requirement_scan_id(768) == -1 && p4_game_tables_items_native_requirement_scan_id(769) == -1);
    assert(p4_game_tables_items_revision(items) == 9 && p4_game_tables_items_count(items) == 3);
    assert(p4_game_tables_items_limit(items) == 1552);
    uint32_t base = p4_game_tables_items_base(items); assert(base);
    assert(P4_G32(base - 16) == 1552 && file[0] == 0 && file[1] == 6);
    assert(hash(P4_G2H(base - 12), P4_ITEM_RETAIL_FILE_SIZE - 4) == prefix);
    const P4GameItemView *view = p4_game_tables_items_find(items, 1536); assert(view);
    assert(view == p4_game_tables_items_at(items, 0) && p4_game_tables_items_ui_item(ui, view->id) == icon && view->revision == 3);
    assert(!strcmp(view->key, "gear:sword") && !strcmp(view->icon.key, "art:square") && !strcmp(view->owner, "gear"));
    assert(view->models_registered && view->right_model_id == 1 && view->left_model_id == P4_ITEM_MODEL_NONE);
    assert(view->requirements_mask == 1 && view->requirements[0].item_id == 1550 && view->requirements[0].quantity == 3);
    assert(p4_game_tables_items_requirement_register(items, "gear", 1536, 1, 0, 0) == P4_TABLE_FROZEN);
    assert(!strcmp(P4_G2H(p4_game_tables_items_native_name(view->id)), "Synthetic item"));
    const P4ItemRow *metadata = p4_game_tables_items_metadata(items, 1536); assert(metadata);
    assert(metadata->primary_value == 321 && metadata->element == 19 && metadata->granted_skill == 0x456 && metadata->price == 123456);
    assert(!p4_game_tables_items_find(items, 1537) && !p4_game_tables_items_metadata(items, 1537));
    const P4GameItemCategoryView *cv = p4_game_tables_items_category_find(items, 6);
    assert(cv && cv == p4_game_tables_items_category_at(items, 0) && cv->filter_b == 0x2000);
    assert(p4_game_tables_items_ui_category(ui, cv->id) == category_icon && cv->stock_policy == P4_ITEM_STOCK_NONE &&
        !strcmp(P4_G2H(p4_game_tables_items_native_category_name(card.id)), "Skill Cards"));
    assert(p4_game_tables_items_category_count(items) == 1 && !p4_game_tables_items_category_find(items, 7));
    assert(p4_game_tables_items_register(items, "gear", &card) == P4_TABLE_FROZEN);

    assert(p4_game_tables_items_state_set(items, 1536, 99, 255) == P4_TABLE_OK);
    assert(p4_game_tables_items_state_set(items, 1550, 7, 3) == P4_TABLE_OK);
    assert(p4_game_tables_items_state_set(items, 1551, 2, 1) == P4_TABLE_OK);
    assert(p4_game_tables_items_native_action_ready(6) && !p4_game_tables_items_native_action_ready(7));
    assert(p4_game_tables_items_native_action(1550) == P4_ITEM_ACTION_DEFERRED);
    P4GameItemState deferred_state;
    assert(p4_game_tables_items_state_get(items, 1550, &deferred_state) == P4_TABLE_OK &&
        deferred_state.quantity == 7 && deferred_state.availability == 3);
    assert(p4_game_tables_items_state_set(items, 1536, 100, 0) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_state_set(items, 1536, 0, 256) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_state_set(items, 1, 1, 1) == P4_TABLE_NOT_FOUND);
    P4GameItemState saved[3], state;
    uint32_t equipped = 99;
    assert(p4_game_tables_items_equipment_get(items, 1, 0, &equipped) == P4_TABLE_NOT_FOUND && equipped == 99);
    assert(p4_game_tables_items_equipment_set(items, 1, 0, 1536) == P4_TABLE_OK);
    assert(p4_game_tables_items_equipment_set(items, 10, 0, 1536) == P4_TABLE_OK);
    assert(p4_game_tables_items_equipment_set(items, 2, 0, 1536) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_equipment_set(items, 1, 0, 1550) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_equipment_set(items, 0, 0, 1536) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_equipment_set(items, 1, 3, 1536) == P4_TABLE_INVALID);
    assert(p4_game_tables_items_equipment_get(items, 1, 0, &equipped) == P4_TABLE_OK && equipped == 1536);
    assert(p4_game_tables_items_equipment_clear(items, 1, 0) == P4_TABLE_OK);
    assert(p4_game_tables_items_equipment_get(items, 1, 0, &equipped) == P4_TABLE_NOT_FOUND && equipped == 1536);
    assert(p4_game_tables_items_equipment_set(items, 1, 0, equipped) == P4_TABLE_OK);
    assert(p4_game_tables_items_state_get(items, 1536, &saved[0]) == P4_TABLE_OK);
    assert(p4_game_tables_items_state_get(items, 1550, &saved[1]) == P4_TABLE_OK);
    assert(p4_game_tables_items_state_get(items, 1551, &saved[2]) == P4_TABLE_OK);
    assert(saved[0].quantity == 99 && saved[0].availability == 255);
    p4_game_tables_items_state_clear(items);
    assert(p4_game_tables_items_state_get(items, 1536, &state) == P4_TABLE_OK && !state.quantity && !state.availability);
    assert(p4_game_tables_items_equipment_get(items, 1, 0, &equipped) == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_items_equipment_get(items, 10, 0, &equipped) == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_items_equipment_set(items, 1, 0, equipped) == P4_TABLE_OK);
    for (size_t i = 0; i < 3; ++i)
        assert(p4_game_tables_items_state_set(items, saved[i].id, saved[i].quantity, saved[i].availability) == P4_TABLE_OK);
    assert(p4_game_tables_items_state_get(items, 1550, &state) == P4_TABLE_OK && state.quantity == 7 && state.availability == 3);
    list_rebuild(1);
    shop_layout(1);
    uint32_t camp_bytes = p4_game_tables_items_native_camp_work_bytes();
    uint8_t *camp = p4t_galloc(camp_bytes + 16, 16); assert(camp);
    memset(camp, 0x5a, camp_bytes + 16);
    uint32_t camp_prefix = hash(camp, 0x9740);
    P4GameItemInventoryRow *inventory = p4_game_tables_items_native_inventory_rows(camp + 0x2bc);
    assert((uint8_t *)inventory == camp + 0x9740 + (767 + 3) * 4);
    quantity_calls = 0;
    assert(p4_game_tables_items_native_inventory_refresh(camp + 0x2bc) == 770);
    assert(quantity_calls == 770 && quantity_ids[768] == 1550 && quantity_ids[769] == 1551);
    assert(inventory[768].id == 1550 && inventory[768].quantity == 7);
    assert(inventory[769].id == 1551 && inventory[769].quantity == 2);
    assert(hash(camp, 0x9740) == camp_prefix);
    for (unsigned i = 0; i < 16; ++i) assert(camp[camp_bytes + i] == 0x5a);
    action_mode = P4_ITEM_ACTION_HANDLED;
    assert(p4_game_tables_items_native_action(1550) == P4_ITEM_ACTION_HANDLED);
    assert(p4_game_tables_items_state_get(items, 1550, &state) == P4_TABLE_OK && state.quantity == 6);
    action_mode = P4_ITEM_ACTION_ERROR;
    assert(p4_game_tables_items_native_action(1550) == P4_ITEM_ACTION_ERROR);
    assert(p4_game_tables_items_state_get(items, 1550, &state) == P4_TABLE_OK && state.quantity == 6);

    P4GameItems *unresolved = p4_game_tables_items_create(&allocator); assert(unresolved);
    P4GameItemDesc orphan = {.id = 1536, .revision = 1, .key = "gear:orphan", .icon = {"art", "art:square", 1}};
    assert(p4_game_tables_items_register(unresolved, "gear", &orphan) == P4_TABLE_OK);
    assert(p4_game_tables_items_requirement_register(unresolved, "gear", 1536, 0, P4_ITEM_MAX_ID, 1) == P4_TABLE_OK);
    before = allocations;
    assert(compile_items(unresolved, file, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 9) == P4_TABLE_NOT_FOUND);
    assert(allocations == before && !p4_game_tables_items_revision(unresolved));
    p4_game_tables_items_destroy(unresolved);
    for (uint32_t policy = P4_ITEM_STOCK_NONE; policy <= P4_ITEM_STOCK_REQUIREMENTS; ++policy) {
        P4GameItems *policy_items = p4_game_tables_items_create(&allocator); assert(policy_items);
        P4GameItemCategoryDesc declared = category; declared.stock_policy = policy;
        assert(p4_game_tables_items_register(policy_items, "gear", &card) == P4_TABLE_OK);
        assert(p4_game_tables_items_category_register(policy_items, "gear", &declared) == P4_TABLE_OK);
        assert(p4_game_tables_items_action_register(policy_items, "gear", 6, NULL, card_action, &action_mode) == P4_TABLE_OK);
        assert(p4_game_tables_items_requirement_register(policy_items, "gear", 1550, 0, 0x500, 2) == P4_TABLE_OK);
        before = allocations;
        int result = compile_items(policy_items, file, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 9);
        if (policy == P4_ITEM_STOCK_REQUIREMENTS) {
            assert(result == P4_TABLE_OK);
            assert(p4_game_tables_items_category_find(policy_items, 6)->stock_policy == policy);
        } else assert(result == P4_TABLE_INVALID && allocations == before && !p4_game_tables_items_revision(policy_items));
        p4_game_tables_items_destroy(policy_items);
    }

    P4GameItems *limit = p4_game_tables_items_create(&allocator); assert(limit);
    P4GameItemDesc last = {.id = P4_ITEM_MAX_ID, .revision = 1, .key = "gear:limit", .icon = {"art", "art:square", 1},
        .metadata = {.price = 42, .first_moon_gate = 0xfac0beefu}};
    assert(p4_game_tables_items_register(limit, "gear", &last) == P4_TABLE_OK);
    assert(p4_game_tables_items_models_register(limit, "gear", P4_ITEM_MAX_ID, 1, P4_ITEM_MODEL_NONE) == P4_TABLE_OK);
    assert(p4_game_tables_items_requirement_register(limit, "gear", P4_ITEM_MAX_ID, 0, P4_ITEM_MAX_ID, 255) == P4_TABLE_OK);
    assert(compile_items(limit, file, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 9) == P4_TABLE_OK);
    assert(p4_game_tables_items_limit(limit) == P4_ITEM_MAX_ID + 1);
    assert(p4_game_tables_items_metadata(limit, P4_ITEM_MAX_ID)->price == 42);
    assert(p4_game_tables_items_metadata(limit, P4_ITEM_MAX_ID)->first_moon_gate == 0xfac0beefu);
    view = p4_game_tables_items_find(limit, P4_ITEM_MAX_ID);
    assert(view && view->requirements_mask == 1 && view->requirements[0].item_id == P4_ITEM_MAX_ID && view->requirements[0].quantity == 255);
    assert(p4_game_tables_items_state_set(limit, P4_ITEM_MAX_ID, 1, 2) == P4_TABLE_OK);
    p4_game_tables_items_destroy(limit);
    assert(p4_game_tables_items_session_end(1) == P4_TABLE_OK && !p4_game_tables_items_active());
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    /* Item-only teardown must not erase the preparer needed by other families. */
    assert(p4_game_tables_prepare_register(prepare, &prepared) == P4_TABLE_CONFLICT);
    accessory_overlay(&allocator, file, names, descriptions, resources);
    battle_expansion(&allocator, file, names, descriptions, resources);
    assert(p4_game_tables_prepare_clear() == P4_TABLE_OK);
    assert(hash(P4_G2H(base - 12), P4_ITEM_RETAIL_FILE_SIZE - 4) == prefix);
    assert(hash(retail_guard, 0x30000u) == guard); /* Original retail copy, no expansion/state writes. */
    p4_resources_retire(resources);
    assert(p4_resources_query(icon) && !destroyed);
    p4_game_tables_items_ui_destroy(ui);
    p4_game_tables_items_destroy(items);
    semantic_phase(&allocator, file);
    derived_rows(&allocator, file);
    memset(file, 0, P4_ITEM_RETAIL_FILE_SIZE); free(file);
    assert(!p4_resources_query(icon) && !p4_resources_query(category_icon) && rasters == 2 && destroyed == 2);
    p4_game_tables_text_destroy(names); p4_game_tables_text_destroy(descriptions);
    assert(!live_arenas && allocations == releases + 2); /* Two intentional guest allocation failures. */
    assert(item_uses && category_uses && text_uses);
    puts("game_items: CPU semantic seal, private native bindings, exact retail rows, sparse/category IDs, full-width requirements, bounded inventory/equipment and atomic publication passed");
    return 0;
}