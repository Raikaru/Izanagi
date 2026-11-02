/* 00106cd0: expanded side references precede the original signed-halfword
 * load. No reference beyond retail enters the fixed save equipment fields. */
#include "p4pc/game_items_native.h"
s16 datGetEquippedItemByUnit(s16 unitId, s16 slotIndex)
{
    s32 expanded = p4_game_tables_items_native_equipment((u32)(s32)unitId, (u32)(s32)slotIndex);
    if (expanded >= 0) return (s16)expanded;
    return (s16)p4_game_tables_items_retail_equipment(unitId, slotIndex);
}
int32_t p4_game_tables_items_retail_equipment(int32_t unit, int32_t slot)
{
    s16 unitId = (s16)unit, slotIndex = (s16)slot;
    if (unitId == 1) return (s16)D_00797574[slotIndex];
    return (s16)D_00796D8C[unitId].items[slotIndex];
}
