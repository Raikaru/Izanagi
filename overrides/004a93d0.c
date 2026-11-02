#include "p4pc/vu0.h"

void effBlurFilterUpdateScreenRect(u8 *arg0)
{
    u8 *base;
    u8 *obj;
    s32 a;
    s32 b;
    s32 tmp;
    s32 packed;
    f32 scale;

    obj = *(u8 **)(arg0 + 0x24);
    base = obj + 0xC0;
    a = *(s32 *)(obj + 0xB8);
    if (a == 0) {
        b = 0;
    } else {
        b = *(s32 *)(arg0 + 0x1C);
    }
    if (b == 0) {
        *(s32 *)(base + 0x10) = 0x43A00000;
        *(s32 *)(base + 0x14) = 0x43600000;
        *(s32 *)(base + 0x18) = 0;
        *(s32 *)(base + 0x1C) = 0;
        *(s32 *)(base + 0x20) = 0x44200000;
        *(s32 *)(base + 0x24) = 0x43E00000;
        *(s32 *)(base + 4) = *(s32 *)(obj + 0x28);
    }
    if (a >= b) {
        tmp = mdlEffectSamplePackedColor(obj, obj + 0x24, b, a);
        packed = *(s32 *)(arg0 + 0x10);
        scale = fGpffff8044;
        *(s32 *)base = (s32)p4_vu0_packed_color_scale2((uint32_t)packed, (uint32_t)tmp, scale);
        *(f32 *)(base + 0xC) = 1.0f + fGpffff80f0 * mdlEffectSampleScalarKeys(obj + 0x34, b, a);
        *(f32 *)(base + 8) = fGpffff80f0 * mdlEffectSampleScalarKeys(obj + 0x60, b, a);
        return;
    }
    *(s8 *)(base + 3) = 0;
}
