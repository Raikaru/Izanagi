/* Three thunks that retail keeps in one block (func_003ca740, func_003ca780, func_003ca7a0); the plugin offset is the word
 * at gp - 0x48F8 (0x007647F8) and each thunk works on the record at arg0 + offset.
 *
 * func_003ca740 (60 bytes): stores the record's word +0xC in the global at 0x008872E4, increments the halfword at 0x008872E8
 * and tail-calls the function at record + 0x10 with arg0 (retail clobbers a1 to a3 first; its result is returned).
 * func_003ca780 (32 bytes): clears the global at 0x008872E4 and tail-calls the function at record + 0x14 with the same arguments.
 * func_003ca7a0 (140 bytes): calls the function at record + 0x18 with arg0; 0 returns 0. Otherwise, with the object e = record word +0xC:
 * a non-zero e is passed to func_003cb300(e, arg0) and the function in the word at 0x00887380 is called as f(0, 0, e[3] == 1);
 * arg0 is returned. */

extern void p4c_003cb300_003ca740(u8 *e, u32 o) __asm__("func_003cb300");

typedef s32 (*P4ThunkFn_003ca740)(u32 a0, u32 a1, u32 a2, u32 a3);
typedef void (*P4CallFn_003ca740)(u32 a0, u32 a1, u32 a2);

s32 func_003ca740(u32 arg0)
{
    u8 *rec = (u8 *)arg0 + *(s32 *)0x007647F8;
    u16 count = *(u16 *)0x008872E8;

    *(u32 *)0x008872E4 = *(u32 *)(rec + 0xC);
    *(u16 *)0x008872E8 = count + 1;
    return (*(P4ThunkFn_003ca740 *)(rec + 0x10))(arg0, 0, 0, 0);
}

s32 func_003ca780(u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
    u8 *rec = (u8 *)arg0 + *(s32 *)0x007647F8;

    *(u32 *)0x008872E4 = 0;
    return (*(P4ThunkFn_003ca740 *)(rec + 0x14))(arg0, arg1, arg2, arg3);
}

u32 func_003ca7a0(u32 arg0)
{
    u8 *rec = (u8 *)arg0 + *(s32 *)0x007647F8;
    u8 *e;
    P4CallFn_003ca740 fn;

    if ((*(s32 (**)(u32))(rec + 0x18))(arg0) == 0) {
        return 0;
    }
    e = *(u8 **)(rec + 0xC);
    if (e != (u8 *)0) {
        fn = *(P4CallFn_003ca740 *)0x00887380;
        p4c_003cb300_003ca740(e, arg0);
        fn(0, 0, *(u32 *)(e + 0xC) == 1);
    }
    return arg0;
}
