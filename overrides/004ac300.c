#include "p4pc/vu0.h"

void EffModelUpdate(s32 arg0, u8 *arg1)
{
    s32 spBC;
    s32 spB8;
    f32 spA0[3];
    u8 sp60[0x40];
    f32 temp_f0;
    f32 temp_f20;
    s32 temp_3;
    u32 temp_17;
    u32 temp_16;
    u8 *temp_3_2;
    u8 *temp_4;
    f32 scale;

    temp_17 = *(u32 *)(arg1 + 0x98);
    temp_16 = *(u32 *)(arg1 + 0x8C);
    if ((temp_16 >= temp_17) || (temp_16 == 0)) {
        if (temp_16 == 0x4C9) {
            mdlSetAlphaBlend(*(u8 **)(arg1 + 0x94));
        }
        temp_3 = mdlEffectSamplePackedColor(arg1 + 0x2C, arg1 + 0x50, temp_17, temp_16);
        spB8 = *(s32 *)(arg1 + 0x28);
        scale = D_00761134;
        spBC = (s32)p4_vu0_packed_color_scale2((uint32_t)spB8, (uint32_t)temp_3, scale);
        mdlSetColor(*(u8 **)(arg1 + 0x94), &spBC);
        temp_f20 = mdlEffectSampleScalarKeys(arg1 + 0x60, temp_17, temp_16) / 10.0f;
        mdlEffectBuildBasisRows(sp60, arg1 + 0x10);
        mdlApplyModelMatrix(*(u8 **)(arg1 + 0x94), sp60, 0);
        temp_f0 = *(f32 *)(arg1 + 0x20) * temp_f20;
        spA0[2] = temp_f0;
        spA0[1] = temp_f0;
        spA0[0] = temp_f0;
        mdlScale(*(u8 **)(arg1 + 0x94), spA0, 2);
        spA0[0] = *(f32 *)(arg1 + 0);
        spA0[1] = *(f32 *)(arg1 + 4);
        spA0[2] = *(f32 *)(arg1 + 8);
        mdlTranslateMatrix((RwMatrix *)*(u8 **)(arg1 + 0x94), (const RwV3d *)spA0, 2);
        if (*(u8 *)(arg1 + 0x90) != 0) {
            temp_3_2 = *(u8 **)(arg1 + 0x94);
            *(s32 *)(temp_3_2 + 0xD8) = *(s32 *)(temp_3_2 + 0xD8) | 0x18;
        } else {
            temp_4 = *(u8 **)(arg1 + 0x94);
            *(s32 *)(temp_4 + 0xD8) = *(s32 *)(temp_4 + 0xD8) & ~0x18;
        }
        mdlSetLayerWeight(*(u8 **)(arg1 + 0x94), 0, *(f32 *)(arg1 + 0x24));
        mdlSetAttachedOwner(*(u8 **)(arg1 + 0x94), TnGetTableBase());
        mdlQueueModelForDraw(arg0, *(u8 **)(arg1 + 0x94));
        *(s32 *)(arg1 + 0x24) = 0;
    }
}
