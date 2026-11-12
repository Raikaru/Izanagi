/* func_00414930 (RxHeap block carve): takes a 32-byte node from the slot pool
 * (func_00414820), then splits the region {base=blk[0], size=blk[1]} so that
 * the node sits at the top of the region and the rest stays as the free block.
 * Returns 1 on success, 0 when the slot pool is exhausted.
 *
 * Direct transliteration of the retail code. Float fields in the node are
 * copied as raw 32-bit words (retail lwc1/swc1), not converted. */

#define P4_HEAP_WD(p, off) (*(u32 *)((u8 *)(p) + (off)))

u8 *func_00414820(u32 *w);

s32 func_00414930(u32 *blk, u32 *prev, u32 *w)
{
    u32 *slot;
    u8 *base;
    u8 *end;
    u8 *head;
    u8 *tail;
    u32 remaining;

    slot = (u32 *)func_00414820(w);
    if (slot == NULL)
        return 0;

    base = (u8 *)blk[0];
    end = base + blk[1];
    head = base + 32;
    tail = end - 32;

    P4_HEAP_WD(base, 0) = 0;
    P4_HEAP_WD(base, 4) = 0;
    P4_HEAP_WD(base, 8) = 0;
    P4_HEAP_WD(base, 12) = 0;

    /* Copy the 16-byte header (now zero) to the node's top, then the next
     * 16 bytes of the original region header. */
    P4_HEAP_WD(end, -32) = P4_HEAP_WD(base, 0);
    P4_HEAP_WD(end, -28) = P4_HEAP_WD(base, 4);
    P4_HEAP_WD(end, -24) = P4_HEAP_WD(base, 8);
    P4_HEAP_WD(end, -20) = P4_HEAP_WD(base, 12);
    P4_HEAP_WD(end, -16) = P4_HEAP_WD(base, 16);
    P4_HEAP_WD(end, -12) = P4_HEAP_WD(base, 20);
    P4_HEAP_WD(end, -8) = P4_HEAP_WD(base, 24);
    P4_HEAP_WD(end, -4) = P4_HEAP_WD(base, 28);

    P4_HEAP_WD(base, 4) = (u32)head;
    P4_HEAP_WD(base, 32) = (u32)base;
    P4_HEAP_WD(base, 36) = (u32)tail;
    P4_HEAP_WD(end, -32) = (u32)head;

    remaining = (u32)(tail - (head + 32));
    P4_HEAP_WD(base, 40) = remaining;
    P4_HEAP_WD(base, 44) = (u32)slot;

    slot[1] = (u32)head;
    slot[0] = remaining;

    if (prev != NULL) {
        u8 *prev_end = (u8 *)prev[0] + prev[1];
        P4_HEAP_WD(prev_end, -28) = (u32)base;
        P4_HEAP_WD(base, 0) = (u32)(prev_end - 32);
    }
    return 1;
}
