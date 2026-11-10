/* func_003d7ac0 (100 bytes): writes a 0x40-byte block (two 0x20-byte halves at arg0 and arg0 + 0x20)
 * to the stream arg1 (func_003e2ab0 writes a block, func_003df240 a word-sized chunk; both return 0 on
 * failure). Returns arg0 on success and 0 when either write fails. */

extern s32 p4c_003df240_003d7ac0(s32 stream, void *p, s32 n) __asm__("func_003df240");
extern s32 p4c_003e2ab0_003d7ac0(s32 stream, void *p, s32 n) __asm__("func_003e2ab0");

u8 *func_003d7ac0(u8 *arg0, s32 arg1)
{
    if (p4c_003e2ab0_003d7ac0(arg1, arg0, 0x20) == 0) {
        return (u8 *)0;
    }
    if (p4c_003df240_003d7ac0(arg1, arg0 + 0x20, 0x20) == 0) {
        return (u8 *)0;
    }
    return arg0;
}
