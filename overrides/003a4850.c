/* func_003a4850 (152 bytes): fills arg1/16 quadwords (truncating toward zero) at arg0 with
 * the 16-byte pattern at arg2. The retail code fills scratchpad 0x70000000 with the pattern and
 * sends it to arg0 with the SPR DMA channel in chunks of 0x400 quadwords; only the destination
 * is changed. Bytes beyond the last whole quadword are not written. */

void func_003a4850(u8 *arg0, s32 arg1, u8 *arg2)
{
    s32 n;
    s32 i;

    n = arg1 / 16;
    for (i = 0; i < n; i++) {
        ((u32 *)arg0)[4 * i + 0] = ((u32 *)arg2)[0];
        ((u32 *)arg0)[4 * i + 1] = ((u32 *)arg2)[1];
        ((u32 *)arg0)[4 * i + 2] = ((u32 *)arg2)[2];
        ((u32 *)arg0)[4 * i + 3] = ((u32 *)arg2)[3];
    }
}
