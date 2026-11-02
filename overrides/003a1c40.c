/* func_003a1c40 (1176 bytes): like func_003a1730 but interleaved: allocates the vertex streams of the geometry arg0 for the stream flags arg1 as one array of n records and
 * returns 1 (0 when the allocation fails). The record size is the sum of the element sizes (0x00752D20 + 4 * k for the bits 1, 8, 0x10, 4, 2, 0x40, 0x20, 0x80, 0x100, k = 0..8)
 * of the bits asked for, rounded up to 16 bytes when bit 8 is among them; it is stored to arg0[0xA4 / 4] and is the stride of every stream. arg0[0xA8 / 4] has its low 9 bits
 * replaced by the bits asked for and arg0[0xA0 / 4] counts them. One block of 0x10 + n * record bytes (n = arg0[0]) comes from the allocator slot 0x008873E8 (tag 0x30000),
 * aligned up to 16 bytes; stream k starts at the running offset inside the record and its pointer / stride pair is stored at arg0[0x54 / 4 + 2 * j] / the next word
 * (bits 1, 8, 0x10, 4, 2, 0x40, 0x20, 0x80 -> [0x54], [0x5C], [0x64], [0x6C], [0x74], [0x7C], [0x84], [0x8C], bit 0x100 -> [0x94] / [0x98]). arg0[3] = the raw block. */

typedef void *(*P4AllocFn_003a1c40)(u32 size, u32 tag);

s32 func_003a1c40(u32 *arg0, u32 arg1)
{
    static const u32 bits[9] = {1, 8, 0x10, 4, 2, 0x40, 0x20, 0x80, 0x100};
    static const u32 slot[8] = {0x54 / 4, 0x5C / 4, 0x64 / 4, 0x6C / 4, 0x74 / 4, 0x7C / 4, 0x84 / 4, 0x8C / 4};
    u32 rec = 0;
    u32 off = 0;
    u32 p;
    u32 aligned;
    u32 k;

    arg0[0xA8 / 4] = arg0[0xA8 / 4] & 0xFFFFFE00u;
    arg0[0xA0 / 4] = 0;
    for (k = 0; k < 9; k++) {
        if ((arg1 & bits[k]) != 0) {
            arg0[0xA0 / 4] = arg0[0xA0 / 4] + 1;
            rec = rec + *(u32 *)(0x00752D20 + 4 * k);
            arg0[0xA8 / 4] = arg0[0xA8 / 4] | bits[k];
        }
    }
    if ((arg1 & 8) != 0) {
        rec = (rec + 0xF) & 0xFFFFFFF0u;
    }
    arg0[0xA4 / 4] = rec;
    p = (u32)(*(P4AllocFn_003a1c40 *)0x008873E8)((u32)((s32)arg0[0] * (s32)rec) + 0x10, 0x30000);
    if (p == 0) {
        return 0;
    }
    aligned = (p + 0xF) & 0xFFFFFFF0u;
    for (k = 0; k < 8; k++) {
        if ((arg1 & bits[k]) != 0) {
            arg0[slot[k]] = aligned + off;
            arg0[slot[k] + 1] = arg0[0xA4 / 4];
            off = off + *(u32 *)(0x00752D20 + 4 * k);
        }
    }
    if ((arg1 & 0x100) != 0) {
        arg0[0x94 / 4] = aligned + off;
        arg0[0x98 / 4] = arg0[0xA4 / 4];
    }
    arg0[3] = p;
    return 1;
}
