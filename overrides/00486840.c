#include "p4pc/vu0.h"

u_long128 mdlEffectResolveChildOffset(u8 *arg0, u8 *arg1, u_long128 *arg2)
{
    /* Retail's lqc2 of values[] also reads the word after values[2]. That
     * stack slot is not a defined value, so this override zeroes it. */
    u32 values[4];
    u8 work[0x10];
    s32 flags;
    u_long128 value;
    u32 w44;

    __builtin_memset(values, 0, sizeof values);

    flags = *(s32 *)(arg0 + 0x68);
    if ((flags & 0x18) == 0) {
        value = *(u_long128 *)(arg0 + 0x40);
        __builtin_memcpy(arg2, &value, sizeof value);
        return value;
    }

    p4_vu0_lqc2(10, arg0 + 0x40);
    if (flags & 0x10) {
        p4_vu0_set_xf(2, 0.0f);
        p4_vu0_add(10, P4_VU_Y, 0, 2, P4_VU_BC_X);
    }
    p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
    p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Y);
    p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Z);
    p4_vu0_sqrt(2, P4_VU_BC_X);
    values[0] = (u32)p4_vu0_cfc2(22);
    values[1] = values[0];
    values[2] = values[0];

    mdlEffectAimDirection(arg1, work);

    p4_vu0_lqc2(10, work);
    p4_vu0_sub(10, P4_VU_XYZ, 0, 10, P4_VU_NOBC);
    flags = *(s32 *)(arg0 + 0x68);
    if (flags & 0x10) {
        p4_vu0_set_xf(2, 0.0f);
        p4_vu0_add(10, P4_VU_Y, 0, 2, P4_VU_BC_X);
    }
    p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
    p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
    p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
    p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
    p4_vu0_mulq(10, P4_VU_XYZ, 10);

    p4_vu0_lqc2(11, values);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
    if (flags & 0x10) {
        __builtin_memcpy(&w44, arg0 + 0x44, sizeof w44);
        p4_vu0_set_x(2, w44);
        p4_vu0_add(10, P4_VU_Y, 0, 2, P4_VU_BC_X);
    }

    p4_vu0_sqc2(arg2, 10);
    __builtin_memcpy(&value, arg2, sizeof value);
    return value;
}
