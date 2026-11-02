/* func_00390350 (432 bytes): reads one pipeline slot (a bounds block plus its payload) from the stream arg0 into
 * the word at arg2 + arg3 of the object arg2; arg1 is not used. Returns arg0 on success, 0 on failure.
 *
 * A word version is read first. Below 0x36001 a second word follows and the payload is func_0038fe90(stream, second,
 * version - 1, bounds, &done) with bounds a box of six floats: for an object whose byte at +0 is 8 the box is built from the
 * sphere at *(f32 **)(arg2 + 0x5C) (centre x, y, z at +4, +8, +0xC and radius at +0x10: the min corner
 * (x - r, y - r, z - r) is placed in words 3, 4, 5 and the max corner (x + r, y + r, z + r) in words 0, 1, 2), and otherwise
 * the six floats at arg2 + 0x60 are copied as they are. A non-zero `done` returns arg0 straight away; otherwise the result
 * is arg0 when the stored word is non-zero. From 0x36001 a chunk of type 0x2C is required (func_003df050) and the
 * payload is func_0038fd30(stream). The payload is stored into the slot in both cases. */

extern u32 p4c_0038fd30_00390350(s32 s) __asm__("func_0038fd30");
extern u32 p4c_0038fe90_00390350(s32 s, u32 second, u32 v, void *box, void *done) __asm__("func_0038fe90");
extern s32 p4c_003df050_00390350(s32 stream, s32 type, void *size, void *version) __asm__("func_003df050");
extern s32 p4c_003df360_00390350(s32 stream, void *p, s32 n) __asm__("func_003df360");

s32 func_00390350(s32 arg0, s32 arg1, u8 *arg2, s32 arg3)
{
    u32 *slot = (u32 *)(arg2 + arg3);
    u32 version;
    u32 second;
    u32 done;
    f32 box[6];
    f32 *s;

    if (p4c_003df360_00390350(arg0, &version, 4) == 0) {
        return 0;
    }
    if (version < 0x36001) {
        if (p4c_003df360_00390350(arg0, &second, 4) == 0) {
            return 0;
        }
        if (*arg2 == 8) {
            s = *(f32 **)(arg2 + 0x5C);
            box[3] = s[1] - s[4];
            box[4] = s[2] - s[4];
            box[5] = s[3] - s[4];
            box[0] = s[1] + s[4];
            box[1] = s[2] + s[4];
            box[2] = s[3] + s[4];
        } else {
            box[0] = *(f32 *)(arg2 + 0x60);
            box[1] = *(f32 *)(arg2 + 0x64);
            box[2] = *(f32 *)(arg2 + 0x68);
            box[3] = *(f32 *)(arg2 + 0x6C);
            box[4] = *(f32 *)(arg2 + 0x70);
            box[5] = *(f32 *)(arg2 + 0x74);
        }
        done = 0;
        *slot = p4c_0038fe90_00390350(arg0, second, version - 1, box, &done);
        if (done != 0) {
            return arg0;
        }
    } else {
        if (p4c_003df050_00390350(arg0, 0x2C, 0, 0) == 0) {
            return 0;
        }
        *slot = p4c_0038fd30_00390350(arg0);
    }
    if (*slot == 0) {
        return 0;
    }
    return arg0;
}
