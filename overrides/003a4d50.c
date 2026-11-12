/* func_003a4d50 (268 bytes): block copy. arg0 destination, arg1 source, arg2 byte count.
 * The retail code copies arg2/16 quadwords (the quotient truncates toward zero) through the
 * scratchpad with the SPR DMA channels, in chunks of 0x400 quadwords, then copies the remaining
 * arg2 - 16*quadwords bytes with memcpy (the port's func_0043f810). The DMA path has no effect other than the copy (it
 * only bounces through scratchpad 0x70000000), so the copy is done directly. A count of zero or
 * less copies nothing. */

extern void *p4_memcpy(void *dst, const void *src, u32 n) __asm__("func_0043f810");

void func_003a4d50(u8 *arg0, u8 *arg1, s32 arg2)
{
    if (arg2 > 0) {
        p4_memcpy(arg0, arg1, (u32)arg2);
    }
}
