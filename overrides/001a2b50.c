/* Native 001a2b50: the AI-to-READY transition is the battle decision stage
 * (p4.battle.decide, include/p4pc/battle_decide.h). Every enemy choice passes
 * here once the AI released its command record: table picks, script
 * SetActionCode/Param decodes and the summon plan alike. With no enabled
 * decider this is the retail body. */
#include "p4pc/guest.h"
#include "p4pc/battle_decide.h"
#include "p4pc/mod_entry.h"
extern void btlAICommandSetActionCode(u8 *, u8 *, u16);

void btlActionAiUpdate(u8 *action)
{
    s32 battleFlags;

    if (btlEffectHasAiProcedure(action + 0x38) == 0) {
        btlAICommandSetUnitAction(action, action + 0x38);
    }
    if (btlAICommandTryReleaseProc(action + 0x38) != 0) {
        if (((BtlAction *)action)->unit->genus == 0 &&
            p4_ai_item_denied(((BtlAction *)action)->unit->charId, action)) {
            if (p4_ai_item_finish(((BtlAction *)action)->unit->charId, action))
                btlAICommandSetActionCode(action, action + 0x38, 0x8002);
        }
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
