#include "p4pc/game_items.h"
#include <string.h>

_Static_assert(sizeof(P4GameItemEquipmentRow) == 4, "native equipment row ABI");

uint32_t p4_game_tables_items_native_camp_work_bytes(void)
{
    /* Both submenus remain at their original root offsets (item2bc, gear31dc).
     * Append their row arrays after the entire Camp work, not inside either
     * submenu: live animation/other menus extend through9740. */
    return p4_game_tables_items_native_expanded() ?
        0x9740u + p4_game_tables_items_native_gear_scan_count() * sizeof(P4GameItemEquipmentRow) +
            p4_game_tables_items_native_inventory_capacity() * sizeof(P4GameItemInventoryRow) : 0x9740u;
}
P4GameItemEquipmentRow *p4_game_tables_items_native_equipment_rows(void *work)
{
    return (P4GameItemEquipmentRow *)((uint8_t *)work +
        (p4_game_tables_items_native_expanded() ? 0x9740u - 0x31dcu : 0x4au));
}
void p4_game_tables_items_native_equipment_reset(void *work)
{
    /* The original initializer still clears its exact1598 bytes. Reset only
     * the appended rows here; reopening a submenu must not expose stale rows. */
    if (p4_game_tables_items_native_expanded())
        memset(p4_game_tables_items_native_equipment_rows(work), 0,
            p4_game_tables_items_native_gear_scan_count() * sizeof(P4GameItemEquipmentRow));
}
_Static_assert(sizeof(P4GameItemInventoryRow) == 4, "native inventory row ABI");
uint32_t p4_game_tables_items_native_inventory_capacity(void)
{
    return 768u + p4_game_tables_items_native_row_count() - P4_ITEM_RETAIL_COUNT;
}
P4GameItemInventoryRow *p4_game_tables_items_native_inventory_rows(void *work)
{
    return (P4GameItemInventoryRow *)((uint8_t *)work +
        (p4_game_tables_items_native_expanded() ?
            0x9740u - 0x2bcu + p4_game_tables_items_native_gear_scan_count() * sizeof(P4GameItemEquipmentRow) : 0x3eu));
}
extern int32_t datItemCount(int16_t id);
uint16_t p4_game_tables_items_native_inventory_refresh(void *work)
{
    P4GameItemInventoryRow *rows = p4_game_tables_items_native_inventory_rows(work);
    uint16_t count = 0;
    for (uint32_t id = 0x300; id < 0x600; ++id) {
        int16_t quantity = (int16_t)(datItemCount((int16_t)id) & 255);
        if (quantity) rows[count++] = (P4GameItemInventoryRow){(int16_t)id, quantity};
    }
    const P4GameItems *items = p4_game_tables_items_active();
    for (size_t i = 0, total = p4_game_tables_items_count(items); i < total; ++i) {
        const P4GameItemView *row = p4_game_tables_items_at(items, i);
        const P4ItemRow *metadata = (const P4ItemRow *)p4_game_tables_items_native_metadata(row->id);
        if (metadata->kind < 3) continue;
        int16_t quantity = (int16_t)(datItemCount((int16_t)row->id) & 255);
        if (quantity) rows[count++] = (P4GameItemInventoryRow){(int16_t)row->id, quantity};
    }
    return count;
}
