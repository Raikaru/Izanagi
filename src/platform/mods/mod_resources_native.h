#ifndef P4PC_MOD_RESOURCES_NATIVE_H
#define P4PC_MOD_RESOURCES_NATIVE_H
#include "p4pc/mod_resources.h"
/* Only the no-device boot path registers a token without a raster or pixels. */
int p4_resources_image_metadata(P4Resources *resources, const char *owner, const char *key,
    const struct P4ContentSource *source, uint32_t width, uint32_t height,
    uint32_t revision, uint32_t *token);
/* Actual RW/context adapter. On failure it releases all partial allocations and
 * leaves output addresses untouched. No fallback raster is permitted. */
int p4_resources_raster_create(const struct P4ContentSource *source,
    const uint8_t *rgba, uint32_t width, uint32_t height,
    uint32_t *raster, uint32_t *pixels, uint32_t *stride, uint32_t *padded_rows);
void p4_resources_raster_destroy(uint32_t raster);
#endif
