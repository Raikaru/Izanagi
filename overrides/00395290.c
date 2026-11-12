/* func_00395290 (532 bytes): reads a table of per-slot entries from the stream arg0 and returns the object func_00394fd0
 * built for it, or 0 on failure.
 *
 * A chunk of type 1 is opened, then four words are read: a (must be at most 0x100, else the error record {0x122,
 * func_003df590(0x80000004)} is reported through func_003df4d0 and 0 returned), b, c and d; b and d are
 * not used further. The object is func_00394fd0(c) (0 when it fails). For each of the c slots a count e
 * is read; when e is non-zero func_00395100(object, slot, e) must succeed (it returns the destination list)
 * and the e entries of 20 bytes at *(object[1] + slot * 8 + 4) are read: 16 bytes (func_003df300) followed by a word at +0x10
 * (func_003df360). Any read failure returns 0. */

extern u8 *p4c_00394fd0_00395290(u32 n) __asm__("func_00394fd0");
extern s32 p4c_00395100_00395290(u8 *o, u32 i, u32 e) __asm__("func_00395100");
extern s32 p4c_003df050_00395290(s32 stream, s32 type, void *size, void *version) __asm__("func_003df050");
extern s32 p4c_003df300_00395290(s32 stream, void *p, s32 n) __asm__("func_003df300");
extern s32 p4c_003df360_00395290(s32 stream, void *p, s32 n) __asm__("func_003df360");
extern void p4c_003df4d0_00395290(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_00395290(s32 code, ...) __asm__("func_003df590");

u32 func_00395290(s32 arg0)
{
    u32 size;
    u32 version;
    u32 a;
    u32 b;
    u32 c;
    u32 d;
    u32 e;
    u32 i;
    u32 j;
    u8 *obj;
    u8 *ent;
    s32 err[2];

    if (p4c_003df050_00395290(arg0, 1, &size, &version) == 0) {
        return 0;
    }
    if (p4c_003df360_00395290(arg0, &a, 4) == 0) {
        return 0;
    }
    if (p4c_003df360_00395290(arg0, &b, 4) == 0) {
        return 0;
    }
    if (p4c_003df360_00395290(arg0, &c, 4) == 0) {
        return 0;
    }
    if (p4c_003df360_00395290(arg0, &d, 4) == 0) {
        return 0;
    }
    if (!(a <= 0x100)) {
        err[0] = 0x122;
        err[1] = p4c_003df590_00395290(0x80000004);
        p4c_003df4d0_00395290(err);
        return 0;
    }
    obj = (u8 *)p4c_00394fd0_00395290(c);
    if (obj == (u8 *)0) {
        return 0;
    }
    if (c == 0) {
        return (u32)obj;
    }
    for (i = 0; i < c; i++) {
        if (p4c_003df360_00395290(arg0, &e, 4) == 0) {
            return 0;
        }
        if (e != 0) {
            if (p4c_00395100_00395290(obj, i, e) == 0) {
                return 0;
            }
            ent = *(u8 **)(*(u8 **)(obj + 4) + i * 8 + 4);
            for (j = 0; j < e; j++) {
                if (p4c_003df300_00395290(arg0, ent, 0x10) == 0) {
                    return 0;
                }
                if (p4c_003df360_00395290(arg0, ent + 0x10, 4) == 0) {
                    return 0;
                }
                ent += 0x14;
            }
        }
    }
    return (u32)obj;
}
