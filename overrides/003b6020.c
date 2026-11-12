/* func_003b6020 (440 bytes): strided fill of four words from arg2. The words w0..w3 are
 * the ones at arg2+0x80, +0x84, +0x88 and +0x8C. For each of arg1 records, with dst base =
 * *(arg0+0) and slot stride S = *(arg0+4), w_k goes to dst + k*S (k = 0..3); then dst
 * advances by 4*S. Does nothing when arg1 <= 0. The retail code unrolls the loop by eight
 * with a scalar tail; the result is the same for every count. Leaf function. */

void func_003b6020(u8 *arg0, s32 arg1, u8 *arg2)
{
    u8 *dst;
    u32 stride;
    u32 w0;
    u32 w1;
    u32 w2;
    u32 w3;
    s32 i;

    dst = *(u8 **)arg0;
    w0 = *(u32 *)(arg2 + 0x80);
    w1 = *(u32 *)(arg2 + 0x84);
    w2 = *(u32 *)(arg2 + 0x88);
    stride = *(u32 *)(arg0 + 4);
    w3 = *(u32 *)(arg2 + 0x8C);

    for (i = 0; i < arg1; i++) {
        *(u32 *)dst = w0;
        *(u32 *)(dst + stride) = w1;
        *(u32 *)(dst + 2 * stride) = w2;
        *(u32 *)(dst + 3 * stride) = w3;
        dst += 4 * stride;
    }
}
