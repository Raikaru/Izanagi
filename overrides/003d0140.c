/* func_003d0140 (224 bytes): picks two state bytes for a draw record. arg0 is the record, whose
 * source flags word is **(arg0+0xC) (flags); arg1 is a mode. Writes arg0[0x29] and arg0[0x28] from
 * the byte at 0x00764A74 (gp-0x467C; gp is 0x007690F0), call it g:
 *
 *   flags & 1                      -> [0x29] = 4, [0x28] = g
 *   (flags & 0xFF) == 0, flags & 2 -> [0x29] = 3, [0x28] = g | 4
 *   (flags & 0x10) != 0            -> [0x29] = 4, [0x28] = g
 *   (flags & 8) != 0               -> [0x29] = 2, [0x28] = g | 0x10
 *   otherwise                      -> [0x29] = 1, [0x28] = g | 0x14
 *
 * then, unless arg1 == 2, bit 1 is set in [0x28]. */

void func_003d0140(u8 *arg0, s32 arg1)
{
    u32 flags;
    u8 g;

    flags = **(u32 **)(arg0 + 0xC);
    g = *(u8 *)0x00764A74;
    if ((flags & 1) == 1) {
        *(u8 *)(arg0 + 0x29) = 4;
        *(u8 *)(arg0 + 0x28) = g;
    } else if ((flags & 0xFF) == 0 || (flags & 2) == 2) {
        *(u8 *)(arg0 + 0x29) = 3;
        *(u8 *)(arg0 + 0x28) = g | 4;
    } else if ((flags & 0x10) == 0x10) {
        *(u8 *)(arg0 + 0x29) = 4;
        *(u8 *)(arg0 + 0x28) = g;
    } else if ((flags & 8) == 8) {
        *(u8 *)(arg0 + 0x29) = 2;
        *(u8 *)(arg0 + 0x28) = g | 0x10;
    } else {
        *(u8 *)(arg0 + 0x29) = 1;
        *(u8 *)(arg0 + 0x28) = g | 0x14;
    }
    if (arg1 != 2) {
        *(u8 *)(arg0 + 0x28) = *(u8 *)(arg0 + 0x28) | 2;
    }
}
