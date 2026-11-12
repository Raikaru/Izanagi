/* func_003d5600 (268 bytes): writes an animation object (arg0) to a stream (arg1). Returns 1, or 0 when a
 * write fails (func_003df240 writes a word, func_003df1a0 a float, both return 0 on failure).
 *
 * func_003deff0(stream, 0x1B, func_003d5710(arg0), 0x37002, 0x37) opens the chunk. The words written
 * are the magic 0x100, the type id (the first word of the type record that arg0[0] points to), arg0[4],
 * arg0[8] and the float arg0[0xC]. The type's writer (slot +0x24 of the type record) is then called as
 * writer(arg0, stream). */

extern s32 p4c_003d5710_003d5600(u8 *obj) __asm__("func_003d5710");
extern s32 p4c_003deff0_003d5600(s32 stream, s32 type, s32 size, s32 version, s32 lib) __asm__("func_003deff0");
extern s32 p4c_003df1a0_003d5600(s32 stream, void *p, s32 n) __asm__("func_003df1a0");
extern s32 p4c_003df240_003d5600(s32 stream, void *p, s32 n) __asm__("func_003df240");

typedef void (*P4WriteFn_003d5600)(u8 *obj, s32 stream);

s32 func_003d5600(u8 *arg0, s32 arg1)
{
    s32 magic;

    magic = 0x100;
    p4c_003deff0_003d5600(arg1, 0x1B, p4c_003d5710_003d5600(arg0), 0x37002, 0x37);
    if (p4c_003df240_003d5600(arg1, &magic, 4) == 0) {
        return 0;
    }
    if (p4c_003df240_003d5600(arg1, *(s32 **)arg0, 4) == 0) {
        return 0;
    }
    if (p4c_003df240_003d5600(arg1, (s32 *)(arg0 + 4), 4) == 0) {
        return 0;
    }
    if (p4c_003df240_003d5600(arg1, (s32 *)(arg0 + 8), 4) == 0) {
        return 0;
    }
    if (p4c_003df1a0_003d5600(arg1, (s32 *)(arg0 + 0xC), 4) == 0) {
        return 0;
    }
    (*(P4WriteFn_003d5600 *)(*(u8 **)arg0 + 0x24))(arg0, arg1);
    return 1;
}
