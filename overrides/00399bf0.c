/* func_00399bf0 (376 bytes): selects the shape mode (arg1) of the collision record hanging off the object arg0 and
 * sets up the two 0x40-byte slot entries accordingly. Returns arg0, or 0 when the record cannot be had.
 *
 * The record is the pointer at arg0 + the plugin offset in 0x007646D0; when it is null a 0x90-byte record is
 * allocated (pool *(u32 *)0x00884A80, tag 0x30120, through the pool-allocator slot 0x008873F8) and zeroed. When arg1 is 0 or the
 * record already holds a different non-zero mode (word +0x80), func_00398410 resets it first. The mode is stored in
 * +0x80 and the type words are set: mode 1: +0x20 = 1; 2: +0x20 = 2; 3: +0x20 = 1 and +0x60 = 2; 4: +0x20 = 4 and
 * func_0039a590(arg0, 5, 6); 5: +0x20 = 5; 6: +0x20 = 5, +0x60 = 4 and func_0039a590(arg0, 5, 6); any other mode
 * changes nothing. func_0039a7f0(arg0) then finishes the update. */

extern void p4c_00398410_00399bf0(u8 *r) __asm__("func_00398410");
extern s32 p4c_0039a590_00399bf0(u32 a, u32 b, u32 c) __asm__("func_0039a590");
extern void p4c_0039a7f0_00399bf0(u32 a) __asm__("func_0039a7f0");
extern void p4c_0043f9c8_00399bf0(u8 *p, s32 v, s32 n) __asm__("func_0043f9c8");

typedef void *(*P4PoolAllocFn_00399bf0)(u32 pool, u32 tag);

u32 func_00399bf0(u32 arg0, u32 arg1)
{
    u8 *rec;
    u32 mode;

    rec = *(u8 **)(arg0 + *(s32 *)0x007646D0);
    if (rec == (u8 *)0) {
        rec = (u8 *)(*(P4PoolAllocFn_00399bf0 *)0x008873F8)(*(u32 *)0x00884A80, 0x30120);
        if (rec != (u8 *)0) {
            p4c_0043f9c8_00399bf0(rec, 0, 0x90);
            *(u8 **)(arg0 + *(s32 *)0x007646D0) = rec;
        }
    }
    if (rec == (u8 *)0) {
        return 0;
    }
    mode = *(u32 *)(rec + 0x80);
    if (arg1 == 0 || (mode != 0 && mode != arg1)) {
        p4c_00398410_00399bf0(rec);
    }
    *(u32 *)(rec + 0x80) = arg1;
    switch (arg1) {
    case 1:
        *(u32 *)(rec + 0x20) = 1;
        break;
    case 2:
        *(u32 *)(rec + 0x20) = 2;
        break;
    case 3:
        *(u32 *)(rec + 0x20) = 1;
        *(u32 *)(rec + 0x60) = 2;
        break;
    case 4:
        *(u32 *)(rec + 0x20) = 4;
        p4c_0039a590_00399bf0(arg0, 5, 6);
        break;
    case 5:
        *(u32 *)(rec + 0x20) = 5;
        break;
    case 6:
        *(u32 *)(rec + 0x20) = 5;
        *(u32 *)(rec + 0x60) = 4;
        p4c_0039a590_00399bf0(arg0, 5, 6);
        break;
    default:
        break;
    }
    p4c_0039a7f0_00399bf0(arg0);
    return arg0;
}
