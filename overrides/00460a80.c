#include "p4pc/ui_overlay.h"

/* Keep the retail pool reset and clear. Only the main ordering table owns the
 * overlay buckets; the other caller clears a private shadow queue. */
void PrimClearNodes(s32 nodes, s32 count)
{
    memset((void *)nodes, 0, count * 0x30);
    kwlnTaskRegisterCallback(D_00724B88);
    if ((u32)nodes == 0x00793e80u && count == 0xd8)
        p4_ui_overlay_queue_cleared();
}
