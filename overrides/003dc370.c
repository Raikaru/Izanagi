/* func_003dc370 (288 bytes): reads a chunk list from the stream arg0 and returns the object that func_003ef0d0() created
 * for it, or 0 on failure. A 4-byte header (two halfwords: count a at +0, mode b at +2) is read with func_003e2910 (a short
 * read returns 0) and the object p = func_003ef0d0() (0 fails). With b = 0 each of the a entries is read by func_003db630(arg0, &p) (which may
 * replace p). With b > 0 the current context is saved (func_003ef6d0) and set to p (func_003ef6b0(p)); each of the a entries
 * must be read by func_003dbe40(arg0) (a failure restores the saved context and returns 0); the context is restored at the end.
 * Returns p. */

extern void p4c_003db630_003dc370(s32 s, u32 *p) __asm__("func_003db630");
extern s32 p4c_003dbe40_003dc370(s32 s) __asm__("func_003dbe40");
extern s32 p4c_003e2910_003dc370(s32 s, void *b, s32 n) __asm__("func_003e2910");
extern u32 p4c_003ef0d0_003dc370(void) __asm__("func_003ef0d0");
extern void p4c_003ef6b0_003dc370(u32 c) __asm__("func_003ef6b0");
extern u32 p4c_003ef6d0_003dc370(void) __asm__("func_003ef6d0");

u32 func_003dc370(s32 arg0)
{
    u16 hdr[2];
    u32 p;
    u32 saved;
    u32 i;

    if (p4c_003e2910_003dc370(arg0, hdr, 4) < 4) {
        return 0;
    }
    p = p4c_003ef0d0_003dc370();
    if (p == 0) {
        return 0;
    }
    if (hdr[1] > 0) {
        saved = p4c_003ef6d0_003dc370();
        p4c_003ef6b0_003dc370(p);
        for (i = 0; i < hdr[0]; i++) {
            if (p4c_003dbe40_003dc370(arg0) == 0) {
                p4c_003ef6b0_003dc370(saved);
                return 0;
            }
        }
        p4c_003ef6b0_003dc370(saved);
        return p;
    }
    for (i = 0; i < hdr[0]; i++) {
        p4c_003db630_003dc370(arg0, &p);
    }
    return p;
}
