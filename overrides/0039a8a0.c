/* func_0039a8a0 (112 bytes): p = the pointer in the word at arg0 + *(s32 *)0x007646D0 (gp - 0x4A20; gp is 0x007690F0). When p[0x80 / 4] is 3 the 64 bit word at
 * p + 0x68 is set from the flag p[0x4C / 4], else the one at p + 0x28 from the flag p[0xC / 4]: 0x0000008000000068 when the flag is 0, otherwise 0x58. */

void func_0039a8a0(u8 *arg0)
{
    u8 *p = *(u8 **)(arg0 + *(s32 *)0x007646D0);
    s32 flag;

    if (*(s32 *)(p + 0x80) == 3) {
        flag = *(s32 *)(p + 0x4C);
        *(u64 *)(p + 0x68) = flag == 0 ? 0x0000008000000068ULL : 0x58ULL;
    } else {
        flag = *(s32 *)(p + 0xC);
        *(u64 *)(p + 0x28) = flag == 0 ? 0x0000008000000068ULL : 0x58ULL;
    }
}
