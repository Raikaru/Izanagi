#include "p4pc/game_passive_grant_native.h"

/* Search eight fixed skills, then the party accessory-granted skill, then the
 * unit's passive grants (game_passive_grant.h); with nothing granted the last
 * test is one load and the original miss. */
static inline s32 datCalcHasFixedSkill(u8 *unit, s32 requestedSkill)
{
    s32 skill;
    s32 index;
    u8 *table;
    s32 equipment;

    table = datCalcFixedSkillTable(unit);
    index = 0;
    skill = requestedSkill & 0xFFFF;
    while ((index & 0xFFFF) < 8) {
        if (skill == ((u16 *)(table))[(u16)index]) {
            return 1;
        }
        index = (index + 1) & 0xFFFF;
    }
    if (!(((DatUnit *)unit)->flags & 4)) {
        equipment = (u32)(u16)datGetEquippedItemByUnit((s16)((DatUnit *)(unit))->id, 2);
        if (equipment >= 0) {
            if (skill == datItemGrantedSkill((s16)equipment)) {
                return 1;
            }
        }
    }
    return P4_PASSIVE_UNIT_GRANTED(unit, skill);
}
