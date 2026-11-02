/* func_003d51c0 (320 bytes): allocates an animation record of arg1 elements for the type arg0. The type is looked up in the
 * table at 0x00886E50 (0x30 bytes per type, the type id in word 0; the count is the word at gp - 0x48CC, 0x00764824); an unknown type
 * reports {0x1B7, func_003df590(2)} through func_003df4d0 and returns 0. The record (allocator slot 0x008873E8, tag
 * 0x3001B, size e[11] + arg1 * e[2] + 0x18 for the table entry e) gets: +0 = e, +4 = arg1, +8 = arg2, +0xC = the float arg3, +0x10 =
 * record + 0x18, and +0x14 = record[4] + arg1 * e[2] when e[11] > 0 else 0. Returns the record. */

extern void p4c_003df4d0_003d51c0(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003d51c0(s32 code, ...) __asm__("func_003df590");

typedef void *(*P4AllocFn_003d51c0)(u32 size, u32 tag);

u32 *func_003d51c0(u32 arg0, u32 arg1, u32 arg2, f32 arg3)
{
    u32 *e = (u32 *)0;
    u32 *r;
    s32 i;
    s32 err[2];
    s32 n = *(s32 *)0x00764824;

    for (i = 0; i < n; i++) {
        if (*(u32 *)(0x00886E50 + i * 0x30) == arg0) {
            e = (u32 *)(0x00886E50 + i * 0x30);
            break;
        }
    }
    if (e == (u32 *)0) {
        err[0] = 0x1B7;
        err[1] = p4c_003df590_003d51c0(2);
        p4c_003df4d0_003d51c0(err);
        return (u32 *)0;
    }
    r = (u32 *)(*(P4AllocFn_003d51c0 *)0x008873E8)(e[11] + arg1 * e[2] + 0x18, 0x3001B);
    r[1] = arg1;
    *(f32 *)(r + 3) = arg3;
    r[2] = arg2;
    r[0] = (u32)e;
    r[4] = (u32)r + 0x18;
    if ((s32)e[11] > 0) {
        r[5] = r[4] + arg1 * e[2];
    } else {
        r[5] = 0;
    }
    return r;
}
