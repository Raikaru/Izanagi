/* func_003a93b0 (1232 bytes): culls and batches arg2 instances of a draw object and returns arg0. arg0 + 0x6C is the packet channel, arg0 + 0x1C a position that is transformed by each instance
 * matrix (arg1, 0x40 bytes each; func_003e42a0(&centre, arg0 + 0x1C, matrix)) and arg0[0x28 / 4] a radius (with the matrix flag word & 3 == 3 the radius is used as is, otherwise it is
 * scaled by the square root of the largest squared row length of the matrix). The sphere is tested against the six planes (0x14 bytes: normal, distance) at
 * *(u32 *)0x008872E0 + 0x94 (a distance above the radius culls it) and, if it passes, against the six planes at *(u32 *)0x008872E0 + *(s32 *)0x00764A24 (gp-0x46CC; gp is 0x007690F0) + 0x60 (a plane with distance
 * above -radius makes it "partly inside", else it is fully inside). A visible instance is appended to the batch buffer (0x80 + ... the buffer at 0x00884B20, 0x50 bytes per entry: the matrix with word +0x3C = 1 when partly
 * inside else 0, then the four colour bytes at arg3 + 4 * index as floats / 255.0). Every 0x30 entries, and at the end for the rest, the batch goes through func_0039c900(buffer, count)
 * (its result is the next buffer) and func_004114d0(channel, arg0, 1). */

extern u32 p4c_0039c900_003a93b0(u32 buf, u32 n) __asm__("func_0039c900");
extern void p4c_003e42a0_003a93b0(f32 *o, u8 *s, u8 *m) __asm__("func_003e42a0");
extern void p4c_004114d0_003a93b0(u32 c, u32 o, u32 f) __asm__("func_004114d0");

u8 *func_003a93b0(u8 *arg0, u8 *arg1, u32 arg2, u8 *arg3)
{
    u32 buf = 0x00884B20;
    u32 n = 0;
    u32 chan;
    u32 i;
    u8 *inst = arg1;
    u8 *col = arg3;
    f32 c[4];
    u8 *ent;
    u32 k;

    if (arg2 == 0) {
        return arg0;
    }
    chan = *(u32 *)(arg0 + 0x6C);
    for (i = 0; i < arg2; i++, inst += 0x40, col += 4) {
        f32 *m = (f32 *)inst;
        u32 state;
        f32 *pl;
        f32 r;

        c[0] = c[1] = c[2] = c[3] = 0.0f;
        p4c_003e42a0_003a93b0(c, arg0 + 0x1C, inst);
        if ((*(u32 *)(inst + 0xC) & 3) == 3) {
            c[3] = *(f32 *)(arg0 + 0x28);
        } else {
            f32 l0 = m[1] * m[1];
            f32 l1;
            f32 l2;
            f32 mx;

            l0 = l0 + m[0] * m[0];
            l0 = l0 + m[2] * m[2];
            l1 = m[5] * m[5];
            l1 = l1 + m[4] * m[4];
            l1 = l1 + m[6] * m[6];
            l2 = m[9] * m[9];
            l2 = l2 + m[8] * m[8];
            l2 = l2 + m[10] * m[10];
            mx = (l1 < l2) ? l2 : l1;
            if (l0 < mx) {
                l0 = (l1 < l2) ? l2 : l1;
            }
            c[3] = *(f32 *)(arg0 + 0x28) * __builtin_sqrtf(l0);
        }
        r = c[3];
        state = 2;
        pl = (f32 *)(*(u32 *)0x008872E0 + 0x94);
        for (k = 0; k < 6; k++, pl += 5) {
            f32 dist = c[1] * pl[1];

            dist = dist + c[0] * pl[0];
            dist = dist + c[2] * pl[2];
            dist = dist - pl[3];
            if (!(dist <= r)) {
                state = 0;
                break;
            }
        }
        if (state == 2) {
            pl = (f32 *)(*(u32 *)0x008872E0 + *(s32 *)0x00764A24 + 0x60);
            for (k = 0; k < 6; k++, pl += 5) {
                f32 dist = c[1] * pl[1];

                dist = dist + c[0] * pl[0];
                dist = dist + c[2] * pl[2];
                dist = dist - pl[3];
                if (!(dist <= -r)) {
                    state = 1;
                    break;
                }
            }
        }
        if (state == 0) {
            continue;
        }
        ent = (u8 *)(buf + n * 0x50);
        for (k = 0; k < 16; k++) {
            ((u32 *)ent)[k] = ((u32 *)inst)[k];
        }
        *(u32 *)(ent + 0x3C) = state & 1;
        *(f32 *)(ent + 0x40) = (f32)(u32)col[0] / 255.0f;
        *(f32 *)(ent + 0x44) = (f32)(u32)col[1] / 255.0f;
        *(f32 *)(ent + 0x48) = (f32)(u32)col[2] / 255.0f;
        *(f32 *)(ent + 0x4C) = (f32)(u32)col[3] / 255.0f;
        n++;
        if (n == 0x30) {
            buf = p4c_0039c900_003a93b0(buf, n);
            p4c_004114d0_003a93b0(chan, (u32)arg0, 1);
            n = 0;
        }
    }
    if (n != 0) {
        p4c_0039c900_003a93b0(buf, n);
        p4c_004114d0_003a93b0(chan, (u32)arg0, 1);
    }
    return arg0;
}
