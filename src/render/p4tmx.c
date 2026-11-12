/* Port-side TMX decoder. Format selection, palette conversion, the 8-bit palette
 * swizzle and pixel order follow tmxDecodeStream in decomp/src/Kernel/sdkUttmx.c. */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/tmx.h"

#define TMX_HEADER_BYTES 0x40u

static uint32_t tmx_u16(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}

/* Retail alpha: values at or above 0x7F are opaque, otherwise a * 255 / 128. */
static uint8_t tmx_alpha(uint32_t a)
{
    if (a >= 0x7Fu) {
        return 0xFFu;
    }
    return (uint8_t)((a * 0xFFu) >> 7);
}

static void tmx_rgb555(uint32_t v, uint8_t *rgb)
{
    rgb[0] = (uint8_t)((v & 31u) * 8u);
    rgb[1] = (uint8_t)(((v >> 5) & 31u) * 8u);
    rgb[2] = (uint8_t)(((v >> 10) & 31u) * 8u);
}

/* Converts `entries` palette entries into RGBA8. Entry 0 of an RGB555 palette is
 * transparent; RGBA32 palettes keep their own alpha. */
static void tmx_convert_palette(uint8_t *pal, const uint8_t *src, uint32_t entries, uint32_t format)
{
    uint32_t i;

    for (i = 0; i < entries; i++) {
        uint8_t *out = pal + i * 4u;

        if (format == 0u) {
            out[0] = src[i * 4u + 0u];
            out[1] = src[i * 4u + 1u];
            out[2] = src[i * 4u + 2u];
            out[3] = tmx_alpha(src[i * 4u + 3u]);
        } else {
            tmx_rgb555(tmx_u16(src + i * 2u), out);
            out[3] = (i == 0u) ? 0u : 0xFFu;
        }
    }
}

/* 8-bit palettes store entries 8..15 and 16..23 of each 32-entry block swapped. */
static void tmx_swizzle_palette8(uint8_t *pal)
{
    uint32_t i;

    for (i = 0; i < 256u; i++) {
        uint32_t mod = i % 0x20u;

        if (mod >= 8u && mod < 16u) {
            uint8_t *a = pal + i * 4u;
            uint8_t *b = pal + (i + 8u) * 4u;
            uint32_t k;

            for (k = 0; k < 4u; k++) {
                uint8_t t = a[k];
                a[k] = b[k];
                b[k] = t;
            }
        }
    }
}

void p4tmx_free(P4TmxImage *image)
{
    if (image == NULL) {
        return;
    }
    free(image->rgba);
    image->rgba = NULL;
    image->width = 0;
    image->height = 0;
}

int p4tmx_decode(const uint8_t *stream, size_t length, P4TmxImage *out)
{
    uint32_t depth;
    uint32_t width;
    uint32_t height;
    uint32_t paletteCount;
    uint32_t paletteFormat = 0;
    uint32_t paletteBits = 0;
    uint32_t paletteEntries = 0;
    uint32_t direct;
    size_t paletteBytes = 0;
    size_t pixelBytes;
    size_t pixels;
    size_t remaining;
    const uint8_t *source;
    uint8_t pal[256 * 4];
    uint8_t *rgba;
    size_t p;

    if (out == NULL) {
        return -1;
    }
    out->width = 0;
    out->height = 0;
    out->rgba = NULL;

    if (stream == NULL || length < TMX_HEADER_BYTES) {
        return -1;
    }
    if (stream[0] != 2 || stream[1] != 0 || memcmp(stream + 8, "TMX0", 4) != 0) {
        return -1;
    }

    switch (stream[0x16]) {
    case 0x00: depth = 32; break;
    case 0x01: depth = 24; break;
    case 0x02:
    case 0x0A: depth = 16; break;
    case 0x13:
    case 0x1B: depth = 8; break;
    case 0x14:
    case 0x24:
    case 0x2C: depth = 4; break;
    default: return -1;
    }

    width = tmx_u16(stream + 0x12);
    height = tmx_u16(stream + 0x14);
    if (width == 0u || height == 0u) {
        return -1;
    }
    direct = depth >= 16u;
    pixels = (size_t)width * (size_t)height;

    paletteCount = stream[0x10];
    source = stream + TMX_HEADER_BYTES;
    remaining = length - TMX_HEADER_BYTES;

    if (paletteCount != 0u) {
        if (direct) {
            return -1;
        }
        paletteFormat = stream[0x11];
        switch (paletteFormat) {
        case 0x00: paletteBits = 32; break;
        case 0x02:
        case 0x0A: paletteBits = 16; break;
        default: return -1;
        }
        paletteEntries = 1u << depth;
        paletteBytes = ((size_t)paletteBits * (size_t)paletteCount * (size_t)paletteEntries) >> 3;
        if (remaining < paletteBytes) {
            return -1;
        }
        tmx_convert_palette(pal, source, paletteEntries, paletteFormat);
        if (depth == 8u) {
            tmx_swizzle_palette8(pal);
        }
        source += paletteBytes;
        remaining -= paletteBytes;
    } else if (!direct) {
        return -1;
    }

    switch (depth) {
    case 32: pixelBytes = pixels * 4u; break;
    case 24: pixelBytes = pixels * 3u; break;
    case 16: pixelBytes = pixels * 2u; break;
    case 8:  pixelBytes = pixels; break;
    default:
        if ((width & 1u) != 0u) {
            return -1;
        }
        pixelBytes = pixels / 2u;
        break;
    }
    if (remaining < pixelBytes) {
        return -1;
    }

    rgba = malloc(pixels * 4u);
    if (rgba == NULL) {
        return -1;
    }

    for (p = 0; p < pixels; p++) {
        uint8_t *out_px = rgba + p * 4u;

        switch (depth) {
        case 32:
            out_px[0] = source[p * 4u + 0u];
            out_px[1] = source[p * 4u + 1u];
            out_px[2] = source[p * 4u + 2u];
            out_px[3] = tmx_alpha(source[p * 4u + 3u]);
            break;
        case 24:
            out_px[0] = source[p * 3u + 0u];
            out_px[1] = source[p * 3u + 1u];
            out_px[2] = source[p * 3u + 2u];
            out_px[3] = 0xFFu;
            break;
        case 16:
            tmx_rgb555(tmx_u16(source + p * 2u), out_px);
            out_px[3] = 0xFFu;
            break;
        case 8: {
            uint32_t idx = source[p];
            memcpy(out_px, pal + idx * 4u, 4u);
            break;
        }
        default: {
            /* Low nibble is the even pixel of each pair, as in tmxUnpackNibbles. */
            uint8_t byte = source[p >> 1];
            uint32_t idx = (p & 1u) ? (uint32_t)(byte >> 4) : (uint32_t)(byte & 0x0Fu);
            memcpy(out_px, pal + idx * 4u, 4u);
            break;
        }
        }
    }

    out->width = width;
    out->height = height;
    out->rgba = rgba;
    return 0;
}
