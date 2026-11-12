/* func_004b55a0: when the retail callback at 0x00764C94 reports non-zero, runs the +0x10
 * callback of the 28-byte table row selected by obj+0x2c (the table base is the pointer at
 * 0x00764C90) with obj, then increments obj+0x28. */
void func_004b55a0(u8 *obj)
{
    void (*cb)(u8 *);

    if ((*(s32 (**)(void))0x00764C94)() != 0) {
        cb = *(void (**)(u8 *))(*(u8 **)0x00764C90 + 0x10 + *(s32 *)(obj + 0x2C) * 0x1C);
        if (cb != 0) {
            cb(obj);
        }
        *(s32 *)(obj + 0x28) = *(s32 *)(obj + 0x28) + 1;
    }
}
