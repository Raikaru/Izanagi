/* func_003d5130 (144 bytes): finds a table record by id. The table starts at 0x00886E50 with 0x30-byte
 * records whose first word is the id; the record count is *(s32 *)0x00764824 (gp-0x48CC; gp is
 * 0x007690F0). Returns the record, or reports error {0x1B7, func_003df590(2)} through func_003df4d0
 * and returns 0. */

extern void p4c_003df4d0_003d5130(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003d5130(s32 code, ...) __asm__("func_003df590");

u8 *func_003d5130(u32 arg0)
{
    s32 count;
    s32 i;
    u32 *rec;

    count = *(s32 *)0x00764824;
    rec = (u32 *)0x00886E50;
    for (i = 0; i < count; i++) {
        if (arg0 == rec[0]) {
            return (u8 *)(0x00886E50 + 0x30 * i);
        }
        rec += 12;
    }
    {
        s32 err[2];

        err[0] = 0x1B7;
        err[1] = p4c_003df590_003d5130(2);
        p4c_003df4d0_003d5130(err);
    }
    return (u8 *)0;
}
