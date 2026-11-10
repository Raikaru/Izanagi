/* func_003d8500 (416 bytes): writes one image row to the stream arg0 and returns the number of bytes written (padded to a multiple of 4
 * with zero bytes). arg1 is the image ({+4 = width, +0x10 = row stride, +0x14 = pixels}), arg2 the row (counted from the start of the pixels), arg3 the
 * format and arg4 a scratch buffer. Format 4 packs two pixel bytes per output byte (high nibble first); format 8 copies width bytes; any other
 * format converts 4-byte RGBA pixels into 3-byte BGR. The row is written with func_003e2ab0(stream, buffer, size). */

extern s32 p4c_003e2ab0_003d8500(s32 stream, void *p, s32 n) __asm__("func_003e2ab0");
extern void *p4c_0043f810_003d8500(void *dst, const void *src, u32 n) __asm__("func_0043f810");

s32 func_003d8500(u32 arg0, u8 *arg1, s32 arg2, s32 arg3, u8 *arg4)
{
    s32 n = 0;
    s32 w;
    u8 *row;
    s32 i;
    u8 zero = 0;

    if (arg3 == 4) {
        w = *(s32 *)(arg1 + 4);
        row = *(u8 **)(arg1 + 0x14) + arg2 * *(s32 *)(arg1 + 0x10);
        for (i = 0; i < w; i += 2) {
            arg4[i >> 1] = (u8)((row[i] << 4) | row[i + 1]);
        }
        n += w >> 1;
    } else if (arg3 == 8) {
        w = *(s32 *)(arg1 + 4);
        row = *(u8 **)(arg1 + 0x14) + arg2 * *(s32 *)(arg1 + 0x10);
        p4c_0043f810_003d8500(arg4, row, (u32)w);
        n += w;
    } else {
        w = *(s32 *)(arg1 + 4);
        row = *(u8 **)(arg1 + 0x14) + arg2 * *(s32 *)(arg1 + 0x10);
        for (i = 0; i < w; i++) {
            arg4[i * 3 + 2] = row[i * 4 + 0];
            arg4[i * 3 + 1] = row[i * 4 + 1];
            arg4[i * 3 + 0] = row[i * 4 + 2];
        }
        n += w * 3;
    }
    p4c_003e2ab0_003d8500(arg0, arg4, (u32)n);
    while ((n & 3) != 0) {
        p4c_003e2ab0_003d8500(arg0, &zero, 1);
        n++;
    }
    return n;
}
