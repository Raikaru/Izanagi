#include "p4pc/vu0.h"

static inline void effectVuSetZ10(f32 value)
{
    uint32_t bits;

    __builtin_memcpy(&bits, &value, 4);
    p4_vu0_set_x(2, bits);
    p4_vu0_add(10, P4_VU_Z, 0, 2, P4_VU_BC_X);
}
