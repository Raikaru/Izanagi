/* func_003db360 (224 bytes): runs a test over the entries of arg0 and records the passing ones. arg0 has a halfword count at
 * +0x84 and an array of 8-byte entries at +4 (three halfwords a, b, c at +0, +2 and +4 and an index halfword at +6).
 * For each entry, arg1[6] (a callback) is called with the element ((u32 *)arg1[1][4])[index] and arg1[2]; a non-zero
 * result calls func_003db190(arg0[4], arg0[2], a, b, c, arg1). Returns arg0. */

extern void p4c_003db190_003db360(u32 a, u32 b, u32 c, u32 d, u32 e, u32 *f) __asm__("func_003db190");

typedef s32 (*P4TestFn_003db360)(u32 elem, u32 arg);

u8 *func_003db360(u8 *arg0, u32 *arg1)
{
    u32 count = *(u16 *)(arg0 + 0x84);
    u32 i;
    u16 *e;

    for (i = 0; i < count; i++) {
        e = (u16 *)(*(u8 **)(arg0 + 4) + i * 8);
        if (((P4TestFn_003db360)arg1[6])(((u32 *)((u32 *)arg1[1])[4])[e[3]], arg1[2]) != 0) {
            p4c_003db190_003db360(*(u32 *)(arg0 + 0x10), *(u32 *)(arg0 + 8), e[0], e[1], e[2], arg1);
        }
        count = *(u16 *)(arg0 + 0x84);
    }
    return arg0;
}
