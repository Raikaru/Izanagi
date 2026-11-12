/* func_003bb3a0 (256 bytes): reads entry arg1 of the table arg0 into arg2. arg0+0xC is the kind and
 * arg0+8 the raw count (usable entries: kind 2 gives count - 3, kind 1 count - 2; any other kind
 * reports error {0x102, func_003df590(1, arg1)} and leaves 0 usable). The entries are 12-byte float
 * triples at arg0+0x14. For 0 <= arg1 < usable, the triple is copied to arg2 and arg2 returned.
 * Otherwise it reports error {0x102, func_003df590(0x80000003, 0x0070AF50)} through func_003df4d0
 * and returns 0. */

u8 *func_003bb3a0(u8 *arg0, s32 arg1, f32 *arg2)
{
    s32 kind;
    s32 usable;
    s32 err[2];

    if (arg1 >= 0) {
        kind = *(s32 *)(arg0 + 0xC);
        if (kind == 2) {
            usable = *(s32 *)(arg0 + 8) - 3;
        } else if (kind == 1) {
            usable = *(s32 *)(arg0 + 8) - 2;
        } else {
            err[0] = 0x102;
            err[1] = func_003df590(1, arg1);
            func_003df4d0(err);
            usable = 0;
        }
        if (arg1 < usable) {
            f32 *p = (f32 *)(*(u8 **)(arg0 + 0x14) + arg1 * 12);

            arg2[0] = p[0];
            arg2[1] = p[1];
            arg2[2] = p[2];
            return (u8 *)arg2;
        }
    }
    err[0] = 0x102;
    err[1] = func_003df590(0x80000003, 0x0070AF50);
    func_003df4d0(err);
    return (u8 *)0;
}
