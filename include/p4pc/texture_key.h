/* Stable decoded texture identity. Wire format: docs/platform/content_packs.md. */
#ifndef P4PC_TEXTURE_KEY_H
#define P4PC_TEXTURE_KEY_H
#include <stddef.h>
#include <stdint.h>

enum P4TextureFormat {
    P4_TEX_ARGB1555 = 1, P4_TEX_RGB565, P4_TEX_ARGB4444, P4_TEX_L8,
    P4_TEX_RGBA8888, P4_TEX_RGB888, P4_TEX_RGB555, P4_TEX_PAL4, P4_TEX_PAL8
};
enum P4PaletteFormat { P4_PAL_NONE, P4_PAL_RGBA8888, P4_PAL_RGB555 };
#define P4_TEXTURE_KEY_HEX 64

typedef struct P4TexturePixels {
    uint32_t width, height, format, palette_format, palette_entries;
    const uint8_t *rgba;       /* top-down, tightly packed decoded RGBA8 */
    const uint8_t *palette;    /* canonical RGBA8 in logical index order */
} P4TexturePixels;

/* Returns 0 and a lowercase hex digest, or -1 for invalid dimensions/format/
 * palette. No allocation, GL access, padding read or native-struct hashing. */
int p4_texture_key(const P4TexturePixels *pixels, char hex[P4_TEXTURE_KEY_HEX + 1]);
#endif
