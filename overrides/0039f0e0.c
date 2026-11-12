/* func_0039f0e0 (128 bytes): skin weights pre-step. arg0 is a pipeline object (+8 its record, +0x2C the
 * target), arg1 a pointer to a word. Returns 1.
 *
 * q = *(*(arg0+8) + 0x18). When bit 24 of q[8] is clear: v = *(q + *(u32 *)0x00886498) is read, func_003d0fa0(arg0, q) runs and
 * func_0039c000(arg0[0x2C], v, *arg1) follows. */

extern s32 p4c_0039c000_0039f0e0(s32 a, s32 b, s32 c) __asm__("func_0039c000");
extern s32 p4c_003d0fa0_0039f0e0(u8 *a, u8 *b) __asm__("func_003d0fa0");

s32 func_0039f0e0(u8 *arg0, s32 *arg1)
{
    u8 *q;
    s32 v;

    q = *(u8 **)(*(u8 **)(arg0 + 8) + 0x18);
    if (*(u32 *)(q + 8) & 0x01000000) {
        return 1;
    }
    v = *(s32 *)(q + *(s32 *)0x00886498);
    p4c_003d0fa0_0039f0e0(arg0, q);
    p4c_0039c000_0039f0e0(*(s32 *)(arg0 + 0x2C), v, *arg1);
    return 1;
}
