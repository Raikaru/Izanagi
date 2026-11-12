/* func_003c3ae0 (472 bytes): reads one light-like chunk from a stream. arg0 is the
 * stream. Returns the new object, or 0.
 *
 * func_003df050(stream, 1, &size, &version) finds the chunk (0 on failure). A version below 0x35000 or
 * at or above 0x37003 reports error {2, func_003df590(0x80000004)} through func_003df4d0 and returns 0.
 * The 0x18-byte body buffer is zeroed (func_0043f9c8, the port's memset) and `size` bytes are read with
 * func_003e2910 (the byte count must match). Body word 5 (+0x14) holds the type in bits 16..23;
 * func_003c3d20(type) makes the object. Its floats +0x14, +0x18, +0x1C and +0x20 are the body floats 0..3
 * and +0x28 is body float 4, except for versions below 0x30300 where it is
 * -sqrt(1 / (1 + f4 * f4)). Object byte +3 is 1 when the floats at +0x18, +0x1C and +0x20 are equal, else
 * 0; byte +2 is the low byte of body word 5. func_003e3110(0x0070AFD0, stream, object) must succeed
 * (non-zero) or 0 is returned; otherwise the object is returned. */

extern u8 *p4c_003c3d20_003c3ae0(u32 type) __asm__("func_003c3d20");
extern s32 p4c_003df050_003c3ae0(s32 stream, s32 type, void *size, void *version) __asm__("func_003df050");
extern void p4c_003df4d0_003c3ae0(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003c3ae0(s32 code, ...) __asm__("func_003df590");
extern s32 p4c_003e2910_003c3ae0(s32 stream, void *buf, s32 n) __asm__("func_003e2910");
extern s32 p4c_003e3110_003c3ae0(u32 list, s32 stream, void *obj) __asm__("func_003e3110");
extern void *p4c_0043f9c8_003c3ae0(void *dst, s32 c, u32 n) __asm__("func_0043f9c8");

s32 func_003c3ae0(s32 arg0)
{
    u32 size;
    u32 version;
    u32 body[6];
    f32 *bf;
    u8 *obj;
    u32 w;

    if (p4c_003df050_003c3ae0(arg0, 1, &size, &version) == 0) {
        return 0;
    }
    if (version < 0x35000 || !(version < 0x37003)) {
        s32 err[2];

        err[0] = 2;
        err[1] = p4c_003df590_003c3ae0(0x80000004);
        p4c_003df4d0_003c3ae0(err);
        return 0;
    }
    p4c_0043f9c8_003c3ae0(body, 0, 0x18);
    if (p4c_003e2910_003c3ae0(arg0, body, size) != (s32)size) {
        return 0;
    }
    bf = (f32 *)body;
    w = body[5];
    obj = (u8 *)p4c_003c3d20_003c3ae0((w >> 16) & 0xFF);
    if (obj == (u8 *)0) {
        return 0;
    }
    *(f32 *)(obj + 0x14) = bf[0];
    *(f32 *)(obj + 0x18) = bf[1];
    *(f32 *)(obj + 0x1C) = bf[2];
    *(f32 *)(obj + 0x20) = bf[3];
    if (version < 0x30300) {
        f32 t;

        t = 0.0f + 1.0f;
        t = t + bf[4] * bf[4];
        t = 1.0f / t;
        *(f32 *)(obj + 0x28) = -__builtin_sqrtf(t);
    } else {
        *(f32 *)(obj + 0x28) = bf[4];
    }
    if (*(f32 *)(obj + 0x18) == *(f32 *)(obj + 0x1C) && *(f32 *)(obj + 0x18) == *(f32 *)(obj + 0x20)) {
        *(u8 *)(obj + 3) = 1;
    } else {
        *(u8 *)(obj + 3) = 0;
    }
    *(u8 *)(obj + 2) = (u8)w;
    if (p4c_003e3110_003c3ae0(0x0070AFD0, arg0, obj) == 0) {
        return 0;
    }
    return (s32)obj;
}
