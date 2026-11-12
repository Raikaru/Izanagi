/* func_0039beb0 (336 bytes): copies the 4x4 matrix (64 bytes) of a frame into arg1, going through a one-entry cache.
 * arg0 is the frame (null: the word at +4 of the pointer at 0x008872E0); arg2 + 8 is a record pointer whose first byte
 * is a parent index (0xFF: none). The cache key is the three words at gp - 0x49FC (record), gp - 0x4A00 (frame) and the 64-bit
 * gp - 0x49F8 (the halfword at 0x008872E8); on a miss the matrix is func_003e9700(frame) copied into D_00884AE0
 * (func_003e0960) when the record's index is 0xFF, else func_003e05f0(D_00884AE0, func_003e9700(record[1]),
 * frame matrix copy) composes the two; afterwards, unless the matrix flag word at 0x00884AEC has both low bits set,
 * func_003e0670(D_00884AE0, D_00884AE0) normalises it. The key is stored and D_00884AE0 copied to arg1. Returns the frame. */

extern void p4c_003e05f0_0039beb0(void *d, void *a, void *b) __asm__("func_003e05f0");
extern void p4c_003e0670_0039beb0(void *d, void *s) __asm__("func_003e0670");
extern void p4c_003e0960_0039beb0(void *m, void *src) __asm__("func_003e0960");
extern u8 *p4c_003e9700_0039beb0(u32 o) __asm__("func_003e9700");

u8 *func_0039beb0(u8 *arg0, u32 *arg1, u8 *arg2)
{
    u8 *rec;
    u32 stamp;
    u8 tmp[64] __attribute__((aligned(16)));
    u8 *m;

    if (arg0 == (u8 *)0) {
        arg0 = *(u8 **)(*(u32 *)0x008872E0 + 4);
    }
    rec = *(u8 **)(arg2 + 8);
    stamp = *(u16 *)0x008872E8;
    if (rec == *(u8 **)0x007646F4 && arg0 == *(u8 **)0x007646F0 && (u64)stamp == *(u64 *)0x007646F8) {
        goto copy;
    }
    m = p4c_003e9700_0039beb0(arg0);
    if (*rec == 0xFF) {
        p4c_003e0960_0039beb0((void *)0x00884AE0, m);
    } else {
        p4c_003e0960_0039beb0(tmp, m);
        p4c_003e05f0_0039beb0((void *)0x00884AE0, p4c_003e9700_0039beb0(*(u32 *)(rec + 4)), tmp);
    }
    if ((*(u32 *)0x00884AEC & 3) != 3) {
        p4c_003e0670_0039beb0((void *)0x00884AE0, (void *)0x00884AE0);
    }
    *(u8 **)0x007646F4 = rec;
    *(u64 *)0x007646F8 = (u64)stamp;
    *(u8 **)0x007646F0 = arg0;
copy:
    {
        u32 *src = (u32 *)0x00884AE0;
        s32 i;

        for (i = 0; i < 16; i++) {
            arg1[i] = src[i];
        }
    }
    return arg0;
}
