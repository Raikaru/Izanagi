/* func_003a2bb0 (304 bytes): looks up a 64-bit state word from the table at 0x007083D0 and stores it in
 * the target object. arg0 holds the selectors (+0x5C and +0x60, small codes) and the target (+0x4C).
 * Returns 1, or 0 when the table entry is 0xFFFFFFFF.
 *
 * Both selectors are mapped 1 -> 0, 2 -> 1, 5 -> 2, 6 -> 3, 7 -> 4, 8 -> 5; any other value gives 2 for the
 * first (column) and 3 for the second (row). The entry is the word at 0x007083D0 + 24 * row + 4 * column;
 * when column and row are both 1 it gets 0x80 in its upper word. The result is stored as a 64-bit value at
 * target + 0xB8. */

static s32 p4_2bb0_map(u32 code, s32 other)
{
    switch (code) {
    case 1:
        return 0;
    case 2:
        return 1;
    case 5:
        return 2;
    case 6:
        return 3;
    case 7:
        return 4;
    case 8:
        return 5;
    default:
        return other;
    }
}

s32 func_003a2bb0(u8 *arg0)
{
    u8 *target;
    s32 col;
    s32 row;
    u32 w;
    u64 v;

    target = *(u8 **)(arg0 + 0x4C);
    col = p4_2bb0_map(*(u32 *)(arg0 + 0x5C), 2);
    row = p4_2bb0_map(*(u32 *)(arg0 + 0x60), 3);
    w = *(u32 *)(0x007083D0 + 24 * row + 4 * col);
    if (w == 0xFFFFFFFFu) {
        return 0;
    }
    v = (u64)w;
    if (col == 1 && row == 1) {
        v |= (u64)0x80 << 32;
    }
    *(u64 *)(target + 0xB8) = v;
    return 1;
}
