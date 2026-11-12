#include "p4pc/vu0.h"

void mdlEffectInitDirectionEntry(u8 *arg0, u8 *arg1)
{
    f32 sp40[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    f32 squares[4];
    f32 temp_f0;
    s32 temp_4;

    if (*(u8 *)(arg0 + 0x10) == 0) {
        sp40[0] = *(f32 *)(arg0 + 0x14);
        sp40[1] = *(f32 *)(arg0 + 0x18);
        sp40[2] = *(f32 *)(arg0 + 0x1C);
    } else {
        sp40[0] = 2.0f * (effMiscRandFloat(0) - 0.5f);
        sp40[1] = 2.0f * (effMiscRandFloat(0) - 0.5f);
        sp40[2] = 2.0f * (effMiscRandFloat(0) - 0.5f);
    }
    p4_vu0_lqc2(10, sp40);
    p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
    p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Y);
    p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Z);
    p4_vu0_sqrt(2, P4_VU_BC_X);
    {
        union {
            int32_t i;
            f32 f;
        } cvt;

        cvt.i = p4_vu0_cfc2(22);
        temp_f0 = cvt.f;
    }
    if (temp_f0 == 0.0f) {
        goto normalized_zero;
    }
    p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
    /* vmulax/vmadday/vmaddz accumulate lane w as (x*x + y*y) + z*z. */
    squares[0] = 0.0f;
    squares[1] = 0.0f;
    squares[2] = 0.0f;
    squares[3] = (p4pc_vu0.vf[2].f[0] + p4pc_vu0.vf[2].f[1]) + p4pc_vu0.vf[2].f[2];
    p4_vu0_store(2, P4_VU_W, squares);
    p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
    p4_vu0_mulq(10, P4_VU_XYZ, 10);
    p4_vu0_sqc2(sp40, 10);
    *(f32 *)(arg1 + 0) = sp40[0];
    *(f32 *)(arg1 + 4) = sp40[1];
    *(f32 *)(arg1 + 8) = sp40[2];
    goto normalized_done;
normalized_zero:
    *(f32 *)(arg1 + 0) = 0.0f;
    *(f32 *)(arg1 + 4) = 1.0f;
    *(f32 *)(arg1 + 8) = 0.0f;
normalized_done:
    temp_f0 = *(f32 *)(arg0 + 0x28);
    temp_f0 = (1.0f - temp_f0) + temp_f0 * effMiscRandFloat(0);
    *(f32 *)(arg1 + 0x10) = *(f32 *)(arg0 + 0x24) * temp_f0;
    *(f32 *)(arg1 + 0x0C) = iGpffff8080 * effMiscRandFloat(0);
    temp_4 = *(s32 *)(arg0 + 0x34);
    switch (temp_4) {
    case 1:
        *(u8 *)(arg1 + 0x14) = (u8)(effMiscRand(0) % 6);
        break;
    case 2:
        *(u8 *)(arg1 + 0x14) =
            (u8)(((u32)((u8 *)arg1 - *(u8 **)(arg0 + 0x5C)) / 0x18) % 6);
        break;
    default:
        *(s8 *)(arg1 + 0x14) = -1;
        break;
    }
}
