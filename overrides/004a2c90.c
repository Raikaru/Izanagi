#include "p4pc/vu0.h"

void FunnelWindPackColors(u8 *arg0)
{
    union {
        s32 w;
        u8 b[4];
    } spAC;
    s32 spA8;
    s32 sp9C;
    s32 temp_3_2;
    u8 *temp_3;
    u32 *var_19;
    u8 *temp_18;
    u32 var_17;
    u8 *temp_16;
    u32 temp_6;
    u32 temp_7;
    u32 temp_23;
    u32 temp_21;
    u32 temp_6_2;
    u32 combined;
    f32 scale;

    temp_3 = *(u8 **)(arg0 + 0x3C);
    temp_18 = *(u8 **)(arg0 + 0x40);
    temp_16 = *(u8 **)(temp_3 + 4);
    temp_6 = *(u32 *)(arg0 + 0x34);
    temp_7 = *(u32 *)(temp_18 + 0x34);
    if ((temp_7 >= temp_6) || (temp_7 == 0)) {
        var_19 = *(u32 **)temp_3;
        temp_3_2 = mdlEffectSamplePackedColor(temp_18, temp_18 + 0x24, temp_6, temp_7);
        spA8 = *(s32 *)(arg0 + 0x30);
        scale = fGpffff8044;
        /* Retail: (spA8 colour * scale) is kept in vf11 and multiplied into the
         * (temp_3_2 colour * scale), then *255 and packed; the helper runs that pipeline. */
        combined = p4_vu0_packed_color_scale2((uint32_t)spA8, (uint32_t)temp_3_2, scale);
        temp_23 = *(u32 *)(temp_18 + 0x38);
        temp_21 = *(u32 *)(temp_18 + 0x80);
        var_17 = 0;
        while (var_17 < temp_23) {
            temp_6_2 = *var_19;
            if (temp_6_2 < temp_21) {
                sp9C = mdlEffectSamplePackedColor(temp_18 + 0x3C, temp_18 + 0x60, temp_6_2, temp_21);
                scale = fGpffff8044;
                spAC.w = (s32)p4_vu0_packed_color_scale2((uint32_t)sp9C, combined, scale);
                if (spAC.b[3] != 0xFF) {
                    u8 *dst = *(u8 **)(*(u8 **)(temp_16 + 0x54) + (var_17 & 0xFFFF) * 4);
                    *(PolygonWindColor *)(dst + 4) = *(PolygonWindColor *)&spAC;
                } else {
                    spAC.b[3] = 0xFE;
                    {
                        u8 *dst = *(u8 **)(*(u8 **)(temp_16 + 0x54) + (var_17 & 0xFFFF) * 4);
                        *(PolygonWindColor *)(dst + 4) = *(PolygonWindColor *)&spAC;
                    }
                    spAC.b[3] = 0xFF;
                }
            } else if (iGpffffbb64.c3 != 0xFF) {
                u8 *dst = *(u8 **)(*(u8 **)(temp_16 + 0x54) + (var_17 & 0xFFFF) * 4);
                *(PolygonWindColor *)(dst + 4) = iGpffffbb64;
            } else {
                iGpffffbb64.c3 = 0xFE;
                {
                    u8 *dst = *(u8 **)(*(u8 **)(temp_16 + 0x54) + (var_17 & 0xFFFF) * 4);
                    *(PolygonWindColor *)(dst + 4) = iGpffffbb64;
                }
                iGpffffbb64.c3 = 0xFF;
            }
            var_17 += 1;
            var_19 += 0xC;
        }
        mdlEffectApplyCombinedGeometryMatrix(temp_16, arg0, arg0 + 0x10, arg0 + 0x20);
        if (*(u8 *)(temp_18 + 0xB9) != 0) {
            *(u16 *)temp_16 = *(u16 *)temp_16 | 1;
        } else {
            *(u16 *)temp_16 = *(u16 *)temp_16 & 0xFFFE;
        }
        mdlEffectAppendGeometryPrimitive(temp_16, *(u16 *)(temp_18 + 0x28));
    }
}
