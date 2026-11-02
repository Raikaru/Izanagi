/* func_00398970 (944 bytes): writes the collision record of the object arg2 to the stream arg0 (arg1 is not used); the
 * counterpart of func_00398d20. Returns arg0, or 0 as soon as a write fails.
 *
 * The record is the pointer at arg2 + the plugin offset in 0x007646D0. The mode word (+0x80) goes first; then for each of
 * the two 0x40-byte entries (all words written with func_003df240 (stream, &word, 4), floats and blocks with func_003df1a0): the type
 * word (+0x20), followed by type 1: the float -(entry + 0xC), the object words +4 and +8 each as a presence word
 * (non-zero if set) followed, when set, by func_003e62b0(object, stream); type 2: the float at +8, the word at +0xC,
 * the object word +4 as presence plus func_003e62b0; type 4: the words +4 and +8, then the object word +0 as presence plus
 * func_003e62b0. Types 0 and 5 and anything else write nothing more. */

extern s32 p4c_003df1a0_00398970(s32 stream, void *p, s32 n) __asm__("func_003df1a0");
extern s32 p4c_003df240_00398970(s32 stream, void *p, s32 n) __asm__("func_003df240");
extern s32 p4c_003e62b0_00398970(u32 o, s32 s) __asm__("func_003e62b0");

s32 func_00398970(s32 arg0, s32 arg1, u8 *arg2)
{
    u8 *rec = *(u8 **)(arg2 + *(s32 *)0x007646D0);
    u32 w;
    u32 type;
    u32 present;
    f32 fl;
    u8 *e;
    u32 i;
    u32 obj;

    w = *(u32 *)(rec + 0x80);
    if (p4c_003df240_00398970(arg0, &w, 4) == 0) {
        return 0;
    }
    for (i = 0; i < 2; i++) {
        e = rec + (i << 6);
        type = *(u32 *)(e + 0x20);
        if (p4c_003df240_00398970(arg0, &type, 4) == 0) {
            return 0;
        }
        switch (type) {
        case 4:
            w = *(u32 *)(e + 4);
            if (p4c_003df240_00398970(arg0, &w, 4) == 0) {
                return 0;
            }
            w = *(u32 *)(e + 8);
            if (p4c_003df240_00398970(arg0, &w, 4) == 0) {
                return 0;
            }
            obj = *(u32 *)e;
            present = obj != 0;
            if (p4c_003df240_00398970(arg0, &present, 4) == 0) {
                return 0;
            }
            if (present != 0 && p4c_003e62b0_00398970(obj, arg0) == 0) {
                return 0;
            }
            break;
        case 2:
            if (p4c_003df1a0_00398970(arg0, e + 8, 4) == 0) {
                return 0;
            }
            w = *(u32 *)(e + 0xC);
            if (p4c_003df240_00398970(arg0, &w, 4) == 0) {
                return 0;
            }
            obj = *(u32 *)(e + 4);
            present = obj != 0;
            if (p4c_003df240_00398970(arg0, &present, 4) == 0) {
                return 0;
            }
            if (present != 0 && p4c_003e62b0_00398970(obj, arg0) == 0) {
                return 0;
            }
            break;
        case 1:
            fl = -*(f32 *)(e + 0xC);
            if (p4c_003df1a0_00398970(arg0, &fl, 4) == 0) {
                return 0;
            }
            obj = *(u32 *)(e + 4);
            present = obj != 0;
            if (p4c_003df240_00398970(arg0, &present, 4) == 0) {
                return 0;
            }
            if (present != 0 && p4c_003e62b0_00398970(obj, arg0) == 0) {
                return 0;
            }
            obj = *(u32 *)(e + 8);
            present = obj != 0;
            if (p4c_003df240_00398970(arg0, &present, 4) == 0) {
                return 0;
            }
            if (present != 0 && p4c_003e62b0_00398970(obj, arg0) == 0) {
                return 0;
            }
            break;
        default:
            break;
        }
    }
    return arg0;
}
