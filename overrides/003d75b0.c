/* func_003d75b0 (288 bytes): writes the animation slot table of the object arg2 to the stream arg0 (arg1 is not used) and returns arg0, or 0
 * when a header or the mask write fails. The table is at arg2 + the offset word at gp - 0x48B4 (0x0076483C); its eight slot words start at +8. A chunk of type 1
 * (func_003deff0(stream, 1, 4 + 0x20 * used, 0x37002, 0x37)) holds the bit mask of the used slots (func_003df240, 4 bytes) and, for each used slot,
 * the 0x20-byte record func_003d8060(slot[0]) (written with func_003e2ab0; a failure there is not checked, as in retail). */

extern u32 p4c_003d8060_003d75b0(u32 a) __asm__("func_003d8060");
extern s32 p4c_003deff0_003d75b0(s32 s, s32 t, u32 size, u32 ver, u32 x) __asm__("func_003deff0");
extern s32 p4c_003df240_003d75b0(s32 s, void *p, s32 n) __asm__("func_003df240");
extern s32 p4c_003e2ab0_003d75b0(s32 s, void *p, s32 n) __asm__("func_003e2ab0");

u8 *func_003d75b0(s32 arg0, s32 arg1, u8 *arg2)
{
    u32 *blk = (u32 *)(arg2 + *(s32 *)0x0076483C);
    u32 mask = 0;
    u32 size = 4;
    u32 i;

    for (i = 0; i < 8; i++) {
        if (blk[2 + i] != 0) {
            mask = mask | (1u << i);
            size += 0x20;
        }
    }
    if (p4c_003deff0_003d75b0(arg0, 1, size, 0x37002, 0x37) == 0) {
        return (u8 *)0;
    }
    if (p4c_003df240_003d75b0(arg0, &mask, 4) == 0) {
        return (u8 *)0;
    }
    for (i = 0; i < 8; i++) {
        if (blk[2 + i] != 0) {
            p4c_003e2ab0_003d75b0(arg0, (void *)p4c_003d8060_003d75b0(*(u32 *)blk[2 + i]), 0x20);
        }
    }
    return (u8 *)arg0;
}
