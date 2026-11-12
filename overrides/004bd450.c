#include "p4pc/vu0.h"

void effMiscCombineRotationVU(void)
{
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 24, 28, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 25, 28, P4_VU_BC_Y);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 26, 28, P4_VU_BC_Z);
    p4_vu0_madd(28, P4_VU_XYZW, 27, 28, P4_VU_BC_W);
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 24, 29, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 25, 29, P4_VU_BC_Y);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 26, 29, P4_VU_BC_Z);
    p4_vu0_madd(29, P4_VU_XYZW, 27, 29, P4_VU_BC_W);
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 24, 30, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 25, 30, P4_VU_BC_Y);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 26, 30, P4_VU_BC_Z);
    p4_vu0_madd(30, P4_VU_XYZW, 27, 30, P4_VU_BC_W);
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 24, 31, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 25, 31, P4_VU_BC_Y);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 26, 31, P4_VU_BC_Z);
    p4_vu0_madd(31, P4_VU_XYZW, 27, 31, P4_VU_BC_W);
}
