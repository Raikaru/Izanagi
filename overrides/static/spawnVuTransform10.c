#include "p4pc/vu0.h"

/* VF28..30 contain the basis established by effMiscBuildRotationVU. The W lane is
 * transformed too; normalization below intentionally changes XYZ only. */
static inline void spawnVuTransform10(void)
{
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);  /* vmulax ACC, vf28, vf10.x */
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y); /* vmadday ACC, vf29, vf10.y */
    p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);        /* vmaddz vf10, vf30, vf10.z */
}
