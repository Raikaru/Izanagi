/* func_003d77f0 (432 bytes): reads the animation slot table of the object arg2 from the stream arg0 (arg1 is not used; the UVAnim material plugin
 * reader) and returns arg0, or 0 on failure. The table is at arg2 + the offset word at gp - 0x48B4 (0x0076483C); its slots are the words from +8.
 * A chunk of type 1 is opened (func_003df050) and a bit mask read (func_003df360, 4 bytes). For each of the eight slots whose mask bit is set a
 * 0x20-byte record is read (func_003e2910); the animation is looked up with func_003d60d0(0x0070B610) and func_003d6170(that, record), or made
 * with func_003d76d0(record) when there is no such entry or it fails (func_003d7cd0(anim) otherwise); then the slot interpolator func_003d5790(
 * func_003d5750(anim), (*anim)[1]) is made and set up with func_003d5840(it, anim); a failure there releases the animation (func_003d7c50) and
 * returns 0. Finally func_003d7490(table) must succeed, giving arg0 (else 0). */

extern s32 p4c_003d5750_003d77f0(u8 *a) __asm__("func_003d5750");
extern u32 p4c_003d5790_003d77f0(s32 n, u32 s) __asm__("func_003d5790");
extern s32 p4c_003d5840_003d77f0(u32 it, u8 *a) __asm__("func_003d5840");
extern u32 p4c_003d60d0_003d77f0(u32 *t) __asm__("func_003d60d0");
extern u32 *p4c_003d6170_003d77f0(u32 d, u32 *r) __asm__("func_003d6170");
extern u8 *p4c_003d7490_003d77f0(u8 *t) __asm__("func_003d7490");
extern u32 *p4c_003d76d0_003d77f0(u8 *r) __asm__("func_003d76d0");
extern void p4c_003d7c50_003d77f0(u32 a) __asm__("func_003d7c50");
extern void p4c_003d7cd0_003d77f0(u32 a) __asm__("func_003d7cd0");
extern s32 p4c_003df050_003d77f0(s32 stream, s32 type, void *size, void *version) __asm__("func_003df050");
extern s32 p4c_003df360_003d77f0(s32 stream, void *p, s32 n) __asm__("func_003df360");
extern s32 p4c_003e2910_003d77f0(s32 s, void *b, s32 n) __asm__("func_003e2910");

s32 func_003d77f0(s32 arg0, s32 arg1, u8 *arg2)
{
    u32 *blk = (u32 *)(arg2 + *(s32 *)0x0076483C);
    u32 mask;
    u32 rec[8];
    u32 i;
    u32 *anim;
    u32 d;
    u32 it;

    if (p4c_003df050_003d77f0(arg0, 1, 0, 0) == 0) {
        return 0;
    }
    if (p4c_003df360_003d77f0(arg0, &mask, 4) == 0) {
        return 0;
    }
    for (i = 0; i < 8; i++) {
        if ((mask & (1u << i)) == 0) {
            continue;
        }
        if (p4c_003e2910_003d77f0(arg0, rec, 0x20) == 0) {
            return 0;
        }
        d = p4c_003d60d0_003d77f0((u32 *)0x0070B610);
        if (d == 0) {
            anim = p4c_003d76d0_003d77f0((u8 *)rec);
        } else {
            anim = p4c_003d6170_003d77f0(d, rec);
            if (anim != (u32 *)0) {
                p4c_003d7cd0_003d77f0((u32)anim);
            } else {
                anim = p4c_003d76d0_003d77f0((u8 *)rec);
            }
        }
        if (anim == (u32 *)0) {
            return 0;
        }
        it = p4c_003d5790_003d77f0(p4c_003d5750_003d77f0((u8 *)anim), ((u32 *)anim[0])[1]);
        blk[2 + i] = it;
        if (it == 0 || p4c_003d5840_003d77f0(it, (u8 *)anim) == 0) {
            p4c_003d7c50_003d77f0((u32)anim);
            return 0;
        }
    }
    if (p4c_003d7490_003d77f0((u8 *)blk) == 0) {
        return 0;
    }
    return arg0;
}
