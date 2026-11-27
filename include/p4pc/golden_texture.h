#ifndef P4PC_GOLDEN_TEXTURE_H
#define P4PC_GOLDEN_TEXTURE_H
#include <stddef.h>
#include <stdint.h>
#define P4_GOLDEN_TEXTURE_MIPS 15u
#define P4_GOLDEN_TEXTURE_BUDGET (128u << 20)
typedef struct P4GoldenTextureLevel {
    uint32_t width, height;
    uint8_t *rgba; /* top-down, packed, straight RGBA8 */
} P4GoldenTextureLevel;
typedef struct P4GoldenTexture {
    uint32_t level_count;
    uint8_t srgb, png_storage, hd_coordinates; /* TMX +0x02 == 1: physical corners are logical corners times 2 */
    void *storage;
    P4GoldenTextureLevel levels[P4_GOLDEN_TEXTURE_MIPS];
} P4GoldenTexture;
/* CPU-only bounded codecs; no mode, filesystem, renderer or guest side effects.
 * Output must be zero-initialized/unowned. Failure leaves it unchanged. */
int p4_golden_texture_decode(const uint8_t *bytes, size_t size, P4GoldenTexture *out);
void p4_golden_texture_free(P4GoldenTexture *texture);
#endif
