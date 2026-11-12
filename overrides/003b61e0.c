/* func_003b61e0 (424 bytes): strided fill of one word from arg2. The word is the one at
 * arg2+0x7C. For each of arg1 records, with dst base = *(arg0+0) and slot stride
 * S = *(arg0+4), it is stored at dst, dst+S, dst+2*S and dst+3*S; then dst advances by
 * 4*S. Does nothing when arg1 <= 0. The retail code unrolls the loop by eight with a scalar
 * tail; the result is the same for every count. Leaf function. */

void func_003b61e0(u8 *arg0, s32 arg1, u8 *arg2)
{
    u8 *dst;
    u32 stride;
    u32 w;
    s32 i;

    dst = *(u8 **)arg0;
    w = *(u32 *)(arg2 + 0x7C);
    stride = *(u32 *)(arg0 + 4);

    for (i = 0; i < arg1; i++) {
        *(u32 *)dst = w;
        *(u32 *)(dst + stride) = w;
        *(u32 *)(dst + 2 * stride) = w;
        *(u32 *)(dst + 3 * stride) = w;
        dst += 4 * stride;
    }
}
