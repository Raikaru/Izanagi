#include "mod_resources_native.h"
#include "p4pc/gmem.h"
#include "p4pc/image.h"
#include "p4gl.h"
#include <math.h>

extern int32_t func_003f5070(int32_t state, uint32_t value);
extern int32_t func_0040c0f0(int32_t primitive, uint8_t *vertices, int32_t count);
static int ready(void)
{
    /* The no-device battle backend counts: authored rasters keep their guest shadows and its sinks take the upload and draw. */
    return p4pc_image_loaded() && p4gl_device_ready() && P4_G32(0x00887404u) == 3u;
}

extern int32_t RwRasterDestroy(void *);
int p4_resources_raster_create(const P4ContentSource *source,
    const uint8_t *rgba, uint32_t width, uint32_t height,
    uint32_t *raster_address, uint32_t *pixel_address, uint32_t *stride_out, uint32_t *rows_out)
{
    /* Retail 003e8a50 writes STARTED=3 at 00887404 only after device start,
     * engine-plugin construction (including raster freelists), and finalize.
     * Never read guest globals before the image owner confirms its mapping. */
    if (!ready()) return P4_RESOURCE_NOT_READY;
    void *raster = p4gl_raster_create_authored(rgba, (int32_t)width, (int32_t)height, source);
    if (!raster) return P4_RESOURCE_RENDER_ERROR;
    int32_t stride = 0, rows = 0;
    const uint8_t *pixels = p4gl_raster_authored_shadow(raster, &stride, &rows);
    if (!pixels || stride <= 0 || (uint32_t)stride < width * 4u || rows < (int32_t)height) {
        RwRasterDestroy(raster);
        return P4_RESOURCE_RENDER_ERROR;
    }
    *raster_address = P4_H2G(raster); *pixel_address = P4_H2G(pixels);
    *stride_out = (uint32_t)stride; *rows_out = (uint32_t)rows;
    return P4_RESOURCE_OK;
}
void p4_resources_raster_destroy(uint32_t raster)
{
    /* RASTERDESTROY releases both the actual GPU texture and its owned guest
     * shadow. The image view has no independent allocation or ownership. */
    RwRasterDestroy(P4_G2H(raster));
}

int p4_resources_draw(uint32_t token, float x, float y, float depth, uint32_t rgba,
    float scale_x, float scale_y)
{
    if (!isfinite(x) || !isfinite(y) || !isfinite(depth) || !isfinite(scale_x) ||
        !isfinite(scale_y) || scale_x < 0 || scale_y < 0) return P4_RESOURCE_INVALID;
    if (!ready()) return P4_RESOURCE_NOT_READY;
    const P4ResourceImage *image = p4_resources_query(token);
    if (!image) return P4_RESOURCE_NOT_FOUND;
    const float width = (float)image->width * scale_x, height = (float)image->height * scale_y;
    if (!isfinite(x + width) || !isfinite(y + height)) return P4_RESOURCE_INVALID;
    if (scale_x == 0 || scale_y == 0) return P4_RESOURCE_OK;
    P4Sky2Vertex vertices[4] = {{0}};
    const float u = (float)image->width / (float)image->texture_width;
    const float v = (float)image->height / (float)image->texture_height;
    /* Native sprites (0046B380) write screen z = D_008872F8 - depth: the Z
     * scale minus the caller's sprite depth, larger = nearer. The same
     * convention keeps an authored image at its caller's depth. */
    const float z = P4_GDATA(float, 0x008872F8u) - depth;
    for (unsigned i = 0; i < 4; ++i) {
        vertices[i].x = x + ((i & 1u) ? width : 0.0f);
        vertices[i].y = y + ((i & 2u) ? height : 0.0f);
        vertices[i].z = z; vertices[i].recipZ = 1.0f;
        vertices[i].u = (i & 1u) ? u : 0.0f;
        vertices[i].v = (i & 2u) ? v : 0.0f;
        vertices[i].r = (float)(rgba & 255u);
        vertices[i].g = (float)((rgba >> 8) & 255u);
        vertices[i].b = (float)((rgba >> 16) & 255u);
        vertices[i].a = (float)(rgba >> 24);
    }
    static const uint32_t cells[] = {P4GL_RS_TEXTURERASTER, P4GL_RS_TEXTUREFILTER, P4GL_RS_VERTEXALPHAENABLE};
    uint32_t values[3], written = p4gl_render_state_written();
    for (unsigned i = 0; i < 3; ++i) {
        values[i] = p4gl_render_state(cells[i]);
        func_003f5070((int32_t)cells[i], i ? 1u : image->raster);
    }
    /* Same primitive entry point the native sprite owner uses. Stack vertices
     * are consumed synchronously; no guest address or foreign queue is forged. */
    const int32_t drawn = func_0040c0f0(4, (uint8_t *)vertices, 4);
    for (unsigned i = 0; i < 3; ++i) p4gl_render_state_restore(cells[i], values[i], written);
    return drawn ? P4_RESOURCE_OK : P4_RESOURCE_RENDER_ERROR;
}
