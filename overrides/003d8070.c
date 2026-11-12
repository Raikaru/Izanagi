/* func_003d8070 (192 bytes): creates the interpolator for animation arg1 in slot arg2 of the object arg0's table
 * (the table is at arg0 + the offset word at 0x0076483C, slot words from +8) and returns arg0, or 0 on failure.
 * The interpolator is func_003d5790(func_003d5750(arg1), (*(u32 *)arg1)[1]) (0 fails), set up with func_003d5840(it,
 * arg1), and registered with func_003d7490(table) (0 frees it again with func_003d5830(it) and fails). On success the
 * counter at arg1[5] + 0x40 is incremented. */

extern s32 p4c_003d5750_003d8070(u8 *a) __asm__("func_003d5750");
extern u32 p4c_003d5790_003d8070(s32 n, u32 s) __asm__("func_003d5790");
extern void p4c_003d5830_003d8070(u32 it) __asm__("func_003d5830");
extern void p4c_003d5840_003d8070(u32 it, u8 *a) __asm__("func_003d5840");
extern s32 p4c_003d7490_003d8070(u8 *t) __asm__("func_003d7490");

u8 *func_003d8070(u8 *arg0, u8 *arg1, s32 arg2)
{
    u8 *table = arg0 + *(s32 *)0x0076483C;
    u32 *slot = (u32 *)(table + arg2 * 4 + 8);
    s32 n;

    n = p4c_003d5750_003d8070(arg1);
    *slot = p4c_003d5790_003d8070(n, ((u32 *)*(u32 *)arg1)[1]);
    if (*slot == 0) {
        return (u8 *)0;
    }
    p4c_003d5840_003d8070(*slot, arg1);
    if (p4c_003d7490_003d8070(table) == 0) {
        p4c_003d5830_003d8070(*slot);
        return (u8 *)0;
    }
    *(s32 *)(*(u8 **)(arg1 + 0x14) + 0x40) += 1;
    return arg0;
}
