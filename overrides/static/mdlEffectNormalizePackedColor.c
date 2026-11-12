#include "p4pc/vu0.h"

/* VU0 boundary: VF10 receives the four colour bytes of *source as floats,
 * each multiplied by scale. The lanes are the byte values, not 0..1 floats,
 * until scale is applied. */
static inline void mdlEffectNormalizePackedColor(const u32 *source, f32 scale)
{
    P4Mmi128 lanes;
    u32 scaleBits;

    lanes = p4_vu0_word_bytes_to_lanes(*source);
    __builtin_memcpy(&scaleBits, &scale, sizeof scaleBits);
    p4_vu0_qmtc2(10, &lanes);
    p4_vu0_itof(10, P4_VU_XYZW, 10);
    p4_vu0_set_x(2, scaleBits);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
}
