/* 00106620: original byte truncation/clamp and key-item flag side effects for
 * retail IDs; new quantities never index either fixed retail array/flag bank. */
#include "p4pc/game_items_native.h"
void datItemSetCount(s16 itemId, s32 count)
{
    s16 id;
    u8 *p;
    s32 value;
    s32 x;
    s32 bit;
    s32 idx;

    if (p4_game_tables_items_native_id_is_new((u32)itemId)) {
        p4_game_tables_items_native_set_quantity((u32)itemId, count);
        return;
    }
    value = count & 0xFF;
    if (value > 0x63)
    {
        value = 0x63;
    }
    id = (s16)itemId;
    p = (u8 *)D_0079757A + id;
    *p = value;
    if (datEntryGetKind(itemId) == 4)
    {
        if (*p != 0)
        {
            x = id - 0x200;
            if ((x < 0) || (x >= 0x1600))
            {
                FUN_0046d730((const char *)D_005E4298, 0x471);
            }
            if (x == 0x1576)
            {
                func_00440b68(&iGpffff9b18);
            }
            x = (s16)itemId - 0x200;
            bit = 1 << (x % 32);
            idx = x / 32;
            D_0079B1CC[idx] |= bit;
        }
        else
        {
            s32 bit2;
            s32 idx2;

            x = id - 0x200;
            if ((x < 0) || (x >= 0x1600))
            {
                FUN_0046d730((const char *)D_005E4298, 0x471);
            }
            if (x == 0x1576)
            {
                func_00440b68(&iGpffff9b18);
            }
            x = (s16)itemId - 0x200;
            bit2 = 1 << (x % 32);
            idx2 = x / 32;
            D_0079B1CC[idx2] &= ~bit2;
        }
    }
}
