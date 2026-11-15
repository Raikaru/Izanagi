/* 001f6930: a unit's per-turn ailment recovery decision. Each eligible roll's
 * chance passes the ailment wear-off stage before datCalcRandomBelow(100),
 * which still draws exactly as retail; the stage never runs while no modifier
 * is registered. The callees keep this unit's own declarations. */
#include "p4pc/game_afflictions_native.h"
#include "p4pc/game_wearoff_stage_native.h"
#pragma push
/* measured: retail leaves loop invariants inside the cure loop; hoisting them makes the loop
 * larger. */
#pragma opt_loop_invariants off
/* measured: retail does not propagate copies in this function. */
#pragma opt_propagation off
/* Build the status-cure mask from per-status probabilities, counters and passive skills. */
s32 btlSoundSelectCureableStatuses(u8 *action)
{
    BtlUnit *unitState;
    extern s32 func_00244f60(u32 action, u16 *value, u16 *option, u16 extra, s32 flags);
    extern s32 datCalcGetLevel(u8 *unit);
    extern u8 datCalcGetUnitStat(DatUnit *unit, s32 statIndex);
    s32 cureMask;
    u32 statusIndex;
    s32 chance;
    s32 requiredTurns;
    s32 countedStatus;
    u8 *unit;
    s32 statusBit;
    s32 statuses;
    u16 *fearData;
    u16 *panicData;
    u16 *rageData;
    u16 *exhaustionData;
    u16 *silenceData;
    u16 *poisonData;
    s32 hasTurnThreshold;

    unit = *(u8 **)&((BtlAction *)action)->unit;
    unitState = (BtlUnit *)unit;
    /* The untyped DatUnit load keeps retail register allocation across the status and skill calls. */
    statuses = datCalcGetBadStatus(*(s32 *)&unitState->datUnit);
    cureMask = 0;
    statusIndex = 0;
    goto loop_31_test;
loop_31_body:
    countedStatus = 1;
    statusBit = 1 << statusIndex;
    if (statuses & statusBit) {
        switch (statusBit) {
        case 0x10:
            fearData = *(u16 **)&((BtlUnit *)unit)->datUnit;
            chance = func_00244f60(8, fearData, fearData, 0xFFFF, 0x10);
            requiredTurns = 3;
            hasTurnThreshold = countedStatus;
            break;
        case 0x4:
            panicData = *(u16 **)&((BtlUnit *)unit)->datUnit;
            chance = func_00244f60(8, panicData, panicData, 0xFFFF, 4);
            requiredTurns = 3;
            hasTurnThreshold = countedStatus;
            break;
        case 0x2:
            rageData = *(u16 **)&((BtlUnit *)unit)->datUnit;
            chance = func_00244f60(8, rageData, rageData, 0xFFFF, 2);
            requiredTurns = 3;
            hasTurnThreshold = countedStatus;
            break;
        case 0x40:
            exhaustionData = *(u16 **)&((BtlUnit *)unit)->datUnit;
            chance = func_00244f60(8, exhaustionData, exhaustionData, 0xFFFF, 0x40);
            requiredTurns = 3;
            hasTurnThreshold = countedStatus;
            break;
        case 0x80:
            silenceData = *(u16 **)&((BtlUnit *)unit)->datUnit;
            chance = func_00244f60(8, silenceData, silenceData, 0xFFFF, 0x80);
            requiredTurns = 6;
            hasTurnThreshold = countedStatus;
            break;
        case 8:
            poisonData = *(u16 **)&((BtlUnit *)unit)->datUnit;
            chance = func_00244f60(8, poisonData, poisonData, 0xFFFF, 8);
            requiredTurns = 6;
            hasTurnThreshold = countedStatus;
            break;
        default:
            chance = 0;
            requiredTurns = 0;
            hasTurnThreshold = 0;
            break;
        }
        if ((hasTurnThreshold == 1) && (requiredTurns != 0)) {
            if (datCalcHasSkill(*(s32 *)&unitState->datUnit, 0x22F) != 0) {
                requiredTurns = requiredTurns >> 1;
                if (requiredTurns <= 0) {
                    requiredTurns = 1;
                }
            }
            if (datCalcHasSkill(*(s32 *)&((BtlUnit *)unit)->datUnit, 0x230) != 0) {
                requiredTurns = 1;
            }
        }
        if (countedStatus == 1) {
            if (MEMBER(u8, action + statusIndex, BtlAction, targetBlock.recovery.statusTurns[0]) <= 1) {
                goto loop_31_increment;
            }
        }
        if ((MEMBER(u8, action + statusIndex, BtlAction, targetBlock.recovery.statusTurns[0]) >= requiredTurns) && (requiredTurns != 0)) {
            cureMask |= statusBit;
        } else {
            /* An eligible ailment's roll: its chance passes the wear-off stage. */
            if (requiredTurns != 0 && p4_game_wearoff_native_active()) {
                DatUnit *datUnit = *(DatUnit **)&((BtlUnit *)unit)->datUnit;
                chance = p4_game_wearoff_native_rate((datUnit->flags & 4) != 0, datUnit->id, (u32)statusBit,
                    (u32)datCalcGetLevel((u8 *)datUnit), datCalcGetUnitStat(datUnit, 4),
                    MEMBER(u8, action + statusIndex, BtlAction, targetBlock.recovery.statusTurns[0]),
                    (u32)requiredTurns, chance, p4_game_afflictions_native_count(datUnit));
            }
            if ((s32)datCalcRandomBelow(0x64) < chance) {
                cureMask |= statusBit;
            }
        }
    }
loop_31_increment:
    statusIndex += 1;
loop_31_test:
    if (statusIndex < 0x18U) {
        goto loop_31_body;
    }
    return cureMask;
}

#pragma pop
