/* func_003c9640 (136 bytes): runs the pipeline of an object that has a use count. arg0+0x84 (u16) is the
 * count; with 0 the object is returned untouched. Otherwise the pipeline is arg0[0x7C], else
 * (*(u32 *)0x008872E4)[0x6C], else *(0x008872E0 + *(s32 *)0x00764AA8 + 0x40) (gp-0x4648); it runs as
 * func_004114d0(pipeline, arg0, 1). Returns arg0, or 0 when that call returns 0. */

extern s32 p4c_004114d0_003c9640(s32 a, u8 *obj, s32 c) __asm__("func_004114d0");

u8 *func_003c9640(u8 *arg0)
{
    u8 *pipe;

    if (!(*(u16 *)(arg0 + 0x84) > 0)) {
        return arg0;
    }
    pipe = *(u8 **)(arg0 + 0x7C);
    if (pipe == (u8 *)0) {
        pipe = *(u8 **)(*(u8 **)0x008872E4 + 0x6C);
        if (pipe == (u8 *)0) {
            pipe = *(u8 **)(0x008872E0 + *(s32 *)0x00764AA8 + 0x40);
        }
    }
    if (p4c_004114d0_003c9640((s32)pipe, arg0, 1) == 0) {
        return (u8 *)0;
    }
    return arg0;
}
