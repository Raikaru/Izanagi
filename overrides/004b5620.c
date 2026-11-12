/* func_004b5620: when the retail callback at 0x00764C94 reports non-zero, runs the +0x14
 * callback of the 28-byte table row selected by obj+0x2c (table base: the pointer at
 * 0x00764C90) with obj. Sibling of func_004b55a0, without the counter increment. */
void func_004b5620(u8 *obj)
{
    void (*cb)(u8 *);

    if ((*(s32 (**)(void))0x00764C94)() != 0) {
        cb = *(void (**)(u8 *))(*(u8 **)0x00764C90 + 0x14 + *(s32 *)(obj + 0x2C) * 0x1C);
        if (cb != 0) {
            cb(obj);
        }
    }
}
