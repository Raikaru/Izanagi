/* 00106c80: original signed comparisons for retail; a new weapon/armor ID
 * cannot be classified as an accessory solely because its high byte is >=2. */
#include "p4pc/game_items_native.h"
s32 datIdBand(s16 itemId)
{
    if (p4_game_tables_items_native_id_is_new((u32)itemId))
        return p4_game_tables_items_native_band((u32)itemId);
    if (itemId < 0x100)
    {
        return 0;
    }
    if (itemId < 0x200)
    {
        return 1;
    }
    return 2;
}
