/* func_003a9f40 (368 bytes): draws one batch of a draw state through the device register hooks and returns 0. The object's
 * drawing record r = *(arg0 + the offset word at gp - 0x49F0 (0x00764700)) + 0x9C. arg1 is the state (+0x58 flags, +0xC and +0x24 values,
 * +0x5C / +0x60 / +0x64 register request). When arg1[0x64] is set the registers 0xA and 0xB are saved with the get hook at
 * 0x00887304 (after setting register 0xC to 1 and writing the new values arg1[0x5C] and arg1[0x60] through the set hook at 0x00887300) and
 * restored afterwards. With flag 2 of arg1[0x58] the batch is func_003a93b0(r[0], arg1[3], arg2, arg1[9]); otherwise
 * func_003a14f0(r[1], r[2]) is called first and func_003a9880(r[0], arg1[3], arg2, m + 0x20) with m =
 * func_003e9700(((u32 *)r[2])[1]). */

extern void p4c_003a14f0_003a9f40(u32 a, u32 b) __asm__("func_003a14f0");
extern void p4c_003a93b0_003a9f40(u32 a, u32 b, u32 c, u32 d) __asm__("func_003a93b0");
extern void p4c_003a9880_003a9f40(u32 a, u32 b, u32 c, u32 d) __asm__("func_003a9880");
extern u8 *p4c_003e9700_003a9f40(u32 o) __asm__("func_003e9700");

typedef void (*P4SetRegFn_003a9f40)(u32 reg, u32 value);
typedef void (*P4GetRegFn_003a9f40)(u32 reg, u32 *out);

s32 func_003a9f40(u32 arg0, u32 *arg1, u32 arg2)
{
    u32 *r = *(u32 **)(arg0 + *(s32 *)0x00764700);
    u32 save[2];
    u8 *m;

    r = (u32 *)r[39];
    if (arg1[25] != 0) {
        (*(P4SetRegFn_003a9f40 *)0x00887300)(0xC, 1);
        (*(P4GetRegFn_003a9f40 *)0x00887304)(0xA, &save[0]);
        (*(P4SetRegFn_003a9f40 *)0x00887300)(0xA, arg1[23]);
        (*(P4GetRegFn_003a9f40 *)0x00887304)(0xB, &save[1]);
        (*(P4SetRegFn_003a9f40 *)0x00887300)(0xB, arg1[24]);
    }
    if ((arg1[22] & 2) != 0) {
        p4c_003a93b0_003a9f40(r[0], arg1[3], arg2, arg1[9]);
    } else {
        m = p4c_003e9700_003a9f40(((u32 *)r[2])[1]);
        p4c_003a14f0_003a9f40(r[1], r[2]);
        p4c_003a9880_003a9f40(r[0], arg1[3], arg2, (u32)(m + 0x20));
    }
    if (arg1[25] != 0) {
        (*(P4SetRegFn_003a9f40 *)0x00887300)(0xA, save[0]);
        (*(P4SetRegFn_003a9f40 *)0x00887300)(0xB, save[1]);
    }
    return 0;
}
