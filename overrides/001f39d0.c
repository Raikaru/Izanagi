/* Native 001f39d0: observe real committed costs, not net HP/SP changes.
 * Reuse its computed native cost; no second cost calculation or RNG call. */
#include "member_access.h"
#include "btl_action_internal.h"
#include "p4pc/guest.h"
#include "p4pc/battle_events.h"
#include "p4pc/battle_events_native.h"

extern u8 *iGpffffb3b8;
extern s32 datItemCount(s16 itemId);
extern void datItemSetCount(s16 itemId, s32 count);
extern s32 datCalcAddHp(DatUnit *unit, s32 hpDelta);
extern s32 datCalcAddSp(DatUnit *unit, s32 spDelta);
extern u32 datCalcSkillCost(u8 *unit, s32 skillId);

s32 btlTargetConsumeActionCost(u8 **actionPacket)
{
    u8 *action;
    u16 skillId;
    u8 *unit;
    s32 cost;
    u16 actionKind;
    u8 costKind;
    s32 off;
    u8 *base;
    u8 *ptr;
    s16 itemId;
    u32 observed;
    u16 hpBefore, spBefore;
    s32 countBefore;

    action = *actionPacket;
    unit = (u8 *)((BtlAction *)action)->unit;
    actionKind = ((BtlAction *)action)->unk_6c;
    switch (actionKind) {
    case 2:
        skillId = ((BtlAction *)action)->unk_6e;
        cost = datCalcSkillCost((u8 *)((BtlUnit *)unit)->datUnit, skillId);
        if (cost != 0) {
            base = iGpffffb3b8;
            off = (s32)((skillId & 0xFFFF) * 0x28);
            ptr = (u8 *)(off + (s32)base);
            costKind = ptr[3];
            observed = (p4_battle_observer_interest & P4_BATTLE_OBSERVE_COST) &&
                       (costKind == 1 || costKind == 2);
            if (observed) {
                hpBefore = ((BtlUnit *)unit)->datUnit->hp;
                spBefore = ((BtlUnit *)unit)->datUnit->sp;
            }
            switch (costKind) {
            case 1:
                datCalcAddHp((DatUnit *)MEMBER(s32, unit, BtlUnit, datUnit), -cost);
                break;
            case 2:
                datCalcAddSp((DatUnit *)MEMBER(s32, unit, BtlUnit, datUnit), -cost);
                break;
            }
            if (observed)
                p4_battle_emit_cost(P4_H2G(action), costKind, (u32)cost,
                                   hpBefore, spBefore, 0, 0, 0);
        }
        break;
    case 3:
        if (((BtlUnit *)unit)->genus == 0) {
            actionKind = ((BtlActionParameters *)((BtlAction *)action)->unk_70)->item;
            itemId = (s16)actionKind;
            countBefore = datItemCount((s16)actionKind) & 0xFF;
            observed = p4_battle_observer_interest & P4_BATTLE_OBSERVE_COST;
            if (observed) {
                hpBefore = ((BtlUnit *)unit)->datUnit->hp;
                spBefore = ((BtlUnit *)unit)->datUnit->sp;
            }
            datItemSetCount(itemId, (countBefore - 1) & 0xFF);
            if (observed)
                p4_battle_emit_cost(P4_H2G(action), P4_BATTLE_COST_ITEM, 1,
                                   hpBefore, spBefore, (u16)itemId,
                                   (u32)countBefore, (u32)(datItemCount(itemId) & 0xFF));
        }
        break;
    case 9:
    case 1:
        break;
    }
    return 1;
}
