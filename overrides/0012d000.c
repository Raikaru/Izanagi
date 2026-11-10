#include "p4pc/vu0.h"
#include "p4pc/settings_native.h"
#include "p4pc/game_items_native.h"

/* 0012d000..0012d13c: retail allocation/task/OT setup, verified against the
 * ELF. Associate only the real child of the field trigger that accepted the
 * shortcut; never manufacture Camp or force a task state from the host. */
s32 cmpMainTaskCreate(s32 parentTask, s16 flags)
{
    s32 task;
    u8 *work;

    chkmemAllocSiteTag(D_005E5890, 0x27E);
    work = (u8 *)D_008873F4[0](1, p4_game_tables_items_native_camp_work_bytes(), 0x40000);
    if (iGpffffb1ec != 0) {
        kAssertFail(D_005E5890, 0x27F);
    }
    iGpffffb1ec = work;
    if (work == NULL) {
        return 0;
    }
    ((CmpMainWork *)work)->flags = flags;
    task = (s32)kwlnTaskCreateChild((void *)parentTask, (const void *)&iGpffff9cb8,
        0xC7, 0, 0, cmpMainTaskStep, cmpMainTaskDestroy, work);
    nLineSelectSet(0);
    nLineSetActive(1);
    memset(work + 0x970C, 0, 0x30);
    ((CmpMainWork *)work)->drawCallback = (s32)cmpMainUpdateMenus;
    ((CmpMainWork *)work)->nodeContext = 0;
    cmpMainSetDefaultTableIndex();
    p4_settings_native_camp_created((u32)parentTask, (u32)work);
    return task;
}
