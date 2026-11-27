#include "p4pc/game_mode.h"
#include "p4pc/golden_recipe.h"
s8 fclCombineItemSlot(s8 materialCount, u16 persona)
{
    s32 i;
    s32 id;
    u8 **table;
    if (p4_game_mode() == P4_GAME_MODE_GOLDEN) {
        for (unsigned row = 0; row < P4_GOLDEN_RECIPE_MAX; ++row) {
            int index = p4_golden_recipe_group_index((unsigned)(u8)materialCount, row);
            if (index < 0) break;
            if (p4_golden_recipe_result((unsigned)index) == persona) return (s8)row;
        }
        return 0;
    }
    i = 0;
    id = persona;
    table = (u8 **)((u8 *)D_00642F00 + materialCount * 8);
    for (; i < 6; i++) {
        if (!operandOrderNotEqual(((FclMaterialRecipe *)table[-4])[i].result, id)) {
            return i;
        }
    }
    return 0;
}
