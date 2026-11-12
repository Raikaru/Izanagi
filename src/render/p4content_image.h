#ifndef P4CONTENT_IMAGE_H
#define P4CONTENT_IMAGE_H

#include "p4content_mips.h"

#ifdef __cplusplus
extern "C" {
#endif

#define P4_CONTENT_MIP_LIMIT 15u
#define P4_CONTENT_KTX_METADATA_LIMIT 65536u

typedef enum P4ContentImageFormat {
    P4_IMAGE_RGBA8,
    P4_IMAGE_BC1, P4_IMAGE_BC2, P4_IMAGE_BC3, P4_IMAGE_BC7,
    P4_IMAGE_ETC2_RGB, P4_IMAGE_ETC2_RGBA, P4_IMAGE_ETC2_A1,
    P4_IMAGE_ASTC_4x4, P4_IMAGE_ASTC_5x4, P4_IMAGE_ASTC_5x5,
    P4_IMAGE_ASTC_6x5, P4_IMAGE_ASTC_6x6, P4_IMAGE_ASTC_8x5,
    P4_IMAGE_ASTC_8x6, P4_IMAGE_ASTC_8x8, P4_IMAGE_ASTC_10x5,
    P4_IMAGE_ASTC_10x6, P4_IMAGE_ASTC_10x8, P4_IMAGE_ASTC_10x10,
    P4_IMAGE_ASTC_12x10, P4_IMAGE_ASTC_12x12,
    P4_IMAGE_BASIS
} P4ContentImageFormat;

typedef enum P4ContentImageContainer {
    P4_IMAGE_PNG = 1, P4_IMAGE_DDS, P4_IMAGE_KTX2
} P4ContentImageContainer;

enum {
    P4_IMAGE_FLIP_X = 1u,
    P4_IMAGE_FLIP_Y = 2u,
    P4_IMAGE_PREMULTIPLIED = 4u,
    P4_IMAGE_SRGB_HINT = 8u,
    P4_IMAGE_INTERLACED = 16u,
    /* Physical fourth channel carries data, even if DFD maps it to green. */
    P4_IMAGE_CHANNEL_A = 32u
};
enum { P4_IMAGE_SWIZZLE_ZERO = 4, P4_IMAGE_SWIZZLE_ONE = 5 };

typedef struct P4ContentImageFormatInfo {
    const char *name;
    uint8_t block_width, block_height, bytes_per_block, compressed;
} P4ContentImageFormatInfo;

typedef struct P4ContentImageInput {
    void *user;
    uint64_t size;
    /* Worker-owned exact random read: 0 success, nonzero I/O/EOF failure.
     * The common package loader supplies the already-contained file handle;
     * codecs never open paths or implement their own package resolver. */
    int (*read_at)(void *user, uint64_t offset, void *output, size_t size);
} P4ContentImageInput;

typedef struct P4ContentImageLevel {
    uint32_t width, height, row_bytes;
    uint64_t offset, size, uncompressed_size;
} P4ContentImageLevel;

typedef struct P4ContentImageInfo {
    P4ContentImageContainer container;
    P4ContentImageFormat format;
    uint32_t width, height, level_count, full_level_count, flags;
    uint32_t supercompression, basis_model;
    uint64_t dfd_offset, dfd_size, kvd_offset, kvd_size, sgd_offset, sgd_size;
    /* Decoded physical RGBA -> logical output, including DFD/KTXswizzle.
     * Indices 0..3 select channels; 4 is zero and 5 is opaque one. */
    uint8_t swizzle[4];
    /* Undo premultiplication before the final swizzle. */
    uint8_t premultiply_alpha, premultiply_rgb_mask;
    P4ContentImageLevel levels[P4_CONTENT_MIP_LIMIT];
} P4ContentImageInfo;

const P4ContentImageFormatInfo *p4_content_image_format(P4ContentImageFormat format);
uint64_t p4_content_image_level_size(P4ContentImageFormat format,
                                    uint32_t width, uint32_t height);
/* Bounded header/index inspection, no allocation. Sniffs bytes, not extensions.
 * Rejects animated PNG containers; game animation uses separately keyed source
 * frames. On failure info is unchanged and error (when supplied) is terminated. */
int p4_content_image_probe(const P4ContentImageInput *input,
                           P4ContentImageInfo *info, char *error, size_t error_size);
/* Read one already-probed encoded mip; caller owns/budgets the destination. */
int p4_content_image_read_level(const P4ContentImageInput *input,
                                const P4ContentImageInfo *info, unsigned level,
                                void *output, size_t capacity,
                                char *error, size_t error_size);
/* Canonicalize a decoded physical RGBA8 level in place, including orientation,
 * alpha convention and component mapping. Padding is untouched; no allocation.
 * Color-profile hints never apply a gamma/ICC transfer to engine color bytes. */
int p4_content_image_transform_rgba(uint8_t *pixels, size_t bytes,
                                    uint32_t width, uint32_t height, size_t stride,
                                    const P4ContentImageInfo *info);

#ifdef __cplusplus
}
#endif
#endif
