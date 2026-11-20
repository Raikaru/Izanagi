#include "p4pc/game_passive_grant_native.h"

/* Search learned skills, then the accessory-granted skill, then the unit's
 * passive grants (game_passive_grant.h); with nothing granted the last test is
 * one load and the original miss. */
static inline s32 datCalcHasRequestedSkill(DatUnit *unit, s32 requestedSkill)
{
    s32 skill;
    s32 count;
    s32 index;
    u8 *base;
    s32 result;
    s32 limit;

    count = datCalcGetTableLength(unit) & 0xFFFF;
    base = datCalcSkillList(unit);
    index = 0;
    skill = requestedSkill & 0xFFFF;
    limit = count & 0xFFFF;
    while ((index & 0xFFFF) < limit) {
        if (skill == ((u16 *)base)[(u16)index]) {
            return 1;
        }
        index = (index + 1) & 0xFFFF;
    }
    if (!(unit->flags & 4)) {
        result = (s32)((u32)(u16)datGetEquippedItemByUnit((s16)((DatUnit *)((u8 *)unit))->id, 2) & 0xFFFF);
        if (result >= 0) {
            if (skill == datItemGrantedSkill((s16)result)) {
                return 1;
            }
        }
    }
    return P4_PASSIVE_UNIT_GRANTED(unit, skill);
}
