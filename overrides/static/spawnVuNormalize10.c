#include "p4pc/vu0.h"

/* Normalizes VF10.xyz in place. VF2.w receives the squared length; Q holds its reciprocal
 * square root. vwaitq is implicit because p4_vu0_rsqrt writes Q synchronously. */
static inline void spawnVuNormalize10(void)
{
    p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);            /* vmul.xyz vf2, vf10, vf10 */
    p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);        /* vmulax.w ACC, vf0, vf2.x */
    p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);       /* vmadday.w ACC, vf0, vf2.y */
    p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);               /* vmaddz.w vf2, vf0, vf2.z */
    p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);              /* vrsqrt Q, vf0.w, vf2.w */
    p4_vu0_mulq(10, P4_VU_XYZ, 10);                          /* vmulq.xyz vf10, vf10, Q */
}
