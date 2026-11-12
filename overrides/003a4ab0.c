/* func_003a4ab0 (168 bytes): fills arg1/16 quadwords (truncating toward zero) at arg0 with the
 * 32-byte pattern at arg2: quadword k receives pattern quadword (k & 1). The retail code builds
 * the pattern in scratchpad 0x70000000 and sends it to arg0 with the SPR DMA channel in chunks
 * of 0x400 quadwords; only the destination is changed. Bytes beyond the last whole quadword
 * are not written. */

void func_003a4ab0(u8 *arg0, s32 arg1, u8 *arg2)
{
    s32 n;
    s32 i;

    n = arg1 / 16;
    for (i = 0; i < n; i++) {
        u32 *d = (u32 *)arg0 + 4 * i;
        u32 *p = (u32 *)arg2 + 4 * (i & 1);

        d[0] = p[0];
        d[1] = p[1];
        d[2] = p[2];
        d[3] = p[3];
    }
}
