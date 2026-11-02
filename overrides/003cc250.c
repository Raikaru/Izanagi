/* func_003cc250 (104 bytes): arg1 points to an object pointer. If the u16
 * at the object's +0 is 0, returns 0. Otherwise copies the word at +4 into
 * +24, and if the word at +12 has bit 0 set, returns 1. If bit 0 is clear,
 * calls the function pointer stored at retail 0x00887300 with (1, 0), then
 * returns 1 whatever the callee returned. arg0 is unused.
 *
 * Written as a direct transliteration of the retail code. */

typedef s32 (*P4Call2Fn)(s32 a, s32 b);

s32 func_003cc250(s32 arg0, u8 **arg1)
{
    u8 *obj;
    P4Call2Fn fn;

    (void)arg0;
    obj = *arg1;
    if (*(u16 *)obj == 0) {
        return 0;
    }
    *(u32 *)(obj + 24) = *(u32 *)(obj + 4);
    if (*(u32 *)(obj + 12) & 1) {
        return 1;
    }
    fn = *(P4Call2Fn *)0x00887300;
    fn(1, 0);
    return 1;
}
