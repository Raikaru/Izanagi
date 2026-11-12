#include "p4pc/vu0.h"

void FlashBoxPackColors(u8 *arg0)
{
    union
    {
        s32 w;
        u8 b[4];
    } sp4C;
    s32 sp48;
    s32 temp_3;
    u8 *temp_2;
    u8 *temp_17;
    u8 *temp_16;
    u32 temp_6;
    u32 temp_7;
    f32 scale;

    temp_2 = *(u8 **)(arg0 + 0x3C);
    temp_17 = *(u8 **)(arg0 + 0x40);
    temp_16 = *(u8 **)(temp_2 + 4);
    temp_6 = *(u32 *)(arg0 + 0x34);
    temp_7 = *(u32 *)(temp_17 + 0x34);
    if ((temp_7 >= temp_6) || (temp_7 == 0))
    {
        temp_3 = mdlEffectSamplePackedColor(temp_17, temp_17 + 0x24, temp_6, temp_7);
        sp48 = *(s32 *)(arg0 + 0x30);
        scale = fGpffff8044;
        /* Retail: two VU colour passes (sp48 * scale kept in vf11, then temp_3 * scale * vf11 * 255,
         * packed); the helper runs that pipeline. */
        sp4C.w = (s32)p4_vu0_packed_color_scale2((uint32_t)sp48, (uint32_t)temp_3, scale);
        if (sp4C.b[3] != 0xFF)
        {
            u8 *dst = *(u8 **)(temp_16 + 0x14);
            *(Color4 *)(dst + 4) = *(Color4 *)&sp4C;
        }
        else
        {
            sp4C.b[3] = 0xFE;
            {
                u8 *dst = *(u8 **)(temp_16 + 0x14);
                *(Color4 *)(dst + 4) = *(Color4 *)&sp4C;
            }
            sp4C.b[3] = 0xFF;
        }
        GeometryBlockSetFrameMatrixCombined(temp_16, arg0, arg0 + 0x10, arg0 + 0x20);
        if (*(u8 *)(temp_17 + 0x56) != 0)
        {
            *(u16 *)temp_16 = *(u16 *)temp_16 | 1;
        }
        else
        {
            *(u16 *)temp_16 = *(u16 *)temp_16 & 0xFFFE;
        }
        {
            s32 temp_28 = *(u16 *)(temp_17 + 0x28);
            GeometryBlockBindResource(temp_16, temp_28);
        }
    }
}
