/* Keep the retail encoded-ID gate unchanged. New IDs instead use their
 * declared kind; the original granted-skill/active-flag checks still decide. */
#include "p4pc/game_items_native.h"
#include "p4pc/game_skills_native.h"
s32 datCalcIsFlaggedPersonaSkill(s32 skillId)
{
    u16 id16;
    u32 item = (u16)skillId;
    if (p4_game_tables_items_native_id_is_new((u32)(item))) {
        if (!p4_game_tables_items_native_new_consumable(item)) return 0;
    } else if (item <= 0x300 || item >= 0x400) {
        return 0;
    }
    id16 = datCalcUnderlyingSkillId(skillId) & 0xFFFF;
    if (id16 == 0) return 0;
    if (id16 >= P4_SKILL_RETAIL_ACTIVE_COUNT &&
        (id16 < P4_SKILL_RETAIL_COUNT || !p4_game_skill_active(id16))) return 0;
    return (iGpffffb3b8[id16 * 0x28 + 1] & 2) != 0;
}
