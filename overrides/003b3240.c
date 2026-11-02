/* rpPTankSkyInsColorPVC (0x003b3240, 816 bytes): fills four words per record. The words are the
 * ones at arg2+0x80, +0x84, +0x88 and +0x8C. arg0: +0 destination base, +4 slot stride S;
 * arg1 is the record count. When S == 4 the four words are copied to a 16-byte buffer and
 * func_003a4850(dst, 4*S*n, buffer) fills the block (called whatever n is). Otherwise, for each of
 * arg1 > 0 records, word k goes to dst + k*S (k = 0..3) and dst advances by 4*S. The retail
 * code unrolls the loop by eight; the result is the same for every count. */

void func_003b3240(u8 *arg0, s32 arg1, u8 *arg2)
{
    u8 *dst;
    s32 stride;
    s32 i;

    dst = *(u8 **)arg0;
    stride = *(s32 *)(arg0 + 4);
    if (stride == 4) {
        u32 pattern[4];

        pattern[0] = *(u32 *)(arg2 + 0x80);
        pattern[1] = *(u32 *)(arg2 + 0x84);
        pattern[2] = *(u32 *)(arg2 + 0x88);
        pattern[3] = *(u32 *)(arg2 + 0x8C);
        func_003a4850(dst, arg1 * stride * 4, (u8 *)pattern);
        return;
    }
    for (i = 0; i < arg1; i++) {
        *(u32 *)dst = *(u32 *)(arg2 + 0x80);
        *(u32 *)(dst + stride) = *(u32 *)(arg2 + 0x84);
        *(u32 *)(dst + 2 * stride) = *(u32 *)(arg2 + 0x88);
        *(u32 *)(dst + 3 * stride) = *(u32 *)(arg2 + 0x8C);
        dst += 4 * stride;
    }
}
