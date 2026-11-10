/* Native 001f3010, retaining hit/status/death/recovery ownership. The copied
 * observer records requested deltas separately from effective resource writes;
 * in particular a zero-delta Guard-clear packet can clamp an over-max save. */
#include "mdl_effect_internal.h"
#include "member_access.h"
#include "btl_target_internal.h"
#include "btl_motion_internal.h"
#include "btl_target_state_packet_internal.h"
#include "btl_action_internal.h"
#include "p4pc/guest.h"
#include "p4pc/battle_events.h"
#include "p4pc/battle_events_native.h"

s32 btlTargetApplyHpSpChange(u8 *work)
{
    s32 wasStatusHigh;
    s32 wasDead;
    s32 preserveHp;
    s32 wasLowHp;
    s32 wasStatusOne;
    s32 spDelta;
    s32 hpDelta;
    s32 unitFlags;
    s32 level;
    s32 buffMask;
    u16 hitFlags;
    u8 targetGenus;
    u8 sourceGenus;
    u8 *deadUnit;
    u8 *action;
    u8 *unit;
    s32 *maxLevel;
    u8 *battle;
    u8 *killBattle;
    u8 *levelBattle;
    u8 *source;
    BtlTargetPacketWork *packetWork = (BtlTargetPacketWork *)work;
    BtlUnit *targetUnit;
    BtlUnit *defeatedUnit;
    BtlAction *targetAction;
    u32 observed;
    P4BattleHit observedHit;

    action = (u8 *)(((BtlTargetPacketWork *)work)->action);
    targetAction = (BtlAction *)action;
    if ((((BtlAction *)action)->unk_1a & 1) == 0) {
        return 1;
    }
    unit = (u8 *)(((BtlAction *)action)->unit);
    targetUnit = (BtlUnit *)unit;
    observed = p4_battle_observer_interest & P4_BATTLE_OBSERVE_HIT;
    if (observed) p4_battle_hit_begin(&observedHit, P4_H2G(work));
    wasDead = datCalcIsDead((u8 *)targetUnit->datUnit, 0);
    wasLowHp = datCalcIsLowHp((u8 *)targetUnit->datUnit);
    wasStatusOne = datCalcChkBadStatus((s32)(u8 *)targetUnit->datUnit, 1);
    wasStatusHigh = datCalcChkBadStatus((s32)(u8 *)targetUnit->datUnit, 0x100000);
    if (packetWork->targetFlags & BTL_HIT_RESULT_CRITICAL) {
        ((BtlAction *)action)->flags = ((BtlAction *)action)->flags | BTL_ACTION_FLAG_HIT_CRITICAL;
    }
    if (((BtlTargetPacketWork *)work)->targetFlags & BTL_HIT_RESULT_WEAK) {
        ((BtlAction *)action)->flags = ((BtlAction *)action)->flags | BTL_ACTION_FLAG_HIT_WEAK;
    }
    if ((s32)(((BtlTargetPacketWork *)work)->result.addedStatus) & 0x100000) {
        ((BtlAction *)action)->flags = ((BtlAction *)action)->flags | BTL_ACTION_FLAG_DOWN;
    }
    if (((BtlTargetPacketWork *)work)->result.flags & 0x200) {
        ((BtlAction *)action)->unk_1a = ((BtlAction *)action)->unk_1a | 0x80;
    }
    if (packetWork->result.flags & 0x80) {
        if (datCalcIsGuarding((u8 *)targetUnit->datUnit) != 0) {
            datCalcClearStatusBlocked((u8 *)(((BtlUnit *)unit)->datUnit));
        } else {
            datCalcSetStatusBlocked((u8 *)(((BtlUnit *)unit)->datUnit));
        }
    }
    if ((s32)(((BtlBattleState *)iGpffffb3ac)->flags) & 0x10) {
        datCalcAddHp((u8 *)(((BtlUnit *)unit)->datUnit), ((BtlTargetPacketWork *)work)->result.hpDelta);
        datCalcAddSp((u8 *)targetUnit->datUnit, ((BtlTargetPacketWork *)work)->result.spDelta);
        datAddMoney(((BtlTargetPacketWork *)work)->result.field14);
        if (observed) observedHit.resources_applied = 1;
    }
    hpDelta = ((BtlTargetPacketWork *)work)->result.hpDelta;
    if ((hpDelta < 0) || (spDelta = ((BtlTargetPacketWork *)work)->result.spDelta, spDelta < 0)) {
        btlPanelStartRingEffect(unit);
        if ((packetWork->result.flags & 0x300) == 0) {
            datCalcClearStatusBlocked((u8 *)(((BtlUnit *)unit)->datUnit));
        }
    } else if ((hpDelta > 0) || (spDelta > 0)) {
        btlPanelStartPulseEffect(unit);
    }
    if ((s32)(((BtlBattleState *)iGpffffb3ac)->flags) & 0x20) {
        btlSoundClearStatusCounters(action, MEMBER(s32, work, BtlTargetPacketWork, result.addedStatus));
        datCalcSetBadStatus((u8 *)((BtlUnit *)unit)->datUnit, MEMBER(s32, work, BtlTargetPacketWork, result.addedStatus));
        datCalcClearBadStatus((u8 *)targetUnit->datUnit, MEMBER(s32, work, BtlTargetPacketWork, result.removedStatus));
        if (observed) observedHit.statuses_applied = 1;
    }
    buffMask = (s32)(((BtlTargetPacketWork *)work)->result.field10);
    if (buffMask != 0) {
        datCalcApplyBuffMask((u8 *)(((BtlUnit *)unit)->datUnit), buffMask, 1);
    }
    if ((datCalcIsDead((u8 *)(((BtlUnit *)unit)->datUnit), 0) != 0) && (wasDead == 0)) {
        if (((BtlUnit *)unit)->genus == 1) {
            battle = iGpffffb3ac;
            ((BtlBattleState *)battle)->unkC5A = ((BtlBattleState *)battle)->unkC5A + 1;
            if (packetWork->result.flags & 8) {
                killBattle = iGpffffb3ac;
                ((BtlBattleState *)killBattle)->unkC5C = ((BtlBattleState *)killBattle)->unkC5C + 1;
            }
            level = datCalcGetLevel((u8 *)targetUnit->datUnit) & 0xff;
            levelBattle = iGpffffb3ac;
            ((BtlBattleState *)levelBattle)->unkCE4 = ((BtlBattleState *)levelBattle)->unkCE4 + level;
            maxLevel = &((BtlBattleState *)iGpffffb3ac)->maximumDefeatedLevel;
            if (*maxLevel < level) {
                *maxLevel = level;
            }
        }
        preserveHp = (packetWork->result.flags & 0x10) != 0;
        deadUnit = (u8 *)targetAction->unit;
        defeatedUnit = (BtlUnit *)deadUnit;
        datCalcClearStatusBlocked((u8 *)defeatedUnit->datUnit);
        if (preserveHp == 1) {
            if (((BtlAction *)action)->targetBlock.recovery.enduranceSkill != 0x154) {
                datCalcSetHp((u8 *)(((BtlUnit *)deadUnit)->datUnit), 1);
            }
            datCalcClearBadStatus((u8 *)(((BtlUnit *)deadUnit)->datUnit), 0x80000);
            ((BtlUnit *)deadUnit)->flags3 = (s32)(((BtlUnit *)deadUnit)->flags3) | 0x10;
        } else {
            datCalcSetHp((u8 *)(((BtlUnit *)deadUnit)->datUnit), 0);
            datCalcClearBadStatus((u8 *)defeatedUnit->datUnit, 0xffffff);
            datCalcSetBadStatus((u8 *)defeatedUnit->datUnit, 0x80000);
            unitFlags = (s32)(((BtlUnit *)deadUnit)->flags3) | 1;
            ((BtlUnit *)deadUnit)->flags3 = unitFlags;
            ((BtlUnit *)deadUnit)->flags3 = unitFlags & ~0x10;
        }
        if (datCalcIsDead((u8 *)targetUnit->datUnit, 0) != 0) {
            if (((BtlBattleState *)iGpffffb3ac)->selectedAction == action) {
                datEntryDrawGatedByFlag39(0x19, 0x96, 0x19, 0);
                btlActionRunTargetCallback(action, 0x10, ((s32)packetWork->result.addedStatus & 0x80000) != 0, 0, 2);
            } else {
                btlActionRunTargetCallback(action, 0x1c, 0, 0, 0);
            }
        }
    } else if ((datCalcIsLowHp((u8 *)targetUnit->datUnit) != 0) && (wasLowHp == 0)) {
        btlActionRunTargetCallback(action, 0x1b, 0, 0, 0);
    } else {
        targetGenus = targetAction->unit->genus;
        if ((targetGenus == 0) && (targetGenus != ((BtlTargetPacketWork *)work)->source->unit->genus)) {
            if ((datCalcChkBadStatus((s32)(u8 *)targetUnit->datUnit, 1) != 0) && (wasStatusOne == 0)) {
                btlActionRunTargetCallback(action, 0x16, 0, 0, 0);
            } else if ((datCalcChkBadStatus((s32)(u8 *)targetUnit->datUnit, 0x100000) != 0) && (wasStatusHigh == 0)) {
                btlActionRunTargetCallback(action, 0x17, 0, 0, 0);
            } else if (((BtlTargetPacketWork *)work)->effect & 6) {
                btlActionRunTargetCallback(action, 0x1a, 0, 0, 0);
            }
        } else if ((datCalcChkBadStatus((s32)(u8 *)targetUnit->datUnit, 0x100000) == 0) || (wasStatusHigh == 1)) {
            source = (u8 *)(((BtlTargetPacketWork *)work)->source);
            sourceGenus = ((BtlUnit *)((u8 *)(((BtlAction *)source)->unit)))->genus;
            if ((sourceGenus == 0) &&
                (sourceGenus != targetAction->unit->genus) &&
                !(((BtlTargetPacketWork *)work)->effect & 4) &&
                (packetWork->result.hpDelta != 0 || ((BtlTargetPacketWork *)work)->result.spDelta != 0 ||
                 (s32)packetWork->result.addedStatus != 0 || (s32)((BtlTargetPacketWork *)work)->result.removedStatus != 0 ||
                 (s32)packetWork->result.field10 != 0)) {
                hitFlags = packetWork->targetFlags;
                if (hitFlags & 2) {
                    btlActionRunTargetCallback(source, 0x14, 0, 0, 0);
                } else if (hitFlags & 4) {
                    btlActionRunTargetCallback(source, 0x15, 0, 0, 0);
                }
            }
        }
    }
    btlUnitUpdateHpGauge((u8 *)(((BtlTargetPacketWork *)work)->source), action, (s32 *)(work + 8));
    btlUnitUpdateStatusMotion(unit, 6);
    if (observed) p4_battle_hit_end(&observedHit, P4_H2G(action));
    return 1;
}
