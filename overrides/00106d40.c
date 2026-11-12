/* 00106d40: this second retail setter has the same three-halfword layout
 * as 00105dc0; both must observe/clear the same expanded reference overlay. */
#include "p4pc/game_items_native.h"
void datAssignEquippedItem(s16 unitId, s16 slotIndex, s16 itemId)
{
    if (p4_game_tables_items_native_set_equipment((u32)(s32)unitId, (u32)(s32)slotIndex, itemId)) return;
    if (unitId == 1)
    {
        D_00797574[slotIndex] = itemId;
    }
    else
    {
        D_00796D8C[unitId].items[slotIndex] = itemId;
    }
}
