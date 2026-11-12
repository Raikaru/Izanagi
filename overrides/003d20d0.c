/* func_003d20d0 (368 bytes): sets up one draw record of a request (arg0) and the destination matrix pointer arg1. arg0[2] is
 * the source record: func_003d2010(source, arg0) fills the two request records, arg0[5] is set to 0x7FFF8, and *arg1 becomes the camera's
 * matrix pointer (the camera is the pointer at 0x008872E0, +0x20) when source[1] is 0, else func_003e05f0(*arg1, source[1], camera + 0x20) is called.
 * The draw mode bytes +0x28 / +0x29 / +0x2A of arg0 are chosen from the word t[0] of arg0[3] and the default byte at gp - 0x467C (0x00764A74):
 * t bit 0: +0x29 = 4, +0x28 = byte; low byte 0 or bit 1: 3 and byte | 4; bit 0x10: 4 and byte; bit 8: 2 and byte | 0x10; otherwise 1 and
 * byte | 0x14. Bit 2 of source[0] gives the mode 2 (otherwise 1); a mode other than 2 ORs 2 into +0x28; +0x2A is set to 1. Returns nothing. */

extern void p4c_003d2010_003d20d0(u8 *s, u8 *d) __asm__("func_003d2010");
extern void p4c_003e05f0_003d20d0(void *d, void *a, void *b) __asm__("func_003e05f0");

void func_003d20d0(u8 *arg0, u32 **arg1)
{
    u32 *source = *(u32 **)(arg0 + 8);
    u8 *camera;
    u32 *t;
    u32 w;
    u32 mode;
    u8 byte;

    p4c_003d2010_003d20d0((u8 *)source, arg0);
    *(u32 *)(arg0 + 0x14) = 0x7FFF8;
    camera = *(u8 **)0x008872E0;
    if (source[1] == 0) {
        *arg1 = (u32 *)(camera + 0x20);
    } else {
        p4c_003e05f0_003d20d0(*arg1, (void *)source[1], camera + 0x20);
    }
    mode = ((source[0] & 4) != 0) ? 2 : 1;
    t = *(u32 **)(arg0 + 0xC);
    w = t[0];
    byte = *(u8 *)0x00764A74;
    if ((w & 1) == 1) {
        arg0[0x29] = 4;
        arg0[0x28] = byte;
    } else if ((w & 0xFF) == 0 || (w & 2) == 2) {
        arg0[0x29] = 3;
        arg0[0x28] = (u8)(byte | 4);
    } else if ((w & 0x10) == 0x10) {
        arg0[0x29] = 4;
        arg0[0x28] = byte;
    } else if ((w & 8) == 8) {
        arg0[0x29] = 2;
        arg0[0x28] = (u8)(byte | 0x10);
    } else {
        arg0[0x29] = 1;
        arg0[0x28] = (u8)(byte | 0x14);
    }
    if (mode != 2) {
        arg0[0x28] = (u8)(arg0[0x28] | 2);
    }
    arg0[0x2A] = 1;
}
