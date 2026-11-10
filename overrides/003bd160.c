/* func_003bd160: runs fourteen plugin registration calls in order (the
 * arguments are retail callback functions). Each result is kept in a
 * retail gp global at 0x007690F0 - N (N = 18856 down to 18804). Any negative
 * result returns 0. The final result is (last call >= 0).
 *
 * The callbacks are passed by symbol: RenderWare calls them on the host, so a raw PS2 address
 * would jump into PS2 RAM. Each p4cb_ name is the address of the retail function's symbol
 * (a native, or a trap stub while the function is asm-only). They are the user-data plugin's
 * engine ctor/dtor (0x003BBE60, 0x003BBE80), UserDataObjectConstruct (0x003BCBC0), its
 * destructor, copy and stream callbacks (0x003BCBE0, 0x003BCC80, 0x003BCD50) and
 * UserDataObjectStreamWrite (0x003BCE20) with its size callback (0x003BCE50). */

extern u8 p4cb_003bbe60[] __asm__("func_003bbe60");
extern u8 p4cb_003bbe80[] __asm__("func_003bbe80");
extern u8 p4cb_003bcbc0[] __asm__("func_003bcbc0");
extern u8 p4cb_003bcbe0[] __asm__("func_003bcbe0");
extern u8 p4cb_003bcc80[] __asm__("func_003bcc80");
extern u8 p4cb_003bcd50[] __asm__("func_003bcd50");
extern u8 p4cb_003bce20[] __asm__("func_003bce20");
extern u8 p4cb_003bce50[] __asm__("func_003bce50");

s32 func_003bd160(void)
{
    s32 r;

    r = func_003e8930(0, 287, p4cb_003bbe60, p4cb_003bbe80);
    if (r < 0) {
        return 0;
    }

    r = func_003c2b70(8, 287, p4cb_003bcbc0, p4cb_003bcbe0, p4cb_003bcc80);
    *(s32 *)(0x007690F0 - 18856) = r;
    if (r < 0) {
        return 0;
    }

    r = func_003c2ba0(287, p4cb_003bcd50, p4cb_003bce20, p4cb_003bce50);
    *(s32 *)(0x007690F0 - 18852) = r;
    if (r < 0) {
        return 0;
    }

    r = func_003c8d00(8, 287, p4cb_003bcbc0, p4cb_003bcbe0, p4cb_003bcc80);
    *(s32 *)(0x007690F0 - 18848) = r;
    if (r < 0) {
        return 0;
    }

    r = func_003c8d30(287, p4cb_003bcd50, p4cb_003bce20, p4cb_003bce50);
    *(s32 *)(0x007690F0 - 18844) = r;
    if (r < 0) {
        return 0;
    }

    r = func_003e9eb0(8, 287, p4cb_003bcbc0, p4cb_003bcbe0, p4cb_003bcc80);
    *(s32 *)(0x007690F0 - 18840) = r;
    if (r < 0) {
        return 0;
    }

    r = func_003e5520(287, p4cb_003bcd50, p4cb_003bce20, p4cb_003bce50);
    *(s32 *)(0x007690F0 - 18836) = r;
    if (r < 0) {
        return 0;
    }

    r = func_003e8410(8, 287, p4cb_003bcbc0, p4cb_003bcbe0, p4cb_003bcc80);
    *(s32 *)(0x007690F0 - 18832) = r;
    if (r < 0) {
        return 0;
    }

    r = func_003e5220(287, p4cb_003bcd50, p4cb_003bce20, p4cb_003bce50);
    *(s32 *)(0x007690F0 - 18828) = r;
    if (r < 0) {
        return 0;
    }

    r = func_003c3920(8, 287, p4cb_003bcbc0, p4cb_003bcbe0, p4cb_003bcc80);
    *(s32 *)(0x007690F0 - 18824) = r;
    if (r < 0) {
        return 0;
    }

    r = func_003c3950(287, p4cb_003bcd50, p4cb_003bce20, p4cb_003bce50);
    *(s32 *)(0x007690F0 - 18820) = r;
    if (r < 0) {
        return 0;
    }

    r = func_003c4310(8, 287, p4cb_003bcbc0, p4cb_003bcbe0, p4cb_003bcc80);
    *(s32 *)(0x007690F0 - 18816) = r;
    if (r < 0) {
        return 0;
    }

    r = func_003c4340(287, p4cb_003bcd50, p4cb_003bce20, p4cb_003bce50);
    *(s32 *)(0x007690F0 - 18812) = r;
    if (r < 0) {
        return 0;
    }

    r = func_003ef890(8, 287, p4cb_003bcbc0, p4cb_003bcbe0, p4cb_003bcc80);
    *(s32 *)(0x007690F0 - 18808) = r;
    if (r < 0) {
        return 0;
    }

    r = func_003e6210(287, p4cb_003bcd50, p4cb_003bce20, p4cb_003bce50);
    *(s32 *)(0x007690F0 - 18804) = r;
    return r >= 0;
}
