/* Metadata-only fixtures link the real resource service and family compilers.
 * Any actual raster allocation/destruction here is an error, not fake success.
 * Native RW/GL qualification uses the separate real receiver/render gate. */
#ifndef P4_TEST_NO_RESOURCE_WORK_H
#define P4_TEST_NO_RESOURCE_WORK_H
#include "p4pc/mod_resources.h"
#include <assert.h>
int p4_resources_raster_create(const P4ContentSource *source, const uint8_t *rgba,
    uint32_t width, uint32_t height, uint32_t *raster, uint32_t *pixels,
    uint32_t *stride, uint32_t *rows)
{
    (void)source; (void)rgba; (void)width; (void)height;
    (void)raster; (void)pixels; (void)stride; (void)rows;
    assert(!"metadata-only fixture unexpectedly creates a raster");
    return P4_RESOURCE_RENDER_ERROR;
}
void p4_resources_raster_destroy(uint32_t raster)
{
    (void)raster;
    assert(!"metadata-only fixture unexpectedly destroys a raster");
}
#endif
