/* func_00390290 (52 bytes): arg0 + arg1 addresses a 16-byte slot whose first word is a pointer. When the
 * pointer is set, rpCollTreeStreamGetSize(pointer) runs and its result plus 0x10 is returned; otherwise 0. */

extern u8 *p4c_0038fb20_00390290(u8 *p) __asm__("rpCollTreeStreamGetSize");

u8 *func_00390290(u8 *arg0, s32 arg1)
{
    u8 *slot;

    slot = arg0 + arg1;
    if (*(u32 *)slot != 0) {
        return p4c_0038fb20_00390290(*(u8 **)slot) + 0x10;
    }
    return (u8 *)0;
}
