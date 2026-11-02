/* func_003c57d0 (544 bytes): appends a 20-byte record to the list arg0 ({capacity, count, items}) and returns arg0 (0 when the allocation
 * fails: the error {2, func_003df590(0x80000013, size)} goes to func_003df4d0). A full list (count >= capacity) grows to count + 1
 * records: the first allocation uses the allocator slot 0x008873E8 and a later growth the realloc slot 0x008873F0 (tag 0x1030502). The new
 * record at items + 20 * count gets the word +8 = arg1, the halfwords +0 = arg2, +2 = arg3, +4 = arg4, +0xC = arg5, +0xE = arg6, +0x10 = arg7 and +0x12 = arg8 (the ninth
 * argument, a stack halfword), and the count is incremented. */

extern void p4c_003df4d0_003c57d0(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003c57d0(s32 code, ...) __asm__("func_003df590");

typedef void *(*P4AllocFn_003c57d0)(u32 size, u32 tag);
typedef void *(*P4ReallocFn_003c57d0)(void *p, u32 size, u32 tag);

u32 *func_003c57d0(u32 *arg0, u32 arg1, u16 arg2, u16 arg3, u16 arg4, u16 arg5, u16 arg6, u16 arg7, u16 arg8)
{
    u32 n = arg0[1];
    u32 bytes;
    void *p;
    u8 *e;
    s32 err[2];

    if (!(n < arg0[0])) {
        bytes = (n + 1) * 20;
        if (n != 0) {
            p = (*(P4ReallocFn_003c57d0 *)0x008873F0)((void *)arg0[2], bytes, 0x1030502);
        } else {
            p = (*(P4AllocFn_003c57d0 *)0x008873E8)(bytes, 0x1030502);
        }
        if (p == (void *)0) {
            err[0] = 2;
            err[1] = p4c_003df590_003c57d0(0x80000013, bytes);
            p4c_003df4d0_003c57d0(err);
            return (u32 *)0;
        }
        arg0[2] = (u32)p;
        arg0[0] = arg0[1] + 1;
    }
    e = (u8 *)arg0[2] + arg0[1] * 20;
    *(u32 *)(e + 8) = arg1;
    *(u16 *)(e + 0) = arg2;
    *(u16 *)(e + 2) = arg3;
    *(u16 *)(e + 4) = arg4;
    *(u16 *)(e + 0xC) = arg5;
    *(u16 *)(e + 0xE) = arg6;
    *(u16 *)(e + 0x10) = arg7;
    *(u16 *)(e + 0x12) = arg8;
    arg0[1] = arg0[1] + 1;
    return arg0;
}
