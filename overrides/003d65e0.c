/* func_003d65e0 (160 bytes): evaluates the animation arg0 into the record arg1. arg1[0] = arg2 and arg1[1] = the float arg4 (the time); the
 * words from arg1 + 8 are filled from the matrix arg3 by the type of the animation's table (the first word of (*arg0)[0]): the type at 0x0070B650 by
 * func_003d6c10(arg1 + 8, arg3) (a non-zero result returns 0 at once), the type at 0x0070B680 by func_003d6fb0(arg1 + 8, arg3). Always returns 0. */

extern s32 p4c_003d6c10_003d65e0(u32 *a, u32 *b) __asm__("func_003d6c10");
extern f32 *p4c_003d6fb0_003d65e0(f32 *a, f32 *b) __asm__("func_003d6fb0");

s32 func_003d65e0(u32 *arg0, u32 *arg1, u32 arg2, u32 *arg3, f32 arg4)
{
    arg1[0] = arg2;
    *(f32 *)(arg1 + 1) = arg4;
    if (((u32 *)arg0[0])[0] == *(u32 *)0x0070B650) {
        if (p4c_003d6c10_003d65e0(arg1 + 2, arg3) != 0) {
            return 0;
        }
    }
    if (((u32 *)arg0[0])[0] == *(u32 *)0x0070B680) {
        p4c_003d6fb0_003d65e0(arg1 + 2, arg3);
    }
    return 0;
}
