/* Port-side decoder for TMX textures (the "TMX0" container in the field and battle
 * archives). Mirrors tmxDecodeStream (decomp/src/Kernel/sdkUttmx.c), which is not
 * built by the port, and writes RGBA8 pixels with no RwImage or retail heap. */
#ifndef P4PC_TMX_H
#define P4PC_TMX_H

#include <stddef.h>
#include <stdint.h>

typedef struct P4TmxImage {
    uint32_t width;
    uint32_t height;
    uint8_t *rgba; /* width * height * 4 bytes, top row first; owned by the image */
} P4TmxImage;

/* Decodes one TMX stream of `length` bytes. Returns 0 on success. Returns -1 for
 * malformed input and for the formats retail leaves undefined: indexed images
 * without a palette, direct-colour images with a palette, and odd widths at 4 bits.
 * On failure `out` is left with rgba == NULL. Release with p4tmx_free. */
int p4tmx_decode(const uint8_t *stream, size_t length, P4TmxImage *out);

void p4tmx_free(P4TmxImage *image);

#endif
