/* 001067f0: retail IDs retain their exact ELF string pointer map. */
#include "p4pc/game_items_native.h"
u32 datGetItemNameId(s16 itemId)
{
    if (p4_game_tables_items_native_id_is_new((u32)(itemId)))
        return p4_game_tables_items_native_name((u32)itemId);
    return D_005E2200[itemId];
}
