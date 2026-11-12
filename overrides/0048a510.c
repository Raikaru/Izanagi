#include "p4pc/vu0.h"

s32 mdlEffectAnchorOnScreen(void)
{
    u8 raw[0x40];
    f32 temp_f0;
    f32 temp_f1;
    f32 temp_f20;
    f32 temp_f3;
    u8 *ptr;

    p4_vu0_sqc2(D_00713D10, 10);
    *(f32 *)(raw + 0x20) = D_00713D10[0];
    *(f32 *)(raw + 0x24) = D_00713D14[0];
    *(f32 *)(raw + 0x28) = D_00713D18[0];
    func_003e42a0((RwV3d *)(raw + 0x30), (const RwV3d *)(raw + 0x20),
                  (const RwMatrix *)mdlEffectCameraMatrix((u8 *)(u32)kwlnGetCamera()));
    ptr = (u8 *)(u32)kwlnGetCamera();
    temp_f20 = *(f32 *)(ptr + 0x80);
    ptr = (u8 *)(u32)kwlnGetCamera();
    temp_f0 = *(f32 *)(ptr + 0x84);
    temp_f1 = *(f32 *)(raw + 0x38);
    if ((temp_f1 <= temp_f20) || !(temp_f1 < temp_f0)) {
        return 0;
    }
    temp_f3 = 640.0f * (*(f32 *)(raw + 0x30) / temp_f1);
    *(f32 *)(raw + 0x18) = temp_f3;
    temp_f1 = 448.0f * (*(f32 *)(raw + 0x34) / temp_f1);
    *(f32 *)(raw + 0x1c) = temp_f1;
    if (!(temp_f3 < 0.0f) && !(temp_f1 < 0.0f) &&
        (temp_f3 <= 640.0f) && (temp_f1 <= 448.0f)) {
        *(f32 *)(raw + 0) = temp_f3;
        *(f32 *)(raw + 4) = temp_f1;
        *(f32 *)(raw + 8) = 0.0f;
        *(f32 *)(raw + 0xc) = 0.0f;
        p4_vu0_lqc2(10, raw);
        return 1;
    }
    return 0;
}
