/* Native 001a1c80, preserving the reviewed lookAtUnit=NULL mirror fix.
 * Policy target keys are chosen only from this actual native filtered list. */
#include "p4pc/battle_sim.h"
#include "p4pc/guest.h"

void btlActionTargetInit(u8 *action)
{
    BtlAction *actionRecord = (BtlAction *)(action);

    u8 *lookAtUnit = NULL;
    s32 unitIndex;
    u8 entryType;
    u8 *packet;

    switch (((BtlAction *)action)->unk_6c) {
    case 4:
        btlEffectCollectTargetUnits(action, action + 0x98, 2, 0, 0);
        unitIndex = btlActionNormalizeUnitIndex(actionRecord->unk_6e);
        break;
    default:
        unitIndex = ((BtlAction *)action)->unk_6e;
        entryType = ((DatSkillEntry *)operandOrderAdd(unitIndex * 0x28,
                                               (u32)iGpffffb3b8))->unknown04[4];
        switch (entryType) {
        case 1:
        case 2:
            {
                btlEffectCollectTargetUnits(
                    action,
                    action + 0x98,
                    ((DatSkillEntry *)operandOrderAdd(unitIndex * 0x28,
                                                  (u32)iGpffffb3b8))->unknown04[5],
                    ((DatSkillEntry *)operandOrderAdd(unitIndex * 0x28,
                                                  (u32)iGpffffb3b8))->unknown04[6],
                    0);
            }
            break;
        default:
            btlSkillClassifyTargets(action, action + 0x98, unitIndex & 0xFFFF, 0);
            break;
        }
        break;
    }
init_1c80:
    btlEffectSelectNearestEntry(action, action + 0x98);
    entryType = ((DatSkillEntry *)operandOrderAdd(
        ((BtlAction *)action)->unk_6e * 0x28, (u32)iGpffffb3b8))->unknown04[4];
    switch (entryType) {
    case 1:
    case 2:
        goto type12_2_1c80;
    default:
        goto typeother_2_1c80;
    }
type12_2_1c80:
    lookAtUnit = NULL;
    goto after_type_1c80;
typeother_2_1c80:
    btlEffectSelectTargetByStatus(action, action + 0x98);
after_type_1c80:
    if (p4_battle_sim_active) p4_battle_policy_target(P4_H2G(action));
    btlSelectionMarkTargets(
        ((BtlBattleState *)D_0076449C)->uiTask,
        action,
        action + 0x98,
        (s64)(s16)unitIndex);
    packet = (u8 *)btlCameraCreateSetStatePacket((BtlAction *)action, 0x22);
    ((BtlPacket *)packet)->actionUID = *(s64 *)action;
    btlPacketEnqueue(packet, 0);
    btlDamageSetActive(((BtlBattleState *)D_0076449C)->uiTask);
    btlUnitRequestLookAtOthers(action);
    if (lookAtUnit != NULL) {
        packet = ((u8 *)btlUnitCreateLookAtUnitPacket((BtlUnit *)(NULL), (BtlUnit *)(MEMBER(u8 *, lookAtUnit, BtlAction, unit)), (u16)(1)));
        btlPacketEnqueue(packet, 1);
        packet = (u8 *)btlUnitCreateLookAtDeactivatePacket((BtlUnit *)(s32)((BtlAction *)lookAtUnit)->unit, 0);
        btlPacketEnqueue(packet, 1);
    }
}
