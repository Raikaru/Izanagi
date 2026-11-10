#include "p4pc/vu0.h"

void effBlurFilterStepSlotsB(u8 *arg0)
{
    f32 vec[4];
    u8 *obj;
    u8 *base;
    s32 b;
    s32 a;
    u8 *list;
    s32 tmp;
    s32 packed;
    f32 scale;
    f32 f;
    f32 prod;

    list = *(u8 **)(arg0 + 0x20);
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
            *(s32 *)(base + 0x1C) = 0x43A00000;
            *(s32 *)(base + 0x20) = 0x43600000;
            *(f32 *)(base + 0x24) = f;
        } else {
            prod = f * *(f32 *)(arg0 + 0x14);
            p4_vu0_lqc2(10, arg0);
            *(f32 *)(base + 0x24) = mdlEffectAnchorScreenScale(prod);
            p4_vu0_sqc2(vec, 10);
            *(f32 *)(base + 0x1C) = vec[0];
            *(f32 *)(base + 0x20) = vec[1];
        }
        tmp = mdlEffectSamplePackedColor(obj, obj + 0x24, b, a);
        packed = *(s32 *)(arg0 + 0x10);
        scale = fGpffff8044;
        *(s32 *)(base + 0xC) = (s32)p4_vu0_packed_color_scale2((uint32_t)packed, (uint32_t)tmp, scale);
        *(f32 *)(base + 0x18) = fGpffff80f0 * mdlEffectSampleScalarKeys(obj + 0x34, b, a);
        *(f32 *)(base + 0x14) = fGpffff80f0 * mdlEffectSampleScalarKeys(obj + 0x60, b, a);
        *(s32 *)(base + 0x10) = *(s32 *)(obj + 0x28);
        effBlurFilterTickSlotsB(base, list);
    } else {
        *(s32 *)(base + 0xC) = 0;
    }
}
