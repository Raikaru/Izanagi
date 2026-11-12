#include "p4pc/vu0.h"

/* Same semantics as effectVuUnpackColor10; retail's $2 transfer register is a codegen detail. */
static inline void effectVuUnpackColor10V0(const u32 *word, f32 scale)
{
    P4Mmi128 lanes = p4_vu0_word_bytes_to_lanes(*word);
    uint32_t bits;

    p4_vu0_qmtc2(10, &lanes);
    p4_vu0_itof(10, P4_VU_XYZW, 10);
    __builtin_memcpy(&bits, &scale, 4);
    p4_vu0_set_x(2, bits);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
}
