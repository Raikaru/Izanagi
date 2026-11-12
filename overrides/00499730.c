#include "p4pc/vu0.h"

void RingRebuildStrip(u8 *arg0)
{
    u8 *temp_3;
    u8 *temp_5;
    u8 **temp_17;
    u8 *temp_19;
    u8 *temp_16;
    u8 *temp_18a;
    s32 temp_21;
    s32 temp_20;
    f32 step;
    u32 limit;
    u8 *temp_4;
    s32 stmp;
    u32 counter;
    u8 *var_7;
    u8 *var_6;
    s32 bias;
    u8 *addr;
    f32 fscale;
    f32 t2;
    f32 t1;
    f32 t0;
    u8 *temp_18b;
    u8 *temp_17b;
    u8 *temp_17c;

    temp_3 = *(u8 **)(arg0 + 0x3C);
    temp_5 = *(u8 **)(temp_3 + 4);
    temp_17 = *(u8 ***)(temp_5 + 0x3C);
    temp_19 = *(u8 **)(arg0 + 0x40);
    temp_16 = *(u8 **)(temp_3 + 8);
    temp_18a = *(u8 **)temp_3;
    temp_21 = *(s32 *)(arg0 + 0x34);
    temp_20 = *(s32 *)(temp_19 + 0x34);
    if ((temp_20 >= temp_21) || (temp_20 == 0)) {
        RingInstanceAdvance(temp_5);
        step = (f32)temp_21 / (f32)temp_20;
        RpGeometryLock(*(RpGeometry **)(*(u8 **)(temp_16 + 0x10) + 0x18), 2);
        RpGeometryLock(*(RpGeometry **)(*(u8 **)(*temp_17 + 0x10) + 0x18), 2);
        limit = *(u32 *)(temp_19 + 0x38);
        temp_4 = *temp_17;
        stmp = *(s16 *)(temp_4 + 8) - 1;
        var_7 = *(u8 **)(*(u8 **)(*(u8 **)(*(u8 **)(temp_16 + 0x10) + 0x18) + 0x5C) + 0x14);
        var_6 = *(u8 **)(*(u8 **)(*(u8 **)(*(u8 **)(temp_4 + 0x10) + 0x18) + 0x5C) + 0x14);
        if (*(u8 *)(temp_19 + 0xC4) == 0) {
            counter = 0;
            bias = -0x48;
            while (counter < limit) {
                D_00713D10[0] = *(f32 *)(var_6 + 0xC);
                D_00713D14[0] = *(f32 *)(var_6 + 0x10);
                D_00713D18[0] = *(f32 *)(var_6 + 0x14);
                p4_vu0_lqc2(10, D_00713D10);
                p4_vu0_move(12, P4_VU_XYZW, 10);
                D_00713D10[0] = *(f32 *)(var_6 + 0x24);
                D_00713D14[0] = *(f32 *)(var_6 + 0x28);
                D_00713D18[0] = *(f32 *)(var_6 + 0x2C);
                p4_vu0_lqc2(11, D_00713D10);
                p4_vu0_sqc2(D_00713D10, 10);
                *(f32 *)(var_7 + 0x18) = D_00713D10[0];
                *(f32 *)(var_7 + 0x1C) = D_00713D14[0];
                *(f32 *)(var_7 + 0x20) = D_00713D18[0];
                /* Normalise vf10.xyz: vf2.w = |vf10.xyz|^2 via ACC; vf10 *= rsqrt(vf2.w). */
                p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
                p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
                p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
                p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
                p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
                p4_vu0_mulq(10, P4_VU_XYZ, 10);
                fscale = *(f32 *)temp_18a * step;
                p4_vu0_set_xf(2, fscale);
                p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
                p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
                p4_vu0_sqc2(D_00713D10, 10);
                *(f32 *)(var_7 + 0) = D_00713D10[0];
                *(f32 *)(var_7 + 4) = D_00713D14[0];
                *(f32 *)(var_7 + 8) = D_00713D18[0];
                t2 = *(f32 *)(var_6 + 0x3C);
                t1 = *(f32 *)(var_6 + 0x40);
                t0 = *(f32 *)(var_6 + 0x44);
                *(f32 *)(var_7 + 0xC) = t2;
                *(f32 *)(var_7 + 0x10) = t1;
                *(f32 *)(var_7 + 0x14) = t0;
                if (counter == 0) {
                    addr = var_6 + stmp * 12 + bias;
                    t2 = *(f32 *)(addr + 0);
                    t1 = *(f32 *)(addr + 4);
                    t0 = *(f32 *)(addr + 8);
                    *(f32 *)(var_7 + 0x24) = t2;
                    *(f32 *)(var_7 + 0x28) = t1;
                    *(f32 *)(var_7 + 0x2C) = t0;
                } else {
                    t2 = *(f32 *)(var_6 - 0x24);
                    t1 = *(f32 *)(var_6 - 0x20);
                    t0 = *(f32 *)(var_6 - 0x1C);
                    *(f32 *)(var_7 + 0x24) = t2;
                    *(f32 *)(var_7 + 0x28) = t1;
                    *(f32 *)(var_7 + 0x2C) = t0;
                }
                counter += 1;
                var_7 += 0x30;
                var_6 += 0x30;
                temp_18a += 4;
            }
        }
        temp_18b = *temp_17;
        temp_17b = *(u8 **)(*(u8 **)(temp_18b + 0x10) + 0x18);
        func_003c22f0(temp_17b);
        if (*(u16 *)temp_18b & 4) {
            *(u16 *)(temp_17b + 0xC) = *(u16 *)(temp_17b + 0xC) | 1;
        }
        temp_17c = *(u8 **)(*(u8 **)(temp_16 + 0x10) + 0x18);
        func_003c22f0(temp_17c);
        if (*(u16 *)temp_16 & 4) {
            *(u16 *)(temp_17c + 0xC) = *(u16 *)(temp_17c + 0xC) | 1;
        }
    }
}
