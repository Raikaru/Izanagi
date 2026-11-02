/* func_003b7290 (448 bytes): reads a table object's index block from a stream (the mirror of
 * func_003b71b0). arg0 is the stream (func_003df360 reads a word and returns 0 on failure,
 * func_003e2910 reads a block and returns its length, 0 on failure), arg1 the object whose header is
 * at arg1+0x2C. Returns the stream, or 0.
 *
 * Three words are read: kind (A), count n (B) and second count m (C). n <= 0 ends the read. Otherwise
 * func_003b7150(arg1) runs and a block of *(arg1) + 2n + 2m bytes is allocated from the slot
 * 0x008873E8 (tag 0x30116) and zeroed (func_0043f9c8 is the port's memset). The header becomes
 * +0 = A, +4 = n, +8 = m, +0xC = the block, +0x10 = block + *(arg1), +0x14 = +0x10 + 2n; the block is
 * read from the stream; on a failed read it is freed (slot 0x008873EC) and 0 returned. A failed
 * allocation also returns 0. */

typedef void *(*P4AllocFn)(u32 size, u32 tag);
typedef void (*P4FreeFn)(void *ptr);

s32 func_003b7290(s32 arg0, u8 *arg1)
{
    u8 *hdr;
    s32 a;
    s32 n;
    s32 m;
    s32 base;
    u32 size;

    hdr = arg1 + 0x2C;
    if (func_003df360(arg0, &a, 4) == 0) {
        return 0;
    }
    if (func_003df360(arg0, &n, 4) == 0) {
        return 0;
    }
    if (func_003df360(arg0, &m, 4) == 0) {
        return 0;
    }
    if (n <= 0) {
        return arg0;
    }
    base = *(s32 *)arg1;
    func_003b7150(arg1);
    size = (u32)(base + n * 2 + m * 2);
    *(void **)(hdr + 0xC) = (*(P4AllocFn *)0x008873E8)(size, 0x30116);
    if (*(u8 **)(hdr + 0xC) == (u8 *)0) {
        return 0;
    }
    func_0043f9c8(size >= 0 ? *(u8 **)(hdr + 0xC) : (u8 *)0, 0, size);
    *(s32 *)(hdr + 0) = a;
    *(s32 *)(hdr + 4) = n;
    *(s32 *)(hdr + 8) = m;
    *(u32 *)(hdr + 0x10) = *(u32 *)(hdr + 0xC) + (u32)base;
    *(u32 *)(hdr + 0x14) = *(u32 *)(hdr + 0x10) + (u32)(n * 2);
    if (func_003e2910(arg0, *(s32 *)(hdr + 0xC), *(s32 *)arg1 + *(s32 *)(hdr + 4) * 2 + *(s32 *)(hdr + 8) * 2) != 0) {
        return arg0;
    }
    (*(P4FreeFn *)0x008873EC)(hdr);
    return 0;
}
