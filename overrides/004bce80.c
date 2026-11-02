#include "p4pc/vu0.h"

void effMiscNormalizeVU(void)
{
    p4_vu0_mul(2, P4_VU_XYZW, 10, 10, P4_VU_NOBC);
    p4_vu0_add(P4_VU_ACC, P4_VU_W, 2, 2, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
    p4_vu0_madd(3, P4_VU_W, 0, 2, P4_VU_BC_Z);
    p4_vu0_rsqrt(0, P4_VU_BC_W, 3, P4_VU_BC_W);
    p4_vu0_mulq(10, P4_VU_XYZW, 10);
}
