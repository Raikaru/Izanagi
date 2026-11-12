/* func_003d8760 (1172 bytes): converts one row of a source image into the raster's 32-bit (or 8-bit)
 * row buffer. arg0 is the target description: +4 pixel count n, +0xC format byte, +0x10 row stride,
 * +0x14 base, +0x18 palette (4 bytes per entry). arg1 is the row index (the destination is
 * base + stride*arg1), arg2 the source row, arg3 the source depth class and arg4 a channel swap flag for
 * 24-bit sources. The key is (arg3 << 8) | format byte. Returns 1, or 0 after reporting error
 * {0x19D, func_003df590(0x80000008, 0x120)} through func_003df4d0 for an unknown key (0x120 is the
 * stale register value the retail code passes as the second argument).
 *
 *   0x2020  32-bit source: R, G, B copied, alpha 0xFF
 *   0x1820  24-bit source: RGB (arg4 != 0) or BGR (arg4 == 0), alpha 0xFF
 *   0x1020  RGB565, 0x0F20  RGB555: expanded to bytes with the low bits clear, alpha 0xFF
 *   0x0820  8-bit index, 0x0420  4-bit index (high nibble first), 0x0120  1-bit index (MSB first):
 *           4 palette bytes per pixel
 *   0x0808  8-bit copy (the port's memcpy), 0x0408 and 0x0404  4-bit values to bytes, 0x0108 and
 *           0x0104  1-bit values to bytes. */

extern void p4c_003df4d0_003d8760(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003d8760(s32 code, ...) __asm__("func_003df590");

extern void *p4_memcpy(void *dst, const void *src, u32 n) __asm__("func_0043f810");

s32 func_003d8760(u8 *arg0, s32 arg1, u8 *arg2, s32 arg3, s32 arg4)
{
    s32 n;
    u8 *pal;
    u8 *dst;
    u8 *src;
    s32 key;
    s32 i;

    n = *(s32 *)(arg0 + 4);
    pal = *(u8 **)(arg0 + 0x18);
    dst = *(u8 **)(arg0 + 0x14) + *(s32 *)(arg0 + 0x10) * arg1;
    src = arg2;
    key = (arg3 << 8) | *(s32 *)(arg0 + 0xC);

    switch (key) {
    case 0x2020:
        for (i = 0; i < n; i++) {
            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
            dst[3] = 0xFF;
            src += 4;
            dst += 4;
        }
        return 1;
    case 0x1820:
        for (i = 0; i < n; i++) {
            if (arg4 != 0) {
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
            } else {
                dst[2] = src[0];
                dst[1] = src[1];
                dst[0] = src[2];
            }
            dst[3] = 0xFF;
            src += 3;
            dst += 4;
        }
        return 1;
    case 0x1020:
        for (i = 0; i < n; i++) {
            u32 v = *(u16 *)src;

            dst[0] = (u8)((v >> 8) & 0xF8);
            dst[1] = (u8)((v >> 3) & 0xFC);
            dst[2] = (u8)((v << 3) & 0xF8);
            dst[3] = 0xFF;
            src += 2;
            dst += 4;
        }
        return 1;
    case 0x0F20:
        for (i = 0; i < n; i++) {
            u32 v = *(u16 *)src;

            dst[0] = (u8)((v >> 7) & 0xF8);
            dst[1] = (u8)((v >> 2) & 0xF8);
            dst[2] = (u8)((v << 3) & 0xF8);
            dst[3] = 0xFF;
            src += 2;
            dst += 4;
        }
        return 1;
    case 0x0820:
        for (i = 0; i < n; i++) {
            u8 *e = pal + 4 * (u32)src[0];

            dst[0] = e[0];
            dst[1] = e[1];
            dst[2] = e[2];
            dst[3] = e[3];
            src += 1;
            dst += 4;
        }
        return 1;
    case 0x0420:
        for (i = 0; i < n; i++) {
            u8 *e = pal + 4 * (u32)((src[i >> 1] >> (4 - 4 * (i & 1))) & 0xF);

            dst[0] = e[0];
            dst[1] = e[1];
            dst[2] = e[2];
            dst[3] = e[3];
            dst += 4;
        }
        return 1;
    case 0x0120:
        for (i = 0; i < n; i++) {
            u8 *e = pal + 4 * (u32)((src[i >> 3] >> (7 - (i & 7))) & 1);

            dst[0] = e[0];
            dst[1] = e[1];
            dst[2] = e[2];
            dst[3] = e[3];
            dst += 4;
        }
        return 1;
    case 0x0808:
        p4_memcpy(dst, src, (u32)n);
        return 1;
    case 0x0408:
    case 0x0404:
        for (i = 0; i < n; i++) {
            dst[i] = (u8)((src[i >> 1] >> (4 - 4 * (i & 1))) & 0xF);
        }
        return 1;
    case 0x0108:
    case 0x0104:
        for (i = 0; i < n; i++) {
            dst[i] = (u8)((src[i >> 3] >> (7 - (i & 7))) & 1);
        }
        return 1;
    default: {
        s32 err[2];

        err[0] = 0x19D;
        err[1] = p4c_003df590_003d8760(0x80000008, 0x120);
        p4c_003df4d0_003d8760(err);
        return 0;
    }
    }
}
