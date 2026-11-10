/* movie_upload: the movie frame upload (decomp gsUploadRasterTiles, retail 0x00143ba0).
 *
 * Retail builds a GIF packet that copies the decoded frame (macroblock columns of RGB32, see
 * src/movie/mpeg2_dec.h) into the movie raster's GS texture memory, column by column. The port has
 * no GS memory; the frame is converted to linear RGBA8 and handed to the OpenGL raster
 * (p4gl_raster_set_rgba). The IPU's alpha of 0x80 means
 * "opaque" on the GS; the GL device treats 8888 alpha as 0..255, so it becomes 0xff. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mpeg2_dec.h"
#include "p4gl.h"

/* Writes the macroblock-column RGB32 image src (mbw columns by mbh rows of 16x16 pixels) into
 * w x h pixels at dst, clipping what falls outside. */
void p4pc_movie_detile_rgb32(const uint8_t *src, int32_t mbw, int32_t mbh, uint8_t *dst, int32_t stride,
                             int32_t w, int32_t h) {
    for (int32_t c = 0; c < mbw; c++) {
        const int32_t x0 = c * 16;
        if (x0 >= w) break;
        const int32_t cw = w - x0 < 16 ? w - x0 : 16;
        for (int32_t r = 0; r < mbh; r++) {
            const int32_t y0 = r * 16;
            if (y0 >= h) break;
            const int32_t rh = h - y0 < 16 ? h - y0 : 16;
            const uint8_t *mb = src + ((size_t)c * mbh + r) * 1024;
            for (int32_t y = 0; y < rh; y++) {
                uint8_t *o = dst + (size_t)(y0 + y) * stride + (size_t)x0 * 4;
                const uint8_t *s = mb + y * 64;
                for (int32_t x = 0; x < cw; x++) { /* whole pixels with the alpha forced: the loop vectorises, four byte stores per pixel did not */
                    uint32_t px;

                    memcpy(&px, s + x * 4, 4);
                    px |= 0xff000000u; /* little-endian: byte 3 is alpha */
                    memcpy(o + x * 4, &px, 4);
                }
            }
        }
    }
}

/* gsUploadRasterTiles(raster, owner, tileIndex, tileCount): owner is the frame's address, tileIndex
 * and tileCount its width and height in macroblocks (retail names them for the GS tile loop). The
 * game creates the movie raster with rwRASTERDONTALLOCATE (flags 0x584), so there is no shadow to
 * lock: the converted frame goes to the GL texture directly. */
int p4gl_gl_draw_enabled(void); /* src/render/p4gl_gl.c: 0 while P4PC_TURBO=2 drops the frame's draws */

/* Upload the three planes using the exact integer converter. P4PC_MOVIE_GPU=0 keeps the CPU reference path.
 * Returns 1 when the raster has the picture. */
static int g_gpu_frames;

static int gpu_convert(P4RwRaster *r, const P4Mpeg2Frame *pic) {
    static int state; /* 0 not tried, 1 enabled/ready, -1 off */

    if (state == 0) {
        const char *e = getenv("P4PC_MOVIE_GPU");
        state = e && e[0] == '0' ? -1 : 1;
    }
    if (state < 0) return 0;
    const int32_t stride[3] = {pic->stride[0], pic->stride[1], pic->stride[2]};
    if (p4gl_raster_set_yuv420(r, pic->plane, stride, pic->width, pic->height) != 0) return 0;
    g_gpu_frames++;
    return 1;
}

/* Pictures the GPU converted since the start (the tests check that the path they mean to test ran). */
int p4pc_movie_upload_gpu_frames(void) { return g_gpu_frames; }

void p4pc_movie_upload(void *raster, uint32_t owner, int32_t mbw, int32_t mbh) {
    static uint8_t *rgba; /* one frame buffer, kept: a malloc and free of a megabyte per frame is a page-fault storm */
    static size_t cap;
    P4RwRaster *r = raster;
    const P4Mpeg2Frame *pic;
    if (r == NULL || owner == 0 || r->width <= 0 || r->height <= 0) return;
    if (!p4gl_gl_draw_enabled()) return; /* nothing is drawn this frame (P4PC_TURBO=2): the picture would never be seen, and the next drawn frame uploads its own */
    pic = p4mpeg2_resolve_stamp((const void *)P4_G2H(owner));
    if (pic != NULL && pic->width == r->width && pic->height == r->height && gpu_convert(r, pic)) return;
    size_t bytes = (size_t)r->width * r->height * 4;
    if (bytes > cap) {
        uint8_t *n = realloc(rgba, bytes);
        if (!n) return;
        rgba = n;
        cap = bytes;
    }
    if (pic != NULL && pic->width == r->width && pic->height == r->height) {
        /* Reference conversion reuses the platform's fallback RGBA frame. */
        if (p4mpeg2_frame_to_rgba_ipu(pic, rgba, r->width * 4) == 0) p4gl_raster_set_rgba(r, rgba, r->width, r->height);
        return;
    }
    if (mbw * 16 < r->width || mbh * 16 < r->height) {
        memset(rgba, 0, bytes); /* the frame does not cover the raster: the rest is transparent black */
    }
    p4pc_movie_detile_rgb32((const uint8_t *)P4_G2H(owner), mbw, mbh, rgba, r->width * 4, r->width, r->height);
    p4gl_raster_set_rgba(r, rgba, r->width, r->height);
}
