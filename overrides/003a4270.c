/* func_003a4270 (212 bytes): fills memory at arg0 with the word arg2. The byte count arg1 is
 * split into q = arg1/16 quadwords (truncating toward zero), each four copies of the word, and a
 * remainder r = arg1 - 16*q; the retail code then stores r further words (a word per remaining
 * byte, not a byte per byte: that is how it is written) after the quadwords. The quadwords go
 * through scratchpad 0x70000000 and the SPR DMA channel in chunks of 0x400 quadwords; only the
 * destination is changed. */

void func_003a4270(u8 *arg0, s32 arg1, u32 arg2)
{
    s32 q;
    s32 r;
    s32 i;
    u32 *d;

    q = arg1 / 16;
    r = arg1 - q * 16;
    d = (u32 *)arg0;
    for (i = 0; i < q; i++) {
        d[0] = arg2;
        d[1] = arg2;
        d[2] = arg2;
        d[3] = arg2;
        d += 4;
    }
    for (i = 0; i < r; i++) {
        d[i] = arg2;
    }
}
