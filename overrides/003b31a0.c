/* rpPTankSkyInsColorPV (0x003b31a0, 156 bytes): instances four words per record into four
 * strided slots. arg0: +0 destination base, +4 slot stride S. arg1 is the record count, arg2:
 * +0x2C source base, +0x30 source stride. When the source stride equals 4*S the source is
 * contiguous with the slots and the whole block is copied with func_003a4d50(dst, src, 4*S*n),
 * whatever n is. Otherwise, for each of arg1 > 0 records, source words w0..w3 go to dst, dst+S,
 * dst+2*S and dst+3*S; then dst advances by 4*S and the source by its stride. */

void func_003b31a0(u8 *arg0, s32 arg1, u8 *arg2)
{
    u8 *dst;
    u8 *src;
    s32 stride;
    s32 srcStride;
    s32 i;

    stride = *(s32 *)(arg0 + 4);
    srcStride = *(s32 *)(arg2 + 0x30);
    src = *(u8 **)(arg2 + 0x2C);
    dst = *(u8 **)arg0;
    if (srcStride == stride * 4) {
        func_003a4d50(dst, src, arg1 * stride * 4);
        return;
    }
    for (i = 0; i < arg1; i++) {
        *(u32 *)dst = *(u32 *)(src + 0);
        *(u32 *)(dst + stride) = *(u32 *)(src + 4);
        *(u32 *)(dst + 2 * stride) = *(u32 *)(src + 8);
        *(u32 *)(dst + 3 * stride) = *(u32 *)(src + 12);
        dst += 4 * stride;
        src += srcStride;
    }
}
