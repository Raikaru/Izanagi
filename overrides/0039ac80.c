/* func_0039ac80 (432 bytes): copies a light-style link slot from arg1 into arg0 and updates the
 * derived fields. Both owners hold a slot at owner + *(s32 *)0x007646DC (gp-0x4A14; gp is
 * 0x007690F0): +0 a record pointer, +4 a float, +8 a tag. The tag must equal *(arg1+0x18)
 * ("the slot is current"); if not, arg0's slot is cleared and arg0 returned. Otherwise the slot is
 * copied (+0, +4 and the tag) and, when the record is non-null, the block at arg0+0x4C is updated
 * from it: halfword +4 = record halfword +4, halfword +6 = record halfword +6, +8 = record float
 * +8, +0xC = 1.0f / record float +8, +0x10 = slot float. After each store the dirty bits 3 are set
 * in the block's first word and, when bit 2 was clear and arg0+4 holds an object, func_003e9680
 * is called on it. Returns arg0. */

#define P4_MARK_DIRTY(blk, owner)                                         \
    do {                                                                   \
        *(u32 *)(blk) = *(u32 *)(blk) | 3;                                 \
        if (!(*(u32 *)(blk) & 4) && *(u8 **)((owner) + 4) != (u8 *)0) {    \
            func_003e9680(*(u8 **)((owner) + 4));                          \
        }                                                                  \
    } while (0)

u8 *func_0039ac80(u8 *arg0, u8 *arg1)
{
    u8 *srcSlot;
    u8 *dstSlot;
    u8 *blk;
    u8 *rec;
    s32 tag;

    tag = *(s32 *)(arg1 + 0x18);
    srcSlot = arg1 + *(s32 *)0x007646DC;
    dstSlot = arg0 + *(s32 *)0x007646DC;
    blk = arg0 + 0x4C;
    if (*(s32 *)(srcSlot + 8) != tag) {
        *(u32 *)(dstSlot + 0) = 0;
        *(u32 *)(dstSlot + 4) = 0;
        *(u32 *)(dstSlot + 8) = 0;
        return arg0;
    }
    *(u32 *)(dstSlot + 0) = *(u32 *)(srcSlot + 0);
    *(u32 *)(dstSlot + 4) = *(u32 *)(srcSlot + 4);
    *(s32 *)(dstSlot + 8) = *(s32 *)(srcSlot + 8);
    rec = *(u8 **)dstSlot;
    if (rec == (u8 *)0) {
        return arg0;
    }
    *(s16 *)(blk + 4) = *(s16 *)(rec + 4);
    P4_MARK_DIRTY(blk, arg0);
    *(s16 *)(blk + 6) = *(s16 *)(*(u8 **)dstSlot + 6);
    P4_MARK_DIRTY(blk, arg0);
    *(f32 *)(blk + 8) = *(f32 *)(*(u8 **)dstSlot + 8);
    *(f32 *)(blk + 0xC) = 1.0f / *(f32 *)(*(u8 **)dstSlot + 8);
    P4_MARK_DIRTY(blk, arg0);
    *(f32 *)(blk + 0x10) = *(f32 *)(dstSlot + 4);
    P4_MARK_DIRTY(blk, arg0);
    return arg0;
}
