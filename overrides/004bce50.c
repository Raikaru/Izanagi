#include "p4pc/vu0.h"

void effMiscQuatMultiplyVU(void)
{
    p4_vu0_mul(2, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
    p4_vu0_opmula(10, 11);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZ, 11, 10, P4_VU_BC_W);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZ, 10, 11, P4_VU_BC_W);
    p4_vu0_opmsub(10, 11, 10);
    p4_vu0_mul(P4_VU_ACC, P4_VU_W, 10, 11, P4_VU_NOBC);
    p4_vu0_msub(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
    p4_vu0_msub(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
    p4_vu0_msub(10, P4_VU_W, 0, 2, P4_VU_BC_Z);
}
