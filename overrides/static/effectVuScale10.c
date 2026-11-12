#include "p4pc/vu0.h"

static inline void effectVuScale10(f32 scale)
{
    uint32_t bits;

    __builtin_memcpy(&bits, &scale, 4);
    p4_vu0_set_x(2, bits);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
}
