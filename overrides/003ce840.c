/* func_003ce840 (412 bytes): size, in bytes, of a block that is serialized for arg0.
 * Returns 0 when bit 24 (0x01000000) of the word at arg0+8 is clear. Otherwise the size
 * starts at 0x10 and, for each of the n entries of the table whose pointer is at
 * arg0 + *(s32 *)0x00764820 (gp-0x48D0; gp is 0x007690F0; n is the table's first word,
 * entries are the words after it), adds 8 + obj[8] - (obj[0x20] - obj). Differences are
 * 32-bit wrapping. The retail code unrolls the loop by eight with a scalar tail; the sum
 * is the same for every n. Leaf function. */

u32 func_003ce840(u8 *arg0)
{
    u32 size;
    u32 *table;
    u32 n;
    u32 i;

    if (!(*(u32 *)(arg0 + 8) & 0x01000000)) {
        return 0;
    }
    size = 0x10;
    table = *(u32 **)(arg0 + *(s32 *)0x00764820);
    n = table[0];
    for (i = 1; i <= n; i++) {
        u32 obj = table[i];

        size += 8 + (*(u32 *)(obj + 8) - (*(u32 *)(obj + 0x20) - obj));
    }
    return size;
}
