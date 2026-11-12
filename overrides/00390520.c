/* func_00390520: runs six setup calls in order (the arguments are retail
 * callback functions). Calls 1 and 2 must return > 0, calls 3, 4 and 5
 * must return >= 0, and call 6's result (>= 0) is the return value. Any
 * earlier failure returns 0. Results of calls 1, 2, 3 and 5 are kept in the
 * retail gp globals at 0x007646B4, 0x007646B8, 0x007646C0 and 0x007646BC.
 *
 * The callbacks are passed by name: RenderWare calls them on the host, so a raw PS2 address
 * would jump into PS2 RAM. func_00390290 and func_00390350 are asm-only (trap stubs). */

extern s32 func_00390290();
extern s32 func_00390350();

s32 func_00390520(void)
{
    s32 *g_r1 = (s32 *)0x007646B4;
    s32 *g_r2 = (s32 *)0x007646B8;
    s32 *g_r3 = (s32 *)0x007646C0;
    s32 *g_r5 = (s32 *)0x007646BC;
    s32 r;

    *g_r1 = func_003e8930(4, 285, (u8 *)func_003901e0, (u8 *)func_00390210);
    if (*g_r1 <= 0) {
        return 0;
    }

    *g_r2 = func_003c1ab0(4, 285, (u8 *)func_00390500, 0, 0);
    if (*g_r2 <= 0) {
        return 0;
    }

    *g_r3 = func_003c8d00(4, 285, (u8 *)func_00390280, (u8 *)func_00390230, 0);
    if (*g_r3 < 0) {
        return 0;
    }

    r = func_003c8d30(285, (u8 *)func_00390350, (u8 *)func_003902d0, (u8 *)func_00390290);
    if (r < 0) {
        return 0;
    }

    *g_r5 = func_003c2b70(4, 285, (u8 *)func_00390280, (u8 *)func_00390230, 0);
    if (*g_r5 < 0) {
        return 0;
    }

    r = func_003c2ba0(285, (u8 *)func_00390350, (u8 *)func_003902d0, (u8 *)func_00390290);
    return r >= 0;
}
