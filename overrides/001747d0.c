#include "p4pc/vu0.h"
#include "p4pc/mod_field_host.h"

/* FldEvent_TriggerUpdate, SLUS-217.82 001747d0..00174a9c, plus the port's
 * field host slot (include/p4pc/mod_field_host.h). In mode 0 the eleven
 * retail {test, run} pairs of D_005F17D0 are polled first, unchanged; only
 * when none fired and no encounter is pending (eventPending: only pair 0 is
 * polled then) may p4_mod_field_host_enter take slot P4_FIELD_HOST_SLOT. The
 * slot then enters mode 1 exactly as a retail trigger does (idle animation,
 * camera flag 2, icon off) and its run is p4_mod_field_host_run until that
 * returns 0, so retail interactions always win and the field is held like an
 * NPC talk. With no host request the enter test returns 0 and this is the
 * retail function. */
s32 FldEvent_TriggerUpdate(u8 *task)
{
    u8 *h;
    s32 t;
    s32 i;
    s32 v;

    h = ((SdkTask *)task)->work;
    MEMBER(s32, h, FldEventWork, unknown14) = 0;
    t = MEMBER(s32, h, FldEventWork, interactionCooldown);
    if (t > 0) {
        ((FldEventWork *)h)->interactionCooldown = t - 1;
    }
    FldUnit_UpdateFieldState();
    if (MEMBER(s32, h, FldEventWork, resourcesReady) == 0) {
        ((FldEventWork *)h)->resourcesReady = MT_Scene_AllResFlagSet();
    }
    switch (((FldEventWork *)h)->mode) {
    case 0:
        for (i = 0; i < ((((FldEventWork *)h)->eventPending != 0) ? 1 : 0xB); i++) {
            if (((s32 (*)(u8 *))D_005F17D0[i * 2])(task) == 1) {
                ((FldEventWork *)h)->unknown0c = i;
                break;
            }
        }
        if (((FldEventWork *)h)->unknown0c < 0 && ((FldEventWork *)h)->eventPending == 0 &&
            p4_mod_field_host_enter((u32)task) != 0) {
            ((FldEventWork *)h)->unknown0c = P4_FIELD_HOST_SLOT;
        }
        if (((FldEventWork *)h)->unknown0c > -1) {
            v = (s16)mdlGetLayerAnimation(D_007EFA00[0], 0);
            if (v == FldMoveAnimPrimary(1) || v == FldMoveAnimSecondary(1)) {
                mdlDispatchAnimIfPresent((u8*)D_007EFA00[0], 0, (s16)FldCharacterAnimSelect(1), 4, 1);
            }
            if (((FldFrame *)MT_Scene_GetFrameState())->cameraTask != 0) {
                FldFrame_SetStateFlags((s32)((FldFrame *)MT_Scene_GetFrameState())->cameraTask, 2);
            }
            ((FldEventWork *)h)->motionState = 0;
            ((FldEventWork *)h)->unknown14 = 0;
            ((FldEventWork *)h)->mode = 1;
        } else {
            FldEvent_StepMoveToward(task);
            break;
        }
    case 1:
        if (((FldEventWork *)h)->unknown0c == P4_FIELD_HOST_SLOT) {
            v = p4_mod_field_host_run((u32)task);
        } else {
            v = ((s32 (*)(u8 *))D_005F17D4[MEMBER(s32, h, FldEventWork, unknown0c) * 2])(task);
        }
        if (v != 1) {
            if (((FldFrame *)MT_Scene_GetFrameState())->cameraTask != 0) {
                FldFrame_ClearStateFlags((s32)((FldFrame *)MT_Scene_GetFrameState())->cameraTask, 2);
            }
            ((FldEventWork *)h)->unknown0c = -1;
            ((FldEventWork *)h)->mode = 0;
        }
        break;
    case 2:
        break;
    }
    if (((FldEventWork *)h)->unknown14 == 1) {
        (u8 *)mdlGetMatrix((void *)(D_007EFA00[0]));
        if (D_007EFB64[0] != 0) {
            FldPlayerIconUpdatePos((u8 *)D_007EFB64[0]);
            FldPlayerIconSetActive(D_007EFB64[0], 1);
        }
    } else if (D_007EFB64[0] != 0) {
        FldPlayerIconSetActive(D_007EFB64[0], 0);
    }
    return 0;
}
