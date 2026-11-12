/* rpPTankSkyInsUV4 (0x003b3f50, 204 bytes): copies four two-word pairs per record. arg0: +0
 * destination base, +4 slot stride S; arg1 is the record count; arg2: +0x44 source base,
 * +0x48 source stride. When the source stride equals 4*S the block is copied with
 * func_003a4d50(dst, src, 4*S*n) (whatever n is). Otherwise, for each of arg1 > 0 records the
 * pairs at source +0, +8, +0x10 and +0x18 go to dst, dst+S, dst+2*S and dst+3*S; then dst
 * advances by 4*S and the source by its stride. */

void func_003b3f50(u8 *arg0, s32 arg1, u8 *arg2)
{
    u8 *dst;
    u8 *src;
    s32 stride;
    s32 srcStride;
    s32 i;

    stride = *(s32 *)(arg0 + 4);
    srcStride = *(s32 *)(arg2 + 0x48);
    src = *(u8 **)(arg2 + 0x44);
    dst = *(u8 **)arg0;
    if (srcStride == stride * 4) {
        func_003a4d50(dst, src, arg1 * stride * 4);
        return;
    }
    for (i = 0; i < arg1; i++) {
        *(u32 *)dst = *(u32 *)(src + 0);
        *(u32 *)(dst + 4) = *(u32 *)(src + 4);
        *(u32 *)(dst + stride) = *(u32 *)(src + 8);
        *(u32 *)(dst + stride + 4) = *(u32 *)(src + 12);
        *(u32 *)(dst + 2 * stride) = *(u32 *)(src + 16);
        *(u32 *)(dst + 2 * stride + 4) = *(u32 *)(src + 20);
        *(u32 *)(dst + 3 * stride) = *(u32 *)(src + 24);
        *(u32 *)(dst + 3 * stride + 4) = *(u32 *)(src + 28);
        dst += 4 * stride;
        src += srcStride;
    }
}
