/* func_003ce2e0 (180 bytes): releases the entries of an object's mesh cache table. The slot is at
 * arg0 + *(s32 *)0x00764820 (gp-0x48D0; gp is 0x007690F0). When its table pointer is set and the first word of
 * the table equals the halfword at (*(arg0+0x54))+4 the table is used as is, otherwise func_003ce050(slot,
 * that halfword) returns the slot that now holds the table. For each of the table's n entries (n = first
 * word, entries from +4) a non-zero entry goes to func_003e1ea0. Returns 1. */

extern s32 p4c_003ce050_003ce2e0(void *slot, s32 n) __asm__("func_003ce050");
extern void p4c_003e1ea0_003ce2e0(u32 p) __asm__("func_003e1ea0");

s32 func_003ce2e0(u8 *arg0)
{
    u8 *slot;
    u8 *table;
    u32 n;
    u32 i;
    s32 found;

    slot = arg0 + *(s32 *)0x00764820;
    found = 0;
    if (*(u32 *)slot != 0 && *(u16 *)(*(u8 **)(arg0 + 0x54) + 4) == **(u32 **)slot) {
        found = 1;
    }
    if (found) {
        table = *(u8 **)slot;
    } else {
        u8 *r = (u8 *)p4c_003ce050_003ce2e0(slot, *(u16 *)(*(u8 **)(arg0 + 0x54) + 4));

        table = *(u8 **)r;
    }
    n = *(u32 *)table;
    for (i = 0; i < n; i++) {
        table += 4;
        if (*(u32 *)table != 0) {
            p4c_003e1ea0_003ce2e0(*(u32 *)table);
        }
    }
    return 1;
}
