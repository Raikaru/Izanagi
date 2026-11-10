/* render_smoke: the OpenGL 3.3 device path, offscreen, end to end.
 *
 * A TMX texture from the retail disc (named by P4PC_ISO; the first TMX found by walking the
 * virtual tree) goes through RASTERCREATE, RASTERLOCK and RASTERUNLOCK. The lock round trip is
 * checked byte for byte against the decoded TMX pixels. The raster is then bound through
 * RwRenderState TEXTURERASTER with nearest filtering, and a full-frame Im2D quad with UVs 0..1
 * is drawn into the readback FBO. Two pixels are checked against the texels nearest sampling
 * picks, and the frame is written as a PNG to argv[1] (default render_smoke.png).
 *
 * The boot heap is created as p4pc_viewer creates it, from the retail SLUS_217.82 named by
 * P4PC_ELF. Exits 77 (CTest skip) when P4PC_ISO or P4PC_ELF is unset. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4gl.h"
#include "p4gl_gl.h"
#include "p4gl_readback.h"
#include "../src/platform/config/p4cfg.h"
#include "p4pc/gmem.h"
#include "p4pc/tmx.h"
#include "p4pc/image.h"
#include "p4png.h"
#include "p4pc/os.h"
#include "p4pc/settings.h"
#include "../src/platform/config/p4cfg.h"
#include "vfs.h"
#include "guest_test.h"

#define SKIP_RETURN 77
#define FRAME 256u
#define TMX_MAX_BYTES (4u << 20)
#define TMX_SCAN_LIMIT 200000u
#define TMX_MAX_DEPTH 8

/* RASTERCREATE format byte for 8888 (rwRASTERFORMAT8888 >> 8, see p4gl_raster.c). */
#define SMOKE_FMT_8888 0x05
/* RwPrimitiveType RWPRIMTYPETRISTRIP (see p4gl_im2d.c). */
#define SMOKE_PRIM_TRISTRIP 4
/* RwTextureFilterMode (decomp/include/rw/sky2/rwplcore.h:5309). */
#define SMOKE_FILTER_NEAREST 1
#define SMOKE_FILTER_LINEAR 2

void p4gl_context_capture_next(const char *path);

/* Boot heap creation (overrides/00100230.c). RenderWare allocates from it. */
void *kwlnProbeHeapBudget(void);
/* RenderWare raster handlers (src/render/p4gl_raster.c) and the Im2D primitive entry point
 * (src/render/p4gl_im2d.c). Port-owned, so the test calls them directly. */
int32_t func_003f98a0(void *pOut, void *pInOut, int32_t flags);
int32_t func_003fb830(void *pOut, void *pInOut, int32_t nI);
int32_t func_003fc5f0(void *pOut, void *pInOut, int32_t nI);
int32_t func_003fd230(void *pOut, void *pInOut, int32_t nI);
int32_t func_003f5070(int32_t state, uint32_t value);
int32_t func_0040c0f0(int32_t primType, uint8_t *vertices, int32_t numVertices);
int32_t func_0040afb0(uint8_t *vertices, int32_t numVertices, int32_t vert1, int32_t vert2);
int32_t func_0040d0a0(int32_t primType, uint8_t *vertices, int32_t numVertices, uint16_t *indices, int32_t numIndices);
int32_t func_003f6a10(void *pOut, void *pInOut, int32_t nI);
int32_t func_003f73d0(void *pOut, void *pInOut, int32_t nI);
int32_t RpSkyRenderStateSet(int32_t state, uint32_t value);
int32_t func_00401b80(void);
void calendarDrawBackdrop(void *node, void *work);

/* The six RwIm3D entry points (decomp baim3d.c, replaced through overrides/004104*.c) and the
 * Im3D module record that locates their state in RenderWare's global block. */
void *RwIm3DTransform(void *verts, uint32_t numVerts, void *ltm, uint32_t flags);
int32_t RwIm3DEnd(void);
int32_t RwIm3DRenderIndexedPrimitive(int32_t primType, uint16_t *indices, int32_t numIndices);
int32_t RwIm3DRenderPrimitive(int32_t primType);
int32_t RwIm3DRenderTriangle(int32_t vert1, int32_t vert2, int32_t vert3);
int32_t RwIm3DRenderLine(int32_t vert1, int32_t vert2);
typedef struct SmokeModuleInfo {
    int32_t globalsOffset;
    int32_t numInstances;
} SmokeModuleInfo;
#define IM3D_MODULE P4_GDATA(SmokeModuleInfo, 0x00764aa0u)   /* _rwIm3DModule */
#define OUR_GLOBALS 0x008872e0u                              /* ourGlobals, where the block addresses RwEngineInstance */

static int failures;
static unsigned scanned;

static void fail(const char *what)
{
    fprintf(stderr, "render_smoke: FAIL %s\n", what);
    failures++;
}

/* A TMX stream starts with stream[0] == 2, stream[1] == 0 and "TMX0" at +8 (sdkUttmx.c). */
static int is_tmx(const char *path, long long size)
{
    unsigned char head[16];
    int fd;
    int got = 0;

    if (size < (long long)sizeof head || size > (long long)TMX_MAX_BYTES) {
        return 0;
    }
    fd = vfs_open(path, 0);
    if (fd < 0) {
        return 0;
    }
    while (got < (int)sizeof head) {
        int n = vfs_read(fd, head + got, (int)sizeof head - got);
        if (n <= 0) {
            break;
        }
        got += n;
    }
    vfs_close(fd);
    return got == (int)sizeof head && head[0] == 2 && head[1] == 0 &&
           memcmp(head + 8, "TMX0", 4) == 0;
}

/* Depth-first walk of the virtual tree. Writes the first TMX path found to out. */
static int find_tmx(const char *dir, int depth, char *out, size_t cap)
{
    char name[256];
    int is_dir;
    long long size;
    int found = 0;
    int dh = vfs_dir_open(dir);

    if (dh < 0) {
        return 0;
    }
    while (!found && vfs_dir_next(dh, name, sizeof name, &is_dir, &size) == 1) {
        char path[512];

        if (++scanned > TMX_SCAN_LIMIT) {
            break;
        }
        if (dir[0] == '\0') {
            snprintf(path, sizeof path, "%s", name);
        } else {
            snprintf(path, sizeof path, "%s/%s", dir, name);
        }
        if (is_dir) {
            if (depth < TMX_MAX_DEPTH) {
                found = find_tmx(path, depth + 1, out, cap);
            }
        } else if (is_tmx(path, size)) {
            snprintf(out, cap, "%s", path);
            found = 1;
        }
    }
    vfs_dir_close(dh);
    return found;
}

static uint8_t *read_file(const char *path, size_t *len)
{
    long long size = 0;
    int is_dir = 0;
    size_t got = 0;
    uint8_t *buf;
    int fd;

    if (vfs_stat(path, &is_dir, &size) != 0 || is_dir || size <= 0) {
        return NULL;
    }
    fd = vfs_open(path, 0);
    if (fd < 0) {
        return NULL;
    }
    buf = malloc((size_t)size);
    while (buf != NULL && got < (size_t)size) {
        int n = vfs_read(fd, buf + got, (int)((size_t)size - got));
        if (n <= 0) {
            break;
        }
        got += (size_t)n;
    }
    vfs_close(fd);
    if (buf == NULL || got != (size_t)size) {
        free(buf);
        return NULL;
    }
    *len = got;
    return buf;
}

/* Creates a raster for the decoded TMX and uploads its pixels through RASTERLOCK and
 * RASTERUNLOCK. Returns 1 on success; on failure the raster is destroyed and 0 is returned. */
static int raster_upload(P4RwRaster *r, const P4TmxImage *img)
{
    uint32_t lockAddr = 0;   /* the lock hands back a guest address */
    uint8_t *pixels;
    const size_t rowBytes = (size_t)img->width * 4u;

    memset(r, 0, sizeof *r);
    r->width = (int32_t)img->width;
    r->height = (int32_t)img->height;
    r->depth = 32;
    if (func_003f98a0(NULL, r, (int32_t)(SMOKE_FMT_8888 << 8)) != 1) {
        fail("RASTERCREATE");
        return 0;
    }
    if ((size_t)r->stride != rowBytes) {
        fail("RASTERCREATE stride");
    }
    if (func_003fc5f0(&lockAddr, r, 0) != 1 || lockAddr == 0) {
        fail("RASTERLOCK");
        func_003fb830(NULL, r, 0);
        return 0;
    }
    pixels = P4_G2H(lockAddr);
    for (uint32_t y = 0; y < img->height; y++) {
        memcpy(pixels + (size_t)y * (size_t)r->stride, img->rgba + (size_t)y * rowBytes, rowBytes);
    }
    if (func_003fd230(NULL, r, 0) != 1) {
        fail("RASTERUNLOCK");
    }
    return 1;
}

/* A second lock must return the same bytes the first lock wrote, which the upload in
 * RASTERUNLOCK must not disturb. */
static void raster_round_trip(const P4TmxImage *img)
{
    P4RwRaster *r = p4t_galloc(sizeof *r, 16);
    uint32_t lockAddr = 0;
    const size_t rowBytes = (size_t)img->width * 4u;

    if (!raster_upload(r, img)) {
        return;
    }
    if (func_003fc5f0(&lockAddr, r, 0) != 1 || lockAddr == 0) {
        fail("RASTERLOCK (second)");
    } else {
        const uint8_t *pixels = P4_G2H(lockAddr);

        for (uint32_t y = 0; y < img->height; y++) {
            if (memcmp(pixels + (size_t)y * (size_t)r->stride, img->rgba + (size_t)y * rowBytes,
                       rowBytes) != 0) {
                fail("raster lock round trip");
                break;
            }
        }
        func_003fd230(NULL, r, 0);
    }
    func_003fb830(NULL, r, 0);
}

/* The texel that nearest sampling picks for pixel (px, py) of the full-frame quad, whose UVs
 * run 0..1 across the frame: the sample sits at the pixel centre. */
static const uint8_t *texel_at(const P4TmxImage *img, unsigned px, unsigned py)
{
    const unsigned tx = (unsigned)((px + 0.5) * img->width / FRAME);
    const unsigned ty = (unsigned)((py + 0.5) * img->height / FRAME);

    return img->rgba + ((size_t)ty * img->width + tx) * 4u;
}

/* The blend is SRC_ALPHA over black, so the expected colour is the texel scaled by its alpha.
 * Allows 3 levels of rounding. */
static int pixel_matches(const uint8_t *got, const uint8_t *texel)
{
    for (int c = 0; c < 3; c++) {
        const int want = (texel[c] * texel[3] + 127) / 255;
        const int diff = (int)got[c] - want;

        if (diff > 3 || diff < -3) {
            return 0;
        }
    }
    return 1;
}

/* RwIm3D (overrides/004104d0.c and siblings). Vertices are RwIm3DVertex (36 bytes). The Im3D
 * state block is placed in a scratch buffer through the module's globalsOffset, so the retail
 * entry points run without RenderWare initialisation. The camera is a zeroed RwCamera with an
 * frame (the identity camera), view window 1x1, near 0.5 and far 50, opened through CAMERABEGINUPDATE,
 * so a quad of half-size z at depth z fills the frame exactly. RenderWare's camera +x is the screen's
 * left, so the test quad is built with x negated to keep the corner layout (top-left first). */
#define IM3D_VERTEXUV 1u
#define IM3D_ALLOPAQUE 2u
#define IM3D_PRIM_LINELIST 1
#define IM3D_PRIM_TRILIST 3
#define IM3D_PRIM_TRISTRIP 4

typedef struct Im3DVertex {
    float x, y, z;
    uint8_t rgba[4];
    float nx, ny, nz;
    float u, v;
} Im3DVertex;
_Static_assert(sizeof(Im3DVertex) == 36, "RwIm3DVertex layout");

/* RwMatrix: right, flags, up, pad1, at, pad2, pos, pad3. */
typedef struct Im3DMatrix {
    float right[3];
    uint32_t flags;
    float up[3];
    uint32_t pad1;
    float at[3];
    uint32_t pad2;
    float pos[3];
    uint32_t pad3;
} Im3DMatrix;

static void put_f32(uint8_t *base, size_t off, float value)
{
    memcpy(base + off, &value, sizeof value);
}

/* Quad as a triangle strip: top-left, top-right, bottom-left, bottom-right, half-size h at depth
 * z, with uv 0..1 over the quad. */
static void im3d_quad(Im3DVertex q[4], float z, float h, uint8_t r, uint8_t g, uint8_t b,
                      uint8_t a)
{
    static const float corner[4][2] = {{-1.0f, 1.0f}, {1.0f, 1.0f}, {-1.0f, -1.0f}, {1.0f, -1.0f}};
    static const float uv[4][2] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}};

    memset(q, 0, 4 * sizeof *q);
    for (int i = 0; i < 4; i++) {
        q[i].x = -corner[i][0] * h; /* camera +x is the screen's left (p4gl_camera_object_matrix) */
        q[i].y = corner[i][1] * h;
        q[i].z = z;
        q[i].rgba[0] = r;
        q[i].rgba[1] = g;
        q[i].rgba[2] = b;
        q[i].rgba[3] = a;
        q[i].u = uv[i][0];
        q[i].v = uv[i][1];
    }
}

static void expect_rgb(const uint8_t *rgba, unsigned x, unsigned y, int r, int g, int b,
                       const char *what)
{
    const uint8_t *got = rgba + ((size_t)y * FRAME + x) * 4u;

    if (abs((int)got[0] - r) > 3 || abs((int)got[1] - g) > 3 || abs((int)got[2] - b) > 3) {
        fprintf(stderr, "render_smoke: Im3D %s: pixel (%u,%u) is %u %u %u, want %d %d %d\n", what,
                x, y, got[0], got[1], got[2], r, g, b);
        fail(what);
    }
}

static void expect_int(int32_t got, int32_t want, const char *what)
{
    if (got != want) {
        fprintf(stderr, "render_smoke: Im3D %s: returned %d, want %d\n", what, (int)got, (int)want);
        fail(what);
    }
}

/* The sky image <-> raster handlers (src/render/p4gl_image.c): pixel conversions, the raster format chosen for an
 * image, and an 8-bit image with a palette set into a PAL8 texture raster and read back. */
int32_t func_00407c10(void *pOut, void *pInOut, int32_t nI);
int32_t func_00407d10(void *pOut, void *pInOut, int32_t nI);
int32_t _rwSkyImageFindRasterFormat(void *pOut, void *pInOut, int32_t nI);
int32_t _rwSkyRasterSetImage(void *pOut, void *pInOut, int32_t nI);
int32_t func_00407ff0(void *pOut, void *pInOut, int32_t nI);
int32_t func_003fd460(void *pOut, void *pInOut, int32_t nI);
int32_t func_003fd520(void *pOut, void *pInOut, int32_t nI);

/* RwImage as the retail code sees it: pixel and palette pointers are guest addresses. */
typedef struct SmokeImage {
    int32_t flags, width, height, depth, stride;
    uint32_t cpPixels;
    uint32_t palette;
} SmokeImage;

static void image_conversions(void)
{
    const uint8_t rgba[4] = {10, 20, 30, 255};
    uint8_t back[4];
    uint32_t pixel = 0;
    uint8_t *indices = p4t_galloc(3 * 5, 16);
    uint8_t *palette = p4t_galloc(256 * 4, 16);
    SmokeImage *im = p4t_galloc(sizeof *im, 16);
    P4RwRaster *scratch = p4t_galloc(sizeof *scratch, 16);
    P4RwRaster *r = p4t_galloc(sizeof *r, 16);
    uint32_t px = 0;    /* lock results are guest addresses */
    uint32_t pal = 0;

    *im = (SmokeImage){0, 3, 5, 8, 3, P4_H2G(indices), P4_H2G(palette)};
    expect_int(func_00407c10(&pixel, (void *)rgba, 0x500), 1, "RGBTOPIXEL 8888");
    if (pixel != (10u | (20u << 8) | (30u << 16) | (0x80u << 24))) {
        fail("RGBTOPIXEL 8888 value");
    }
    expect_int(func_00407d10(back, &pixel, 0x500), 1, "PIXELTORGB 8888");
    if (memcmp(back, rgba, 4) != 0) {
        fail("PIXELTORGB 8888 round trip");
    }
    expect_int(func_00407c10(&pixel, (void *)rgba, 0x700), 0, "RGBTOPIXEL of an unknown format");

    /* A 3 x 5 8-bit image becomes a 4 x 8 PAL8 texture raster. */
    memset(scratch, 0, sizeof *scratch);
    expect_int(_rwSkyImageFindRasterFormat(scratch, im, 4), 1, "FINDRASTERFORMAT texture");
    expect_int(scratch->width, 4, "texture raster width rounds up to 4");
    expect_int(scratch->height, 8, "texture raster height rounds up to 8");
    expect_int(scratch->cFormat, 0x25, "an 8-bit texture is PAL8 | 8888");
    expect_int(_rwSkyImageFindRasterFormat(scratch, im, 1), 1, "FINDRASTERFORMAT z-buffer");
    expect_int(scratch->cFormat, 0, "a z-buffer has no format");

    for (int i = 0; i < 15; i++) {
        indices[i] = (uint8_t)(i % 7);
    }
    memset(palette, 0, 256 * 4);
    for (int i = 0; i < 7; i++) {
        palette[4 * i] = (uint8_t)(40 * i);
        palette[4 * i + 1] = (uint8_t)(20 + i);
        palette[4 * i + 2] = (uint8_t)(200 - i);
        palette[4 * i + 3] = (uint8_t)(255 - 20 * i);
    }
    memset(r, 0, sizeof *r);
    r->width = 3;
    r->height = 5;
    r->depth = 8;
    if (func_003f98a0(NULL, r, 0x2504) != 1) {
        fail("RASTERCREATE PAL8 | 8888");
        return;
    }
    expect_int(_rwSkyRasterSetImage(r, im, 0), 1, "RASTERSETIMAGE");
    if (func_003fc5f0(&px, r, 0) != 1 || px == 0) {
        fail("RASTERLOCK after SETIMAGE");
    } else {
        const uint8_t *rows = P4_G2H(px);

        for (int y = 0; y < 5; y++) {
            if (memcmp(rows + (size_t)y * (size_t)r->stride, indices + 3 * y, 3) != 0) {
                fail("SETIMAGE index rows");
                break;
            }
        }
        func_003fd230(NULL, r, 0);
    }
    if (func_003fd460(&pal, r, 0) != 1 || pal == 0) {
        fail("RASTERLOCKPALETTE after SETIMAGE");
    } else {
        const uint8_t *entries = P4_G2H(pal);

        expect_int(entries[4 * 3], 120, "palette red kept");
        expect_int(entries[4 * 3 + 3], (195 + 1) >> 1, "palette alpha halved to the PS2 scale");
        func_003fd520(NULL, r, 0);
    }
    memset(indices, 0xAA, 3 * 5);
    memset(palette, 0, 256 * 4);
    expect_int(func_00407ff0(im, r, 0), 1, "IMAGEGETRASTER");
    expect_int(indices[0], 0, "GETRASTER first index");
    expect_int(indices[4], 4, "GETRASTER an index");
    expect_int(palette[4 * 3], 120, "GETRASTER palette red");
    func_003fb830(NULL, r, 0);
}

static void im3d_frame(P4RwRaster *tex, const P4TmxImage *img, const char *png)
{
    /* Everything the engine keeps a pointer to is guest storage: the module's globals block is addressed as
     * ourGlobals + globalsOffset, the vertex array is held by the pending Transform. */
    uint8_t *camera = p4t_galloc(256, 16);
    uint8_t *globals = p4t_galloc(1024, 16);
    uint8_t *rgba = malloc((size_t)FRAME * FRAME * 4u);
    Im3DVertex *q = p4t_galloc(4 * sizeof *q, 16);
    Im3DVertex *tri = p4t_galloc(6 * sizeof *tri, 16);
    Im3DMatrix *ltm = p4t_galloc(sizeof *ltm, 16);
    char out[600];
    uint16_t *idx = p4t_galloc(8 * sizeof *idx, 16);

    if (rgba == NULL) {
        fail("Im3D buffers");
        return;
    }
    IM3D_MODULE.globalsOffset = (int32_t)(P4_H2G(globals) - OUR_GLOBALS);
    IM3D_MODULE.numInstances = 1;

    memset(camera, 0, 256);
    put_f32(camera, 32, 1.0f);   /* view right.x */
    put_f32(camera, 52, 1.0f);   /* view up.y */
    put_f32(camera, 72, 1.0f);   /* view at.z */
    put_f32(camera, 104, 1.0f);  /* viewWindow */
    put_f32(camera, 108, 1.0f);
    put_f32(camera, 128, 0.5f);  /* near */
    put_f32(camera, 132, 50.0f); /* far */

    /* No Transform pending: every render call and End fail. */
    expect_int(RwIm3DRenderPrimitive(IM3D_PRIM_TRILIST), 0, "RenderPrimitive before Transform");
    expect_int(RwIm3DRenderTriangle(0, 1, 2), 0, "RenderTriangle before Transform");
    expect_int(RwIm3DRenderLine(0, 1), 0, "RenderLine before Transform");
    expect_int(RwIm3DRenderIndexedPrimitive(IM3D_PRIM_TRILIST, idx, 3), 0,
               "RenderIndexedPrimitive before Transform");
    expect_int(RwIm3DEnd(), 0, "End before Transform");

    func_003f6a10(NULL, camera, 0);
    func_003f5070(P4GL_RS_TEXTURERASTER, 0);
    im3d_quad(q, 2.0f, 2.0f, 255, 255, 255, 255);
    if (RwIm3DTransform(q, 65537u, NULL, 0) != NULL) {
        fail("Im3D Transform of 65537 vertices must fail");
    }
    expect_int(RwIm3DEnd(), 0, "End after a rejected Transform");

    /* Textured quad, uv from the vertices, nearest sampling: the frame is the TMX. */
    func_003f5070(P4GL_RS_TEXTURERASTER, P4_H2G(tex));
    func_003f5070(P4GL_RS_TEXTUREFILTER, SMOKE_FILTER_NEAREST);
    p4gl_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (RwIm3DTransform(q, 4, NULL, IM3D_VERTEXUV) != (void *)q) {
        fail("Im3D Transform returns the vertex array");
    }
    expect_int(RwIm3DRenderPrimitive(IM3D_PRIM_TRISTRIP), 1, "textured RenderPrimitive");
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) != 0) {
        fail("Im3D readback");
    } else {
        static const unsigned probes[3][2] = {{128u, 128u}, {8u, 8u}, {240u, 200u}};

        for (int p = 0; p < 3; p++) {
            const uint8_t *got = rgba + ((size_t)probes[p][1] * FRAME + probes[p][0]) * 4u;
            const uint8_t *texel = texel_at(img, probes[p][0], probes[p][1]);

            if (!pixel_matches(got, texel)) {
                fprintf(stderr, "render_smoke: Im3D pixel (%u,%u) is %u %u %u, texel %u %u %u %u\n",
                        probes[p][0], probes[p][1], got[0], got[1], got[2], texel[0], texel[1],
                        texel[2], texel[3]);
                fail("Im3D textured pixel does not match the TMX texel");
            }
        }
        snprintf(out, sizeof out, "%s.im3d.png", png);
        if (p4png_write_rgba(out, FRAME, FRAME, rgba) != 0) {
            fail("Im3D PNG write");
        }
    }
    /* The uv are read whatever the flags say, so flags 0 draws the same frame. */
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RwIm3DEnd();
    RwIm3DTransform(q, 4, NULL, 0);
    RwIm3DRenderPrimitive(IM3D_PRIM_TRISTRIP);
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        const uint8_t *got = rgba + ((size_t)40 * FRAME + 200) * 4u;

        if (!pixel_matches(got, texel_at(img, 200, 40))) {
            fail("Im3D uv are read with flags 0");
        }
    }
    expect_int(RwIm3DEnd(), 1, "End after Transform");
    expect_int(RwIm3DEnd(), 0, "second End");
    func_003f5070(P4GL_RS_TEXTURERASTER, 0);

    /* Depth: a green quad at z 1.5, then a red quad whose object-space z of 1 becomes world z 3
     * through the LTM. The red quad stays behind. With the depth test off it covers the green. */
    func_003f5070(P4GL_RS_ZWRITEENABLE, 1);
    memset(ltm, 0, sizeof *ltm);
    ltm->right[0] = 1.0f;
    ltm->up[1] = 1.0f;
    ltm->at[2] = 1.0f;
    ltm->pos[2] = 2.0f;
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    im3d_quad(q, 1.5f, 1.8f, 0, 255, 0, 255);
    RwIm3DTransform(q, 4, NULL, 0);
    expect_int(RwIm3DRenderPrimitive(IM3D_PRIM_TRISTRIP), 1, "green RenderPrimitive");
    RwIm3DEnd();
    im3d_quad(q, 1.0f, 1.2f, 255, 0, 0, 255);
    RwIm3DTransform(q, 4, ltm, 0);
    expect_int(RwIm3DRenderPrimitive(IM3D_PRIM_TRISTRIP), 1, "red RenderPrimitive");
    RwIm3DEnd();
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 128, 128, 0, 255, 0, "depth test keeps the nearer quad");
    }
    func_003f5070(P4GL_RS_ZTESTENABLE, 0);
    func_003f5070(P4GL_RS_ZWRITEENABLE, 0);
    RwIm3DTransform(q, 4, ltm, 0);
    RwIm3DRenderPrimitive(IM3D_PRIM_TRISTRIP);
    RwIm3DEnd();
    func_003f5070(P4GL_RS_ZTESTENABLE, 1);
    func_003f5070(P4GL_RS_ZWRITEENABLE, 1);
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 128, 128, 255, 0, 0, "depth test off draws over the nearer quad");
    }

    /* RpSky ALPHA_1 0x48 (Cs * As + Cd) adds to the destination: half-alpha red over a (0,0,128)
     * clear is (128,0,128). TEST_1 0x71801 (ATST NEVER, AFAIL FB_ONLY, the game's own value) still
     * writes colour; TEST_1 0x1 (ATST NEVER, AFAIL KEEP) writes none. */
    p4gl_glClearColor(0.0f, 0.0f, 128.0f / 255.0f, 1.0f);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    p4gl_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    RpSkyRenderStateSet(2, 0x48);
    im3d_quad(q, 2.0f, 2.0f, 255, 0, 0, 128);
    RwIm3DTransform(q, 4, NULL, 0);
    RwIm3DRenderPrimitive(IM3D_PRIM_TRISTRIP);
    RwIm3DEnd();
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 128, 128, 128, 0, 128, "additive ALPHA_1 0x48");
    }
    RpSkyRenderStateSet(2, 0x44);
    RpSkyRenderStateSet(3, 0x71801);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    im3d_quad(q, 2.0f, 2.0f, 0, 255, 0, 255);
    RwIm3DTransform(q, 4, NULL, 0);
    RwIm3DRenderPrimitive(IM3D_PRIM_TRISTRIP);
    RwIm3DEnd();
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 128, 128, 0, 255, 0, "TEST_1 NEVER with FB_ONLY writes colour");
    }
    RpSkyRenderStateSet(3, 0x1);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RwIm3DTransform(q, 4, NULL, 0);
    RwIm3DRenderPrimitive(IM3D_PRIM_TRISTRIP);
    RwIm3DEnd();
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 128, 128, 0, 0, 0, "TEST_1 NEVER with KEEP writes nothing");
    }
    RpSkyRenderStateSet(3, 0);

    /* The dream battle's crossed haze uses GEQUAL 127 / RGB_ONLY (0x35fb).
     * Subthreshold fragments colour the image but must preserve both Ad and Z. */
    p4gl_glClearColor(0.0f, 0.0f, 0.0f, 64.0f / 255.0f);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RpSkyRenderStateSet(3, 0x35fb);
    im3d_quad(q, 1.5f, 1.8f, 255, 0, 0, 128);
    RwIm3DTransform(q, 4, NULL, 0);
    RwIm3DRenderPrimitive(IM3D_PRIM_TRISTRIP);
    RwIm3DEnd();
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 128, 128, 128, 0, 0, "RGB_ONLY failure still blends RGB");
        expect_int(rgba[((size_t)128 * FRAME + 128) * 4u + 3u], 64,
                   "RGB_ONLY failure preserves destination alpha");
    }
    RpSkyRenderStateSet(3, 0);
    im3d_quad(q, 2.0f, 2.0f, 0, 255, 0, 255);
    RwIm3DTransform(q, 4, NULL, 0);
    RwIm3DRenderPrimitive(IM3D_PRIM_TRISTRIP);
    RwIm3DEnd();
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 128, 128, 0, 255, 0, "RGB_ONLY failure does not occlude later geometry");
    }
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RpSkyRenderStateSet(3, 0x35fb);
    im3d_quad(q, 1.5f, 1.8f, 255, 0, 0, 255);
    RwIm3DTransform(q, 4, NULL, 0);
    RwIm3DRenderPrimitive(IM3D_PRIM_TRISTRIP);
    RwIm3DEnd();
    RpSkyRenderStateSet(3, 0);
    im3d_quad(q, 2.0f, 2.0f, 0, 255, 0, 255);
    RwIm3DTransform(q, 4, NULL, 0);
    RwIm3DRenderPrimitive(IM3D_PRIM_TRISTRIP);
    RwIm3DEnd();
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 128, 128, 255, 0, 0, "RGB_ONLY passing fragments still write depth");
    }
    p4gl_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    /* CULLMODE: the quad runs clockwise in the image (top-left, top-right, bottom-left). Front faces
     * run counter-clockwise in the image (checked on a closed solid of the game's data, whose inside
     * faces show when the sense is the other way), so CULLBACK removes this quad and CULLFRONT keeps it. */
    im3d_quad(q, 2.0f, 2.0f, 255, 255, 255, 255);
    func_003f5070(20, 2);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RwIm3DTransform(q, 4, NULL, 0);
    RwIm3DRenderPrimitive(IM3D_PRIM_TRISTRIP);
    RwIm3DEnd();
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 128, 128, 0, 0, 0, "CULLBACK removes a clockwise quad");
    }
    func_003f5070(20, 3);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RwIm3DTransform(q, 4, NULL, 0);
    RwIm3DRenderPrimitive(IM3D_PRIM_TRISTRIP);
    RwIm3DEnd();
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 128, 128, 255, 255, 255, "CULLFRONT keeps a clockwise quad");
    }
    func_003f5070(20, 1);

    /* ALLOPAQUE forces alpha to 1; without it a zero vertex alpha blends to nothing. */
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    im3d_quad(q, 2.0f, 2.0f, 0, 0, 255, 0);
    RwIm3DTransform(q, 4, NULL, 0);
    RwIm3DRenderPrimitive(IM3D_PRIM_TRISTRIP);
    RwIm3DEnd();
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 128, 128, 0, 0, 0, "zero alpha blends away");
    }
    RwIm3DTransform(q, 4, NULL, IM3D_ALLOPAQUE);
    RwIm3DRenderPrimitive(IM3D_PRIM_TRISTRIP);
    RwIm3DEnd();
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 128, 128, 0, 0, 255, "ALLOPAQUE forces alpha 1");
    }

    /* Indexed primitives, the index rounding and the rejected inputs. */
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    im3d_quad(q, 2.0f, 2.0f, 255, 255, 0, 255);
    RwIm3DTransform(q, 4, NULL, 0);
    idx[0] = 0; idx[1] = 1; idx[2] = 2; idx[3] = 2; idx[4] = 1; idx[5] = 3; idx[6] = 9;
    expect_int(RwIm3DRenderIndexedPrimitive(IM3D_PRIM_TRILIST, idx, 7), 1,
               "indexed triangle list of 7 indices (rounds to 6)");
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 128, 128, 255, 255, 0, "indexed triangle list fills the frame");
        expect_rgb(rgba, 240, 240, 255, 255, 0, "indexed triangle list covers the corner");
    }
    idx[0] = 0; idx[1] = 1; idx[2] = 4;
    expect_int(RwIm3DRenderIndexedPrimitive(IM3D_PRIM_TRILIST, idx, 3), 0,
               "indexed draw with an index past the vertices");
    idx[0] = 0; idx[1] = 1; idx[2] = 2; idx[3] = 3;
    expect_int(RwIm3DRenderIndexedPrimitive(IM3D_PRIM_TRISTRIP, idx, 4), 1, "indexed strip");
    expect_int(RwIm3DRenderIndexedPrimitive(IM3D_PRIM_LINELIST, idx, 3), 1,
               "indexed line list of 3 indices (rounds to 2)");
    expect_int(RwIm3DRenderIndexedPrimitive(IM3D_PRIM_TRILIST, idx, -1), 1,
               "indexed triangle list of a negative count rounds to nothing");
    expect_int(RwIm3DRenderIndexedPrimitive(0, idx, 4), 0, "indexed draw of primitive type 0");
    expect_int(RwIm3DRenderPrimitive(9), 0, "RenderPrimitive of primitive type 9");
    expect_int(RwIm3DRenderPrimitive(0), 0, "RenderPrimitive of primitive type 0");

    /* One triangle (top-left half of the quad) and one line. */
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    expect_int(RwIm3DRenderTriangle(0, 1, 2), 1, "RenderTriangle");
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 20, 20, 255, 255, 0, "RenderTriangle covers its half");
        expect_rgb(rgba, 235, 235, 0, 0, 0, "RenderTriangle leaves the other half");
    }
    expect_int(RwIm3DRenderTriangle(0, 1, 7), 0, "RenderTriangle with a bad index");
    expect_int(RwIm3DRenderLine(0, 3), 1, "RenderLine");
    expect_int(RwIm3DRenderLine(0, 7), 0, "RenderLine with a bad index");
    memset(tri, 0, 6 * sizeof *tri);
    expect_int(RwIm3DEnd(), 1, "final End");
    func_003f73d0(NULL, camera, 0);

    /* Outside a camera update Transform fails. */
    expect_int(RwIm3DTransform(q, 4, NULL, 0) == NULL, 1, "Transform outside a camera update");
    expect_int(RwIm3DEnd(), 1, "End after the pool was written");

    IM3D_MODULE.globalsOffset = 0;
    IM3D_MODULE.numInstances = 0;
    free(rgba);
}

/* GS RGBA is screen-affine even when STQ uses perspective. A triangle with W
 * 1, 2, 4 distinguishes it from GL's default (roughly 185,46,23 at the probe). */
static void affine_colour_frame(void)
{
    const P4ClipVertex v[3] = {
        {{-0.75f, -0.75f, 0.0f, 1.0f}, {1, 0, 0, 1}, {0, 0}},
        {{ 1.50f, -1.50f, 0.0f, 2.0f}, {0, 1, 0, 1}, {0, 0}},
        {{-3.00f,  3.00f, 0.0f, 4.0f}, {0, 0, 1, 1}, {0, 0}},
    };
    const uint32_t indices[3] = {0, 1, 2};
    const P4ClipDrawParams dp = {.color = {1, 1, 1, 1}, .own = 1};
    uint8_t *rgba = malloc((size_t)FRAME * FRAME * 4u);

    if (rgba == NULL) {
        fail("affine colour readback allocation");
        return;
    }
    p4gl_glClearColor(0, 0, 0, 1);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    expect_int(p4gl_clip_draw_triangles(v, 3, indices, 3, &dp), 1, "affine colour triangle");
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) != 0) {
        fail("affine colour readback");
    } else {
        expect_rgb(rgba, 80, 176, 128, 64, 63, "GS screen-affine colour");
    }
    free(rgba);
}

/* PS2PTank (overrides/003a8840.c, src/render/p4gl_ptank.c). A synthetic atomic carries a PTank record
 * in the retail layout: the plugin offset iGpffffb610 gives the record pointer, which holds the
 * active count, the platform flags (2 is the 3D sprite generator) and publicData with the position,
 * size and colour clusters. The camera is the Im3D test's, so a billboard of width 2z at depth z
 * fills the frame. */
int32_t func_003a8840(void *node, void *params);
#define PTANK_OFFSET P4_GDATA(int32_t, 0x00764700u)          /* iGpffffb610, the PTank plugin offset */

static void put_u32(uint8_t *base, size_t off, uint32_t value)
{
    memcpy(base + off, &value, sizeof value);
}

static void ptank_frame(P4RwRaster *tex, const P4TmxImage *img)
{
    /* The synthetic atomic and everything it points to is guest storage: the PTank code reads these objects
     * through the guest addresses stored in them. */
    uint8_t *camera = p4t_galloc(256, 16);
    uint8_t *atomic = p4t_galloc(64, 16);
    uint8_t *geometry = p4t_galloc(128, 16);
    uint8_t *material = p4t_galloc(16, 16);
    uint8_t *texture = p4t_galloc(16, 16);
    uint8_t *record = p4t_galloc(0x200, 16);
    uint32_t *materials = p4t_galloc(sizeof(uint32_t), 16);
    float (*positions)[4] = p4t_galloc(2 * sizeof *positions, 16);
    float (*sizes)[2] = p4t_galloc(2 * sizeof *sizes, 16);
    uint8_t (*colors)[4] = p4t_galloc(2 * sizeof *colors, 16);
    uint8_t *rgba = malloc((size_t)FRAME * FRAME * 4u);
    uint32_t *nodeParam = p4t_galloc(sizeof(uint32_t), 16);   /* the RxPipelineNodeParam: dataParam is the atomic's guest address */
    const int32_t savedOffset = PTANK_OFFSET;
    const size_t pd = 0x50;  /* publicData */

    if (rgba == NULL) {
        fail("PTank buffers");
        return;
    }
    memset(camera, 0, 256);
    put_f32(camera, 32, 1.0f);
    put_f32(camera, 52, 1.0f);
    put_f32(camera, 72, 1.0f);
    put_f32(camera, 104, 1.0f);
    put_f32(camera, 108, 1.0f);
    put_f32(camera, 128, 0.5f);
    put_f32(camera, 132, 50.0f);

    /* Atomic: no frame (identity LTM), geometry -> material list -> material -> texture -> raster. */
    memset(atomic, 0, 64);
    memset(geometry, 0, 128);
    memset(material, 0, 16);
    memset(texture, 0, 16);
    put_u32(texture, 0, P4_H2G(tex));
    put_u32(material, 0, P4_H2G(texture));
    memset(material + 4, 255, 4);
    materials[0] = P4_H2G(material);
    put_u32(geometry, 32, P4_H2G(materials));
    put_u32(atomic, 24, P4_H2G(geometry));
    PTANK_OFFSET = 40;
    put_u32(atomic, 40, P4_H2G(record));

    /* Record: maxPCount 2, actPCount 2, platFlags 2. Clusters: position (0), size (3), colour (4);
     * data flags POSITION | COLOUR | SIZE; vertexAlphaBlend on; cUV is the whole texture. */
    memset(record, 0, 0x200);
    put_u32(record, 0x00, 2);
    put_u32(record, 0x04, 2);
    put_u32(record, 0x44, 2);
    put_u32(record, pd + 0x04, P4_H2G(positions));
    put_u32(record, pd + 0x08, sizeof positions[0]);
    put_u32(record, pd + 0x04 + 8 * 3, P4_H2G(sizes));
    put_u32(record, pd + 0x08 + 8 * 3, sizeof sizes[0]);
    put_u32(record, pd + 0x04 + 8 * 4, P4_H2G(colors));
    put_u32(record, pd + 0x08 + 8 * 4, sizeof colors[0]);
    put_u32(record, pd + 0x58, 0x1 | 0x2 | 0x4);
    put_u32(record, pd + 0x64, 1);
    put_f32(record, pd + 0x90 + 8, 1.0f);
    put_f32(record, pd + 0x90 + 12, 1.0f);

    nodeParam[0] = P4_H2G(atomic);
    func_003f6a10(NULL, camera, 0);
    func_003f5070(P4GL_RS_TEXTUREFILTER, SMOKE_FILTER_NEAREST);
    func_003f5070(P4GL_RS_TEXTURERASTER, 0);
    p4gl_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    /* Textured billboard filling the frame: the frame is the TMX. */
    memset(positions, 0, 2 * sizeof *positions);
    positions[0][2] = 2.0f;
    positions[1][2] = 100.0f;   /* beyond the far plane: clipped */
    sizes[0][0] = sizes[0][1] = 4.0f;
    sizes[1][0] = sizes[1][1] = 1.0f;
    memset(colors, 255, 2 * sizeof *colors);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    expect_int(func_003a8840(NULL, nodeParam), 1, "PTank draw");
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        const unsigned probes[3][2] = {{128u, 128u}, {8u, 8u}, {230u, 40u}};

        for (int p = 0; p < 3; p++) {
            const uint8_t *got = rgba + ((size_t)probes[p][1] * FRAME + probes[p][0]) * 4u;

            if (!pixel_matches(got, texel_at(img, probes[p][0], probes[p][1]))) {
                fail("PTank textured pixel does not match the TMX texel");
            }
        }
    }

    /* Untextured red particle of size 2 at z 2: the centre quarter of the frame. */
    material[0] = material[1] = material[2] = material[3] = 0;
    sizes[0][0] = sizes[0][1] = 2.0f;
    colors[0][0] = 255; colors[0][1] = 0; colors[0][2] = 0; colors[0][3] = 255;
    put_u32(geometry, 32, 0);   /* no material list: no texture, white material colour */
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    expect_int(func_003a8840(NULL, nodeParam), 1, "PTank untextured draw");
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 128, 128, 255, 0, 0, "PTank particle covers the centre");
        expect_rgb(rgba, 8, 8, 0, 0, 0, "PTank particle leaves the corner");
    }

    /* A 4 x 1 particle is a horizontal band; rotated by a quarter turn it is a vertical band. */
    sizes[0][0] = 4.0f;
    sizes[0][1] = 1.0f;
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    func_003a8840(NULL, nodeParam);
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 20, 128, 255, 0, 0, "PTank band is horizontal");
        expect_rgb(rgba, 128, 20, 0, 0, 0, "PTank band is not vertical");
    }
    put_f32(record, pd + 0x78, 1.5707963f);   /* cRotate */
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    func_003a8840(NULL, nodeParam);
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 128, 20, 255, 0, 0, "PTank rotated band is vertical");
        expect_rgb(rgba, 20, 128, 0, 0, 0, "PTank rotated band is not horizontal");
    }

    /* The 2D sprite generator is skipped. */
    put_u32(record, 0x44, 0);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    expect_int(func_003a8840(NULL, nodeParam), 0, "PTank 2D generator is skipped");

    /* With a matrix per particle (the field rain, data flags colour | matrix) the generator does not
     * matter: the matrix places a 2 x 2 quad at z 2, centre quarter of the frame. */
    {
        static const float matrixInit[16] = {1.0f, 0, 0, 0,  0, 1.0f, 0, 0,  0, 0, 1.0f, 0,  0, 0, 2.0f, 0};
        float *matrix = p4t_galloc(sizeof matrixInit, 16);

        memcpy(matrix, matrixInit, sizeof matrixInit);

        put_u32(record, pd + 0x58, 0x2 | 0x8);
        put_u32(record, pd + 0x04 + 8 * 1, P4_H2G(matrix));
        put_u32(record, pd + 0x08 + 8 * 1, 64);
        put_u32(record, pd + 0x04, 0);
        p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        put_u32(record, 0x04, 1);
        expect_int(func_003a8840(NULL, nodeParam), 1, "PTank matrix particle draw");
        if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
            expect_rgb(rgba, 128, 128, 255, 0, 0, "PTank matrix particle covers the centre");
            expect_rgb(rgba, 8, 8, 0, 0, 0, "PTank matrix particle leaves the corner");
        }
    }

    func_003f73d0(NULL, camera, 0);
    func_003f5070(P4GL_RS_TEXTURERASTER, 0);
    PTANK_OFFSET = savedOffset;
    free(rgba);
}

/* The draw raster (retail skyDrawRaster, gp 0x007649F4) shows the frame so far: a red rectangle is
 * drawn in the top-left corner, the raster is bound as the texture, and a half-size copy of the
 * whole frame is drawn in the bottom-right. Its top-left quarter must be red, which also checks
 * that the copy is not flipped. */
static void im2d_rect(float x0, float y0, float x1, float y1, float r, float g, float b, int textured)
{
    P4Sky2Vertex v[4];
    const float cx[4] = {x0, x1, x0, x1};
    const float cy[4] = {y0, y0, y1, y1};
    const float us[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    const float vs[4] = {0.0f, 0.0f, 1.0f, 1.0f};

    memset(v, 0, sizeof v);
    for (int i = 0; i < 4; i++) {
        v[i].x = cx[i];
        v[i].y = cy[i];
        v[i].camZ = 1.0f;
        v[i].u = textured ? us[i] : 0.0f;
        v[i].v = textured ? vs[i] : 0.0f;
        v[i].recipZ = 1.0f;
        v[i].r = r * 255.0f;
        v[i].g = g * 255.0f;
        v[i].b = b * 255.0f;
        v[i].a = 255.0f;
    }
    if (func_0040c0f0(SMOKE_PRIM_TRISTRIP, (uint8_t *)v, 4) != 1) {
        fail("Im2D rect");
    }
}

/* Retail Sky's 12.4 conversion precedes rasterization, not just pixel-edge ownership.
 * The negative case retains the original dreambtl feedback quad's source coordinates:
 * (111.2368927,-92.5898895) becomes (111.1875,-92.625), not truncation to -92.5625.
 * Compare the complete draw against explicit GS-grid inputs on this same device so the
 * witness does not depend on GL line rules or a particular GPU's integer-edge tie rule. */
static void im2d_fractional_frame(void)
{
    static const float rectangles[][4] = {
        {32.02f, 32.02f, 128.02f, 128.02f},
        {111.2368927f, -92.5898895f, 391.2368774f, 187.4101105f},
        {-0.02f, 31.98f, 128.02f, 160.02f},
    };
    static const float triangles[][6] = {
        {-0.02f, 64.02f, 128.02f, 192.02f, 128.02f, 64.02f},
        {64.02f, -0.02f, 192.02f, 128.02f, 64.02f, 128.02f},
        {111.2368927f, -92.5898895f, 111.2368927f, 187.4101105f,
         391.2368774f, 187.4101105f},
    };
    const size_t bytes = (size_t)FRAME * FRAME * 4u;
    uint8_t *actual = malloc(bytes);
    uint8_t *expected = malloc(bytes);
    int negativeDistinguished = 0;

    if (actual == NULL || expected == NULL) {
        fail("Im2D fractional readback buffers");
        free(actual);
        free(expected);
        return;
    }
    func_003f5070(P4GL_RS_TEXTURERASTER, 0);
    p4gl_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    for (unsigned i = 0; i < sizeof rectangles / sizeof rectangles[0]; i++) {
        const float *r = rectangles[i];

        p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        im2d_rect(r[0], r[1], r[2], r[3], 1.0f, 1.0f, 1.0f, 0);
        if (p4gl_readback_rgba(FRAME, FRAME, actual) != 0) {
            fail("Im2D fractional readback");
            continue;
        }
        p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        im2d_rect(floorf(r[0] * 16.0f) / 16.0f, floorf(r[1] * 16.0f) / 16.0f,
                  floorf(r[2] * 16.0f) / 16.0f, floorf(r[3] * 16.0f) / 16.0f,
                  1.0f, 1.0f, 1.0f, 0);
        if (p4gl_readback_rgba(FRAME, FRAME, expected) != 0) {
            fail("Im2D GS-grid readback");
        } else if (memcmp(actual, expected, bytes) != 0) {
            fail("Im2D fractional coordinates must rasterize on the GS 12.4 grid");
        }
    }
    /* Offscreen vertices still determine visible diagonals. The opposite orientations
     * ensure a truncation-to-zero implementation cannot hide behind an edge tie. */
    for (unsigned i = 0; i < sizeof triangles / sizeof triangles[0]; i++) {
        P4Sky2Vertex v[3];

        for (unsigned pass = 0; pass < 3; pass++) {
            memset(v, 0, sizeof v);
            for (unsigned vertex = 0; vertex < 3; vertex++) {
                const float x = triangles[i][2 * vertex];
                const float y = triangles[i][2 * vertex + 1];

                v[vertex].x = pass == 0 ? x : (pass == 1 ? floorf(x * 16.0f) : truncf(x * 16.0f)) / 16.0f;
                v[vertex].y = pass == 0 ? y : (pass == 1 ? floorf(y * 16.0f) : truncf(y * 16.0f)) / 16.0f;
                v[vertex].camZ = 1.0f;
                v[vertex].recipZ = 1.0f;
                v[vertex].r = v[vertex].g = v[vertex].b = v[vertex].a = 255.0f;
            }
            p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            if (func_0040c0f0(SMOKE_PRIM_TRISTRIP, (uint8_t *)v, 3) != 1 ||
                p4gl_readback_rgba(FRAME, FRAME, pass == 0 ? actual : expected) != 0) {
                fail("Im2D fractional diagonal draw/readback");
                break;
            }
            if (pass == 1 && memcmp(actual, expected, bytes) != 0) {
                fail("Im2D offscreen diagonals must use the GS 12.4 grid");
            } else if (pass == 2 && memcmp(actual, expected, bytes) != 0) {
                negativeDistinguished = 1;
            }
        }
    }
    if (!negativeDistinguished) {
        fail("Im2D negative-coordinate witness must distinguish floor from truncation");
    }
    free(actual);
    free(expected);
}

/* The frozen software-GS AA1 line path splits 16-bit coverage between adjacent
 * minor-axis pixels. A 0.25-pixel offset gives factors95/128 and31/128; AA1
 * keeps an already partial source alpha when ABE is enabled. */
static void im2d_line_frame(void)
{
    static const float xy[6][2] = {
        {16.25f, 40.25f}, {80.25f, 40.25f},
        {100.25f, 16.25f}, {100.25f, 80.25f},
        {140.25f, 40.25f}, {204.25f, 40.25f},
    };
    uint16_t reversed[4] = {1, 0, 3, 2};
    P4Sky2Vertex v[6];
    const size_t bytes = (size_t)FRAME * FRAME * 4u;
    uint8_t *rgba = malloc(bytes);
    uint8_t *reference = malloc(bytes);

    if (rgba == NULL || reference == NULL) {
        fail("AA1 line readback buffers");
        free(rgba);
        free(reference);
        return;
    }
    memset(v, 0, sizeof v);
    for (unsigned i = 0; i < 6; i++) {
        v[i].x = xy[i][0];
        v[i].y = xy[i][1];
        v[i].recipZ = 1.0f;
        v[i].r = v[i].g = v[i].b = 255.0f;
        v[i].a = i < 4 ? 255.0f : 128.0f;
    }
    func_003f5070(P4GL_RS_TEXTURERASTER, 0);
    func_003f5070(P4GL_RS_VERTEXALPHAENABLE, 1);
    func_003f5070(P4GL_RS_ZTESTENABLE, 0);
    func_003f5070(P4GL_RS_ZWRITEENABLE, 0);
    RpSkyRenderStateSet(2, 0x44);
    RpSkyRenderStateSet(3, 0);
    p4gl_glClearColor(0, 0, 0, 1);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    expect_int(func_0040c0f0(1, (uint8_t *)v, 6), 1, "AA1 line list");
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) != 0) {
        fail("AA1 line list readback");
    } else {
        expect_rgb(rgba, 30, 40, 189, 189, 189, "AA1 horizontal primary coverage");
        expect_rgb(rgba, 30, 41, 61, 61, 61, "AA1 horizontal neighbour coverage");
        expect_rgb(rgba, 100, 30, 189, 189, 189, "AA1 vertical primary coverage");
        expect_rgb(rgba, 101, 30, 61, 61, 61, "AA1 vertical neighbour coverage");
        expect_rgb(rgba, 150, 40, 128, 128, 128, "AA1 keeps partial source alpha");
        expect_rgb(rgba, 150, 41, 128, 128, 128, "AA1 neighbour keeps partial source alpha");
        expect_rgb(rgba, 16, 40, 0, 0, 0, "AA1 first endpoint diamond exit");
        expect_rgb(rgba, 80, 40, 189, 189, 189, "AA1 last endpoint diamond exit");
        expect_rgb(rgba, 81, 40, 0, 0, 0, "AA1 endpoint excludes following pixel");
    }
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    expect_int(func_0040c0f0(1, (uint8_t *)v, 4), 1, "AA1 reference line list");
    if (p4gl_readback_rgba(FRAME, FRAME, reference) != 0) {
        fail("AA1 reference readback");
    } else {
        p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        expect_int(func_0040d0a0(1, (uint8_t *)v, 6, reversed, 4), 1, "AA1 indexed reversed lines");
        if (p4gl_readback_rgba(FRAME, FRAME, rgba) != 0 ||
            memcmp(rgba, reference, bytes) != 0) fail("AA1 indexed/reversed coverage");
    }
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    expect_int(func_0040c0f0(2, (uint8_t *)v, 2), 1, "AA1 polyline");
    if (p4gl_readback_rgba(FRAME, FRAME, reference) != 0) {
        fail("AA1 polyline readback");
    } else {
        p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        expect_int(func_0040afb0((uint8_t *)v, 6, 1, 0), 1, "AA1 single indexed line");
        if (p4gl_readback_rgba(FRAME, FRAME, rgba) != 0 ||
            memcmp(rgba, reference, bytes) != 0) fail("AA1 indexed line/polyline coverage");
    }
    func_003f5070(P4GL_RS_VERTEXALPHAENABLE, 0);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    expect_int(func_0040afb0((uint8_t *)v, 6, 4, 5), 1, "AA1 without ABE");
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 150, 40, 189, 189, 189, "AA1 without ABE substitutes coverage");
        expect_rgb(rgba, 150, 41, 61, 61, 61, "AA1 without ABE neighbour coverage");
    } else {
        fail("AA1 without ABE readback");
    }
    /* A following ordinary draw uses the non-instanced VAO and its zero divisors. */
    func_003f5070(P4GL_RS_ZTESTENABLE, 1);
    func_003f5070(P4GL_RS_ZWRITEENABLE, 1);
    v[0].z = v[1].z = 65535.0f;
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    expect_int(func_0040afb0((uint8_t *)v, 6, 0, 1), 1, "AA1 near line never writes Z");
    im2d_rect(16, 16, 80, 80, 1, 0, 0, 0);
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 40, 40, 255, 0, 0, "ordinary Im2D after instanced lines");
    } else {
        fail("ordinary Im2D after AA1 readback");
    }
    free(rgba);
    free(reference);
}

static void frame_raster_frame(void)
{
    uint8_t *rgba = malloc((size_t)FRAME * FRAME * 4u);
    const void *drawRaster = P4_G2H(P4_G32(0x007649F4u));
    const void *displayRaster = P4_G2H(P4_G32(0x007649F0u));

    if (rgba == NULL) {
        fail("frame raster buffer");
        return;
    }
    if (drawRaster == NULL || displayRaster == NULL || drawRaster == displayRaster ||
        (const void *)P4_G2H(func_00401b80()) != displayRaster) {
        fail("frame rasters exist and RpSkyGetDisplayBufferRaster returns the display one");
        free(rgba);
        return;
    }
    p4gl_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    func_003f5070(P4GL_RS_TEXTURERASTER, 0);
    im2d_rect(0.0f, 0.0f, 128.0f, 128.0f, 1.0f, 0.0f, 0.0f, 0);
    func_003f5070(P4GL_RS_TEXTURERASTER, P4_H2G(drawRaster));
    func_003f5070(P4GL_RS_TEXTUREFILTER, SMOKE_FILTER_NEAREST);
    im2d_rect(128.0f, 128.0f, 256.0f, 256.0f, 1.0f, 1.0f, 1.0f, 1);
    func_003f5070(P4GL_RS_TEXTURERASTER, 0);
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) == 0) {
        expect_rgb(rgba, 10, 10, 255, 0, 0, "frame: the source red rectangle");
        expect_rgb(rgba, 140, 140, 255, 0, 0, "frame: copy shows the red corner at its top left");
        expect_rgb(rgba, 140, 250, 0, 0, 0, "frame: copy is dark at its bottom left (not flipped)");
        expect_rgb(rgba, 250, 140, 0, 0, 0, "frame: copy is dark at its top right");
    }
    free(rgba);
}

/* RwTextureAddressMode values (decomp/include/rw/sky2/rwplcore.h:5385): wrap 1, mirror 2, clamp 3, border 4. */
#define SMOKE_ADDRESS_MIRROR 2
#define SMOKE_ADDRESS_CLAMP 3
#define SMOKE_ADDRESS_BORDER 4

/* Texel index of i on an axis of n texels under an address mode; *outside is set for a border mode's transparent black. */
static int border_index(int i, int n, int mode, int *outside)
{
    *outside = 0;
    switch (mode) {
    case SMOKE_ADDRESS_BORDER:
        *outside = i < 0 || i >= n;
        return i < 0 ? 0 : i >= n ? n - 1 : i;
    case SMOKE_ADDRESS_CLAMP:
        return i < 0 ? 0 : i >= n ? n - 1 : i;
    case SMOKE_ADDRESS_MIRROR: {
        const int p = 2 * n;
        const int m = ((i % p) + p) % p;

        return m < n ? m : p - 1 - m;
    }
    default:
        return ((i % n) + n) % n;
    }
}

static void border_texel(const P4TmxImage *img, int ix, int iy, int modeU, int modeV, double out[4])
{
    int zu, zv;
    const int x = border_index(ix, (int)img->width, modeU, &zu);
    const int y = border_index(iy, (int)img->height, modeV, &zv);
    const uint8_t *t = img->rgba + ((size_t)y * img->width + (size_t)x) * 4u;

    for (int c = 0; c < 4; c++) {
        out[c] = (zu || zv) ? 0.0 : (double)t[c];
    }
}

/* What the sampler returns at texel position (tx, ty) (texel units, the bias already in): nearest, or the bilinear blend of the four
 * texels around it, each looked up under the address modes. */
static void border_model(const P4TmxImage *img, double tx, double ty, int modeU, int modeV, int linear, double out[4])
{
    if (!linear) {
        border_texel(img, (int)floor(tx), (int)floor(ty), modeU, modeV, out);
    } else {
        const double px = tx - 0.5;
        const double py = ty - 0.5;
        const int ix = (int)floor(px);
        const int iy = (int)floor(py);
        const double fx = px - ix;
        const double fy = py - iy;
        double a[4], b[4], c[4], d[4];

        border_texel(img, ix, iy, modeU, modeV, a);
        border_texel(img, ix + 1, iy, modeU, modeV, b);
        border_texel(img, ix, iy + 1, modeU, modeV, c);
        border_texel(img, ix + 1, iy + 1, modeU, modeV, d);
        for (int k = 0; k < 4; k++) {
            out[k] = (a[k] * (1.0 - fx) + b[k] * fx) * (1.0 - fy) + (c[k] * (1.0 - fx) + d[k] * fx) * fy;
        }
    }
}

/* A full-frame Im2D quad whose UVs run -1..2, so the texture fills the middle third and the rest is outside it, drawn over a blue clear
 * colour with the default source-alpha blend. The model above says what each pixel must be under the address modes and the filter;
 * every probe of a 7-pixel grid is compared (nearest probes that sit within 0.02 texel of a texel edge are skipped, the rounding there is the
 * hardware's). With address mode border the outside is transparent black, which leaves the blue. This is the path that is hardware
 * GL_CLAMP_TO_BORDER on desktop GL, and the fragment shader's emulation (p4SampleBorder) on OpenGL ES without it
 * (P4PC_GLES_BORDER=shader forces it). Im2D adds 1/64 texel to the sample (p4gl_im2d.c), so the model does. */
static void border_case(P4RwRaster *tex, const P4TmxImage *img, int addressU, int addressV, int filter, const char *what)
{
    P4Sky2Vertex v[4];
    uint8_t *rgba = malloc((size_t)FRAME * FRAME * 4u);
    const float corners[4][2] = {{0.0f, 0.0f}, {(float)FRAME, 0.0f}, {0.0f, (float)FRAME}, {(float)FRAME, (float)FRAME}};
    const float uvs[4][2] = {{-1.0f, -1.0f}, {2.0f, -1.0f}, {-1.0f, 2.0f}, {2.0f, 2.0f}};
    unsigned checked = 0, bad = 0;

    if (rgba == NULL) {
        fail("border frame buffer");
        return;
    }
    func_003f5070(P4GL_RS_TEXTURERASTER, P4_H2G(tex));
    func_003f5070(P4GL_RS_TEXTUREFILTER, (uint32_t)filter);
    func_003f5070(P4GL_RS_TEXTUREADDRESSU, (uint32_t)addressU);
    func_003f5070(P4GL_RS_TEXTUREADDRESSV, (uint32_t)addressV);
    p4gl_glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    memset(v, 0, sizeof v);
    for (int i = 0; i < 4; i++) {
        v[i].x = corners[i][0];
        v[i].y = corners[i][1];
        v[i].camZ = 1.0f;
        v[i].u = uvs[i][0];
        v[i].v = uvs[i][1];
        v[i].recipZ = 1.0f;
        v[i].r = v[i].g = v[i].b = v[i].a = 255.0f;
    }
    if (func_0040c0f0(SMOKE_PRIM_TRISTRIP, (uint8_t *)v, 4) != 1) {
        fail("border quad");
    }
    if (p4gl_readback_rgba(FRAME, FRAME, rgba) != 0) {
        fail("border readback");
    } else {
        for (unsigned py = 3; py < FRAME; py += 7) {
            for (unsigned px = 3; px < FRAME; px += 7) {
                /* Im2D shifts the quad half a pixel: pixel p samples at p / FRAME of the quad's span. */
                const double tx = ((double)px / FRAME * 3.0 - 1.0) * (double)img->width + 1.0 / 64.0;
                const double ty = ((double)py / FRAME * 3.0 - 1.0) * (double)img->height + 1.0 / 64.0;
                const int linear = filter != SMOKE_FILTER_NEAREST;
                double s[4];
                double want[3];
                const uint8_t *got = rgba + ((size_t)py * FRAME + px) * 4u;
                int worst = 0;

                if (!linear) {
                    const double fx = tx - floor(tx);
                    const double fy = ty - floor(ty);

                    if (fx < 0.02 || fx > 0.98 || fy < 0.02 || fy > 0.98) {
                        continue;
                    }
                }
                border_model(img, tx, ty, addressU, addressV, linear, s);
                for (int c = 0; c < 3; c++) {
                    const double bg = c == 2 ? 255.0 : 0.0;

                    want[c] = (s[c] * s[3] + bg * (255.0 - s[3])) / 255.0;
                    if (abs((int)got[c] - (int)(want[c] + 0.5)) > worst) {
                        worst = abs((int)got[c] - (int)(want[c] + 0.5));
                    }
                }
                checked++;
                if (worst > 4) {
                    if (bad++ < 4) {
                        fprintf(stderr, "render_smoke: Im2D border %s: pixel (%u,%u) is %u %u %u, want %.0f %.0f %.0f\n", what, px, py, got[0],
                                got[1], got[2], want[0], want[1], want[2]);
                    }
                }
            }
        }
        if (bad != 0) {
            fprintf(stderr, "render_smoke: Im2D border %s: %u of %u probes differ\n", what, bad, checked);
            fail(what);
        }
    }
    func_003f5070(P4GL_RS_TEXTUREADDRESSU, 0);
    func_003f5070(P4GL_RS_TEXTUREADDRESSV, 0);
    func_003f5070(P4GL_RS_TEXTURERASTER, 0);
    free(rgba);
}

static void border_frame(P4RwRaster *tex, const P4TmxImage *img)
{
    static const struct {
        int u, v, filter;
        int emulationOnly;
        const char *what;
    } cases[] = {
        {SMOKE_ADDRESS_BORDER, SMOKE_ADDRESS_BORDER, SMOKE_FILTER_NEAREST, 0, "nearest, border/border"},
        {SMOKE_ADDRESS_BORDER, SMOKE_ADDRESS_BORDER, 2, 0, "linear, border/border"},
        {SMOKE_ADDRESS_BORDER, SMOKE_ADDRESS_CLAMP, SMOKE_FILTER_NEAREST, 0, "nearest, border/clamp"},
        {SMOKE_ADDRESS_BORDER, SMOKE_ADDRESS_CLAMP, 2, 0, "linear, border/clamp"},
        {1, SMOKE_ADDRESS_BORDER, 2, 0, "linear, wrap/border"},
        {SMOKE_ADDRESS_MIRROR, SMOKE_ADDRESS_BORDER, SMOKE_FILTER_NEAREST, 0, "nearest, mirror/border"},
        /* Border on one axis with a mirrored other axis under linear filtering: llvmpipe in Mesa 25.0.7 (the check hosts; 26.2.4 does not) returns
         * the border for pixels the sampler model (NVIDIA's driver, ANGLE and the shader emulation agree with it) reads as texels, so this case
         * checks the emulation only. No scene is known to combine mirror with border. */
        {SMOKE_ADDRESS_BORDER, SMOKE_ADDRESS_MIRROR, 2, 1, "linear, border/mirror"},
    };

    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (cases[i].emulationOnly && p4gl_gl_border_native()) {
            continue;
        }
        border_case(tex, img, cases[i].u, cases[i].v, cases[i].filter, cases[i].what);
    }
}

static unsigned sUndrawnUploads;
static void (P4GL_APIENTRY *sUploadBuffer)(GLenum, GLsizeiptr, const void *, GLenum);

static void P4GL_APIENTRY count_undrawn_upload(GLenum target, GLsizeiptr size, const void *data, GLenum usage)
{
    sUndrawnUploads++;
    sUploadBuffer(target, size, data, usage);
}

static void undrawn_quad(P4Sky2Vertex vertices[4])
{
    P4Sky2Vertex before[4];
    int observed;

    memcpy(before, vertices, sizeof before);
    sUndrawnUploads = 0;
    sUploadBuffer = p4gl_glBufferData;
    p4gl_glBufferData = count_undrawn_upload;
    p4gl_gl_set_draw(0);
    observed = p4gl_gl_prepare_draw();
    if (func_0040c0f0(SMOKE_PRIM_TRISTRIP, (uint8_t *)vertices, 4) != 1) {
        fail("undrawn Im2D quad");
    }
    p4gl_gl_set_draw(1);
    p4gl_glBufferData = sUploadBuffer;
    if ((!observed && sUndrawnUploads != 0) || (observed && sUndrawnUploads == 0)) {
        fail("undrawn Im2D preparation/observer policy");
    }
    if (memcmp(before, vertices, sizeof before) != 0) {
        fail("undrawn Im2D changed guest vertices");
    }
}

/* Binds tex as TEXTURERASTER (nearest filter), draws a full-frame white-vertex Im2D quad with
 * UVs 0..1 into the readback FBO, reads it back, checks two texel-matched pixels and writes the
 * PNG. */
static void draw_frame(P4RwRaster *tex, const P4TmxImage *img, const char *png)
{
    P4Sky2Vertex v[4];
    uint8_t *rgba = malloc((size_t)FRAME * FRAME * 4u);
    const float corners[4][2] = {{0.0f, 0.0f}, {(float)FRAME, 0.0f},
                                 {0.0f, (float)FRAME}, {(float)FRAME, (float)FRAME}};
    const float uvs[4][2] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}};
    const unsigned probes[2][2] = {{128u, 128u}, {8u, 8u}};

    if (rgba == NULL) {
        fail("frame buffer allocation");
        return;
    }
    if (func_003f5070(P4GL_RS_TEXTURERASTER, P4_H2G(tex)) != 1 ||
        func_003f5070(P4GL_RS_TEXTUREFILTER, SMOKE_FILTER_NEAREST) != 1) {
        fail("render state");
    }
    if (p4gl_readback_target_create(FRAME, FRAME, FRAME, FRAME) != 0) {
        fail("readback target");
        free(rgba);
        return;
    }
    p4gl_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); /* Im2D tests Z, as the GS does */

    memset(v, 0, sizeof v);
    for (int i = 0; i < 4; i++) {
        v[i].x = corners[i][0];
        v[i].y = corners[i][1];
        v[i].camZ = 1.0f;
        v[i].u = uvs[i][0];
        v[i].v = uvs[i][1];
        v[i].recipZ = 1.0f;
        v[i].r = 255.0f;
        v[i].g = 255.0f;
        v[i].b = 255.0f;
        v[i].a = 255.0f;
    }
    undrawn_quad(v);
    if (func_0040c0f0(SMOKE_PRIM_TRISTRIP, (uint8_t *)v, 4) != 1) {
        fail("Im2D quad");
    }

    if (p4gl_readback_rgba(FRAME, FRAME, rgba) != 0) {
        fail("readback");
    } else {
        for (int p = 0; p < 2; p++) {
            const unsigned px = probes[p][0];
            const unsigned py = probes[p][1];
            const uint8_t *got = rgba + ((size_t)py * FRAME + px) * 4u;
            const uint8_t *texel = texel_at(img, px, py);

            if (!pixel_matches(got, texel)) {
                fprintf(stderr, "render_smoke: pixel (%u,%u) is %u %u %u, texel %u %u %u %u\n",
                        px, py, got[0], got[1], got[2], texel[0], texel[1], texel[2], texel[3]);
                fail("textured pixel does not match the TMX texel");
            }
        }
        if (p4png_write_rgba(png, FRAME, FRAME, rgba) != 0) {
            fail("PNG write");
        }
    }
    func_003f5070(P4GL_RS_TEXTURERASTER, 0);
    border_frame(tex, img);
    frame_raster_frame();
    ptank_frame(tex, img);
    im3d_frame(tex, img, png);
    affine_colour_frame();
    im2d_fractional_frame();
    im2d_line_frame();
    free(rgba);
}

/* Exercise the actual wide target and Im2D shader, as well as the camera seam.
 * A point outside the old horizontal clip range must enter the wider range;
 * nested texture cameras and the original view-window bytes must survive. */
static void widescreen_frame(void)
{
    uint8_t *camera = p4t_galloc(0x190, 16);
    uint8_t *shadow = p4t_galloc(0x190, 16);
    P4RwRaster *shadowRaster = p4t_galloc(sizeof *shadowRaster, 16);
    uint8_t *rgba = malloc((size_t)1120 * 448 * 4);
    float projection[6], originalProjection[6], retail[3] = {1.0f, 0.75f, 1.0f};
    int originalProjectionValid;

    if (rgba == NULL) {
        fail("widescreen readback allocation");
        return;
    }
    memset(camera, 0, 0x190);
    put_u32(camera, 16, 0x00400590u); /* object.sync, not projectionType */
    put_u32(camera, 20, 1);
    put_f32(camera, 104, retail[0]);
    put_f32(camera, 108, retail[1]);
    put_f32(camera, 112, retail[2]);
    put_f32(camera, 128, 1.0f);
    put_f32(camera, 132, 1000.0f);
    memcpy(shadow, camera, 0x190);
    memset(shadowRaster, 0, sizeof *shadowRaster);
    shadowRaster->cType = 5;
    put_u32(shadow, 96, P4_H2G(shadowRaster));
    if (p4gl_camera_retail_view_x(camera) != retail[0])
        fail("retail layout window outside a camera update");
    originalProjectionValid = p4gl_camera_projection(camera, originalProjection);
    p4gl_aspect_set(4.0f / 3.0f);
    p4gl_aspect_camera_begin(camera);
    if (!originalProjectionValid || !p4gl_camera_projection(camera, projection) ||
        projection[0] * -1.2f >= 1.0f || projection[0] * -1.4f <= 1.0f ||
        projection[1] != originalProjection[1]) {
        fail("Hor+ horizontal boundary / unchanged vertical projection");
    }
    if (p4gl_camera_retail_view_x(camera) != retail[0])
        fail("panel pixel aspect excludes Hor+ projection width");
    {
        float fitted[6], model[4][4], mvp[16];
        p4gl_layout_queue_begin(0x00795050u);
        if (!p4gl_camera_projection(camera, fitted) ||
            !p4gl_camera_object_matrix(NULL, camera, model, mvp) ||
            fabsf(fitted[0] / projection[0] - 0.75f) > 0.00001f ||
            fabsf(fitted[1] / projection[1] - 0.75f) > 0.00001f ||
            fabsf(mvp[0] - fitted[0]) > 0.00001f || fabsf(mvp[5] - fitted[1]) > 0.00001f)
            fail("screen-model cut-ins keep their 4:3 projection canvas");
        p4gl_layout_end();
        p4gl_camera_projection(camera, fitted);
        if (memcmp(fitted, projection, sizeof fitted) != 0) fail("screen-model scope restores world projection");
    }
    p4gl_aspect_camera_begin(shadow);
    if (memcmp(shadow + 104, retail, sizeof retail) != 0) fail("shadow camera must not widen");
    if (p4gl_camera_retail_view_x(camera) != retail[0] ||
        p4gl_camera_retail_view_x(shadow) != retail[0])
        fail("nested camera layout window ownership");
    p4gl_aspect_camera_end(shadow);
    p4gl_aspect_camera_end(camera);
    if (memcmp(camera + 104, retail, sizeof retail) != 0) fail("exact camera window restoration");
    if (p4gl_camera_retail_view_x(camera) != retail[0])
        fail("retail layout window after camera restoration");
    if (p4gl_readback_configure(1, 854, 448) != 0) {
        fail("widescreen target");
    } else {
        func_003f5070(P4GL_RS_TEXTURERASTER, 0);
        func_003f5070(P4GL_RS_ZTESTENABLE, 0);
        func_003f5070(P4GL_RS_ZWRITEENABLE, 0);
        RpSkyRenderStateSet(3, 0);
        RpSkyRenderStateSet(2, 0x44);
        p4gl_glClearColor(0, 0, 0, 1);
        p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        {
            p4gl_layout_begin(0, 21);
            im2d_rect(0, 0, 640, 60, 1, 0, 0, 0);
            p4gl_layout_end();
            p4gl_layout_queue_begin(0x00795050u);
            im2d_rect(0, 0, 100, 60, 1, 1, 1, 0);
            p4gl_layout_end();
        }
        im2d_rect(0, 100, 100, 200, 1, 0, 0, 0);
        im2d_rect(-100, 270, 0, 280, 1, 0, 1, 0);
        im2d_rect(640, 270, 740, 280, 1, 0, 1, 0);
        p4gl_layout_begin(P4_H2G(calendarDrawBackdrop), 0);
        im2d_rect(540, 100, 640, 200, 0, 1, 0, 0);
        p4gl_layout_end();
        p4gl_layout_queue_begin(0x00795050u); /* screen-model scopes cannot erase a preceding full-width background */
        p4gl_layout_end();
        p4gl_layout_queue_begin(0x00794c60u);
        im2d_rect(0, 220, 100, 260, 1, 1, 0, 0);
        p4gl_layout_end();
        p4gl_layout_queue_begin(0x00794930u);
        im2d_rect(0, 330, 100, 360, 0, 1, 1, 0);
        p4gl_layout_end();
        p4gl_layout_begin(0x001221a0u, 0);
        im2d_rect(0, 315, 100, 325, 1, 0, 0, 0);
        p4gl_layout_end();
        p4gl_layout_queue_begin(0x007943c0u);
        im2d_rect(0, 370, 80, 380, 0, 0, 1, 0);
        p4gl_layout_end();
        p4gl_layout_queue_begin(0x00794960u);
        im2d_rect(0, 400, 80, 415, 0, 1, 0, 0);
        p4gl_layout_begin(0x00363540u, 0);
        im2d_rect(570, 370, 630, 380, 1, 1, 1, 0);
        p4gl_layout_end();
        p4gl_layout_end();
        p4gl_layout_begin(0x002aa450u, 0);
        im2d_rect(570, 390, 630, 430, 1, 0, 1, 0);
        p4gl_layout_end();
        {
            P4Sky2Vertex line[2] = {{0}};
            for (int i = 0; i < 2; i++) {
                line[i].x = 240.0f + 160.0f * i;
                line[i].y = 300.0f;
                line[i].recipZ = 1.0f;
                line[i].b = line[i].a = 255.0f;
            }
            func_003f5070(P4GL_RS_VERTEXALPHAENABLE, 0);
            func_0040afb0((uint8_t *)line, 2, 0, 1);
        }
        if (p4gl_readback_rgba(854, 448, rgba) != 0) {
            fail("widescreen pixel readback");
        } else {
            const uint8_t *bar = rgba + ((size_t)150 * 854 + 20) * 4;
            const uint8_t *center = rgba + ((size_t)150 * 854 + 130) * 4;
            const uint8_t *right = rgba + ((size_t)150 * 854 + 820) * 4;
            const uint8_t *line = rgba + ((size_t)300 * 854 + 427) * 4;
            const uint8_t *left = rgba + ((size_t)240 * 854 + 20) * 4;
            const uint8_t *fogLeft = rgba + ((size_t)345 * 854 + 20) * 4;
            const uint8_t *fogRight = rgba + ((size_t)345 * 854 + 120) * 4;
            const uint8_t *loading = rgba + ((size_t)410 * 854 + 820) * 4;
            const uint8_t *model = rgba + ((size_t)70 * 854 + 130) * 4;
            const uint8_t *modelTop = rgba + ((size_t)20 * 854 + 130) * 4;
            const uint8_t *modelBar = rgba + ((size_t)20 * 854 + 20) * 4;
            const uint8_t *clipLeft = rgba + ((size_t)275 * 854 + 20) * 4;
            const uint8_t *clipRight = rgba + ((size_t)275 * 854 + 820) * 4;
            const uint8_t *filter3 = rgba + ((size_t)375 * 854 + 20) * 4;
            const uint8_t *filter = rgba + ((size_t)405 * 854 + 20) * 4;
            const uint8_t *party = rgba + ((size_t)375 * 854 + 820) * 4;
            const uint8_t *fade = rgba + ((size_t)320 * 854 + 20) * 4;
            if (bar[0] != 0 || center[0] != 255 || center[1] != 0 || right[1] != 255) {
                fail("centred unscaled UI / right-edge anchoring");
            }
            if (line[2] != 253) fail("wide AA1 line uses full GS coverage (127/128) in the UI canvas");
            if (left[0] != 255 || left[1] != 255) fail("field HUD queue stays on the left edge");
            if (fogLeft[1] != 255 || fogLeft[2] != 255 || fogRight[1] != 255 || fogRight[2] != 255)
                fail("split fog/noise quads cover the full-width canvas");
            if (loading[0] != 255 || loading[2] != 255) fail("loading corner stays on the right edge");
            if (model[0] != 255 || model[1] != 255 || modelTop[0] != 255 || modelTop[1] != 0 ||
                modelBar[0] != 255 || modelBar[1] != 0 || modelBar[2] != 0)
                fail("screen-model foreground fit preserves the full-width background without side mattes");
            if (clipLeft[0] != 0 || clipLeft[2] != 0 || clipRight[0] != 0 || clipRight[2] != 0)
                fail("centred UI overscan cannot leak into the wide world");
            if (filter3[2] != 255 || filter[1] != 255 || party[0] != 255 || party[1] != 255)
                fail("both colour-filter grids fill while the shared party producer stays edge-anchored");
            if (fade[0] != 255 || fade[1] != 0 || fade[2] != 0)
                fail("segmented full-screen fades cover the extra horizontal extent");
            if (p4png_write_rgba("render_smoke_widescreen.png", 854, 448, rgba) != 0) fail("widescreen PNG");
        }
    }
    {
        static const unsigned widths[] = {640, 853, 1120};
        static const float ratios[] = {1.0f, 4.0f / 3.0f, 1.75f};
        for (unsigned aspect = 0; aspect < 3; aspect++) {
            const unsigned width = widths[aspect];
            p4gl_aspect_set(ratios[aspect]);
            if (p4gl_readback_configure(1, width, 448) != 0) {
                fail("solid canvas edge target");
                continue;
            }
            p4gl_glClearColor(1, 1, 1, 1);
            p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            im2d_rect(0, 0, 640, 42, 0, 0, 0, 0);
            im2d_rect(0, 406, 640, 448, 0, 0, 0, 0);
            if (p4gl_readback_rgba(width, 448, rgba) != 0) {
                fail("solid canvas edge readback");
                continue;
            }
            for (unsigned x = 0; x < width; x++) {
                const uint8_t *top = rgba + ((size_t)20 * width + x) * 4;
                const uint8_t *bottom = rgba + ((size_t)427 * width + x) * 4;
                if (top[0] || top[1] || top[2] || bottom[0] || bottom[1] || bottom[2]) {
                    fail("solid title/fade bars cover every native column, including x=0");
                    break;
                }
            }
        }
    }
    p4gl_aspect_set(1.0f);
    free(rgba);
}

/* Exercise the real movie-only draw seam, including the 368-high movie.
 * Colour gradients distinguish uniform cropping from a non-uniform fit.
 * Neither path may edit the producer's vertices. */
static void movie_fit_frames(void)
{
    static const char *const modes[] = {"bars", "zoom", "stretch"};
    static const unsigned widths[] = {640, 853, 1120};
    static const float ratios[] = {1.0f, 4.0f / 3.0f, 1.75f};
    int movieRow = -1;
    for (int i = 0; i < p4_settings_entry_count(); i++) {
        char label[64], value[32];
        const int restart = p4_settings_entry(i, label, sizeof label, value, sizeof value);
        if (restart >= 0 && !strcmp(label, "Movie fit")) {
            if (movieRow >= 0 || restart != 0)
                fail("movie fit has one live registry descriptor");
            movieRow = i;
        }
    }
    if (movieRow < 0)
        fail("movie fit is registered with the settings model");
    uint8_t *rgba = malloc((size_t)1120 * 448 * 4);
    if (rgba == NULL) {
        fail("movie fit readback allocation");
        return;
    }
    p4_os_setenv("P4PC_MOVIE_FIT", "", 1);
    func_003f5070(P4GL_RS_TEXTURERASTER, 0);
    func_003f5070(P4GL_RS_ZTESTENABLE, 0);
    func_003f5070(P4GL_RS_ZWRITEENABLE, 0);
    func_003f5070(P4GL_RS_VERTEXALPHAENABLE, 0);
    func_003f5070(20, 1); /* rwCULLNONE */
    RpSkyRenderStateSet(2, 0x44);
    RpSkyRenderStateSet(3, 0);
    p4gl_frame_mask_set(0);
    p4gl_glClearColor(0, 0, 0, 1);
    for (unsigned aspect = 0; aspect < 3; aspect++) {
        p4gl_aspect_set(ratios[aspect]);
        if (p4gl_readback_configure(1, widths[aspect], 448) != 0) {
            fail("movie fit target");
            continue;
        }
        for (unsigned letterbox = 0; letterbox < 2; letterbox++) {
            P4Sky2Vertex vertices[4] = {{0}}, original[4];
            for (unsigned i = 0; i < 4; i++) {
                vertices[i].x = (i & 2) ? 640.0f : 0.0f;
                vertices[i].y = (i & 1) ? (letterbox ? 408.0f : 448.0f) : (letterbox ? 40.0f : 0.0f);
                vertices[i].r = (i & 2) ? 255.0f : 0.0f;
                vertices[i].g = vertices[i].a = 255.0f;
                vertices[i].b = (i & 1) ? 255.0f : 0.0f;
                vertices[i].recipZ = 1.0f;
            }
            memcpy(original, vertices, sizeof original);
            for (unsigned mode = 0; mode < 3; mode++) {
                char ini[64];
                snprintf(ini, sizeof ini, "[video]\nmovie_fit = %s\n", modes[mode]);
                p4cfg_clear(); /* parse_text replaces entries, not the cached typed video policy */
                p4cfg_parse_text(ini);
                if (movieRow >= 0) {
                    char label[64], value[32];
                    if (p4_settings_entry(movieRow, label, sizeof label, value, sizeof value) != 0 ||
                        strcmp(value, modes[mode]))
                        fail("movie fit registry descriptor formats the active INI choice");
                }
                p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                if (p4gl_movie_draw(SMOKE_PRIM_TRISTRIP, (uint8_t *)vertices, 4) != 1 ||
                    p4gl_readback_rgba(widths[aspect], 448, rgba) != 0) {
                    fail("movie fit draw/readback");
                    continue;
                }
                if (memcmp(vertices, original, sizeof original) != 0)
                    fail("movie fit cannot mutate guest vertices");
                const uint8_t *left = rgba + ((size_t)224 * widths[aspect] + 8) * 4;
                const uint8_t *top = rgba + ((size_t)8 * widths[aspect] + widths[aspect] / 2) * 4;
                if (mode == P4CFG_MOVIE_BARS) {
                    if (aspect != 0 && (left[0] || left[1] || left[2]))
                        fail("movie bars preserve the picture rather than stretch it");
                    if (letterbox && (top[0] || top[1] || top[2]))
                        fail("movie bars preserve the original 368-high aperture");
                } else {
                    if (left[1] != 255 || top[1] != 255)
                        fail("movie zoom/stretch must cover the complete canvas");
                    const size_t edgePixels[] = {
                        (size_t)224 * widths[aspect],
                        (size_t)225 * widths[aspect] - 1,
                        widths[aspect] / 2,
                        (size_t)447 * widths[aspect] + widths[aspect] / 2
                    };
                    for (unsigned edge = 0; edge < 4; edge++)
                        if (rgba[edgePixels[edge] * 4 + 1] != 255)
                            fail("movie zoom/stretch fills the first and last native cells");
                    if (!letterbox && aspect != 0 &&
                        ((mode == P4CFG_MOVIE_ZOOM && top[2] < 25) ||
                         (mode == P4CFG_MOVIE_STRETCH && top[2] > 10)))
                        fail("wide movie zoom crops vertically while stretch retains the full image");
                    if (letterbox && aspect == 0 &&
                        ((mode == P4CFG_MOVIE_ZOOM && left[0] < 20) ||
                         (mode == P4CFG_MOVIE_STRETCH && left[0] > 10)))
                        fail("short movie zoom crops centrally without a horizontal squeeze");
                }
            }
        }
    }
    p4cfg_clear();
    p4cfg_parse_text("");
    p4gl_aspect_set(1.0f);
    free(rgba);
}

static void cell_quad(P4Sky2Vertex out[4], float l, float t, float r, float b,
                      float red, float green, float blue)
{
    memset(out, 0, 4 * sizeof *out);
    for (unsigned i = 0; i < 4; i++) {
        out[i].x = i & 1 ? r : l;
        out[i].y = i & 2 ? b : t;
        out[i].recipZ = 1;
        out[i].r = red * 255; out[i].g = green * 255; out[i].b = blue * 255;
        out[i].a = 255;
    }
}

static void logical_cell_scene(P4RwRaster *texture)
{
    P4Sky2Vertex mixed[18] = {{0}}, quad[4];
    static const unsigned corners[6] = {0, 1, 2, 1, 3, 2};
    static const uint16_t fan[4] = {0, 1, 3, 2};
    func_003f5070(P4GL_RS_TEXTURERASTER, 0);
    func_003f5070(P4GL_RS_ZTESTENABLE, 0);
    func_003f5070(P4GL_RS_ZWRITEENABLE, 0);
    func_003f5070(P4GL_RS_VERTEXALPHAENABLE, 0);
    RpSkyRenderStateSet(2, 0x44);
    RpSkyRenderStateSet(3, 0);
    p4gl_frame_mask_set(0);
    p4gl_glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    p4gl_glClearColor(0, 0, 1, 1);
    p4gl_glClear(GL_COLOR_BUFFER_BIT);
    /* The title's top letterbox must cover row0/col0, not leak the 3D below. */
    im2d_rect(0, 0, 640, 42, 0, 0, 0, 0);
    im2d_rect(0, 64, 10.0625f, 96.5f, 1, 0, 0, 0);
    im2d_rect(10.0625f, 64, 20.75f, 96.5f, 0, 1, 0, 0);
    /* Mixed runs also exercise both VAOs, the indexed transport, and a nonzero
     * instance offset. Nonrectangular triangles are entirely off screen. */
    for (unsigned i = 0; i < 18; i++) {
        mixed[i].x = -30 - (float)i;
        mixed[i].y = -40 - (float)(i % 2);
        mixed[i].r = mixed[i].a = 255;
        mixed[i].recipZ = 1;
    }
    cell_quad(quad, 40, 120, 80, 150, 0, 1, 1);
    for (unsigned i = 0; i < 6; i++) mixed[3 + i] = quad[corners[i]];
    cell_quad(quad, 80, 120, 120, 150, 1, 1, 0);
    for (unsigned i = 0; i < 6; i++) mixed[12 + i] = quad[corners[i]];
    if (!func_0040c0f0(3, (uint8_t *)mixed, 18)) fail("mixed logical-cell list");
    cell_quad(quad, 40.25f, 180.0625f, 90.875f, 220.5f, 0, 1, 0);
    if (!func_0040d0a0(5, (uint8_t *)quad, 4, (uint16_t *)fan, 4))
        fail("logical-cell indexed fan");
    func_003f5070(P4GL_RS_TEXTURERASTER, P4_H2G(texture));
    func_003f5070(P4GL_RS_TEXTUREFILTER, SMOKE_FILTER_NEAREST);
    im2d_rect(100.0625f, 100.125f, 108.6875f, 108.625f, 1, 1, 1, 1);
    func_003f5070(P4GL_RS_TEXTURERASTER, 0);
}

/* Fine clip-space geometry must survive a feedback copy without a virtual-half
 * phase shift; the smooth gradient alone cannot distinguish a one-pixel copy. */
static int logical_cell_detail_scene(unsigned w, unsigned h)
{
    const P4ClipVertex gradient[3] = {
        {{-1,-1,0,1},{1,0,0,1},{0,0}},
        {{ 3,-1,0,1},{0,1,0,1},{0,0}},
        {{-1, 3,0,1},{0,0,1,1},{0,0}}
    };
    const uint32_t triangle[3] = {0,1,2}, quad[6] = {0,1,2,2,1,3};
    const P4ClipDrawParams params = {.color={1,1,1,1}, .own=1};
    if (!p4gl_clip_draw_triangles(gradient, 3, triangle, 3, &params)) return 0;
    for (unsigned axis = 0; axis < 2; axis++) for (unsigned i = 0; i < 8; i++) {
        const float l = axis ? -1.0f : 2.0f * (w / 2 + i) / w - 1.0f;
        const float r = axis ?  1.0f : 2.0f * (w / 2 + i + 1) / w - 1.0f;
        const float t = axis ? 1.0f - 2.0f * (h / 2 + i) / h : 1.0f;
        const float b = axis ? 1.0f - 2.0f * (h / 2 + i + 1) / h : -1.0f;
        const float red = axis ? 0.0f : (float)(i & 1);
        const float green = axis ? (float)(i & 1) : 0.0f;
        const P4ClipVertex stripe[4] = {
            {{l,t,0,1},{red,green,0,1},{0,0}}, {{r,t,0,1},{red,green,0,1},{0,0}},
            {{l,b,0,1},{red,green,0,1},{0,0}}, {{r,b,0,1},{red,green,0,1},{0,0}}
        };
        if (!p4gl_clip_draw_triangles(stripe, 4, quad, 6, &params)) return 0;
    }
    return 1;
}

static void logical_cell_frame(void)
{
    static const unsigned sizes[2][2] = {{1366, 768}, {1277, 954}};
    const size_t maximum = (size_t)1277 * 954 * 4;
    uint8_t texels[8 * 8 * 4];
    uint8_t *reference = malloc((size_t)853 * 448 * 4);
    uint8_t *actual = malloc(maximum), *copy = malloc(maximum);
    P4RwRaster *texture = p4t_galloc(sizeof *texture, 16);
    P4TmxImage image = {8, 8, texels};
    int uploaded = 0;
    const float savedAspect = p4gl_aspect_width();
    if (!reference || !actual || !copy || !texture) { fail("logical-cell fixture allocation"); goto done; }
    for (unsigned y = 0; y < 8; y++) for (unsigned x = 0; x < 8; x++) {
        uint8_t *p = texels + (y * 8 + x) * 4;
        p[0] = x & 1 ? 255 : 0; p[1] = y & 1 ? 255 : 0;
        p[2] = (x + y) & 1 ? 255 : 0; p[3] = 255;
    }
    if (!(uploaded = raster_upload(texture, &image))) { fail("logical-cell texture"); goto done; }
    for (unsigned frame = 0; frame < 4; frame++) {
        const unsigned profile = frame / 2, size = frame % 2;
        if (!size) p4gl_aspect_set(profile ? 4.0f / 3.0f : 1.0f);
        const unsigned nativeWidth = p4gl_aspect_native_width();
        if (!size) {
            expect_int((int)nativeWidth, profile ? 853 : 640, "aspect-native cell grid");
            if (p4gl_readback_configure(1, nativeWidth, 448)) { fail("logical-cell native target"); goto done; }
            logical_cell_scene(texture);
            if (p4gl_readback_rgba(nativeWidth, 448, reference)) { fail("logical-cell native oracle"); goto done; }
        }
        const unsigned w = sizes[size][0], h = sizes[size][1];
        int mismatch = 0;
        if (p4gl_readback_configure(1, w, h)) { fail("logical-cell scaled target"); continue; }
        logical_cell_scene(texture);
        if (p4gl_readback_rgba(w, h, actual)) { fail("logical-cell scaled readback"); continue; }
        for (unsigned y = 0; y < h && !mismatch; y++) for (unsigned x = 0; x < w; x++) {
            const unsigned sx = (unsigned)(((uint64_t)x * 2 + 1) * nativeWidth / ((uint64_t)w * 2));
            const unsigned sy = (unsigned)(((uint64_t)y * 2 + 1) * 448 / ((uint64_t)h * 2));
            if (memcmp(actual + ((size_t)y * w + x) * 4, reference + ((size_t)sy * nativeWidth + sx) * 4, 4)) {
                fprintf(stderr, "cell mismatch %ux%u from%ux448 pixel%u,%u, GS%u,%u\n", w, h, nativeWidth, x, y, sx, sy);
                mismatch = 1;
                break;
            }
        }
        if (mismatch) fail("Im2D coverage/texels must fill the native GS logical cells");
        /* Both profiles have complete coverage/texel oracles; copy-phase detail
         * is exercised at both physical sizes in the default profile below. */
        if (profile) continue;
        {
            P4Sky2Vertex corners[4], legacy[6];
            const unsigned order[6] = {0,1,2,2,1,3};
            cell_quad(corners, 0, 0, 640, 448, 1, 1, 1);
            for (unsigned i = 0; i < 4; i++) {
                corners[i].u = i & 1 ? 1 : 0;
                corners[i].v = i & 2 ? 1 : 0;
            }
            for (unsigned i = 0; i < 6; i++) legacy[i] = corners[order[i]];
            /* This genuine discontinuous-colour mesh uses the continuous path.
             * Its sub-byte colour delta cannot change RGB by more than one. */
            legacy[4].r -= 1.0f / 4096.0f;
            if (!logical_cell_detail_scene(w, h)) { fail("continuous 3D cell fixture"); continue; }
            func_003f5070(P4GL_RS_TEXTURERASTER, P4_G32(0x007649F4u));
            if (func_0040c0f0(IM3D_PRIM_TRILIST, (uint8_t *)legacy, 6) != 1 ||
                p4gl_readback_rgba(w, h, copy)) {
                func_003f5070(P4GL_RS_TEXTURERASTER, 0);
                fail("continuous feedback oracle"); continue;
            }
            func_003f5070(P4GL_RS_TEXTURERASTER, 0);
            if (!logical_cell_detail_scene(w, h)) { fail("continuous 3D cell redraw"); continue; }
            func_003f5070(P4GL_RS_TEXTURERASTER, P4_G32(0x007649F4u));
            im2d_rect(0, 0, 640, 448, 1, 1, 1, 1);
            func_003f5070(P4GL_RS_TEXTURERASTER, 0);
            if (p4gl_readback_rgba(w, h, actual)) { fail("scaled frame feedback readback"); continue; }
            /* Coverage at the outer logical cell intentionally changes; compare
             * interpolation in the interior against the original continuous path. */
            const unsigned mx = (w + 639) / 640, my = (h + 447) / 448;
            int shifted = 0;
            for (unsigned y = my; y + my < h && !shifted; y++) {
                for (unsigned x = mx; x + mx < w && !shifted; x++) {
                    const size_t p = ((size_t)y * w + x) * 4;
                    for (unsigned channel = 0; channel < 4; channel++) {
                        if (abs((int)actual[p + channel] - (int)copy[p + channel]) > 1) {
                            shifted = 1;
                            break;
                        }
                    }
                }
            }
            if (shifted) fail("cell feedback must retain the original continuous UV phase");
            int detail = 0, copiedDetail = 0;
            const unsigned y = h / 4;
            for (unsigned x = 1; x < w; x++) {
                const uint64_t a = ((uint64_t)x * 2 - 1) * 640 / ((uint64_t)w * 2);
                const uint64_t b = ((uint64_t)x * 2 + 1) * 640 / ((uint64_t)w * 2);
                const size_t p = ((size_t)y * w + x) * 4;
                if (a == b) {
                    detail |= memcmp(actual + p - 4, actual + p, 3) != 0;
                    copiedDetail |= memcmp(copy + p - 4, copy + p, 3) != 0;
                }
            }
            if (!detail || !copiedDetail) fail("3D and its feedback must retain sub-cell detail");
        }
    }
done:
    p4gl_aspect_set(savedAspect);
    func_003f5070(P4GL_RS_TEXTURERASTER, 0);
    if (uploaded) func_003fb830(NULL, texture, 0);
    free(reference); free(actual); free(copy);
}

static void resolution_frame(const char *png)
{
    P4RwRaster *rasters[2];
    uint8_t *cameras[2];
    P4RwRaster *oversizedRaster = NULL;
    const int virtualSizes[2][2] = {{128, 128}, {64, 32}};
    const int pixelSizes[2][2] = {{273, 219}, {137, 55}};
    int w = 0, h = 0;
    uint8_t targetPixel[4];
    GLint viewport[4];
    if (p4gl_readback_configure(1, 1366, 768) != 0) {
        fail("fractional main target");
        return;
    }
    p4gl_glClearColor(0, 0, 1, 1);
    p4gl_glClear(GL_COLOR_BUFFER_BIT);
    for (unsigned i = 0; i < 2; i++) {
        rasters[i] = p4t_galloc(sizeof *rasters[i], 16);
        cameras[i] = p4t_galloc(0x160, 16);
        if (rasters[i] == NULL || cameras[i] == NULL) { fail("fractional camera fixture"); return; }
        memset(rasters[i], 0, sizeof *rasters[i]);
        memset(cameras[i], 0, 0x160);
        rasters[i]->parent = P4_H2G(rasters[i]);
        rasters[i]->cType = 5;
        rasters[i]->width = virtualSizes[i][0];
        rasters[i]->height = virtualSizes[i][1];
        put_u32(cameras[i], 96, P4_H2G(rasters[i]));
        if (!func_003f6a10(NULL, cameras[i], 0)) fail("fractional camera begin");
        if (!p4gl_target_size(&w, &h) || w != virtualSizes[i][0] || h != virtualSizes[i][1])
            fail("camera virtual dimensions changed with resolution");
        if (!p4gl_target_pixel_size(&w, &h) || w != pixelSizes[i][0] || h != pixelSizes[i][1])
            fail("camera physical density");
        p4gl_glGetIntegerv(0x0BA2u /* GL_VIEWPORT */, viewport);
        if (viewport[2] != w || viewport[3] != h) fail("camera physical viewport");
        p4gl_glClearColor(i == 0 ? 1 : 0, i == 1 ? 1 : 0, 0, 1);
        p4gl_glClear(GL_COLOR_BUFFER_BIT);
        p4gl_glReadPixels(w - 1, h - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, targetPixel);
        if (targetPixel[i] != 255 || targetPixel[1u - i] != 0 || targetPixel[2] != 0)
            fail("camera physical storage / independent colour target");
        if (i == 0) {
            uint8_t *oversizedCamera = p4t_galloc(0x160, 16);
            GLint textureLimit = 0;
            oversizedRaster = p4t_galloc(sizeof *oversizedRaster, 16);
            if (oversizedCamera == NULL || oversizedRaster == NULL) {
                fail("camera allocation-failure fixture"); return;
            }
            memset(oversizedCamera, 0, 0x160);
            memset(oversizedRaster, 0, sizeof *oversizedRaster);
            oversizedRaster->parent = P4_H2G(oversizedRaster);
            oversizedRaster->cType = 5;
            oversizedRaster->height = 1;
            put_u32(oversizedCamera, 96, P4_H2G(oversizedRaster));
            p4gl_glGetIntegerv(0x0D33u /* GL_MAX_TEXTURE_SIZE */, &textureLimit);
            for (unsigned attempt = 0; attempt < 2; attempt++) {
                /* At 1366/640 density the second case is 4294967297 pixels:
                 * narrowing first would wrongly accept a one-pixel target. */
                oversizedRaster->width = attempt == 0 ? textureLimit :
                    (int32_t)((UINT64_C(1) << 32) * 640u / 1366u + 1u);
                expect_int(func_003f6a10(NULL, oversizedCamera, 0), 0, "oversized camera begin fails");
                if (!p4gl_target_pixel_size(&w, &h) || w != pixelSizes[0][0] || h != pixelSizes[0][1])
                    fail("failed camera begin changed the current physical target");
                p4gl_glReadPixels(w - 1, h - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, targetPixel);
                if (targetPixel[0] != 255 || targetPixel[1] != 0 || targetPixel[2] != 0)
                    fail("failed camera begin changed the enclosing framebuffer");
            }
        }
    }
    func_003f73d0(NULL, cameras[1], 0);
    if (!p4gl_target_pixel_size(&w, &h) || w != 273 || h != 219) fail("nested camera physical restore");
    if (!p4gl_target_size(&w, &h) || w != 128 || h != 128) fail("nested camera virtual restore");
    p4gl_glGetIntegerv(0x0BA2u /* GL_VIEWPORT */, viewport);
    if (viewport[2] != pixelSizes[0][0] || viewport[3] != pixelSizes[0][1])
        fail("nested camera viewport restore");
    p4gl_glReadPixels(pixelSizes[0][0] - 1, pixelSizes[0][1] - 1, 1, 1,
                     GL_RGBA, GL_UNSIGNED_BYTE, targetPixel);
    if (targetPixel[0] != 255 || targetPixel[1] != 0 || targetPixel[2] != 0)
        fail("nested camera read/draw framebuffer restore");
    func_003f73d0(NULL, cameras[0], 0);
    if (!p4gl_target_pixel_size(&w, &h) || w != 1366 || h != 768) fail("main physical restore");
    if (!p4gl_target_size(&w, &h) || w != 640 || h != 448) fail("main virtual restore");
    p4gl_glGetIntegerv(0x0BA2u /* GL_VIEWPORT */, viewport);
    if (viewport[2] != 1366 || viewport[3] != 768) fail("main viewport restore");
    p4gl_glReadPixels(1365, 767, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, targetPixel);
    if (targetPixel[0] != 0 || targetPixel[1] != 0 || targetPixel[2] != 255)
        fail("main read/draw framebuffer restore");
    for (unsigned i = 0; i < 2; i++) func_003fb830(NULL, rasters[i], 0);
    if (oversizedRaster != NULL) func_003fb830(NULL, oversizedRaster, 0);
    {
        /* A nearest, one-texel opaque stem must survive fractional scaling
         * with GS GEQUAL128. Bilinear alpha would reject the entire stem. */
        uint8_t glyph[8 * 8 * 4] = {0};
        uint8_t probe[30 * 25 * 4];
        P4TmxImage image = {8, 8, glyph};
        P4RwRaster *texture = p4t_galloc(sizeof *texture, 16);
        for (unsigned y = 0; y < 8; y++) {
            uint8_t *pixel = glyph + (y * 8 + 3) * 4;
            pixel[0] = pixel[1] = pixel[2] = pixel[3] = 255;
        }
        if (texture == NULL || !raster_upload(texture, &image)) {
            fail("fractional glyph fixture");
        } else {
            p4gl_glClearColor(0, 0, 0, 1);
            p4gl_glClear(GL_COLOR_BUFFER_BIT);
            func_003f5070(P4GL_RS_TEXTURERASTER, P4_H2G(texture));
            func_003f5070(P4GL_RS_TEXTUREFILTER, SMOKE_FILTER_NEAREST);
            RpSkyRenderStateSet(2, 0x44);
            RpSkyRenderStateSet(3, 0);
            im2d_rect(100, 100, 108, 108, 1, 1, 1, 1);
            p4gl_glReadPixels(210, 578, 30, 25, GL_RGBA, GL_UNSIGNED_BYTE, probe);
            for (unsigned i = 0; i < 30 * 25; i++)
                if (probe[i * 4] > 0 && probe[i * 4] < 240)
                    fail("fractional nearest sampling changed opaque texels");
            func_003f5070(P4GL_RS_TEXTUREFILTER, SMOKE_FILTER_LINEAR);
            p4gl_glClear(GL_COLOR_BUFFER_BIT);
            im2d_rect(100, 100, 108, 108, 1, 1, 1, 1);
            p4gl_glReadPixels(210, 578, 30, 25, GL_RGBA, GL_UNSIGNED_BYTE, probe);
            {
                int interpolated = 0;
                for (unsigned i = 0; i < 30 * 25; i++)
                    interpolated |= probe[i * 4] > 0 && probe[i * 4] < 240;
                if (!interpolated) fail("fractional authored linear sampling");
            }
            func_003f5070(P4GL_RS_TEXTUREFILTER, SMOKE_FILTER_NEAREST);
            {
                static const uint32_t tests[] = {0x80b, 0x180b, 0x380b}; /* GEQUAL128: KEEP, FB_ONLY, RGB_ONLY */
                for (unsigned test = 0; test < sizeof tests / sizeof tests[0]; test++) {
                    p4gl_glClear(GL_COLOR_BUFFER_BIT);
                    RpSkyRenderStateSet(3, tests[test]);
                    im2d_rect(100, 100, 108, 108, 1, 1, 1, 1);
                    p4gl_glReadPixels(210, 578, 30, 25, GL_RGBA, GL_UNSIGNED_BYTE, probe);
                    for (unsigned y = 5; y < 15; y++) {
                        int lit = 0;
                        for (unsigned x = 0; x < 30; x++) lit |= probe[(y * 30 + x) * 4] > 240;
                        if (!lit) fail("fractional alpha-tested 1-texel stem dropped");
                    }
                }
            }
            RpSkyRenderStateSet(3, 0);
            func_003f5070(P4GL_RS_TEXTURERASTER, 0);
            func_003fb830(NULL, texture, 0);
        }
    }
    {
        /* GLES wide lines are optional: one GS pixel must instead cover two
         * physical rows/columns at this fractional density. Exercise indexed
         * lines, an unindexed list, a strip, and clipping through the eye. */
        uint8_t *camera = p4t_galloc(256, 16);
        Im3DVertex *line = p4t_galloc(2 * sizeof *line, 16);
        uint8_t probe[16 * 16 * 4];
        if (camera == NULL || line == NULL) { fail("fractional Im3D line fixture"); return; }
        memset(camera, 0, 256);
        put_f32(camera, 32, 1); put_f32(camera, 52, 1); put_f32(camera, 72, 1);
        put_f32(camera, 104, 1); put_f32(camera, 108, 1);
        put_f32(camera, 128, 0.5f); put_f32(camera, 132, 50);
        func_003f6a10(NULL, camera, 0);
        func_003f5070(P4GL_RS_ZTESTENABLE, 0);
        func_003f5070(P4GL_RS_ZWRITEENABLE, 0);
        func_003f5070(P4GL_RS_VERTEXALPHAENABLE, 0);
        for (unsigned variant = 0; variant < 4; variant++) {
            int covered = 0;
            memset(line, 0, 2 * sizeof *line);
            for (unsigned i = 0; i < 2; i++) {
                line[i].z = 2;
                memset(line[i].rgba, 255, sizeof line[i].rgba);
            }
            if (variant == 0) { line[0].x = -0.5f; line[1].x = 0.5f; }
            else if (variant == 3) {
                line[0].x = line[1].x = 0.4f;
                line[0].z = -1;
            } else { line[0].y = -0.5f; line[1].y = 0.5f; }
            p4gl_glClearColor(0, 0, 0, 1);
            p4gl_glClear(GL_COLOR_BUFFER_BIT);
            if (RwIm3DTransform(line, 2, NULL, IM3D_ALLOPAQUE) == NULL)
                fail("fractional line transform");
            if (variant == 0)
                expect_int(RwIm3DRenderLine(1, 0), 1, "fractional indexed Im3D line");
            else
                expect_int(RwIm3DRenderPrimitive(variant == 2 ? 2 : IM3D_PRIM_LINELIST),
                           1, "fractional Im3D line primitive");
            expect_int(RwIm3DEnd(), 1, "fractional line end");
            p4gl_glReadPixels(variant == 3 ? 350 : 676, 376, 16, 16,
                             GL_RGBA, GL_UNSIGNED_BYTE, probe);
            for (unsigned axis = 0; axis < 16; axis++) {
                int lit = 0;
                for (unsigned along = 0; along < 16; along++) {
                    const unsigned x = variant == 1 || variant == 2 ? axis : along;
                    const unsigned y = variant == 1 || variant == 2 ? along : axis;
                    lit |= probe[(y * 16 + x) * 4] > 240;
                }
                covered += lit;
            }
            expect_int(covered, 2, "fractional Im3D virtual-pixel stroke width");
        }
        func_003f73d0(NULL, camera, 0);
    }
    {
        /* Independent closed-form3x2 ->640x448 box oracle: only columns213
         * and426 straddle a source boundary; rows divide exactly at224. */
        uint8_t image[24] = {0,0,200,255, 60,0,200,255, 120,0,200,255,
                             0,100,200,255, 60,100,200,255, 120,100,200,255};
        uint8_t row[640 * 3];
        char path[1024];
        double difference = -1;
        FILE *file;
        snprintf(path, sizeof path, "%s.area.rgb", png);
        file = fopen(path, "wb");
        if (file == NULL) { fail("area reference fixture"); return; }
        for (unsigned y = 0; y < 448; y++) {
            for (unsigned x = 0; x < 640; x++) {
                row[x * 3] = x < 213 ? 0 : x == 213 ? 40 : x < 426 ? 60 : x == 426 ? 80 : 120;
                row[x * 3 + 1] = y < 224 ? 0 : 100;
                row[x * 3 + 2] = 200;
            }
            if (fwrite(row, 1, sizeof row, file) != sizeof row) fail("area reference write");
        }
        if (fclose(file) != 0) fail("area reference close");
        if (p4gl_reference_mad(image, 3, 2, path, &difference) != 0 || difference != 0)
            fail("fractional reference area oracle");
        image[0] = 30;
        if (p4gl_reference_mad(image, 3, 2, path, &difference) != 0 || difference <= 0)
            fail("scaled reference must detect changed pixels");
        remove(path);
    }
    {
        /* Exercise captureFrame itself, not just the readback helper. The
         * asymmetric GPU gradient detects a missing/double vertical flip. */
        const P4ClipVertex vertices[3] = {
            {{-1, -1, 0, 1}, {1, 0, 0, 0.25f}, {0, 0}},
            {{ 3, -1, 0, 1}, {0, 1, 0, 0.50f}, {0, 0}},
            {{-1,  3, 0, 1}, {0, 0, 1, 0.75f}, {0, 0}}
        };
        const uint32_t indices[3] = {0, 1, 2};
        const P4ClipDrawParams params = {.color = {1, 1, 1, 1}, .own = 1};
        uint8_t expected[24] = {0}, actual[18] = {0};
        char rgbPath[1024], capturePath[1024];
        FILE *file;
        p4_os_setenv("P4PC_RESOLUTION", "3x2", 1);
        p4cfg_clear();
        if (p4gl_context_apply_settings() != 0) { fail("capture target configuration"); return; }
        if (!p4gl_clip_draw_triangles(vertices, 3, indices, 3, &params))
            fail("capture gradient draw");
        if (p4gl_readback_rgba(3, 2, expected) != 0) fail("capture readback oracle");
        if (expected[2] <= expected[14]) fail("capture oracle row asymmetry");
        snprintf(rgbPath, sizeof rgbPath, "%s.capture.rgb", png);
        snprintf(capturePath, sizeof capturePath, "%s.capture.png", png);
        file = fopen(rgbPath, "wb");
        if (file == NULL) { fail("capture RGB fixture"); return; }
        fclose(file);
        p4_os_setenv("P4PC_CAPTURE_RGB", rgbPath, 1);
        p4gl_context_capture_next(capturePath);
        p4gl_context_present();
        p4_os_setenv("P4PC_CAPTURE_RGB", "", 1);
        file = fopen(rgbPath, "rb");
        if (file == NULL) fail("capture RGB output");
        else {
            if (fread(actual, 1, sizeof actual, file) != sizeof actual || fgetc(file) != EOF)
                fail("capture physical byte count");
            fclose(file);
            for (unsigned pixel = 0; pixel < 6; pixel++)
                if (memcmp(actual + pixel * 3, expected + pixel * 4, 3) != 0)
                    fail("capture RGB orientation/content");
        }
        remove(rgbPath);
        if (p4gl_readback_configure(1, 1, 1) != 0) fail("single-pixel capture target");
        p4gl_glClearColor(0, 1, 0, 1);
        p4gl_glClear(GL_COLOR_BUFFER_BIT);
        if (p4gl_readback_rgba(1, 1, expected) != 0 ||
            expected[0] != 0 || expected[1] != 255 || expected[2] != 0 || expected[3] != 255)
            fail("single-row readback must not flip or allocate a row");
    }
    if (p4gl_glGetError() != GL_NO_ERROR) fail("fractional target GL state");
}

static int test_main(int argc, char **argv)
{
    const char *iso = getenv("P4PC_ISO");
    const char *elf = getenv("P4PC_ELF");
    const char *png = argc > 1 ? argv[1] : "render_smoke.png";
    char tmxPath[512];
    P4TmxImage img = {0, 0, NULL};
    uint8_t *stream;
    size_t streamLen = 0;

    if (iso == NULL || iso[0] == '\0' || elf == NULL || elf[0] == '\0') {
        fprintf(stderr, "render_smoke: P4PC_ISO or P4PC_ELF unset, skipped\n");
        return SKIP_RETURN;
    }
    p4_os_headless_env();

    /* RenderWare allocates from the boot heap, so boot it the way p4pc_viewer does. */
    p4_os_setenv("P4PC_TRAP_CONTINUE", "1", 0);
    p4pc_trap_configure();
    p4pc_load_image(elf);
    p4pc_apply_patches();
    p4pc_kernel_pre_init();
    kwlnProbeHeapBudget();

    if (vfs_mount(iso, NULL) != 0) {
        fprintf(stderr, "render_smoke: cannot mount %s\n", iso);
        return 1;
    }
    tmxPath[0] = '\0';
    if (!find_tmx("", 0, tmxPath, sizeof tmxPath)) {
        fail("no TMX on the disc");
        return 1;
    }
    fprintf(stderr, "render_smoke: TMX %s\n", tmxPath);

    stream = read_file(tmxPath, &streamLen);
    if (stream == NULL || p4tmx_decode(stream, streamLen, &img) != 0) {
        fail("TMX decode");
        free(stream);
        return 1;
    }
    free(stream);

    if (p4gl_context_open() != 0) {
        fail("GL context");
        p4tmx_free(&img);
        return 1;
    }
    {
        P4RwRaster *tex = p4t_galloc(sizeof *tex, 16);   /* bound as the texture raster: guest storage */

        raster_round_trip(&img);
        image_conversions();
        if (raster_upload(tex, &img)) {
            draw_frame(tex, &img, png);
            widescreen_frame();
            logical_cell_frame();
            resolution_frame(png);
            movie_fit_frames();
            func_003fb830(NULL, tex, 0);
        }
    }
    {
        const unsigned w = (unsigned)img.width;
        const unsigned h = (unsigned)img.height;

        p4tmx_free(&img);
        p4gl_context_close();
        if (failures != 0) {
            return 1;
        }
        printf("render_smoke: ok, %ux%u TMX, frame %s\n", w, h, png);
    }
    return 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}
