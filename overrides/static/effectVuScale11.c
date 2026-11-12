#include "p4pc/vu0.h"

static inline void effectVuScale11(f32 scale)
{
    uint32_t bits;

    __builtin_memcpy(&bits, &scale, 4);
    p4_vu0_set_x(2, bits);
    p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
}
