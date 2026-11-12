#include "p4pc/vu0.h"

u8 *ThunderTextureForIndex(u16 arg0, u8 *arg1)
{
    u8 *p;
    s32 size;
    u32 idx;

    if (arg0 >= 5)
    {
        kAssertFail(D_00713E50, 0x6A1);
    }
    idx = arg0;
    size = D_00713E84[idx * 6];
    chkmemAllocSiteTag(D_00713E50, 0x6A5);
    p = (u8 *)(*jtbl_008873E8)(size + 0x40, 0x40000);
    if (p == NULL)
    {
        kAssertFail(D_00713E50, 0x6A6);
    }
    *(u32 *)(p + 0x34) = (u32)(p + 0x40);
    *(u32 *)(p + 0x28) = 0;
    *(u32 *)(p + 0x2C) = idx;
    *(u32 *)(p + 0x24) = -1;
    *(u32 *)(p + 0x20) = 0x3F800000;
    p4_vu0_sqc2(p, 0);
    p4_vu0_sqc2(p + 0x10, 0);
    memcpy(*(void **)(p + 0x34), arg1, size);
    *(u32 *)(p + 0x30) = ((u32 (*)(u8 *))D_00713E74[arg0 * 6])(arg1);
    ((void (*)(u8 *))D_00713E70[arg0 * 6])(p);
    return p;
}
