/* Real guest RW raster + GL upload, using only authored test pixels. */
#include "p4pc/mod_resources.h"
#include "p4pc/content_source.h"
#include "p4pc/image.h"
#include "p4pc/platform.h"
#include "p4pc/os.h"
#include "p4pc/tables.h"
#include "p4gl.h"
#include "p4gl_gl.h"
#include "p4gl_readback.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void *kwlnProbeHeapBudget(void);
extern void kwlnInitEngine(void);
extern int32_t func_003f5070(int32_t, uint32_t);
extern int32_t RpSkyRenderStateSet(int32_t, uint32_t);
extern P4RwRaster *RwRasterCreate(int32_t, int32_t, int32_t, uint32_t);
extern int32_t RwRasterDestroy(void *);

static void check_draw(uint32_t token, const uint8_t *pixels, unsigned scale_x, unsigned scale_y)
{
    static uint8_t readback[640 * 448 * 4];
    assert(!p4gl_readback_target_create(640, 448, 640, 448));
    assert(func_003f5070(P4GL_RS_TEXTURERASTER, 0));
    assert(func_003f5070(P4GL_RS_TEXTUREFILTER, 2)); /* baseline linear */
    assert(func_003f5070(P4GL_RS_VERTEXALPHAENABLE, 0));
    assert(func_003f5070(P4GL_RS_ZTESTENABLE, 0));
    assert(func_003f5070(P4GL_RS_ZWRITEENABLE, 0));
    /* A zero low ALPHA byte uses the renderer's copy mapping; a nonzero
     * retained high bit still makes accidental Sky policy reset observable. */
    assert(RpSkyRenderStateSet(2, 0x100));
    assert(RpSkyRenderStateSet(3, 3)); /* enabled, ALWAYS */
    p4gl_frame_mask_set(0);
    p4gl_gs_clamp_set(5); /* native clamp on both axes */
    p4gl_render_state_restore(P4GL_RS_TEXTUREFILTER, 2,
        p4gl_render_state_written() & ~(1u << P4GL_RS_TEXTUREFILTER));
    uint32_t states[32], written = p4gl_render_state_written();
    for (unsigned i = 0; i < 32; ++i) states[i] = p4gl_render_state(i);
    const uint32_t alpha = p4gl_sky_alpha_raw(), test = p4gl_sky_test_raw(), mask = p4gl_frame_mask_get();
    uint32_t clamp_x, clamp_y;
    assert(p4gl_gs_clamp_get(&clamp_x, &clamp_y));
    p4gl_glClearColor(0, 0, 0, 0);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    assert(p4_resources_draw(token, 16, 32, 69, 0xffffffffu,
        (float)scale_x, (float)scale_y) == P4_RESOURCE_OK);
    assert(p4gl_render_state_written() == written);
    for (unsigned i = 0; i < 32; ++i) assert(p4gl_render_state(i) == states[i]);
    assert(p4gl_sky_alpha_raw() == alpha && p4gl_sky_test_raw() == test && p4gl_frame_mask_get() == mask);
    uint32_t actual_clamp_x, actual_clamp_y;
    assert(p4gl_gs_clamp_get(&actual_clamp_x, &actual_clamp_y) &&
        actual_clamp_x == clamp_x && actual_clamp_y == clamp_y);
    assert(!p4gl_readback_rgba(640, 448, readback));
    for (unsigned y = 0; y < 448; ++y) for (unsigned x = 0; x < 640; ++x) {
        static const uint8_t clear[4] = {0};
        const uint8_t *expected = clear;
        if (x >= 16 && x < 16 + 13 * scale_x && y >= 32 && y < 32 + 11 * scale_y)
            expected = pixels + (((y - 32) / scale_y) * 13 + (x - 16) / scale_x) * 4;
        if (memcmp(readback + (y * 640 + x) * 4, expected, 4)) {
            fprintf(stderr, "mod_resources_render: authored draw mismatch at %u,%u (scale %u,%u)\n",
                x, y, scale_x, scale_y);
            abort();
        }
    }
}

static void check_texture(const P4ResourceImage *image, const uint8_t *expected)
{
    uint8_t readback[16 * 16 * 4];
    GLuint texture = p4gl_raster_texture(P4_G2H(image->raster)); assert(texture);
    GLuint fbo = 0;
    p4gl_glGenFramebuffers(1, &fbo); p4gl_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    p4gl_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    assert(p4gl_glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    p4gl_glReadPixels(0, 0, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, readback);
    assert(!memcmp(readback, expected, sizeof readback));
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, 0); p4gl_glDeleteFramebuffers(1, &fbo);
}

static int test_main(int argc, char **argv)
{
    (void)argc; (void)argv;
    const char *elf = getenv("P4PC_ELF");
    if (!elf || !elf[0]) { fprintf(stderr, "mod_resources_render: P4PC_ELF unset\n"); return 77; }
    p4_os_headless_env();
    p4_os_setenv("P4PC_INPUT", "script", 1);
    p4_os_setenv("P4PC_MODS", "0", 1);
    p4_os_setenv("P4PC_CONFIG", "none", 1);
    p4_os_setenv("P4PC_ASPECT", "4:3", 1);
    p4_os_setenv("P4PC_TRAP_CONTINUE", "0", 1);
    p4_os_setenv("P4PC_TEST_CONTEXT_LOSS", "1", 1);
    p4pc_trap_configure();
    uint8_t pixels[13 * 11 * 4], padded[16 * 16 * 4] = {0};
    for (unsigned y = 0; y < 11; ++y) for (unsigned x = 0; x < 13; ++x) {
        size_t i = (y * 13 + x) * 4;
        pixels[i] = (uint8_t)(x * 17); pixels[i + 1] = (uint8_t)(y * 17);
        pixels[i + 2] = (uint8_t)((x ^ y) * 17); pixels[i + 3] = (uint8_t)(x * 11 + y * 3);
        memcpy(padded + (y * 16 + x) * 4, pixels + i, 4);
    }
    P4ContentSource source = {0}; source.origin = P4_TEXTURE_OVERRIDE; source.rank = 1;
    strcpy(source.provider, "test.resources"); strcpy(source.key, "authored/test-pattern.rgba");
    P4Resources *resources = p4_resources_create(2 * sizeof padded); assert(resources);
    uint32_t token = 99;
    assert(!p4pc_image_loaded() && !p4gl_context_is_current());
    assert(p4_resources_image(resources, "test.resources", "test.resources:icon", &source,
        pixels, sizeof pixels, 13, 11, 1, &token) == P4_RESOURCE_NOT_READY);
    assert(token == 99 && !p4_resources_count(resources));
    p4pc_load_image(elf);
    assert(p4pc_image_loaded() && !p4gl_context_is_current());
    assert(p4_resources_image(resources, "test.resources", "test.resources:icon", &source,
        pixels, sizeof pixels, 13, 11, 1, &token) == P4_RESOURCE_NOT_READY);
    assert(token == 99 && !p4_resources_count(resources));
    p4pc_apply_patches(); p4pc_kernel_pre_init();
    kwlnProbeHeapBudget();
    /* Reuse the actual game's RW/plugin/device/free-list owner. Do not invent
     * raster-module globals or substitute a host-only raster allocation. */
    kwlnInitEngine();
    assert(p4gl_context_is_current());
    /* Ordinary game creation is not changed and cannot be retagged as an
     * authored resource, even when it uses the same retail flags. */
    P4RwRaster *ordinary = RwRasterCreate(13, 11, 32, 0x0584u);
    assert(ordinary && !(ordinary->cFlags & 0x80u));
    assert(p4gl_raster_set_authored_rgba(ordinary, pixels, 13, 11, &source) != 0);
    assert(RwRasterDestroy(ordinary));
    int image_result = p4_resources_image(resources, "test.resources", "test.resources:icon", &source,
        pixels, sizeof pixels, 13, 11, 1, &token);
    if (image_result != P4_RESOURCE_OK) fprintf(stderr, "mod_resources_render: image creation returned %d\n", image_result);
    assert(image_result == P4_RESOURCE_OK);
    assert(!p4_resources_query(token));
    assert(p4_resources_freeze(resources, 1) == P4_RESOURCE_OK);
    const P4ResourceImage *image = p4_resources_query(token); assert(image && image->raster && image->pixels);
    assert(image->width == 13 && image->height == 11 && image->stride == 16 * 4);
    assert(image->texture_width == 16 && image->texture_height == 16);
    assert(!memcmp(P4_G2H(image->pixels), padded, sizeof padded));
    check_texture(image, padded);
    check_draw(token, pixels, 1, 1);
    assert(p4_resources_pin(token) == P4_RESOURCE_OK);
    const uint32_t raster = image->raster, shadow = image->pixels;
    /* The existing test seam replaces the real context on its first present.
     * Logical IDs, guest shadow and pins must survive; only GPU caches rebuild. */
    p4gl_context_present();
    assert(p4gl_context_is_current());
    image = p4_resources_query(token);
    assert(image && image->raster == raster && image->pixels == shadow);
    assert(!memcmp(P4_G2H(shadow), padded, sizeof padded));
    check_texture(image, padded); /* Includes odd straight alpha after reupload. */
    check_draw(token, pixels, 2, 2);
    p4_resources_retire(resources);
    assert(p4_resources_query(token) && p4_resources_pin(token) == P4_RESOURCE_RETIRED);
    assert(p4_resources_release(token) == P4_RESOURCE_OK && !p4_resources_query(token));
    p4gl_context_close();
    assert(!p4gl_context_is_current());
    puts("mod_resources_render: real guest raster, NPOT RGBA/alpha, native draws/state ownership, context recovery and pinned retirement passed");
    return 0;
}
int main(int argc, char **argv) { return p4t_on_guest_stack(test_main, argc, argv); }
