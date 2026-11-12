/* func_003c1ea0 (656 bytes): grows the per-vertex record table of the mesh arg0 by arg1 entries and returns the previous count (-1 when the
 * allocation fails: {2, func_003df590(0x80000013, size)} goes to func_003df4d0). An entry is 0x1C bytes plus, unless flag 0x1000000 of the word at +8
 * is set, 12 * n bytes (n = arg0[5]) and with flag 0x10 another 12 * n; arg0[23] is the table and arg0[6] the count. The new table is made with the
 * allocator slot 0x008873E8 or, for an existing one, the realloc slot 0x008873F0 (tag 0x3000F), the per-entry data region that follows the 0x1C-byte
 * headers (the old count * (entry - 0x1C) bytes) is moved up by 0x1C * arg1 bytes, the count grows and each entry's +0x14 / +0x18
 * pointers are set to its data (the vectors of n elements, in the region after all the headers; 0 when the flags say there is none). The new
 * entries get words +4 to +0x10 cleared and +0 = arg0. */

extern void p4c_003df4d0_003c1ea0(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003c1ea0(s32 code, ...) __asm__("func_003df590");

typedef void *(*P4AllocFn_003c1ea0)(u32 size, u32 tag);
typedef void *(*P4ReallocFn_003c1ea0)(void *p, u32 size, u32 tag);

s32 func_003c1ea0(u32 *arg0, s32 arg1)
{
    u32 flags = arg0[2];
    u32 entry;
    u32 total;
    u8 *tab;
    u8 *p;
    s32 err[2];
    u32 cursor;
    s32 i;
    u32 *e;

    if ((flags & 0x1000000) != 0) {
        entry = 0x1C;
    } else {
        entry = arg0[5] * 12 + 0x1C;
        if ((flags & 0x10) != 0) {
            entry += arg0[5] * 12;
        }
    }
    total = entry * (arg0[6] + (u32)arg1);
    tab = (u8 *)arg0[23];
    if (tab != (u8 *)0) {
        p = (u8 *)(*(P4ReallocFn_003c1ea0 *)0x008873F0)(tab, total, 0x3000F);
        if (p == (u8 *)0) {
            err[0] = 2;
            err[1] = p4c_003df590_003c1ea0(0x80000013, total);
            p4c_003df4d0_003c1ea0(err);
            return -1;
        }
        {
            u8 *src = p + entry * arg0[6] - 1;
            u8 *dst = src + 0x1C * (u32)arg1;
            u32 cnt = (entry - 0x1C) * arg0[6];

            while (cnt != 0) {
                *dst = *src;
                cnt--;
                src--;
                dst--;
            }
        }
    } else {
        p = (u8 *)(*(P4AllocFn_003c1ea0 *)0x008873E8)(total, 0x3000F);
        if (p == (u8 *)0) {
            err[0] = 2;
            err[1] = p4c_003df590_003c1ea0(0x80000013, total);
            p4c_003df4d0_003c1ea0(err);
            return -1;
        }
    }
    arg0[6] = arg0[6] + (u32)arg1;
    arg0[23] = (u32)p;
    cursor = (u32)p + arg0[6] * 0x1C;
    for (i = 0; i < (s32)arg0[6]; i++) {
        e = (u32 *)(p + i * 0x1C);
        e[5] = 0;
        e[6] = 0;
        if ((arg0[2] & 0x1000000) == 0 && arg0[5] != 0) {
            e[5] = cursor;
            cursor += arg0[5] * 12;
            if ((arg0[2] & 0x10) != 0) {
                e[6] = cursor;
                cursor += arg0[5] * 12;
            }
        }
    }
    for (i = (s32)arg0[6] - arg1; i < (s32)arg0[6]; i++) {
        e = (u32 *)(p + i * 0x1C);
        e[1] = 0;
        e[2] = 0;
        e[3] = 0;
        e[4] = 0;
        e[0] = (u32)arg0;
    }
    return (s32)arg0[6] - arg1;
}
