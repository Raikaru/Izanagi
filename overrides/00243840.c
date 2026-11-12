/* Preserve 00243840's u16 normalization and retail bank/stride/assert. New
 * names are owned RAW game-encoded streams, not truncated 19-byte copies. */
#include "p4pc/game_skills_native.h"
#include "p4pc/guest.h"
u8 *datCalcGetIdRecord(s32 skillId)
{
    u16 id = (u16)skillId;
    if (id >= P4_SKILL_RETAIL_COUNT) {
        u32 name = p4_game_skill_name(id);
        if (name) return (u8 *)P4_G2H(name);
        kAssertFail(D_00635938, 0x14E4);
    }
    return iGpffffb448 + id * 0x13;
}
