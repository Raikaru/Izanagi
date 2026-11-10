#include "p4pc/vu0.h"

void mdlEffectAimDirection(u8 *arg0, u8 *arg1)
{
    u8 *obj;
    f32 temp[4];
    f32 squares[4];

    obj = func_003e9700(*(s32 *)((u8 *)(u32)kwlnGetCamera() + 4));
    temp[0] = *(f32 *)(obj + 0x30);
    temp[1] = *(f32 *)(obj + 0x34);
    temp[2] = *(f32 *)(obj + 0x38);
    temp[3] = 0.0f;
    p4_vu0_lqc2(10, arg0);
    p4_vu0_lqc2(11, temp);
    p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
    p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
    /* vmulax/vmadday/vmaddz accumulate lane w as (x*x + y*y) + z*z. */
    squares[0] = 0.0f;
    squares[1] = 0.0f;
    squares[2] = 0.0f;
    squares[3] = (p4pc_vu0.vf[2].f[0] + p4pc_vu0.vf[2].f[1]) + p4pc_vu0.vf[2].f[2];
    p4_vu0_store(2, P4_VU_W, squares);
    p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
    p4_vu0_mulq(10, P4_VU_XYZ, 10);
    p4_vu0_sqc2(arg1, 10);
}
