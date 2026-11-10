/* func_003dbbc0 (640 bytes): reads a chunk of type 1 from the stream arg0 and builds an object from it; returns the object (0 on failure). The
 * chunk's version must be in [0x35000, 0x37003) (otherwise the error {0x1B3, func_003df590(0x80000004)} goes to func_003df4d0). Its body (size bytes, a local
 * word of 4 that starts as 0) holds: byte 0 = kind, nibble at bit 8 = A, nibble at bit 12 = B (set to A when 0), byte at bit 16 = flags. The renderer state is
 * switched for the read (func_003ef000 / func_003ef040 with 1/1 or 1/0 for kinds 3 to 6, 0/0 otherwise, bit 0 of the flags chooses the second value), two
 * records of 0x80 bytes are read from the stream (func_003e4f60), the object is created with func_003ef2e0(0) and takes the records with
 * func_003ef470 / func_003ef510; when the object's word +0x54 is 1 its word +0x50 gets the low half of the body word. The renderer state saved
 * by func_003ef020 / func_003ef060 is restored on every exit after the switch. When creation fails, func_003e3560(0x0070B800, stream) is called. */

extern s32 p4c_003df050_003dbbc0(s32 stream, s32 type, void *size, void *version) __asm__("func_003df050");
extern void p4c_003df4d0_003dbbc0(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003dbbc0(s32 code, ...) __asm__("func_003df590");
extern s32 p4c_003e2910_003dbbc0(s32 stream, void *buf, s32 n) __asm__("func_003e2910");
extern void p4c_003e3560_003dbbc0(u32 a, u32 b) __asm__("func_003e3560");
extern s32 p4c_003e4f60_003dbbc0(void *rec, u32 stream) __asm__("func_003e4f60");
extern void p4c_003ef000_003dbbc0(u32 a) __asm__("func_003ef000");
extern u32 p4c_003ef020_003dbbc0(void) __asm__("func_003ef020");
extern void p4c_003ef040_003dbbc0(u32 a) __asm__("func_003ef040");
extern u32 p4c_003ef060_003dbbc0(void) __asm__("func_003ef060");
extern u32 p4c_003ef2e0_003dbbc0(u32 a) __asm__("func_003ef2e0");
extern void p4c_003ef470_003dbbc0(u32 o, void *rec) __asm__("func_003ef470");
extern void p4c_003ef510_003dbbc0(u32 o, void *rec) __asm__("func_003ef510");

u32 func_003dbbc0(u32 arg0)
{
    u32 size = 0;
    u32 version = 0;
    u32 body = 0;
    u32 recA[32];
    u32 recB[32];
    u32 savedA;
    u32 savedB;
    u32 kind;
    u32 a;
    u32 flags;
    u32 obj;
    s32 err[2];
    u32 i;

    for (i = 0; i < 32; i++) {
        recA[i] = 0;
        recB[i] = 0;
    }
    if (p4c_003df050_003dbbc0(arg0, 1, &size, &version) == 0) {
        return 0;
    }
    if (version < 0x35000 || !(version < 0x37003)) {
        err[0] = 0x1B3;
        err[1] = p4c_003df590_003dbbc0(0x80000004);
        p4c_003df4d0_003dbbc0(err);
        return 0;
    }
    if (p4c_003e2910_003dbbc0(arg0, &body, size) != (s32)size) {
        return 0;
    }
    kind = body & 0xFF;
    a = (body >> 8) & 0xF;
    if (((body >> 12) & 0xF) == 0) {
        body = body | (a << 12);
    }
    flags = (body >> 16) & 0xFF;
    savedA = p4c_003ef020_003dbbc0();
    savedB = p4c_003ef060_003dbbc0();
    if (kind - 3 < 4) {
        p4c_003ef000_003dbbc0(1);
        if ((flags & 1) != 0) {
            p4c_003ef040_003dbbc0(0);
        } else {
            p4c_003ef040_003dbbc0(1);
        }
    } else {
        p4c_003ef000_003dbbc0(0);
        p4c_003ef040_003dbbc0(0);
    }
    if (p4c_003e4f60_003dbbc0(recA, arg0) == 0) {
        p4c_003ef000_003dbbc0(savedA);
        p4c_003ef040_003dbbc0(savedB);
        return 0;
    }
    if (p4c_003e4f60_003dbbc0(recB, arg0) == 0) {
        p4c_003ef000_003dbbc0(savedA);
        p4c_003ef040_003dbbc0(savedB);
        return 0;
    }
    obj = p4c_003ef2e0_003dbbc0(0);
    if (obj == 0) {
        p4c_003e3560_003dbbc0(0x0070B800, arg0);
        p4c_003ef000_003dbbc0(savedA);
        p4c_003ef040_003dbbc0(savedB);
        return 0;
    }
    p4c_003ef470_003dbbc0(obj, recA);
    p4c_003ef510_003dbbc0(obj, recB);
    if (*(s32 *)(obj + 0x54) == 1) {
        *(u32 *)(obj + 0x50) = body & 0xFFFF;
    }
    p4c_003ef000_003dbbc0(savedA);
    p4c_003ef040_003dbbc0(savedB);
    return obj;
}
