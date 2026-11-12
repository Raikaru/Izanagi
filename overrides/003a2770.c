/* func_003a2770 (432 bytes): fetches one two-word entry out of an object by selector mask.
 *
 * obj = *(arg0 + *(s32 *)0x00764700) (gp-0x49F0; gp is 0x007690F0). If (obj[0xA8] & arg2)
 * is zero: arg1[0] = arg1[1] = 0 and returns 0. Otherwise, when arg2 is exactly one of the
 * values below, arg1[0], arg1[1] receive the two words at obj + offset:
 *
 *   0x001 -> 0x54   0x002 -> 0x74   0x004 -> 0x6C   0x008 -> 0x5C   0x010 -> 0x64
 *   0x020 -> 0x84   0x040 -> 0x7C   0x080 -> 0x8C   0x100 -> 0x94
 *
 * (any other arg2 leaves arg1 as it is). Then if obj[0xA4] != 0, arg1[1] = obj[0xA4].
 * If arg1[0] != 0 and (arg3 & 0x40000000) == 0x40000000, obj[0x3C] |= arg3 | arg2
 * (obj re-read from the same global). Returns 1. */

s32 func_003a2770(u8 *arg0, u32 *arg1, u32 arg2, u32 arg3)
{
    u8 *obj;
    s32 off;

    obj = *(u8 **)(arg0 + *(s32 *)0x00764700);
    if ((*(u32 *)(obj + 0xA8) & arg2) == 0) {
        arg1[0] = 0;
        arg1[1] = 0;
        return 0;
    }
    switch (arg2) {
    case 0x001: off = 0x54; break;
    case 0x002: off = 0x74; break;
    case 0x004: off = 0x6C; break;
    case 0x008: off = 0x5C; break;
    case 0x010: off = 0x64; break;
    case 0x020: off = 0x84; break;
    case 0x040: off = 0x7C; break;
    case 0x080: off = 0x8C; break;
    case 0x100: off = 0x94; break;
    default: off = 0; break;
    }
    if (off != 0) {
        u32 w0 = *(u32 *)(obj + off);
        u32 w1 = *(u32 *)(obj + off + 4);

        arg1[0] = w0;
        arg1[1] = w1;
    }
    if (*(u32 *)(obj + 0xA4) != 0) {
        arg1[1] = *(u32 *)(obj + 0xA4);
    }
    if (arg1[0] != 0 && (arg3 & 0x40000000) == 0x40000000) {
        obj = *(u8 **)(arg0 + *(s32 *)0x00764700);
        *(u32 *)(obj + 0x3C) |= arg3 | arg2;
    }
    return 1;
}
