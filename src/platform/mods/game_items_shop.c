#include "p4pc/game_items.h"

extern uint32_t datEntryGetKind(int16_t);
extern uint32_t datItemMoonGateFirst(int16_t);
extern uint32_t datItemMoonGateSecond(int16_t);
extern int8_t datItemAvailableMonth(int16_t);
extern int8_t datItemAvailableDay(int16_t);
extern int32_t fclCalendarMonthOrder(int32_t);
extern int32_t fclItemIsDateAvailable(int16_t);
extern uint8_t clndGetMoonPhase(int32_t);
extern void datSetStateTableEntry(int32_t, uint8_t);

int32_t p4_game_tables_items_native_gear_scan_id(uint32_t ordinal)
{
    if (ordinal < 0x2ffu) return (int32_t)ordinal;
    const P4GameItemView *row = p4_game_tables_items_at(p4_game_tables_items_active(), ordinal - 0x2ffu);
    if (!row) p4_game_tables_items_native_refuse(P4_TABLE_NOT_FOUND);
    /* The old encoded domain consists only of weapon/armor/accessory rows.
     * Select the same policy by actual native kind for declared new IDs. */
    return datEntryGetKind((int16_t)row->id) < 3u ? (int32_t)row->id : -1;
}
uint32_t p4_game_tables_items_native_shop_work_bytes(void)
{
    return p4_game_tables_items_native_expanded() ?
        0x1174u + p4_game_tables_items_native_gear_scan_count() * sizeof(int16_t) + 3u * sizeof(int16_t) : 0x1174u;
}
int16_t *p4_game_tables_items_native_shop_unlocked(void *work)
{
    /* 002ca830 allocates the complete task work; 002ca770 releases that same
     * block. Keep all original C16..1173 fields and child addresses in place. */
    return (int16_t *)((uint8_t *)work + (p4_game_tables_items_native_expanded() ? 0x1174u : 0x16u));
}
static int16_t *wide_cursors(void *work)
{
    return p4_game_tables_items_native_shop_unlocked(work) + p4_game_tables_items_native_gear_scan_count();
}
int16_t p4_game_tables_items_native_shop_cursor(void *work, uint32_t tab)
{
    if (tab >= 3) p4_game_tables_items_native_refuse(P4_TABLE_INVALID);
    return p4_game_tables_items_native_expanded() ? wide_cursors(work)[tab] : ((int8_t *)work)[0x1168u + tab];
}
void p4_game_tables_items_native_shop_save_cursor(void *work, uint32_t tab, int16_t cursor)
{
    if (tab >= 3) p4_game_tables_items_native_refuse(P4_TABLE_INVALID);
    if (p4_game_tables_items_native_expanded()) wide_cursors(work)[tab] = cursor;
    else ((int8_t *)work)[0x1168u + tab] = (int8_t)cursor;
}
uint32_t p4_game_tables_items_native_material_work_bytes(void)
{
    return p4_game_tables_items_native_expanded() ?
        0x180cu + p4_game_tables_items_native_row_count() * sizeof(int32_t) : 0x180cu;
}
int32_t *p4_game_tables_items_native_material_rows(void *work)
{
    return (int32_t *)((uint8_t *)work + (p4_game_tables_items_native_expanded() ? 0x180cu : 4u));
}

/* Retail loops retain exact encoded domains. New rows use the real declared
 * stock policy; category6 and unknown flags never imply gear or date stock.
 * Existing getters/setters remain the gameplay hook boundaries. */
void p4_game_tables_items_native_stock_defaults(void)
{
    P4GameItems *items = p4_game_tables_items_active();
    size_t count = p4_game_tables_items_count(items);
    for (size_t i = 0; i < count; ++i) {
        int16_t id = (int16_t)p4_game_tables_items_at(items, i)->id;
        uint32_t policy = p4_game_tables_items_native_stock_policy((uint16_t)id);
        if (policy == P4_ITEM_STOCK_REQUIREMENTS) {
            if (p4_game_tables_items_native_requirement_item(id, 0, datItemMoonGateFirst(id)) == 0 &&
                p4_game_tables_items_native_requirement_item(id, 1, datItemMoonGateSecond(id)) == 0)
                datSetStateTableEntry(id, (uint8_t)(clndGetMoonPhase(id) | 4));
            else if (!p4_game_tables_items_native_requirement_quantity(id, 0, datItemMoonGateFirst(id)) &&
                !p4_game_tables_items_native_requirement_quantity(id, 1, datItemMoonGateSecond(id)))
                datSetStateTableEntry(id, (uint8_t)(clndGetMoonPhase(id) | 4));
        } else if (policy == P4_ITEM_STOCK_CALENDAR) {
            if ((fclCalendarMonthOrder((uint8_t)datItemAvailableMonth(id)) & 255) < (fclCalendarMonthOrder(4) & 255))
                datSetStateTableEntry(id, (uint8_t)(clndGetMoonPhase(id) | 4));
            else if ((fclCalendarMonthOrder((uint8_t)datItemAvailableMonth(id)) & 255) == (fclCalendarMonthOrder(4) & 255) &&
                datItemAvailableDay(id) < 2)
                datSetStateTableEntry(id, (uint8_t)(clndGetMoonPhase(id) | 4));
        }
    }
}
void p4_game_tables_items_native_stock_clear(void)
{
    P4GameItems *items = p4_game_tables_items_active();
    size_t count = p4_game_tables_items_count(items);
    for (size_t i = 0; i < count; ++i) {
        int16_t id = (int16_t)p4_game_tables_items_at(items, i)->id;
        if (p4_game_tables_items_native_stock_policy((uint16_t)id) != P4_ITEM_STOCK_NONE && (clndGetMoonPhase(id) & 2)) {
            datSetStateTableEntry(id, 0);
            datSetStateTableEntry(id, 4);
        }
    }
}
int32_t p4_game_tables_items_native_unlock_consumables(void)
{
    P4GameItems *items = p4_game_tables_items_active();
    size_t count = p4_game_tables_items_count(items);
    int32_t found = 0;
    for (size_t i = 0; i < count; ++i) {
        int16_t id = (int16_t)p4_game_tables_items_at(items, i)->id;
        if (p4_game_tables_items_native_stock_policy((uint16_t)id) == P4_ITEM_STOCK_CALENDAR && fclItemIsDateAvailable(id) == 1) {
            if (clndGetMoonPhase(id) == 0) {
                datSetStateTableEntry(id, (uint8_t)(clndGetMoonPhase(id) | 1));
                found = 1;
            } else if (clndGetMoonPhase(id) & 1) found = 1;
        }
    }
    return found;
}
