#include "p4pc/vu0.h"

void RingRebuildCircleScaled(u8 *arg0)
{
    u8 *temp_18;
    u8 *temp_17;
    s32 temp_4;
    s32 temp_3;
    s16 tmp2;
    s32 var_16;
    u32 var_18;
    u8 *var_19;
    f32 var_f5;
    f32 temp_f0;
    f32 temp_f4;
    f32 temp_f3;
    f32 var_f1;
    f32 phase;
    f32 step;
    f32 zero;
    f32 fade;
    f32 f2;
    f32 f1;
    f32 f0;
    u_long128 stk;
    u_long128 sp80;
    u_long128 sp70;
    u_long128 sp60;
    u8 *tmp16;

    {
        u8 *tmp3p = *(u8 **)(arg0 + 0x3C);
        temp_18 = *(u8 **)(arg0 + 0x40);
        temp_17 = *(u8 **)tmp3p;
        temp_4 = *(s32 *)(arg0 + 0x34);
        temp_3 = *(s32 *)(temp_18 + 0x34);
    }
    if ((temp_3 >= temp_4) || (temp_3 == 0)) {
        if (*(u8 *)(temp_18 + 0x64) != 0) {
            var_f5 = (f32)(temp_3 - temp_4);
        } else {
            var_f5 = (f32)temp_4;
        }
        temp_f3 = fabsf(*(f32 *)(temp_18 + 0x5C));
        temp_f4 = *(f32 *)(temp_18 + 0x60);
        if (temp_f4 < 0.0f) {
            temp_f0 = 0.5f * (-temp_f3 / (0.5f * temp_f4));
            if (var_f5 > temp_f0) {
                var_f5 = temp_f0;
            }
        }
        f2 = temp_f4 * var_f5;
        f0 = temp_f3 + 0.5f * f2;
        f1 = var_f5 * f0;
        fade = f1 + *(f32 *)(temp_18 + 0x40);
        tmp2 = *(s16 *)(temp_17 + 8);
        var_16 = tmp2 / 4;
        RpGeometryLock(*(RpGeometry **)(*(u8 **)(temp_17 + 0x10) + 0x18), 2);
        var_19 = *(u8 **)(*(u8 **)(*(u8 **)(*(u8 **)(temp_17 + 0x10) + 0x18) + 0x5C) + 0x14);
        phase = 0.0f;
        var_f1 = (f32)*(u32 *)(temp_18 + 0x38);
        step = fGpffff80d0 / var_f1;
        zero = 0.0f;
        ((s32 *)&stk)[1] = 0;
        ((s32 *)&stk)[3] = 0x3F800000;
        f2 = *(f32 *)(temp_18 + 0x50) + fade;
        f1 = f2 + *(f32 *)(temp_18 + 0x54);
        f0 = f1 + *(f32 *)(temp_18 + 0x58);
        /* Seed: sp80 = D_00713CE0 * f2; sp70 = vf11 * f1; sp60 = vf11 * f0 (vf11 = D_00713CE0). */
        p4_vu0_lqc2(10, D_00713CE0);
        p4_vu0_move(11, P4_VU_XYZW, 10);
        p4_vu0_set_xf(2, f2);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_sqc2(&sp80, 10);
        p4_vu0_move(10, P4_VU_XYZW, 11);
        p4_vu0_set_xf(2, f1);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_set_xf(2, f0);
        p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
        p4_vu0_sqc2(&sp70, 10);
        p4_vu0_sqc2(&sp60, 11);
        var_18 = 0;
        while (var_18 < (u32)var_16) {
            ((f32 *)&stk)[0] = cosf(phase);
            ((f32 *)&stk)[2] = sinf(phase);
            *(f32 *)(var_19 + 0) = zero;
            *(f32 *)(var_19 + 4) = zero;
            *(f32 *)(var_19 + 8) = zero;
            p4_vu0_lqc2(11, &stk);
            p4_vu0_lqc2(10, &sp80);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(var_19 + 0xC) = D_00713D10[0];
            *(f32 *)(var_19 + 0x10) = D_00713D14[0];
            *(f32 *)(var_19 + 0x14) = D_00713D18[0];
            p4_vu0_lqc2(10, &sp70);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(var_19 + 0x18) = D_00713D10[0];
            *(f32 *)(var_19 + 0x1C) = D_00713D14[0];
            *(f32 *)(var_19 + 0x20) = D_00713D18[0];
            p4_vu0_lqc2(10, &sp60);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(var_19 + 0x24) = D_00713D10[0];
            *(f32 *)(var_19 + 0x28) = D_00713D14[0];
            *(f32 *)(var_19 + 0x2C) = D_00713D18[0];
            phase = phase + step;
            var_18 += 1;
            var_19 += 0x30;
        }
        tmp16 = *(u8 **)(*(u8 **)(temp_17 + 0x10) + 0x18);
        func_003c22f0(tmp16);
        if (*(u16 *)temp_17 & 4) {
            *(u16 *)(tmp16 + 0xC) = *(u16 *)(tmp16 + 0xC) | 1;
        }
    }
}
