/* OpenGL 3.3 core device layer for the RenderWare core (replaces the PS2 sky2 driver,
 * see docs/renderer_port.md). Internal to src/render; the seam is func_004010c0 in
 * src/render/p4gl_device.c. */
#ifndef P4PC_RENDER_P4GL_H
#define P4PC_RENDER_P4GL_H

#include <stddef.h>
#include <stdint.h>

/* Port-side layout of struct RwDevice (decomp/include/rw/plcore/badevice.h:241-262).
 * The retail device block lives at 0x0070C220 (D_0070C220). Offsets are checked below. The block is guest
 * memory: every function slot is a guest code address (P4_FN_G2H / P4_FN_H2G), so the struct has the same
 * layout on a 32-bit and a 64-bit host. */
typedef struct P4RwDevice
{
    float gammaCorrection;                  /* +0x00 */
    uint32_t fpSystem;                      /* +0x04 */
    float zBufferNear;                      /* +0x08 */
    float zBufferFar;                       /* +0x0c */
    uint32_t fpRenderStateSet;              /* +0x10 */
    uint32_t fpRenderStateGet;              /* +0x14 */
    uint32_t fpIm2DRenderLine;              /* +0x18 */
    uint32_t fpIm2DRenderTriangle;          /* +0x1c */
    uint32_t fpIm2DRenderPrimitive;         /* +0x20 */
    uint32_t fpIm2DRenderIndexedPrimitive;  /* +0x24 */
    uint32_t fpIm3DRenderLine;              /* +0x28 */
    uint32_t fpIm3DRenderTriangle;          /* +0x2c */
    uint32_t fpIm3DRenderPrimitive;         /* +0x30 */
    uint32_t fpIm3DRenderIndexedPrimitive;  /* +0x34 */
} P4RwDevice;

_Static_assert(sizeof(P4RwDevice) == 0x38, "RwDevice layout");
_Static_assert(offsetof(P4RwDevice, zBufferNear) == 0x08, "RwDevice layout");
_Static_assert(offsetof(P4RwDevice, fpSystem) == 0x04, "RwDevice layout");
_Static_assert(offsetof(P4RwDevice, fpRenderStateSet) == 0x10, "RwDevice layout");
_Static_assert(offsetof(P4RwDevice, fpRenderStateGet) == 0x14, "RwDevice layout");
_Static_assert(offsetof(P4RwDevice, fpIm2DRenderLine) == 0x18, "RwDevice layout");
_Static_assert(offsetof(P4RwDevice, fpIm2DRenderIndexedPrimitive) == 0x24, "RwDevice layout");
_Static_assert(offsetof(P4RwDevice, fpIm3DRenderLine) == 0x28, "RwDevice layout");
_Static_assert(offsetof(P4RwDevice, fpIm3DRenderIndexedPrimitive) == 0x34, "RwDevice layout");

/* Port-side mirror of struct RwRaster (decomp/include/rw/inc/rwcore.h:1365), 32-bit layout.
 * p4gl_raster.c owns the handlers; the render_smoke test reads the same fields. A raster is guest memory:
 * its pointers are guest addresses (the pixel and palette storage is guest memory too). */
typedef struct P4RwRaster {
    uint32_t parent;     /* P4RwRaster */
    uint32_t cpPixels;   /* uint8_t */
    uint32_t palette;    /* uint8_t */
    int32_t width, height, depth;
    int32_t stride;
    int16_t nOffsetX, nOffsetY;
    uint8_t cType;
    uint8_t cFlags;
    uint8_t privateFlags;
    uint8_t cFormat;
    uint32_t originalPixels; /* uint8_t */
    int32_t originalWidth, originalHeight, originalStride;
} P4RwRaster;

_Static_assert(sizeof(P4RwRaster) == 52, "RwRaster layout (32-bit)");
_Static_assert(offsetof(P4RwRaster, parent) == 0, "RwRaster.parent offset");
_Static_assert(offsetof(P4RwRaster, cpPixels) == 4, "RwRaster.cpPixels offset");
_Static_assert(offsetof(P4RwRaster, palette) == 8, "RwRaster.palette offset");
_Static_assert(offsetof(P4RwRaster, width) == 12, "RwRaster.width offset");
_Static_assert(offsetof(P4RwRaster, stride) == 24, "RwRaster.stride offset");
_Static_assert(offsetof(P4RwRaster, nOffsetX) == 28, "RwRaster.nOffsetX offset");
_Static_assert(offsetof(P4RwRaster, cType) == 32, "RwRaster.cType offset");
_Static_assert(offsetof(P4RwRaster, cFlags) == 33, "RwRaster.cFlags offset");
_Static_assert(offsetof(P4RwRaster, cFormat) == 35, "RwRaster.cFormat offset");
_Static_assert(offsetof(P4RwRaster, originalPixels) == 36, "RwRaster.originalPixels offset");
_Static_assert(offsetof(P4RwRaster, originalWidth) == 40, "RwRaster.originalWidth offset");

/* Port-side layout of RwSky2DVertex (decomp/include/rw/sky2/rwcore.h). Offsets are checked
 * below; the last float is the 4 bytes after objNormal that the core never reads. */
typedef struct P4Sky2Vertex
{
    float x, y, z;      /* +0x00 scrVertex */
    float camZ;         /* +0x0c camVertex_z */
    float u, v;         /* +0x10 texture coordinates (Im2D samples the bound raster) */
    float recipZ;       /* +0x18 */
    float pad1;         /* +0x1c */
    float r, g, b, a;   /* +0x20 colour */
    float nx, ny, nz;   /* +0x30 objNormal */
    float pad2;         /* +0x3c */
} P4Sky2Vertex;

_Static_assert(offsetof(P4Sky2Vertex, x) == 0x00, "RwSky2DVertex layout");
_Static_assert(offsetof(P4Sky2Vertex, camZ) == 0x0c, "RwSky2DVertex layout");
_Static_assert(offsetof(P4Sky2Vertex, u) == 0x10, "RwSky2DVertex layout");
_Static_assert(offsetof(P4Sky2Vertex, r) == 0x20, "RwSky2DVertex layout");
_Static_assert(offsetof(P4Sky2Vertex, nx) == 0x30, "RwSky2DVertex layout");
_Static_assert(sizeof(P4Sky2Vertex) == 64, "RwSky2DVertex layout");

/* Number of bytes REGISTER copies from the device block (retail 0x4011a4 copies 8 words). */
#define P4GL_REGISTER_BYTES 0x40u

/* GL context on the pad window. p4gl_context_open is idempotent; it returns 0 on success. */
int p4gl_context_open(void);
/* The RenderWare device's OPEN: p4gl_context_open, or, when that fails, the reason on stderr (and in a message box for a player's run)
 * and exit status 1. Returns only with a context. */
void p4gl_context_open_device(void);
void p4gl_context_close(void);
/* True only when this thread owns the live game's SDL/EGL context. */
int p4gl_context_is_current(void);
/* True when the device can take raster creation and draws: this thread owns the live context, or the battle simulator's
 * no-device backend is open (rasters keep their CPU shadows; uploads and draws go to its sinks). False with no device at all. */
int p4gl_device_ready(void);

/* Present the back buffer (RASTERSHOWRASTER / retail stdFunc slot 20). */
void p4gl_context_present(void);

/* Request a PNG on the next present. Written is 0 until that request writes its PNG successfully
 * (including after a failed capture); readback brightness alone does not prove a file exists. */
void p4gl_context_capture_next(const char *path);
int p4gl_context_capture_written(void);

/* Widescreen is a projection/layout choice, not a stretch of the finished frame.
 * The game keeps its 640x448 coordinate grid; frame textures retain the entire target. */
void p4gl_aspect_set(float relativeWidth);
float p4gl_aspect_width(void);
/* Nominal 1x storage/sample grid; the game's coordinate width stays 640. */
uint32_t p4gl_aspect_native_width(void);
void p4gl_aspect_camera_begin(void *camera);
void p4gl_aspect_camera_end(void *camera);
/* Original window for pixel-aspect layout calculations, not world projection. */
float p4gl_camera_retail_view_x(const void *camera);
enum { P4GL_LAYOUT_CENTER, P4GL_LAYOUT_LEFT, P4GL_LAYOUT_RIGHT, P4GL_LAYOUT_FILL, P4GL_LAYOUT_ZOOM };
void p4gl_layout_begin(uint32_t callback, unsigned command);
/* Explicit Im2D layer policy; restored by the same balanced layout_end. */
void p4gl_layout_push(int layout);
void p4gl_layout_queue_begin(uint32_t queue);
void p4gl_layout_node_begin(const void *node);
void p4gl_layout_end(void);
/* Render-only hidden HUD scopes: ordering-table buckets (queue addresses) and
 * node callback retail-entry ranges whose draws are dropped at the flush.
 * Copied; an empty set costs one test per scope. Set by the mod services. */
void p4gl_hud_hidden_set(const uint32_t *queues, unsigned queueCount, const uint32_t (*callbacks)[2],
                         unsigned callbackCount);
float p4gl_layout_model_scale(void); /* fitted screen-space EPL models, not world geometry */
enum {
    P4GL_MODEL_FIT, P4GL_MODEL_COVER,
    P4GL_MODEL_PORTRAIT_A, P4GL_MODEL_PORTRAIT_B, P4GL_MODEL_PORTRAIT_C, P4GL_MODEL_PORTRAIT_D,
    P4GL_MODEL_GLINT_A, P4GL_MODEL_GLINT_B, P4GL_MODEL_GLINT_C, P4GL_MODEL_GLINT_D,
    P4GL_MODEL_FILL_X /* solid backings and streak bands: preserve authored height */
};
int p4gl_layout_model_material(const void *material);
/* Movie-only Im2D seam: no texture replacement or mutation of guest vertices. */
int32_t p4gl_movie_draw(int32_t primitive, uint8_t *vertices, int32_t count);

/* Writes the Im2D entry points (device +0x18..+0x24) into a device block. REGISTER calls this
 * on the block it copies to the core (src/render/p4gl_im2d.c). */
void p4gl_im2d_install(P4RwDevice *device);

/* RwRenderState ids the GL draw path reads (decomp/include/rw/sky2/rwplcore.h:5162-5279). */
#define P4GL_RS_TEXTURERASTER 1
#define P4GL_RS_TEXTUREADDRESS 2
#define P4GL_RS_TEXTUREADDRESSU 3
#define P4GL_RS_TEXTUREADDRESSV 4
#define P4GL_RS_ZTESTENABLE 6
#define P4GL_RS_ZWRITEENABLE 8
#define P4GL_RS_TEXTUREFILTER 9
#define P4GL_RS_VERTEXALPHAENABLE 12
#define P4GL_RS_FOGENABLE 14
#define P4GL_RS_CULLMODE 20
#define P4GL_RS_FOGCOLOR 15

/* RenderState value by RwRenderState id (src/render/p4gl_state.c); 0 when unset. */
uint32_t p4gl_render_state(uint32_t state);
/* Scoped native draws preserve unset/default semantics as well as values. */
uint32_t p4gl_render_state_written(void);
void p4gl_render_state_restore(uint32_t state, uint32_t value, uint32_t written_mask);

/* Virtual draw dimensions: main640x448, or the active camera raster's dimensions. */
int p4gl_target_size(int *width, int *height);
/* A camera-texture target has independent virtual and physical sizes; zero clears it. */
void p4gl_target_override(int width, int height, int pixelWidth, int pixelHeight);
/* Actual pixel dimensions for viewports, texture copies and destination-alpha fetches. */
int p4gl_target_pixel_size(int *width, int *height);
/* Camera-texture render targets (p4gl_raster.c): begin binds the raster's framebuffer and returns its size, end copies the
 * result, rows flipped, into the texture the raster samples as and rebinds the main target. */
int p4gl_raster_target_begin(P4RwRaster *r, int *w, int *h);
void p4gl_raster_target_end(P4RwRaster *r);
void p4gl_raster_target_clear(P4RwRaster *r, uint32_t bits, const float rgba[4]); /* RwCameraClear of a camera-texture camera */

/* Frame rasters (src/render/p4gl_raster.c): the retail skyDrawRaster (0x007649F4, the frame so far)
 * and skyDisplayRaster (0x007649F0, the presented frame) as RwRasters whose GL texture is a copy
 * of the framebuffer. Open creates them and sets the two retail globals; the display copy is taken
 * at present. RpSkyGetDisplayBufferRaster (0x00401b80) returns the display raster. */
void p4gl_frame_rasters_open(void);
void p4gl_frame_rasters_present(void);

/* GL texture name of a raster uploaded through RASTERUNLOCK; 0 if it has none
 * (src/render/p4gl_raster.c). */
uint32_t p4gl_raster_texture(const P4RwRaster *raster);
/* 1 for the draw and display frame rasters (their texture is a copy of the framebuffer). */
int p4gl_raster_is_frame(const P4RwRaster *raster);
/* Replaces level 0 of a raster with RGBA8 pixels, for rasters made without a CPU copy (movie frames). 0 or -1. */
int p4gl_raster_set_rgba(P4RwRaster *raster, const uint8_t *rgba, int32_t width, int32_t height);

/* Native import provenance. Metadata is allocated only while content services
 * are enabled, except explicit authored resources retain their actual owner.
 * It never lives in guest RW structures. Dynamic producers must exclude a
 * raster before its first upload/copy. */
struct P4ContentSource;
/* Create the pixels of a fresh standalone DONTALLOCATE/8888 texture from real
 * authored RGBA8. Requires OVERRIDE provenance with provider/key/rank. Game/
 * current-context thread only; 0 success, -1 failure. Allocates a guest-readable
 * POT-padded shadow, clears DONTALLOCATE and retains native owner metadata.
 * RASTERDESTROY owns both; the caller must not free the returned lock's pixels.
 * Existing/movie/framebuffer rasters cannot be retagged through this API. */
int p4gl_raster_set_authored_rgba(P4RwRaster *raster, const uint8_t *rgba,
                                 int32_t width, int32_t height,
                                 const struct P4ContentSource *source);
/* Creates and initializes its own fresh RW raster; never adopts a caller's
 * raster or changes ordinary RASTERCREATE. Game/current-context thread only. */
P4RwRaster *p4gl_raster_create_authored(const uint8_t *rgba, int32_t width, int32_t height,
                                        const struct P4ContentSource *source);
/* Borrow the explicitly authored guest shadow without locking/reuploading.
 * NULL for other/dead rasters; optional stride/row outputs are then zero.
 * Allocation bounds do not depend on mutable guest dimensions. Game thread
 * only; the pointer remains owned by the raster until destroy/context close. */
const uint8_t *p4gl_raster_authored_shadow(const P4RwRaster *raster,
                                         int32_t *stride, int32_t *padded_rows);
void p4gl_raster_content_source(P4RwRaster *raster, const struct P4ContentSource *source);
void p4gl_raster_content_exclude(void *raster, int origin);
const char *p4gl_raster_content_key(const P4RwRaster *raster);
void p4gl_raster_content_reset(void);
/* Replaces level 0 of a raster with the width x height picture of three 8-bit planes (4:2:0, chroma planes (w+1)/2 x (h+1)/2, strides in
 * bytes), converted by the exact integer GPU shader.
 * Returns 0, or -1 when the platform/converter or GL pass cannot support it: the caller uses the reference CPU converter. */
int p4gl_raster_set_yuv420(P4RwRaster *raster, const uint8_t *const plane[3], const int32_t stride[3], int32_t width, int32_t height);
/* The GL texture of a raster that has no CPU pixels, created like p4gl_raster_set_rgba does (the image at the top-left of a transparent
 * black power-of-two texture) when it has none; 0 for an unknown raster. For render passes that write the image. */
uint32_t p4gl_raster_image_texture(P4RwRaster *raster);

/* Binds the GL texture of a raster value with the current sampler state. Returns 0 when the
 * raster has no GL texture (src/render/p4gl_im2d.c). */
int p4gl_texture_bind(uint32_t rasterValue);
/* As above, using a material texture's packed RwTexture.filterAddressing instead
 * of the immediate-mode render states. */
int p4gl_texture_bind_sampler(uint32_t rasterValue, uint32_t sampler);
/* The p4SampleBorder mode (the fragment shaders' uBorder) of the texture the last p4gl_texture_bind bound: 0 when the hardware samples
 * it, else the border emulation's packed wrap and filter modes (a context with no GL_CLAMP_TO_BORDER; src/render/p4gl_gl.c). A textured draw sets
 * its uBorder uniform from it right after the bind. */
int p4gl_texture_border_mode(void);

/* Camera whose RwCameraBeginUpdate is open, NULL outside one (src/render/p4gl_camera.c). */
const void *p4gl_camera_current(void);

/* Object-to-camera matrix m and column-major clip matrix mvpCol under the camera; NULL ltm is the
 * identity. Returns 0 for a degenerate camera (src/render/p4gl_camera.c). */
int p4gl_camera_object_matrix(const void *ltm, const void *camera, float m[4][4], float mvpCol[16]);

/* Constants that take a camera-space point to clip space: clip = (x * out[0], y * out[1],
 * z * out[2] + out[3], z). Returns 0 for a degenerate camera (src/render/p4gl_camera.c). */
int p4gl_camera_projection(const void *camera, float out[6]);

/* Im3D (src/render/p4gl_im3d.c; called from overrides/00410420.c and the other RwIm3D entry points).
 * Transform copies numVerts RwIm3DVertex records (36 bytes) and moves them to clip space under the
 * open camera with the object-to-world matrix ltm (NULL: identity); returns 1 on success, 0 with no
 * camera. Draw renders a RwPrimitiveType from the transformed vertices, sequentially when indices
 * is NULL, else through numIndices u16 indices; returns 1 when the call is valid, 0 when an index
 * is out of range or the GL state is not ready. End drops the transformed copy. */
int p4gl_im3d_transform(const void *verts, uint32_t numVerts, const void *ltm, uint32_t flags);
int p4gl_im3d_draw(int32_t primType, const uint16_t *indices, int32_t numIndices);
void p4gl_im3d_end(void);

/* A vertex in clip space, 40 bytes: position (x, y, z, w), colour (r, g, b, a in 0..1) and uv. */
typedef struct P4ClipVertex {
    float pos[4];
    float col[4];
    float uv[2];
} P4ClipVertex;
_Static_assert(sizeof(P4ClipVertex) == 40, "P4ClipVertex layout");

/* How p4gl_clip_draw_triangles sets blending, the alpha test and depth. With own == 0 it takes the
 * RpSky ALPHA_1 and TEST_1 state and RwRenderState ZTESTENABLE and ZWRITEENABLE, as Im3D does;
 * with own != 0 it takes the fields below (PTank carries its own GS registers). */
typedef struct P4ClipDrawParams {
    uint32_t raster;    /* P4RwRaster pointer value of the texture, 0 for none */
    float color[4];     /* multiplies the texel and the vertex colour (the material colour) */
    int own;
    int blend;          /* own: blending on */
    uint64_t alpha;     /* own: GS ALPHA register */
    uint32_t test;      /* own: GS TEST register, for the alpha test */
    int zTest;          /* own: depth test */
    int zWrite;         /* own: depth write */
} P4ClipDrawParams;

/* Draws indexed triangles from clip-space vertices with the Im3D program. Every index must be below
 * numVertices. Returns 1 when drawn (src/render/p4gl_im3d.c). */
int p4gl_clip_draw_triangles(const P4ClipVertex *verts, uint32_t numVerts, const uint32_t *indices,
                             uint32_t numIndices, const P4ClipDrawParams *params);

/* PS2PTank node body (overrides/003a8840.c): draws the particles of one PTank atomic
 * (src/render/p4gl_ptank.c). Returns 1 when drawn, 0 when skipped. */
int p4gl_ptank_draw(void *atomic);

/* GS ALPHA and TEST decoding, RpSky ALPHA_1 and TEST_1 for the GL draw paths, depth, frame mask and
 * CLAMP_1 override (src/render/p4gl_state.c). */
void p4gl_gs_alpha_apply(uint64_t reg);
int32_t p4gl_gs_test_func(uint32_t test);
float p4gl_gs_test_ref(uint32_t test);
void p4gl_sky_apply_blend(void);
/* P4PC_DRAW_LOG=<first>[-<last>]: a line per draw (src/render/p4gl_state.c). bbox is x0, y0, x1, y1 in pixels or NULL; rgba a vertex colour (0..255) or NULL. */
void p4gl_drawlog_frame(void);
int p4gl_drawlog(const char *path, int primType, int numVerts, const float *bbox, const float *rgba, const char *extra); /* 0: leave this draw out (P4PC_DRAW_UPTO / P4PC_DRAW_SKIP) */
int32_t p4gl_sky_alpha_func(void);
int p4gl_sky_alpha_uses_source(void);
int p4gl_sky_date(void);
uint32_t p4gl_sky_alpha_raw(void);
uint32_t p4gl_sky_test_raw(void);
int p4gl_gs_log_on(void);
int p4gl_gs_skip(void);
void p4gl_gs_log_draw(const char *kind, unsigned mode, int count, const float *box, uint32_t z, const float *rgba, uint32_t tex, uint64_t alpha, uint32_t test,
                      unsigned zt, unsigned zw);
/* Returns AFAIL (FB_ONLY=1, RGB_ONLY=3) when a colour-only failure pass is needed. */
int p4gl_gs_alpha_color_fail(uint32_t test, int32_t *func, float *ref);
void p4gl_frame_mask_rgb_only(void);
float p4gl_sky_alpha_ref(void);
void p4gl_depth_apply_values(int zTest, int zWrite);
void p4gl_depth_apply(void);
void p4gl_depth_restore(void);
void p4gl_atomic_set_viewer_light(int on);
void p4gl_cull_apply(void);
void p4gl_frame_mask_set(uint32_t fbmsk);
uint32_t p4gl_frame_mask_get(void);
void p4gl_frame_mask_apply(void);
void p4gl_frame_mask_restore(void);
void p4gl_gs_clamp_set(uint64_t clamp);
void p4gl_gs_clamp_clear(void);
int p4gl_gs_clamp_get(uint32_t *wms, uint32_t *wmt);

/* The value the game set for a RwRenderState id, else dflt; the FARFOGPLANE float when finite and past
 * fogStart, else 0 (src/render/p4gl_state.c). */
uint32_t p4gl_render_state_or(uint32_t state, uint32_t dflt);
float p4gl_sky_far_fog(float fogStart);

#endif
