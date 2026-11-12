/* func_003dd290 (252 bytes): initializes a device object. arg0 is the object, arg1 the slot count, arg2
 * the name and arg3 a mode. Returns 1 on success, 2 when func_003dd7d0() fails (device layer not up), 6
 * when func_003dd990(arg2) finds the name already in use, or the result of func_003dd620(arg0, arg3)
 * when that is not 1.
 *
 * The name is copied to arg0 + 0x50 with the function at 0x008873AC, +4 = arg1, +0x10 = 0, and for each
 * slot i the object's function at +0x14 is called as f(arg0, i); the returned slot gets +0x38 = 1 and
 * +0x30 = arg0[0x10]. Then +0xC = 0 and func_003dd620 runs; on 1 the state +8 = 2 and +0x20, +0x24 are
 * cleared. */

extern s32 p4c_003dd620_003dd290(u8 *d, s32 m) __asm__("func_003dd620");
extern s32 p4c_003dd7d0_003dd290(void) __asm__("func_003dd7d0");
extern s32 p4c_003dd990_003dd290(u8 *n) __asm__("func_003dd990");

typedef void (*P4NameCopyFn_003dd290)(u8 *dst, u8 *src);
typedef u8 *(*P4SlotFn_003dd290)(u8 *dev, s32 index);

s32 func_003dd290(u8 *arg0, s32 arg1, u8 *arg2, s32 arg3)
{
    s32 i;
    s32 r;

    if (p4c_003dd7d0_003dd290() == 0) {
        return 2;
    }
    if (p4c_003dd990_003dd290(arg2) != 0) {
        return 6;
    }
    (*(P4NameCopyFn_003dd290 *)0x008873AC)(arg0 + 0x50, arg2);
    *(s32 *)(arg0 + 4) = arg1;
    *(u32 *)(arg0 + 0x10) = 0;
    for (i = 0; i < arg1; i++) {
        u8 *slot = (*(P4SlotFn_003dd290 *)(arg0 + 0x14))(arg0, i);

        *(u32 *)(slot + 0x38) = 1;
        *(u32 *)(slot + 0x30) = *(u32 *)(arg0 + 0x10);
    }
    *(u32 *)(arg0 + 0xC) = 0;
    r = p4c_003dd620_003dd290(arg0, arg3);
    if (r == 1) {
        *(u32 *)(arg0 + 8) = 2;
        *(u32 *)(arg0 + 0x20) = 0;
        *(u32 *)(arg0 + 0x24) = 0;
    }
    return r;
}
