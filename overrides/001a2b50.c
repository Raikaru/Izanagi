/* Native 001a2b50: the AI-to-READY transition is the battle decision stage
 * (p4.battle.decide, include/p4pc/battle_decide.h). Every enemy choice passes
 * here once the AI released its command record: table picks, script
 * SetActionCode/Param decodes and the summon plan alike. With no enabled
 * decider this is the retail body. */
#include "p4pc/guest.h"
#include "p4pc/battle_decide.h"

void btlActionAiUpdate(u8 *action)
{
    s32 battleFlags;

    if (btlEffectHasAiProcedure(action + 0x38) == 0) {
        btlAICommandSetUnitAction(action, action + 0x38);
    }
    if (btlAICommandTryReleaseProc(action + 0x38) != 0) {
        if (p4_battle_decide_interest)
            p4_battle_decide_ready(P4_H2G(action));
        battleFlags = ((BtlBattleState *)D_0076449C)->flags;
        if ((battleFlags & 0x1000) &&
            (battleFlags & 0x04000000) &&
            ((BtlAction *)action)->unit->genus == 1) {
            ((BtlAction *)action)->flags |= 0x4000;
        }
        ((BtlAction *)action)->flags |= 2;
        btlActionSetState((BtlAction *)action, 0xF);
    }
}
