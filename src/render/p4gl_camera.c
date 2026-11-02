/* RenderWare camera and frame handlers for the OpenGL device (stdFunc slots 1, 10, 20, 21).
 * Updates nest: camera textures keep their guest dimensions while physical storage and
 * viewports follow rendering density, then the enclosing camera and target are restored. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "p4gl.h"
#include "p4pc/platform.h"
#include "p4pc/capture_state.h"
#include "p4pc/battle_live.h"
#include "p4gl_gl.h"
#include "../platform/sound/p4snd.h"

/* RwCameraClearMode bits (rwcore.h:4584-4585). */
#define P4_CAMERA_CLEAR_IMAGE 0x1u
#define P4_CAMERA_CLEAR_Z 0x2u

/* RwCamera fields (retail layout; decomp rwcore.h). */
#define P4_CAMERA_PROJECTION 20   /* RwObjectHasFrame (including sync callback) is 20 bytes */
#define P4_CAMERA_VIEWWINDOW 104   /* RwCamera.viewWindow (RwV2d) */
#define P4_CAMERA_NEAR 128         /* RwCamera.nearPlane */
#define P4_CAMERA_FAR 132          /* RwCamera.farPlane */

/* Mirror of RwRGBA: four bytes, r g b a. */
typedef struct P4RwRGBA {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
} P4RwRGBA;

/* Camera between BEGIN and END; NULL outside an update. */
static const void *sCurrentCamera;
/* The camera-texture raster the open update renders into (RwCamera.frameBuffer of the field's shadow camera); NULL for the main target. */
static P4RwRaster *sTextureTarget;
#define P4_CAMERA_STACK 8
static struct {
    const void *camera;
    P4RwRaster *target;
} sStack[P4_CAMERA_STACK];
static int sDepth;
#define P4_CAMERA_FRAMEBUFFER 96   /* RwCamera.frameBuffer */
#define P4_RASTER_TYPE_MASK 0x0Fu
#define P4_RASTER_TYPE_CAMERATEXTURE 5

static float sAspectWidth = 1.0f;
static uint32_t sAspectNativeWidth = 640;
static struct {
    void *camera;
    float viewX, recipX;
    int changed;
} sAspectStack[P4_CAMERA_STACK];
static unsigned sAspectDepth;

void p4gl_aspect_set(float relativeWidth)
{
    sAspectWidth = relativeWidth;
    const uint32_t width = (uint32_t)(640.0f * relativeWidth + 0.5f);
    sAspectNativeWidth = width ? width : 1u;
}

float p4gl_aspect_width(void)
{
    return sAspectWidth;
}

uint32_t p4gl_aspect_native_width(void)
{
    return sAspectNativeWidth;
}

static void camera_dirty(void *camera)
{
    uint32_t frame;
    extern void *func_003e9680(void *);

    memcpy(&frame, (uint8_t *)camera + 4, sizeof frame);
    if (frame != 0) {
        func_003e9680(P4_G2H(frame));
    }
}

/* Run before RenderWare synchronises the frame, not at the device BEGIN slot:
 * that slot is too late to rebuild the CPU frustum. Preserve the retail window
 * outside Begin/End, so camera setup and repeated FOV queries never accumulate
 * the aspect multiplier. The view matrix, clip planes, corners and bounding box
 * are all built by the unchanged RW synchronisation from this wider window. */
void p4gl_aspect_camera_begin(void *camera)
{
    uint32_t frameBuffer, projection;
    P4RwRaster *raster;
    float x;

    if (sAspectWidth == 1.0f) return;
    if (sAspectDepth == P4_CAMERA_STACK) {
        fprintf(stderr, "p4gl: widescreen camera nesting exceeds device stack\n");
        abort();
    }
    sAspectStack[sAspectDepth].camera = camera;
    sAspectStack[sAspectDepth].changed = 0;
    memcpy(&frameBuffer, (uint8_t *)camera + P4_CAMERA_FRAMEBUFFER, sizeof frameBuffer);
    memcpy(&projection, (uint8_t *)camera + P4_CAMERA_PROJECTION, sizeof projection);
    raster = P4_G2H(frameBuffer);
    if (projection == 1 && (raster == NULL || (raster->cType & 7u) != P4_RASTER_TYPE_CAMERATEXTURE)) {
        memcpy(&sAspectStack[sAspectDepth].viewX, (uint8_t *)camera + 104, sizeof(float));
        memcpy(&sAspectStack[sAspectDepth].recipX, (uint8_t *)camera + 112, sizeof(float));
        x = sAspectStack[sAspectDepth].viewX * sAspectWidth;
        memcpy((uint8_t *)camera + 104, &x, sizeof x);
        x = 1.0f / x;
        memcpy((uint8_t *)camera + 112, &x, sizeof x);
        sAspectStack[sAspectDepth].changed = 1;
        camera_dirty(camera);
    }
    sAspectDepth++;
}

void p4gl_aspect_camera_end(void *camera)
{
    if (sAspectWidth == 1.0f) return;
    if (sAspectDepth == 0 || sAspectStack[sAspectDepth - 1].camera != camera) {
        fprintf(stderr, "p4gl: unbalanced widescreen camera update\n");
        abort();
    }
    sAspectDepth--;
    if (sAspectStack[sAspectDepth].changed) {
        memcpy((uint8_t *)camera + 104, &sAspectStack[sAspectDepth].viewX, sizeof(float));
        memcpy((uint8_t *)camera + 112, &sAspectStack[sAspectDepth].recipX, sizeof(float));
        camera_dirty(camera);
    }
}

/* The camera whose update is open (src/render/p4gl_atomic.c reads its view matrix). */
const void *p4gl_camera_current(void)
{
    return sCurrentCamera;
}

/* RwMatrix element at (row, col): row 0 right, 1 up, 2 at, 3 pos; column c at byte 4c. A NULL
 * matrix is the identity. */
static float rw_mat_at(const void *m, int row, int col)
{
    float value;

    if (m == NULL) {
        return row == col ? 1.0f : 0.0f;
    }
    memcpy(&value, (const uint8_t *)m + row * 16 + col * 4, sizeof value);
    return value;
}

static float camera_f32(const void *camera, size_t off)
{
    float value;

    memcpy(&value, (const uint8_t *)camera + off, sizeof value);
    return value;
}

/* PSC's 2D panel matrix normalises pixel aspect from viewWindow.y/x.
 * The live camera must stay Hor+ for projection and culling, but this layout
 * calculation needs the retail X or it squashes card heights by the width. */
float p4gl_camera_retail_view_x(const void *camera)
{
    for (unsigned i = sAspectDepth; i != 0; ) {
        --i;
        if (sAspectStack[i].camera == camera && sAspectStack[i].changed)
            return sAspectStack[i].viewX;
    }
    return camera_f32(camera, P4_CAMERA_VIEWWINDOW);
}

/* RwCamera.object.parent: the camera's RwFrame; RwFrame.ltm (RwMatrix) sits at +80. */
#define P4_CAMERA_FRAME 4
#define P4_FRAME_LTM_OFFSET 80

static float dot3(const void *a, int ra, const void *b, int rb)
{
    return rw_mat_at(a, ra, 0) * rw_mat_at(b, rb, 0) + rw_mat_at(a, ra, 1) * rw_mat_at(b, rb, 1) +
           rw_mat_at(a, ra, 2) * rw_mat_at(b, rb, 2);
}

/* Column-vector object-to-camera matrix m[row][col] (x right, y up, z along the view) from the
 * row-vector object frame LTM and the camera's own frame LTM (a NULL camera frame is the
 * identity): a world point is p * L.B + L.pos, and a camera coordinate is that point minus the
 * camera position dotted with the camera's right, up and at. The camera's viewMatrix field is not
 * used: RenderWare's RwCameraBeginUpdate stores in it the view matrix already scaled by the view
 * window and mapped to the unit square (right.x = -0.5 / viewWindow.x, at.x = 0.5, ...). */
static void object_to_camera(const void *ltm, const void *camera, float m[4][4])
{
    const void *frame;
    uint32_t frameAddr;
    const void *cam = NULL;
    float d[3];
    int i;
    int j;

    memcpy(&frameAddr, (const uint8_t *)camera + P4_CAMERA_FRAME, sizeof frameAddr);
    frame = P4_G2H(frameAddr);
    if (frame != NULL) {
        cam = (const uint8_t *)frame + P4_FRAME_LTM_OFFSET;
    }
    for (j = 0; j < 3; j++) {
        for (i = 0; i < 3; i++) {
            m[j][i] = dot3(ltm, i, cam, j);
        }
        for (i = 0; i < 3; i++) {
            d[i] = rw_mat_at(ltm, 3, i) - rw_mat_at(cam, 3, i);
        }
        m[j][3] = d[0] * rw_mat_at(cam, j, 0) + d[1] * rw_mat_at(cam, j, 1) + d[2] * rw_mat_at(cam, j, 2);
    }
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
}

/* Object-to-camera matrix m (for normals and lighting) and the column-major clip matrix
 * mvpCol = P * M, with P the RenderWare perspective from viewWindow and the near/far planes
 * (camera +z is the view direction; GL clip depth). RenderWare maps a camera-space point to the
 * screen as x = 0.5 - 0.5 * cx / (viewWindow.x * cz) and y = 0.5 - 0.5 * cy / (viewWindow.y * cz) in
 * the unit square with y down (RwCameraBeginUpdate, bacamera.c), so camera +x is the screen's LEFT:
 * the clip x here is -cx / viewWindow.x and the image is the mirror of a right-handed one. A NULL ltm
 * is the identity. Returns 0 for a degenerate camera. */
int p4gl_camera_object_matrix(const void *ltm, const void *camera, float m[4][4], float mvpCol[16])
{
    float vwX = camera_f32(camera, P4_CAMERA_VIEWWINDOW);
    float vwY = camera_f32(camera, P4_CAMERA_VIEWWINDOW + 4);
    float nearPlane = camera_f32(camera, P4_CAMERA_NEAR);
    float farPlane = camera_f32(camera, P4_CAMERA_FAR);
    float mvp[4][4];
    float a;
    float b;
    float bigA;
    float bigB;
    int r;
    int c;

    if (!(vwX > 0.0f) || !(vwY > 0.0f) || !(nearPlane > 0.0f) || !(farPlane > nearPlane)) {
        fprintf(stderr, "p4gl_camera: degenerate camera (view window %g x %g, near %g, far %g)\n",
                (double)vwX, (double)vwY, (double)nearPlane, (double)farPlane);
        return 0;
    }

    object_to_camera(ltm, camera, m);

    a = -1.0f / vwX;
    b = 1.0f / vwY;
    {
        const float fit = p4gl_layout_model_scale();
        if (fit != 1.0f) {
            a *= fit;
            b *= fit;
        }
    }
    bigA = (farPlane + nearPlane) / (farPlane - nearPlane);
    bigB = -2.0f * farPlane * nearPlane / (farPlane - nearPlane);
    for (c = 0; c < 4; c++) {
        mvp[0][c] = a * m[0][c];
        mvp[1][c] = b * m[1][c];
        mvp[2][c] = bigA * m[2][c] + bigB * m[3][c];
        mvp[3][c] = m[2][c];
    }
    {
        int width = 0;
        int height = 0;

        /* GS pixel centres are at integer coordinates, GL's at +0.5: the same coordinates land half a pixel
         * further right and down here. */
        if (p4gl_target_size(&width, &height)) {
            /* Kept at every internal resolution, as is Im2D's: the picture at N x is the 1x picture's geometry on a finer grid, not a shifted one (a polygon
             * lands half a pixel, of the logical size, right and down of where its coordinates say; docs/platform/video_presentation.md "Rendering resolution"). */
            for (c = 0; c < 4; c++) {
                mvp[0][c] += mvp[3][c] / (float)width;
                mvp[1][c] -= mvp[3][c] / (float)height;
            }
        }
    }
    for (c = 0; c < 4; c++) {
        for (r = 0; r < 4; r++) {
            mvpCol[c * 4 + r] = mvp[r][c];
        }
    }
    return 1;
}

/* The projection constants that turn a camera-space point (x, y, z) into clip space: clip =
 * (x * out[0] + z * out[4], y * out[1] + z * out[5], z * out[2] + out[3], z). The same perspective
 * as the matrix above: out[0] = -1 / viewWindow.x (camera +x is the screen's left), out[1] =
 * 1 / viewWindow.y, out[2], out[3] map the near and far planes to GL depth, and out[4], out[5] are
 * the half-pixel GS-centre shift (right and down, as in p4gl_camera_object_matrix). Returns 0 for a
 * degenerate camera. */
int p4gl_camera_projection(const void *camera, float out[6])
{
    float vwX = camera_f32(camera, P4_CAMERA_VIEWWINDOW);
    float vwY = camera_f32(camera, P4_CAMERA_VIEWWINDOW + 4);
    float nearPlane = camera_f32(camera, P4_CAMERA_NEAR);
    float farPlane = camera_f32(camera, P4_CAMERA_FAR);

    if (!(vwX > 0.0f) || !(vwY > 0.0f) || !(nearPlane > 0.0f) || !(farPlane > nearPlane)) {
        fprintf(stderr, "p4gl_camera: degenerate camera (view window %g x %g, near %g, far %g)\n",
                (double)vwX, (double)vwY, (double)nearPlane, (double)farPlane);
        return 0;
    }
    out[0] = -1.0f / vwX;
    out[1] = 1.0f / vwY;
    {
        const float fit = p4gl_layout_model_scale();
        if (fit != 1.0f) {
            out[0] *= fit;
            out[1] *= fit;
        }
    }
    out[2] = (farPlane + nearPlane) / (farPlane - nearPlane);
    out[3] = -2.0f * farPlane * nearPlane / (farPlane - nearPlane);
    out[4] = 0.0f;
    out[5] = 0.0f;
    {
        int width = 0;
        int height = 0;

        if (p4gl_target_size(&width, &height)) {
            out[4] = 1.0f / (float)width;
            out[5] = -1.0f / (float)height;
        }
    }
    return 1;
}

/* CAMERABEGINUPDATE (stdFunc 1): RwCameraBeginUpdate(camera) calls (NULL, camera, 0).
 * The GL context is current from OPEN. */
int32_t func_003f6a10(void *pOut, void *pInOut, int32_t nI)
{
    P4RwRaster *fb;
    uint32_t fbAddr;
    int w = 0;
    int h = 0;

    (void)pOut;
    (void)nI;
    if (pInOut == NULL || sDepth >= P4_CAMERA_STACK) {
        return 0;
    }
    memcpy(&fbAddr, (const uint8_t *)pInOut + P4_CAMERA_FRAMEBUFFER, sizeof fbAddr);
    fb = (P4RwRaster *)P4_G2H(fbAddr);
    if (fb != NULL && (fb->cType & P4_RASTER_TYPE_MASK) == P4_RASTER_TYPE_CAMERATEXTURE) {
        /* A failed scaled target must not become a successful main-target
         * draw or consume a camera-stack entry. Prepare preserves prior state. */
        if (!p4gl_raster_target_begin(fb, &w, &h)) return 0;
    } else {
        fb = NULL;
    }
    /* Updates nest: retain the enclosing camera/target only after preparation succeeds. */
    sStack[sDepth].camera = sCurrentCamera;
    sStack[sDepth].target = sTextureTarget;
    sDepth++;
    sCurrentCamera = pInOut;
    sTextureTarget = fb;
    return 1;
}

/* CAMERAENDUPDATE (stdFunc 10): calls (NULL, camera, 0). Leaves the render target of a camera-texture camera and returns to
 * the enclosing update's camera and target. */
int32_t func_003f73d0(void *pOut, void *pInOut, int32_t nI)
{
    P4RwRaster *left;
    int w = 0;
    int h = 0;

    (void)pOut;
    (void)nI;
    if (pInOut == NULL || sDepth == 0) {
        return 0;
    }
    left = sTextureTarget;
    sDepth--;
    sCurrentCamera = sStack[sDepth].camera;
    sTextureTarget = sStack[sDepth].target;
    if (left != NULL) {
        p4gl_target_override(0, 0, 0, 0);
        p4gl_raster_target_end(left);
        if ((sTextureTarget == NULL || !p4gl_raster_target_begin(sTextureTarget, &w, &h)) &&
            p4gl_target_pixel_size(&w, &h)) {
            p4gl_glViewport(0, 0, w, h);
        }
    }
    return 1;
}

/* CAMERACLEAR (stdFunc 21): called as (camera, color, clearMode). Clears the default
 * framebuffer, or the render target of a camera whose frame buffer is a camera-texture raster. */
int32_t func_003f7490(void *pOut, void *pInOut, int32_t nI)
{
    const P4RwRGBA *color = pInOut;
    uint32_t mode = (uint32_t)nI;
    GLbitfield bits = 0;

    uint32_t fbAddr;
    P4RwRaster *fb;
    float rgba[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (pOut == NULL) {
        return 0;
    }
    if (mode & P4_CAMERA_CLEAR_IMAGE) {
        if (color != NULL) {
            rgba[0] = color->red / 255.0f;
            rgba[1] = color->green / 255.0f;
            rgba[2] = color->blue / 255.0f;
            rgba[3] = color->alpha / 255.0f;
        }
        bits |= GL_COLOR_BUFFER_BIT;
    }
    if (mode & P4_CAMERA_CLEAR_Z) {
        bits |= GL_DEPTH_BUFFER_BIT;
    }
    if (bits == 0) {
        return 1;
    }
    memcpy(&fbAddr, (const uint8_t *)pOut + P4_CAMERA_FRAMEBUFFER, sizeof fbAddr);
    fb = (P4RwRaster *)P4_G2H(fbAddr);
    if (fb != NULL && (fb->cType & P4_RASTER_TYPE_MASK) == P4_RASTER_TYPE_CAMERATEXTURE) {
        p4gl_raster_target_clear(fb, bits, rgba);   /* a camera-texture camera clears its own target */
        return 1;
    }
    if (bits & GL_COLOR_BUFFER_BIT) {
        p4gl_glClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
    }
    p4gl_glClear(bits);
    return 1;
}

/* RASTERSHOWRASTER (stdFunc 20): called as (raster, dev, flags). Presents the window, then waits for the next
 * 60 Hz tick like the retail flip waits for the GS vsync: the game loop (kwlnMainLoop) never blocks anywhere else, so
 * this is where its worker threads (async model and file loads) get the CPU. */
void p4pc_heap_verify(void);                      /* src/platform/kernel_heap.c, P4PC_HEAP_CHECK */
void p4pc_battle_harness_frame(unsigned frame);   /* src/platform/battle_harness.c */
void p4pc_menu_harness_frame(unsigned frame);     /* src/platform/menu_harness.c */
void p4pc_newgame_gate_frame(unsigned frame);     /* src/platform/newgame_gate.c */
void p4pc_title_driver_frame(unsigned frame);     /* src/platform/title_driver.c, P4PC_AUTOPRESS_NEWGAME */
void p4pc_battle_driver_frame(unsigned frame);    /* src/platform/battle_driver.c, P4PC_BATTLE_DRIVER */
void p4pc_profile_flip(unsigned frame);           /* src/platform/profile.c, P4PC_PROFILE */
void p4pc_state_dump_flip(unsigned frame);        /* src/platform/state_dump.c, P4PC_DUMP_RAM_AT */
void p4pc_state_dump_request(unsigned frame);
void p4pc_lockstep_frame(void (*capture)(const char *));
void p4pc_lockstep_presented(void);
void p4gl_context_capture_next(const char *path);
int32_t func_003f79b0(void *pOut, void *pInOut, int32_t nI)
{
    (void)pOut;
    (void)pInOut;
    (void)nI;
    static unsigned flips;
    if (p4_battle_live_active) p4_battle_live_before_present();

    p4pc_heap_verify();
    p4pc_battle_harness_frame(++flips);
    p4pc_menu_harness_frame(flips);
    p4pc_title_driver_frame(flips);
    p4pc_battle_driver_frame(flips);
    p4pc_capture_state_flip(flips);
    if (p4pc_capture_state_wanted(flips)) p4pc_state_dump_request(flips);
    p4pc_newgame_gate_frame(flips);
    p4pc_profile_flip(flips);
    p4pc_state_dump_flip(flips);
    p4pc_lockstep_frame(p4gl_context_capture_next);
    p4gl_context_present();
    p4pc_lockstep_presented();
    if (p4_battle_live_active) p4_battle_live_after_present();
    p4snd_virtual_tick();
    p4pc_vsync_wait();
    return 1;
}
