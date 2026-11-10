/* func_003d3780 (352 bytes): sets the size words of an object's raster record. arg0 must be non-null and arg1, arg2 positive
 * (else 0 is returned). The record r = func_004125d0(arg0); one with a flag in the low nibble of r[15] returns 0. A missing record
 * is created with func_00412520(arg0, 0x40) (0 fails): its first ten words are cleared, r[15] = 0, r[14] = 4 and r[12] = r[13] = 276 (69 * 4)
 * and it is looked up again with func_004125d0(arg0). Then r[13] = arg1 * arg2, r[14] = arg1 and r[15] = 0x10; returns arg0. */

extern u32 p4c_00412520_003d3780(u32 a, u32 n) __asm__("func_00412520");
extern u32 p4c_004125d0_003d3780(u32 a) __asm__("func_004125d0");

u32 func_003d3780(u32 arg0, s32 arg1, s32 arg2)
{
    u32 *r;
    s32 i;

    if (arg0 == 0 || arg1 <= 0 || arg2 <= 0) {
        return 0;
    }
    r = (u32 *)p4c_004125d0_003d3780(arg0);
    if (r != (u32 *)0 && (r[15] & 0xF) != 0) {
        return 0;
    }
    if (r == (u32 *)0) {
        r = (u32 *)p4c_00412520_003d3780(arg0, 0x40);
        if (r == (u32 *)0) {
            return 0;
        }
        for (i = 0; i < 10; i++) {
            r[i] = 0;
        }
        r[15] = 0;
        r[14] = 4;
        r[13] = r[14] * 69;
        r[12] = r[14] * 69;
        r = (u32 *)p4c_004125d0_003d3780(arg0);
    }
    r[13] = (u32)(arg2 * arg1);
    r[14] = (u32)arg1;
    r[15] = 0x10;
    return arg0;
}
