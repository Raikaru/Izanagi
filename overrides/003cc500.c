/* func_003cc500 (84 bytes): selects a mode in the 64-bit flag word at gp - 0x4710 (0x007649E0): bits 5 to 8 (0x1E0)
 * are cleared and replaced by 0x160 for arg0 == 6, 0xC0 for 5, 0x120 for 4, 0x80 for 3, 0x60 for 2 and 0 for anything else. */

void func_003cc500(s32 arg0)
{
    u64 v = *(u64 *)0x007649E0 & ~(u64)0x1E0;

    if (arg0 == 6) {
        v = v | 0x160;
    } else if (arg0 == 5) {
        v = v | 0xC0;
    } else if (arg0 == 4) {
        v = v | 0x120;
    } else if (arg0 == 3) {
        v = v | 0x80;
    } else if (arg0 == 2) {
        v = v | 0x60;
    }
    *(u64 *)0x007649E0 = v;
}
