#ifndef P4PC_MOD_RESOURCES_NATIVE_H
#define P4PC_MOD_RESOURCES_NATIVE_H
#include "p4pc/mod_resources.h"
/* Actual RW/context adapter. On failure it releases all partial allocations and
 * leaves output addresses untouched. No fallback raster is permitted. */
int p4_resources_raster_create(const struct P4ContentSource *source,
    const uint8_t *rgba, uint32_t width, uint32_t height,
    uint32_t *raster, uint32_t *pixels, uint32_t *stride, uint32_t *padded_rows);
void p4_resources_raster_destroy(uint32_t raster);
#endif
