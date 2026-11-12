/* _rpPTankSkyClonePipeAllocCallBack (0x003a9c70, 480 bytes): allocates and initializes the
 * per-tank matrix storage of a cloned pipeline. arg0 is the pipeline record, arg1 the matrix
 * count n, arg2 the flags. Returns the raw allocation, or 0 when the allocator fails.
 *
 * The block is 0x1C + 64*n (+ 4*n with flags bit 1) + 0x10 bytes from the allocator slot
 * 0x008873E8 with tag 0x3012F. Its 16-aligned address A is stored at arg0+0x4C; A+0 .. A+8 are
 * cleared and arg0+4 .. arg0+0x4B are zeroed (func_0043f9c8 is the port's memset). The matrices
 * start at (A + 0xC + 0xF) & ~0xF (stored at arg0+0xC; stride 0x40 at arg0+0x10). arg0+0x50 = 1,
 * arg0+0x54 = 0, arg0+0x58 = 8. Each of the n 64-byte matrices gets 1.0f at +0, +0x14 and +0x28, zero
 * at +4, +8, +0x10, +0x18, +0x20, +0x24, +0x30 and +0x34 and +0x38, and 0x20003 OR-ed into +0xC. With
 * flags bit 1 the per-matrix bytes follow the matrices (pointer at arg0+0x24, size word arg0+0x28
 * = 4), arg0+0x50 grows by 1, arg0+0x58 gets bit 1 and the n bytes at arg0+0x24 are filled with 0xFF. */

typedef void *(*P4AllocFn)(u32 size, u32 tag);

u8 *func_003a9c70(u8 *arg0, s32 arg1, u32 arg2)
{
    u32 size;
    u8 *raw;
    u32 *aligned;
    u8 *cur;
    s32 stride;
    s32 i;
    u32 bit2;

    bit2 = arg2 & 2;
    size = 0xC + (u32)(arg1 << 6) + 0x10;
    if (bit2 != 0) {
        size += (u32)(arg1 << 2);
    }
    raw = (u8 *)(*(P4AllocFn *)0x008873E8)(size + 0x10, 0x3012F);
    if (raw == (u8 *)0) {
        return (u8 *)0;
    }
    aligned = (u32 *)(((u32)raw + 0xF) & 0xFFFFFFF0u);
    aligned[1] = 0;
    aligned[2] = 0;
    aligned[0] = 0;
    *(u32 **)(arg0 + 0x4C) = aligned;
    func_0043f9c8(arg0 + 4, 0, 0x48);
    *(u32 *)(arg0 + 0xC) = ((u32)aligned + 0xC + 0xF) & 0xFFFFFFF0u;
    *(u32 *)(arg0 + 0x10) = 0x40;
    *(u32 *)(arg0 + 0x50) = 1;
    *(u32 *)(arg0 + 0x54) = 0;
    *(u32 *)(arg0 + 0x58) = 8;
    cur = *(u8 **)(arg0 + 0xC);
    stride = *(s32 *)(arg0 + 0x10);
    for (i = 0; i < arg1; i++) {
        u32 *m = (u32 *)cur;

        m[0x28 / 4] = 0x3F800000;
        m[0x14 / 4] = 0x3F800000;
        m[0] = 0x3F800000;
        m[0x10 / 4] = 0;
        m[0x8 / 4] = 0;
        m[0x4 / 4] = 0;
        m[0x24 / 4] = 0;
        m[0x20 / 4] = 0;
        m[0x18 / 4] = 0;
        m[0x38 / 4] = 0;
        m[0x34 / 4] = 0;
        cur = cur + stride;
        m[0x30 / 4] = 0;
        m[0xC / 4] = m[0xC / 4] | 0x20003;
    }
    if (bit2 != 0) {
        *(u32 *)(arg0 + 0x24) = *(u32 *)(arg0 + 0xC) + (u32)(arg1 << 6);
        *(u32 *)(arg0 + 0x28) = 4;
        *(u32 *)(arg0 + 0x50) = *(u32 *)(arg0 + 0x50) + 1;
        *(u32 *)(arg0 + 0x58) = *(u32 *)(arg0 + 0x58) | 2;
        func_0043f9c8(*(u8 **)(arg0 + 0x24), 0xFF, (u32)(arg1 << 2));
    }
    return raw;
}
