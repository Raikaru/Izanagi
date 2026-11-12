/* func_003a2340 (68 bytes): forwards to func_003a2390(arg0, func_003a20e0(arg1), arg2, 0x00708460) and
 * returns its result. */

extern s32 p4c_003a20e0_003a2340(s32 a) __asm__("func_003a20e0");
extern s32 p4c_003a2390_003a2340(s32 a, s32 b, s32 c, s32 d) __asm__("func_003a2390");

s32 func_003a2340(s32 arg0, s32 arg1, s32 arg2)
{
    s32 v;

    v = p4c_003a20e0_003a2340(arg1);
    return p4c_003a2390_003a2340(arg0, v, arg2, 0x00708460);
}
