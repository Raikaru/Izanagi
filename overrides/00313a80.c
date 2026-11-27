#include "p4pc/game_mode.h"
#include "p4pc/golden_recipe.h"
s32 fclCombineSlotOwned(s8 materialCount, s8 slot)
{
    if (p4_game_mode() == P4_GAME_MODE_GOLDEN) {
        int index = p4_golden_recipe_group_index((unsigned)(u8)materialCount, (unsigned)(u8)slot);
        int result = index < 0 ? -1 : p4_golden_recipe_result((unsigned)index);
        return result > 0 && datPersonaFindRosterRecordById((u16)result) != 0;
    }
    return datPersonaFindRosterRecordById(((FclMaterialRecipe *)((u8 **)D_00642EF0)[materialCount * 2])[slot].result) != 0;
}
