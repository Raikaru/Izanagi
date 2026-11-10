/* func_003cfa80 (1312 bytes): appends a light/shadow record for the object arg0 to the packet buffer. The write cursor is the pointer at 0x00764814 (gp-0x48DC; gp is
 * 0x007690F0) and the quadword count the word at 0x00764810 (gp-0x48E0). Nothing is written once the count reaches 0x7C. The first record of a buffer (count 0) starts with
 * a 4 x 4 block of the matrix arg1 (rows of three floats followed by arg2, 16-byte rows; the last row ends with arg3) or, for a null arg1, the identity with the same
 * arg2 / arg3 entries (cursor += 0x40, count += 4). Then the header {x, y, z, type} is written at the cursor without advancing: the position is that of the object's frame,
 * arg0[+0x18 .. +0x20] (three floats), the type is the byte at arg0 + 1 (nothing more is written for type 0). The cursor then moves past the header and, per type: 1: the frame's row
 * at +0x20 and 0 (cursor += 0x20, count += 2); 2: nothing (cursor += 0x10, count += 1); 0x80: the frame position (+0x30) and the float at arg0 + 0x14
 * (cursor += 0x20, count += 2); 0x81 / 0x82: the frame position and arg0[+0x14], then the row at +0x20 and arg0[+0x28] (cursor += 0x30, count += 3). Other types leave the cursor
 * in place. The frame matrix is func_003e9700(arg0[1]); when the frame is 0 the reads of its rows give 0 (retail would fault). */

extern u8 *p4c_003e9700_003cfa80(u32 frame) __asm__("func_003e9700");

void func_003cfa80(u8 *arg0, u32 *arg1, f32 arg2, f32 arg3)
{
    u32 type = *(u8 *)(arg0 + 1);
    u32 frame = *(u32 *)(arg0 + 4);
    u32 *g;
    u32 *row = (u32 *)0;
    u32 *pos = (u32 *)0;
    u32 f2 = *(u32 *)&arg2;
    u32 f3 = *(u32 *)&arg3;
    u32 zero[3] = {0, 0, 0};
    u32 *m;

    if (!(*(u32 *)0x00764810 < 0x7C)) {
        return;
    }
    if (*(u32 *)0x00764810 == 0) {
        g = *(u32 **)0x00764814;
        if (arg1 != (u32 *)0) {
            g[0] = arg1[0];
            g[1] = arg1[1];
            g[2] = arg1[2];
            g[3] = f2;
            g[4] = arg1[4];
            g[5] = arg1[5];
            g[6] = arg1[6];
            g[7] = f2;
            g[8] = arg1[8];
            g[9] = arg1[9];
            g[10] = arg1[10];
            g[11] = f2;
            g[12] = arg1[12];
            g[13] = arg1[13];
            g[14] = arg1[14];
            g[15] = f3;
        } else {
            g[0] = 0x3F800000;
            g[1] = 0;
            g[2] = 0;
            g[3] = f2;
            g[4] = 0;
            g[5] = 0x3F800000;
            g[6] = 0;
            g[7] = f2;
            g[8] = 0;
            g[9] = 0;
            g[10] = 0x3F800000;
            g[11] = f2;
            g[12] = 0;
            g[13] = 0;
            g[14] = 0;
            g[15] = f3;
        }
        *(u32 **)0x00764814 = g + 16;
        *(u32 *)0x00764810 = *(u32 *)0x00764810 + 4;
    }
    if (frame != 0) {
        m = (u32 *)p4c_003e9700_003cfa80(frame);
        row = m + 8;
        pos = m + 12;
    } else {
        row = zero;
        pos = zero;
    }
    g = *(u32 **)0x00764814;
    g[0] = *(u32 *)(arg0 + 0x18);
    g[1] = *(u32 *)(arg0 + 0x1C);
    g[2] = *(u32 *)(arg0 + 0x20);
    g[3] = type;
    if (type == 0) {
        return;
    }
    if (type == 0x82 || type == 0x81) {
        g[4] = pos[0];
        g[5] = pos[1];
        g[6] = pos[2];
        g[7] = *(u32 *)(arg0 + 0x14);
        g[8] = row[0];
        g[9] = row[1];
        g[10] = row[2];
        g[11] = *(u32 *)(arg0 + 0x28);
        *(u32 **)0x00764814 = g + 12;
        *(u32 *)0x00764810 = *(u32 *)0x00764810 + 3;
    } else if (type == 0x80) {
        g[4] = pos[0];
        g[5] = pos[1];
        g[6] = pos[2];
        g[7] = *(u32 *)(arg0 + 0x14);
        *(u32 **)0x00764814 = g + 8;
        *(u32 *)0x00764810 = *(u32 *)0x00764810 + 2;
    } else if (type == 2) {
        *(u32 **)0x00764814 = g + 4;
        *(u32 *)0x00764810 = *(u32 *)0x00764810 + 1;
    } else if (type == 1) {
        g[4] = row[0];
        g[5] = row[1];
        g[6] = row[2];
        g[7] = 0;
        *(u32 **)0x00764814 = g + 8;
        *(u32 *)0x00764810 = *(u32 *)0x00764810 + 2;
    }
}
