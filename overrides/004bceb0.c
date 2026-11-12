#include "p4pc/vu0.h"

void effMiscBuildRotationVU(void)
{
    p4_vu0_add(1, P4_VU_XYZ, 0, 0, P4_VU_BC_W);
    p4_vu0_add(2, P4_VU_XYZW, 10, 10, P4_VU_NOBC);
    p4_vu0_mul(28, P4_VU_W, 0, 0, P4_VU_BC_X);
    p4_vu0_mul(29, P4_VU_W, 0, 0, P4_VU_BC_X);
    p4_vu0_mul(30, P4_VU_W, 0, 0, P4_VU_BC_X);
    p4_vu0_mul(3, P4_VU_XYZW, 10, 2, P4_VU_NOBC);
    p4_vu0_mul(4, P4_VU_XYZW, 10, 2, P4_VU_BC_Y);
    p4_vu0_mul(5, P4_VU_XYZW, 10, 2, P4_VU_BC_Z);
    p4_vu0_mul(6, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
    p4_vu0_add(P4_VU_ACC, P4_VU_XYZ, 0, 0, P4_VU_BC_W);
    p4_vu0_msub(P4_VU_ACC, P4_VU_X, 1, 3, P4_VU_BC_Y);
    p4_vu0_msub(28, P4_VU_X, 1, 3, P4_VU_BC_Z);
    p4_vu0_msub(P4_VU_ACC, P4_VU_Y, 1, 3, P4_VU_BC_X);
    p4_vu0_msub(29, P4_VU_Y, 1, 3, P4_VU_BC_Z);
    p4_vu0_msub(P4_VU_ACC, P4_VU_Z, 1, 3, P4_VU_BC_X);
    p4_vu0_msub(30, P4_VU_Z, 1, 3, P4_VU_BC_Y);
    p4_vu0_mul(P4_VU_ACC, P4_VU_Y, 1, 4, P4_VU_BC_X);
    p4_vu0_msub(28, P4_VU_Y, 1, 5, P4_VU_BC_W);
    p4_vu0_add(29, P4_VU_X, 4, 5, P4_VU_BC_W);
    p4_vu0_sub(30, P4_VU_X, 5, 4, P4_VU_BC_W);
    p4_vu0_mul(P4_VU_ACC, P4_VU_Z, 1, 5, P4_VU_BC_X);
    p4_vu0_madd(28, P4_VU_Z, 1, 4, P4_VU_BC_W);
    p4_vu0_mul(P4_VU_ACC, P4_VU_Z, 1, 5, P4_VU_BC_Y);
    p4_vu0_msub(29, P4_VU_Z, 1, 6, P4_VU_BC_W);
    p4_vu0_add(30, P4_VU_Y, 5, 6, P4_VU_BC_W);
    p4_vu0_move(31, P4_VU_XYZW, 0);
}
