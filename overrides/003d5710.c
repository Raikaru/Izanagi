/* func_003d5710 (56 bytes): loads the object pointer at *arg0, loads the
 * function pointer at +0x28 of that object, calls it with arg0 unchanged,
 * and returns the call result + 20.
 *
 * Retail passes the same a0 (arg0) to the indirect call; the m2c draft
 * shows no argument, which the asm does not match. */

typedef s32 (*P4IndirectFn)(u8 **arg);

s32 func_003d5710(u8 **arg0)
{
    u8 *obj;
    P4IndirectFn fn;

    obj = *arg0;
    fn = *(P4IndirectFn *)(obj + 0x28);
    return fn(arg0) + 20;
}
