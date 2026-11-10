#include "p4pc/vu0.h"

/* retail 0x0044b610 is cosf; the unit declares it without parameters, but retail passes the angle in $f12 (the asm leaves it from the incoming argument) */
extern f32 p4c_cosf_0048a340(f32 x) __asm__("func_0044b610");

void mdlEffectAxisAngleBasisVU(f32 param_1)
{
    f32 temp_f20;
    f32 temp_f21;
    f32 temp_f0;
    f32 one;
    f32 x;
    f32 y;
    f32 z;
    f32 square;
    f32 blend;
    f32 factor;
    f32 xy;
    f32 xz;
    f32 zterm;
    f32 yterm;
    f32 xyterm;
    f32 xzterm;
    f32 yzterm;
    f32 xterm;
    u8 raw[0x50];

    temp_f21 = param_1;
    temp_f20 = p4c_cosf_0048a340(temp_f21);
    temp_f0 = sinf(temp_f21);
    p4_vu0_sqc2(raw, 10);

    x = *(f32 *)(raw + 0x0);
    square = x * x;
    one = 1.0f;
    blend = one - square;
    blend = blend * temp_f20 + square;
    *(f32 *)(raw + 0x10) = blend;
    y = *(f32 *)(raw + 0x4);
    xy = x * y;
    factor = one - temp_f20;
    z = *(f32 *)(raw + 0x8);
    zterm = z * temp_f0;
    xyterm = xy * factor;
    blend = zterm + xyterm;
    *(f32 *)(raw + 0x14) = blend;
    xz = x * z;
    yterm = y * temp_f0;
    xzterm = xz * factor;
    blend = xzterm - yterm;
    *(f32 *)(raw + 0x18) = blend;
    *(s32 *)(raw + 0x1c) = 0;
    blend = xyterm - zterm;
    *(f32 *)(raw + 0x20) = blend;
    square = y * y;
    blend = one - square;
    blend = blend * temp_f20 + square;
    *(f32 *)(raw + 0x24) = blend;
    yzterm = y * z;
    xterm = x * temp_f0;
    yzterm = yzterm * factor;
    blend = xterm + yzterm;
    *(f32 *)(raw + 0x28) = blend;
    *(s32 *)(raw + 0x2c) = 0;
    blend = yterm + xzterm;
    *(f32 *)(raw + 0x30) = blend;
    blend = yzterm - xterm;
    *(f32 *)(raw + 0x34) = blend;
    square = z * z;
    blend = one - square;
    blend = blend * temp_f20 + square;
    *(f32 *)(raw + 0x38) = blend;
    *(s32 *)(raw + 0x3c) = 0;
    p4_vu0_lqc2(28, raw + 0x10);
    p4_vu0_lqc2(29, raw + 0x20);
    p4_vu0_lqc2(30, raw + 0x30);
}
