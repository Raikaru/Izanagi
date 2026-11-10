/* func_0039a910 (280 bytes): selects a byte from a 6x6 table and stores it, with 0x80 in the
 * high word, into a 64-bit slot.
 *
 * rec = *(arg0 + *(s32 *)0x007646D0) (gp-0x4A20; gp is 0x007690F0). base = rec when
 * *(rec+0x80) == 4, else rec + 0x40. Two codes at base+4 and base+8 are mapped through the
 * retail jump tables at 0x00752CD0 and 0x00752CA0 (both: 1 -> 0, 2 -> 1, 5 -> 2, 6 -> 3,
 * 7 -> 4, any other value -> 5). With a = map(base[4]) and b = map(base[8]), the 64-bit value
 * (0x80 << 32) | table[a + 6 * b], table at 0x0064F280 (bytes), is stored at base+0x28.
 * Returns nothing. */

static u32 p4_func_0039a910_map(u32 code)
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
    default:
        return 5;
    }
}

void func_0039a910(u8 *arg0)
{
    u8 *rec;
    u8 *base;
    u32 a;
    u32 b;

    rec = *(u8 **)(arg0 + *(s32 *)0x007646D0);
    base = rec;
    if (*(s32 *)(rec + 0x80) != 4) {
        base = rec + 0x40;
    }
    a = p4_func_0039a910_map(*(u32 *)(base + 4));
    b = p4_func_0039a910_map(*(u32 *)(base + 8));
    *(u32 *)(base + 0x28) = ((u8 *)0x0064F280)[a + 6 * b];
    *(u32 *)(base + 0x2C) = 0x80;
}
