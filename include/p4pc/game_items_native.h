#ifndef P4PC_GAME_ITEMS_NATIVE_H
#define P4PC_GAME_ITEMS_NATIVE_H
#include <stdint.h>
#include "p4pc/table_result.h"
#include "p4pc/game_text_native.h"
/* Game-unit bridge: only scalar fields and guest-backed buffer arguments.
 * Never import host allocators, registry descriptors or resource ownership
 * structs into translated game units and their foreign-field census. */
#define P4_ITEM_RETAIL_COUNT 1536u
#define P4_ITEM_ROW_SIZE 68u
#define P4_ITEM_FILE_HEADER_SIZE 16u
#define P4_ITEM_RETAIL_FILE_SIZE 104464u
/* Signed ID/count consumers reserve one extra row for the kind-4 heading. */
#define P4_ITEM_MAX_ID 32765u
#define P4_ITEM_CATEGORY_SKILL_CARD 6u
/* Classify only the allocated ID domain, never signed/packed retail sentinels.
 * Membership is checked separately; unknown new IDs cannot fall back to retail. */
static inline int p4_game_tables_items_native_id_is_new(uint32_t id)
{
    return id >= P4_ITEM_RETAIL_COUNT && id <= P4_ITEM_MAX_ID;
}
/* 00112610/00113280 carry inverse opacity; resource RGBA uses actual alpha.
 * FCL's separate alpha argument and Camp row alpha use their own direct recipe. */
static inline uint32_t p4_game_tables_items_native_inverse_alpha_rgba(uint32_t packed_color)
{
    return packed_color ^ 0xff000000u;
}
/* FCL detail dispatch uses atlas codes, but new gear is classified by its
 * declared metadata kind, never by arbitrary/overlapping retail icon flags. */
static inline int32_t p4_game_tables_items_native_gear_detail_code(uint32_t metadata_kind)
{
    switch (metadata_kind) {
    case 0: return 20;
    case 1: return 27;
    case 2: return 28;
    default: return -1;
    }
}
typedef enum P4GameItemStockPolicy {
    P4_ITEM_STOCK_UNSET = 0, P4_ITEM_STOCK_NONE = 1,
    P4_ITEM_STOCK_CALENDAR = 2, P4_ITEM_STOCK_REQUIREMENTS = 3
} P4GameItemStockPolicy;
typedef enum P4GameItemActionResult {
    P4_ITEM_ACTION_ERROR = -1, P4_ITEM_ACTION_HANDLED = 1, P4_ITEM_ACTION_DEFERRED = 2
} P4GameItemActionResult;
typedef int (*P4GameItemActionFn)(void *user, uint32_t item_id, uint32_t category);
int p4_game_tables_items_native_action_ready(uint32_t category);
int p4_game_tables_items_native_action(uint32_t item_id);

/* Native body adapters, not raw-pointer/guest-address APIs for Luau. Actual
 * loaded-success observation validates before the ORIGINAL retail copy. It
 * never allocates/relocates/publishes an expanded table in001117c0. */
int p4_game_tables_items_native_load(const char *source_label,
    const uint8_t *retail_bytes, uint32_t size);
void p4_game_tables_items_native_use_builtin(void);
_Noreturn void p4_game_tables_items_native_refuse(int result);
int p4_game_tables_items_native_new_consumable(uint32_t id);
int p4_game_tables_items_native_expanded(void);
uint32_t p4_game_tables_items_native_list_capacity(void);
uint32_t p4_game_tables_items_native_work_bytes(void);
void p4_game_tables_items_native_rebuild(uint8_t *work, int32_t filter_a, int32_t filter_b, int8_t kind, int32_t order);
typedef struct P4GameItemBattleRow { int16_t id; uint8_t quantity, reserved; } P4GameItemBattleRow;
uint32_t p4_game_tables_items_native_battle_work_bytes(void);
P4GameItemBattleRow *p4_game_tables_items_native_battle_rows(uint8_t *panel);
uint16_t p4_game_tables_items_native_battle_refresh(uint8_t *panel);
typedef struct P4GameItemEquipmentRow { int16_t id, quantity; } P4GameItemEquipmentRow;
uint32_t p4_game_tables_items_native_camp_work_bytes(void);
P4GameItemEquipmentRow *p4_game_tables_items_native_equipment_rows(void *work);
void p4_game_tables_items_native_equipment_reset(void *work);
typedef struct P4GameItemInventoryRow { int16_t id, quantity; } P4GameItemInventoryRow;
uint32_t p4_game_tables_items_native_inventory_capacity(void);
P4GameItemInventoryRow *p4_game_tables_items_native_inventory_rows(void *work);
uint16_t p4_game_tables_items_native_inventory_refresh(void *work);
uint32_t p4_game_tables_items_native_stock_policy(uint32_t id);
int32_t p4_game_tables_items_native_requirement_scan_id(uint32_t ordinal);
void p4_game_tables_items_native_stock_defaults(void);
uint32_t p4_game_tables_items_native_shop_work_bytes(void);
int16_t *p4_game_tables_items_native_shop_unlocked(void *work);
int16_t p4_game_tables_items_native_shop_cursor(void *work, uint32_t tab);
void p4_game_tables_items_native_shop_save_cursor(void *work, uint32_t tab, int16_t cursor);
uint32_t p4_game_tables_items_native_material_work_bytes(void);
int32_t *p4_game_tables_items_native_material_rows(void *work);
uint32_t p4_game_tables_items_native_gear_scan_count(void);
int32_t p4_game_tables_items_native_gear_scan_id(uint32_t ordinal);
uint32_t p4_game_tables_items_native_requirement_item(uint32_t id, uint32_t gate, uint32_t packed);
uint32_t p4_game_tables_items_native_requirement_quantity(uint32_t id, uint32_t gate, uint32_t packed);
int32_t p4_game_tables_items_native_requirement_empty(uint32_t id, uint32_t gate, uint32_t packed);
void p4_game_tables_items_native_stock_clear(void);
int32_t p4_game_tables_items_native_unlock_consumables(void);
uint32_t p4_game_tables_items_native_row_count(void);
uint32_t p4_game_tables_items_native_id_at(uint32_t index);
int32_t p4_game_tables_items_native_quantity(uint32_t id);
void p4_game_tables_items_native_set_quantity(uint32_t id, int32_t quantity);
int32_t p4_game_tables_items_native_availability(uint32_t id);
void p4_game_tables_items_native_set_availability(uint32_t id, uint32_t availability);
uint32_t p4_game_tables_items_native_name(uint32_t id);
/* One metadata getter for both domains: the active immutable projection when
 * expanded, otherwise the game's actual loaded retail base. Call after item
 * loading; an unknown new ID never aliases a retail row or bank. */
const uint8_t *p4_game_tables_items_native_metadata(uint32_t id);
uint32_t p4_game_tables_items_native_description(uint32_t id);
uint32_t p4_game_tables_items_native_category_name(uint32_t id);
void p4_game_tables_items_native_draw_icon(uint32_t id, float x, float y, float depth, uint32_t rgba, float scale_y);
int p4_game_tables_items_native_draw_category_icon(uint32_t id, float x, float y, float depth, uint32_t rgba);
/* Uses the actual primitive queue. The active item snapshot must remain alive
 * until its task queues are drained; rgb is the FCL 0xRRGGBB convention. */
void p4_game_tables_items_native_queue_icon(uint32_t id, float x, float y, float depth,
    uint32_t rgb, uint32_t alpha, uint32_t queue);
uint32_t p4_game_tables_items_native_model(uint32_t id, uint32_t left);
void p4_game_tables_items_queue_description(int16_t id, int32_t color, int32_t font, int32_t mode,
    int32_t table, int32_t list_index, float x, float y, float depth);
int32_t p4_game_tables_items_native_band(uint32_t id);
int32_t p4_game_tables_items_native_equipment(uint32_t unit, uint32_t slot);
int p4_game_tables_items_native_set_equipment(uint32_t unit, uint32_t slot, int32_t id);
/* Exact original signed-halfword expressions, no overlay/source/runtime use.
 * The native equipment getter shares this body after its explicit side lookup. */
int32_t p4_game_tables_items_retail_equipment(int32_t unit, int32_t slot);
#endif
