/* func_003b8ad0 (592 bytes): reads one shape block (the counterpart of func_003b7680) from the stream arg0 and hands it to
 * func_003b8410(arg1, block); returns arg0, or 0 when anything fails. Chunk type 1 with a version in [0x35000, 0x37003) (else {0x116,
 * func_003df590(0x80000004)} is reported through func_003df4d0), then the platform word 4. A 0x48-byte record is allocated from the pool
 * *(u32 *)0x008864A8 (tag 0x30116, slot 0x008873F8) and cleared; a packed word gives rec[0] = byte 0 (X), rec[1] = byte 1 (S) and rec[4] = byte 2.
 * A block of X * 64 + S + 15 bytes (tag 0x30116) holds the S-byte part at rec[2] and the X matrices at rec[3] (16-byte aligned, rec[2] is
 * rec[3] + 64 * X), both read with func_003e2910; 16 bytes go to rec + 0x1C (func_003df360 must return the stream), func_003b7290(stream, rec)
 * finishes the record. */

extern s32 p4c_003b7290_003b8ad0(s32 s, u32 *r) __asm__("func_003b7290");
extern void p4c_003b8410_003b8ad0(u32 a, u32 *r) __asm__("func_003b8410");
extern s32 p4c_003df050_003b8ad0(s32 stream, s32 type, void *size, void *version) __asm__("func_003df050");
extern s32 p4c_003df360_003b8ad0(s32 stream, void *p, s32 n) __asm__("func_003df360");
extern void p4c_003df4d0_003b8ad0(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003b8ad0(s32 code, ...) __asm__("func_003df590");
extern s32 p4c_003e2910_003b8ad0(s32 s, void *b, s32 n) __asm__("func_003e2910");
extern void p4c_0043f9c8_003b8ad0(void *p, s32 v, s32 n) __asm__("func_0043f9c8");

typedef void *(*P4PoolAllocFn_003b8ad0)(u32 pool, u32 tag);
typedef void *(*P4AllocFn_003b8ad0)(u32 size, u32 tag);

s32 func_003b8ad0(s32 arg0, u32 arg1)
{
    u32 size;
    u32 version;
    u32 platform;
    u32 packed;
    s32 err[2];
    u32 *rec;
    u32 x;
    u32 s;
    s32 n;

    if (p4c_003df050_003b8ad0(arg0, 1, &size, &version) == 0) {
        return 0;
    }
    if (version < 0x35000 || !(version < 0x37003)) {
        err[0] = 0x116;
        err[1] = p4c_003df590_003b8ad0(0x80000004);
        p4c_003df4d0_003b8ad0(err);
        return 0;
    }
    if (p4c_003df360_003b8ad0(arg0, &platform, 4) == 0) {
        return 0;
    }
    if (platform != 4) {
        return 0;
    }
    rec = (u32 *)(*(P4PoolAllocFn_003b8ad0 *)0x008873F8)(*(u32 *)0x008864A8, 0x30116);
    p4c_0043f9c8_003b8ad0(rec, 0, 0x48);
    if (p4c_003df360_003b8ad0(arg0, &packed, 4) == 0) {
        return 0;
    }
    rec[0] = packed & 0xFF;
    rec[1] = (packed >> 8) & 0xFF;
    rec[4] = (packed >> 16) & 0xFF;
    x = rec[0];
    s = rec[1];
    rec[17] = (u32)(*(P4AllocFn_003b8ad0 *)0x008873E8)(x * 64 + s + 15, 0x30116);
    rec[3] = (rec[17] + 15) & ~15u;
    rec[2] = rec[3] + x * 64;
    n = p4c_003e2910_003b8ad0(arg0, (void *)rec[2], (s32)s);
    if ((u32)n != s) {
        return 0;
    }
    n = p4c_003e2910_003b8ad0(arg0, (void *)rec[3], (s32)(x * 64));
    if ((u32)n != x * 64) {
        return 0;
    }
    if (p4c_003df360_003b8ad0(arg0, rec + 7, 16) != arg0) {
        return 0;
    }
    if (p4c_003b7290_003b8ad0(arg0, rec) == 0) {
        return 0;
    }
    p4c_003b8410_003b8ad0(arg1, rec);
    return arg0;
}
