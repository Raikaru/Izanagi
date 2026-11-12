#include "p4pc/vu0.h"

u_long128 mdlEffectResolveChildOrientation(u8 *arg0, u8 *arg1, u_long128 *arg2)
{
    u32 flags;
    u_long128 sp40;
    u_long128 value;
    f32 var_f20;

    if (((flags = *(s32 *)(arg0 + 0x68)) & 0x60) == 0) {
        value = *(u_long128 *)(arg0 + 0x50);
        __builtin_memcpy(arg2, &value, sizeof value);
        return value;
    }
    mdlEffectAimDirection(arg1, (u8 *)&sp40);
    if (*(s32 *)(arg0 + 0x68) & 0x40) {
        var_f20 = 0.0f;
    } else {
        var_f20 = -func_0044b920(-*(f32 *)((u8 *)&sp40 + 4));
    }
    effMiscEulerToQuatVU(var_f20,
                         func_0044b950(*(f32 *)((u8 *)&sp40 + 0),
                                       *(f32 *)((u8 *)&sp40 + 8)),
                         0.0f);
    if (*(s32 *)(arg0 + 0x68) & 0x40) {
        p4_vu0_move(11, P4_VU_XYZW, 10);
        p4_vu0_lqc2(10, arg0 + 0x50);
        effMiscQuatMultiplyVU();
    }
    p4_vu0_sqc2(arg2, 10);
    __builtin_memcpy(&value, arg2, sizeof value);
    return value;
}
