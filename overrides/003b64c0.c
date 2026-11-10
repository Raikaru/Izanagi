/* func_003b64c0 (128 bytes): strided fill from two fixed pairs in arg2.
 * Pair A is the two words at arg2+144 and arg2+148; pair B is the two words
 * at arg2+152 and arg2+156. For i in [0, arg1), with dst = base at arg0+0
 * and stride S = arg0+4:
 *
 *   dst + 0*S : A0, B1        dst + 1*S : B0, B1
 *   dst + 2*S : B0, A1        dst + 3*S : A0, A1
 *
 * then dst advances by 4*S. The source is not advanced. Does nothing when
 * arg1 <= 0. Words are copied as bit patterns (retail lwc1/swc1). Leaf. */

void func_003b64c0(u8 *arg0, s32 arg1, u8 *arg2)
{
    u8 *dst;
    u32 stride;
    u32 *a;
    u32 *b;
    s32 i;

    dst = *(u8 **)arg0;
    stride = *(u32 *)(arg0 + 4);
    a = (u32 *)(arg2 + 144);
    b = (u32 *)(arg2 + 152);

    for (i = 0; i < arg1; i++) {
        u32 *d0 = (u32 *)dst;
        u32 *d1 = (u32 *)(dst + stride);
        u32 *d2 = (u32 *)(dst + 2 * stride);
        u32 *d3 = (u32 *)(dst + 3 * stride);

        d0[0] = a[0];
        d0[1] = b[1];
        d1[0] = b[0];
        d1[1] = b[1];
        d2[0] = b[0];
        d2[1] = a[1];
        d3[0] = a[0];
        d3[1] = a[1];
        dst += 4 * stride;
    }
}
