/* 00110830's recovered name is misleading: this reads the item state table.
 * Retain the original u8 result ABI and signed retail index. */
#include "p4pc/game_items_native.h"
u8 clndGetMoonPhase(s32 index)
{
    if (p4_game_tables_items_native_id_is_new((u32)(index)))
        return (u8)p4_game_tables_items_native_availability((u32)index);
    return D_0079B6D8[index];
}
