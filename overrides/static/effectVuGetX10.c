#include "p4pc/vu0.h"

/* qmfc2 moves the 128-bit register; the low word (lane x) is the returned bit pattern. */
static inline f32 effectVuGetX10(void)
{
    P4Mmi128 g;
    f32 value;

    p4_vu0_qmfc2(&g, 10);
    __builtin_memcpy(&value, &g.w[0], 4);
    return value;
}
