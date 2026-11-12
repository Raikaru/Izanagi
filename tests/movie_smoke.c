/* movie_smoke: a movie frame from the player to pixels, through the game's own upload call.
 *
 * movieNativeTaskUpdate creates a 640x448 raster with RwRasterCreate(0x280, 0x1C0, 0x20, 0x584)
 * (texture, rwRASTERDONTALLOCATE, 8888), asks the player for a frame, copies it to its image buffer,
 * uploads it with gsUploadRasterTiles(raster, image, mbw, mbh) and draws the raster on an Im2D quad
 * with the render states func_003f5070 sets. This test does the same against the OpenGL device,
 * offscreen, with P4CT019 on the tick clock: frame 120 is uploaded, drawn full frame with nearest
 * sampling, read back, and compared with the de-tiled player frame (alpha is opaque). The PNG
 * shows the movie picture. Needs P4PC_ISO and P4PC_ELF (boot heap, like render_smoke); exits 77
 * without them. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4gl.h"
#include "p4gl_gl.h"
#include "p4gl_readback.h"
#include "p4pc/image.h"
#include "mpeg2_dec.h"
#include "p4pc/os.h"
#include "p4png.h"
#include "vfs.h"
#include "guest_test.h"

#define SKIP_RETURN 77
#define W 640u
#define H 448u
#define PRIM_TRISTRIP 4
#define FILTER_NEAREST 1

void *kwlnProbeHeapBudget(void);
int32_t func_003f98a0(void *pOut, void *pInOut, int32_t flags);
int32_t func_003fb830(void *pOut, void *pInOut, int32_t nI);
int32_t func_003f5070(int32_t state, uint32_t value);
int32_t func_0040c0f0(int32_t primType, uint8_t *vertices, int32_t numVertices);

void *func_00509268(const void *prm);
void func_005097e8(void *h);
int32_t func_0050d4d0(void *h);
void p4pc_movie_start(void *h, const char *path, int32_t loop);
void p4pc_mwply_get_cur_frm(void *h, void *frm);
int p4pc_mwply_fill_tiled(void *h);
int p4pc_movie_upload_gpu_frames(void);
void gsUploadRasterTiles(void *raster, uint32_t owner, int32_t tileIndex, int32_t tileCount);

static int failures;
static void fail(const char *what) { fprintf(stderr, "movie_smoke: FAIL %s\n", what); failures++; }
static int32_t rd32(const uint8_t *f, int off) { int32_t v; memcpy(&v, f + off, 4); return v; }

/* The raster's texture, level 0, top row first (the image sits at the top-left of the 1024 x 512 texture); `out` takes the image and the
 * padding right and below it is checked to be transparent black. */
static int read_raster(P4RwRaster *r, uint8_t *out) {
    GLuint fbo = 0;
    const uint32_t tex = p4gl_raster_texture(r);
    if (tex == 0) return -1;
    p4gl_glGenFramebuffers(1, &fbo);
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    p4gl_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    p4gl_glReadPixels(0, 0, (GLsizei)W, (GLsizei)H, GL_RGBA, GL_UNSIGNED_BYTE, out);
    static uint8_t *full;
    if (!full) full = malloc((size_t)1024 * 512 * 4);
    p4gl_glReadPixels(0, 0, 1024, 512, GL_RGBA, GL_UNSIGNED_BYTE, full);
    size_t pad = 0, bx0 = 9999, bx1 = 0, by0 = 9999, by1 = 0;
    for (size_t y = 0; y < 512; y++)
        for (size_t x = 0; x < 1024; x++)
            if ((x >= W || y >= H) && *(const uint32_t *)(full + (y * 1024 + x) * 4) != 0) {
                pad++;
                if (x < bx0) bx0 = x;
                if (x > bx1) bx1 = x;
                if (y < by0) by0 = y;
                if (y > by1) by1 = y;
            }
    if (pad) fprintf(stderr, "movie_smoke: padding box x %zu..%zu y %zu..%zu, texel(700,10) = %08x, texel(10,460) = %08x\n", bx0, bx1, by0, by1,
                     *(const uint32_t *)(full + (10 * 1024 + 700) * 4), *(const uint32_t *)(full + (460 * 1024 + 10) * 4));
    if (pad) { fprintf(stderr, "movie_smoke: %zu texels of the padding are not transparent black\n", pad); failures++; }
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, p4gl_readback_framebuffer());
    return 0;
}

static int test_main(int argc, char **argv) {
    const char *iso = getenv("P4PC_ISO");
    const char *elf = getenv("P4PC_ELF");
    const char *png = argc > 1 ? argv[1] : "movie_smoke.png";
    if (!iso || !*iso || !elf || !*elf) {
        fprintf(stderr, "movie_smoke: P4PC_ISO or P4PC_ELF unset, skipped\n");
        return SKIP_RETURN;
    }
    p4_os_headless_env();
    p4_os_setenv("P4PC_TRAP_CONTINUE", "1", 0);
    p4_os_setenv("P4PC_MOVIE_CLOCK", "tick", 1);
    p4pc_trap_configure();
    p4pc_load_image(elf);
    p4pc_apply_patches();
    p4pc_kernel_pre_init();
    kwlnProbeHeapBudget();
    if (vfs_mount(iso, NULL) != 0) { fprintf(stderr, "movie_smoke: cannot mount %s\n", iso); return 1; }
    if (p4gl_context_open() != 0) { fail("GL context"); return 1; }

    /* The movie raster as the game creates it. */
    P4RwRaster *r = p4t_galloc(sizeof *r, 16);   /* bound as the texture raster: guest storage */
    r->width = (int32_t)W;
    r->height = (int32_t)H;
    r->depth = 32;
    r->cFlags = 0x80;   /* rwRASTERDONTALLOCATE */
    if (func_003f98a0(NULL, r, 0x584) != 1) { fail("RASTERCREATE 0x584"); return 1; }

    int32_t prm[12] = {1, 0x4C4B40, 640, 448, 2, 1, 0, 0x1000, 0x11, 2, 0, 0};
    void *h = func_00509268(prm);
    if (!h) { fail("create player"); return 1; }
    p4pc_movie_start(h, "sfd/P4CT019.sfd", 0);

    uint8_t frm[0x90];
    uint8_t *image = p4t_galloc(W * H * 4, 16);   /* the player writes frames here: the upload reads it by guest address */
    int32_t number = -1, compared = 0;
    for (int tick = 0; tick < 400 && number < 120; tick++) {
        p4pc_mwply_get_cur_frm(h, frm);
        if (rd32(frm, 0)) {
            number = rd32(frm, 0x24);
            size_t n = (size_t)rd32(frm, 0x0c) * rd32(frm, 0x10) * 4;
            memcpy(image, P4_G2H((uint32_t)rd32(frm, 0)), n);   /* the game's memcpy to its image buffer */
            if (number <= 120) {
                /* The upload converts the stamped picture (on the GPU unless P4PC_MOVIE_GPU=0); the raster must hold the CPU
                 * converter's bytes exactly, alpha opaque. */
                const P4Mpeg2Frame *pic = p4mpeg2_resolve_stamp(image);
                if (!pic) { fail("the image buffer is not stamped"); return 1; }
                static uint8_t *cpu, *tex;
                if (!cpu) { cpu = malloc((size_t)W * H * 4); tex = malloc((size_t)W * H * 4); }
                gsUploadRasterTiles(r, P4_H2G(image), rd32(frm, 0x14), rd32(frm, 0x18));
                if (p4mpeg2_frame_to_rgba_ipu(pic, cpu, (int)W * 4) != 0 || read_raster(r, tex) != 0) { fail("frame compare"); return 1; }
                size_t bad = 0;
                for (size_t i = 0; i < (size_t)W * H * 4; i++) bad += cpu[i] != tex[i];
                if (bad) { fprintf(stderr, "movie_smoke: frame %d: %zu bytes of the raster differ from the CPU conversion\n", number, bad); failures++; }
                compared++;
            }
        }
    }
    if (number < 120) { fail("player frame 120"); return 1; }
    /* The game's image buffer holds the player's stamp, not pixels: the upload converts the picture the stamp stands for. */
    gsUploadRasterTiles(r, P4_H2G(image), rd32(frm, 0x14), rd32(frm, 0x18));

    /* Expected picture: the frame as the IPU macroblock columns (what the buffer held before the stamp), de-tiled. */
    if (p4pc_mwply_fill_tiled(h) != 0) { fail("tiled frame"); return 1; }
    memcpy(image, P4_G2H((uint32_t)rd32(frm, 0)), (size_t)rd32(frm, 0x0c) * rd32(frm, 0x10) * 4);
    uint8_t *want = malloc((size_t)W * H * 4);
    for (unsigned c = 0; c < W / 16; c++)
        for (unsigned row = 0; row < H / 16; row++)
            for (unsigned y = 0; y < 16; y++)
                for (unsigned x = 0; x < 16; x++)
                    memcpy(want + (((size_t)(row * 16 + y) * W) + c * 16 + x) * 4,
                           image + ((size_t)c * (H / 16) + row) * 1024 + (y * 16 + x) * 4, 4);

    if (p4gl_readback_target_create(W, H, W, H) != 0) { fail("readback target"); return 1; }
    if (func_003f5070(P4GL_RS_TEXTURERASTER, P4_H2G(r)) != 1 ||
        func_003f5070(P4GL_RS_TEXTUREFILTER, FILTER_NEAREST) != 1)
        fail("render state");
    p4gl_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); /* Im2D tests Z, as the GS does */
    P4Sky2Vertex v[4];
    const float corners[4][2] = {{0.0f, 0.0f}, {(float)W, 0.0f}, {0.0f, (float)H}, {(float)W, (float)H}};
    /* GS textures are power-of-two: the 640x448 image is the top-left of a 1024x512 texture, as the game's own UVs assume. */
    const float uMax = 640.0f / 1024.0f, vMax = 448.0f / 512.0f;
    const float uvs[4][2] = {{0.0f, 0.0f}, {uMax, 0.0f}, {0.0f, vMax}, {uMax, vMax}};
    memset(v, 0, sizeof v);
    for (int i = 0; i < 4; i++) {
        v[i].x = corners[i][0];
        v[i].y = corners[i][1];
        v[i].camZ = 1.0f;
        v[i].u = uvs[i][0];
        v[i].v = uvs[i][1];
        v[i].recipZ = 1.0f;
        v[i].r = v[i].g = v[i].b = v[i].a = 255.0f;   /* RwIm2DVertex colour range */
    }
    if (func_0040c0f0(PRIM_TRISTRIP, (uint8_t *)v, 4) != 1) fail("Im2D quad");

    uint8_t *got = malloc((size_t)W * H * 4);
    if (p4gl_readback_rgba(W, H, got) != 0) {
        fail("readback");
    } else {
        int64_t sum = 0, lit = 0;
        int worst = 0;
        for (size_t i = 0; i < (size_t)W * H * 4; i += 4) {
            for (int k = 0; k < 3; k++) {
                int d = abs((int)got[i + k] - (int)want[i + k]);
                sum += d;
                if (d > worst) worst = d;
            }
            lit += got[i] + got[i + 1] + got[i + 2];
        }
        const double mean = (double)sum / ((double)W * H * 3);
        printf("movie_smoke: frame %d, mean difference %.3f, worst %d, brightness %.1f\n", number, mean, worst,
               (double)lit / ((double)W * H * 3));
        if (worst > 4 || mean > 0.5) fail("the drawn raster differs from the player's frame");
        if (lit < (int64_t)W * H * 3 * 10) fail("the drawn frame is black");
        if (p4png_write_rgba(png, W, H, got) != 0) fail("PNG write");
    }
    {
        const char *g = getenv("P4PC_MOVIE_GPU");
        const int gpu = p4pc_movie_upload_gpu_frames();
        printf("movie_smoke: %d frames compared byte for byte with the CPU conversion, %d converted on the GPU\n", compared, gpu);
        if (compared < 10) fail("too few frames compared");
        if (!(g && g[0] == '0') && gpu == 0) fail("the GPU conversion did not run (set P4PC_MOVIE_GPU=0 to test the CPU path alone)");
    }
    func_005097e8(h);
    func_003fb830(NULL, r, 0);
    p4gl_context_close();
    return failures ? 1 : 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}
