/* func_003999a0: runs seven setup calls in order (the arguments are retail
 * callback functions). Every call must return >= 0; any failure returns 0.
 * The result is (call 7 >= 0). Results of calls 2, 4 and 6 are kept in the
 * retail gp globals at 0x007646D0, 0x007646D4 and 0x007646D8.
 *
 * The callbacks are passed by symbol: RenderWare calls them on the host, so a raw PS2 address
 * would jump into PS2 RAM. Each p4cb_ name is the address of the retail function's symbol
 * (a native, or a trap stub while the function is asm-only). */

extern u8 p4cb_00398350[] __asm__("func_00398350");
extern u8 p4cb_003982e0[] __asm__("func_003982e0");
extern u8 p4cb_003983f0[] __asm__("func_003983f0");
extern u8 p4cb_00398540[] __asm__("func_00398540");
extern u8 p4cb_003985b0[] __asm__("func_003985b0");
extern u8 p4cb_00398d20[] __asm__("func_00398d20");
extern u8 p4cb_00398970[] __asm__("func_00398970");
extern u8 p4cb_003990a0[] __asm__("func_003990a0");
extern u8 p4cb_00399210[] __asm__("func_00399210");
extern u8 p4cb_00399230[] __asm__("func_00399230");
extern u8 p4cb_00399250[] __asm__("func_00399250");
extern u8 p4cb_003992b0[] __asm__("func_003992b0");
extern u8 p4cb_00399280[] __asm__("func_00399280");
extern u8 p4cb_00399320[] __asm__("func_00399320");
extern u8 p4cb_00399340[] __asm__("func_00399340");
extern u8 p4cb_00399360[] __asm__("func_00399360");
extern u8 p4cb_00399380[] __asm__("func_00399380");
extern u8 p4cb_003993e0[] __asm__("func_003993e0");
extern u8 p4cb_003993b0[] __asm__("func_003993b0");
extern u8 p4cb_00399450[] __asm__("func_00399450");

s32 func_003999a0(void)
{
    s32 *g_r2 = (s32 *)0x007646D0;
    s32 *g_r4 = (s32 *)0x007646D4;
    s32 *g_r6 = (s32 *)0x007646D8;
    s32 r;

    r = func_003e8930(0, 288, p4cb_00398350, p4cb_003982e0);
    if (r < 0) {
        return 0;
    }

    *g_r2 = func_003c4310(4, 288, p4cb_003983f0, p4cb_00398540, p4cb_003985b0);
    if (*g_r2 < 0) {
        return 0;
    }

    r = func_003c4340(288, p4cb_00398d20, p4cb_00398970, p4cb_003990a0);
    if (r < 0) {
        return 0;
    }

    *g_r4 = func_003c1ab0(4, 288, p4cb_00399210, p4cb_00399230, p4cb_00399250);
    if (*g_r4 < 0) {
        return 0;
    }

    r = func_003c1b10(288, p4cb_003992b0, p4cb_00399280, p4cb_00399320);
    if (r < 0) {
        return 0;
    }

    *g_r6 = func_003c8d00(4, 288, p4cb_00399340, p4cb_00399360, p4cb_00399380);
    if (*g_r6 < 0) {
        return 0;
    }

    r = func_003c8d30(288, p4cb_003993e0, p4cb_003993b0, p4cb_00399450);
    return r >= 0;
}
