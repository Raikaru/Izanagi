#include "p4pc/vu0.h"

u8 *mdlEffectAllocateTrackPath(u16 arg0, s32 arg1, u8 *arg2)
{
    u8 *block;
    u8 *cursor;
    s32 count;
    s32 rowBytes;
    s32 total;
    s32 slotBytes;

    if ((arg0 & 0xFFFF) >= 0xC) {
        kAssertFail(D_00713E10, 0xD66);
    }
    count = arg1 * (*(s32 *)(arg2 + 0xC4) * *(s32 *)(arg2 + 0xC0) + 1);
    rowBytes = count << 5;
    total = arg1 * D_00713D58[(arg0 & 0xFFFF) * 4];
    slotBytes = D_00713D5C[(arg0 & 0xFFFF) * 4];
    total += rowBytes + 0x30;
    total += slotBytes * 2;
    chkmemAllocSiteTag(D_00713E10, 0xD73);
    block = (u8 *)jtbl_008873E8[0](total, 0x40000);
    if (block == NULL) {
        kAssertFail(D_00713E10, 0xD74);
    }
    cursor = block + 0x30;
    *(u8 **)(block + 0x18) = cursor;
    cursor += rowBytes;
    *(u8 **)(block + 0x20) = cursor;
    cursor += slotBytes;
    *(u8 **)(block + 0x24) = cursor;
    cursor += slotBytes;
    *(u8 **)(block + 0x1C) = cursor;
    *(u16 *)(block + 0x00) = arg0;
    *(s32 *)(block + 0x04) = arg1;
    *(s32 *)(block + 0x08) = count;
    *(s32 *)(block + 0x0C) = 0;
    *(s32 *)(block + 0x10) = 0;
    *(s32 *)(block + 0x14) = 0;
    *(u8 **)(block + 0x28) = block;
    memcpy(*(void **)(block + 0x20), arg2, slotBytes);
    memcpy(*(void **)(block + 0x24), arg2, slotBytes);
    cursor = *(u8 **)(block + 0x20);
    /* Retail zeroes the first two quads of the slot from VF0 (sqc2 vf0). */
    p4_vu0_sqc2(cursor, 0);
    p4_vu0_sqc2(cursor + 0x10, 0);
    if (*(u8 *)(cursor + 0xBC) != 0) {
        *(s32 *)(block + 0x0C) |= 1;
    }
    return block;
}
