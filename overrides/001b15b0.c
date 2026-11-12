/* Native 001b15b0, with copied post-admission events at the actual inner
 * admitted actor. Queue reordering and native state/turn ownership stay here. */
#include "btl_action_internal.h"
#include "btl_effect_internal.h"
#include "p4pc/guest.h"
#include "p4pc/battle_events.h"
#include "p4pc/battle_events_native.h"

extern u8 *D_0076449C;
extern void btlActionSetState(BtlAction *action, u16 state);
extern void btlBossCollectTargets(u8 *);
extern s32 datCalcUnitClass(u8 *);
extern void btlActionQueueSortByRank(s32);
extern void btlActionQueueRequeueLeadingEnemies(s32);

void btlActionUpdateQueue(void)
{
    u16 queueFlags;
    u16 dispatchedState;
    u8 *unit;
    u8 *activeAction;
    u8 *countBattle;
    u8 *activeBattle;
    u8 *enemyOrderBattle;
    u8 *partyOrderBattle;
    u8 *bossOrderBattle;
    u8 *advanceBattle;
    u8 *dispatchedBattle;
    u8 *action;
    u8 *bossBattle;
    u8 *actionSlot;
    u8 *battle;

    battle = D_0076449C;
    queueFlags = ((BtlBattleState *)battle)->queueFlags;
    if ((queueFlags & 1) == 0) goto done;
    if ((*(s32 *)&((BtlBattleState *)battle)->flags & 0x80000) != 0) goto done;
    if ((queueFlags & 8) == 0) goto done;
    actionSlot = battle + 0x2CC;
    action = *(u8 **)actionSlot;
    if (action != NULL) {
        if (((BtlAction *)action)->currState != 1) goto done;
        ((BtlAction *)action)->flags |= 4;
        activeAction = *(u8 **)actionSlot;
        dispatchedState = *(u16 *)&((BtlAction *)activeAction)->unk_16;
        btlActionSetState((BtlAction *)activeAction, dispatchedState);
        countBattle = D_0076449C;
        ((BtlBattleState *)countBattle)->actionCount += 1;
        activeBattle = D_0076449C;
        ((BtlBattleState *)activeBattle)->queueFlags &= 0xFFF7;
        if (p4_battle_observer_interest & P4_BATTLE_OBSERVE_ADMISSION)
            p4_battle_emit_admission(P4_H2G(activeAction), P4_BATTLE_QUEUE_FOLLOWUP,
                                    ((BtlBattleState *)countBattle)->actionCount, dispatchedState);
        goto done;
    }
    actionSlot = D_0076449C + 0x29C;
    if (((BtlBattleState *)D_0076449C)->actionCount == 0) goto after_count;
    if ((queueFlags & 2) != 0) {
        action = *(u8 **)actionSlot;
        if (((BtlUnit *)*(u8 * *)&((BtlAction *)action)->unit)->genus == 1) {
            btlActionQueueSortByRank(1);
            btlActionQueueRequeueLeadingEnemies(1);
            enemyOrderBattle = D_0076449C;
            ((BtlBattleState *)enemyOrderBattle)->queueFlags &= 0xFFFD;
        }
    }
    if ((((BtlBattleState *)D_0076449C)->queueFlags & 4) != 0) {
        action = *(u8 **)actionSlot;
        if (((BtlUnit *)*(u8 * *)&((BtlAction *)action)->unit)->genus == 0) {
            btlActionQueueSortByRank(1);
            btlActionQueueRequeueLeadingEnemies(1);
            partyOrderBattle = D_0076449C;
            ((BtlBattleState *)partyOrderBattle)->queueFlags &= 0xFFFB;
        }
    }
    bossBattle = D_0076449C;
    if ((*(s32 *)&((BtlBattleState *)bossBattle)->flags & 0x200000) != 0 &&
        (((BtlBattleState *)bossBattle)->queueFlags & 0x10) != 0) {
        btlBossCollectTargets(0);
        bossOrderBattle = D_0076449C;
        ((BtlBattleState *)bossOrderBattle)->queueFlags &= 0xFFEF;
    }
after_count:
    action = *(u8 **)actionSlot;
    if (action == NULL) goto done;
    if (((BtlAction *)action)->currState != 1) goto done;
    unit = *(u8 * *)&((BtlAction *)action)->unit;
    if (((BtlUnit *)unit)->genus == 0) {
        if ((s16)datCalcUnitClass(*(u8 * *)&((BtlUnit *)unit)->datUnit) == 4) {
            *(s16 *)&((BtlAction *)*(u8 **)actionSlot)->state = 5;
        } else {
            *(s16 *)&((BtlAction *)*(u8 **)actionSlot)->state = 8;
        }
    } else {
        *(s16 *)&((BtlAction *)*(u8 **)actionSlot)->state = 8;
    }
    action = *(u8 **)actionSlot;
    btlActionSetState((BtlAction *)action, 2);
    advanceBattle = D_0076449C;
    ((BtlBattleState *)advanceBattle)->queueFlags &= 0xFFF7;
    dispatchedBattle = D_0076449C;
    ((BtlBattleState *)dispatchedBattle)->actionCount += 1;
    if (p4_battle_observer_interest & P4_BATTLE_OBSERVE_ADMISSION)
        p4_battle_emit_admission(P4_H2G(action), P4_BATTLE_QUEUE_ORDINARY,
                                ((BtlBattleState *)dispatchedBattle)->actionCount, 2);
done:
    ;
}
