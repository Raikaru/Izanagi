/* func_00398d20 (896 bytes): reads the collision record of the object arg2 from the stream arg0 (arg1 is not used); the
 * counterpart of func_00398970. Returns arg0, or 0 as soon as a read fails.
 *
 * A missing record is allocated and zeroed as in func_00399bf0 (0x90 bytes, pool *(u32 *)0x00884A80, tag 0x30120, slot
 * 0x008873F8). The mode word is read and applied with func_00399bf0(arg2, mode). Then for each of the two 0x40-byte
 * entries a type word is read and, by type: 1: a float f (func_003df300, 4 bytes) and two object references (func_00398890)
 * a and b; when a is non-zero the entry gets +4 = a, +8 = b and +0x10 = 1.0 / (float)(**(entry + 4))[3], otherwise a
 * non-zero b is bound through func_00399db0(arg2, b) and released with func_003ef3a0(b) (both zero clear +4 and +8);
 * a failed second reference releases a; func_0039a030(arg2, f) closes the case. Type 2: a float f, a word w and a reference r; a non-zero r goes to
 * func_0039a150(arg2, r) and is released, then func_0039a2e0(arg2, f) and func_0039a260(arg2, w). Type 4: two words
 * (read together as 8 bytes) and a reference r; a non-zero r goes to func_0039a4c0(arg2, r) and is released, then
 * func_0039a590(arg2, word0, word1). Other types read nothing more. */

extern s32 p4c_00398890_00398d20(s32 s, u32 *o) __asm__("func_00398890");
extern u32 p4c_00399bf0_00398d20(u8 *o, u32 m) __asm__("func_00399bf0");
extern u8 *p4c_00399db0_00398d20(u8 *o, u32 t) __asm__("func_00399db0");
extern void p4c_0039a030_00398d20(u8 *o, f32 f) __asm__("func_0039a030");
extern void p4c_0039a150_00398d20(u8 *o, u32 v) __asm__("func_0039a150");
extern void p4c_0039a260_00398d20(u8 *o, u32 v) __asm__("func_0039a260");
extern void p4c_0039a2e0_00398d20(u8 *o, f32 f) __asm__("func_0039a2e0");
extern void p4c_0039a4c0_00398d20(u8 *o, u32 v) __asm__("func_0039a4c0");
extern s32 p4c_0039a590_00398d20(u8 *o, u32 a, u32 b) __asm__("func_0039a590");
extern s32 p4c_003df300_00398d20(s32 stream, void *p, s32 n) __asm__("func_003df300");
extern s32 p4c_003df360_00398d20(s32 stream, void *p, s32 n) __asm__("func_003df360");
extern void p4c_003ef3a0_00398d20(u32 t) __asm__("func_003ef3a0");
extern void p4c_0043f9c8_00398d20(u8 *p, s32 v, s32 n) __asm__("func_0043f9c8");

typedef void *(*P4PoolAllocFn_00398d20)(u32 pool, u32 tag);

s32 func_00398d20(s32 arg0, s32 arg1, u8 *arg2)
{
    u8 *rec;
    u32 mode;
    u32 type;
    u32 i;
    u8 *e;
    f32 f;
    u32 a;
    u32 b;
    u32 w;
    u32 pair[2];
    u32 r;

    rec = *(u8 **)(arg2 + *(s32 *)0x007646D0);
    if (rec == (u8 *)0) {
        rec = (u8 *)(*(P4PoolAllocFn_00398d20 *)0x008873F8)(*(u32 *)0x00884A80, 0x30120);
        if (rec != (u8 *)0) {
            p4c_0043f9c8_00398d20(rec, 0, 0x90);
            *(u8 **)(arg2 + *(s32 *)0x007646D0) = rec;
        }
    }
    if (rec == (u8 *)0) {
        return 0;
    }
    if (p4c_003df360_00398d20(arg0, &mode, 4) == 0) {
        return 0;
    }
    p4c_00399bf0_00398d20(arg2, mode);
    for (i = 0; i < 2; i++) {
        if (p4c_003df360_00398d20(arg0, &type, 4) == 0) {
            return 0;
        }
        switch (type) {
        case 4:
            r = 0;
            if (p4c_003df360_00398d20(arg0, pair, 8) == 0) {
                return 0;
            }
            if (p4c_00398890_00398d20(arg0, &r) == 0) {
                return 0;
            }
            if (r != 0) {
                p4c_0039a4c0_00398d20(arg2, r);
                p4c_003ef3a0_00398d20(r);
            }
            p4c_0039a590_00398d20(arg2, pair[0], pair[1]);
            break;
        case 2:
            r = 0;
            if (p4c_003df300_00398d20(arg0, &f, 4) == 0) {
                return 0;
            }
            if (p4c_003df360_00398d20(arg0, &w, 4) == 0) {
                return 0;
            }
            if (p4c_00398890_00398d20(arg0, &r) == 0) {
                return 0;
            }
            if (r != 0) {
                p4c_0039a150_00398d20(arg2, r);
                p4c_003ef3a0_00398d20(r);
            }
            p4c_0039a2e0_00398d20(arg2, f);
            p4c_0039a260_00398d20(arg2, w);
            break;
        case 1:
            e = rec + (i << 6);
            a = 0;
            b = 0;
            if (p4c_003df300_00398d20(arg0, &f, 4) == 0) {
                return 0;
            }
            if (p4c_00398890_00398d20(arg0, &a) == 0) {
                return 0;
            }
            if (p4c_00398890_00398d20(arg0, &b) == 0) {
                if (a != 0) {
                    p4c_003ef3a0_00398d20(a);
                }
                return 0;
            }
            if (a != 0) {
                *(u32 *)(e + 4) = a;
                *(u32 *)(e + 8) = b;
                *(f32 *)(e + 0x10) = 1.0f / (f32)*(s32 *)(**(u8 ***)(e + 4) + 0xC);
            } else if (b != 0) {
                p4c_00399db0_00398d20(arg2, b);
                p4c_003ef3a0_00398d20(b);
            } else {
                *(u32 *)(e + 4) = 0;
                *(u32 *)(e + 8) = 0;
            }
            p4c_0039a030_00398d20(arg2, f);
            break;
        default:
            break;
        }
    }
    return arg0;
}
