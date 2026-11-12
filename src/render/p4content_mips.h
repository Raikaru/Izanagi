#ifndef P4CONTENT_MIPS_H
#define P4CONTENT_MIPS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Host-private replacement images, never guest raster storage. */
#define P4_CONTENT_IMAGE_LIMIT 16384u
#define P4_CONTENT_ATLAS_LIMIT 4096u

typedef struct P4ContentCell {
    uint32_t x, y, width, height;
} P4ContentCell;

/* Validate logical-source rectangles once before starting a mip chain. */
int p4_content_atlas_validate(const P4ContentCell *cells, size_t count,
                              uint32_t logical_width, uint32_t logical_height);
/* Pixel centers, rather than rounded extents, assign boundary texels to cells.
 * Returns 1 for nonempty output, 0 for no texel center, -1 for invalid bounds. */
int p4_content_atlas_project(const P4ContentCell *cell,
                             uint32_t logical_width, uint32_t logical_height,
                             uint32_t image_width, uint32_t image_height,
                             P4ContentCell *projected);
/* Highest consecutive representable level. A sampler must clamp each cell's
 * LOD here: no one-texel atlas mip can preserve several different cells.
 * Returns -1 when the base cell is invalid or unrepresentable. */
int p4_content_atlas_max_level(const P4ContentCell *cell,
                                   uint32_t logical_width, uint32_t logical_height,
                                   uint32_t image_width, uint32_t image_height);
size_t p4_content_mip_workspace_size(size_t cell_count);

/* Area-average an RGBA8 image to independent, no-larger positive dimensions.
 * RGB is averaged premultiplied and stored straight; a zero-alpha result has
 * zero RGB. Source has source_stride bytes per row (at least width * 4);
 * output is tightly packed. Buffers are disjoint. No allocation.
 *
 * Optional atlas rectangles have already passed atlas_validate. workspace is
 * malloc-aligned storage of mip_workspace_size(count) bytes. Cell-owned output
 * centers filter only their own projected source rectangle, with replicated
 * edges. Unclaimed centers use the ordinary full-image filter. A cell which
 * cannot be represented in the source or output is not written specially;
 * the sampler's max-level clamp prevents its use at such a level.
 * Returns 0 on success, -1 for an invalid contract; no output on rejection. */
int p4_content_rgba_reduce(const uint8_t *source, uint32_t source_width,
                            uint32_t source_height, size_t source_stride,
                            uint8_t *output,
                            uint32_t output_width, uint32_t output_height,
                            const P4ContentCell *cells, size_t cell_count,
                            uint32_t logical_width, uint32_t logical_height,
                            void *workspace, size_t workspace_size);

#ifdef __cplusplus
}
#endif

#endif
