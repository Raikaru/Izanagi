/* func_003c2630 (1072 bytes): creates a geometry block with arg0 vertices, arg1 triangles and the format flags arg2 and returns it (0 on failure).
 * arg0 must be in [0, 0x10000) and arg1 >= 0 (arg0 >= 0x10000 also reports {2, func_003df590(6, arg1)} to func_003df4d0). The number of texture sets is bits 16 to 23 of arg2, else 2
 * with flag 0x80, else 1 with flag 4, else 0 (stored at +0x1C); the stored format (+8) is (arg2 & 0x7B) with flag 4 for one set and 0x80
 * for two or more, plus bits 24 to 27 of arg2. The block is allocated with the allocator slot 0x008873E8 (tag 0x3000F) with *(u32 *)0x0070AFB0 bytes of
 * header and, unless flag 0x1000000 is set, space for the vertex normals (4 bytes per vertex, flag 8), 8 bytes per vertex and set, and 8 bytes per triangle.
 * func_003c4a40(block + 0x20) initialises the embedded record (a failure returns 0 without freeing, as retail). Fields: byte +0 = 8, +0xE = 1, +0x10 = triangle count,
 * +0x14 = vertex count, +0x34 = pointers to the texture sets (zeroed 0x20 bytes first) and +0x30 = the normals, +0x2C = the triangles (halfword +6 of each = 0xFFFF),
 * laid out after the header in that order. func_003c1ea0(block, 1) adds the first morph target; on success the block is added to the list
 * func_003e3b70(0x0070AFB0, block), otherwise the record is destroyed (func_003c49a0(block + 0x20)), the block freed (slot 0x008873EC) and 0 returned. */

extern s32 p4c_003c1ea0_003c2630(u8 *p, s32 n) __asm__("func_003c1ea0");
extern void p4c_003c49a0_003c2630(void *p) __asm__("func_003c49a0");
extern s32 p4c_003c4a40_003c2630(u8 *p) __asm__("func_003c4a40");
extern void p4c_003df4d0_003c2630(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003c2630(s32 code, ...) __asm__("func_003df590");
extern void p4c_003e3b70_003c2630(u32 list, u8 *p) __asm__("func_003e3b70");
extern void *p4c_0043f9c8_003c2630(void *dst, s32 c, u32 n) __asm__("func_0043f9c8");

typedef void *(*P4AllocFn_003c2630)(u32 size, u32 tag);
typedef void (*P4FreeFn_003c2630)(void *p);

u8 *func_003c2630(s32 arg0, s32 arg1, u32 arg2)
{
    u32 size;
    u32 uvs;
    u32 five;
    u32 fmt;
    u32 noData = arg2 & 0x1000000;
    u8 *p;
    s32 err[2];
    u32 cursor;
    s32 i;

    if (arg0 < 0 || !(arg0 < 0x10000) || arg1 < 0) {
        if (arg0 >= 0 && !(arg0 < 0x10000)) {
            err[0] = 2;
            err[1] = p4c_003df590_003c2630(6, arg1);
            p4c_003df4d0_003c2630(err);
        }
        return (u8 *)0;
    }
    size = *(u32 *)0x0070AFB0;
    if ((arg2 & 0xFF0000) != 0) {
        uvs = (arg2 & 0xFF0000) >> 16;
    } else if ((arg2 & 0x80) != 0) {
        uvs = 2;
    } else {
        uvs = (arg2 & 4) != 0;
    }
    if (uvs == 1) {
        five = 4;
    } else if (uvs >= 2) {
        five = 0x80;
    } else {
        five = 0;
    }
    fmt = (arg2 & 0xFF & 0xFFFFFF7Bu) | five;
    if (noData == 0) {
        if ((fmt & 8) != 0) {
            size += (u32)arg0 << 2;
        }
        if (uvs != 0) {
            size += (u32)((s32)uvs * (arg0 << 3));
        }
        size += (u32)arg1 << 3;
    }
    p = (u8 *)(*(P4AllocFn_003c2630 *)0x008873E8)(size, 0x3000F);
    if (p == (u8 *)0) {
        return (u8 *)0;
    }
    if (p4c_003c4a40_003c2630(p + 0x20) == 0) {
        return (u8 *)0;
    }
    *(u32 *)(p + 0x5C) = 0;
    *(u32 *)(p + 0x18) = 0;
    p[0] = 8;
    p[1] = 0;
    p[2] = 0;
    p[3] = 0;
    *(u32 *)(p + 4) = 0;
    *(u32 *)(p + 0x58) = 0;
    *(u16 *)(p + 0xC) = 0;
    *(u16 *)(p + 0xE) = 1;
    *(u32 *)(p + 0x54) = 0;
    *(u32 *)(p + 0x1C) = uvs;
    p4c_0043f9c8_003c2630(p + 0x34, 0, 0x20);
    *(u32 *)(p + 0x30) = 0;
    *(u32 *)(p + 0x2C) = 0;
    *(u32 *)(p + 8) = fmt | (arg2 & 0xF000000);
    *(u32 *)(p + 0x10) = (u32)arg1;
    *(u32 *)(p + 0x14) = (u32)arg0;
    if (noData == 0) {
        cursor = (u32)p + *(u32 *)0x0070AFB0;
        if ((fmt & 8) != 0 && arg0 != 0) {
            *(u32 *)(p + 0x30) = cursor;
            cursor += (u32)arg0 << 2;
        }
        if (uvs != 0 && arg0 != 0) {
            for (i = 0; i < (s32)uvs; i++) {
                *(u32 *)(p + 0x34 + 4 * i) = cursor;
                cursor += (u32)arg0 << 3;
            }
        }
        if (arg1 != 0) {
            *(u32 *)(p + 0x2C) = cursor;
            for (i = 0; i < arg1; i++) {
                *(u16 *)(cursor + i * 8 + 6) = 0xFFFF;
            }
        }
    }
    if (p4c_003c1ea0_003c2630(p, 1) >= 0) {
        p4c_003e3b70_003c2630(0x0070AFB0, p);
        return p;
    }
    p4c_003c49a0_003c2630(p + 0x20);
    (*(P4FreeFn_003c2630 *)0x008873EC)(p);
    return (u8 *)0;
}
