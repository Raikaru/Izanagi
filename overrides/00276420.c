/* frFontRasterUploadGlyph (retail 00276420): unpacks a 32x32 4-bit glyph (16 bytes a row, the even pixel in the low nibble) into the
 * glyph cache entry's PAL4 raster and copies in the 16-entry palette param_3. The retail code writes the pixels in the GS PSMT4 block
 * layout (the address table D_0063bce0), the layout the PS2 raster buffer is uploaded in as it is. The port's rasters keep PAL4 pixels
 * linear (native textures are un-swizzled when they are read, p4gl_nativetex.c), so the swizzled glyphs came out as horizontal streaks: the
 * pixels are written linear here, 16 bytes a row, as the raster handlers read them. The palette copy and the entry's palette id are as retail. */
#include "p4pc/content_source.h"
extern void p4gl_raster_content_exclude(void *raster, int origin);

void frFontRasterUploadGlyph(int param_1, u8 *param_2, int param_3)
{
    u32 palette;
    u32 x;
    u32 y;
    u8 *pixels;
    u8 *src;

    if (param_2 == (u8 *)0x0) {
        func_00440b68(D_0063bde0);
        kAssertFail(D_0063bc88, 0x17e);
        return;
    }
    src = param_2;
    if (p4_content_sources_enabled)
        p4gl_raster_content_exclude(P4_G2H(*(u32 *)(param_1 + 0x14)), P4_TEXTURE_PROCEDURAL);
    pixels = (u8 *)func_003ec6a0(*(u32 *)(param_1 + 0x14), 0, 9);
    for (y = 0; y < 0x20; y++) {
        for (x = 0; x < 0x10; x++) {
            pixels[y * 0x10 + x] = *src++;
        }
    }
    func_003ec2a0(*(u32 *)(param_1 + 0x14));
    palette = func_003ec3d0(*(u32 *)(param_1 + 0x14), 1);
    if (palette == 0) {
        kAssertFail(D_0063bc88, 0x1b1);
    }
    memcpy(palette, (u32)D_0063b5f0 + param_3 * 0x40, 0x40);
    func_003ec2e0(*(u32 *)(param_1 + 0x14));
    *(short *)(param_1 + 8) = (short)param_3;
}
