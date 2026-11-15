/* p4pc_viewer: renders one game model through the RenderWare engine without
 * booting the game. It loads the same retail image as p4pc (src/image.c), runs
 * the start-up that kwlnBoot runs before the main loop (kernel init, boot heap,
 * IOP and platform init), then brings up the engine with the game's own
 * kwlnInitEngine. It loads the RMD with the game's model loader, draws its clump
 * through the game's own atomic path, and reads the frame back.
 *
 * Usage: p4pc_viewer [SLUS_217.82 [RMD [out.png]]]
 *   The disc is read through P4PC_ISO, as for p4pc. RMD is a VFS path such as
 *   field/rmd/m<MAJOR>_<MINOR>.RMD (K_Cmd_CREATE_FLD_MDL, decomp/src/Kosaka/k_command.c),
 *   and is required. out.png defaults to p4pc_viewer.png. `pack.bin#name.RMD` takes the named entry
 *   of a pack file that stores RMDs after a 0x100-byte name (title/title.bin#tc01.RMD, loaded the way
 *   the title scene does, from memory); `ptank` draws a PTank atomic instead of a model.
 *
 * The camera is placed from the clump's bounds (frame_camera). */
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/gmem.h"
#include "p4pc/image.h"
#include "p4pc/os.h"
#include "p4pc/platform.h"
#include "p4pc/ps2_libc.h"
#include "p4pc/content.h"
#include "p4pc/content_source.h"
#include "p4pc/mod_models.h"
#include "../render/p4gl.h"
#include "../render/p4gl_gl.h"
#include "../render/p4gl_readback.h"
#include "p4png.h"
#include "vfs.h"

#define VIEW_W 640u
#define VIEW_H 448u
void p4gl_context_capture_next(const char *path);

/* Decomp functions, native when compiled, otherwise generated trap stubs. */
void *kwlnProbeHeapBudget(void);
void kwlnInitIop(void);
void kwlnInitEngine(void);
/* Kernel camera with its own frame and Z buffer (kwlnTask.c, FUN_004574D0). */
void *kwlnCameraCreate(uint32_t width, uint32_t height, uint32_t createZBuffer);
/* Model loader, clump accessor and atomic draw (mdlManager.c). Type 4 is the RMD model type
 * K_Cmd_CREATE_FLD_MDL passes. */
void *mdlCreateModelObjectFromData(uint32_t type, uint16_t id, void *path, uint32_t flags);
void *mdlGetClump(void *model);
void *mdlCreateModelObjectFromMemory(uint32_t type, uint16_t id, void *memory, uint32_t size, uint32_t flags);
void mdlDrawVisibleAtomics(void *clump);
void mdlAdvanceModelFromFlags(void *model);
/* Camera begin and end (src/render/p4gl_camera.c). */
int32_t func_003f6a10(void *pOut, void *pInOut, int32_t nI);
int32_t func_003f73d0(void *pOut, void *pInOut, int32_t nI);
float func_0044a4d0(float value); /* retail sqrtf for the guest camera */
/* Model readiness (mdlManager.c, FUN_004782B0) and the disc request pump (sdkCdvd.c). */
int32_t mdlPrepareModel(void *model);
void H_Cdvd_UpdateRequests(void);
/* The asynchronous load queue: hsfdInitWorker creates its worker thread, sdkWrapPollRequests is the per-frame
 * pump that hands finished disc requests to that thread (sdkWrap.c, h_sfdply.c). */
void hsfdInitWorker(void);
void sdkWrapPollRequests(void);
/* ChangeThreadPriority (src/platform/kernel_thread.c). Threads are cooperative coroutines, so the
 * worker only runs when the boot thread is below it and enters the kernel (WakeupThread). */
int32_t func_00421670(int32_t id, int32_t prio);

/* RenderWare entry points the viewer drives (compiled decomp units). */
void *RpClumpForAllAtomics(void *clump, void *(*callback)(void *atomic, void *data), void *data);
/* RwFrameTranslate is retail 0x003E9C10 (0x003E9D50 is RwFrameRotate). */
void *func_003e9c10(void *frame, const float *translation, int combineOp);
void *RwCameraSetNearClipPlane(void *camera, float nearClip);
void *RwCameraSetFarClipPlane(void *camera, float farClip);
void *RwCameraBeginUpdate(void *camera);
void *RwCameraEndUpdate(void *camera);
#define RW_COMBINE_REPLACE 0

/* RpPTankAtomicCreate (0x003a2340), the lock (0x003a2770) and the frame calls the PTank test uses. */
uint32_t func_003a2340(uint32_t maxP, uint32_t dataFlags, uint32_t platFlags);   /* RpPTankAtomicCreate: the atomic's guest address */
int32_t func_003a2770(void *atomic, uint32_t *lock, uint32_t flag, uint32_t lockMode);
void *RwFrameCreate(void);
void *RwFrameGetLTM(void *frame);
void *RpAtomicSetFrame(void *atomic, void *frame);

/* Axis-aligned bounds of the clump in world space from each atomic's morph target 0 bounding
 * sphere (RpMorphTarget +4: centre, +16: radius) moved by its frame's LTM (RwFrame +80). */
typedef struct Bounds {
    float lo[3];
    float hi[3];
    int count;
} Bounds;

static float f32_at(const void *base, size_t off)
{
    float v;

    memcpy(&v, (const uint8_t *)base + off, sizeof v);
    return v;
}

static const void *ptr_at(const void *base, size_t off)
{
    uint32_t v; /* a pointer field of a retail struct is a guest address */

    memcpy(&v, (const uint8_t *)base + off, sizeof v);
    return P4_G2H(v);
}

/* Stores a host pointer into a retail struct field (a guest address: 4 bytes). */
static void put_ptr(void *base, size_t off, const void *p)
{
    uint32_t v = P4_H2G(p);

    memcpy((uint8_t *)base + off, &v, sizeof v);
}

static void bounds_add(Bounds *b, const float p[3], float r)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (b->count == 0 || p[i] - r < b->lo[i]) {
            b->lo[i] = p[i] - r;
        }
        if (b->count == 0 || p[i] + r > b->hi[i]) {
            b->hi[i] = p[i] + r;
        }
    }
    b->count++;
}

/* Bounds of one atomic: its morph target 0 vertices (RpGeometry +0x14 numVertices, +0x5C morph
 * target; RpMorphTarget +0x14 vertices) moved by the frame LTM, or the bounding sphere when the
 * geometry holds no vertices. */
static void *bounds_atomic(void *atomic, void *data)
{
    Bounds *b = data;
    const uint8_t *geometry = ptr_at(atomic, 24);
    const uint8_t *morph = geometry != NULL ? ptr_at(geometry, 92) : NULL;
    void *frame = (void *)ptr_at(atomic, 4);
    /* The frame's LTM is computed lazily: RwFrameGetLTM brings it up to date. */
    const uint8_t *ltm = frame != NULL ? RwFrameGetLTM(frame) : NULL;
    const uint8_t *verts;
    int32_t nverts;
    float c[3];
    float p[3];
    int i;

    if (morph == NULL) {
        return atomic;
    }
    memcpy(&nverts, geometry + 0x14, sizeof nverts);
    verts = ptr_at(morph, 0x14);
    if (verts != NULL && nverts > 0) {
        int32_t v;

        for (v = 0; v < nverts; v++) {
            for (i = 0; i < 3; i++) {
                c[i] = f32_at(verts, (size_t)v * 12u + 4u * (size_t)i);
            }
            for (i = 0; i < 3; i++) {
                p[i] = ltm != NULL ? c[0] * f32_at(ltm, 0 + 4 * (size_t)i) + c[1] * f32_at(ltm, 16 + 4 * (size_t)i) +
                                         c[2] * f32_at(ltm, 32 + 4 * (size_t)i) + f32_at(ltm, 48 + 4 * (size_t)i)
                                   : c[i];
            }
            bounds_add(b, p, 0.0f);
        }
        return atomic;
    }
    for (i = 0; i < 3; i++) {
        c[i] = f32_at(morph, 4 + 4 * (size_t)i);
    }
    for (i = 0; i < 3; i++) {
        p[i] = ltm != NULL ? c[0] * f32_at(ltm, 0 + 4 * (size_t)i) + c[1] * f32_at(ltm, 16 + 4 * (size_t)i) +
                                 c[2] * f32_at(ltm, 32 + 4 * (size_t)i) + f32_at(ltm, 48 + 4 * (size_t)i)
                           : c[i];
    }
    bounds_add(b, p, f32_at(morph, 16));
    return atomic;
}

/* Puts the camera (identity orientation, looking down +z) in front of the model so that its
 * bounding sphere fills the view, and sets the clip planes around it. */
static void frame_camera(void *camera, void *clump)
{
    Bounds b;
    float centre[3];
    float radius = 0.0f;
    float window;
    float dist;
    float pos[3];
    int i;

    memset(&b, 0, sizeof b);
    RpClumpForAllAtomics(clump, bounds_atomic, &b);
    if (b.count == 0) {
        fprintf(stderr, "p4pc_viewer: clump has no bounding sphere; camera left at the default\n");
        return;
    }
    for (i = 0; i < 3; i++) {
        float half = (b.hi[i] - b.lo[i]) * 0.5f;

        centre[i] = (b.lo[i] + b.hi[i]) * 0.5f;
        radius += half * half;
    }
    radius = func_0044a4d0(radius);
    window = f32_at(camera, 104);   /* RwCamera.viewWindow.x */
    if (f32_at(camera, 108) < window) {
        window = f32_at(camera, 108);
    }
    dist = radius / (window > 0.01f ? window : 0.5f) * 1.15f;
    pos[0] = centre[0];
    pos[1] = centre[1];
    pos[2] = centre[2] - dist;
    func_003e9c10(P4_G2H(*(uint32_t *)((uint8_t *)camera + 4)), pos, RW_COMBINE_REPLACE);
    RwCameraSetNearClipPlane(camera, dist - radius * 1.5f > dist * 0.02f ? dist - radius * 1.5f : dist * 0.02f);
    RwCameraSetFarClipPlane(camera, dist + radius * 2.5f);
    fprintf(stderr, "p4pc_viewer: bounds centre %.1f %.1f %.1f radius %.1f, camera at z %.1f\n",
            (double)centre[0], (double)centre[1], (double)centre[2], (double)radius, (double)pos[2]);
}

/* A pack file of RMDs: entries are a 0x100-byte NUL-padded name followed by the RMD file, on 0x80
 * boundaries. The RMD is a 14-byte header (f0f000f0), top-level chunks and a footer chunk (f0f00004),
 * so its size follows from walking the chunks. Returns a malloc'd copy of the named RMD (kept for the
 * model's life), or NULL. */
static uint8_t *load_pack_rmd(const char *pack, const char *name, uint32_t *rmdSize)
{
    char path[256];
    long long size = 0;
    int isDir = 0;
    uint8_t *file;
    int fd;
    size_t off;
    size_t i;
    P4ContentReadScope inputScope;

    snprintf(path, sizeof path, "/%s", pack);
    for (i = 0; path[i] != '\0'; i++) {
        if (path[i] >= 'a' && path[i] <= 'z') {
            path[i] = (char)(path[i] - 'a' + 'A');
        }
    }
    if (vfs_stat(path, &isDir, &size) != 0 || size <= 0 || (fd = vfs_open(path, 0)) < 0) {
        return NULL;
    }
    file = malloc((size_t)size);
    if (p4_content_sources_enabled)
        p4_content_host_read_begin(&inputScope,file,(size_t)size);
    if (file == NULL || vfs_read(fd, file, (int)size) != (int)size) {
        if (p4_content_sources_enabled) p4_content_host_read_end(&inputScope);
        free(file);
        vfs_close(fd);
        return NULL;
    }
    vfs_close(fd);
    for (off = 0; off + 0x100 + 14 <= (size_t)size; off += 0x80) {
        uint32_t magic;
        const uint8_t *rmd;
        size_t pos;

        if (strncmp((const char *)file + off, name, 0x100) != 0) {
            continue;
        }
        rmd = file + off + 0x100;
        memcpy(&magic, rmd, sizeof magic);
        if (magic != 0xF0F000F0u) {
            continue;
        }
        for (pos = 14; pos + 12 <= (size_t)size - off - 0x100;) {
            uint32_t type;
            uint32_t len;

            memcpy(&type, rmd + pos, 4);
            memcpy(&len, rmd + pos + 4, 4);
            pos += 12;
            if (type == 0xF0F00004u) {
                uint8_t *copy = p4pc_heap_malloc(pos);      /* the model keeps it: game memory, so it has a guest address */

                if (copy != NULL) {
                    memcpy(copy, rmd, pos);
                    if (p4_content_sources_enabled) {
                        P4ContentSource source;
                        if (p4_content_source_find(rmd,pos,&source))
                            p4_content_source_mark(copy,pos,&source);
                    }
                    *rmdSize = (uint32_t)pos;
                }
                if (p4_content_sources_enabled) p4_content_host_read_end(&inputScope);
                free(file);
                return copy;
            }
            pos += len;
        }
    }
    if (p4_content_sources_enabled) p4_content_host_read_end(&inputScope);
    free(file);
    return NULL;
}

/* PTank path of the viewer (rmd path "ptank"): creates a four-particle matrix tank with the retail
 * create chain, fills the matrix and colour clusters through the retail lock, and draws it through
 * the atomic's render callback. Each particle is a 1 x 1 quad (the matrix rows are half-extents),
 * in a row at z 4; the colours are red, green, blue, white. Returns the atomic or NULL. */
static void *make_ptank(void) {
    static const float xs[4] = {-1.5f, -0.5f, 0.5f, 1.5f};
    static const uint8_t cols[4][4] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}, {255, 255, 255, 255}};
    uint32_t lock[2];
    uint8_t *atomic = P4_G2H(func_003a2340(4, 0x2008000Au, 0));
    uint8_t *record;
    uint32_t recordAddr;
    int i;

    if (atomic == NULL) {
        fprintf(stderr, "p4pc_viewer: RpPTankAtomicCreate failed\n");
        return NULL;
    }
    RpAtomicSetFrame(atomic, RwFrameCreate());
    if (func_003a2770(atomic, lock, 8, 0x40000000u) == 0 || lock[0] == 0) {
        fprintf(stderr, "p4pc_viewer: PTank matrix lock failed\n");
        return NULL;
    }
    for (i = 0; i < 4; i++) {
        float m[16] = {0.5f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0, 1.0f, 0, xs[i], 0, 4.0f, 1.0f};

        memcpy((uint8_t *)P4_G2H(lock[0]) + (size_t)i * lock[1], m, sizeof m);
    }
    if (func_003a2770(atomic, lock, 2, 0x40000000u) == 0 || lock[0] == 0) {
        fprintf(stderr, "p4pc_viewer: PTank colour lock failed\n");
        return NULL;
    }
    for (i = 0; i < 4; i++) {
        memcpy((uint8_t *)P4_G2H(lock[0]) + (size_t)i * lock[1], cols[i], 4);
    }
    /* The record sits at atomic + the PTank plugin offset (gp-18928, 0x00764700); +4 is actPCount. */
    memcpy(&recordAddr, atomic + *(const int32_t *)P4_G2H(0x00764700u), sizeof recordAddr);
    record = (uint8_t *)P4_G2H(recordAddr);
    *(uint32_t *)(record + 4) = 4;
    return atomic;
}

/* P4PC_REF_RGB names a raw 640x448 RGB reference (the PCSX2 frame, kept beside the retail image, never in the
 * repository): the frame's mean absolute difference from it is printed, and with P4PC_REF_MAX (percent) a frame
 * above that fails the run. tests/title_mad.sh uses this as the renderer's regression gate. */
static int check_reference(const uint8_t *rgba, uint32_t width, uint32_t height) {
    const char *refPath = getenv("P4PC_REF_RGB");
    const char *maxText = p4gl_gate_limit("P4PC_REF_MAX");
    double mad;

    if (refPath == NULL) {
        return 0;
    }
    if (p4gl_reference_mad(rgba, width, height, refPath, &mad) != 0) return 1;
    fprintf(stderr, "p4pc_viewer: mean abs diff from %s = %.4f%%\n", refPath, mad);
    if (maxText != NULL && mad > atof(maxText)) {
        fprintf(stderr, "p4pc_viewer: FAIL: above the %s%% limit\n", maxText);
        return 1;
    }
    return 0;
}

/* Reads the frame back and writes it as an opaque PNG. The framebuffer alpha holds GS blend state (the
 * mask the title composes with), not coverage, so it is not written. */
static int write_frame(const char *pngPath) {
    uint32_t pw = 0, ph = 0;
    uint8_t *pixels;
    int bad;

    p4gl_readback_pixel_size(&pw, &ph);
    if ((uint64_t)pw * ph > SIZE_MAX / 4u) {
        fprintf(stderr, "p4pc_viewer: %ux%u exceeds addressable RGBA capture storage\n", pw, ph);
        return 1;
    }
    /* Resolve at capture time: native storage may have followed a resize. */
    pixels = malloc((size_t)pw * ph * 4u);
    if (pixels == NULL) return 1;
    bad = p4gl_readback_rgba(pw, ph, pixels) != 0;
    for (size_t i = 3; !bad && i < (size_t)pw * ph * 4; i += 4) pixels[i] = 0xFF;
    if (!bad) bad = p4png_write_rgba(pngPath, pw, ph, pixels) != 0;
    if (!bad) bad = check_reference(pixels, pw, ph);
    free(pixels);
    if (!bad && getenv("P4PC_CAPTURE_WINDOW") != NULL) {
        p4gl_context_capture_next(pngPath);
        p4gl_context_present();
    }
    return bad;
}

static int viewer_target_create(void) {
    uint32_t w = 0, h = 0;
    p4gl_readback_pixel_size(&w, &h);
    return p4gl_readback_configure(1, w ? w : VIEW_W, h ? h : VIEW_H);
}

/* Loads the RMD, draws its clump into the readback FBO and writes the PNG. Returns 0 on
 * success. */
static int render_rmd(const char *rmdPath, const char *pngPath) {
    p4gl_atomic_set_viewer_light(1); /* a bare model has no game lights: a headlight makes its shape readable */
    void *model;
    void *clump;
    void *camera;
    void *ptank = NULL;
    int ptankMode = strcmp(rmdPath, "ptank") == 0;

    {
        const char *hash = strchr(rmdPath, '#');

        if (ptankMode) {
            model = NULL;
        } else if (hash != NULL) {
            char pack[128];
            uint32_t rmdSize = 0;
            uint8_t *rmd;

            snprintf(pack, sizeof pack, "%.*s", (int)(hash - rmdPath), rmdPath);
            rmd = load_pack_rmd(pack, hash + 1, &rmdSize);
            if (rmd == NULL) {
                fprintf(stderr, "p4pc_viewer: %s is not in %s\n", hash + 1, pack);
                return 1;
            }
            model = mdlCreateModelObjectFromMemory(9, 0xFF01, rmd, rmdSize, 0);
        } else {
            /* P4PC_VIEWER_MODEL: the RMD gets that glTF as its override (docs/modding/models.md). */
            if (p4_model_viewer_override(rmdPath) != 0) {
                return 1;
            }
            model = mdlCreateModelObjectFromData(4, 0, P4_G2H(p4_gport_str(rmdPath)), 0);
        }
    }
    /* The load is asynchronous: the game advances it from its frame task through the disc request
     * update and mdlPrepareModel, which returns 1 once the model is ready. */
    if (model != NULL) {
        int i;

        for (i = 0; i < 5000 && mdlPrepareModel(model) == 0; i++) {
            H_Cdvd_UpdateRequests();
            sdkWrapPollRequests();
            p4pc_wait_frame_boundary();
        }
    }
    if (model != NULL) {
        const void *aux = ptr_at(model, 0x30C);
        uint32_t flags;

        memcpy(&flags, (const uint8_t *)model + 0xD8, sizeof flags);
        if (aux != NULL) {
            int k;

            for (k = 4; k <= 0x18; k += 4) {
                const uint8_t *req = ptr_at(aux, (size_t)k);
                uint32_t st = 0;

                if (req != NULL && k != 0x14 && k != 0x18) {
                    memcpy(&st, req + 0x1A4, sizeof st);
                }
                fprintf(stderr, "p4pc_viewer: aux request +%d = %p (state %u, path %.40s)\n", k, (const void *)req,
                        (unsigned)st, req != NULL && k != 0x14 && k != 0x18 ? (const char *)req + 0xA4 : "");
            }
        }
        fprintf(stderr, "p4pc_viewer: model %p flags 0x%x aux %p aux state %d\n", model, (unsigned)flags,
                aux, aux != NULL ? (int)((const uint8_t *)aux)[0x3C] : -1);
    }
    if (model != NULL) {
        /* One animation step: the game's per-frame update evaluates the hierarchy (poses the skin). */
        mdlAdvanceModelFromFlags(model);
    }
    clump = model != NULL ? mdlGetClump(model) : NULL;
    if (clump == NULL && !ptankMode) {
        fprintf(stderr, "p4pc_viewer: cannot load %s\n", rmdPath);
        return 1;
    }
    if (clump != NULL && p4_model_viewer_dump(clump) != 0) {   /* P4PC_VIEWER_DUMP */
        return 1;
    }
    if (p4gl_context_open() != 0) {
        fprintf(stderr, "p4pc_viewer: GL context\n");
        return 1;
    }
    camera = kwlnCameraCreate(VIEW_W, VIEW_H, 1);
    if (camera == NULL) {
        fprintf(stderr, "p4pc_viewer: camera\n");
        return 1;
    }
    if (viewer_target_create() != 0) {
        fprintf(stderr, "p4pc_viewer: readback target\n");
        return 1;
    }

    if (ptankMode) {
        ptank = make_ptank();
        if (ptank == NULL) {
            return 1;
        }
    } else {
        frame_camera(camera, clump);
    }
    p4gl_glClearColor(0.25f, 0.3f, 0.4f, 1.0f);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* RwCameraBeginUpdate builds the view matrix from the camera frame, then reaches the device. */
    if (RwCameraBeginUpdate(camera) == NULL) {
        fprintf(stderr, "p4pc_viewer: camera begin\n");
        return 1;
    }
    if (ptank != NULL) {
        /* RpAtomicRender: the atomic's render callback is at +0x48. */
        (*(void *(**)(void *))((uint8_t *)ptank + 0x48))(ptank);
    } else {
        mdlDrawVisibleAtomics(clump);
    }
    RwCameraEndUpdate(camera);

    if (write_frame(pngPath) != 0) {
        fprintf(stderr, "p4pc_viewer: readback or PNG write failed\n");
        return 1;
    }
    return 0;
}

void p4pc_profile_flip(unsigned flip); /* src/platform/profile.c */

/* P4PC_DRAW_WINDOW=w: a scene is stepped for its whole frame count, but only the last w frames draw (glClear / glDrawArrays /
 * glDrawElements are dropped before: the rasteriser is nearly all of a gate's time on llvmpipe). Uploads, blits and state calls
 * still run, so the textures and targets the last frames sample are valid. w is the smallest warm-up that reproduces the
 * full-render frame exactly (frame feedback and render-to-texture passes need earlier frames); unset draws every frame. */
static void drawWindow(int frame, int frames)
{
    static int w = -2, head;

    if (w == -2) {
        const char *e = getenv("P4PC_DRAW_WINDOW");
        const char *h = getenv("P4PC_DRAW_HEAD");

        w = e != NULL && *e != '\0' ? atoi(e) : -1;
        head = h != NULL ? atoi(h) : 0;
    }
    p4gl_drawlog_frame();
    if (w >= 0) {
        p4gl_gl_set_draw(frame < head || frame >= frames - w);
    }
}

/* Title mode (rmd path "title"): runs the title task's own update (titleTaskUpdate) on a task record of ours
 * until it has loaded title/title.bin, built the sprite pack and the 15 models and placed the game's camera,
 * then draws the "press any button" frame (state 7) through the task's draw callback (titleDrawTaskView), the
 * way the draw queue would. The task record is the 0x90-byte work area (0x40 bytes of KwlnTask with the
 * work pointer at +0x38). */
uint32_t kwlnGetCamera(void);   /* a guest address: the mirror's `int` */
void titleDrawTaskView(void *unused, void *task);
void titleDrawScene(int32_t index, int32_t brightness, int32_t unused, int32_t mode, void *work);
int32_t shdSpriteDrawNow(float x, float y, float z, int32_t rgb, uint8_t alpha, int32_t sprite, int32_t frame, void *pack, int32_t flags);
void *H_Cdvd_AcquireDirectory(void *name, int32_t flag);
int32_t H_Cdvd_IsFileLoaded(void *dir);
void *sdkSpritePackCreate(void *spec);
void *func_003e9cb0(void *frame, const float *matrix, int combine);
void K_View_SetFov(void *camera, float fov);
void *func_003e81c0(void *camera, float far);
void *func_003e8180(void *camera, float near);
void resourceLoadSprites(void);
int32_t H_Cdvd_LookupRequest(uint32_t name, int32_t *size);   /* decomp: the name's guest address */
int32_t shdSpriteEnsureLoaded(void *pack);
int32_t taskLoadSlotModels(void *work);
void kwlnTaskInitPool(int32_t count);
void PrimCreatePools(int32_t nodes, int32_t extra);

static int render_title(const char *pngPath) {
    uint8_t *work = p4pc_heap_calloc(1, 0x90);     /* the title task's records are game memory */
    uint8_t *task = p4pc_heap_calloc(1, 0x40);
    void *camera;
    int i;

    if (work == NULL || task == NULL) {
        return 1;
    }
    p4gl_atomic_set_viewer_light(0);
    put_ptr(task, 0x38, work);
    kwlnTaskInitPool(0x190);
    PrimCreatePools(0x7D0, 0x64);
    if (p4gl_context_open() != 0 || viewer_target_create() != 0) {
        fprintf(stderr, "p4pc_viewer: GL context\n");
        return 1;
    }
    /* States 1 to 3 of the title task (titleTaskUpdate): the pack directory, the sprite pack, the models. The
     * task itself also queues sound and a draw node, which the viewer has no use for. */
    {
        void *dir = H_Cdvd_AcquireDirectory((void *)P4_G2H(0x005E5710u), 1);
        void *pack;

        put_ptr(work, 0x38, dir);
        for (i = 0; i < 6000 && H_Cdvd_IsFileLoaded(dir) == 0; i++) {
            H_Cdvd_UpdateRequests();
            sdkWrapPollRequests();
            p4pc_wait_frame_boundary();
        }
        pack = sdkSpritePackCreate((void *)P4_G2H(0x005E5220u));
        put_ptr(work, 0x3C, pack);
        for (i = 0; i < 6000; i++) {
            H_Cdvd_UpdateRequests();
            resourceLoadSprites();
            sdkWrapPollRequests();
            if (shdSpriteEnsureLoaded(pack) != 0 && taskLoadSlotModels(work) != 0) {
                break;
            }
            p4pc_wait_frame_boundary();
        }
    }
    fprintf(stderr, "p4pc_viewer: title loaded after %d steps (sprites %p model0 %p)\n", i, ptr_at(work, 0x3C),
            ptr_at(work, 0x44));
    {
        static const char *names[] = {"title/title.bin", "title/p4_tittle01.spr", "title/tittle_bg01.spr", "title/tc01.RMD"};
        int k;

        for (k = 0; k < 4; k++) {
            int32_t sz = 0;
            int32_t r = H_Cdvd_LookupRequest(p4_gport_str(names[k]), &sz);

            fprintf(stderr, "p4pc_viewer: lookup %s -> %#x size %d\n", names[k], (unsigned)r, (int)sz);
        }
    }
    camera = P4_G2H(kwlnGetCamera());
    {
        /* State 4: the title camera (a quaternion rotation, a translation, fov 50, planes 10 and 25600). */
        const float q[4] = {0.1636199951171875f, 0.11909999698400497f, -0.10168199986219406f, 0.9740149974822998f};
        const float tr[3] = {20.662647247314453f, 584.5792236328125f, -248.3194580078125f};
        float m[16] = {0};
        void *frame = (void *)ptr_at(camera, 4);

        m[0] = 1.0f - 2.0f * (q[1] * q[1] + q[2] * q[2]);
        m[1] = 2.0f * (q[0] * q[1] + q[3] * q[2]);
        m[2] = 2.0f * (q[2] * q[0] - q[3] * q[1]);
        m[4] = 2.0f * (q[0] * q[1] - q[3] * q[2]);
        m[5] = 1.0f - 2.0f * (q[0] * q[0] + q[2] * q[2]);
        m[6] = 2.0f * (q[1] * q[2] + q[3] * q[0]);
        m[8] = 2.0f * (q[2] * q[0] + q[3] * q[1]);
        m[9] = 2.0f * (q[1] * q[2] - q[3] * q[0]);
        m[10] = 1.0f - 2.0f * (q[0] * q[0] + q[1] * q[1]);
        ((uint32_t *)m)[3] = 3;
        func_003e9cb0(frame, m, 0);
        func_003e9c10(frame, tr, 2);
        K_View_SetFov(camera, 50.0f);
        func_003e81c0(camera, 25600.0f);
        func_003e8180(camera, 10.0f);
    }
    if (camera == NULL) {
        fprintf(stderr, "p4pc_viewer: no game camera\n");
        return 1;
    }
    {
        /* P4PC_TITLE_FRAME=n plays the title from its first drawn frame: 20 frames of state 3 (bars slide in),
         * 236 of state 5 (the silhouette intro, ended by timer in titleTaskUpdate) and then state 7 (the settled
         * title), the draw callback counting its own timers as in retail. Unset: one frame of state 7 with
         * the fade timer at its end, PRESS ANY BUTTON fully visible. */
        const char *frameEnv = getenv("P4PC_TITLE_FRAME");
        const char *stage = getenv("P4PC_TITLE_STAGE");
        const int frames = frameEnv != NULL ? func_0043c6b0(frameEnv) + 1 : 1;
        int frame;

        if (frameEnv == NULL) {
            *(int32_t *)work = 7;
            *(int32_t *)(work + 4) = 7; /* the draw callback switches on the previous-state word */
            *(int32_t *)(work + 0xC) = 0x14;
        }
        for (frame = 0; frame < frames; frame++) {
            drawWindow(frame, frames);
            if (frameEnv != NULL) {
                const int state = frame < 20 ? 3 : frame < 256 ? 5 : 7;

                if (frame == 20 || frame == 256) {
                    *(int32_t *)(work + 0xC) = 0; /* titleTaskUpdate zeroes the timer entering states 5 and 7 */
                }
                *(int32_t *)work = state;
                *(int32_t *)(work + 4) = state;
                if (state == 5 && *(int32_t *)(work + 0xC) == 0x54) {
                    *(int32_t *)(work + 0xC) = 0x55; /* the draw starts the title jingle at timer 0x55 (sound is unported): skip that frame */
                }
            }
            p4gl_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            if (RwCameraBeginUpdate(camera) == NULL) {
                fprintf(stderr, "p4pc_viewer: camera begin\n");
                return 1;
            }
            if (stage != NULL && stage[0] == '1') {
                titleDrawScene(0xA, 0xFF, 0xFF, 0x40, work);
            } else if (stage != NULL && stage[0] == '2') {
                shdSpriteDrawNow(0.0f, 0.0f, 0.0f, 0xFFFFFF, 0xFF, 0, 0, (void *)ptr_at(work, 0x3C), 1);
            } else {
                titleDrawTaskView(NULL, task);
            }
            if (frame + 1 < frames) {
                RwCameraEndUpdate(camera);
            }
        }
    }
    RwCameraEndUpdate(camera);
    if (write_frame(pngPath) != 0) {
        fprintf(stderr, "p4pc_viewer: readback or PNG write failed\n");
        return 1;
    }
    return 0;
}

/* Field mode (rmd path "field:<field>:<room>", e.g. field:1:1): the game's own scene path. kwlnInitSubsystems
 * creates the draw tasks (clear, resource render, frame flush), then fieldSceneLoad / fieldSceneLoadStep load
 * the field pack and build the scene, kwlnRunFrame runs the task frames, and the frame task's flush draws the
 * queued resources through the sky2 replacement. The camera is the scene's own (MT_Scene_GetCameraResFirst). */
void kwlnInitSubsystems(void);
void *kwlnTaskGetTaskByName(const void *name);
int32_t kwlnTaskDestroyWithHierarchy(void *task);
void gameSequenceStop(void);
void kwlnTaskDumpAll(void);
void *MT_Scene_GetResListHead(int32_t type);
int32_t kwlnRunFrame(void);
void fieldSceneLoad(int32_t field, int32_t room);
void fieldSceneRequestRoom(int64_t state);
int32_t fieldSceneLoadStep(void);
float MT_Scene_GetCameraFov(void);
int32_t mtSceneResListPrepareAll(void);
void sequenceRequest(int32_t type, int32_t arg1, int32_t size, int32_t delay);
void datInitDefaultGameState(void);

static int render_field(const char *spec, const char *pngPath) {
    int field = 1;
    int room = 1;
    int i;

    func_00442100(spec, "field:%d:%d", &field, &room);
    p4gl_atomic_set_viewer_light(0);
    if (p4gl_context_open() != 0 || viewer_target_create() != 0) {
        fprintf(stderr, "p4pc_viewer: GL context\n");
        return 1;
    }
    kwlnInitSubsystems();
    fprintf(stderr, "p4pc_viewer: subsystems initialised\n");
    {
        /* The debug field sequence: FldRootCreate with no event reader, as K_SeqFieldRecordStart does. The field root
         * task then runs its own state machine (load task, unit install, script, camera) under kwlnRunFrame. */
        uint16_t *start = (uint16_t *)P4_G2H(p4_gport_alloc(16, 16)); /* the sequence reads its parameters from guest memory */
        const int frames = getenv("P4PC_FIELD_FRAMES") != NULL ? func_0043c6b0(getenv("P4PC_FIELD_FRAMES")) : 600;

        start[0] = (uint16_t)field;
        start[1] = (uint16_t)room;

        /* Let the real boot chain finish before taking over. Manually starting
         * its callbacks and waiting 600 frames left the normal boot running too:
         * on slow hosts its logo/movie/title could draw over the field. Sequence
         * 27, with no delayed start pending, means its tables are ready. */
        {
            uint32_t sequenceWork;
            void *logo;

            for (;;) {
                const uint32_t task = P4_GPDATA(uint32_t, 0xb1a8);

                sequenceWork = task != 0 ? P4_G32(task + 0x38u) : 0;
                if (sequenceWork != 0 && P4_G32(sequenceWork + 4u) == 27u &&
                    (P4_G32(sequenceWork) & 1u) == 0u) {
                    break;
                }
                kwlnRunFrame();
                p4_os_usleep(1000);
            }
            /* As in the menu/battle harness, the logo is a separate root task,
             * not a child stopped by gameSequenceStop. Stop both at takeover. */
            logo = kwlnTaskGetTaskByName(P4_GARRAY(char, 0x005E57A8u));
            if (logo != NULL) {
                kwlnTaskDestroyWithHierarchy(logo);
            }
            gameSequenceStop();
            while ((int32_t)P4_G32(sequenceWork + 4u) != -1) {
                kwlnRunFrame();
                p4_os_usleep(1000);
            }
        }
        datInitDefaultGameState();
        if (getenv("P4PC_FIELD_DATE") != NULL || getenv("P4PC_FIELD_SLOT") != NULL) {
            /* The date (days since April 1) and the time slot (0 early morning .. 5 evening) the field environment (sky, light
             * colours, weather) is built from; the calendar task latches them at its state 1, so it is sent back there. */
            extern void datSetDateIndex(int16_t index);
            extern void datSetTimeSlot(uint8_t slot);
            uint8_t *iGpffffb1dc = (uint8_t *)P4_G2H(P4_GPDATA(uint32_t, 0xb1dc)); /* retail data word holding a guest address */

            if (getenv("P4PC_FIELD_DATE") != NULL) {
                datSetDateIndex((int16_t)func_0043c6b0(getenv("P4PC_FIELD_DATE")));
            }
            if (getenv("P4PC_FIELD_SLOT") != NULL) {
                datSetTimeSlot((uint8_t)func_0043c6b0(getenv("P4PC_FIELD_SLOT")));
            }
            if (iGpffffb1dc != NULL) {
                *(int32_t *)iGpffffb1dc = 1;
            }
        }
        /* Sequence 9 (FIELD RECORD, the sequence the PCSX2 reference runs in)
         * starts K_SeqFieldRecordStart after the boot sequence is quiescent. */
        if (getenv("P4PC_FIELD_STEP") != NULL) {
            /* P4PC_FIELD_STEP="index,param": a dungeon floor the way the game enters it: the step task (the sequence K_SeqFldStepStart
             * starts it, FldStepTaskCreate) loads the step table, whose entry `index` names the field and room, and creates the
             * field root itself with the retail flags (the savestates of the dungeon scenes have 6, 1023). The FIELD RECORD sequence
             * is not started: its field would be a second root, and without the step task FldStepIndex is 0, so every
             * FldUnit_RequestSceneResource entry whose stored value is 0 matches, past the two slots it has. */
            extern int32_t FldStepTaskCreate(int32_t parent, int32_t index, int32_t param);
            int index = 0;
            int param = 0;

            if (func_00442100(getenv("P4PC_FIELD_STEP"), "%d,%d", &index, &param) >= 1) {
                *(int32_t *)P4_G2H(0x00764334u) = FldStepTaskCreate(0, index, param);
            }
        } else
        sequenceRequest(9, P4_H2G(start), 0, 0);
        for (i = 0; i < frames; i++) {
            drawWindow(i, frames);
            uint32_t handle = *(uint32_t *)P4_G2H(0x007D2510u);
            uint32_t work = handle != 0 ? *(uint32_t *)P4_G2H(handle + 0x38) : 0;

            if (getenv("P4PC_FIELD_TASKS") != NULL && i == frames - 2) {
                kwlnTaskDumpAll(); /* P4PC_FIELD_TASKS=1: the task tree two frames before the end, to see what a held field waits on */
            }
            if (getenv("P4PC_FIELD_WEATHER") != NULL) {
                /* P4PC_FIELD_WEATHER=n: the weather value of the current day and period (the calendar state's word at +8,
                 * calendarPushValue; the day table's own value is used while it is -1). The day's weather is rolled and
                 * stored at run time, so a reference frame's value is read from its savestate (corpus: 0 clear, 1 rain,
                 * 2 cloudy). Written every frame: the calendar task resets it when it relatches the date. */
                uint8_t *iGpffffb1dc = (uint8_t *)P4_G2H(P4_GPDATA(uint32_t, 0xb1dc));

                if (iGpffffb1dc != NULL) {
                    *(int16_t *)(iGpffffb1dc + 8) = (int16_t)func_0043c6b0(getenv("P4PC_FIELD_WEATHER"));
                }
            }
            if (field >= 0x28 && field < 0x3C && room == 0) {
                /* A dungeon floor (band A): the resource queue takes the floor's parts from the table the previous floor left
                 * (D_007E8060) unless the work object's flag word has bit 31, which the retail load task sets right before
                 * fieldSceneLoad (k_fldEvent.c, state 13) so that the parts are created from the pack. Alone the debug
                 * sequence leaves it clear and the table is empty. */
                *(uint32_t *)P4_G2H(*(uint32_t *)P4_G2H(0x00762EA0u) + 0x88u) |= 0x80000000u;
            }
            if (getenv("P4PC_FIELD_PLAYER") != NULL && i >= frames - 12) {
                /* P4PC_FIELD_PLAYER="x,y,z": the protagonist's world position (the viewer spawns it at the field's entry 0). The
                 * player's frame task is the one the field camera controller follows: its work block's word 0xA2 is the
                 * player's resource, whose +0x220 is the task FldFrameTask_GetPosition reads. */
                extern void FldFrameTask_SetPosition(uint8_t *task, uint8_t *position);
                float pos[3] = {0, 0, 0};
                const uint32_t cameraTask = *(uint32_t *)P4_G2H(0x007D2514u);

                if (func_00442100(getenv("P4PC_FIELD_PLAYER"), "%f,%f,%f", &pos[0], &pos[1], &pos[2]) == 3 && cameraTask != 0) {
                    const uint32_t work = *(uint32_t *)P4_G2H(cameraTask + 0x38);
                    const uint32_t res = work != 0 ? *(uint32_t *)P4_G2H(work + 0x288) : 0;
                    const uint32_t task = res != 0 ? *(uint32_t *)P4_G2H(res + 0x220) : 0;

                    if (task != 0) {
                        FldFrameTask_SetPosition((uint8_t *)P4_G2H(task), (uint8_t *)pos);
                    }
                }
            }
            if (getenv("P4PC_FIELD_CAMERA") != NULL && i >= frames - 4) {
                /* P4PC_FIELD_CAMERA="right(3),pad,up(3),pad,at(3),pad,pos(3),pad, viewWindow x y, near, far": the camera of a
                 * reference frame (a PCSX2 savestate's camera words). The field camera controller is stopped and the camera
                 * frame, view window and planes are written before each frame's draw. */
                extern void *MT_Scene_GetFrameState(void);
                float v[20];
                const char *s = getenv("P4PC_FIELD_CAMERA");
                int k;

                for (k = 0; k < 20; k++) {
                    int consumed = 0;
                    v[k] = 0.0f;
                    (void)func_00442100(s, "%f%n", &v[k], &consumed);
                    s += consumed;
                    if (*s == ',') {
                        s++;
                    }
                }
                if (i == frames - 4) {
                    uint8_t *frameState = MT_Scene_GetFrameState();
                    void *task = P4_G2H(*(uint32_t *)(frameState + 4));

                    if (task != NULL) {
                        kwlnTaskDestroyWithHierarchy(task);
                        *(int32_t *)(frameState + 4) = 0;
                    }
                }
                {
                    uint8_t *cam = (uint8_t *)P4_G2H(kwlnGetCamera());
                    uint8_t *frame = (uint8_t *)P4_G2H(*(uint32_t *)(cam + 4));

                    memcpy(frame + 16, v, 64);  /* RwFrame.modelling */
                    memcpy(frame + 80, v, 64);  /* RwFrame.ltm */
                    memcpy(cam + 104, v + 16, 8);
                    memcpy(cam + 128, v + 18, 8);
                }
            }
            if (i == frames - 60) {
                /* The field script opens its tutorial message window ("You can save the game by using the calendar...")
                 * a few seconds in; the PCSX2 reference frame was captured with it dismissed. The open message windows
                 * are reset the way evtMessageWindowReset does (state 0, release, clear, state 1). */
                extern void itfMesWindowSetState(int32_t window, int8_t state);
                extern void itfMesWindowRelease(int32_t window);
                extern void itfMesWindowClear(int32_t window, int32_t arg);
                extern void itfMesWindowClearFlagBits(int32_t window, int32_t bits);
                int32_t w;

                for (w = 0; w < 16; w++) {
                    const uint32_t obj = *(uint32_t *)P4_G2H(0x00881808u + (uint32_t)w * 32u);

                    if (obj != 0 && *(uint32_t *)P4_G2H(obj) != 0) {
                        itfMesWindowSetState(w, 0);
                        itfMesWindowRelease(w);
                        itfMesWindowClear(w, 0);
                        itfMesWindowClearFlagBits(w, 0x800000);
                        itfMesWindowClearFlagBits(w, 0x100000);
                        itfMesWindowSetState(w, 1);
                    }
                }
            }
            kwlnRunFrame();
            if (i == 100) {
                /* The field script's "calendar on" command (k_command_00299e60) runs after the field has loaded and shows
                 * the date window and the time-of-day panel: the viewer does not run the script. */
                extern void calendarSetMode(int32_t mode);

                calendarSetMode(1);
            }
            if (i % 25 == 0) {
                fprintf(stderr, "p4pc_viewer: frame %d field state %d\n", i, work != 0 ? *(int32_t *)P4_G2H(work + 0x0) : -1);
            }
            p4_os_usleep(1000);
            p4pc_profile_flip((unsigned)i + 1u); /* P4PC_PROFILE / P4PC_PROFILE_EVERY, as in the game */
        }
    }
    if (getenv("P4PC_DUMP_TASKS") != NULL) {
        int ty;

        kwlnTaskDumpAll();
        for (ty = 0; ty < 0x20; ty++) {
            const uint8_t *res = MT_Scene_GetResListHead(ty);
            int n = 0;

            while (res != NULL && n < 1000) {
                res = ptr_at(res, 0x138);
                n++;
            }
            fprintf(stderr, "p4pc_viewer: scene resources type %#x: %d\n", ty, n);
        }
    }
    if (write_frame(pngPath) != 0) {
        fprintf(stderr, "p4pc_viewer: readback or PNG write failed\n");
        return 1;
    }
    return 0;
}


/* World mode (rmd path "world:<pack>#<entry>", e.g. world:field/pack/f007_002.p01#f007_002.rws): reads the
 * RenderWare stream of a field pack entry (an optional texture dictionary, then the RpWorld) and renders the
 * world through RpWorldRender from a fixed camera (P4PC_WORLD_CAM="x y z  rx ry rz  ux uy uz  ax ay az fov near
 * far"; the default is the Dojima living room camera of the PCSX2 reference). */
void *RwStreamOpen(int type, int access, void *data);
int RwStreamClose(void *stream, void *data);
uint32_t RwStreamFindChunk(void *stream, uint32_t type, uint32_t *length, uint32_t *version);   /* RwBool */
void *RpClumpStreamRead(void *stream);
void *RpClumpRender(void *clump);
void *RwTexDictionaryStreamRead(void *stream);
void *RwTexDictionarySetCurrent(void *dict);

static uint8_t *load_pack_entry(const char *pack, const char *name, uint32_t *size)
{
    char path[256];
    long long total = 0;
    int isDir = 0;
    uint8_t *file;
    int fd;
    size_t off;
    size_t i;

    snprintf(path, sizeof path, "/%s", pack);
    for (i = 0; path[i] != '\0'; i++) {
        if (path[i] >= 'a' && path[i] <= 'z') {
            path[i] = (char)(path[i] - 'a' + 'A');
        }
    }
    if (vfs_stat(path, &isDir, &total) != 0 || total <= 0 || (fd = vfs_open(path, 0)) < 0) {
        return NULL;
    }
    file = p4pc_heap_malloc((size_t)total);       /* the stream reads it as guest memory */
    if (file == NULL || vfs_read(fd, file, (int)total) != (int)total) {
        p4pc_heap_free(file);
        vfs_close(fd);
        return NULL;
    }
    vfs_close(fd);
    for (off = 0; off + 0x100 <= (size_t)total; off += 0x80) {
        if (strncmp((const char *)file + off, name, 0x100) == 0) {
            *size = (uint32_t)((size_t)total - off - 0x100);
            return file + off + 0x100;
        }
    }
    p4pc_heap_free(file);
    return NULL;
}

static int render_world(const char *spec, const char *pngPath) {
    char pack[160];
    const char *hash = strchr(spec + 6, '#');
    uint32_t size = 0;
    uint8_t *data;
    struct { uint32_t start; uint32_t length; } *memory = p4pc_heap_calloc(1, 8);     /* RwMemory: a guest address and a length */
    void *stream;
    void *world = NULL;
    void *clumps[16];
    int nClumps = 0;
    void *camera;
    float cam[15] = {-317.4375f, 244.9866f, 421.0957f, -0.67193f, 0.0f, -0.74061f, 0.17129f, 0.97289f, -0.15541f,
                     0.72053f, -0.23129f, -0.65371f, 54.2f, 100.0f, 7000.0f};
    uint32_t type = 0;
    uint32_t length = 0;
    uint32_t version = 0;

    if (hash == NULL) {
        return 2;
    }
    snprintf(pack, sizeof pack, "%.*s", (int)(hash - (spec + 6)), spec + 6);
    data = load_pack_entry(pack, hash + 1, &size);
    if (data == NULL) {
        fprintf(stderr, "p4pc_viewer: %s is not in %s\n", hash + 1, pack);
        return 1;
    }
    if (getenv("P4PC_WORLD_CAM") != NULL) {
        func_00442100(getenv("P4PC_WORLD_CAM"), "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f", &cam[0], &cam[1], &cam[2],
                     &cam[3], &cam[4], &cam[5], &cam[6], &cam[7], &cam[8], &cam[9], &cam[10], &cam[11], &cam[12], &cam[13],
                     &cam[14]);
    }
    p4gl_atomic_set_viewer_light(0);
    if (p4gl_context_open() != 0 || viewer_target_create() != 0) {
        fprintf(stderr, "p4pc_viewer: GL context\n");
        return 1;
    }
    memory->start = P4_H2G(data);
    memory->length = size;
    stream = RwStreamOpen(3, 1, memory);
    if (stream == NULL) {
        fprintf(stderr, "p4pc_viewer: RwStreamOpen\n");
        return 1;
    }
    if (RwStreamFindChunk(stream, 0x16, &length, &version) != 0) {
        void *dict = RwTexDictionaryStreamRead(stream);

        fprintf(stderr, "p4pc_viewer: texture dictionary %p\n", dict);
        RwTexDictionarySetCurrent(dict);
    }
    /* A field's .rws holds clumps after its texture dictionary (the map and its props), not an RpWorld. */
    while (nClumps < 16 && RwStreamFindChunk(stream, 0x10, &length, &version) != 0) {
        clumps[nClumps] = RpClumpStreamRead(stream);
        fprintf(stderr, "p4pc_viewer: clump %d %p (%u bytes)\n", nClumps, clumps[nClumps], (unsigned)length);
        if (clumps[nClumps] != NULL) {
            Bounds b = {{0}, {0}, 0};

            RpClumpForAllAtomics(clumps[nClumps], bounds_atomic, &b);
            fprintf(stderr, "p4pc_viewer: clump %d bounds (%g %g %g) .. (%g %g %g), %d atomics\n", nClumps, (double)b.lo[0],
                    (double)b.lo[1], (double)b.lo[2], (double)b.hi[0], (double)b.hi[1], (double)b.hi[2], b.count);
            nClumps++;
        }
    }
    RwStreamClose(stream, NULL);
    (void)world;
    if (nClumps == 0) {
        return 1;
    }
    camera = kwlnCameraCreate(VIEW_W, VIEW_H, 1);
    if (camera == NULL) {
        return 1;
    }
    {
        float m[16] = {0};
        void *frame = (void *)ptr_at(camera, 4);

        m[0] = cam[3]; m[1] = cam[4]; m[2] = cam[5];
        m[4] = cam[6]; m[5] = cam[7]; m[6] = cam[8];
        m[8] = cam[9]; m[9] = cam[10]; m[10] = cam[11];
        ((uint32_t *)m)[3] = 3;
        func_003e9cb0(frame, m, 0);
        func_003e9c10(frame, cam, 2);
        K_View_SetFov(camera, cam[12]);
        func_003e8180(camera, cam[13]);
        func_003e81c0(camera, cam[14]);
    }
    p4gl_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    p4gl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (RwCameraBeginUpdate(camera) == NULL) {
        return 1;
    }
    {
        int k;

        for (k = 0; k < nClumps; k++) {
            RpClumpRender(clumps[k]);
        }
    }
    RwCameraEndUpdate(camera);
    if (write_frame(pngPath) != 0) {
        return 1;
    }
    return 0;
}

static int g_image_loaded;   /* the 64-bit main loads the image before it carves the stack out of the block */

static int viewer_main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "SLUS_217.82";
    const char *rmd = argc > 2 ? argv[2] : NULL;
    const char *png = argc > 3 ? argv[3] : "p4pc_viewer.png";

    if (rmd == NULL) {
        fprintf(stderr, "usage: p4pc_viewer [SLUS_217.82 [RMD [out.png]]]\n");
        return 2;
    }
    if (p4_content_init()!=0) return 1;
    const int titleMode = strcmp(rmd, "title") == 0;
    const int fieldMode = strncmp(rmd, "field:", 6) == 0;
    const int worldMode = strncmp(rmd, "world:", 6) == 0;

    /* Unported work is reported and skipped, so the run lists what is left. */
    p4_os_setenv("P4PC_TRAP_CONTINUE", "1", 0);
    p4pc_trap_configure();

    if (!g_image_loaded) {
        p4pc_load_image(path);
    }
    p4pc_apply_patches();
    p4pc_kernel_pre_init();
    /* kwlnBoot creates the boot heap (kwlnProbeHeapBudget) before anything allocates. */
    kwlnProbeHeapBudget();

    kwlnInitIop();
    kwlnInitEngine();
    /* Preserve worker/pool allocation order, including field viewers' preflight. */
    hsfdInitWorker();
    func_00421670(1, 0x40);   /* boot thread (id 1) below the load worker (0x1C) */
    fprintf(stderr, "p4pc_viewer: engine initialised\n");

    const int failed = titleMode ? render_title(png) != 0
        : fieldMode ? render_field(rmd, png) != 0
        : worldMode ? render_world(rmd, png) != 0 : render_rmd(rmd, png) != 0;
    {
        /* P4PC_DUMP_RAM=<file>: the PS2 RAM (0x00100000..0x02000000) after the render, to diff two builds' game state word by word (the first
         * differing word of a frame that differs names the code that computed it; tools/ram_diff.py). Host pointers in it differ by build. */
        const char *dump = getenv("P4PC_DUMP_RAM");
        FILE *df = dump != NULL ? fopen(dump, "wb") : NULL;

        if (df != NULL) {
            fwrite(P4_G2H(0x00100000u), 1, 0x02000000u - 0x00100000u, df);
            fclose(df);
        }
    }
    if (failed) {
        return 1;
    }
    fprintf(stderr, "p4pc_viewer: wrote %s\n", png);
    return 0;
}

#ifdef P4_GUEST64
/* 64-bit host: the engine runs on a stack in guest memory, so the address of a local (the matrices and records handed to the mirror as pointers
 * or m2c integers) is a guest address, as src/crt.c does for the game. The process stack only hosts main() until the switch. */
#define VIEWER_STACK_SIZE (4u << 20)
static int g_argc;
static char **g_argv;

static void viewer_entry(void) {
    int status=viewer_main(g_argc,g_argv);
    if (p4_content_finish()!=0 && status==0) status=1;
    exit(status);
}

int main(int argc, char **argv) {
    if (p4_os_restart_init(argc, argv) != 0) return 1;
    void *stack;
    p4_coro *boot, *run;

    g_argc = argc;
    g_argv = argv;
    p4_os_setenv("P4PC_TRAP_CONTINUE", "1", 0);
    p4pc_trap_configure();
    p4pc_load_image(argc > 1 ? argv[1] : "SLUS_217.82");
    g_image_loaded = 1;
    stack = p4_guest_stack_alloc(VIEWER_STACK_SIZE);
    if (stack == NULL) {
        fprintf(stderr, "p4pc_viewer: no guest memory for the stack\n");
        return 2;
    }
    boot = p4_coro_adopt();
    run = p4_coro_create(stack, VIEWER_STACK_SIZE, viewer_entry);
    p4_coro_switch(boot, run);
    return 0;
}
#else
int main(int argc, char **argv) {
    if (p4_os_restart_init(argc, argv) != 0) return 1;
    int status=viewer_main(argc,argv);
    if (p4_content_finish()!=0 && status==0) status=1;
    return status;
}
#endif
