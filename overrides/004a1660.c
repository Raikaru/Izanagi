#include "p4pc/vu0.h"

void *FlashQuadCopyAlloc(s32 arg0, void *arg1)
{
    u8 *p18;
    u_long128 *quadSrc;
    u_long128 quad;
    s32 temp_16;
    s32 temp_17;

    if ((u16)arg0 >= 0xA) {
        kAssertFail(D_00713FF0, 0xCD2);
    }
    temp_16 = arg0 & 0xFFFF;
    temp_17 = D_00714028[temp_16].parameterBytes;
    chkmemAllocSiteTag(D_00713FF0, 0xCD6);
    p18 = jtbl_008873E8[0](temp_17 + 0x50, 0x40000);
    if (p18 == NULL) {
        kAssertFail(D_00713FF0, 0xCD7);
    }
    *(u32 *)(p18 + 0x40) = (u32)(p18 + 0x50);
    *(u32 *)(p18 + 0x34) = 0;
    *(u32 *)(p18 + 0x38) = (u32)temp_16;
    *(u32 *)(p18 + 0x30) = -1;
    quadSrc = (u_long128 *)D_00713CE0;
    quad = *quadSrc;
    *(u_long128 *)(p18 + 0x20) = quad;
    p4_vu0_sqc2(p18, 0);
    p4_vu0_sqc2(p18 + 16, 0);
    memcpy(*(void **)(p18 + 0x40), arg1, (void *)temp_17);
    return p18;
}
