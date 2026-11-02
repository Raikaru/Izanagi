/* func_003b4120 (264 bytes): like func_003b6540 (four pairs from arg2+0x90..0xAC) with a block
 * fast path. arg0: +0 destination base, +4 slot stride S; arg1 is the record count. When
 * S == 8 the eight words at arg2+0x90 are copied to a 32-byte buffer and
 * func_003a4ab0(dst, 4*S*n, buffer) fills the block (called whatever n is). Otherwise, for each
 * of arg1 > 0 records, pair k (words arg2+0x90+8k and +0x94+8k) goes to dst + k*S and dst
 * advances by 4*S. */

void func_003b4120(u8 *arg0, s32 arg1, u8 *arg2)
{
    u8 *dst;
    s32 stride;
    s32 i;
    s32 k;

    stride = *(s32 *)(arg0 + 4);
    if (stride == 8) {
        u32 pattern[8];

        for (k = 0; k < 8; k++) {
            pattern[k] = *(u32 *)(arg2 + 0x90 + 4 * k);
        }
        func_003a4ab0(*(u8 **)arg0, *(s32 *)(arg0 + 4) * arg1 * 4, (u8 *)pattern);
        return;
    }
    dst = *(u8 **)arg0;
    for (i = 0; i < arg1; i++) {
        for (k = 0; k < 4; k++) {
            *(u32 *)(dst + k * stride) = *(u32 *)(arg2 + 0x90 + 8 * k);
            *(u32 *)(dst + k * stride + 4) = *(u32 *)(arg2 + 0x94 + 8 * k);
        }
        dst += 4 * stride;
    }
}
