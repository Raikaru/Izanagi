#include "p4pc/vu0.h"

/* pexew moves word 2 (lane z) into the low word, then mtc1 reads it as a float. */
static inline f32 effectVuGetZ10(void)
{
    P4Mmi128 g;
    f32 value;

    p4_vu0_qmfc2(&g, 10);
    __builtin_memcpy(&value, &g.w[2], 4);
    return value;
}
