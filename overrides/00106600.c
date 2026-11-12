/* 00106600: signed-halfword retail index and byte result remain unchanged. */
#include "p4pc/game_items_native.h"
s32 datItemCount(s16 itemId)
{
    if (p4_game_tables_items_native_id_is_new((u32)(itemId)))
        return p4_game_tables_items_native_quantity((u32)itemId);
    return D_0079757A[itemId];
}
