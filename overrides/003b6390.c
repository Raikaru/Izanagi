/* func_003b6390 (144 bytes): strided copy of 16-byte source records into
 * eight-byte slots. For i in [0, arg1), with src = source base at arg2+60
 * (stride arg2+64) and dst = destination base at arg0+0 (stride arg0+4):
 *
 *   dst + 0*S : w0, w3        dst + 1*S : w2, w3
 *   dst + 2*S : w2, w1        dst + 3*S : w0, w1
 *
 * where w0..w3 are the four words of the source record. After each record,
 * dst advances by 4*S and src by its stride. Does nothing when arg1 <= 0.
 * Words are copied as bit patterns (the retail code uses lwc1/swc1).
 * Leaf function. */

void func_003b6390(u8 *arg0, s32 arg1, u8 *arg2)
{
    u8 *dst;
    u8 *src;
    u32 stride;
    u32 srcStride;
    s32 i;

    dst = *(u8 **)arg0;
    stride = *(u32 *)(arg0 + 4);
    src = *(u8 **)(arg2 + 60);
    srcStride = *(u32 *)(arg2 + 64);

    for (i = 0; i < arg1; i++) {
        u32 *s = (u32 *)src;
        u32 *d0 = (u32 *)dst;
        u32 *d1 = (u32 *)(dst + stride);
        u32 *d2 = (u32 *)(dst + 2 * stride);
        u32 *d3 = (u32 *)(dst + 3 * stride);

        d0[0] = s[0];
        d0[1] = s[3];
        d1[0] = s[2];
        d1[1] = s[3];
        d2[0] = s[2];
        d2[1] = s[1];
        d3[0] = s[0];
        d3[1] = s[1];
        dst += 4 * stride;
        src += srcStride;
    }
}
