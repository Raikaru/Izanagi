/* 002bdea0: retain each original 0->4 store for retail stock, then apply the
 * same policy to explicit new equipment/consumable rows, never key-item flags. */
#include "p4pc/game_items_native.h"
void fclClearMoonFlagEntries(void)
{
    s32 itemId;

    for (itemId = 0; itemId < 0x3FF; itemId++) {
        u8 flags;

        flags = clndGetMoonPhase(itemId);
        if (flags & 2) {
            datSetStateTableEntry(itemId, 0);
            datSetStateTableEntry(itemId, 4);
        }
    }
    p4_game_tables_items_native_stock_clear();
}
