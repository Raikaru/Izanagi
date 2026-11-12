/* func_00414820 (RxHeap slot pool): returns the next free 8-byte slot record
 * from the pool, growing the pool by 32 slots through the realloc slot when it
 * is full. Heap work object layout: w[3] pool base, w[4] capacity (slots),
 * w[5] count (slots handed out).
 *
 * Direct transliteration of the retail code. Stride is 8 bytes per slot
 * (retail `sll 3`). On realloc, a moved pool has its back-links rebuilt:
 * each slot's [4] record gets its +0xC set to the slot. If realloc fails the
 * retail path reports through func_003df590/func_003df4d0 and returns NULL. */

typedef void *(*P4HeapReallocFn)(void *p, u32 size, u32 flags);

u8 *func_00414820(u32 *w)
{
    extern s32 func_003df590();
    extern u8 *func_003df4d0();
    P4HeapReallocFn realloc_fn = *(P4HeapReallocFn *)0x008873F0;
    u32 count;
    u8 *old;
    u8 *nb;
    u8 *p;
    u32 i;
    u32 sp48;

    count = w[5];
    if (count < w[4]) {
        u8 *base = (u8 *)w[3];
        if (base == NULL)
            return NULL;
        w[5] = count + 1;
        return base + count * 8;
    }
    w[4] = w[4] + 32;
    old = (u8 *)w[3];
    nb = (u8 *)realloc_fn(old, w[4] * 8, 0x01030409);
    if (nb == NULL) {
        /* retail reports the failure, takes the 32 slots back and returns NULL (it does not retry) */
        sp48 = 1;
        func_003df590(0x80000013u, w[4] * 8);
        func_003df4d0(&sp48);
        w[4] = w[4] - 32;
        return NULL;
    }
    if (nb != old && count != 0) {
        p = nb;
        for (i = count; i != 0; i--) {
            *(u32 *)(*(u8 **)(p + 4) + 12) = (u32)p;
            p += 8;
        }
    }
    w[3] = (u32)nb;
    /* retail hands the slot out straight away (no second capacity test) */
    count = w[5];
    w[5] = count + 1;
    return nb + count * 8;
}
