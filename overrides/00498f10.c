#include "p4pc/vu0.h"

void RingRebuildCircle(u8 *arg0)
{
    u_long128 pair;
    u_long128 slot80;
    u_long128 slot70;
    u_long128 slot60;
    f32 var_f5;
    f32 temp_f0;
    f32 temp_f4;
    f32 temp_f3;
    f32 var_f1;
    f32 temp_f22;
    f32 var_f21;
    f32 temp_f20;
    f32 sum0;
    f32 sum1;
    f32 sum2;
    s32 temp_3;
    s32 temp_4;
    s16 temp_2;
    s32 var_16;
    u32 var_18;
    u8 *temp_16;
    u8 *temp_17;
    u8 *temp_18;
    u8 *var_19;

    temp_16 = *(u8 **)(arg0 + 0x3C);
    temp_18 = *(u8 **)(arg0 + 0x40);
    temp_17 = *(u8 **)temp_16;
    temp_4 = *(s32 *)(arg0 + 0x34);
    temp_3 = *(s32 *)(temp_18 + 0x34);
    if ((temp_3 >= temp_4) || (temp_3 == 0)) {
        if (*(u8 *)(temp_18 + 0x68) != 0) {
            var_f5 = (f32)(temp_3 - temp_4);
        } else {
            var_f5 = (f32)temp_4;
        }
        temp_f3 = fabsf(*(f32 *)(temp_18 + 0x60));
        temp_f4 = *(f32 *)(temp_18 + 0x64);
        if (temp_f4 < 0.0f) {
            temp_f0 = 0.5f * (-temp_f3 / (0.5f * temp_f4));
            if (!(var_f5 <= temp_f0)) {
                var_f5 = temp_f0;
            }
        }
        sum2 = temp_f4 * var_f5;
        temp_f0 = 0.5f * sum2 + temp_f3;
        temp_f22 = var_f5 * temp_f0;
        temp_f22 += *(f32 *)(temp_18 + 0x40);
        temp_2 = *(s16 *)(temp_17 + 8);
        var_16 = temp_2 / 4;
        RpGeometryLock(*(RpGeometry **)(*(u8 **)(temp_17 + 0x10) + 0x18), 2);
        var_19 = *(u8 **)(*(u8 **)(*(u8 **)(*(u8 **)(temp_17 + 0x10) + 0x18) + 0x5C) + 0x14);
        var_f21 = 0.0f;
        var_f1 = (f32)*(u32 *)(temp_18 + 0x38);
        temp_f20 = fGpffff80d0 / var_f1;
        ((s32 *)&pair)[1] = 0;
        ((s32 *)&pair)[3] = 0x3F800000;
        sum0 = *(f32 *)(temp_18 + 0x50) + temp_f22;
        sum1 = sum0 + *(f32 *)(temp_18 + 0x54);
        sum2 = sum1 + *(f32 *)(temp_18 + 0x58);
        /* Seed: vf10 = D_00713CE0 * sum0; vf11 = D_00713CE0; stash to slot80. */
        p4_vu0_lqc2(10, D_00713CE0);
        p4_vu0_move(11, P4_VU_XYZW, 10);
        p4_vu0_set_xf(2, sum0);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_sqc2(&slot80, 10);
        /* vf10 = vf11 * sum1; vf11 = vf11 * sum2; stash to slot70 and slot60. */
        p4_vu0_move(10, P4_VU_XYZW, 11);
        p4_vu0_set_xf(2, sum1);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_set_xf(2, sum2);
        p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
        p4_vu0_sqc2(&slot70, 10);
        p4_vu0_sqc2(&slot60, 11);
        var_18 = 0;
        while (var_18 < (u32)var_16) {
            ((f32 *)&pair)[0] = cosf(var_f21);
            ((f32 *)&pair)[2] = sinf(var_f21);
            p4_vu0_lqc2(10, &pair);
            p4_vu0_move(11, P4_VU_XYZW, 10);
            p4_vu0_set_xf(2, temp_f22);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(var_19 + 0) = D_00713D10[0];
            *(f32 *)(var_19 + 4) = D_00713D14[0];
            *(f32 *)(var_19 + 8) = D_00713D18[0];
            p4_vu0_lqc2(10, &slot80);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(var_19 + 0xC) = D_00713D10[0];
            *(f32 *)(var_19 + 0x10) = D_00713D14[0];
            *(f32 *)(var_19 + 0x14) = D_00713D18[0];
            p4_vu0_lqc2(10, &slot70);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(var_19 + 0x18) = D_00713D10[0];
            *(f32 *)(var_19 + 0x1C) = D_00713D14[0];
            *(f32 *)(var_19 + 0x20) = D_00713D18[0];
            p4_vu0_lqc2(10, &slot60);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(var_19 + 0x24) = D_00713D10[0];
            *(f32 *)(var_19 + 0x28) = D_00713D14[0];
            *(f32 *)(var_19 + 0x2C) = D_00713D18[0];
            var_f21 += temp_f20;
            var_18 += 1;
            var_19 += 0x30;
        }
        temp_16 = *(u8 **)(*(u8 **)(temp_17 + 0x10) + 0x18);
        func_003c22f0(temp_16);
        if (*(u16 *)temp_17 & 4) {
            *(u16 *)(temp_16 + 0xC) = *(u16 *)(temp_16 + 0xC) | 1;
        }
    }
}
