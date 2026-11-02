#include "p4pc/vu0.h"

void mdlEffectBuildBasisRows(u8 *arg0, u8 *arg1)
{
    f32 work[4];
    u8 *base;

    p4_vu0_lqc2(10, arg1);
    effMiscNormalizeVU();
    effMiscBuildRotationVU();
    base = (u8 *)work;
    p4_vu0_sqc2(base, 28);
    *(f32 *)(arg0 + 0) = work[0];
    *(f32 *)(arg0 + 4) = work[1];
    *(f32 *)(arg0 + 8) = work[2];
    p4_vu0_sqc2(base, 29);
    *(f32 *)(arg0 + 0x10) = work[0];
    *(f32 *)(arg0 + 0x14) = work[1];
    *(f32 *)(arg0 + 0x18) = work[2];
    p4_vu0_sqc2(base, 30);
    *(f32 *)(arg0 + 0x20) = work[0];
    *(f32 *)(arg0 + 0x24) = work[1];
    *(f32 *)(arg0 + 0x28) = work[2];
    *(s32 *)(arg0 + 0x30) = 0;
    *(s32 *)(arg0 + 0x34) = 0;
    *(s32 *)(arg0 + 0x38) = 0;
}
