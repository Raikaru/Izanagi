/* func_00396940 (252 bytes): copies a frame's hierarchy link (arg1 -> arg0) and clones the record.
 * Both frames hold a link slot at frame + *(s32 *)0x007646C8 (gp-0x4A28): +0 an id, +4 the
 * hierarchy record. arg0's id is set from arg1's. When arg1 has a record without bit 0 in its flags
 * word, func_00396fb0(count, 0, 0, flags, record[8][8]) builds a new record, each node's +0, +4
 * and +8 is copied from the source node (+0xC cleared), arg0's link +4 becomes the new record and
 * the new record's +0x14 becomes arg0. Returns arg0. */

void *func_00396fb0(s32 arg0, s32 *arg1, s32 *arg2, u32 arg3, s32 arg4);

u8 *func_00396940(u8 *arg0, u8 *arg1)
{
    u8 *dstSlot;
    u8 *srcSlot;
    u8 *src;
    u8 *dup;
    s32 i;

    dstSlot = arg0 + *(s32 *)0x007646C8;
    srcSlot = arg1 + *(s32 *)0x007646C8;
    src = *(u8 **)(srcSlot + 4);
    *(s32 *)dstSlot = *(s32 *)srcSlot;
    if (src == (u8 *)0 || (*(u32 *)src & 1)) {
        return arg0;
    }
    dup = (u8 *)func_00396fb0(*(s32 *)(src + 4), 0, 0, *(u32 *)src, *(s32 *)(*(u8 **)(src + 0x20) + 0x20));
    i = 0;
    if (0 < *(s32 *)(dup + 4)) {
        s32 off = 0;

        do {
            i += 1;
            *(s32 *)(*(u8 **)(dup + 0x10) + off + 0xC) = 0;
            *(s32 *)(*(u8 **)(dup + 0x10) + off + 8) = *(s32 *)(*(u8 **)(src + 0x10) + off + 8);
            *(s32 *)(*(u8 **)(dup + 0x10) + off + 4) = *(s32 *)(*(u8 **)(src + 0x10) + off + 4);
            *(s32 *)(*(u8 **)(dup + 0x10) + off + 0) = *(s32 *)(*(u8 **)(src + 0x10) + off + 0);
            off += 0x10;
        } while (i < *(s32 *)(dup + 4));
    }
    *(u8 **)(dstSlot + 4) = dup;
    *(u8 **)(dup + 0x14) = arg0;
    return arg0;
}
