#include "p4pc/vu0.h"

void RingUpdateRowColourAlt(u8 *param_1)
{
    u8 *iVar5;
    u8 *iVar1;
    u16 *puVar2;
    u8 colourStack[16];
    int iVar3;
    int iVar4;
    int iVar6;
    u8 *dest;
    f32 scale;

    iVar5 = (u8 *)param_1;
    iVar3 = *(int *)(iVar5 + 0x3c);
    iVar1 = *(u8 **)(iVar5 + 0x40);
    puVar2 = *(u16 **)iVar3;
    iVar6 = *(int *)(iVar5 + 0x34);
    iVar4 = *(int *)(iVar1 + 0x34);
    if (((u32)iVar6 <= (u32)iVar4) || (iVar4 == 0)) {
        iVar3 = mdlEffectSamplePackedColor((char *)iVar1, (u32 *)(iVar1 + 0x24), iVar6, iVar4);
        *(u32 *)&colourStack[8] = *(u32 *)(iVar5 + 0x30);
        scale = iGpffff8044;
        *(u32 *)&colourStack[0] = p4_vu0_packed_color_scale2(*(u32 *)&colourStack[8], (u32)iVar3, scale);
        *(u32 *)&colourStack[12] = *(u32 *)&colourStack[0];
        if (colourStack[15] != 0xff) {
            dest = *(u8 **)(puVar2 + 10);
            *(RwRGBA *)(dest + 4) = *(RwRGBA *)&colourStack[12];
        }
        else {
            colourStack[15] = 0xfe;
            dest = *(u8 **)(puVar2 + 10);
            *(RwRGBA *)(dest + 4) = *(RwRGBA *)&colourStack[12];
            colourStack[15] = 0xff;
        }
        if (*(u8 *)(*(int *)(puVar2 + 10) + 7) != 0) {
            GeometryBlockSetFrameMatrixCombined(puVar2, iVar5, iVar5 + 0x10, iVar5 + 0x20);
            if (*(u8 *)(iVar1 + 0x3c) != 0) {
                *puVar2 = *puVar2 | 1;
            }
            else {
                *puVar2 = *puVar2 & 0xfffe;
            }
            GeometryBlockBindResource((int)puVar2, *(u16 *)(iVar1 + 0x28));
        }
    }
}
