#include "p4pc/vu0.h"

static inline void spawnVuMultiply10(void)
{
    p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC); /* vmul.xyzw vf10, vf10, vf11 */
}
