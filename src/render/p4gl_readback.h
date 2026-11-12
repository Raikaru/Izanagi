/* p4gl_readback: an offscreen RGBA8 colour target and CPU readback, for p4pc_viewer's
 * PNG output. See p4gl_readback.c. Needs a current GL context with p4gl_gl_load done. */
#ifndef P4PC_RENDER_P4GL_READBACK_H
#define P4PC_RENDER_P4GL_READBACK_H

#include <stdint.h>

/* Physical target size, independent of the game's virtual640x448 coordinates.
 * Resolution/aspect policy is resolved by the context before allocation. */
void p4gl_readback_pixel_size(uint32_t *w, uint32_t *h);

/* glViewport over the whole target (its pixel size). The main target's viewport must be this whenever it is bound again. */
void p4gl_readback_viewport(void);

/* Creates exact physical storage with an independent virtual coordinate space.
 * The game's main target uses virtual640x448; smoke/movie surfaces may use others.
 * Binds the framebuffer and its pixel viewport. Returns0/-1. */
int p4gl_readback_target_create(uint32_t w, uint32_t h, uint32_t virtualWidth, uint32_t virtualHeight);

/* The framebuffer object that target_create made and bound, 0 (the default framebuffer) when none. */
unsigned p4gl_readback_framebuffer(void);
void p4gl_readback_virtual_size(uint32_t *w, uint32_t *h);


/* Reads the target into rgba (w * h * 4 bytes; w and h are the target's pixel size), top row first. Returns 0 on success,
 * -1 when GL reports an error. */
int p4gl_readback_rgba(uint32_t w, uint32_t h, uint8_t *rgba);

/* The GL context was replaced (p4gl_context_recover): the target's objects went with it. Forgets them; size reads 0 x 0 until target_create is called again. */
void p4gl_readback_context_lost(void);

/* Live settings/resize transition. Preserves the picture and old target on failure.
 * Zero width/height preserves that physical axis, or defaults640x448. */
int p4gl_readback_configure(int offscreen, uint32_t width, uint32_t height);
/* Prepare/copy the candidate before invoking commit (once). A nonzero callback
 * result cancels it: old target, pixels, bindings and viewport remain intact.
 * The callback may persist configuration, but must not issue GL/readback calls. */
int p4gl_readback_configure_checked(int offscreen, uint32_t width, uint32_t height,
                                    int (*commit)(const void *), const void *userdata);

/* Compare top-down RGBA pixels with raw RGB. A reference is either the exact
 * physical dimensions, or640x448 (exact area/box reduction, including fractional
 * and per-axis ratios). No intermediate image is allocated. Returns0/-1. */
int p4gl_reference_mad(const uint8_t *rgba, uint32_t width, uint32_t height,
                       const char *path, double *mad);

#endif /* P4PC_RENDER_P4GL_READBACK_H */
