#include "p4pc/vu0.h"

void *WindAllocKindEntry(s32 arg0, void *arg1)
{
    u8 *p18;
    u_long128 *dst128;
    s32 temp_16;
    s32 temp_17;

    if ((u16)arg0 >= 4) {
        kAssertFail(D_00714110, 0x59F);
    }
    temp_16 = arg0 & 0xFFFF;
    temp_17 = D_00714148[temp_16].parameterBytes;
    chkmemAllocSiteTag(D_00714110, 0x5A3);
    p18 = jtbl_008873E8[0](temp_17 + 0x50, 0x40000);
    if (p18 == NULL) {
        kAssertFail(D_00714110, 0x5A4);
    }
    *(u32 *)(p18 + 0x40) = (u32)(p18 + 0x50);
    *(u32 *)(p18 + 0x34) = 0;
    *(u32 *)(p18 + 0x38) = (u32)temp_16;
    *(u32 *)(p18 + 0x30) = -1;
    dst128 = &D_00713CE0;
    *(u_long128 *)(p18 + 0x20) = *dst128;
    p4_vu0_sqc2(p18, 0);
    p4_vu0_sqc2(p18 + 16, 0);
    memcpy(*(void **)(p18 + 0x40), arg1, (u32)temp_17);
    return p18;
}
