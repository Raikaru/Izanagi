/* Expanded items live after the complete task work; render/component offsets
 * stay retail and the existing task destructor owns the same allocation. */
#include "p4pc/game_items_native.h"
s32 btlUiCreateTask(s32 parentTask)
{
    u8 *work;
    s32 task;
    u32 bytes = p4_game_tables_items_native_battle_work_bytes();
    chkmemAllocSiteTag(&D_006265B0, 0x3A);
    work = (u8 *)(*jtbl_008873E8)(bytes, 0x40000);
    if (!work) p4_game_tables_items_native_refuse(P4_TABLE_NOMEM);
    memset(work, 0, bytes);
    task = (s32)kwlnTaskCreateChild((void *)(parentTask), (const void *)(&D_00626500), 0xF, 0, 0,
        btlUiTaskInit, btlUiTaskDestroy, work);
    btlUiInitComponents(task, work);
    return task;
}
