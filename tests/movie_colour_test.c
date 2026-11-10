/* Exhaustive converter-domain proof, not a player benchmark. The reference is
 * the unchanged shipped libswscale conversion, not a second colour converter. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>
#include <libswscale/swscale.h>
#include <libavutil/cpu.h>
#include "mpeg2_dec.h"
#include "p4gl_gl.h"
#include "p4gl_profile.h"
#include "p4gl_yuv_shader.h"
#include "p4pc/hostmath.h"
#ifdef __ANDROID__
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <dlfcn.h>
#include <time.h>
#include <unistd.h>
#endif

#ifndef GL_R8
#define GL_R8 0x8229
#define GL_RED 0x1903
#endif

static uint64_t triples, mismatch[3], channels[3][3], separableR, separableB;
static uint8_t red[256*256], blue[256*256];
static unsigned first[3][6];
#ifdef __ANDROID__
static EGLDisplay sDisplay = EGL_NO_DISPLAY;
static EGLSurface sSurface = EGL_NO_SURFACE;
static EGLContext sContext = EGL_NO_CONTEXT;
static void *sGles;
#else
static SDL_Window *sWindow;
static SDL_GLContext sContext;
#endif
static GLuint sPlaneTexture[3], sOutput, sFbo, sVao, sProgram;
static uint8_t *sPlane[3], *sPixels;
static int sGpu, sGlReady, sPassActive;
static uint64_t gpuMismatch, gpuChannels[4];
static unsigned gpuFirst[11];
static uint64_t geometryTriples, geometryMismatch;

#ifdef __ANDROID__
/* Standalone, real GLES transport: no Java activity or mocked GL entry points.
 * Compile this fixture and its GL helpers with SDL_GL_GetProcAddress renamed
 * to this resolver; the shader and readback path below are otherwise shared. */
SDL_FunctionPointer p4colour_gl_proc(const char *name)
{
    void *symbol = sGles ? dlsym(sGles, name) : NULL;
    return symbol ? (SDL_FunctionPointer)symbol : (SDL_FunctionPointer)eglGetProcAddress(name);
}
#endif

static unsigned clip(int value)
{
    return value < 0 ? 0u : value > 255 ? 255u : (unsigned)value;
}

/* FFmpeg7.1.1 yuv2rgb.c coefficients, and the separately truncated signed-high
 * products of x86 yuv_2_rgb.asm / AArch64 yuv2rgb_neon.S. All products/sums fit
 * signed32; saturating16-bit instructions cannot saturate on 8-bit input. */
static void integer_rgb(unsigned y, unsigned u, unsigned v, unsigned model, unsigned out[3])
{
    if (model == 2) {
        /* ff_yuv2rgb_c_init_tables/fill_table: pointer offsets followed by the
         * rounded luma table lookup, folded without allocating the tables. */
        const int ro = (((int)v*89830) >> 16)-175;
        const int go = (((int)u*-22049) >> 16)+(((int)v*-45756) >> 16)+134;
        const int bo = (((int)u*113537) >> 16)-221;
        out[0] = clip((((int)y+ro)*76309-1304898) >> 16);
        out[1] = clip((((int)y+go)*76309-1304898) >> 16);
        out[2] = clip((((int)y+bo)*76309-1304898) >> 16);
        return;
    }
    const int shift = model ? 15 : 16;
    const int yy = (((int)y*8 - 128)*9539) >> shift;
    const int uu = ((int)u - 128)*8;
    const int vv = ((int)v - 128)*8;
    const int round = model ? 1 : 0;
    const int post = model ? 1 : 0;
    out[0] = clip((yy + ((vv*13075) >> shift) + round) >> post);
    out[1] = clip((yy + ((uu*-3209) >> shift) + ((vv*-6660) >> shift) + round) >> post);
    out[2] = clip((yy + ((uu*16525) >> shift) + round) >> post);
}

static int gpu_open(int desktop)
{
    GLuint vs, fs;
    GLint linked = GL_FALSE;
    static const char vertex[] =
        "const vec2 kCorner[6] = vec2[6](vec2(-1.0,-1.0),vec2(1.0,-1.0),vec2(-1.0,1.0),vec2(-1.0,1.0),vec2(1.0,-1.0),vec2(1.0,1.0));\n"
        "void main() { gl_Position = vec4(kCorner[gl_VertexID],0.0,1.0); }\n";
#ifdef __ANDROID__
    EGLConfig config;
    EGLint count = 0;
    const EGLint attributes[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_NONE};
    const EGLint surface[] = {EGL_WIDTH, 64, EGL_HEIGHT, 64, EGL_NONE};
    const EGLint context[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    if (desktop) return 77;
    sGles = dlopen("libGLESv3.so", RTLD_NOW | RTLD_LOCAL);
    sDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!sGles || sDisplay == EGL_NO_DISPLAY || !eglInitialize(sDisplay, NULL, NULL)
        || !eglBindAPI(EGL_OPENGL_ES_API) || !eglChooseConfig(sDisplay, attributes, &config, 1, &count) || count != 1) {
        fprintf(stderr, "movie_colour: GLES EGL setup unavailable, error 0x%x\n", (unsigned)eglGetError());
        return 77;
    }
    sSurface = eglCreatePbufferSurface(sDisplay, config, surface);
    sContext = eglCreateContext(sDisplay, config, EGL_NO_CONTEXT, context);
    if (sSurface == EGL_NO_SURFACE || sContext == EGL_NO_CONTEXT || !eglMakeCurrent(sDisplay, sSurface, sSurface, sContext)) {
        fprintf(stderr, "movie_colour: GLES EGL context unavailable, error 0x%x\n", (unsigned)eglGetError());
        return 77;
    }
#else
    const char *driver = getenv("SDL_VIDEODRIVER");
#ifdef _WIN32
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, driver ? driver : "windows");
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, driver ? driver : "offscreen");
#endif
    if (!P4GL_YUV_INTEGER_SUPPORTED || !SDL_Init(SDL_INIT_VIDEO)) return 77;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, desktop ? SDL_GL_CONTEXT_PROFILE_CORE : SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, desktop ? 3 : 0);
    sWindow = SDL_CreateWindow("movie colour exhaustive", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    sContext = sWindow ? SDL_GL_CreateContext(sWindow) : NULL;
    if (sContext == NULL || !SDL_GL_MakeCurrent(sWindow, sContext)) {
        fprintf(stderr, "movie_colour: no requested GL context: %s\n", SDL_GetError());
        return 77;
    }
#endif
    if (p4gl_gl_load() != 0) return 1;
    sGlReady = 1;
    if (!p4gl_gl_pass_begin()) return 1;
    sPassActive = 1;
    vs = p4gl_gl_compile_shader(GL_VERTEX_SHADER, vertex, "movie colour proof vertex");
    fs = p4gl_gl_compile_shader(GL_FRAGMENT_SHADER, kP4YuvIntegerFragment, "movie integer colour proof");
    if (!vs || !fs) return 1;
    sProgram = p4gl_glCreateProgram();
    p4gl_glAttachShader(sProgram, vs);
    p4gl_glAttachShader(sProgram, fs);
    p4gl_glLinkProgram(sProgram);
    p4gl_glGetProgramiv(sProgram, GL_LINK_STATUS, &linked);
    p4gl_glDeleteShader(vs);
    p4gl_glDeleteShader(fs);
    if (linked != GL_TRUE) return 1;
    p4gl_glUseProgram(sProgram);
    p4gl_glUniform1i(p4gl_glGetUniformLocation(sProgram, "uY"), 0);
    p4gl_glUniform1i(p4gl_glGetUniformLocation(sProgram, "uU"), 1);
    p4gl_glUniform1i(p4gl_glGetUniformLocation(sProgram, "uV"), 2);
    p4gl_glGenTextures(3, sPlaneTexture);
    p4gl_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned i = 0; i < 3; i++) {
        const int w = i ? 2048 : 4096, h = i ? 64 : 128;
        sPlane[i] = malloc((size_t)w*(size_t)h);
        if (sPlane[i] == NULL) return 1;
        p4gl_glActiveTexture(GL_TEXTURE0 + i);
        p4gl_glBindTexture(GL_TEXTURE_2D, sPlaneTexture[i]);
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        p4gl_glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    }
    sPixels = malloc(4096u*128u*4u);
    if (sPixels == NULL) return 1;
    p4gl_glActiveTexture(GL_TEXTURE0);
    p4gl_glGenTextures(1, &sOutput);
    p4gl_glBindTexture(GL_TEXTURE_2D, sOutput);
    p4gl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4096, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    p4gl_glGenFramebuffers(1, &sFbo);
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, sFbo);
    p4gl_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sOutput, 0);
    if (p4gl_glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return 1;
    p4gl_glGenVertexArrays(1, &sVao);
    p4gl_glBindVertexArray(sVao);
    p4gl_glViewport(0, 0, 4096, 128);
    sGpu = 1;
    return 0;
}

static int gpu_compare(const uint8_t *reference, int y0, int count)
{
    if (count != 128) return -1;
    for (unsigned i = 0; i < 3; i++) {
        const int w = i ? 2048 : 4096, h = i ? 64 : 128;
        p4gl_glActiveTexture(GL_TEXTURE0 + i);
        p4gl_glBindTexture(GL_TEXTURE_2D, sPlaneTexture[i]);
        p4gl_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED, GL_UNSIGNED_BYTE, sPlane[i]);
    }
    p4gl_glDrawArrays(GL_TRIANGLES, 0, 6);
    p4gl_glReadPixels(0, 0, 4096, 128, GL_RGBA, GL_UNSIGNED_BYTE, sPixels);
    if (p4gl_glGetError() != GL_NO_ERROR) return -1;
    for (unsigned i = 0; i < 4096u*128u; i++) {
        unsigned bad = 0;
        for (unsigned c = 0; c < 4; c++) {
            if (sPixels[i*4u+c] != reference[i*4u+c]) { gpuChannels[c]++; bad = 1; }
        }
        if (bad && gpuMismatch++ == 0) {
            const unsigned row = i / 4096u + (unsigned)y0, x = i % 4096u;
            const unsigned b = (row >> 1)*2048u + (x >> 1);
            gpuFirst[0] = (b & 63u)*4u+(x & 1u)+2u*(row & 1u);
            gpuFirst[1] = (b >> 6) & 255u;
            gpuFirst[2] = b >> 14;
            for (unsigned c = 0; c < 4; c++) {
                gpuFirst[3+c] = reference[i*4u+c];
                gpuFirst[7+c] = sPixels[i*4u+c];
            }
        }
    }
    return 0;
}

static void gpu_close(void)
{
    if (sContext != NULL) {
        if (sGlReady) {
            if (sPassActive) p4gl_gl_pass_end();
            p4gl_profile_close();
            p4gl_glDeleteTextures(3, sPlaneTexture);
            p4gl_glDeleteTextures(1, &sOutput);
            p4gl_glDeleteFramebuffers(1, &sFbo);
            if (sProgram) p4gl_glDeleteProgram(sProgram);
        }
        /* The context owns and releases the proof's VAO. */
#ifdef __ANDROID__
        eglMakeCurrent(sDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(sDisplay, sContext);
#else
        SDL_GL_DestroyContext(sContext);
#endif
    }
#ifdef __ANDROID__
    if (sSurface != EGL_NO_SURFACE) eglDestroySurface(sDisplay, sSurface);
    if (sDisplay != EGL_NO_DISPLAY) eglTerminate(sDisplay);
    if (sGles) dlclose(sGles);
#else
    if (sWindow != NULL) SDL_DestroyWindow(sWindow);
#endif
    for (unsigned i = 0; i < 3; i++) free(sPlane[i]);
    free(sPixels);
#ifndef __ANDROID__
    SDL_Quit();
#endif
}

/* The emitted GLSL highp float expression is binary32 on supported contexts.
 * Exhaust all 256 possible integer channels, including the unclamped n=255
 * intermediate. The .25 offset must stay strictly below the nearest tie and
 * strictly above the truncation boundary. */
static int unorm_window(void)
{
    volatile float scale = 1.0f / 255.0f;
    for (unsigned n = 0; n <= 255; n++) {
        volatile float value = ((float)n + 0.25f) * scale;
        double scaled = (double)value * 255.0;
        if (!(scaled > (double)n && scaled < (double)n + 0.5)) {
            fprintf(stderr, "movie_colour: UNORM interior window failed at %u: %.12f\n", n, scaled);
            return 1;
        }
    }
    return 0;
}

static int rows(const uint8_t *rgba, int32_t y0, int32_t count)
{
    for (int row = 0; row < count; row++) {
        for (int x = 0; x < 4096; x++) {
            const unsigned b = (unsigned)((y0 + row) >> 1)*2048u + (unsigned)(x >> 1);
            const unsigned y = (b & 63u)*4u + (unsigned)((x & 1) + 2*((y0 + row) & 1));
            const unsigned u = (b >> 6) & 255u, v = b >> 14;
            const uint8_t *reference = rgba + ((size_t)row*4096u+(unsigned)x)*4u;
            if (sGpu) {
                sPlane[0][(size_t)row*4096u+(unsigned)x] = (uint8_t)y;
                if (((row | x) & 1) == 0) {
                    sPlane[1][(size_t)(row >> 1)*2048u+(unsigned)(x >> 1)] = (uint8_t)u;
                    sPlane[2][(size_t)(row >> 1)*2048u+(unsigned)(x >> 1)] = (uint8_t)v;
                }
            }
            if (u == 0) red[y + v*256u] = reference[0];
            else if (red[y + v*256u] != reference[0]) separableR++;
            if (v == 0) blue[y + u*256u] = reference[2];
            else if (blue[y + u*256u] != reference[2]) separableB++;
            for (unsigned model = 0; model < 3; model++) {
                unsigned got[3], bad = 0;
                integer_rgb(y, u, v, model, got);
                for (unsigned c = 0; c < 3; c++) {
                    if (got[c] != reference[c]) { channels[model][c]++; bad = 1; }
                }
                if (bad && mismatch[model]++ == 0) {
                    first[model][0] = y; first[model][1] = u; first[model][2] = v;
                    first[model][3] = reference[0]; first[model][4] = reference[1]; first[model][5] = reference[2];
                }
            }
            triples++;
        }
    }
    return sGpu ? gpu_compare(rgba, y0, count) : 0;
}

#define TABLE_SIZE 4096
#define TABLE_BAND 128

static int reference_bands(int (*rows)(const uint8_t *rgba, int32_t y0, int32_t n)) {
    uint8_t *ys = malloc((size_t)TABLE_SIZE * TABLE_BAND);
    uint8_t *us = malloc((size_t)(TABLE_SIZE / 2) * (TABLE_BAND / 2));
    uint8_t *vs = malloc((size_t)(TABLE_SIZE / 2) * (TABLE_BAND / 2));
    uint8_t *out = malloc((size_t)TABLE_SIZE * TABLE_BAND * 4);
    P4Mpeg2Frame f;
    int r = -1;

    if (!ys || !us || !vs || !out) goto done;
    memset(&f, 0, sizeof f);
    f.width = TABLE_SIZE;
    f.height = TABLE_BAND;
    f.plane[0] = ys;
    f.plane[1] = us;
    f.plane[2] = vs;
    f.stride[0] = TABLE_SIZE;
    f.stride[1] = f.stride[2] = TABLE_SIZE / 2;
    for (int y0 = 0; y0 < TABLE_SIZE; y0 += TABLE_BAND) {
        for (int row = 0; row < TABLE_BAND; row++) {
            for (int x = 0; x < TABLE_SIZE; x++) {
                const int32_t by = (y0 + row) >> 1, bx = x >> 1;
                const int32_t b = by * (TABLE_SIZE / 2) + bx;
                const int k = (x & 1) + 2 * ((y0 + row) & 1);

                ys[(size_t)row * TABLE_SIZE + x] = (uint8_t)((b & 63) * 4 + k);
                if (((row | x) & 1) == 0) {
                    us[(size_t)(row >> 1) * (TABLE_SIZE / 2) + bx] = (uint8_t)((b >> 6) & 255);
                    vs[(size_t)(row >> 1) * (TABLE_SIZE / 2) + bx] = (uint8_t)(b >> 14);
                }
            }
        }
        if (p4mpeg2_frame_to_rgba_ipu(&f, out, TABLE_SIZE * 4) != 0) goto done;
        for (size_t i = 3; i < (size_t)TABLE_SIZE * TABLE_BAND * 4; i += 4)
            if (out[i] != 0xff) goto done;
        if (rows(out, y0, TABLE_BAND) != 0) goto done;
    }
    r = 0;
done:
    free(ys);
    free(us);
    free(vs);
    free(out);
    return r;
}

/* Preserve the former movie-player test's exhaustive 640x448 geometry witness,
 * now independent of both a 64MiB lookup table and the retail disc. */
static int geometry_reference(void)
{
    enum { W = 640, H = 448, BLOCKS = (W / 2) * (H / 2), ALL = 2048 * 2048 };
    uint8_t *y = malloc((size_t)W * H);
    uint8_t *u = malloc((size_t)(W / 2) * (H / 2));
    uint8_t *v = malloc((size_t)(W / 2) * (H / 2));
    uint8_t *rgba = malloc((size_t)W * H * 4);
    P4Mpeg2Frame f;
    int result = -1;
    if (!y || !u || !v || !rgba) goto done;
    memset(&f, 0, sizeof f);
    f.width = W;
    f.height = H;
    f.plane[0] = y; f.plane[1] = u; f.plane[2] = v;
    f.stride[0] = W;
    f.stride[1] = f.stride[2] = W / 2;
    for (int first = 0; first < ALL; first += BLOCKS) {
        for (int j = 0; j < BLOCKS; j++) {
            const int b = first + j < ALL ? first + j : ALL - 1;
            const int bx = j % (W / 2), by = j / (W / 2);
            u[by * (W / 2) + bx] = (uint8_t)((b >> 6) & 255);
            v[by * (W / 2) + bx] = (uint8_t)(b >> 14);
            for (int k = 0; k < 4; k++)
                y[(size_t)(by * 2 + (k >> 1)) * W + bx * 2 + (k & 1)] = (uint8_t)((b & 63) * 4 + k);
        }
        if (p4mpeg2_frame_to_rgba_ipu(&f, rgba, W * 4) != 0) goto done;
        for (int j = 0; j < BLOCKS && first + j < ALL; j++) {
            const int bx = j % (W / 2), by = j / (W / 2), b = first + j;
            for (int k = 0; k < 4; k++) {
                unsigned expected[3];
                const size_t o = ((size_t)(by * 2 + (k >> 1)) * W + bx * 2 + (k & 1)) * 4;
                integer_rgb((unsigned)((b & 63) * 4 + k), (unsigned)((b >> 6) & 255),
                    (unsigned)(b >> 14), P4GL_YUV_MODEL, expected);
                geometryMismatch += rgba[o] != expected[0] || rgba[o+1] != expected[1]
                    || rgba[o+2] != expected[2] || rgba[o+3] != 255;
                geometryTriples++;
            }
        }
    }
    result = 0;
done:
    free(y); free(u); free(v); free(rgba);
    return result;
}

#ifdef __ANDROID__
/* CLOCK_BOOTTIME brackets /proc/<pid>/stat start ticks; the game traces use
 * CLOCK_MONOTONIC. Retain both bounds instead of assuming no prior suspend. */
static int clock_calibration(void)
{
    struct timespec lower, boot, upper;
    const long ticks = sysconf(_SC_CLK_TCK);
    if (ticks <= 0 || clock_gettime(CLOCK_MONOTONIC, &lower) != 0
        || clock_gettime(CLOCK_BOOTTIME, &boot) != 0 || clock_gettime(CLOCK_MONOTONIC, &upper) != 0) {
        perror("movie_colour: Android clock calibration");
        return 1;
    }
    printf("{\"mono_lower_ns\":%llu,\"boottime_ns\":%llu,\"mono_upper_ns\":%llu,\"ticks_per_second\":%ld}\n",
        (unsigned long long)lower.tv_sec*1000000000ull+(unsigned long long)lower.tv_nsec,
        (unsigned long long)boot.tv_sec*1000000000ull+(unsigned long long)boot.tv_nsec,
        (unsigned long long)upper.tv_sec*1000000000ull+(unsigned long long)upper.tv_nsec, ticks);
    return 0;
}
#endif

int main(int argc, char **argv)
{
    const int cpuOnly = argc == 2 && strcmp(argv[1], "--cpu") == 0;
    int rc;
#ifdef __ANDROID__
    if (argc == 2 && strcmp(argv[1], "--clock") == 0) return clock_calibration();
#endif
    if (!P4GL_YUV_INTEGER_SUPPORTED) return 77;
    /* The game creates its GL context in EE mode; llvmpipe inherits this
     * rounding mode, unlike a standalone default-MXCSR test process. */
    if (!cpuOnly) p4pc_fpu_ee_mode();
    if (unorm_window() != 0) return 1;
#if defined(P4GL_TEST_GENERIC_C) && !defined(__ANDROID__)
    /* Exercise the Android SDK's --disable-asm converter with the same pinned
     * library on a desktop test host; Android itself keeps its real CPU flags. */
    av_force_cpu_flags(0);
#endif
    if (!cpuOnly) {
        rc = gpu_open(argc == 2 && strcmp(argv[1], "--gl") == 0);
        if (rc != 0) { gpu_close(); return rc; }
    }
    rc = reference_bands(rows);
    if (rc != 0) { gpu_close(); return 1; }
    if (geometry_reference() != 0) { gpu_close(); return 1; }
    printf("{\"selected_model\":%u,\"cpu_flags\":%d,\"configured_assembly_disabled\":%s,\"triples\":%llu,\"geometry_640x448_triples\":%llu,\"geometry_640x448_mismatches\":%llu,\"swscale_version\":%u,\"integer_models\":[",
        (unsigned)P4GL_YUV_MODEL, av_get_cpu_flags(), strstr(swscale_configuration(), "--disable-asm") ? "true" : "false",
        (unsigned long long)triples, (unsigned long long)geometryTriples,
        (unsigned long long)geometryMismatch, swscale_version());
    for (unsigned model = 0; model < 3; model++) {
        printf("%s{\"name\":\"%s\",\"mismatched_triples\":%llu,\"channel_mismatches\":[%llu,%llu,%llu],\"first_yuv_rgb\":[%u,%u,%u,%u,%u,%u]}",
            model ? "," : "", model == 2 ? "generic_table_fixed16" : model ? "neon_separate_high15_round1" : "x86_separate_high16",
            (unsigned long long)mismatch[model], (unsigned long long)channels[model][0],
            (unsigned long long)channels[model][1], (unsigned long long)channels[model][2],
            first[model][0], first[model][1], first[model][2], first[model][3], first[model][4], first[model][5]);
    }
    printf("],\"red_separability_mismatches\":%llu,\"blue_separability_mismatches\":%llu,\"gpu_tested\":%s,\"gpu_mismatch_triples\":%llu,\"gpu_channel_mismatches\":[%llu,%llu,%llu,%llu],\"gpu_first_yuv_reference_actual\":[",
        (unsigned long long)separableR, (unsigned long long)separableB, sGpu ? "true" : "false",
        (unsigned long long)gpuMismatch, (unsigned long long)gpuChannels[0], (unsigned long long)gpuChannels[1],
        (unsigned long long)gpuChannels[2], (unsigned long long)gpuChannels[3]);
    for (unsigned i = 0; i < 11; i++) printf("%s%u", i ? "," : "", gpuFirst[i]);
    printf("]}\n");
    gpu_close();
    return triples != 16777216ull || geometryTriples != 16777216ull || geometryMismatch != 0
        || mismatch[P4GL_YUV_MODEL] != 0 || gpuMismatch != 0 ? 1 : 0;
}
