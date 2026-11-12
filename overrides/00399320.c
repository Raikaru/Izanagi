/* func_00399320 (32 bytes): returns 4 when the word at arg0 + *(s32 *)0x007646D4 (gp - 0x4A1C; gp is 0x007690F0) is non-zero, else 0. */

s32 func_00399320(u8 *arg0)
{
    s32 r = 0;

    if (*(s32 *)(arg0 + *(s32 *)0x007646D4) != 0) {
        r = 4;
    }
    return r;
}
