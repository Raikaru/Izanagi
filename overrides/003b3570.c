/* rpPTankSkyInsColorPPC (0x003b3570, 784 bytes): fills one word per slot. The word is the one
 * at arg2+0x7C. arg0: +0 destination base, +4 slot stride S; arg1 is the record count. When
 * S == 4 it calls func_003a4270(dst, 4*S*n, word) (whatever n is). Otherwise, for each of
 * arg1 > 0 records, the word goes to dst, dst+S, dst+2*S and dst+3*S and dst advances by 4*S.
 * The retail code unrolls the loop by eight; the result is the same for every count. */

void func_003b3570(u8 *arg0, s32 arg1, u8 *arg2)
{
    u8 *dst;
    s32 stride;
    s32 i;

    dst = *(u8 **)arg0;
    stride = *(s32 *)(arg0 + 4);
    if (stride == 4) {
        func_003a4270(dst, arg1 * stride * 4, *(u32 *)(arg2 + 0x7C));
        return;
    }
    for (i = 0; i < arg1; i++) {
        *(u32 *)dst = *(u32 *)(arg2 + 0x7C);
        *(u32 *)(dst + stride) = *(u32 *)(arg2 + 0x7C);
        *(u32 *)(dst + 2 * stride) = *(u32 *)(arg2 + 0x7C);
        *(u32 *)(dst + 3 * stride) = *(u32 *)(arg2 + 0x7C);
        dst += 4 * stride;
    }
}
