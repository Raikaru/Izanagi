/* func_003be180 (1600 bytes): reads a geometry chunk (type 1, version in [0x35000, 0x37003), otherwise {2, func_003df590(0x80000004)} goes to func_003df4d0) from
 * the stream arg0 and returns the new geometry block (0 on failure; a failed block is destroyed with func_003c9d40). The 0x40-byte body (zeroed first) holds: +0 flag (an
 * extra material chunk follows), +4 / +8 / +0xC the bounding centre (negated into the block), +0x10 triangle count, +0x14 vertex count, +0x18 / +0x1C morph target
 * and mesh counts, +0x20 must be <= 0 (else the version error), +0x24 format flags (the number of texture sets is bits 16 to 23, else 2 with flag 0x80, else 1 with
 * flag 4), +0x28 .. +0x3C six floats. The block (allocator slot 0x008873E8, tag 0x3000B, zeroed) gets the size *(u32 *)0x0070B060 + 24 * body[+0x18] + body[+0x1C] *
 * *(u32 *)0x0070B040 plus, unless flag 0x1000000, 12 bytes per vertex (+4 with flag 0x10, +4 with flag 8, 8 * sets) and 8 per triangle: {byte 7, +3 = 1, func_003c9940(block), +8 = flags, +0xC = 2, +0x44..+0x4C
 * = -centre, +0x20 = sets, empty lists at +0x2C / +0x34 / +0x3C}. A type 8 chunk is followed by the materials (func_003c4fa0(stream, block + 0x10)), then by the meshes: type 9
 * (func_003bd6b0) when the flag was set, else type 0xA (func_003bdd00); both take (stream, &data pointer, block, flags) and give block[7]. Then the sphere floats
 * go to +0x50 .. +0x64, func_003c97e0 / func_003c9e80(block, 0) run, every mesh entry with a first word is passed to func_003c42b0(entry, entry[0]), the block is added to
 * the list at 0x0070B060 (func_003e3b70), its extensions are read (func_003e3110 with *(u32 *)0x00764794 / 0x00764790 zeroed first and a nonzero first one passed to
 * func_003e32f0) and func_003c9b30 finishes it. Unset body words stay zero. */

extern u32 p4c_003bd6b0_003be180(u32 s, u32 *d, u8 *b, u32 f) __asm__("func_003bd6b0");
extern u32 p4c_003bdd00_003be180(u32 s, u32 *d, u8 *b, u32 f) __asm__("func_003bdd00");
extern void p4c_003c42b0_003be180(u32 *e, u32 v) __asm__("func_003c42b0");
extern s32 p4c_003c4fa0_003be180(u32 s, u8 *p) __asm__("func_003c4fa0");
extern void p4c_003c97e0_003be180(u8 *b) __asm__("func_003c97e0");
extern void p4c_003c9940_003be180(u8 *b, u32 size) __asm__("func_003c9940");
extern s32 p4c_003c9b30_003be180(u8 *b) __asm__("func_003c9b30");
extern void p4c_003c9d40_003be180(u8 *b) __asm__("func_003c9d40");
extern void p4c_003c9e80_003be180(u8 *b, s32 z) __asm__("func_003c9e80");
extern s32 p4c_003df050_003be180(s32 stream, s32 type, void *size, void *version) __asm__("func_003df050");
extern void p4c_003df4d0_003be180(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003be180(s32 code, ...) __asm__("func_003df590");
extern s32 p4c_003e2910_003be180(s32 stream, void *buf, s32 n) __asm__("func_003e2910");
extern s32 p4c_003e3110_003be180(u32 list, u32 s, u8 *b) __asm__("func_003e3110");
extern void p4c_003e32f0_003be180(u32 list, u32 a, u8 *b, u32 c) __asm__("func_003e32f0");
extern void p4c_003e3b70_003be180(u32 list, u8 *b) __asm__("func_003e3b70");
extern void *p4c_0043f9c8_003be180(void *dst, s32 c, u32 n) __asm__("func_0043f9c8");

typedef void *(*P4AllocFn_003be180)(u32 size, u32 tag);

u32 func_003be180(u32 arg0)
{
    u32 size = 0;
    u32 version = 0;
    u32 w[16];
    u8 *block;
    u32 flags;
    u32 uvs;
    u32 total;
    u32 data;
    s32 err[2];
    u32 i;
    u32 n;
    u32 *e;
    f32 neg1 = -1.0f;

    for (i = 0; i < 16; i++) {
        w[i] = 0;
    }
    if (p4c_003df050_003be180(arg0, 1, &size, &version) == 0) {
        return 0;
    }
    if (version < 0x35000 || !(version < 0x37003)) {
        err[0] = 2;
        err[1] = p4c_003df590_003be180(0x80000004);
        p4c_003df4d0_003be180(err);
        return 0;
    }
    if (p4c_003e2910_003be180(arg0, w, size) != (s32)size) {
        return 0;
    }
    if ((s32)w[8] > 0) {
        err[0] = 2;
        err[1] = p4c_003df590_003be180(0x80000004);
        p4c_003df4d0_003be180(err);
        return 0;
    }
    total = *(u32 *)0x0070B060 + w[6] * 24 + (u32)((s32)w[7] * *(s32 *)0x0070B040);
    flags = w[9];
    if ((flags & 0xFF0000) != 0) {
        uvs = (u32)((s32)(flags & 0xFF0000) >> 16);
    } else if ((flags & 0x80) != 0) {
        uvs = 2;
    } else {
        uvs = (flags & 4) != 0;
    }
    if ((flags & 0x1000000) == 0) {
        total += w[5] * 12;
        if ((flags & 0x10) != 0) {
            total += w[5] << 2;
        }
        if ((flags & 8) != 0) {
            total += w[5] << 2;
        }
        if (uvs != 0) {
            total += (u32)((s32)uvs * (s32)w[5]) << 3;
        }
        total += w[4] << 3;
    }
    block = (u8 *)(*(P4AllocFn_003be180 *)0x008873E8)(total, 0x3000B);
    if (block == (u8 *)0) {
        err[0] = 2;
        err[1] = p4c_003df590_003be180(0x80000013, total);
        p4c_003df4d0_003be180(err);
        return 0;
    }
    p4c_0043f9c8_003be180(block, 0, total);
    data = (u32)block + *(u32 *)0x0070B060;
    block[0] = 7;
    block[1] = 0;
    block[2] = 0;
    block[3] = 0;
    *(u32 *)(block + 4) = 0;
    block[3] = 1;
    p4c_003c9940_003be180(block, total);
    *(u32 *)(block + 8) = flags;
    *(u32 *)(block + 0xC) = 2;
    *(f32 *)(block + 0x44) = neg1 * *(f32 *)&w[1];
    *(f32 *)(block + 0x48) = neg1 * *(f32 *)&w[2];
    *(f32 *)(block + 0x4C) = neg1 * *(f32 *)&w[3];
    *(u32 *)(block + 0x20) = uvs;
    *(u32 *)(block + 0x1C) = 0;
    *(u32 *)(block + 0x6C) = 0;
    *(u32 *)(block + 0x2C) = (u32)block + 0x2C;
    *(u32 *)(block + 0x30) = (u32)block + 0x2C;
    *(u32 *)(block + 0x34) = (u32)block + 0x34;
    *(u32 *)(block + 0x38) = (u32)block + 0x34;
    *(u32 *)(block + 0x3C) = (u32)block + 0x3C;
    *(u32 *)(block + 0x40) = (u32)block + 0x3C;
    if (p4c_003df050_003be180(arg0, 8, 0, &version) == 0) {
        p4c_003c9d40_003be180(block);
        return 0;
    }
    if (version < 0x35000 || !(version < 0x37003)) {
        err[0] = 2;
        err[1] = p4c_003df590_003be180(0x80000004);
        p4c_003df4d0_003be180(err);
        p4c_003c9d40_003be180(block);
        return 0;
    }
    if (p4c_003c4fa0_003be180(arg0, block + 0x10) == 0) {
        p4c_003c9d40_003be180(block);
        return 0;
    }
    if (w[0] != 0) {
        if (p4c_003df050_003be180(arg0, 9, 0, &version) == 0) {
            p4c_003c9d40_003be180(block);
            return 0;
        }
        if (version < 0x35000 || !(version < 0x37003)) {
            err[0] = 2;
            err[1] = p4c_003df590_003be180(0x80000004);
            p4c_003df4d0_003be180(err);
            p4c_003c9d40_003be180(block);
            return 0;
        }
        *(u32 *)(block + 0x1C) = p4c_003bd6b0_003be180(arg0, &data, block, flags);
    } else {
        if (p4c_003df050_003be180(arg0, 0xA, 0, &version) == 0) {
            p4c_003c9d40_003be180(block);
            return 0;
        }
        if (version < 0x35000 || !(version < 0x37003)) {
            err[0] = 2;
            err[1] = p4c_003df590_003be180(0x80000004);
            p4c_003df4d0_003be180(err);
            p4c_003c9d40_003be180(block);
            return 0;
        }
        *(u32 *)(block + 0x1C) = p4c_003bdd00_003be180(arg0, &data, block, flags);
    }
    if (*(u32 *)(block + 0x1C) == 0) {
        p4c_003c9d40_003be180(block);
        return 0;
    }
    *(u32 *)(block + 0x24) = 0;
    *(u32 *)(block + 0x28) = (u32)block + 0x2C;
    *(u32 *)(block + 0x50) = w[10];
    *(u32 *)(block + 0x54) = w[11];
    *(u32 *)(block + 0x58) = w[12];
    *(u32 *)(block + 0x5C) = w[13];
    *(u32 *)(block + 0x60) = w[14];
    *(u32 *)(block + 0x64) = w[15];
    p4c_003c97e0_003be180(block);
    p4c_003c9e80_003be180(block, 0);
    n = *(u32 *)(block + 0x14);
    for (i = 0; (s32)i < (s32)n; i++) {
        e = (u32 *)(*(u32 *)(*(u32 *)(block + 0x10) + i * 4));
        if (e[0] != 0) {
            p4c_003c42b0_003be180(e, e[0]);
        }
        n = *(u32 *)(block + 0x14);
    }
    p4c_003e3b70_003be180(0x0070B060, block);
    *(u32 *)0x00764794 = 0;
    *(u32 *)0x00764790 = 0;
    if (p4c_003e3110_003be180(0x0070B060, arg0, block) == 0) {
        p4c_003c9d40_003be180(block);
        return 0;
    }
    if (*(u32 *)0x00764794 != 0) {
        p4c_003e32f0_003be180(0x0070B060, *(u32 *)0x00764794, block, *(u32 *)0x00764790);
    }
    if (p4c_003c9b30_003be180(block) == 0) {
        p4c_003c9d40_003be180(block);
        return 0;
    }
    return (u32)block;
}
