/* func_00398890 (224 bytes): reads an optional object reference from the stream arg0 into the word at arg1. A word is read
 * first: 0 stores 0 and returns arg0. Otherwise a chunk of type 6 is required (func_003df050), the object is
 * func_003e6430(arg0), stored to *arg1, and arg0 is returned when it is non-zero. A null object leaves the error
 * record: func_003df550(&e) fetches it, and e's second word of 0x80000000 or 0x16 still returns arg0 (the object was
 * merely absent), any other value reports it through func_003df4d0(&e) and returns 0. Returns 0 when a read fails. */

extern s32 p4c_003df050_00398890(s32 stream, s32 type, void *size, void *version) __asm__("func_003df050");
extern s32 p4c_003df360_00398890(s32 stream, void *p, s32 n) __asm__("func_003df360");
extern void p4c_003df4d0_00398890(u32 *err) __asm__("func_003df4d0");
extern void p4c_003df550_00398890(u32 *e) __asm__("func_003df550");
extern u32 p4c_003e6430_00398890(s32 s) __asm__("func_003e6430");

s32 func_00398890(s32 arg0, u32 *arg1)
{
    u32 present;
    u32 e[2];

    if (p4c_003df360_00398890(arg0, &present, 4) == 0) {
        return 0;
    }
    if (present == 0) {
        *arg1 = 0;
        return arg0;
    }
    if (p4c_003df050_00398890(arg0, 6, 0, 0) == 0) {
        return 0;
    }
    p4c_003df550_00398890(e);
    *arg1 = p4c_003e6430_00398890(arg0);
    if (*arg1 != 0) {
        return arg0;
    }
    p4c_003df550_00398890(e);
    if (e[1] == 0x80000000 || e[1] == 0x16) {
        return arg0;
    }
    p4c_003df4d0_00398890(e);
    return 0;
}
