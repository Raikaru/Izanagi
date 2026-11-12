/* The 576-byte signed grade bank is fixed retail data, not part of the
 * 40-byte battle metadata. Expanded rows carry their explicit grade. */
#include "p4pc/game_skills_native.h"
s32 datGetSelectorByte(s16 index)
{
    if ((u16)index >= P4_SKILL_RETAIL_COUNT && index >= 0)
        return p4_game_skill_inheritance_rate((u16)index);
    return D_00641E60[(s16)index];
}
