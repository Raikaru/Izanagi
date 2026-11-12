/* Observe the real battle constructor without changing its task lifecycle. */
#include "p4pc/battle_events.h"
#include "p4pc/battle_events_native.h"

s32 btlTaskCreate(s32 setupAddress)
{
    s32 battleTask;
    u8 *renderStateBattle;
    u8 *skyBattle;
    u8 *battle;
    u8 *formationBattle;
    u8 *variantBattle;
    u8 *activeBattle;

    if (p4_battle_observer_interest & P4_BATTLE_OBSERVE_REQUEST)
        p4_battle_emit_request(P4_BATTLE_TASK_BEGIN, 0, (u32)setupAddress, 0);
    btlInit();
    ((BtlBattleState *)iGpffffb3ac)->cutinTaskParent =
        (s32)kwlnTaskCreate((const void *)(&iGpffffa088), 0x10, setupAddress == 0, 0, btlUpdateTask, btlReleaseTask, (u8 *)(0));
    (s32)kwlnTaskCreateChild((void *)(((BtlBattleState *)iGpffffb3ac)->cutinTaskParent), (const void *)(D_005F65B0), 0x11, setupAddress == 0, 0, btlUpdateDraw3DTask, 0, (u8 *)(NULL));
    (s32)kwlnTaskCreateChild((void *)(((BtlBattleState *)iGpffffb3ac)->cutinTaskParent), (const void *)(D_005F65C0), 0x11, setupAddress == 0, 0, btlUpdateDraw2DTask, 0, (u8 *)(NULL));
    ((BtlBattleState *)iGpffffb3ac)->renderStateNode.callback = (void *)btlRenderStateSetup;
    renderStateBattle = iGpffffb3ac;
    ((BtlBattleState *)renderStateBattle)->renderStateNode.work = renderStateBattle;
    ((BtlBattleState *)iGpffffb3ac)->skyNode.callback = (void *)btlSkyScroll;
    skyBattle = iGpffffb3ac;
    ((BtlBattleState *)skyBattle)->skyNode.work = skyBattle;
    ((BtlBattleState *)iGpffffb3ac)->uiTask =
        btlUiCreateTask(((BtlBattleState *)iGpffffb3ac)->cutinTaskParent);
    battle = iGpffffb3ac;
    battleTask = ((BtlBattleState *)battle)->cutinTaskParent;
    if (setupAddress != 0) {
        ((BtlBattleState *)battle)->isFieldEncounter = 0;
        memcpy(&((BtlBattleState *)iGpffffb3ac)->encounter, (void *)setupAddress, 0x3C);
        formationBattle = iGpffffb3ac;
        ((BtlBattleState *)formationBattle)->formation = ((BtlBattleState *)formationBattle)->encounter.formation;
        variantBattle = iGpffffb3ac;
        ((BtlBattleState *)variantBattle)->formationVariant = ((BtlBattleState *)variantBattle)->encounter.formationVariant;
        btlMainSetPhase(2);
    } else {
        ((BtlBattleState *)battle)->isFieldEncounter = 1;
        if (datGetFlag(0x1436) == 0) {
            btlMainSetPhase(0x12);
        } else {
            btlMainSetPhase(0x13);
        }
    }
    activeBattle = iGpffffb3ac;
    ((BtlBattleState *)activeBattle)->flags |= 1;
    if (p4_battle_observer_interest & P4_BATTLE_OBSERVE_REQUEST)
        p4_battle_emit_request(P4_BATTLE_TASK_CREATED, 0, (u32)setupAddress, (u32)battleTask);
    return battleTask;
}
