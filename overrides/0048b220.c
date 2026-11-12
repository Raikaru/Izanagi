#include "p4pc/vu0.h"

void mdlEffectSampleKeyState(u8 *arg0, u8 *arg1, s32 arg2, u_long128 *arg3)
{
    s32 mdlEffectSamplePackedColor(u8 *arg0, u8 *arg1, s32 arg2, s32 arg3);
    f32 mdlEffectSampleScalarKeys(u8 *arg0, s32 arg1, s32 arg2);
    f32 mdlEffectProjectAnchorDepth(void);
    u_long128 sp50;
    s32 size;

    size = *(s32 *)(arg1 + 0xB8);
    *(s32 *)(arg0 + 0x14) =
        mdlEffectSamplePackedColor(arg1 + 0x2C, arg1 + 0x50, arg2, size);
    size = *(s32 *)(arg1 + 0xB8);
    *(f32 *)(arg0 + 0x18) =
        mdlEffectSampleScalarKeys(arg1 + 0x60, arg2, size);
    if (*(u8 *)(arg1 + 0x9C) != 2) {
        size = *(s32 *)(arg1 + 0xB8);
        *(f32 *)(arg0 + 0x1C) =
            mdlEffectSampleScalarKeys(arg1 + 0x8C, arg2, size);
        return;
    }
    /* The projection helper reads and writes VF10 through the shared VU0 model,
     * so the two projected positions are passed in VF10 as the asm does. */
    p4_vu0_lqc2(10, arg0);
    mdlEffectProjectAnchorDepth();
    p4_vu0_move(11, P4_VU_XYZW, 10);
    p4_vu0_lqc2(10, arg3);
    mdlEffectProjectAnchorDepth();
    p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
    p4_vu0_sqc2(&sp50, 10);
    if ((*(f32 *)((u8 *)&sp50 + 0) != 0.0f) ||
        (*(f32 *)((u8 *)&sp50 + 4) != 0.0f)) {
        *(f32 *)(arg0 + 0x1C) =
            func_0044b950(*(f32 *)((u8 *)&sp50 + 4),
                          *(f32 *)((u8 *)&sp50 + 0));
        return;
    }
    *(f32 *)(arg0 + 0x1C) = 0.0f;
}
