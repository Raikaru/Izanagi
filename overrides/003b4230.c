/* func_003b4230 (168 bytes): replicates one three-word source record into four
 * destination slots. dst base = *(arg0+0), slot stride S = *(arg0+4); source record
 * pointer = *(arg2+0x14), source stride = *(arg2+0x18). For each of arg1 records the
 * three words at the record go to dst, dst+S, dst+2*S and dst+3*S; then dst advances
 * by 4*S and the source by its stride. Does nothing when arg1 <= 0. Words are copied
 * as bit patterns (retail lwc1/swc1). Leaf function. */

void func_003b4230(u8 *arg0, s32 arg1, u8 *arg2)
{
    u8 *dst;
    u8 *src;
    u32 stride;
    u32 srcStride;
    s32 i;

    dst = *(u8 **)arg0;
    stride = *(u32 *)(arg0 + 4);
    src = *(u8 **)(arg2 + 0x14);
    srcStride = *(u32 *)(arg2 + 0x18);

    for (i = 0; i < arg1; i++) {
        u32 *s = (u32 *)src;
        u32 *d0 = (u32 *)dst;
        u32 *d1 = (u32 *)(dst + stride);
        u32 *d2 = (u32 *)(dst + 2 * stride);
        u32 *d3 = (u32 *)(dst + 3 * stride);

        d0[0] = s[0];
        d0[1] = s[1];
        d0[2] = s[2];
        d1[0] = s[0];
        d1[1] = s[1];
        d1[2] = s[2];
        d2[0] = s[0];
        d2[1] = s[1];
        d2[2] = s[2];
        d3[0] = s[0];
        d3[1] = s[1];
        d3[2] = s[2];
        dst += 4 * stride;
        src += srcStride;
    }
}
