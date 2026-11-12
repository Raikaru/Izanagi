#ifndef P4GL_NATIVETEX_H
#define P4GL_NATIVETEX_H

#include <stdint.h>

/* Decodes the data chunk of a paletted native texture (kind 2): w x h indices, one byte per pixel
 * (a 4-bit texture's index is 0..15), and the palette as RGBA bytes (PS2 alpha, 0x80 opaque; 256 entries
 * for depth 8, 16 for depth 4). Returns 1, or 0 for a malformed stream. */
int p4gl_nativetex_decode(const uint8_t *data, uint32_t size, int32_t w, int32_t h, int32_t depth, uint8_t *indices,
                          uint8_t *palette);

/* A true-colour native texture of kind 2 (depth 32, PSMCT32): the first IMAGE of the GIF stream is the pixels in raster order,
 * 4 bytes each (PS2 alpha). Copies w * h * 4 bytes to rgba. Returns 1, or 0 for a malformed stream. */
int p4gl_nativetex_decode_ct32(const uint8_t *data, uint32_t size, int32_t w, int32_t h, uint8_t *rgba);

#endif
