/* func_003bce50 (192 bytes): sums func_003bc620 over the used entries of a list. arg0 + arg1 is the list
 * header (+0 count, +4 entries of 16 bytes). Returns 0 for a null header or a count of 0 or less, otherwise
 * the sum of func_003bc620(entry) over the entries whose word +4 is non-zero, plus 4 when that sum is above 0. */

extern s32 p4c_003bc620_003bce50(u8 *e) __asm__("func_003bc620");

s32 func_003bce50(u8 *arg0, s32 arg1)
{
    u8 *hdr;
    s32 total;
    s32 i;

    hdr = arg0 + arg1;
    total = 0;
    if (hdr == (u8 *)0) {
        return 0;
    }
    if (*(s32 *)hdr <= 0) {
        return 0;
    }
    for (i = 0; i < *(s32 *)hdr; i++) {
        u8 *e = *(u8 **)(hdr + 4) + 16 * i;

        if (*(u32 *)(e + 4) != 0) {
            total += p4c_003bc620_003bce50(e);
        }
    }
    if (total > 0) {
        total += 4;
    }
    return total;
}
