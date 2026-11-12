#include "p4pc/vu0.h"

void mdlEffectSetEulerRotation(u8 *arg0, f32 arg1, f32 arg2, f32 arg3)
{
    u8 scratch[16];

    effMiscEulerToQuatVU(iGpffff836c * arg1, iGpffff836c * arg2, iGpffff836c * arg3);
    p4_vu0_sqc2(scratch, 10);
    mdlEffectSetChildRotations(arg0, scratch);
}
