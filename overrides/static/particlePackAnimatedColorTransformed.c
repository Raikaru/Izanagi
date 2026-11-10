#include "p4pc/vu0.h"

/* The transformed pass keeps the color-byte scale in an FPR; its bits go to VF2.x. */
static inline u32 particlePackAnimatedColorTransformed(f32 scale)
{
    uint32_t scaleBits;
    P4Mmi128 zero;
    P4Mmi128 r;

    __builtin_memcpy(&scaleBits, &scale, sizeof scaleBits);
    __builtin_memset(&zero, 0, sizeof zero);
    p4_vu0_set_x(2, scaleBits);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
    p4_vu0_ftoi(10, P4_VU_XYZW, 10);
    p4_vu0_qmfc2(&r, 10);
    r = p4_mmi_ppach(zero, r);
    r = p4_mmi_ppacb(zero, r);
    return r.w[0];
}
