#include "p4pc/vu0.h"

f32 mdlEffectProjectAnchorDepth(void)
{
    RwV3d transformed, input;
    f32 depth;
    f32 output[4] __attribute__((aligned(16)));

    p4_vu0_sqc2(D_00713D10, 10);
    input.x = D_00713D10[0];
    input.y = D_00713D14[0];
    input.z = D_00713D18[0];
    func_003e42a0(&transformed, &input,
                  (const RwMatrix *)mdlEffectCameraMatrix((u8 *)(u32)kwlnGetCamera()));
    depth = transformed.z;
    output[0] = 640.0f * (transformed.x / depth);
    output[1] = 448.0f * (transformed.y / depth);
    output[2] = 0.0f;
    output[3] = 0.0f;
    p4_vu0_lqc2(10, output);
    return depth;
}
