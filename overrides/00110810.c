/* 00110810 is the item availability/state byte setter, not a calendar field. */
#include "p4pc/game_items_native.h"
void datSetStateTableEntry(s32 index, u8 flags)
{
    if (p4_game_tables_items_native_id_is_new((u32)(index))) {
        p4_game_tables_items_native_set_availability((u32)index, flags);
        return;
    }
    D_0079B6D8[index] = flags;
}
