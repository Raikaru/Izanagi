/* func_003a1730 (1296 bytes): allocates the native vertex streams of the geometry arg0 for the stream flags arg1 and returns 1 (0 when the allocation fails).
 * arg0[0] is the vertex count n; the nine stream bits (1, 8, 0x10, 4, 2, 0x40, 0x20, 0x80, 0x100) have element sizes at 0x00752D20 + 4 * k
 * (k = 0..8 in that order). arg0[0xA8 / 4] has its low 9 bits replaced by the bits asked for, arg0[0xA0 / 4] counts them, arg0[0x3C / 4] is cleared. One block of
 * 0x10 + n * (sum of the sizes) bytes is made with the allocator slot 0x008873E8 (tag 0x3012F), aligned up to 16 bytes; the streams are laid out in the bit order above
 * (stream k at the running offset, n * size apart): the pointer / stride pairs go to arg0[0x54 / 4] / [0x58 / 4] (stride 0xC, bit 1), [0x5C / 4] / [0x60 / 4] (0x40, bit 8),
 * [0x64 / 4] / [0x68 / 4] (0xC, bit 0x10), [0x6C / 4] / [0x70 / 4] (8, bit 4), [0x74 / 4] / [0x78 / 4] (4, bit 2), [0x7C / 4] / [0x80 / 4] (0x10, bit 0x40),
 * [0x84 / 4] / [0x88 / 4] (4, bit 0x20) and [0x8C / 4] / [0x90 / 4] (0x10, bit 0x80); bit 0x100 gives [0x94 / 4] = the pointer and [0x98 / 4] = 0x20 (its size
 * adds to the block but no further stream follows). Finally arg0[0xA4 / 4] = 0 and arg0[3] = the raw block. */

typedef void *(*P4AllocFn_003a1730)(u32 size, u32 tag);

s32 func_003a1730(u32 *arg0, u32 arg1)
{
    static const u32 bits[9] = {1, 8, 0x10, 4, 2, 0x40, 0x20, 0x80, 0x100};
    static const u32 slot[8] = {0x54 / 4, 0x5C / 4, 0x64 / 4, 0x6C / 4, 0x74 / 4, 0x7C / 4, 0x84 / 4, 0x8C / 4};
    static const u32 stride[8] = {0xC, 0x40, 0xC, 8, 4, 0x10, 4, 0x10};
    u32 total = 0;
    u32 off = 0;
    u32 p;
    u32 aligned;
    u32 k;

    arg0[0xA8 / 4] = arg0[0xA8 / 4] & 0xFFFFFE00u;
    arg0[0xA0 / 4] = 0;
    arg0[0x3C / 4] = 0;
    for (k = 0; k < 9; k++) {
        if ((arg1 & bits[k]) != 0) {
            arg0[0xA0 / 4] = arg0[0xA0 / 4] + 1;
            arg0[0xA8 / 4] = arg0[0xA8 / 4] | bits[k];
            total = total + *(u32 *)(0x00752D20 + 4 * k) * arg0[0];
        }
    }
    p = (u32)(*(P4AllocFn_003a1730 *)0x008873E8)(total + 0x10, 0x3012F);
    if (p == 0) {
        return 0;
    }
    aligned = (p + 0xF) & 0xFFFFFFF0u;
    for (k = 0; k < 8; k++) {
        if ((arg1 & bits[k]) != 0) {
            arg0[slot[k]] = aligned + off;
            arg0[slot[k] + 1] = stride[k];
            off = off + *(u32 *)(0x00752D20 + 4 * k) * arg0[0];
        }
    }
    if ((arg1 & 0x100) != 0) {
        arg0[0x94 / 4] = aligned + off;
        arg0[0x98 / 4] = 0x20;
    }
    arg0[0xA4 / 4] = 0;
    arg0[3] = p;
    return 1;
}
