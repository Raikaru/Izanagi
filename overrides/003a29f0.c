/* func_003a29f0 (224 bytes): rebinds the texture of a draw state. arg0 + 0x4C points at the state record r; its 64-bit
 * flags at +0xC8 are combined with the bits 0x70000 of the 64-bit global at gp - 0x4730 (0x007649C0). arg1 = 0 clears
 * the word pair at r + 0xD0 and bit 0 of the flags. Otherwise the texture t = *arg1 is checked with func_0040f570(t, 1);
 * when that succeeds func_00410390(t, r + 0xD0, arg1[0x50] as a byte) is called, bit 0 of the flags is cleared when
 * (t[0x23] & 0xF) == 6 and set otherwise, and a texture with t[0x14] below 9 stores -1 in the word at gp - 0x46B4
 * (0x00764A3C). The flags are stored back to +0xC8 either way. Returns 1. */

extern s32 p4c_0040f570_003a29f0(u8 *t, s32 f) __asm__("func_0040f570");
extern void p4c_00410390_003a29f0(u8 *t, u8 *d, u32 b) __asm__("func_00410390");

s32 func_003a29f0(u8 *arg0, u8 *arg1)
{
    u8 *r = *(u8 **)(arg0 + 0x4C);
    u64 flags = *(u64 *)(r + 0xC8) | (*(u64 *)0x007649C0 & 0x70000);
    u8 *t;

    if (arg1 == (u8 *)0) {
        *(u64 *)(r + 0xD0) = 0;
        flags = flags & ~(u64)1;
    } else {
        t = *(u8 **)arg1;
        if (p4c_0040f570_003a29f0(t, 1) != 0) {
            p4c_00410390_003a29f0(t, r + 0xD0, arg1[0x50]);
            if ((t[0x23] & 0xF) == 6) {
                flags = flags & ~(u64)1;
            } else {
                flags = flags | 1;
            }
            if (*(s32 *)(t + 0x14) < 9) {
                *(s32 *)0x00764A3C = -1;
            }
        }
    }
    *(u64 *)(r + 0xC8) = flags;
    return 1;
}
