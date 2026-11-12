#include "p4pc/vu0.h"

u8 *EffBattleAllocWork(s32 id, s32 arg1)
{
    EffBattleWork *work;
    s32 size;

    if ((u16)id >= 0xD) {
        kAssertFail(D_007146A0, 0x1B);
    } else {
    }
    size = D_00764C90[(id & 0xFFFF)].size;
    chkmemAllocSiteTag(D_007146A0, 0x1F);
    work = (EffBattleWork *)(*jtbl_008873E8)(size + 0x40, 0x40000);
    if (work == NULL) {
        kAssertFail(D_007146A0, 0x20);
    }
    work->data = (u8 *)work + 0x40;
    work->count = 0;
    work->id = (id & 0xFFFF);
    work->color = 0x80808080;
    work->alpha = 1.0f;
    p4_vu0_sqc2(work, 0);
    p4_vu0_sqc2((u8 *)work + 16, 0);
    memcpy(work->data, (const void *)arg1, size);
    return (u8 *)work;
}
