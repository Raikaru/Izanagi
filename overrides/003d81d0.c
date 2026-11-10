/* func_003d81d0 (292 bytes): writes a BMP file header to the stream arg0. arg1 is the bit depth, arg2
 * the raster description (+4 width, +8 height). Returns 0xE.
 *
 * Writes "BM" (one byte each with func_003e2ab0), the file size word (func_003df240) and two zero
 * halfwords (func_003df270), then the pixel data offset word. The palette takes 4 << arg1 bytes for a
 * depth of 8 or less, else none; the offset is palette + 0x36 and the size is
 * offset + height * (((width * depth + 31) & ~31) / 8). */

extern s32 p4c_003df240_003d81d0(s32 stream, void *p, s32 n) __asm__("func_003df240");
extern s32 p4c_003df270_003d81d0(s32 stream, void *p, s32 n) __asm__("func_003df270");
extern s32 p4c_003e2ab0_003d81d0(s32 stream, void *p, s32 n) __asm__("func_003e2ab0");

s32 func_003d81d0(s32 arg0, s32 arg1, u8 *arg2)
{
    u8 c;
    s32 size;
    s32 offset;
    s32 pal;
    s32 rowBytes;
    u16 z;

    c = 0x42;
    p4c_003e2ab0_003d81d0(arg0, &c, 1);
    c = 0x4D;
    p4c_003e2ab0_003d81d0(arg0, &c, 1);
    rowBytes = (((*(s32 *)(arg2 + 4) * arg1) + 0x1F) & ~0x1F) >> 3;
    pal = ((u32)arg1 < 9) ? (1 << arg1) << 2 : 0;
    size = pal + 0x36 + *(s32 *)(arg2 + 8) * rowBytes;
    p4c_003df240_003d81d0(arg0, &size, 4);
    z = 0;
    p4c_003df270_003d81d0(arg0, &z, 2);
    z = 0;
    p4c_003df270_003d81d0(arg0, &z, 2);
    offset = pal + 0x36;
    p4c_003df240_003d81d0(arg0, &offset, 4);
    return 0xE;
}
