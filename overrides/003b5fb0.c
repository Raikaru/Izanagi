/* func_003b5fb0 (104 bytes): strided record copy. For i in [0, arg1), it
 * copies the four words of source record i (src + i * srcStride) to four
 * destination words spaced by stride: dst, dst + stride, dst + 2*stride,
 * dst + 3*stride. Then dst advances by 4 * stride.
 *
 * Inputs: arg0 holds the destination base at +0 and the stride at +4.
 * arg2 holds the source base at +44 and the source stride at +48.
 * Does nothing when arg1 <= 0. Leaf function. */

void func_003b5fb0(u8 *arg0, s32 arg1, u8 *arg2)
{
    u8 *dst;
    u8 *src;
    u32 stride;
    u32 srcStride;
    s32 i;

    dst = *(u8 **)arg0;
    stride = *(u32 *)(arg0 + 4);
    src = *(u8 **)(arg2 + 44);
    srcStride = *(u32 *)(arg2 + 48);

    for (i = 0; i < arg1; i++) {
        u32 *s = (u32 *)src;

        *(u32 *)dst = s[0];
        *(u32 *)(dst + stride) = s[1];
        *(u32 *)(dst + 2 * stride) = s[2];
        *(u32 *)(dst + 3 * stride) = s[3];
        dst += 4 * stride;
        src += srcStride;
    }
}
