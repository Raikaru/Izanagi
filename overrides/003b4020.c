/* rpPTankSkyInsUV2C (0x003b4020, 256 bytes): like func_003b6540 with a constant pair layout.
 * Let A = (arg2+0x90, arg2+0x94) and B = (arg2+0x98, arg2+0x9C). arg0: +0 destination base,
 * +4 slot stride S; arg1 is the record count. When S == 16 the eight words
 * A0 B1 B0 B1 B0 A1 A0 A1 are copied to a 32-byte buffer and func_003a4ab0(dst, 4*S*n, buffer)
 * fills the block (called whatever n is). Otherwise, for each of arg1 > 0 records, the pairs
 * (A0,B1), (B0,B1), (B0,A1), (A0,A1) go to dst, dst+S, dst+2*S, dst+3*S and dst advances by 4*S. */

void func_003b4020(u8 *arg0, s32 arg1, u8 *arg2)
{
    u8 *dst;
    s32 stride;
    s32 i;

    stride = *(s32 *)(arg0 + 4);
    if (stride == 0x10) {
        u32 pattern[8];

        pattern[0] = *(u32 *)(arg2 + 0x90);
        pattern[1] = *(u32 *)(arg2 + 0x9C);
        pattern[2] = *(u32 *)(arg2 + 0x98);
        pattern[3] = *(u32 *)(arg2 + 0x9C);
        pattern[4] = *(u32 *)(arg2 + 0x98);
        pattern[5] = *(u32 *)(arg2 + 0x94);
        pattern[6] = *(u32 *)(arg2 + 0x90);
        pattern[7] = *(u32 *)(arg2 + 0x94);
        func_003a4ab0(*(u8 **)arg0, *(s32 *)(arg0 + 4) * arg1 * 4, (u8 *)pattern);
        return;
    }
    dst = *(u8 **)arg0;
    for (i = 0; i < arg1; i++) {
        *(u32 *)dst = *(u32 *)(arg2 + 0x90);
        *(u32 *)(dst + 4) = *(u32 *)(arg2 + 0x9C);
        *(u32 *)(dst + stride) = *(u32 *)(arg2 + 0x98);
        *(u32 *)(dst + stride + 4) = *(u32 *)(arg2 + 0x9C);
        *(u32 *)(dst + 2 * stride) = *(u32 *)(arg2 + 0x98);
        *(u32 *)(dst + 2 * stride + 4) = *(u32 *)(arg2 + 0x94);
        *(u32 *)(dst + 3 * stride) = *(u32 *)(arg2 + 0x90);
        *(u32 *)(dst + 3 * stride + 4) = *(u32 *)(arg2 + 0x94);
        dst += 4 * stride;
    }
}
