#include "p4pc/game_items_native.h"

/* 0012e7c0: preserve the retail300..5ff enumeration and one quantity query
 * per candidate; expanded rows live after the complete Camp allocation. */
void titleCollectFlaggedIds(u8 *menu)
{
    s16 count = (s16)p4_game_tables_items_native_inventory_refresh(menu);
    ((RadarMenu *)menu)->inventoryCount = count;
    if ((u32)count > p4_game_tables_items_native_inventory_capacity())
        kAssertFail(D_005E76C8, 0x225);
}
