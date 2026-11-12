#include "p4pc/vu0.h"

/* VU0 boundary: VF10 contains the normalized animated particle color.
 * Packing produces the real four-byte scalar consumed by the color stream. */
static inline u32 particlePackAnimatedColor(void)
{
    P4Mmi128 zero;
    P4Mmi128 r;

    __builtin_memset(&zero, 0, sizeof zero);
    p4_vu0_set_x(2, 0x437F0000U); /* 255.0f */
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
    p4_vu0_ftoi(10, P4_VU_XYZW, 10);
    p4_vu0_qmfc2(&r, 10);
    r = p4_mmi_ppach(zero, r);
    r = p4_mmi_ppacb(zero, r);
    return r.w[0];
}
