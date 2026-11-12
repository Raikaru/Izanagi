/* 002e71c0: the original consumable bank keeps its exact 300..3fe loop;
 * new consumables are explicit rows, not another guessed encoded bank. */
#include "p4pc/game_items_native.h"
s32 fclUnlockMoonPhaseItems(void)
{
    s16 item;
    s32 found;
    s16 itemCopy;

    found = 0;
    item = 0x300;
    goto loop_test;
loop_body:
    if (fclItemIsDateAvailable(item) == 1)
    {
        if ((clndGetMoonPhase(itemCopy) & 0xFF) == 0)
        {
            datSetStateTableEntry(item, (clndGetMoonPhase(item) & 0xFF) | 1);
            found = 1;
        }
        else if (((u8)clndGetMoonPhase(itemCopy)) & 1)
        {
            found = 1;
        }
    }
    item++;
loop_test:
    itemCopy = (s16)item;
    if (itemCopy < 0x3FF)
    {
        goto loop_body;
    }
    /* Always visit expanded rows, even if a retail row already set found. */
    if (p4_game_tables_items_native_unlock_consumables() == 1) found = 1;
    return (s8)found == 1;
}
