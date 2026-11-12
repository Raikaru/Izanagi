/* func_003d72a0 (160 bytes): releases the animation slots of the object arg0 and returns arg0. The slot block is at arg0 + the
 * offset word at gp - 0x48B4 (0x0076483C): its first two words are released with func_003e0f40 and cleared, then each of the eight words from
 * +8 on that is set gets func_003d7c50(first word of it) and func_003d5830(it) (the slot itself is not cleared). */

extern void p4c_003d5830_003d72a0(u32 o) __asm__("func_003d5830");
extern void p4c_003d7c50_003d72a0(u32 o) __asm__("func_003d7c50");
extern void p4c_003e0f40_003d72a0(u32 o) __asm__("func_003e0f40");

u8 *func_003d72a0(u8 *arg0)
{
    u32 *blk = (u32 *)(arg0 + *(s32 *)0x0076483C);
    s32 i;

    for (i = 0; i < 2; i++) {
        if (blk[i] != 0) {
            p4c_003e0f40_003d72a0(blk[i]);
        }
        blk[i] = 0;
    }
    for (i = 0; i < 8; i++) {
        u32 *s = (u32 *)blk[2 + i];

        if (s != (u32 *)0) {
            p4c_003d7c50_003d72a0(s[0]);
            p4c_003d5830_003d72a0((u32)s);
        }
    }
    return arg0;
}
