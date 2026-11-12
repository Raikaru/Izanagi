/* func_003b6420 (152 bytes): strided copy of 32-byte source records. For
 * i in [0, arg1), src = source base at arg2+68 (stride arg2+72), dst = base
 * at arg0+0 (stride arg0+4). The eight words of source record i fill four
 * two-word slots: dst + k*S gets source words 2k and 2k+1 (k = 0..3).
 * Then dst advances by 4*S and src by its stride. Does nothing when
 * arg1 <= 0. Words are copied as bit patterns (retail lwc1/swc1). Leaf. */

void func_003b6420(u8 *arg0, s32 arg1, u8 *arg2)
{
    u8 *dst;
    u8 *src;
    u32 stride;
    u32 srcStride;
    s32 i;

    dst = *(u8 **)arg0;
    stride = *(u32 *)(arg0 + 4);
    src = *(u8 **)(arg2 + 68);
    srcStride = *(u32 *)(arg2 + 72);

    for (i = 0; i < arg1; i++) {
        u32 *s = (u32 *)src;
        u32 *d0 = (u32 *)dst;
        u32 *d1 = (u32 *)(dst + stride);
        u32 *d2 = (u32 *)(dst + 2 * stride);
        u32 *d3 = (u32 *)(dst + 3 * stride);

        d0[0] = s[0];
        d0[1] = s[1];
        d1[0] = s[2];
        d1[1] = s[3];
        d2[0] = s[4];
        d2[1] = s[5];
        d3[0] = s[6];
        d3[1] = s[7];
        dst += 4 * stride;
        src += srcStride;
    }
}
