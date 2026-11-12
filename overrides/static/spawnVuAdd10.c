#include "p4pc/vu0.h"

static inline void spawnVuAdd10(void)
{
    p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC); /* vadd.xyzw vf10, vf10, vf11 */
}
