#include "p4pc/vu0.h"

void RingUpdateLineSegment(u8 *arg0)
{
    u8 *temp_16;
    u8 *temp_18;
    u8 *temp_17;
    u8 colourStack[16];
    s32 temp_3;
    u8 *dest;
    f32 scale;
    u_long128 *dstq;

    temp_16 = *(u8 **)(arg0 + 0x3C);
    temp_18 = *(u8 **)(arg0 + 0x40);
    temp_17 = *(u8 **)(temp_16 + 8);
    if ((*(u32 *)(temp_18 + 0x34) >= *(u32 *)(arg0 + 0x34)) ||
        (*(u32 *)(temp_18 + 0x34) == 0)) {
        temp_3 = mdlEffectSamplePackedColor(temp_18, temp_18 + 0x24,
                               *(u32 *)(arg0 + 0x34),
                               *(u32 *)(temp_18 + 0x34));
        *(u32 *)&colourStack[8] = *(u32 *)(arg0 + 0x30);
        scale = fGpffff8044;
        *(u32 *)&colourStack[0] = p4_vu0_packed_color_scale2(*(u32 *)&colourStack[8], (u32)temp_3, scale);
        *(u32 *)&colourStack[12] = *(u32 *)&colourStack[0];
        if (colourStack[15] != 0xFF) {
            dest = *(u8 **)(temp_17 + 0x14);
            *(LineNovaColor *)(dest + 4) =
                *(LineNovaColor *)&colourStack[12];
        } else {
            colourStack[15] = 0xFE;
            dest = *(u8 **)(temp_17 + 0x14);
            *(LineNovaColor *)(dest + 4) =
                *(LineNovaColor *)&colourStack[12];
            colourStack[15] = 0xFF;
        }
        if (*(u8 *)(*(u8 **)(temp_17 + 0x14) + 7) != 0) {
            *(s32 *)(*(u8 **)(temp_16 + 4) + 0x30) =
                *(s32 *)(arg0 + 0x30);
            dstq = (u_long128 *)(*(u8 **)(temp_16 + 4) + 0x20);
            *dstq = *(u_long128 *)(arg0 + 0x20);
            dstq = (u_long128 *)(*(u8 **)(temp_16 + 4) + 0x0);
            *dstq = *(u_long128 *)(arg0 + 0x0);
            dest = *(u8 **)(temp_16 + 4);
            dstq = (u_long128 *)(dest + 0x10);
            *dstq = *(u_long128 *)(arg0 + 0x10);
            RingInstanceUpdateIfActive(*(void **)(temp_16 + 4));
            GeometryBlockSetFrameMatrixCombined(temp_17, arg0, arg0 + 0x10, arg0 + 0x20);
            if (*(u8 *)(temp_18 + 0x3C) != 0) {
                *(u16 *)temp_17 = *(u16 *)temp_17 | 1;
            } else {
                *(u16 *)temp_17 = *(u16 *)temp_17 & 0xFFFE;
            }
            GeometryBlockBindResource((s32)temp_17, *(u16 *)(temp_18 + 0x28));
        }
    }
}
