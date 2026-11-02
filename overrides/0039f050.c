/* func_0039f050 (144 bytes): runs the handler for the pipeline kind of the object arg0. q = the pointer in the word at (the pointer at (arg0[0x2C / 4] + 8)) + *(s32 *)0x007646D0
 * (gp - 0x4A20; gp is 0x007690F0), kind = q ? q[0x80 / 4] : 0; the handler is the word kind of the table at 0x0064F2B4. The data pointer
 * (arg0[1])[0x18 / 4] is advanced by kind * 16 while the handler runs (handler(arg0)) and restored afterwards. */

void func_0039f050(u8 *arg0)
{
    u8 *q = *(u8 **)(*(u8 **)(*(u8 **)(arg0 + 0x2C) + 8) + *(s32 *)0x007646D0);
    s32 kind = q != (u8 *)0 ? *(s32 *)(q + 0x80) : 0;
    u8 *d = *(u8 **)(arg0 + 4);
    u32 saved = *(u32 *)(d + 0x18);

    *(u32 *)(d + 0x18) = saved + kind * 16;
    (*(void (**)(u8 *))(0x0064F2B4 + kind * 4))(arg0);
    *(u32 *)(*(u8 **)(arg0 + 4) + 0x18) = saved;
}
