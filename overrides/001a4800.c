/* Native 001a4800: observe the actual ready-to-execute transition, not UI
 * button presses, target hits, or repeated readiness polls. */
#include "p4pc/guest.h"
#include "p4pc/battle_events.h"
#include "p4pc/battle_events_native.h"

void btlActionReadyUpdate(u8 *actionBytes)
{
    extern void btlTargetSavePair(u8 *target);
    extern s32 btlActionSkillReachesTargets(u8 *action);
    extern void btlActionRollGroupRatio(s32 context);
    extern s32 btlSoundCanReceiveStatus(u8 *action);
    extern void btlUnitApplyPacketFlags(u8 *packet);
    s32 loopIndex;
    s32 delayFlag;
    s32 actionIndex;
    s32 personaId;
    u16 actionKind;
    u8 *otherAction;
    BtlBattleState *battleState;
    if (((s32)(btlCountUnitRecordsById(0x506)) == (s32)(0)) && ((s32)(btlCountUnitRecordsById(0xC05)) == (s32)(0))) {
        actionIndex = 0;
        while ((s32)(loopIndex = actionIndex & 0xFFFF) < (s32)(((BtlAction *)actionBytes)->unk_6a)) {
            otherAction = (u8 *)((((BtlAction *)actionBytes)->targets[actionIndex & 0xFFFF]));
            if ((s32)(actionBytes) == (s32)(otherAction)) {
                goto incr;
            }
            if (!((((BtlAction *)otherAction)->unk_1a) & 1)) {
                goto incr;
            }
            if (((s32)(datCalcIsDead(((s32)((BtlUnit *)((u8 *)(((s32)((BtlAction *)otherAction)->unit))))->datUnit), 0)) != (s32)(0))) {
                goto incr;
            }
            if ((((BtlAction *)otherAction)->currState) != 1) {
                goto donecheck;
            }
incr:
            actionIndex = (actionIndex + 1) & 0xFFFF;
        }
donecheck:
        if (loopIndex == (((BtlAction *)actionBytes)->unk_6a)) {
            if (!((((BtlAction *)actionBytes)->flags) & 4)) {
                btlTargetSavePair(actionBytes + 0x38);
            }
            (((BtlAction *)actionBytes)->flags) = (u16)((u16)((((BtlAction *)actionBytes)->flags) & 0xFFFD));
            delayFlag = 0;
            actionKind = (u16)((u16)((((BtlAction *)actionBytes)->unk_6c)));
            switch (actionKind) {
            case 1:
            case 2:
            case 3:
                delayFlag = 1;
                /* fallthrough */
            case 9:
                btlTargetResolveSelectedSkill(actionBytes);
                if ((((DatSkillEntry *)operandOrderAdd(((BtlAction *)actionBytes)->unk_6e * 0x28, (s32)iGpffffb3b8))->kind24) == 5) {
                    btlActionSetState((BtlAction *)actionBytes, 0x20U);
                } else if ((((BtlAction *)actionBytes)->targetBlock.message.blocked) == 1) {
                    btlActionSetState((BtlAction *)actionBytes, 0x17U);
                } else if ((s32)(btlActionSkillReachesTargets(actionBytes)) != (s32)(0)) {
                    (((BtlAction *)actionBytes)->flags) = (u16)((u16)((((BtlAction *)actionBytes)->flags) & 0xFFEF));
                    btlActionSetState((BtlAction *)actionBytes, 0x10U);
                } else {
                    (((BtlAction *)actionBytes)->flags) = (u16)((u16)((((BtlAction *)actionBytes)->flags) | 0x10));
                    btlActionSetState((BtlAction *)actionBytes, 0x11U);
                }
                break;
            case 7:
            case 8:
            case 11:
                btlActionSetState((BtlAction *)actionBytes, 0x19U);
                break;
            case 6:
                if ((s32)((((BtlUnit *)((u8 *)((BtlAction *)actionBytes)->unit))->genus)) == (s32)(0)) {
                    if ((s32)(datGetFlag(0x38)) != (s32)(0)) {
                        personaId = 5;
                    } else {
                        personaId = 8;
                    }
                    if (((s32)(datPersonaFindSkillSlot((u8 *)datPersonaGetRecord(personaId & 0xFFFF), 0x114)) != (s32)(-1)) || ((battleState = (BtlBattleState *)iGpffffb3ac)->openingSide == 1 && (battleState->queueFlags & 2))) {
                        btlPacketEnqueue(((u8 *)btlTargetCreateTargetPacket((u32)(actionBytes), (8), (0), (0), (3))), 1);
                        btlPacketEnqueue((u8 *)btlCameraCreateSetStatePacket((BtlAction *)actionBytes, 8), 0);
                        btlActionSetStateWithDelay((BtlAction *)(actionBytes), (u16)(0x1D), (u16)(0xC));
                    } else {
                        btlActionRollGroupRatio(0);
                        btlPacketEnqueue(((u8 *)btlTargetCreateTargetPacket((u32)(actionBytes), (3), (0), (0), (2))), 1);
                        btlPacketEnqueue(btlTargetCreateActionPacket(actionBytes, 0U), 1);
                        btlPacketEnqueue((u8 *)btlCameraCreateSetStatePacket((BtlAction *)actionBytes, 1), 0);
                        if ((s32)(btlSoundCanReceiveStatus(actionBytes)) != (s32)(0)) {
                            btlActionSetStateWithDelay((BtlAction *)(actionBytes), (u16)(0x1B), (u16)(0xC));
                        } else {
                            btlActionSetStateWithDelay((BtlAction *)(actionBytes), (u16)(0x20), (u16)(0xC));
                        }
                    }
                } else {
                    btlActionSetState((BtlAction *)actionBytes, 0x1DU);
                }
                break;
            case 12:
                btlActionSetState((BtlAction *)actionBytes, 0x1DU);
                break;
            case 5:
                btlActionSetState((BtlAction *)actionBytes, 0x1AU);
                break;
            case 10:
                btlActionSetState((BtlAction *)actionBytes, 7U);
                break;
            }
            if ((delayFlag != 0) && ((s32)(btlActionAnyTargetDownOrHitFlagged(actionBytes)) != (s32)(0))) {
                (((BtlAction *)actionBytes)->flags) = (u16)((u16)((((BtlAction *)actionBytes)->flags) | 8));
            } else {
                (((BtlAction *)actionBytes)->flags) = (u16)((u16)((((BtlAction *)actionBytes)->flags) & 0xFFF7));
            }
            btlUnitApplyPacketFlags(actionBytes);
            if ((p4_battle_observer_interest & P4_BATTLE_OBSERVE_COMMAND) &&
                actionKind >= 1 && actionKind <= 12 && actionKind != 4)
                p4_battle_emit_command(P4_H2G(actionBytes), actionKind);
        }
    }
}
