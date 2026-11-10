#include "p4pc/vu0.h"

f32 mdlEffectAnchorScreenScale(f32 fparg0)
{
    u8 raw[0x50];
    f32 temp_f22;
    f32 temp_f21;
    f32 temp_f20;
    f32 temp_f0;
    f32 temp_f1;
    u8 *ptr;

    temp_f20 = fparg0;
    p4_vu0_sqc2(raw + 0x10, 10);
    p4_vu0_sqc2(D_00713D10, 10);
    *(f32 *)(raw + 0x30) = D_00713D10[0];
    *(f32 *)(raw + 0x34) = D_00713D14[0];
    *(f32 *)(raw + 0x38) = D_00713D18[0];
    func_003e42a0((RwV3d *)(raw + 0x40), (const RwV3d *)(raw + 0x30),
                  (const RwMatrix *)mdlEffectCameraMatrix((u8 *)(u32)kwlnGetCamera()));
    temp_f22 = *(f32 *)(raw + 0x48);
    temp_f1 = *(f32 *)(raw + 0x40) / temp_f22;
    temp_f0 = 640.0f * temp_f1;
    *(f32 *)(raw + 0) = temp_f0;
    temp_f1 = *(f32 *)(raw + 0x44) / temp_f22;
    temp_f0 = 448.0f * temp_f1;
    *(f32 *)(raw + 4) = temp_f0;
    *(s32 *)(raw + 8) = 0;
    *(s32 *)(raw + 0xc) = 0;
    p4_vu0_lqc2(10, raw);
    p4_vu0_sqc2(raw + 0x20, 10);
    ptr = (u8 *)(u32)kwlnGetCamera();
    temp_f21 = *(f32 *)(ptr + 0x80);
    ptr = (u8 *)(u32)kwlnGetCamera();
    temp_f0 = *(f32 *)(ptr + 0x84);
    if ((temp_f22 <= temp_f21) || !(temp_f22 < temp_f0)) {
        return 0.0f;
    }
    ptr = (u8 *)(u32)kwlnGetCamera();
    ptr = *(u8 **)(ptr + 4);
    p4_vu0_lqc2(10, raw + 0x10);
    *(f32 *)&D_00713D10[0] = *(f32 *)(ptr + 0x40);
    *(f32 *)&D_00713D14[0] = *(f32 *)(ptr + 0x44);
    *(f32 *)&D_00713D18[0] = *(f32 *)(ptr + 0x48);
    p4_vu0_lqc2(11, D_00713D10);
    p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
    p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
    p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Y);
    p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Z);
    p4_vu0_sqrt(2, P4_VU_BC_X);
    {
        union {
            int32_t i;
            f32 f;
        } cvt;

        cvt.i = p4_vu0_cfc2(22);
        temp_f1 = cvt.f;
    }
    p4_vu0_lqc2(10, raw + 0x20);
    temp_f0 = (650.0f * temp_f20) / temp_f1;
    temp_f1 = 1500.0f;
    if (!(temp_f0 <= temp_f1)) {
        temp_f0 = temp_f1;
    }
    return temp_f0;
}
