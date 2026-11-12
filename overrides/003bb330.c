/* func_003bb330 (112 bytes): number of usable entries of a table object. arg0+0xC is the kind
 * and arg0+8 the raw count: kind 2 gives count - 3, kind 1 gives count - 2. Any other kind reports
 * error {0x102, func_003df590(1)} through func_003df4d0 and returns 0. */

s32 func_003bb330(u8 *arg0)
{
    s32 kind;
    s32 err[2];

    kind = *(s32 *)(arg0 + 0xC);
    if (kind == 2) {
        return *(s32 *)(arg0 + 8) - 3;
    }
    if (kind == 1) {
        return *(s32 *)(arg0 + 8) - 2;
    }
    err[0] = 0x102;
    err[1] = func_003df590(1);
    func_003df4d0(err);
    return 0;
}
