/* func_003b6540 (136 bytes): strided fill from four fixed word pairs in arg2. Pair k
 * is the two words at arg2+0x90+8k and arg2+0x94+8k. For each of arg1 records, with
 * dst base = *(arg0+0) and slot stride S = *(arg0+4), pair k goes to dst + k*S; then
 * dst advances by 4*S. The source is not advanced. Does nothing when arg1 <= 0.
 * Words are copied as bit patterns (retail lwc1/swc1). Leaf function. */

void func_003b6540(u8 *arg0, s32 arg1, u8 *arg2)
{
    u8 *dst;
    u32 stride;
    u32 *pair;
    s32 i;

    dst = *(u8 **)arg0;
    stride = *(u32 *)(arg0 + 4);
    pair = (u32 *)(arg2 + 0x90);

    for (i = 0; i < arg1; i++) {
        u32 *d0 = (u32 *)dst;
        u32 *d1 = (u32 *)(dst + stride);
        u32 *d2 = (u32 *)(dst + 2 * stride);
        u32 *d3 = (u32 *)(dst + 3 * stride);

        d0[0] = pair[0];
        d0[1] = pair[1];
        d1[0] = pair[2];
        d1[1] = pair[3];
        d2[0] = pair[4];
        d2[1] = pair[5];
        d3[0] = pair[6];
        d3[1] = pair[7];
        dst += 4 * stride;
    }
}
