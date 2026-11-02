/* func_003971d0 (448 bytes): builds a sub-record for the subtree that starts at node arg1 of the
 * hierarchy record arg0 (+8 matrix array, +0x10 16-byte nodes, +0x20 anim object). Returns it.
 * arg2 is OR-ed with 1 into the flags; arg3 is passed on.
 *
 * The subtree length n is counted from the node's flags word (+8): if the node's bit 0 is set it is
 * a leaf and n = 1; else n starts at 1 and the following nodes are scanned with a depth counter
 * starting at 1 (bit 1 pushes, bit 0 pops) until it reaches 0, n counting the scanned nodes. The
 * record comes from the freelist block of 0x008873F8 (pool 0x007646CC, tag 0x3011E): +0 = arg2 | 1,
 * +4 = n, +0x20 = func_003d5f50(arg0[0x20], arg1, n, arg3), +0x14 = node +0xC, +8 = arg0 +8 +
 * 64*arg1, +0xC = 0, +0x10 = the node pointer, +0x18 = arg0. +0x1C is the parent index: -1 when
 * arg1 is 0, else found by replaying nodes 0..arg1-1 with a stack (bit 1 pushes the current parent,
 * bit 0 pops it, otherwise the parent becomes the node's index). */

typedef void *(*P4PoolAllocFn)(u32 pool, u32 tag);

u8 *func_003971d0(u8 *arg0, s32 arg1, u32 arg2, s32 arg3)
{
    u8 *nodes;
    u8 *p;
    s32 depth;
    s32 n;
    u8 *rec;
    s32 off;

    n = 1;
    nodes = *(u8 **)(arg0 + 0x10);
    off = arg1 << 4;
    p = nodes + off;
    if (*(u32 *)(p + 8) & 1) {
        depth = 0;
    } else {
        depth = n;
        p += 0x10;
    }
    if (depth != 0) {
        do {
            u32 w = *(u32 *)(p + 8);

            if (w & 2) {
                depth += 1;
            }
            if (w & 1) {
                depth -= 1;
            }
            n += 1;
            p += 0x10;
        } while (depth != 0);
    }
    rec = (u8 *)(*(P4PoolAllocFn *)0x008873F8)(*(u32 *)0x007646CC, 0x3011E);
    *(u32 *)(rec + 0) = arg2 | 1;
    *(s32 *)(rec + 4) = n;
    *(void **)(rec + 0x20) = func_003d5f50(*(u8 **)(arg0 + 0x20), arg1, n, arg3);
    if (arg1 == 0) {
        *(s32 *)(rec + 0x1C) = -1;
    } else {
        s32 stack[32];
        s32 *sp = stack;
        s32 parent = -1;
        s32 i;

        i = 0;
        if (0 < arg1) {
            do {
                u32 w = *(u32 *)(*(u8 **)(arg0 + 0x10) + 16 * i + 8);

                if (w & 2) {
                    *sp = parent;
                    sp += 1;
                }
                w = *(u32 *)(*(u8 **)(arg0 + 0x10) + 16 * i + 8);
                if (w & 1) {
                    sp -= 1;
                    parent = *sp;
                } else {
                    parent = i;
                }
                i += 1;
            } while (i < arg1);
        }
        *(s32 *)(rec + 0x1C) = parent;
    }
    *(u32 *)(rec + 0x14) = *(u32 *)(*(u8 **)(arg0 + 0x10) + off + 0xC);
    *(u32 *)(rec + 8) = *(u32 *)(arg0 + 8) + (u32)(arg1 << 6);
    *(u32 *)(rec + 0xC) = 0;
    *(u8 **)(rec + 0x10) = *(u8 **)(arg0 + 0x10) + off;
    *(u8 **)(rec + 0x18) = arg0;
    return rec;
}
