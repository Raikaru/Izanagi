#include "p4pc/vu0.h"

void effBlurFilterUpdateRectFromScaleAux(u8 *arg0)
{
    f32 vec[4];
    u8 *base;
    u8 *obj;
    s32 b;
    s32 a;
    s32 tmp;
    s32 packed;
    f32 scale;
    f32 f;
    f32 g;
    f32 prod;

    obj = *(u8 **)(arg0 + 0x24);
    base = obj + 0xC0;
    a = *(s32 *)(obj + 0xB8);
    if (a == 0) {
        b = 0;
    } else {
        b = *(s32 *)(arg0 + 0x1C);
    }
    if (a >= b) {
        f = mdlEffectSampleScalarKeys(obj + 0x8C, b, a);
        if (*(u8 *)(obj + 0xBC) != 0) {
            *(f32 *)(base + 4) = 320.0f;
            *(f32 *)(base + 8) = 224.0f;
            *(f32 *)(base + 0x14) = 320.0f - f;
            *(f32 *)(base + 0x18) = 224.0f - f;
            *(f32 *)(base + 0x1C) = 320.0f + f;
            *(f32 *)(base + 0x20) = 224.0f + f;
        } else {
            prod = f * *(f32 *)(arg0 + 0x14);
            p4_vu0_lqc2(10, arg0);
            g = mdlEffectAnchorScreenScale(prod);
            p4_vu0_sqc2(vec, 10);
            *(f32 *)(base + 4) = vec[0];
            *(f32 *)(base + 8) = vec[1];
            *(f32 *)(base + 0x14) = vec[0] - g;
            *(f32 *)(base + 0x18) = vec[1] - g;
            *(f32 *)(base + 0x1C) = vec[0] + g;
            *(f32 *)(base + 0x20) = vec[1] + g;
        }
        tmp = mdlEffectSamplePackedColor(obj, obj + 0x24, b, a);
        packed = *(s32 *)(arg0 + 0x10);
        scale = fGpffff8044;
        *(s32 *)(base + 0xC) = (s32)p4_vu0_packed_color_scale2((uint32_t)packed, (uint32_t)tmp, scale);
        *(s32 *)(base + 0x10) = *(s32 *)(obj + 0x28);
    } else {
        *(s8 *)(base + 0xF) = 0;
    }
}
