/* OpenGL 3.3 core or OpenGL ES 3.0 context (p4gl_api.h) on the SDL window owned by src/platform/pad/pad_sdl.c. The
 * window is shared: the pad layer creates it, this file adds the GL context. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "../platform/config/p4cfg.h"
#include "../platform/pad/pad_sdl.h"
#include "p4pc/os.h"
#include "p4pc/guest.h"
#include "p4pc/capture_state.h"
#include "p4pc/lockstep.h"
#include "p4pc/pace.h"
#include "p4pc/settings.h"
#include "p4pc/settings_native.h"
#include "p4pc/ps2_libc.h"
#include "p4gl.h"
#include "p4gl_api.h"
#include "p4gl_win.h"
#include "p4gl_gl.h"
#include "p4gl_profile.h"
#include "p4gl_readback.h"
#include "../viewer/p4png.h"

static P4GlContext sContext;
/* Set once the battle simulator's no-device backend has replaced the GL entry points (p4gl_context_open). */
static int sNoDevice;

int p4gl_context_is_current(void)
{
    return sContext && p4gl_win_current_context() == sContext;
}
int p4gl_device_ready(void)
{
    return sNoDevice || p4gl_context_is_current();
}
/* P4PC_OFFSCREEN=1: draw into an offscreen framebuffer object instead of the window and never swap. SwapBuffers on a window costs about
 * half a frame on a real GPU (0.6 of 1.3 ms measured on an RTX 4080, Windows) and an unattended run (a gate runner sets this)
 * has nobody looking at it. The same RGBA8 colour and 24-bit depth as the window's framebuffer, and a captured frame is read from it,
 * but not bit for bit the same picture: on llvmpipe 176 edge pixels of the 640x448 new-game gate frame differ (0.0018% MAD, one
 * deterministic set), so the gates, which compare against references made with the window, leave it off. */
static int sOffscreen;
/* video.present = blit (P4PC_PRESENT=blit): draw into an exact-pixel RGBA8 target
 * resolved from video.resolution, then fit it to the window. The window's framebuffer is only written, never read or relied on to
 * keep its contents across a swap: what EGL on Android, ANGLE (flip-model D3D11 swap chains, Metal drawables) and a high-DPI window do not
 * promise. Frames are as in P4PC_OFFSCREEN (same RGBA8 target, captures read it). The window's aspect is the front end's business. */
static int sPresentBlit;
/* video.fit (P4PC_PRESENT_FIT): P4CFG_FIT_4_3 keeps the game's 4:3 picture in the blit with black bars around it (the default on Android, whose window is the
 * phone's whole screen), P4CFG_FIT_INTEGER the largest whole multiple of 640x480 that fits, P4CFG_FIT_STRETCH fills the window (the default elsewhere: the
 * desktop front end sizes the window). */
static int sPresentFit;
static int sAspectMode; /* aspect changes take effect on restart, unlike window resizing */
/* P4PC_TEST_CONTEXT_LOSS=N: replace the GL context after the Nth presented frame, as if the system had taken it (p4gl_context_recover): the
 * test of everything that rebuilds after a loss. */
static unsigned sTestLossFrame;

/* The display's refresh rate under the window in Hz, 0 when SDL does not know (no display: SDL's offscreen driver). */
static float displayRefresh(SDL_Window *window)
{
    const SDL_DisplayID display = window != NULL ? SDL_GetDisplayForWindow(window) : 0;
    const SDL_DisplayMode *mode = display != 0 ? SDL_GetCurrentDisplayMode(display) : NULL;

    return mode != NULL ? mode->refresh_rate : 0.0f;
}

static void setSwapInterval(void)
{
    /* Vsync is best effort: some hosts refuse it, and the game does not depend on it: its 60 Hz tick is the host clock's, not the display's
     * (p4pc_vsync_wait), so a swap that waits for the display's blank only holds the frame until it, and the next tick still comes at its
     * own time. An unattended run (P4PC_TURBO, P4PC_VIRTUAL_CLOCK) is not paced to the display at all. video.vsync = auto waits for the
     * blank unless the display is known to run below 59 Hz, where the wait would be longer than a tick and slow the game. */
    const char *turbo = getenv("P4PC_TURBO");
    const char *virt = getenv("P4PC_VIRTUAL_CLOCK");
    int on = 1;

    if ((turbo != NULL && turbo[0] != '\0' && turbo[0] != '0') || (virt != NULL && virt[0] == '1')) {
        on = 0;
    } else if (p4cfg_video()->vsync == 0) {
        on = 0;
    } else if (p4cfg_video()->vsync == 2) {
        const float hz = displayRefresh(p4pc_pad_sdl_window());

        if (hz > 0.0f && hz < 59.0f) {
            on = 0;
            fprintf(stderr, "p4gl: the display runs at %.1f Hz: not waiting for its blank (video.vsync = on forces it), the game is paced by the clock\n", (double)hz);
        }
    }
    p4gl_win_set_swap_interval(on);
}

static void fitRect(int ww, int wh, int fit, GLint *dx, GLint *dy, GLint *dw, GLint *dh);
static int applySettings(const P4CfgVideo *video, const char *resolution);

/* Aspect controls projection/layout, never the physical storage dimensions. */
static void updateAspect(SDL_Window *window)
{
    const int mode = sAspectMode;
    float aspect = 4.0f / 3.0f;

    if (mode == P4CFG_ASPECT_16_9) {
        aspect = 16.0f / 9.0f;
    } else if (mode == P4CFG_ASPECT_AUTO) {
        int w = 0, h = 0;
        if (!SDL_GetWindowSizeInPixels(window, &w, &h) || w <= 0 || h <= 0) return;
        aspect = (float)w / (float)h;
    }
    p4gl_aspect_set(mode == P4CFG_ASPECT_4_3 ? 1.0f : aspect * 0.75f);
}

static int renderSize(SDL_Window *window, const P4CfgVideo *video, uint32_t *w, uint32_t *h)
{
    updateAspect(window);
    if (video->resolution == P4CFG_RESOLUTION_NATIVE) {
        int ww = 0, wh = 0;
        GLint x, y, width, height;
        if (!SDL_GetWindowSizeInPixels(window, &ww, &wh) || ww <= 0 || wh <= 0) return 0;
        fitRect(ww, wh, video->fit, &x, &y, &width, &height);
        *w = (uint32_t)width;
        *h = (uint32_t)height;
    } else if (video->resolution == P4CFG_RESOLUTION_EXPLICIT) {
        *w = (uint32_t)video->renderWidth;
        *h = (uint32_t)video->renderHeight;
    } else {
        *w = p4gl_aspect_native_width() * (unsigned)video->scale;
        *h = 448u * (unsigned)video->scale;
    }
    return *w > 0 && *h > 0;
}

static void resolutionFormat(char *out, size_t capacity, void *unused)
{
    const P4CfgVideo *video = p4cfg_video();
    (void)unused;
    if (video->resolution == P4CFG_RESOLUTION_NATIVE) p4pc_ee_snprintf(out, capacity, "native");
    else if (video->resolution == P4CFG_RESOLUTION_EXPLICIT)
        p4pc_ee_snprintf(out, capacity, "%dx%d", video->renderWidth, video->renderHeight);
    else p4pc_ee_snprintf(out, capacity, "scale:%d", video->scale);
}

static void resolutionAdjust(int direction, void *unused)
{
    static const char *const choices[] = {"scale:1", "scale:2", "scale:3", "scale:4",
        "scale:5", "scale:6", "scale:7", "scale:8", "native", "1366x768",
        "1920x1080", "2560x1440", "3840x2160"};
    const int count = (int)(sizeof choices / sizeof choices[0]);
    char current[48];
    P4CfgVideo candidate;
    int selected = 0;
    resolutionFormat(current, sizeof current, unused);
    for (int i = 0; i < count; i++) if (strcmp(current, choices[i]) == 0) selected = i;
    selected = (selected + (direction < 0 ? -1 : 1) + count) % count;
    p4cfg_video_preview_resolution(choices[selected], &candidate);
    if (applySettings(&candidate, choices[selected]) != 0)
        p4_settings_message("Resolution unchanged: could not apply or save");
}

static void registerResolutionSetting(void)
{
    static int registered;
    static const P4SettingsEntry entry = {
        .label = "Resolution", .format = resolutionFormat, .adjust = resolutionAdjust,
        .key = "video.resolution", .description = "Choose render size."
    };
    if (!registered) registered = p4_settings_register(&entry) == 0;
}

static void aspectFormat(char *out, size_t size, void *unused)
{
    static const char *const names[] = {"4:3", "16:9", "auto"};
    (void)unused;
    snprintf(out, size, "%s", names[p4cfg_video()->aspect]);
}

static void aspectAdjust(int direction, void *unused)
{
    static const char *const names[] = {"4:3", "16:9", "auto"};
    int mode = p4cfg_video()->aspect;
    (void)unused;
    mode = (mode + (direction < 0 ? 2 : 1)) % 3;
    if (p4cfg_set_str("video.aspect", names[mode]) != 0) {
        p4_settings_message("Could not save aspect setting");
    }
}

static void registerAspectSetting(void)
{
    static int registered;
    static const P4SettingsEntry entry = {
        .label = "Aspect ratio", .format = aspectFormat, .adjust = aspectAdjust,
        .restart = 1, .key = "video.aspect", .description = "Choose field/battle framing."
    };
    if (!registered) {
        registered = p4_settings_register(&entry) == 0;
    }
}

static void movieFitFormat(char *out, size_t size, void *unused)
{
    static const char *const names[] = {"bars", "zoom", "stretch"};
    (void)unused;
    snprintf(out, size, "%s", names[p4cfg_video()->movieFit]);
}

static void movieFitAdjust(int direction, void *unused)
{
    static const char *const names[] = {"bars", "zoom", "stretch"};
    int mode = p4cfg_video()->movieFit;
    (void)unused;
    mode = (mode + (direction < 0 ? 2 : 1)) % 3;
    if (p4cfg_set_str("video.movie_fit", names[mode]) != 0)
        p4_settings_message("Could not save movie fit setting");
}

static void registerMovieFitSetting(void)
{
    static int registered;
    static const P4SettingsEntry entry = {
        .label = "Movie fit", .format = movieFitFormat, .adjust = movieFitAdjust,
        .key = "video.movie_fit", .description = "Movie picture: bars, zoom or stretch."
    };
    if (!registered)
        registered = p4_settings_register(&entry) == 0;
}

extern int p4_battle_sim_renderless;
extern int p4_battle_sim_graphics_open(void);

int p4gl_context_open(void)
{
    SDL_Window *window;
    if (p4_battle_sim_renderless) {
        if (p4pc_pad_sdl_init() != 0) return -1;
        const P4CfgVideo *video = p4cfg_video();
        const int nativeSize = (video->resolution == P4CFG_RESOLUTION_SCALE && video->scale == 1) ||
            (video->resolution == P4CFG_RESOLUTION_EXPLICIT && video->renderWidth == 640 && video->renderHeight == 448);
        if (video->aspect != P4CFG_ASPECT_4_3 || !nativeSize) {
            fprintf(stderr, "p4gl: no-device battle renderer requires4:3 at640x448 (scale:1 or640x448)\n");
            return -1;
        }
        updateAspect(NULL);
        if (p4_battle_sim_graphics_open() != 0) return -1;
        sNoDevice = 1;
        p4pc_fpu_check("the no-device backend's startup");
        p4gl_frame_rasters_open();
        return 0;
    }

    if (sContext != NULL) {
        return 0;
    }
    if (p4pc_pad_sdl_init() != 0) {
        fprintf(stderr, "p4gl: SDL init for the GL window failed: %s\n", SDL_GetError());
        return -1;
    }
    window = p4pc_pad_sdl_window();
    if (window == NULL) {
        fprintf(stderr, "p4gl: no window for the GL context\n");
        return -1;
    }
    p4gl_api_set_attributes();
    sContext = p4gl_win_create_context(window);
    if (sContext == NULL && p4gl_api_wants_es() && !p4gl_api_explicit()) {
        /* The build's default is ES but this machine has none (a Windows driver without the ES profile, no EGL library): the desktop
         * context serves the same renderer. A run that asked for ES (P4PC_GL_API, P4PC_ANGLE) fails instead. */
        fprintf(stderr, "p4gl: SDL_GL_CreateContext (OpenGL ES 3.0) failed: %s; trying desktop OpenGL 3.3 core\n", p4gl_win_error());
        SDL_GL_ResetAttributes();
        p4gl_api_set_attributes_desktop();
        sContext = p4gl_win_create_context(window);
    }
    if (sContext == NULL) {
        fprintf(stderr, "p4gl: SDL_GL_CreateContext (%s) failed: %s\n", p4gl_api_wants_es() ? "OpenGL ES 3.0" : "3.3 core", p4gl_win_error());
        return -1;
    }
    if (!p4gl_win_make_current(window, sContext)) {
        fprintf(stderr, "p4gl: SDL_GL_MakeCurrent failed: %s\n", p4gl_win_error());
        p4gl_win_destroy_context(sContext);
        sContext = NULL;
        return -1;
    }
    if (p4gl_gl_load() != 0) {
        p4gl_win_destroy_context(sContext);
        sContext = NULL;
        return -1;
    }
    p4pc_fpu_check("the GL driver's first calls");
    setSwapInterval();
    {
        const char *want = getenv("P4PC_OFFSCREEN");
        const P4CfgVideo *video = p4cfg_video();
        uint32_t w = 0, h = 0;

        sPresentBlit = video->blit;
        sPresentFit = video->fit;
        sAspectMode = video->aspect;

        sTestLossFrame = getenv("P4PC_TEST_CONTEXT_LOSS") != NULL ? (unsigned)atoi(getenv("P4PC_TEST_CONTEXT_LOSS")) : 0u;
        updateAspect(window);
        if (((want != NULL && want[0] == '1') || sPresentBlit) && p4gl_readback_framebuffer() == 0) {
            if (!renderSize(window, p4cfg_video(), &w, &h) || p4gl_readback_target_create(w, h, 640, 448) != 0) return -1;
            sOffscreen = 1;
            fprintf(stderr, "p4gl: rendering %ux%u pixels in virtual640x448 space\n", w, h);
        }
    }
    registerAspectSetting();
    registerMovieFitSetting();
    p4gl_frame_rasters_open();
    registerResolutionSetting();
    p4pc_pad_set_settings_hooks(p4_settings_event, p4_settings_model_capturing, p4_settings_active, p4_settings_pad);
    return 0;
}

/* The game's start-up does not stop when the device fails to open: it went on and jumped to address 0 (an access violation, e.g. on
 * Windows with SDL's offscreen driver, which has no GL). Nothing can be drawn without a context, so the run ends here, after the p4gl
 * line that names the reason. */
void p4gl_context_open_device(void)
{
    if (p4gl_context_open() == 0) {
        return;
    }
    fprintf(stderr, "p4pc: no OpenGL context, the game cannot start\n");
    if (p4cfg_player_run()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Persona 4",
                                 "Persona 4 could not create an OpenGL context. Update the graphics driver; the log names the reason.", NULL);
    }
    exit(1);
}

void p4gl_context_close(void)
{
    sNoDevice = 0;
    if (sContext != NULL) {
        p4gl_profile_close();
        p4gl_win_destroy_context(sContext);
        sContext = NULL;
    }
}

/* The GL context is gone (the system took the GPU, EGL_CONTEXT_LOST at a swap, SDL's SDL_EVENT_RENDER_DEVICE_RESET after a resume) or a test
 * replaces it. A new context holds nothing: the entry points are resolved again, the state caches forget what they believed the
 * context held, every module drops its GL names and rebuilds on its next use, the rasters' textures are uploaded again from their CPU
 * shadows, and the present target is recreated (its picture is lost; the game redraws the whole frame the next frame). `adopt` is set when
 * SDL made the new context itself (SDL_EVENT_RENDER_DEVICE_RESET): then the current context is taken over instead of creating one. */
static int context_recover(int adopt)
{
    SDL_Window *window = p4pc_pad_sdl_window();
    uint32_t w = 0, h = 0;
    uint32_t virtualWidth = 0, virtualHeight = 0;

    if (window == NULL || sContext == NULL) {
        return -1;
    }
    p4gl_readback_pixel_size(&w, &h);
    p4gl_readback_virtual_size(&virtualWidth, &virtualHeight);
    p4gl_readback_context_lost();
    if (adopt) {
        sContext = p4gl_win_current_context();
    } else {
        p4gl_win_make_current(window, NULL);
        p4gl_win_destroy_context(sContext);
        sContext = p4gl_win_create_context(window);
        if (sContext == NULL || !p4gl_win_make_current(window, sContext)) {
            fprintf(stderr, "p4gl: cannot create a new GL context after the loss: %s\n", p4gl_win_error());
            sContext = NULL;
            return -1;
        }
    }
    if (sContext == NULL || p4gl_gl_load() != 0) {
        fprintf(stderr, "p4gl: the new GL context is unusable\n");
        sContext = NULL;
        return -1;
    }
    setSwapInterval();
    p4gl_gl_context_reset();
    if (w > 0 && h > 0 && p4gl_readback_target_create(w, h, virtualWidth, virtualHeight) != 0) {
        fprintf(stderr, "p4gl: cannot recreate the %ux%u present target\n", w, h);
        sContext = NULL;
        return -1;
    }
    p4gl_im2d_context_lost();
    p4gl_im3d_context_lost();
    p4gl_atomic_context_lost();
    p4gl_raster_context_lost();
    fprintf(stderr, "p4gl: GL context replaced%s; textures uploaded again\n", adopt ? " by SDL" : "");
    return 0;
}

/* P4PC_CAPTURE_FRAMES=a,b,c with P4PC_CAPTURE_PNG=path (a printf pattern with one %d, default
 * p4pc_frame_%d.png): writes the window's frame to a PNG just before the Nth presented frame is swapped
 * (counting from 1). This is how a boot run records what the game draws, headless, for comparison with
 * a PCSX2 frame. */
static int captureWanted(unsigned frame)
{
    static const char *list;
    static int read;
    const char *c;

    if (p4pc_capture_state_wanted(frame)) return 1;
    if (!read) {
        list = getenv("P4PC_CAPTURE_FRAMES");
        read = 1;
    }
    for (c = list; c != NULL && *c != '\0';) {
        char *end;
        unsigned long v = strtoul(c, &end, 10);
        if (end == c) {
            break;
        }
        if (v == frame) {
            return 1;
        }
        c = *end == ',' ? end + 1 : end;
        if (*end != ',') {
            break;
        }
    }
    return 0;
}

/* The fraction of pixels brighter than black (r + g + b > 30) in the last captured frame, -1 before any capture. The battle gate
 * (src/platform/battle_harness.c, P4PC_BATTLE_GATE) reads it to tell a drawn frame from a black one. */
static double sCaptureNonBlack = -1.0;
static double sCaptureMad = -1.0;
static int sCaptureWritten;   /* the requested PNG was written, not merely read back */

/* The mean absolute difference (percent, over RGB) of the last captured frame from the raw RGB reference P4PC_REF_RGB (the
 * same 640x448 file the viewer's gates use), -1 without one or before a capture. The new-game gate
 * (src/platform/newgame_gate.c) reads it. */
double p4gl_context_capture_mad(void)
{
    return sCaptureMad;
}

double p4gl_context_capture_nonblack(void)
{
    return sCaptureNonBlack;
}

int p4gl_context_capture_written(void)
{
    return sCaptureWritten;
}

static char sCaptureNextPath[512];
static char sCaptureWrittenPath[512];

/* Captures the next presented frame to `path` (a harness asks for a frame of a screen it just reached). */
void p4gl_context_capture_next(const char *path)
{
    snprintf(sCaptureNextPath, sizeof sCaptureNextPath, "%s", path);
    sCaptureWritten = 0;
}

/* Android has no writable cwd contract. Resolve relative capture names at the
 * capture seam, without changing the game's or a caller's working directory.
 * A requested PNG is evidence: a failed write must never produce a passing run. */
static void writeCapturePng(const char *path, uint32_t w, uint32_t h, const uint8_t *rgba)
{
#if defined(SDL_PLATFORM_ANDROID)
    char absolute[1024];
    if (path[0] != '/') {
        const char *base = SDL_GetAndroidInternalStoragePath();
        int n = base && *base ? snprintf(absolute, sizeof absolute, "%s/%s", base, path) : -1;
        if (n < 0 || (size_t)n >= sizeof absolute) {
            fprintf(stderr, "p4gl: cannot resolve app-writable capture path for %s\n", path);
            exit(1);
        }
        path = absolute;
    }
#endif
    if (p4png_write_rgba(path, w, h, rgba) != 0) {
        fprintf(stderr, "p4gl: writing %s failed\n", path);
        exit(1);
    }
#if defined(SDL_PLATFORM_ANDROID)
    fprintf(stderr, "p4gl: PNG written to %s\n", path);
#endif
}

/* Read the caller's selected framebuffer into opaque, top-down RGBA. One
 * image and one scratch row avoid a second full-resolution allocation. */
static uint8_t *capturePixels(int w, int h)
{
    const uint64_t rows = (uint64_t)h + (h > 1 ? 1u : 0u);
    uint8_t *pixels;
    GLenum error;
    if (w <= 0 || h <= 0) return NULL;
    if ((uint64_t)w * rows > SIZE_MAX / 4u) {
        fprintf(stderr, "p4gl: %dx%d exceeds addressable RGBA capture storage\n", w, h);
        return NULL;
    }
    const size_t rowBytes = (size_t)w * 4u;
    const size_t bytes = rowBytes * (size_t)h;
    pixels = malloc(rowBytes * (size_t)rows);
    if (pixels == NULL) {
        fprintf(stderr, "p4gl: cannot allocate %dx%d capture storage\n", w, h);
        return NULL;
    }
    p4gl_glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    error = p4gl_glGetError();
    if (error != GL_NO_ERROR) {
        fprintf(stderr, "p4gl: %dx%d capture read failed (GL error 0x%x)\n", w, h, (unsigned)error);
        free(pixels);
        return NULL;
    }
    uint8_t *const row = pixels + bytes;
    for (int y = 0; y < h / 2; y++) {
        uint8_t *a = pixels + (size_t)y * rowBytes;
        uint8_t *b = pixels + (size_t)(h - 1 - y) * rowBytes;
        memcpy(row, a, rowBytes);
        memcpy(a, b, rowBytes);
        memcpy(b, row, rowBytes);
    }
    /* GPU alpha is GS blend state, not PNG coverage. */
    for (size_t i = 3; i < bytes; i += 4) pixels[i] = 0xFF;
    return pixels;
}

static void captureFrame(SDL_Window *window, unsigned frame)
{
    int w = 0, h = 0;
    uint8_t *flipped;
    char path[512];
    const char *pattern = getenv("P4PC_CAPTURE_PNG");

    sCaptureWritten = 0;
    sCaptureNonBlack = -1.0;

    sCaptureWrittenPath[0] = '\0';
    if (sOffscreen) {
        uint32_t ow = 0, oh = 0;

        p4gl_readback_pixel_size(&ow, &oh);
        w = (int)ow;
        h = (int)oh;
    } else {
        SDL_GetWindowSizeInPixels(window, &w, &h);
    }
    if (w <= 0 || h <= 0) {
        return;
    }
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, sOffscreen ? p4gl_readback_framebuffer() : 0);
    flipped = capturePixels(w, h);
    if (flipped == NULL) return;
    {
        size_t n = (size_t)w * (size_t)h, lit = 0, i;

        for (i = 0; i < n; i++) {
            const uint8_t *px = flipped + i * 4u;

            if ((unsigned)px[0] + px[1] + px[2] > 30u) {
                lit++;
            }
        }
        sCaptureNonBlack = (double)lit / (double)n;
    }
    {
        /* P4PC_DUMP_RAM=<printf pattern with one %u>: the PS2 RAM (0x00100000..0x02000000) at each captured frame, to diff two builds' game state
         * word by word (tools/ram_diff.py; the first differing word of a frame that differs names the code that computed it). */
        const char *dump = getenv("P4PC_DUMP_RAM");

        if (dump != NULL) {
            char dpath[512];
            FILE *df;

            snprintf(dpath, sizeof dpath, dump, frame);
            p4pc_dump_ram_admit(dpath);
            df = fopen(dpath, "wb");
            if (df != NULL) {
                fwrite(P4_G2H(0x00100000u), 1, 0x02000000u - 0x00100000u, df);
                fclose(df);
            }
        }
    }
    {
        const char *refPath = getenv("P4PC_REF_RGB");
        sCaptureMad = -1.0;
        if (refPath != NULL) p4gl_reference_mad(flipped, (uint32_t)w, (uint32_t)h, refPath, &sCaptureMad);
    }
    {
        /* P4PC_CAPTURE_RGB=<path>: the captured frame as raw RGB (the reference format of the MAD gates; how refs/*.rgb are made). */
        const char *rgbPath = getenv("P4PC_CAPTURE_RGB");
        FILE *rf = rgbPath != NULL ? fopen(rgbPath, "wb") : NULL;

        if (rf != NULL) {
            size_t k;

            for (k = 0; k < (size_t)w * (size_t)h; k++) {
                fwrite(flipped + k * 4u, 1, 3, rf);
            }
            fclose(rf);
        }
    }
    if (sCaptureNextPath[0] != '\0') {
        snprintf(path, sizeof path, "%s", sCaptureNextPath);
        sCaptureNextPath[0] = '\0';
    } else {
        snprintf(path, sizeof path, pattern != NULL ? pattern : "p4pc_frame_%d.png", (int)frame);
    }
    writeCapturePng(path, (uint32_t)w, (uint32_t)h, flipped);
    snprintf(sCaptureWrittenPath, sizeof sCaptureWrittenPath, "%s", path);
    sCaptureWritten = 1;
    fprintf(stderr, "p4gl: captured frame %u (%dx%d) to %s\n", frame, w, h, path);
    {
        /* P4PC_CAPTURE_REF=file: a raw RGB reference at the captured target size (kept beside the retail image, never committed): the captured frame's
         * mean absolute difference from it is logged. With P4PC_CAPTURE_REF_MAX=percent the process then exits, 1 when the difference is above it, else 0:
         * the battle scene gate (tests/battle_mad.sh) takes its frame this way. */
        const char *refPath = getenv("P4PC_CAPTURE_REF");

        if (refPath != NULL) {
            double mad;
            const char *maxText = p4gl_gate_limit("P4PC_CAPTURE_REF_MAX");
            if (p4gl_reference_mad(flipped, (uint32_t)w, (uint32_t)h, refPath, &mad) != 0) {
                if (maxText != NULL) exit(1);
            } else {
                fprintf(stderr, "p4gl: frame %u mean abs diff from %s = %.4f%%\n", frame, refPath, mad);
                if (maxText != NULL) {
                    const double max = atof(maxText);
                    if (mad > max) fprintf(stderr, "p4gl: FAIL: above the %.1f%% limit\n", max);
                    fflush(stderr);
                    exit(mad > max ? 1 : 0);
                }

            }
        }
    }
    free(flipped);
}

/* P4PC_TEST_RESIZE=frame:action,... (actions: WxH, minimize, restore, fullscreen, windowed): at that presented frame the window is resized, minimised and so
 * on, as a player would, to test that the picture follows and the context survives (docs/platform/video_presentation.md "Present and window"). */
static void testWindowActions(SDL_Window *window, unsigned frame)
{
    static const char *list;
    static int read;
    const char *c;

    if (!read) {
        list = getenv("P4PC_TEST_RESIZE");
        read = 1;
    }
    for (c = list; c != NULL && *c != '\0';) {
        char *end;
        const unsigned long at = strtoul(c, &end, 10);
        int w = 0, h = 0;

        if (end == c || *end != ':') {
            break;
        }
        c = end + 1;
        if (at == frame) {
            if (strncmp(c, "minimize", 8) == 0) {
                SDL_MinimizeWindow(window);
            } else if (strncmp(c, "restore", 7) == 0) {
                SDL_RestoreWindow(window);
            } else if (strncmp(c, "fullscreen", 10) == 0) {
                SDL_SetWindowFullscreen(window, true);
            } else if (strncmp(c, "windowed", 8) == 0) {
                SDL_SetWindowFullscreen(window, false);
            } else if (sscanf(c, "%dx%d", &w, &h) == 2 && w > 0 && h > 0) {
                SDL_SetWindowSize(window, w, h);
            }
            fprintf(stderr, "p4gl: test window action at frame %u: %.12s\n", frame, c);
        }
        c = strchr(c, ',');
        if (c == NULL) {
            break;
        }
        c++;
    }
}

/* P4PC_CAPTURE_WINDOW=1: requested frame and harness captures also include the
 * actual presenter output (bars included), with ".win.png" appended. */
static void captureWindow(unsigned frame, int ww, int wh)
{
    uint8_t *pixels;
    char path[600];
    if (sCaptureWrittenPath[0] == '\0') return;
    p4gl_glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    pixels = capturePixels(ww, wh);
    if (pixels == NULL) return;
    snprintf(path, sizeof path, "%s.win.png", sCaptureWrittenPath);
    writeCapturePng(path, (uint32_t)ww, (uint32_t)wh, pixels);
    fprintf(stderr, "p4gl: captured the window at frame %u (%dx%d) to %s\n", frame, ww, wh, path);
    free(pixels);
}

static unsigned sPresented;            /* frames presented so far */
static unsigned sDrawFrom, sDrawUntil; /* presented-frame numbers [from, until) a harness asked draws for (P4PC_TURBO=2) */

/* A harness that takes its own frames (menu_harness's shot:) says when: draws are wanted for the frames from `delay` presented
 * frames ahead, `count` frames long. Only P4PC_TURBO=2 drops draws, so otherwise this changes nothing. */
void p4gl_context_draw_between(unsigned delay, unsigned count)
{
    sDrawFrom = sPresented + 1u + delay;
    sDrawUntil = sDrawFrom + count;
}

/* P4PC_TURBO=2: whether the frame after `frame` needs its draw calls. Ordinary
 * lockstep always draws because its marks are chosen after drawing. Explicit
 * scenario replay schedules compared images in advance through the normal
 * capture/state/harness windows and retains those windows without other draws. */
static int turboNeedsDraw(unsigned frame)
{
    static int mode = -1;
    static unsigned window = 30;
    const char *c;

    if (mode < 0) {
        const char *t = getenv("P4PC_TURBO");
        const char *w = getenv("P4PC_TURBO_WINDOW");

        mode = t != NULL && t[0] == '2';
        if (w != NULL && atoi(w) > 0) {
            window = (unsigned)atoi(w);
        }
    }
    if (mode == 0 || p4pc_lockstep_draw_active()) {
        return 1;
    }
    if (frame + 1u >= sDrawFrom && frame + 1u < sDrawUntil) {
        return 1;
    }
    if (p4pc_capture_state_draw()) return 1;
    c = getenv("P4PC_CAPTURE_FRAMES");
    while (c != NULL && *c != '\0') {
        char *end;
        unsigned long v = strtoul(c, &end, 10);

        if (end == c) {
            break;
        }
        if (v > frame && v - frame <= window + 1u) {
            return 1;
        }
        c = *end == ',' ? end + 1 : end;
        if (*end != ',') {
            break;
        }
    }
    return 0;
}

/* Practice speed above 1x (include/p4pc/pace.h): only every (rate)th frame is drawn and presented,
 * so the display keeps its own rate while the game updates faster; harness shot windows still draw.
 * At 1x and below every frame draws. */
static int paceNeedsDraw(unsigned frame)
{
    const unsigned quarters = p4pc_pace_effective();

    if (quarters <= 4u) {
        return 1;
    }
    if (frame + 1u >= sDrawFrom && frame + 1u < sDrawUntil) {
        return 1;
    }
    return (frame + 1u) % (quarters / 4u) == 0;
}

/* Where the picture goes in a ww x wh window: stretch fills it; 4:3 is the largest 4:3 rectangle in it, centred; integer is the largest whole multiple of 640x480 that
 * fits (4:3 where the window is smaller than one). The PS2's 640x448 frame is shown 4:3. */
static void fitRect(int ww, int wh, int fit, GLint *dx, GLint *dy, GLint *dw, GLint *dh)
{
    *dx = *dy = 0;
    *dw = ww;
    *dh = wh;
    if (sAspectMode != P4CFG_ASPECT_4_3) {
        const double aspect = (4.0 / 3.0) * (double)p4gl_aspect_width();
        if ((double)ww > (double)wh * aspect) {
            *dw = (GLint)((double)wh * aspect + 0.5);
            *dx = (ww - *dw) / 2;
        } else {
            *dh = (GLint)((double)ww / aspect + 0.5);
            *dy = (wh - *dh) / 2;
        }
        return;
    }
    if (fit == P4CFG_FIT_INTEGER) {
        const int k = ww / 640 < wh / 480 ? ww / 640 : wh / 480;

        if (k >= 1) {
            *dw = 640 * k;
            *dh = 480 * k;
            *dx = (ww - *dw) / 2;
            *dy = (wh - *dh) / 2;
            return;
        }
        fit = P4CFG_FIT_4_3;
    }
    if (fit == P4CFG_FIT_4_3) {
        if ((int64_t)ww * 3 > (int64_t)wh * 4) {
            *dw = (GLint)((int64_t)wh * 4 / 3);
            *dx = (ww - *dw) / 2;
        } else {
            *dh = (GLint)((int64_t)ww * 3 / 4);
            *dy = (wh - *dh) / 2;
        }
    }
}

/* The native Config lives in the centered 640x448 UI layout.
 * Invert the actual presenter rectangle and active (not pending) aspect. */
int p4gl_context_menu_bounds(float rect[4])
{
    SDL_Window *window = p4pc_pad_sdl_window();
    int ww = 0, wh = 0;
    GLint x = 0, y = 0, w, h;
    if (!window || !sContext || !SDL_GetWindowSizeInPixels(window, &ww, &wh) || ww <= 0 || wh <= 0)
        return 0;
    w = ww;
    h = wh;
    if (sPresentBlit) fitRect(ww, wh, sPresentFit, &x, &y, &w, &h);
    const float aspect = p4gl_aspect_width();
    const float menuW = (float)w * (aspect > 1.0f ? 1.0f / aspect : 1.0f);
    const float menuH = (float)h * (aspect < 1.0f ? aspect : 1.0f);
    rect[0] = ((float)x + ((float)w - menuW) * 0.5f) / (float)ww;
    rect[1] = ((float)y + ((float)h - menuH) * 0.5f) / (float)wh;
    rect[2] = menuW / (float)ww;
    rect[3] = menuH / (float)wh;
    return rect[2] > 0 && rect[3] > 0;
}
int p4gl_context_menu_position(float x, float y, float *menuX, float *menuY)
{
    float rect[4];
    if (!p4gl_context_menu_bounds(rect)) return 0;
    x = (x - rect[0]) / rect[2] * 640.0f;
    y = (y - rect[1]) / rect[3] * 448.0f;
    if (!(x >= 0 && x <= 640 && y >= 0 && y <= 448)) return 0;
    *menuX = x;
    *menuY = y;
    return 1;
}

/* SDL_GL_SwapWindow, and the recovery when EGL says the context is lost (EGL_CONTEXT_LOST at eglSwapBuffers: Android took the GPU away, ANGLE lost its device). */
static void swapWindow(SDL_Window *window)
{
    if (p4gl_profile_pass != NULL) p4gl_profile_frame_end(sPresented);
    const uint64_t swap_start = p4gl_gl_hitch_begin();
    const int swapped = p4gl_win_swap(window);
    if (swap_start) p4gl_gl_hitch_api("swap_window_api", swap_start, 0, 0, 0);
    if (!swapped) {
        const char *err = p4gl_win_error();

        if (err != NULL && (strstr(err, "CONTEXT_LOST") != NULL || strstr(err, "0x300e") != NULL)) {
            context_recover(0);
        }
    }
}


static int saveResolution(const void *value)
{
    return p4cfg_set_str("video.resolution", value);
}

static int applySettings(const P4CfgVideo *video, const char *resolution)
{
    SDL_Window *window = p4pc_pad_sdl_window();
    const char *offscreen = getenv("P4PC_OFFSCREEN");
    const int blit = video->blit || sAspectMode != P4CFG_ASPECT_4_3;
    const int target = blit || (offscreen != NULL && offscreen[0] == '1');
    uint32_t w = 0, h = 0;
    const float previousAspect = p4gl_aspect_width();
    if (window == NULL || sContext == NULL) return -1;
#if !defined(SDL_PLATFORM_ANDROID) && !defined(SDL_PLATFORM_IOS)
    int oldWidth = 0, oldHeight = 0;
    const int oldFullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
    SDL_GetWindowSize(window, &oldWidth, &oldHeight);
    if (oldFullscreen != (video->fullscreen != 0) && !SDL_SetWindowFullscreen(window, video->fullscreen != 0)) goto failed;
    if (!video->fullscreen && (oldWidth != video->width || oldHeight != video->height) &&
        !SDL_SetWindowSize(window, video->width, video->height)) goto failed;
#endif
    if (!renderSize(window, video, &w, &h) ||
        p4gl_readback_configure_checked(target, w, h, resolution ? saveResolution : NULL, resolution) != 0) goto failed;
#if !defined(SDL_PLATFORM_ANDROID) && !defined(SDL_PLATFORM_IOS)
    SDL_SetWindowResizable(window, blit != 0);
#endif
    sPresentBlit = blit;
    sOffscreen = target;
    sPresentFit = video->fit;
    setSwapInterval();
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, p4gl_readback_framebuffer());
    if (target) p4gl_readback_viewport();
    else p4gl_glViewport(0, 0, 640, 448);
    return 0;

failed:
    p4gl_aspect_set(previousAspect);
#if !defined(SDL_PLATFORM_ANDROID) && !defined(SDL_PLATFORM_IOS)
    if (oldFullscreen != ((SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0) &&
        !SDL_SetWindowFullscreen(window, oldFullscreen != 0))
        fprintf(stderr, "p4gl: could not restore fullscreen after rejected settings: %s\n", SDL_GetError());
    if (!oldFullscreen) {
        int width = 0, height = 0;
        SDL_GetWindowSize(window, &width, &height);
        if ((width != oldWidth || height != oldHeight) && !SDL_SetWindowSize(window, oldWidth, oldHeight))
            fprintf(stderr, "p4gl: could not restore window after rejected settings: %s\n", SDL_GetError());
    }
#endif
    return -1;
}

int p4gl_context_apply_settings(void)
{
    return applySettings(p4cfg_video(), NULL);
}

void p4gl_context_present(void)
{
    if (p4_battle_sim_renderless) {
        p4_settings_native_frame_end();
        p4pc_fpu_check("the no-device frame boundary");
        return;
    }
    SDL_Window *window = p4pc_pad_sdl_window();

    if (sContext != NULL && window != NULL) {
        const uint64_t t0 = p4_os_now_ns();
        static uint64_t swapNs, presentNs;
        uint64_t t1;

        unsigned frame;
        int capture;

        if (p4pc_pad_sdl_take_device_reset()) {
            context_recover(1); /* SDL made a new context for the resumed window */
        }
        p4pc_fpu_check("the frame's game and GL calls");
        p4gl_frame_rasters_present();
        frame = ++sPresented;
        testWindowActions(window, frame);
        capture = captureWanted(frame) || sCaptureNextPath[0] != '\0';
        if (capture) {
            captureFrame(window, frame);
        }
        static int finishWanted = -1;

        if (finishWanted < 0) {
            finishWanted = getenv("P4PC_GLFINISH") != NULL;
        }
        if (finishWanted) {
            /* Experiment: how much of the swap is the GPU still drawing (the time moves into this call). */
            static void (P4GL_APIENTRY *finish)(void);
            static uint64_t finishNs;
            const uint64_t f0 = p4_os_now_ns();

            if (finish == NULL) {
                finish = (void (P4GL_APIENTRY *)(void))p4gl_win_proc("glFinish");
            }
            finish();
            finishNs += p4_os_now_ns() - f0;
            if (frame % 1000u == 0) {
                fprintf(stderr, "p4gl: glFinish before swap %.3f ms/frame\n", (double)finishNs * 1e-6 / 1000.0);
                finishNs = 0;
            }
        }
        if (p4gl_profile_pass != NULL) p4gl_profile_pass(P4GL_PROFILE_PRESENT);
        t1 = p4_os_now_ns();
        if (!p4gl_gl_draw_enabled()) {
            /* P4PC_TURBO=2 dropped this frame's draw calls: there is nothing to show and no capture wants it, and a swap is a round trip to the
             * compositor (several ms on the Android emulator, where it was most of an unattended run's time) */
            if (p4gl_profile_pass != NULL) p4gl_profile_frame_end(frame);
        } else if (sPresentBlit) {
            int ww = 0, wh = 0;
            uint32_t fw = 0, fh = 0;

            p4gl_readback_pixel_size(&fw, &fh);
            SDL_GetWindowSizeInPixels(window, &ww, &wh);
            /* A minimised window has nothing to show (and some systems fail a swap to it); the game runs on, paced by its clock. */
            if (ww > 0 && wh > 0 && (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) == 0) {
                GLint dx = 0, dy = 0, dw = ww, dh = wh;

                p4gl_glBindFramebuffer(GL_READ_FRAMEBUFFER, p4gl_readback_framebuffer());
                p4gl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                fitRect(ww, wh, sPresentFit, &dx, &dy, &dw, &dh);
                if (dw != ww || dh != wh) {
                    p4gl_gl_clear_black(); /* the bars */
                }
                {
                    static int lastW, lastH;

                    if (ww != lastW || wh != lastH) {
                        lastW = ww;
                        lastH = wh;
                        fprintf(stderr, "p4gl: window %dx%d pixels, %ux%u frame shown at %d,%d size %dx%d\n", ww, wh, (unsigned)fw, (unsigned)fh, dx, dy, dw, dh);
                    }
                }
                p4gl_glBlitFramebuffer(0, 0, (GLint)fw, (GLint)fh, dx, dy, dx + dw, dy + dh, GL_COLOR_BUFFER_BIT,
                                       (uint32_t)dw == fw && (uint32_t)dh == fh ? GL_NEAREST : GL_LINEAR);
                if (getenv("P4PC_CAPTURE_WINDOW") != NULL && capture) {
                    captureWindow(frame, ww, wh);
                }
                swapWindow(window);
            }
            else if (p4gl_profile_pass != NULL) p4gl_profile_frame_end(frame);
            p4gl_glBindFramebuffer(GL_FRAMEBUFFER, p4gl_readback_framebuffer());
        } else if (sOffscreen) {
            if (p4gl_profile_pass != NULL) p4gl_profile_frame_end(frame);
            p4gl_glFlush();
        } else {
            swapWindow(window);
        }
        if (sOffscreen && (sAspectMode == P4CFG_ASPECT_AUTO ||
                          p4cfg_video()->resolution == P4CFG_RESOLUTION_NATIVE)) {
            uint32_t w = 0, h = 0;
            const float previous = p4gl_aspect_width();
            if (renderSize(window, p4cfg_video(), &w, &h) && p4gl_readback_configure(1, w, h) != 0) {
                p4gl_aspect_set(previous);
                fprintf(stderr, "p4gl: cannot resize the render target to %ux%u\n", w, h);
            }
            p4gl_readback_viewport();
        }
        if (sTestLossFrame != 0 && frame == sTestLossFrame) {
            context_recover(0);
        }
        swapNs += p4_os_now_ns() - t1;
        p4_settings_native_frame_end();
        p4gl_gl_set_draw(turboNeedsDraw(frame) && paceNeedsDraw(frame));
        p4pc_fpu_check("the swap");
        presentNs += p4_os_now_ns() - t0;
        if (frame % 1000u == 0) {
            p4gl_gl_stats_report(1000u, (double)swapNs * 1e-6, (double)presentNs * 1e-6);
            swapNs = presentNs = 0;
        }
    } else {
        p4_settings_native_frame_end();
    }
}
