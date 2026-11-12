/* Retail 0x00463930: decoded guest bytes remain the matched body. Only an
 * enabled native content service observes the completed image and its source. */
#include "p4pc/content_source.h"
extern void p4_content_tmx_image(void *image, const void *encoded);
RwImage *tmxDecodeStream(u8 *stream)
{
    RwImage *image;
    s32 depth;
    const u8 *source;
    s32 paletteBytes;
    s32 paletteBits;
    s32 paletteCount;
    s32 paletteFormat;

    source = stream + 0x40;
    if (stream[0] != 2 || stream[1] != 0 ||
        stream[8] != 'T' || stream[9] != 'M' ||
        stream[10] != 'X' || stream[11] != '0') {
        return 0;
    }
    switch (stream[0x16]) {
    case 0:
        depth = 32;
        break;
    case 1:
        depth = 24;
        break;
    case 2:
    case 10:
        depth = 16;
        break;
    case 0x13:
    case 0x1B:
        depth = 8;
        break;
    case 0x14:
    case 0x24:
    case 0x2C:
        depth = 4;
        break;
    default:
        depth = 0;
        break;
    }
    if (depth == 0) {
        return 0;
    }
    if (depth >= 16) {
        depth = 32;
    }
    image = func_003ea2c0(((TmxHeader *)stream)->width, ((TmxHeader *)stream)->height, depth);
    func_003ea3e0(image);
    paletteCount = stream[0x10];
    if (paletteCount != 0) {
        paletteFormat = stream[0x11];
        switch (paletteFormat) {
        case 0:
            paletteBits = 32;
            break;
        case 2:
        case 10:
            paletteBits = 16;
            break;
        default:
            paletteBits = 0;
            break;
        }
        paletteBytes = (paletteBits * (paletteCount * (1 << depth))) >> 3;
        switch (paletteFormat) {
        case 0:
            tmxConvertPalette(image, source);
            break;
        case 2:
        case 10: {
            u8 *palette;
            s32 count;
            s32 i;
            s32 opaque;

            palette = (u8 *)image->palette;
            count = image->depth;
            i = 0;
            count = 1 << count;
            opaque = 255;
            for (; i < count; i++) {
                const u16 *input = (const u16 *)(source + i * 2);
                u8 *output = palette + i * 4;
                u8 alpha;

                output[0] = (*input & 31) * 8;
                output[1] = ((*input >> 5) & 31) * 8;
                output[2] = ((*input >> 10) & 31) * 8;
                alpha = i == 0 ? 0 : opaque;
                output[3] = alpha;
            }
            break;
        }
        }
        tmxSwizzlePalette8(image, depth);
        source += paletteBytes;
    }
    switch (stream[0x16]) {
    case 0:
        tmxConvertRgba32(image, source);
        break;
    case 1:
        tmxConvertRgb24(image, source);
        break;
    case 2:
    case 10:
        tmxConvertRgb555(image, source);
        break;
    case 0x13:
    case 0x1B: {
        u8 *output;
        s32 width;
        s32 height;
        s32 x;
        s32 y;

        output = image->cpPixels;
        width = image->width;
        height = image->height;
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                output[x] = *source;
                source++;
            }
            output += image->stride;
        }
        break;
    }
    case 0x14:
    case 0x24:
    case 0x2C:
        tmxUnpackNibbles(image, source);
        break;
    }
    if (p4_content_sources_enabled) p4_content_tmx_image(image, stream);
    return image;
}
