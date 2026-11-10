/* func_001f5680 (76 bytes): reads the object pointer stored at retail
 * 0x0076449C (gp-0x4C54), then the object pointer at +0xC68 of that. If it
 * is null, returns -1. Otherwise reads the u16 at +2: values 1..5 return
 * value + 0x11, and 0 or values >= 6 return -1.
 *
 * gp is 0x007690F0 (startup code at 0x0001C0 sets gp from a0 =
 * 0x770000 - 0x6F10). gp-19540 resolves to the absolute slot 0x0076449C.
 * Leaf function: no calls, no stack frame. */

s32 func_001f5680(void)
{
    u8 *obj;
    u16 val;

    obj = *(u8 **)0x0076449C;
    obj = *(u8 **)(obj + 0xC68);
    if (obj == 0) {
        return -1;
    }
    val = *(u16 *)(obj + 2);
    if (val >= 6 || val == 0) {
        return -1;
    }
    return (s32)val + 0x11;
}
