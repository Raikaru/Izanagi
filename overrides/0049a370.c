#include "p4pc/vu0.h"

u8 *RingCreateKindInstance(u16 arg0, u8 *arg1)
{
    u8 *p;
    u_long128 *quadSrc;
    u_long128 quad;
    s32 size;
    u32 idx;

    if (arg0 >= 4) {
        kAssertFail(D_00713EF0, 0x2BD);
    }
    idx = arg0;
    size = D_00713F24[idx * 6];
    chkmemAllocSiteTag(D_00713EF0, 0x2C1);
    p = (u8 *)(*jtbl_008873E8)(size + 0x50, 0x40000);
    if (p == NULL) {
        kAssertFail(D_00713EF0, 0x2C2);
    }
    *(u8 **)(p + 0x40) = p + 0x50;
    *(s32 *)(p + 0x34) = 0;
    *(s32 *)(p + 0x38) = idx;
    *(s32 *)(p + 0x30) = -1;
    quadSrc = (u_long128 *)D_00713CE0;
    quad = *quadSrc;
    *(u_long128 *)(p + 0x20) = quad;
    p4_vu0_sqc2(p, 0);
    p4_vu0_sqc2(p + 0x10, 0);
    memcpy(*(void **)(p + 0x40), arg1, size);
    *(u32 *)(p + 0x3C) =
        ((u32 (*)(u8 *))D_00713F14[arg0 * 6])(arg1);
    ((void (*)(u8 *))D_00713F10[arg0 * 6])(p);
    return p;
}
