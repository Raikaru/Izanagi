#include "p4pc/vu0.h"

/* Affine 4x3 product: rows 0-2 use the xyz parts of B's rows, row 3 adds B's
 * row 3 with weight 1 (vf0.w). The w lanes of the stored rows are whatever the
 * VU0 registers held, as on hardware; the flags word at +0xC is the AND of both
 * inputs' flags words. */
u8 *RwMatrixMultiply(u8 *arg0, u8 *arg1, u8 *arg2)
{
    u32 flags;

    p4_vu0_lqc2(1, arg1 + 0x00);
    p4_vu0_lqc2(2, arg1 + 0x10);
    p4_vu0_lqc2(3, arg1 + 0x20);
    p4_vu0_lqc2(4, arg1 + 0x30);
    flags = *(u32 *)(arg1 + 0xC);
    p4_vu0_lqc2(5, arg2 + 0x00);
    p4_vu0_lqc2(6, arg2 + 0x10);
    p4_vu0_lqc2(7, arg2 + 0x20);
    p4_vu0_lqc2(8, arg2 + 0x30);
    flags &= *(u32 *)(arg2 + 0xC);

    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZ, 5, 1, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZ, 6, 1, P4_VU_BC_Y);
    p4_vu0_madd(9, P4_VU_XYZ, 7, 1, P4_VU_BC_Z);

    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZ, 5, 2, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZ, 6, 2, P4_VU_BC_Y);
    p4_vu0_madd(10, P4_VU_XYZ, 7, 2, P4_VU_BC_Z);

    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZ, 5, 3, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZ, 6, 3, P4_VU_BC_Y);
    p4_vu0_madd(11, P4_VU_XYZ, 7, 3, P4_VU_BC_Z);

    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZ, 5, 4, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZ, 6, 4, P4_VU_BC_Y);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZ, 7, 4, P4_VU_BC_Z);
    p4_vu0_madd(12, P4_VU_XYZ, 8, 0, P4_VU_BC_W);

    p4_vu0_sqc2(arg0 + 0x00, 9);
    p4_vu0_sqc2(arg0 + 0x10, 10);
    p4_vu0_sqc2(arg0 + 0x20, 11);
    p4_vu0_sqc2(arg0 + 0x30, 12);
    *(u32 *)(arg0 + 0xC) = flags;
    return arg0;
}
