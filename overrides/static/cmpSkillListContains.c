#include "p4pc/game_passive_grant_native.h"

/* Both callers (00138490's duplicate choice) pass a protagonist roster
 * record's slot array; after its eight slots the passive also counts when
 * granted to that Persona, the protagonist or his equipment
 * (game_passive_grant.h). With nothing granted: one load and the original miss. */
static inline s32 cmpSkillListContains(u16 *skills, u16 skill)
{
    s32 k;

    for (k = 0; k < 8; k++) {
        if (skills[k] == skill) {
            return 1;
        }
    }
    return P4_PASSIVE_ROSTER_GRANTED(skills, skill);
}
