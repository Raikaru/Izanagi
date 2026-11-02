#include "game_items_internal.h"
#include "p4pc/game_items_ui.h"
#include "p4pc/gmem.h"
#include "p4pc/image.h"
#include "p4pc/package_boot.h"
#include "../files/vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ItemBootStage {
    P4GameItems *items;
    P4GameItemsUi *ui;
    int expanded, source_required;
} ItemBootStage;
static ItemBootStage staged;
static P4GameItems *active;
static P4GameItemsUi *active_ui;
static const P4GameItems *source_items;
static unsigned item_loaded; /* 0 absent, 1 actual loaded bytes, 2 retail copy complete */
static int item_preparing;
static int prepare_complete(void *user);
static void commit_complete(void *user);

static int expansion_pending(void) { return staged.expanded; }
static void use_item(uint32_t id)
{
    int result = p4_game_tables_items_runtime_use(active, id);
    if (result) p4_game_tables_items_native_refuse(result);
}
static void use_category(uint32_t id)
{
    int result = p4_game_tables_items_category_runtime_use(active, id);
    if (result) p4_game_tables_items_native_refuse(result);
}
int p4_game_tables_items_source_stage(const P4GameItems *items)
{
    if (!items) return P4_TABLE_INVALID;
    int result = p4_game_tables_items_native_admit(items);
    if (result) return result;
    if (!p4_game_tables_items_count(items) && !p4_game_tables_items_category_count(items)) {
        P4GameItemDefinition original;
        result = p4_game_tables_items_definition_find(items, 0, &original);
        if (result) return result; /* Genuine original-prefix proof only, never an invented empty source. */
    }
    if (item_loaded || staged.items || active) return P4_TABLE_FROZEN;
    if (source_items) return source_items == items ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    source_items = items;
    return P4_TABLE_OK;
}
int p4_game_tables_items_boot_stage(P4GameItems *items)
{
    if (!items || !p4_game_tables_items_semantic_revision(items)) return P4_TABLE_NOT_READY;
    if (item_preparing) return P4_TABLE_CONFLICT;
    if (staged.items) return staged.items == items ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    int expanded = p4_game_tables_items_count(items) || p4_game_tables_items_category_count(items);
    int result = p4_game_tables_items_native_admit(items);
    if (result) return result;
    P4GameItemDefinition original;
    int prefix = p4_game_tables_items_definition_find(items, 0, &original);
    if (prefix && prefix != P4_TABLE_NOT_FOUND) return prefix;
    const int source_required = expanded || !prefix;
    if (source_items && source_items != items) return P4_TABLE_CONFLICT;
    if (source_required && source_items != items) return P4_TABLE_NOT_READY;
    if (source_required) {
        result = p4_game_tables_items_validate_runtime_sources(items);
        if (result) return result;
    }
    result = p4_game_tables_battle_complete_register("item", prepare_complete, commit_complete, NULL);
    if (result) return result;
    if (source_required) {
        result = p4_game_tables_battle_complete_require("item", "table-source");
        if (result) {
            int rollback = p4_game_tables_battle_complete_unregister("item", prepare_complete, commit_complete, NULL);
            return rollback ? rollback : result;
        }
    }
    staged = (ItemBootStage){items, NULL, expanded, source_required};
    return P4_TABLE_OK;
}
int p4_game_tables_items_boot_unstage(P4GameItems *items)
{
    if (!items) return P4_TABLE_INVALID;
    if (item_preparing) return P4_TABLE_CONFLICT;
    if (item_loaded || active || p4_game_tables_items_revision(items)) return P4_TABLE_FROZEN;
    if ((staged.items && staged.items != items) || (source_items && source_items != items)) return P4_TABLE_CONFLICT;
    if (staged.items != items && source_items != items) return P4_TABLE_OK;
    if (staged.items == items) {
        int result = p4_game_tables_battle_complete_unregister("item", prepare_complete, commit_complete, NULL);
        if (result) return result;
        staged = (ItemBootStage){0};
    }
    if (source_items == items) source_items = NULL;
    return P4_TABLE_OK;
}
int p4_game_tables_items_ui_stage(P4GameItemsUi *ui)
{
    if (!staged.items) return P4_TABLE_NOT_READY;
    if (p4_game_tables_items_ui_model(ui) != staged.items)
        return !ui && !staged.expanded ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    if (staged.ui) return staged.ui == ui ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    staged.ui = ui;
    return P4_TABLE_OK;
}
int p4_game_tables_items_ui_detached(const P4GameItemsUi *ui)
{ return !ui || (staged.ui != ui && active_ui != ui); }
P4GameItems *p4_game_tables_items_active(void) { return active; }
uint32_t p4_game_tables_items_native_gear_scan_count(void)
{
    return 0x2ffu + (uint32_t)p4_game_tables_items_count(p4_game_tables_items_active());
}
int p4_game_tables_items_native_action_ready(uint32_t category)
{
    if (!p4_game_tables_items_action_bound(active, category)) return 0;
    use_category(category);
    return 1;
}
int p4_game_tables_items_native_action(uint32_t id)
{
    const P4ItemRow *row = (const P4ItemRow *)p4_game_tables_items_native_metadata(id);
    if (row->kind >= 6) use_category(row->kind);
    return p4_game_tables_items_action_dispatch(active, id);
}
int p4_game_tables_items_native_new_consumable(uint32_t id)
{
    if (id < P4_ITEM_RETAIL_COUNT) return 0;
    const P4ItemRow *row = p4_game_tables_items_metadata(active, id);
    if (!row) return 0;
    use_item(id);
    return row->kind == 3;
}
uint32_t p4_game_tables_items_native_stock_policy(uint32_t id)
{
    const P4ItemRow *row = (const P4ItemRow *)p4_game_tables_items_native_metadata(id);
    if (row->kind < 3) return P4_ITEM_STOCK_REQUIREMENTS;
    if (row->kind == 3) return P4_ITEM_STOCK_CALENDAR;
    if (row->kind < 6) return P4_ITEM_STOCK_NONE;
    const P4GameItemCategoryView *category = p4_game_tables_items_category_find(active, row->kind);
    if (!category) p4_game_tables_items_native_refuse(P4_TABLE_NOT_FOUND);
    use_category(category->id);
    return category->stock_policy;
}
int32_t p4_game_tables_items_native_requirement_scan_id(uint32_t ordinal)
{
    if (ordinal < 0x2ffu) return (int32_t)ordinal;
    const P4GameItemView *row = p4_game_tables_items_at(active, ordinal - 0x2ffu);
    if (!row) p4_game_tables_items_native_refuse(P4_TABLE_NOT_FOUND);
    return p4_game_tables_items_native_stock_policy(row->id) == P4_ITEM_STOCK_REQUIREMENTS ? (int32_t)row->id : -1;
}
int p4_game_tables_items_session_end(int image_live)
{
    if (item_preparing) return P4_TABLE_CONFLICT;
    uint32_t base = active ? p4_game_tables_items_base(active) : 0;
    if (image_live && base && P4_G32(0x007642a0u) == base) P4_G32(0x007642a0u) = 0x0079bf10u;
    active = NULL; active_ui = NULL; source_items = NULL;
    staged = (ItemBootStage){0}; item_loaded = 0;
    return P4_TABLE_OK;
}
static int retail_model_file(uint32_t id, const char *suffix)
{
    char path[256];
    int length = snprintf(path, sizeof path, "%sweapon/wp%03X%s.RMD",
        (const char *)P4_G2H(0x007636e0u), (unsigned)id, suffix);
    if (length < 0 || (size_t)length >= sizeof path) return P4_TABLE_INVALID;
    long long offset, size;
    return !vfs_disc_locate(path, &offset, &size) && size > 0 ? P4_TABLE_OK : P4_TABLE_NOT_FOUND;
}
static int check_retail_models(void *unused, uint32_t right, uint32_t left, uint32_t category_mask)
{
    (void)unused;
    if (!(category_mask & 0x40u) || (category_mask & 0x7fa0u)) {
        int result = retail_model_file(right, "");
        if (result) return result;
    }
    if (category_mask & 0x40u) {
        int result = retail_model_file(right, "_R");
        if (result) return result;
        result = retail_model_file(left - 1000u, "_L");
        if (result) return result;
    }
    if (category_mask & 0x80u) return retail_model_file(right, "_c1");
    return P4_TABLE_OK;
}
static int prepare_complete(void *user)
{
    (void)user;
    if (staged.source_required && (!p4pc_image_loaded() || item_loaded != 2)) return P4_TABLE_NOT_READY;
    if (expansion_pending() && p4_game_tables_items_ui_model(staged.ui) != staged.items) return P4_TABLE_NOT_READY;
    if (staged.source_required) {
        int result = p4_game_tables_items_check_prefix(staged.items, P4_G2H(0x0079bf00u), P4_ITEM_RETAIL_FILE_SIZE);
        if (result) return result;
    }
    if (expansion_pending()) {
        int result = p4_game_tables_items_check_models(staged.items, check_retail_models, NULL);
        if (result) return result;
    }
    return p4_game_tables_items_revision(staged.items) ? P4_TABLE_OK :
        p4_game_tables_items_materialize(staged.items);
}
static void commit_complete(void *user)
{
    (void)user;
    active = staged.expanded ? staged.items : NULL;
    active_ui = staged.expanded ? staged.ui : NULL;
    uint32_t base = active ? p4_game_tables_items_base(active) : 0;
    if (base) P4_G32(0x007642a0u) = base;
}
int p4_game_tables_items_native_load(const char *source_label, const uint8_t *bytes, uint32_t size)
{
    if (!p4pc_image_loaded()) return P4_TABLE_NOT_READY;
    if (item_preparing) return P4_TABLE_CONFLICT;
    if (source_items) {
        /* The real sealed source validator checks winning-byte identity BEFORE
         * the copy and routes exposed-data failures to controlled restart/Quit.
         * No guest allocation, text/image compilation or gameplay publication. */
        item_preparing = 1;
        int result = p4_game_tables_prepare("item", source_label, P4_ITEM_RETAIL_COUNT, bytes, size);
        item_preparing = 0;
        if (result) return result;
    }
    if (!bytes || size != P4_ITEM_RETAIL_FILE_SIZE ||
        bytes[0] || bytes[1] != 6 || bytes[2] || bytes[3]) return P4_TABLE_INVALID;
    item_loaded = 1;
    return P4_TABLE_OK;
}
void p4_game_tables_items_native_use_builtin(void)
{
    if (!p4pc_image_loaded()) p4_game_tables_items_native_refuse(P4_TABLE_NOT_READY);
    uint32_t base = active ? p4_game_tables_items_base(active) : 0;
    /* Original00111848..78 copies ONLY the retail payload, destroys its file
     * and calls00104c50. Preserve an already-active immutable base on reload;
     * first expansion publishes later, solely in canonical native commit. */
    P4_G32(0x007642a0u) = base ? base : 0x0079bf10u;
    if (item_loaded == 1) item_loaded = 2;
}
_Noreturn void p4_game_tables_items_native_refuse(int result)
{
    char reason[192];
    snprintf(reason, sizeof reason, "Item table publication/use refused (%d): %s", result, p4_game_tables_error(result));
    p4_package_boot_runtime_failed(reason);
}
int p4_game_tables_items_native_expanded(void)
{ return active && p4_game_tables_items_count(active) != 0; }
uint32_t p4_game_tables_items_native_list_capacity(void)
{
    /* List positions are not sparse item IDs. Reserve the kind-4 heading and
     * the original accessors' index==count lookahead, without another32766-row
     * array merely because one declared ID is high. Count itself stays s16. */
    return p4_game_tables_items_native_expanded() ? p4_game_tables_items_native_row_count() + 2u : P4_ITEM_RETAIL_COUNT;
}
uint32_t p4_game_tables_items_native_work_bytes(void)
{
    uint32_t capacity = p4_game_tables_items_native_list_capacity();
    return 0x10u + capacity * (p4_game_tables_items_native_expanded() ? 8u : 4u);
}
uint32_t p4_game_tables_items_native_row_count(void)
{ return P4_ITEM_RETAIL_COUNT + (active ? (uint32_t)p4_game_tables_items_count(active) : 0u); }
uint32_t p4_game_tables_items_native_id_at(uint32_t index)
{
    if (index < P4_ITEM_RETAIL_COUNT) return index;
    const P4GameItemView *row = p4_game_tables_items_at(active, index - P4_ITEM_RETAIL_COUNT);
    if (!row) p4_game_tables_items_native_refuse(P4_TABLE_NOT_FOUND);
    return row->id;
}
static P4GameItemMutable *mutable(uint32_t id)
{
    P4GameItemMutable *state = p4_game_tables_items_mutable(active, id);
    if (!state) p4_game_tables_items_native_refuse(P4_TABLE_NOT_FOUND);
    use_item(id);
    return state;
}
int32_t p4_game_tables_items_native_quantity(uint32_t id) { return mutable(id)->quantity; }
void p4_game_tables_items_native_set_quantity(uint32_t id, int32_t quantity)
{
    uint32_t value = (uint8_t)quantity;
    mutable(id)->quantity = (uint8_t)(value > 99 ? 99 : value);
    /* New key items never alias the retail game's global quest-flag bank. */
}
int32_t p4_game_tables_items_native_availability(uint32_t id) { return mutable(id)->availability; }
void p4_game_tables_items_native_set_availability(uint32_t id, uint32_t availability)
{ mutable(id)->availability = (uint8_t)availability; }
static const P4GameItemView *view(uint32_t id)
{
    const P4GameItemView *row = p4_game_tables_items_find(active, id);
    if (!row) p4_game_tables_items_native_refuse(P4_TABLE_NOT_FOUND);
    use_item(id);
    return row;
}
static uint32_t text_binding(uint32_t table, uint32_t id, uint32_t role)
{
    uint32_t stream = p4_game_tables_items_text_binding(active, table, id, role);
    if (!stream) p4_game_tables_items_native_refuse(P4_TABLE_NOT_READY);
    if (table == P4_TEXT_ITEM) use_item(id);
    else if (table == P4_TEXT_ITEM_CATEGORY) use_category(id);
    int result = p4_game_tables_items_text_runtime_use(active, table, id, role);
    if (result) p4_game_tables_items_native_refuse(result);
    return stream;
}
uint32_t p4_game_tables_items_native_name(uint32_t id) { return text_binding(P4_TEXT_ITEM, id, P4_TEXT_NAME); }
uint32_t p4_game_tables_items_native_description(uint32_t id) { return text_binding(P4_TEXT_ITEM, id, P4_TEXT_DESCRIPTION); }
uint32_t p4_game_tables_items_native_category_name(uint32_t id)
{
    if (id < P4_ITEM_RETAIL_COUNT) return 0;
    const P4ItemRow *row = (const P4ItemRow *)p4_game_tables_items_native_metadata(id);
    if (row->kind < 6) return 0;
    const P4GameItemCategoryView *category = p4_game_tables_items_category_find(active, row->kind);
    if (!category) p4_game_tables_items_native_refuse(P4_TABLE_NOT_FOUND);
    return text_binding(P4_TEXT_ITEM_CATEGORY, category->id, P4_TEXT_NAME);
}
const uint8_t *p4_game_tables_items_native_metadata(uint32_t id)
{
    if ((!active || !p4_game_tables_items_base(active)) && id < P4_ITEM_RETAIL_COUNT) {
        const uint32_t base = P4_G32(0x007642a0u);
        if (!base) p4_game_tables_items_native_refuse(P4_TABLE_NOT_READY);
        return P4_G2H(base + id * P4_ITEM_ROW_SIZE);
    }
    const P4ItemRow *row = p4_game_tables_items_metadata(active, id);
    if (!row) p4_game_tables_items_native_refuse(P4_TABLE_NOT_FOUND);
    use_item(id);
    return (const uint8_t *)row;
}
static void draw_resource_icon(uint32_t token, float x, float y, float depth, uint32_t rgba, float scale_y)
{
    const int result = p4_resources_draw(token, x, y, depth, rgba, 1.0f, scale_y);
    if (result != P4_RESOURCE_OK)
        p4_game_tables_items_native_refuse(result == P4_RESOURCE_NOT_READY ? P4_TABLE_NOT_READY :
            result == P4_RESOURCE_NOT_FOUND ? P4_TABLE_NOT_FOUND : P4_TABLE_RENDER_ERROR);
}
void p4_game_tables_items_native_draw_icon(uint32_t id, float x, float y, float depth, uint32_t rgba, float scale_y)
{
    use_item(id);
    uint32_t token = p4_game_tables_items_ui_item(active_ui, id);
    if (!token) p4_game_tables_items_native_refuse(P4_TABLE_NOT_FOUND);
    draw_resource_icon(token, x, y, depth, rgba, scale_y);
}
int p4_game_tables_items_native_draw_category_icon(uint32_t id, float x, float y, float depth, uint32_t rgba)
{
    const P4ItemRow *row = (const P4ItemRow *)p4_game_tables_items_native_metadata(id);
    if (row->kind < 6) return 0;
    const P4GameItemCategoryView *category = p4_game_tables_items_category_find(active, row->kind);
    if (!category) p4_game_tables_items_native_refuse(P4_TABLE_NOT_FOUND);
    use_category(category->id);
    uint32_t token = p4_game_tables_items_ui_category(active_ui, category->id);
    if (!token) p4_game_tables_items_native_refuse(P4_TABLE_NOT_FOUND);
    draw_resource_icon(token, x, y, depth, rgba, 1.0f);
    return 1;
}
uint32_t p4_game_tables_items_native_model(uint32_t id, uint32_t left)
{
    if (left > 1) p4_game_tables_items_native_refuse(P4_TABLE_INVALID);
    const P4GameItemView *row = view(id);
    uint32_t model = left ? row->left_model_id : row->right_model_id;
    if (!row->models_registered || model == P4_ITEM_MODEL_NONE)
        p4_game_tables_items_native_refuse(P4_TABLE_NOT_FOUND);
    return model;
}
typedef struct ItemIconPacket {
    uint32_t id;
    float x, y, depth;
    uint32_t rgba;
} ItemIconPacket;
_Static_assert(sizeof(ItemIconPacket) == 20, "item icon packet must fit a native queue node");
extern uint8_t *PrimAllocQueueNode(void);
extern void PrimAppendNode(uint8_t *, uint8_t *);
static void draw_item_icon(uint8_t *node, int32_t argument)
{
    (void)node;
    const ItemIconPacket *packet = P4_G2H((uint32_t)argument);
    p4_game_tables_items_native_draw_icon(packet->id, packet->x, packet->y, packet->depth, packet->rgba, 1.0f);
}
void p4_game_tables_items_native_queue_icon(uint32_t id, float x, float y, float depth,
    uint32_t rgb, uint32_t alpha, uint32_t queue)
{
    /* Validate the owned binding at enqueue as well as draw. Boot-only item
     * snapshots retain their resource pins until all native task queues drain. */
    (void)view(id);
    uint8_t *node = PrimAllocQueueNode();
    if (!node) p4_game_tables_items_native_refuse(P4_TABLE_NOMEM);
    const uint32_t address = P4_H2G(node);
    const ItemIconPacket packet = {id, x, y, depth,
        ((rgb >> 16) & 255u) | (rgb & 0xff00u) | ((rgb & 255u) << 16) | ((alpha & 255u) << 24)};
    /* Byte 24 is the renderer's native command discriminator, not payload.
     * Keep it zero so float coordinates cannot accidentally request fill mode. */
    P4_G32(address + 8u) = p4_fn_h2g_sig(draw_item_icon, "vpi");
    P4_G32(address + 12u) = 0;
    P4_G32(address + 16u) = address + 28u;
    P4_G32(address + 20u) = 0;
    P4_G32(address + 24u) = 0;
    memcpy(node + 28, &packet, sizeof packet);
    PrimAppendNode(P4_G2H(queue), node);
}
static const P4GameItemRequirement *requirement(uint32_t id, uint32_t gate)
{
    if (gate > 1) p4_game_tables_items_native_refuse(P4_TABLE_INVALID);
    const P4GameItemView *row = p4_game_tables_items_find(active, id);
    if (row) use_item(id);
    return row && (row->requirements_mask & (1u << gate)) ? &row->requirements[gate] : NULL;
}
uint32_t p4_game_tables_items_native_requirement_item(uint32_t id, uint32_t gate, uint32_t packed)
{
    const P4GameItemRequirement *value = requirement(id, gate);
    return value ? value->item_id : (packed & 0xfff00u) >> 8;
}
uint32_t p4_game_tables_items_native_requirement_quantity(uint32_t id, uint32_t gate, uint32_t packed)
{
    const P4GameItemRequirement *value = requirement(id, gate);
    return value ? value->quantity : packed & 255u;
}
int32_t p4_game_tables_items_native_requirement_empty(uint32_t id, uint32_t gate, uint32_t packed)
{
    const P4GameItemRequirement *value = requirement(id, gate);
    return value ? !value->item_id && !value->quantity : packed == 0;
}
int32_t p4_game_tables_items_native_band(uint32_t id)
{
    const P4ItemRow *row = (const P4ItemRow *)p4_game_tables_items_native_metadata(id);
    return row->kind < 2u ? (int32_t)row->kind : 2;
}
int32_t p4_game_tables_items_native_equipment(uint32_t unit, uint32_t slot)
{
    uint32_t id;
    if (p4_game_tables_items_equipment_get(active, unit, slot, &id) != P4_TABLE_OK) return -1;
    use_item(id);
    return (int32_t)id;
}
int p4_game_tables_items_native_set_equipment(uint32_t unit, uint32_t slot, int32_t id)
{
    if (id < (int32_t)P4_ITEM_RETAIL_COUNT) {
        if (active) p4_game_tables_items_equipment_clear(active, unit, slot);
        return 0; /* The original retail setter remains authoritative. */
    }
    use_item((uint32_t)id);
    int result = p4_game_tables_items_equipment_set(active, unit, slot, (uint32_t)id);
    if (result) p4_game_tables_items_native_refuse(result);
    return 1;
}