#include "p4pc/vu0.h"

static inline void effectVuStore11(EffectVuVector *value)
{
    p4_vu0_sqc2(value, 11);
}
