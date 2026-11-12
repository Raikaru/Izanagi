#include "p4pc/vu0.h"

void btlFormationVuTransform(f32 fparg0, f32 fparg1, f32 *arg0, s32 *arg1, u8 *arg2, u8 *arg3)
{
    struct Vec4 {
        f32 value[4];
    };
    f32 fStack_10[4];
    f32 uStack_20[4];
    f32 vectorWork[4];
    f32 origin[4];
    f32 var_f20;
    f32 neg_limit;
    uint32_t neg_bits;
    s32 var_16;
    u8 temp_6;
    u8 temp_7;

    temp_6 = *(u8 *)(arg2 + 1);
    temp_7 = *(u8 *)(arg2 + 2);
    if (temp_6 == 4) {
        var_16 = temp_7 & 0xFF;
        if (var_16 == 8) {
            var_f20 = (*(u16 *)(arg2 + 4) == 0) ? 2.0f : fparg0;
        } else {
            var_f20 = 2.0f;
        }
    } else {
        var_f20 = *(f32 *)((u8 *)arg0 + 4) + fparg1 * D_006094B0[temp_6];
        var_16 = temp_7 & 0xFF;
        if ((var_16 == 8) && (*(u16 *)(arg2 + 4) != 0)) {
            var_f20 += fparg0;
        }
    }
    switch (var_16) {
    case 8:
        *(f32 *)((u8 *)arg3 + 0) = *(f32 *)((u8 *)arg0 + 0);
        *(f32 *)((u8 *)arg3 + 4) = var_f20;
        *(f32 *)((u8 *)arg3 + 8) = *(f32 *)((u8 *)arg0 + 8);
        break;
    case 9:
    case 10:
        mdlEffectCopyCameraAim((u8 *)origin);
        p4_vu0_lqc2(10, origin);
        if (fparg0 < fparg1) {
            fparg1 = fparg1;
        } else {
            fparg1 = fparg0;
        }
        neg_limit = -fparg1;
        if (var_16 == 9) {
            origin[0] = *(f32 *)((u8 *)arg0 + 0);
            origin[1] = *(f32 *)((u8 *)arg0 + 4);
            origin[2] = *(f32 *)((u8 *)arg0 + 8);
        } else {
            origin[0] = *(f32 *)((u8 *)arg0 + 0);
            origin[1] = var_f20;
            origin[2] = *(f32 *)((u8 *)arg0 + 8);
        }
        p4_vu0_lqc2(11, origin);
        p4_vu0_sub(11, P4_VU_XYZW, 11, 10, P4_VU_NOBC);
        if (var_16 == 10) {
            p4_vu0_sqc2(vectorWork, 11);
            vectorWork[1] = 0.0f;
            p4_vu0_lqc2(11, vectorWork);
        }
        /* vmul.xyz / vmulax.w / vmadday.w / vmaddz.w / vrsqrt / vwaitq / vmulq.xyz */
        p4_vu0_mul(2, P4_VU_XYZ, 11, 11, P4_VU_NOBC);
        p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
        p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
        p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
        p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
        p4_vu0_mulq(11, P4_VU_XYZ, 11);
        /* mfc1 + qmtc2 of neg_limit into vf2; only lane x is read by the next op. */
        __builtin_memcpy(&neg_bits, &neg_limit, 4);
        p4_vu0_set_x(2, neg_bits);
        p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
        p4_vu0_lqc2(10, origin);
        p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        p4_vu0_sqc2(arg3, 10);
        break;
    default:
        *(struct Vec4 *)uStack_20 = *(struct Vec4 *)arg1;
        func_003dc740(uStack_20, D_0060A0E0, D_006094D0[temp_7], 2);
        RtQuatTransformVectors(fStack_10, (f32 *)D_0060A0F0, 1, (u8 *)uStack_20);
        fStack_10[0] = fStack_10[0] * fparg0;
        fStack_10[1] = fStack_10[1] * fparg0;
        fStack_10[2] = fStack_10[2] * fparg0;
        *(f32 *)((u8 *)arg3 + 0) = *(f32 *)((u8 *)arg0 + 0) + fStack_10[0];
        *(f32 *)((u8 *)arg3 + 4) = var_f20 + fStack_10[1];
        *(f32 *)((u8 *)arg3 + 8) = *(f32 *)((u8 *)arg0 + 8) + fStack_10[2];
        break;
    }
    *(s32 *)((u8 *)arg3 + 0xC) = 0;
}
