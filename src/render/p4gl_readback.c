/* RGBA8 framebuffer storage for the game and viewer, independent of the window's
 * default framebuffer (which may be absent under SDL's offscreen driver).
 * Physical storage and virtual game coordinates are tracked separately. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4gl_gl.h"
#include "p4gl_readback.h"
#ifdef P4_MOD_GAME_BINDINGS
#include "p4pc/mod_fidelity.h"
#endif

static GLuint p4gl_rb_texture;
static GLuint p4gl_rb_framebuffer;
static GLuint p4gl_rb_depth;
static uint32_t p4gl_rb_width;  /* physical pixels; game coordinates remain independent */
static uint32_t p4gl_rb_height;
static uint32_t p4gl_rb_virtual_width;
static uint32_t p4gl_rb_virtual_height;


int p4gl_readback_target_create(uint32_t w, uint32_t h, uint32_t virtualWidth, uint32_t virtualHeight)
{
    GLenum status;

    GLint textureLimit = 0, renderbufferLimit = 0;
    p4gl_glGetIntegerv(0x0D33u /* GL_MAX_TEXTURE_SIZE */, &textureLimit);
    p4gl_glGetIntegerv(0x84E8u /* GL_MAX_RENDERBUFFER_SIZE */, &renderbufferLimit);
    if (w == 0 || h == 0 || virtualWidth == 0 || virtualHeight == 0 || w > (uint32_t)textureLimit || h > (uint32_t)textureLimit ||
        w > (uint32_t)renderbufferLimit || h > (uint32_t)renderbufferLimit) {
        fprintf(stderr, "p4gl: cannot allocate %ux%u target (texture limit %d, renderbuffer limit %d)\n",
                w, h, textureLimit, renderbufferLimit);
        return -1;
    }

    p4gl_glGenTextures(1, &p4gl_rb_texture);
    p4gl_glBindTexture(GL_TEXTURE_2D, p4gl_rb_texture);
    p4gl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)w, (GLsizei)h, 0, GL_RGBA,
                      GL_UNSIGNED_BYTE, NULL);
    p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    p4gl_glGenFramebuffers(1, &p4gl_rb_framebuffer);
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, p4gl_rb_framebuffer);
    p4gl_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                p4gl_rb_texture, 0);
    p4gl_glGenRenderbuffers(1, &p4gl_rb_depth);
    p4gl_glBindRenderbuffer(GL_RENDERBUFFER, p4gl_rb_depth);
    p4gl_glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, (GLsizei)w, (GLsizei)h);
    p4gl_glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                   p4gl_rb_depth);
    status = p4gl_glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "p4gl_readback: framebuffer incomplete (0x%x)\n", status);
        return -1;
    }

    p4gl_glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    p4gl_rb_width = w;
    p4gl_rb_height = h;
    p4gl_rb_virtual_width = virtualWidth;
    p4gl_rb_virtual_height = virtualHeight;
    return 0;
}

void p4gl_readback_viewport(void)
{
    p4gl_glViewport(0, 0, (GLsizei)p4gl_rb_width, (GLsizei)p4gl_rb_height);
}

void p4gl_readback_pixel_size(uint32_t *w, uint32_t *h)
{
    *w = p4gl_rb_width;
    *h = p4gl_rb_height;
}

void p4gl_readback_virtual_size(uint32_t *w, uint32_t *h)
{
    *w = p4gl_rb_virtual_width;
    *h = p4gl_rb_virtual_height;
}

unsigned p4gl_readback_framebuffer(void)
{
    return p4gl_rb_width != 0 ? p4gl_rb_framebuffer : 0;
}


int p4gl_readback_rgba(uint32_t w, uint32_t h, uint8_t *rgba)
{
    size_t row_bytes = (size_t)w * 4;
    uint8_t *row;
    GLenum err;

    if (!rgba || w == 0 || h == 0 || w != p4gl_rb_width || h != p4gl_rb_height) {
        return -1;
    }

    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, p4gl_rb_framebuffer);
    p4gl_glReadPixels(0, 0, (GLsizei)w, (GLsizei)h, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    err = p4gl_glGetError();
    if (err != GL_NO_ERROR) {
        fprintf(stderr, "p4gl_readback: glReadPixels failed (0x%x)\n", err);
        return -1;
    }
    if (h == 1) return 0;

    /* GL returns the bottom row first; the PNG writer expects the top row first. */
    row = malloc(row_bytes);
    if (!row) {
        return -1;
    }
    for (uint32_t top = 0, bottom = h - 1; top < bottom; top++, bottom--) {
        uint8_t *a = rgba + (size_t)top * row_bytes;
        uint8_t *b = rgba + (size_t)bottom * row_bytes;
        memcpy(row, a, row_bytes);
        memcpy(a, b, row_bytes);
        memcpy(b, row, row_bytes);
    }
    free(row);
    return 0;
}

void p4gl_readback_context_lost(void)
{
    p4gl_rb_texture = p4gl_rb_framebuffer = p4gl_rb_depth = 0;
    p4gl_rb_width = p4gl_rb_height = 0;
    p4gl_rb_virtual_width = p4gl_rb_virtual_height = 0;
}

int p4gl_readback_configure_checked(int offscreen, uint32_t requestedWidth, uint32_t requestedHeight,
                                    int (*commit)(const void *), const void *userdata)
{
    const GLuint texture = p4gl_rb_texture, framebuffer = p4gl_rb_framebuffer, depth = p4gl_rb_depth;
    const uint32_t width = p4gl_rb_width, height = p4gl_rb_height;
    const uint32_t virtualWidth = p4gl_rb_virtual_width, virtualHeight = p4gl_rb_virtual_height;
    const uint32_t w = requestedWidth ? requestedWidth : width ? width : 640;
    const uint32_t h = requestedHeight ? requestedHeight : height ? height : 448;
    GLint read = 0, draw = 0, boundTexture = 0, boundDepth = 0, viewport[4];
    if (offscreen && framebuffer && w == width && h == height) {
        if (commit && commit(userdata) != 0) return -1;
        p4gl_rb_virtual_width = 640; p4gl_rb_virtual_height = 448;
        return 0;
    }
    if (!offscreen && !framebuffer) return commit ? commit(userdata) : 0;
    p4gl_glGetIntegerv(0x8CAAu /* GL_READ_FRAMEBUFFER_BINDING */, &read);
    p4gl_glGetIntegerv(0x8CA6u /* GL_DRAW_FRAMEBUFFER_BINDING */, &draw);
    p4gl_glGetIntegerv(0x8069u /* GL_TEXTURE_BINDING_2D */, &boundTexture);
    p4gl_glGetIntegerv(0x8CA7u /* GL_RENDERBUFFER_BINDING */, &boundDepth);
    p4gl_glGetIntegerv(0x0BA2u /* GL_VIEWPORT */, viewport);
    p4gl_rb_texture = p4gl_rb_framebuffer = p4gl_rb_depth = 0;
    p4gl_rb_width = p4gl_rb_height = 0;
    p4gl_rb_virtual_width = p4gl_rb_virtual_height = 0;
    if (offscreen && p4gl_readback_target_create(w, h, 640, 448) != 0) goto cancel;
    p4gl_glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    p4gl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, p4gl_rb_framebuffer);
    p4gl_glBlitFramebuffer(0, 0, width ? width : 640, height ? height : 448, 0, 0,
                          offscreen ? w : 640, offscreen ? h : 448, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    {
        const GLenum error = p4gl_glGetError();
        if (error != GL_NO_ERROR) {
            fprintf(stderr, "p4gl_readback: target copy failed (0x%x)\n", error);
            goto cancel;
        }
    }
    if (commit && commit(userdata) != 0) goto cancel;
    p4gl_glDeleteTextures(1, &texture);
    p4gl_glDeleteFramebuffers(1, &framebuffer);
    p4gl_glDeleteRenderbuffers(1, &depth);
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, p4gl_rb_framebuffer);
    return 0;

cancel:
    p4gl_glDeleteTextures(1, &p4gl_rb_texture);
    p4gl_glDeleteFramebuffers(1, &p4gl_rb_framebuffer);
    p4gl_glDeleteRenderbuffers(1, &p4gl_rb_depth);
    p4gl_rb_texture = texture; p4gl_rb_framebuffer = framebuffer; p4gl_rb_depth = depth;
    p4gl_rb_width = width; p4gl_rb_height = height;
    p4gl_rb_virtual_width = virtualWidth; p4gl_rb_virtual_height = virtualHeight;
    p4gl_glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)read);
    p4gl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)draw);
    p4gl_glBindTexture(GL_TEXTURE_2D, (GLuint)boundTexture);
    p4gl_glBindRenderbuffer(GL_RENDERBUFFER, (GLuint)boundDepth);
    p4gl_glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    return -1;
}

int p4gl_readback_configure(int offscreen, uint32_t width, uint32_t height)
{
    return p4gl_readback_configure_checked(offscreen, width, height, NULL, NULL);
}

int p4gl_reference_mad(const uint8_t *rgba, uint32_t width, uint32_t height,
                       const char *path, double *mad)
{
    #ifdef P4_MOD_GAME_BINDINGS
    p4_packages_assert_disabled(); /* Reference operation, not a fixture start. */
    #endif
    FILE *file = fopen(path, "rb");
    long length = -1;
    uint8_t *reference = NULL;
    uint32_t rw = width, rh = height;
    double difference = 0.0;

    if (file != NULL && fseek(file, 0, SEEK_END) == 0) length = ftell(file);
    if (length == 640L * 448L * 3L) { rw = 640; rh = 448; }
    if (rgba == NULL || width == 0 || height == 0 || length <= 0 ||
        (uint64_t)length != (uint64_t)rw * rh * 3 ||
        (reference = malloc((size_t)length)) == NULL ||
        fseek(file, 0, SEEK_SET) != 0 || fread(reference, 1, (size_t)length, file) != (size_t)length) {
        fprintf(stderr, "p4gl: cannot read RGB reference %s for %ux%u pixels (expected exact size or640x448)\n", path, width, height);
        if (file != NULL) fclose(file);
        free(reference);
        return -1;
    }
    fclose(file);
    if (rw == width && rh == height) {
        uint64_t sum = 0;
        for (size_t i = 0; i < (size_t)width * height; i++) {
            for (unsigned c = 0; c < 3; c++) {
                const int d = (int)rgba[i * 4 + c] - (int)reference[i * 3 + c];
                sum += (uint64_t)(d < 0 ? -d : d);
            }
        }
        difference = (double)sum;
    } else {
        /* Boundaries are integer numerators in units1/rw and1/rh. Their
         * overlap weights are exact; each destination pixel's area is w*h.
         * This averages before taking abs, unlike comparing against an
         * upsampled reference, and never wraps opposite picture edges. */
        const double area = (double)((uint64_t)width * height);
        for (uint32_t y = 0; y < rh; y++) {
            const uint64_t y0 = (uint64_t)y * height, y1 = (uint64_t)(y + 1) * height;
            for (uint32_t x = 0; x < rw; x++) {
                const uint64_t x0 = (uint64_t)x * width, x1 = (uint64_t)(x + 1) * width;
                uint64_t sum[3] = {0, 0, 0};
                for (uint32_t sy = (uint32_t)(y0 / rh); sy < (y1 + rh - 1) / rh; sy++) {
                    const uint64_t loY = y0 > (uint64_t)sy * rh ? y0 : (uint64_t)sy * rh;
                    const uint64_t hiY = y1 < (uint64_t)(sy + 1) * rh ? y1 : (uint64_t)(sy + 1) * rh;
                    for (uint32_t sx = (uint32_t)(x0 / rw); sx < (x1 + rw - 1) / rw; sx++) {
                        const uint64_t loX = x0 > (uint64_t)sx * rw ? x0 : (uint64_t)sx * rw;
                        const uint64_t hiX = x1 < (uint64_t)(sx + 1) * rw ? x1 : (uint64_t)(sx + 1) * rw;
                        const uint64_t weight = (hiX - loX) * (hiY - loY);
                        const uint8_t *pixel = rgba + ((size_t)sy * width + sx) * 4;
                        sum[0] += pixel[0] * weight;
                        sum[1] += pixel[1] * weight;
                        sum[2] += pixel[2] * weight;
                    }
                }
                for (unsigned c = 0; c < 3; c++) {
                    const double d = (double)sum[c] / area - reference[((size_t)y * rw + x) * 3 + c];
                    difference += d < 0 ? -d : d;
                }
            }
        }
    }
    free(reference);
    *mad = difference / ((double)rw * rh * 3.0 * 255.0) * 100.0;
    return 0;
}
