/* OpenGL 3.3 core / OpenGL ES 3.0 entry points, resolved through SDL_GL_GetProcAddress once the context is
 * current (see p4gl_context_open). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "p4gl_gl.h"
#include "p4gl_win.h"
#include "p4gl_profile.h"

void (P4GL_APIENTRY *p4gl_glGenTextures)(GLsizei n, GLuint *textures);
void (P4GL_APIENTRY *p4gl_glDeleteTextures)(GLsizei n, const GLuint *textures);
void (P4GL_APIENTRY *p4gl_glBindTexture)(GLenum target, GLuint texture);
void (P4GL_APIENTRY *p4gl_glTexImage2D)(GLenum target, GLint level, GLint internalFormat,
                          GLsizei width, GLsizei height, GLint border,
                          GLenum format, GLenum type, const void *pixels);
void (P4GL_APIENTRY *p4gl_glCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y,
                                               GLsizei width, GLsizei height);
void (P4GL_APIENTRY *p4gl_glActiveTexture)(GLenum texture);
void (P4GL_APIENTRY *p4gl_glTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                             GLsizei width, GLsizei height, GLenum format, GLenum type,
                             const void *pixels);
void (P4GL_APIENTRY *p4gl_glTexParameteri)(GLenum target, GLenum pname, GLint param);
void (P4GL_APIENTRY *p4gl_glPixelStorei)(GLenum pname, GLint param);
void (P4GL_APIENTRY *p4gl_glClearColor)(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void (P4GL_APIENTRY *p4gl_glClear)(GLbitfield mask);
void (P4GL_APIENTRY *p4gl_glFlush)(void);
void (P4GL_APIENTRY *p4gl_glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
GLenum (P4GL_APIENTRY *p4gl_glGetError)(void);

void (P4GL_APIENTRY *p4gl_glEnable)(GLenum cap);
void (P4GL_APIENTRY *p4gl_glDisable)(GLenum cap);
void (P4GL_APIENTRY *p4gl_glBlendFunc)(GLenum sfactor, GLenum dfactor);
void (P4GL_APIENTRY *p4gl_glGenVertexArrays)(GLsizei n, GLuint *arrays);
void (P4GL_APIENTRY *p4gl_glBindVertexArray)(GLuint array);
void (P4GL_APIENTRY *p4gl_glGenBuffers)(GLsizei n, GLuint *buffers);
void (P4GL_APIENTRY *p4gl_glBindBuffer)(GLenum target, GLuint buffer);
void (P4GL_APIENTRY *p4gl_glBufferData)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
void (P4GL_APIENTRY *p4gl_glEnableVertexAttribArray)(GLuint index);
void (P4GL_APIENTRY *p4gl_glVertexAttribPointer)(GLuint index, GLint size, GLenum type,
                                   GLboolean normalized, GLsizei stride,
                                   const void *pointer);
void (P4GL_APIENTRY *p4gl_glVertexAttribDivisor)(GLuint index, GLuint divisor);
GLuint (P4GL_APIENTRY *p4gl_glCreateShader)(GLenum type);
void (P4GL_APIENTRY *p4gl_glDeleteShader)(GLuint shader);
void (P4GL_APIENTRY *p4gl_glShaderSource)(GLuint shader, GLsizei count, const GLchar *const *string,
                            const GLint *length);
void (P4GL_APIENTRY *p4gl_glCompileShader)(GLuint shader);
void (P4GL_APIENTRY *p4gl_glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
void (P4GL_APIENTRY *p4gl_glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei *length,
                                GLchar *infoLog);
GLuint (P4GL_APIENTRY *p4gl_glCreateProgram)(void);
void (P4GL_APIENTRY *p4gl_glDeleteProgram)(GLuint program);
void (P4GL_APIENTRY *p4gl_glAttachShader)(GLuint program, GLuint shader);
void (P4GL_APIENTRY *p4gl_glLinkProgram)(GLuint program);
void (P4GL_APIENTRY *p4gl_glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
void (P4GL_APIENTRY *p4gl_glUseProgram)(GLuint program);
GLint (P4GL_APIENTRY *p4gl_glGetUniformLocation)(GLuint program, const GLchar *name);
void (P4GL_APIENTRY *p4gl_glUniform2f)(GLint location, GLfloat v0, GLfloat v1);
void (P4GL_APIENTRY *p4gl_glUniform1i)(GLint location, GLint v0);
void (P4GL_APIENTRY *p4gl_glUniform1f)(GLint location, GLfloat v0);
void (P4GL_APIENTRY *p4gl_glDrawArrays)(GLenum mode, GLint first, GLsizei count);
void (P4GL_APIENTRY *p4gl_glDrawArraysInstanced)(GLenum mode, GLint first, GLsizei count, GLsizei instances);
void (P4GL_APIENTRY *p4gl_glDrawElements)(GLenum mode, GLsizei count, GLenum type, const void *indices);
void (P4GL_APIENTRY *p4gl_glGenFramebuffers)(GLsizei n, GLuint *framebuffers);
void (P4GL_APIENTRY *p4gl_glBindFramebuffer)(GLenum target, GLuint framebuffer);
void (P4GL_APIENTRY *p4gl_glFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget,
                                   GLuint texture, GLint level);
GLenum (P4GL_APIENTRY *p4gl_glCheckFramebufferStatus)(GLenum target);
void (P4GL_APIENTRY *p4gl_glReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height,
                         GLenum format, GLenum type, void *pixels);
void (P4GL_APIENTRY *p4gl_glGenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
void (P4GL_APIENTRY *p4gl_glDeleteFramebuffers)(GLsizei n, const GLuint *framebuffers);
void (P4GL_APIENTRY *p4gl_glDeleteRenderbuffers)(GLsizei n, const GLuint *renderbuffers);
void (P4GL_APIENTRY *p4gl_glBindRenderbuffer)(GLenum target, GLuint renderbuffer);
void (P4GL_APIENTRY *p4gl_glRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
void (P4GL_APIENTRY *p4gl_glFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
void (P4GL_APIENTRY *p4gl_glDeleteBuffers)(GLsizei n, const GLuint *buffers);
void (P4GL_APIENTRY *p4gl_glDepthFunc)(GLenum func);
void (P4GL_APIENTRY *p4gl_glDepthMask)(GLboolean flag);
void (P4GL_APIENTRY *p4gl_glCullFace)(GLenum mode);
void (P4GL_APIENTRY *p4gl_glBlitFramebuffer)(GLint sx0, GLint sy0, GLint sx1, GLint sy1, GLint dx0, GLint dy0,
                               GLint dx1, GLint dy1, GLbitfield mask, GLenum filter);
void (P4GL_APIENTRY *p4gl_glFrontFace)(GLenum mode);
void (P4GL_APIENTRY *p4gl_glColorMask)(GLboolean r, GLboolean g, GLboolean b, GLboolean a);
void (P4GL_APIENTRY *p4gl_glBlendEquation)(GLenum mode);
void (P4GL_APIENTRY *p4gl_glBlendFuncSeparate)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
void (P4GL_APIENTRY *p4gl_glBlendColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void (P4GL_APIENTRY *p4gl_glUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void (P4GL_APIENTRY *p4gl_glUniform3fv)(GLint location, GLsizei count, const GLfloat *value);
void (P4GL_APIENTRY *p4gl_glUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void (P4GL_APIENTRY *p4gl_glUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void (P4GL_APIENTRY *p4gl_glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
const GLubyte *(P4GL_APIENTRY *p4gl_glGetString)(GLenum name);
const GLubyte *(P4GL_APIENTRY *p4gl_glGetStringi)(GLenum name, GLuint index);
void (P4GL_APIENTRY *p4gl_glGetIntegerv)(GLenum pname, GLint *data);

static int sIsEs;
static char sRenderer[200];   /* GL_RENDERER of the loaded context (context_probe) */
static int sBorderNative = 1;

int p4gl_gl_is_es(void)
{
    return sIsEs;
}

int p4gl_gl_border_native(void)
{
    return sBorderNative;
}

/* GLSL for the context. ES needs a precision for every type a shader uses: highp everywhere, as desktop GL computes (a fragment
 * shader may always use highp in ES 3.0, GL_FRAGMENT_PRECISION_HIGH is guaranteed). The fragment stage also carries p4SampleBorder, the
 * emulation of GL_CLAMP_TO_BORDER on a context without it: `mode` packs the wrap mode of each axis (bits 0-1 U, 2-3 V: 0 repeat, 1 mirror,
 * 2 clamp to edge, 3 transparent-black border), bit 4 linear filtering. It fetches texels itself and weights the border ones as zero, which
 * is what the hardware's border does. Unused when the uniform that feeds it is 0, so draws that do not use a border never run it. */
static const char kGlslHeaderGl[] = "#version 330 core\n";
static const char kGlslHeaderEs[] =
    "#version 300 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "precision highp sampler2D;\n";
static const char kGlslBorder[] =
    "int p4Mod(int i, int n) { return i - n * int(floor(float(i) / float(n))); }\n"
    "int p4Wrap(int i, int n, int mode, out bool zero) {\n"
    "    zero = false;\n"
    "    if (mode == 3) { zero = i < 0 || i >= n; return clamp(i, 0, n - 1); }\n"
    "    if (mode == 2) { return clamp(i, 0, n - 1); }\n"
    "    if (mode == 1) { int p = 2 * n; int m = p4Mod(i, p); return m < n ? m : p - 1 - m; }\n"
    "    return p4Mod(i, n);\n"
    "}\n"
    "vec4 p4Texel(sampler2D t, ivec2 i, ivec2 n, ivec2 m) {\n"
    "    bool zx;\n"
    "    bool zy;\n"
    "    ivec2 c = ivec2(p4Wrap(i.x, n.x, m.x, zx), p4Wrap(i.y, n.y, m.y, zy));\n"
    "    return (zx || zy) ? vec4(0.0) : texelFetch(t, c, 0);\n"
    "}\n"
    "vec4 p4SampleBorder(sampler2D t, vec2 uv, int mode) {\n"
    "    ivec2 n = textureSize(t, 0);\n"
    "    ivec2 m = ivec2(mode & 3, (mode >> 2) & 3);\n"
    "    vec2 p = uv * vec2(n);\n"
    "    if ((mode & 16) == 0) { return p4Texel(t, ivec2(floor(p)), n, m); }\n"
    "    p -= 0.5;\n"
    "    vec2 f = fract(p);\n"
    "    ivec2 i = ivec2(floor(p));\n"
    "    vec4 a = mix(p4Texel(t, i, n, m), p4Texel(t, i + ivec2(1, 0), n, m), f.x);\n"
    "    vec4 b = mix(p4Texel(t, i + ivec2(0, 1), n, m), p4Texel(t, i + ivec2(1, 1), n, m), f.x);\n"
    "    return mix(a, b, f.y);\n"
    "}\n";

/* 1 when the context converts a shader's float colour to the 8-bit framebuffer by rounding (hardware drivers), 0 when it truncates (Mesa's llvmpipe on a
 * thread whose MXCSR is the EE's round toward zero). The GS truncates its colour, so on a rounding context every fragment shader truncates the colour
 * itself (P4_ROUND_BIAS, kGlslRoundBias): every scene gate's MAD is 0.2 to 0.3 points lower on NVIDIA hardware, as on llvmpipe. Probed once per context. */
static int sRoundBias;
/* The GS's truncation on a rounding context. floor(255 c + 1/512) is the GS's 8-bit colour: the 1/512 step takes in the float error of a colour that is
 * a whole step, and every fraction the GS truncates (multiples of 1/256) stays below the next step. The shader writes that step less 3/8. Rounding drivers
 * do not split a step at its half: NVIDIA 610.88 turns n - 1/2 + d into n - 1 for d up to 1/16 of a step (measured for every n, renderer_port.md 10.6a),
 * so the half step this used to take off put every whole-step colour one step low (white wrote 254, a movie frame was one step dark on 82 % of its
 * channels). n - 3/8 is n on any conversion within 1/8 step of round to nearest. Blending stays the driver's: NVIDIA blends the 8-bit source and rounds
 * where the GS truncates. */
static const char kGlslRoundBias[] =
    "#define P4_ROUND_BIAS 1\n"
    "vec3 p4GsTruncate(vec3 c) { return max(floor(c * 255.0 + 1.0 / 512.0) - 0.375, 0.0) / 255.0; }\n";
/* A colour's alpha on the GS's scale (0x80 is 1.0) for the alpha test and AA1. An alpha within 1/256 of a whole GS value is that value: the GS compares
 * integers, and a vertex alpha interpolated across a primitive is not exact on every GPU (on AMD's radeonsi a constant 1.0 reads a hair below 128, which
 * failed GEQUAL 0x80 and dropped AA1's coverage). Other fractions compare as they are. */
static const char kGlslGsAlpha[] =
    "float p4GsAlpha(float a) { float t = a * 128.0, n = floor(t + 0.5); return abs(t - n) < 1.0 / 256.0 ? n : t; }\n";
/* 1 when a pixel centre exactly on the edge of a quad falls on another side than the GS's top-left rule puts it: the Im2D vertex shader then moves its
 * vertices a little up and left and the fragment shader takes that off the texture coordinates again (P4_TIE_BIAS). llvmpipe and an Adreno 650 include the
 * bottom edge of the picture and leave out the top one, so every sprite and letterbox bar was a row low at integer coordinates. Probed once per context
 * (probe_edge_rule). */
static int sTieBias;

int p4gl_gl_round_bias(void)
{
    return sRoundBias;
}

GLuint p4gl_gl_compile_shader(GLenum stage, const char *body, const char *what)
{
    const GLchar *sources[6];
    GLint count = 0;
    GLint ok = GL_FALSE;
    GLuint shader = p4gl_glCreateShader(stage);

    sources[count++] = sIsEs ? kGlslHeaderEs : kGlslHeaderGl;
    if (sTieBias) {
        sources[count++] = "#define P4_TIE_BIAS 1\n";
    }
    if (stage == GL_FRAGMENT_SHADER) {
        if (sRoundBias) {
            sources[count++] = kGlslRoundBias;
        }
        sources[count++] = kGlslGsAlpha;
        sources[count++] = kGlslBorder;
    }
    sources[count++] = body;
    p4gl_glShaderSource(shader, count, sources, NULL);
    p4gl_glCompileShader(shader);
    p4gl_glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        GLchar log[1024];
        GLsizei length = 0;

        p4gl_glGetShaderInfoLog(shader, (GLsizei)sizeof(log), &length, log);
        fprintf(stderr, "p4gl: %s shader compile failed: %s\n", what, log);
        return 0;
    }
    return shader;
}

/* Reads the context's identity (ES or desktop, its version, whether a texture border is available) once the entry points are loaded. */
static void context_probe(void)
{
    const char *version = (const char *)p4gl_glGetString(GL_VERSION);
    const char *renderer = (const char *)p4gl_glGetString(GL_RENDERER);
    const char *vendor = (const char *)p4gl_glGetString(GL_VENDOR);
    const char *force = getenv("P4PC_GLES_BORDER");
    int major = 0, minor = 0;
    int n = 0, i;

    if (version == NULL) {
        version = "";
    }
    snprintf(sRenderer, sizeof sRenderer, "%s", renderer != NULL ? renderer : "");
    sIsEs = strncmp(version, "OpenGL ES", 9) == 0;
    sscanf(version + (sIsEs ? 10 : 0), "%d.%d", &major, &minor);
    sBorderNative = 1;
    if (sIsEs && (major < 3 || (major == 3 && minor < 2))) {
        sBorderNative = 0;
        p4gl_glGetIntegerv(GL_NUM_EXTENSIONS, &n);
        for (i = 0; i < n; i++) {
            const char *e = (const char *)p4gl_glGetStringi(GL_EXTENSIONS, (GLuint)i);

            if (e != NULL && (strcmp(e, "GL_EXT_texture_border_clamp") == 0 || strcmp(e, "GL_OES_texture_border_clamp") == 0)) {
                sBorderNative = 1;
            }
        }
    }
    if (force != NULL && strcmp(force, "shader") == 0) {
        sBorderNative = 0;
    } else if (force != NULL && strcmp(force, "native") == 0) {
        sBorderNative = 1;
    }
    fprintf(stderr, "p4gl: %s, %s, %s (%s, texture border %s)\n", version, renderer != NULL ? renderer : "?", vendor != NULL ? vendor : "?",
            sIsEs ? "OpenGL ES" : "desktop OpenGL", sBorderNative ? "hardware" : "shader");
}

static int contains_nocase(const char *hay, const char *needle)
{
    size_t n = strlen(needle), i, j;

    for (i = 0; hay[i] != '\0'; i++) {
        for (j = 0; j < n; j++) {
            char a = hay[i + j], b = needle[j];

            if (a == '\0') {
                return 0;
            }
            if (a >= 'A' && a <= 'Z') {
                a = (char)(a - 'A' + 'a');
            }
            if (b >= 'A' && b <= 'Z') {
                b = (char)(b - 'A' + 'a');
            }
            if (a != b) {
                break;
            }
        }
        if (j == n) {
            return 1;
        }
    }
    return 0;
}

/* The limit (percent, as text) a gate judges a frame by. envName (P4PC_REF_MAX, P4PC_CAPTURE_REF_MAX) holds the CTest limit.
 * tests/gate_limits.txt (P4PC_GATE_LIMITS) lists limits for specific renderers, one row per gate (P4PC_GATE_NAME)
 * and renderer: `<gate> <substring of GL_RENDERER, any case> <limit>`. The first matching row wins; no row,
 * no file or no context: the CTest limit. */
const char *p4gl_gate_limit(const char *envName)
{
    static char text[40];
    const char *base = getenv(envName);
    const char *path = getenv("P4PC_GATE_LIMITS");
    const char *gate = getenv("P4PC_GATE_NAME");
    FILE *f;
    char line[256];

    if (base == NULL || path == NULL || gate == NULL || sRenderer[0] == '\0' || (f = fopen(path, "r")) == NULL) {
        return base;
    }
    while (fgets(line, sizeof line, f) != NULL) {
        char name[96], pat[96], lim[40];
        char *hash = strchr(line, '#');

        if (hash != NULL) {
            *hash = '\0';
        }
        if (sscanf(line, "%95s %95s %39s", name, pat, lim) == 3 && strcmp(name, gate) == 0 && contains_nocase(sRenderer, pat)) {
            snprintf(text, sizeof text, "%s", lim);
            fclose(f);
            fprintf(stderr, "p4gl: gate %s: limit %s for a renderer matching '%s' (the ctest limit is %s)\n", gate, text, pat, base);
            return text;
        }
    }
    fclose(f);
    return base;
}

/* Resolves one entry point. Prints the missing name; returns 0 on success. */
static int p4gl_resolve(void **slot, const char *name)
{
    *slot = (void *)p4gl_win_proc(name);
    if (*slot == NULL) {
        fprintf(stderr, "p4gl: GL entry point %s not available\n", name);
        return 1;
    }
    return 0;
}

/* P4PC_TURBO=2: draw calls outside the flips just before a captured frame are dropped (p4gl_gl_set_draw), the rasteriser being
 * nearly all of an unattended run's time. Clears and draws are skipped; uploads, blits and state calls still run, so the
 * textures and render targets a later frame samples are valid. */
static void (P4GL_APIENTRY *sRealClear)(GLbitfield mask);
static void (P4GL_APIENTRY *sRealDrawArrays)(GLenum mode, GLint first, GLsizei count);
static void (P4GL_APIENTRY *sRealDrawArraysInstanced)(GLenum, GLint, GLsizei, GLsizei);
static void (P4GL_APIENTRY *sRealDrawElements)(GLenum mode, GLsizei count, GLenum type, const void *indices);

static void P4GL_APIENTRY skipClear(GLbitfield mask) { (void)mask; }
static void P4GL_APIENTRY skipDrawArrays(GLenum mode, GLint first, GLsizei count) { (void)mode; (void)first; (void)count; }
static void P4GL_APIENTRY skipDrawArraysInstanced(GLenum m, GLint f, GLsizei n, GLsizei instances)
{
    (void)m; (void)f; (void)n; (void)instances;
}
static void P4GL_APIENTRY skipDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
    (void)mode;
    (void)count;
    (void)type;
    (void)indices;
}

static int sDrawOn = 1;

int p4gl_gl_draw_enabled(void)
{
    return sDrawOn;
}

int p4gl_gl_prepare_draw(void)
{
    static int observed = -1;

    if (sDrawOn) {
        return 1;
    }
    if (observed < 0) {
        observed = getenv("P4PC_DRAW_LOG") != NULL || getenv("P4PC_DRAW_UPTO") != NULL ||
                   getenv("P4PC_DRAW_SKIP") != NULL || getenv("P4PC_GS_LOG") != NULL ||
                   getenv("P4PC_GS_SKIP") != NULL || getenv("P4PC_GLSTATS") != NULL;
    }
    return observed;
}

static int sDrawSuppressed;

static void apply_draw_pointers(void)
{
    const int on = sDrawOn && !sDrawSuppressed;
    if (sRealClear == NULL) {
        sRealClear = p4gl_glClear;
        sRealDrawArrays = p4gl_glDrawArrays;
        sRealDrawArraysInstanced = p4gl_glDrawArraysInstanced;
        sRealDrawElements = p4gl_glDrawElements;
    }
    p4gl_glClear = on ? sRealClear : skipClear;
    p4gl_glDrawArrays = on ? sRealDrawArrays : skipDrawArrays;
    p4gl_glDrawArraysInstanced = on ? sRealDrawArraysInstanced : skipDrawArraysInstanced;
    p4gl_glDrawElements = on ? sRealDrawElements : skipDrawElements;
}

void p4gl_gl_set_draw(int on)
{
    sDrawOn = on != 0;
    apply_draw_pointers();
}

/* Render-only suppression for hidden ordering-table scopes (mod HUD groups,
 * p4gl_im2d.c): clears and draws are dropped like P4PC_TURBO=2's, while the
 * game's callbacks, glyph/sprite bookkeeping, uploads and state still run. */
void p4gl_gl_suppress_draw(int suppressed)
{
    if (sDrawSuppressed == (suppressed != 0)) return;
    sDrawSuppressed = suppressed != 0;
    apply_draw_pointers();
}

/* P4PC_GLSTATS=1: counts of the GL calls the frames make, printed every 1000 presented frames by p4gl_gl_stats_report (called
 * from the flip, src/render/p4gl_context.c). The counting wrappers sit on the function pointers, so they cost nothing when off. */
static struct {
    unsigned draws, bufferData, bufferBytes, texParam, texImage, copyTex, useProgram, bindTexture, uniforms;
} sStats;
static int sStatsOn;
static FILE *sHitchTrace;

static void hitch_note_at(const char *kind, uint64_t start, uint64_t end, unsigned object, int width, int height)
{
    if (!sHitchTrace || !start) return;
    fprintf(sHitchTrace, "%s,%llu,%llu,%u,%d,%d\n", kind, (unsigned long long)start,
            (unsigned long long)(end - start), object, width, height);
}

static void hitch_note(const char *kind, uint64_t start, unsigned object, int width, int height)
{
    if (sHitchTrace && start)
        hitch_note_at(kind, start, p4gl_profile_now_ns(), object, width, height);
}

uint64_t p4gl_gl_hitch_begin(void)
{
    return sHitchTrace ? p4gl_profile_now_ns() : 0;
}

void p4gl_gl_hitch_api(const char *kind, uint64_t start, GLuint object, int width, int height)
{
    if (sHitchTrace && start) {
        const uint64_t end = p4gl_profile_now_ns();
        if (end - start >= 1000000ull)
            hitch_note_at(kind, start, end, object, width, height);
    }
}

static void (P4GL_APIENTRY *rCompileShader)(GLuint);
static void (P4GL_APIENTRY *rLinkProgram)(GLuint);
static void P4GL_APIENTRY cCompileShader(GLuint shader)
{
    const uint64_t start = p4gl_profile_now_ns();
    rCompileShader(shader);
    hitch_note("compile_shader_api", start, shader, 0, 0);
}
static void P4GL_APIENTRY cLinkProgram(GLuint program)
{
    const uint64_t start = p4gl_profile_now_ns();
    rLinkProgram(program);
    hitch_note("link_program_api", start, program, 0, 0);
}

static void (P4GL_APIENTRY *rDrawArrays)(GLenum, GLint, GLsizei);
static void (P4GL_APIENTRY *rDrawArraysInstanced)(GLenum, GLint, GLsizei, GLsizei);
static void (P4GL_APIENTRY *rDrawElements)(GLenum, GLsizei, GLenum, const void *);
static void (P4GL_APIENTRY *rBufferData)(GLenum, GLsizeiptr, const void *, GLenum);
static void (P4GL_APIENTRY *rTexParameteri)(GLenum, GLenum, GLint);
static void (P4GL_APIENTRY *rTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *);
static void (P4GL_APIENTRY *rTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void *);
static void (P4GL_APIENTRY *rCopyTexSubImage2D)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
static void (P4GL_APIENTRY *rUseProgram)(GLuint);
static void (P4GL_APIENTRY *rBindTexture)(GLenum, GLuint);
static void (P4GL_APIENTRY *rUniform1i)(GLint, GLint);
static void (P4GL_APIENTRY *rUniform1f)(GLint, GLfloat);
static void (P4GL_APIENTRY *rUniform2f)(GLint, GLfloat, GLfloat);
static void (P4GL_APIENTRY *rClear)(GLbitfield);
static void (P4GL_APIENTRY *rBlitFramebuffer)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
static void (P4GL_APIENTRY *rFlush)(void);
static void (P4GL_APIENTRY *rReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *);
static GLenum (P4GL_APIENTRY *rGetError)(void);
static void P4GL_APIENTRY cFlush(void)
{
    const uint64_t start = p4gl_profile_now_ns();
    rFlush();
    p4gl_gl_hitch_api("flush_api", start, 0, 0, 0);
}
static void P4GL_APIENTRY cReadPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum t, void *p)
{
    const uint64_t start = p4gl_profile_now_ns();
    rReadPixels(x, y, w, h, f, t, p);
    p4gl_gl_hitch_api("read_pixels_api", start, f, w, h);
}
static GLenum P4GL_APIENTRY cGetError(void)
{
    const uint64_t start = p4gl_profile_now_ns();
    GLenum result = rGetError();
    p4gl_gl_hitch_api("get_error_api", start, result, 0, 0);
    return result;
}
static void P4GL_APIENTRY cClear(GLbitfield mask)
{
    p4gl_profile_touch();
    const uint64_t start = sHitchTrace ? p4gl_profile_now_ns() : 0;
    rClear(mask);
    if (start) p4gl_gl_hitch_api("clear_api", start, mask, 0, 0);
}
static void P4GL_APIENTRY cBlitFramebuffer(GLint a, GLint b, GLint c, GLint d, GLint e, GLint f, GLint g, GLint h, GLbitfield mask, GLenum filter)
{
    const unsigned previous = p4gl_profile_feedback_begin();
    p4gl_profile_touch();
    const uint64_t start = sHitchTrace ? p4gl_profile_now_ns() : 0;
    rBlitFramebuffer(a, b, c, d, e, f, g, h, mask, filter);
    if (start) p4gl_gl_hitch_api("blit_framebuffer_api", start, mask, 0, 0);
    p4gl_profile_feedback_end(previous);
}
static void P4GL_APIENTRY cDrawArrays(GLenum m, GLint f, GLsizei n)
{
    p4gl_profile_touch(); sStats.draws++;
    const uint64_t start = sHitchTrace ? p4gl_profile_now_ns() : 0;
    rDrawArrays(m, f, n);
    if (start) p4gl_gl_hitch_api("draw_arrays_api", start, m, n, 1);
}
static void P4GL_APIENTRY cDrawArraysInstanced(GLenum m, GLint f, GLsizei n, GLsizei instances)
{
    p4gl_profile_touch(); sStats.draws++;
    const uint64_t start = sHitchTrace ? p4gl_profile_now_ns() : 0;
    rDrawArraysInstanced(m, f, n, instances);
    if (start) p4gl_gl_hitch_api("draw_arrays_instanced_api", start, m, n, instances);
}
static void P4GL_APIENTRY cDrawElements(GLenum m, GLsizei n, GLenum t, const void *i)
{
    p4gl_profile_touch(); sStats.draws++;
    const uint64_t start = sHitchTrace ? p4gl_profile_now_ns() : 0;
    rDrawElements(m, n, t, i);
    if (start) p4gl_gl_hitch_api("draw_elements_api", start, m, n, 1);
}
static void P4GL_APIENTRY cBufferData(GLenum t, GLsizeiptr s, const void *d, GLenum u)
{
    p4gl_profile_touch(); sStats.bufferData++; sStats.bufferBytes += (unsigned)s;
    const uint64_t start = sHitchTrace ? p4gl_profile_now_ns() : 0;
    rBufferData(t, s, d, u);
    if (start) p4gl_gl_hitch_api(d ? "buffer_data_api" : "buffer_orphan_api", start, t, (int)s, (int)u);
}
static void P4GL_APIENTRY cTexParameteri(GLenum t, GLenum p, GLint v) { sStats.texParam++; rTexParameteri(t, p, v); }
static void P4GL_APIENTRY cTexImage2D(GLenum t, GLint l, GLint i, GLsizei w, GLsizei h, GLint b, GLenum f, GLenum ty, const void *p)
{
    const uint64_t start = sHitchTrace != NULL ? p4gl_profile_now_ns() : 0;
    p4gl_profile_touch();
    sStats.texImage++;
    rTexImage2D(t, l, i, w, h, b, f, ty, p);
    if (sHitchTrace != NULL) hitch_note("tex_image_api", start, 0, w, h);
}
static void P4GL_APIENTRY cTexSubImage2D(GLenum t, GLint l, GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum ty, const void *p)
{
    const uint64_t start = sHitchTrace != NULL ? p4gl_profile_now_ns() : 0;
    p4gl_profile_touch();
    sStats.texImage++;
    rTexSubImage2D(t, l, x, y, w, h, f, ty, p);
    if (sHitchTrace != NULL) hitch_note("tex_subimage_api", start, 0, w, h);
}
static void P4GL_APIENTRY cCopyTexSubImage2D(GLenum t, GLint l, GLint xo, GLint yo, GLint x, GLint y, GLsizei w, GLsizei h)
{
    const unsigned previous = p4gl_profile_feedback_begin();
    p4gl_profile_touch();
    sStats.copyTex++;
    const uint64_t start = sHitchTrace ? p4gl_profile_now_ns() : 0;
    rCopyTexSubImage2D(t, l, xo, yo, x, y, w, h);
    if (start) p4gl_gl_hitch_api("copy_tex_subimage_api", start, t, w, h);
    p4gl_profile_feedback_end(previous);
}
static void P4GL_APIENTRY cUseProgram(GLuint p) { sStats.useProgram++; rUseProgram(p); }
static void P4GL_APIENTRY cBindTexture(GLenum t, GLuint n) { sStats.bindTexture++; rBindTexture(t, n); }
static void P4GL_APIENTRY cUniform1i(GLint l, GLint v) { sStats.uniforms++; rUniform1i(l, v); }
static void P4GL_APIENTRY cUniform1f(GLint l, GLfloat v) { sStats.uniforms++; rUniform1f(l, v); }
static void P4GL_APIENTRY cUniform2f(GLint l, GLfloat a, GLfloat b) { sStats.uniforms++; rUniform2f(l, a, b); }

static void stats_install(void)
{
    sStatsOn = 1;
    rClear = p4gl_glClear; p4gl_glClear = cClear;
    rBlitFramebuffer = p4gl_glBlitFramebuffer; p4gl_glBlitFramebuffer = cBlitFramebuffer;
    const char *trace = getenv("P4PC_GL_HITCH_TRACE");
    if (sHitchTrace == NULL && trace != NULL && *trace != '\0') {
        sHitchTrace = fopen(trace, "w");
        if (sHitchTrace != NULL) {
            setvbuf(sHitchTrace, NULL, _IOFBF, 65536);
            fprintf(sHitchTrace, "kind,timestamp_ns,duration_ns,object,width,height\n");
        } else {
            fprintf(stderr, "p4gl: cannot open hitch trace %s\n", trace);
        }
    }
    if (sHitchTrace != NULL) {
        rCompileShader = p4gl_glCompileShader; p4gl_glCompileShader = cCompileShader;
        rLinkProgram = p4gl_glLinkProgram; p4gl_glLinkProgram = cLinkProgram;
        rFlush = p4gl_glFlush; p4gl_glFlush = cFlush;
        rReadPixels = p4gl_glReadPixels; p4gl_glReadPixels = cReadPixels;
        rGetError = p4gl_glGetError; p4gl_glGetError = cGetError;
    }
    rDrawArrays = p4gl_glDrawArrays; p4gl_glDrawArrays = cDrawArrays;
    rDrawArraysInstanced = p4gl_glDrawArraysInstanced; p4gl_glDrawArraysInstanced = cDrawArraysInstanced;
    rDrawElements = p4gl_glDrawElements; p4gl_glDrawElements = cDrawElements;
    rBufferData = p4gl_glBufferData; p4gl_glBufferData = cBufferData;
    rTexParameteri = p4gl_glTexParameteri; p4gl_glTexParameteri = cTexParameteri;
    rTexImage2D = p4gl_glTexImage2D; p4gl_glTexImage2D = cTexImage2D;
    rTexSubImage2D = p4gl_glTexSubImage2D; p4gl_glTexSubImage2D = cTexSubImage2D;
    rCopyTexSubImage2D = p4gl_glCopyTexSubImage2D; p4gl_glCopyTexSubImage2D = cCopyTexSubImage2D;
    rUseProgram = p4gl_glUseProgram; p4gl_glUseProgram = cUseProgram;
    rBindTexture = p4gl_glBindTexture; p4gl_glBindTexture = cBindTexture;
    rUniform1i = p4gl_glUniform1i; p4gl_glUniform1i = cUniform1i;
    rUniform1f = p4gl_glUniform1f; p4gl_glUniform1f = cUniform1f;
    rUniform2f = p4gl_glUniform2f; p4gl_glUniform2f = cUniform2f;
}

/* P4PC_GLERR=1: glGetError after every draw, clear, copy, blit and texture upload, printed with the call's name (first 40 only). For bringing up a new GL
 * implementation (ANGLE on Metal, a phone driver): the first INVALID_OPERATION names the call that the driver refuses. Installed outermost. */
static int sErrPrinted;
static void (P4GL_APIENTRY *eDrawArrays)(GLenum, GLint, GLsizei);
static void (P4GL_APIENTRY *eDrawArraysInstanced)(GLenum, GLint, GLsizei, GLsizei);
static void (P4GL_APIENTRY *eDrawElements)(GLenum, GLsizei, GLenum, const void *);
static void (P4GL_APIENTRY *eClear)(GLbitfield);
static void (P4GL_APIENTRY *eCopyTexSubImage2D)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
static void (P4GL_APIENTRY *eBlitFramebuffer)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
static void (P4GL_APIENTRY *eTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *);
static void (P4GL_APIENTRY *eTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void *);
static void err_check(const char *what)
{
    GLenum e;

    while ((e = p4gl_glGetError()) != GL_NO_ERROR) {
        if (sErrPrinted++ < 40) {
            fprintf(stderr, "p4gl: GL error 0x%x after %s\n", e, what);
        }
    }
}
static void P4GL_APIENTRY xDrawArrays(GLenum m, GLint f, GLsizei n) { eDrawArrays(m, f, n); err_check("glDrawArrays"); }
static void P4GL_APIENTRY xDrawArraysInstanced(GLenum m, GLint f, GLsizei n, GLsizei instances)
{
    eDrawArraysInstanced(m, f, n, instances); err_check("glDrawArraysInstanced");
}
static void P4GL_APIENTRY xDrawElements(GLenum m, GLsizei n, GLenum t, const void *i) { eDrawElements(m, n, t, i); err_check("glDrawElements"); }
static void P4GL_APIENTRY xClear(GLbitfield m) { eClear(m); err_check("glClear"); }
static void P4GL_APIENTRY xCopyTexSubImage2D(GLenum t, GLint l, GLint xo, GLint yo, GLint x, GLint y, GLsizei w, GLsizei h) { eCopyTexSubImage2D(t, l, xo, yo, x, y, w, h); err_check("glCopyTexSubImage2D"); }
static void P4GL_APIENTRY xBlitFramebuffer(GLint a, GLint b, GLint c, GLint d, GLint e, GLint f, GLint g, GLint h, GLbitfield m, GLenum fl) { eBlitFramebuffer(a, b, c, d, e, f, g, h, m, fl); err_check("glBlitFramebuffer"); }
static void P4GL_APIENTRY xTexImage2D(GLenum t, GLint l, GLint i, GLsizei w, GLsizei h, GLint b, GLenum f, GLenum ty, const void *p) { eTexImage2D(t, l, i, w, h, b, f, ty, p); err_check("glTexImage2D"); }
static void P4GL_APIENTRY xTexSubImage2D(GLenum t, GLint l, GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum ty, const void *p) { eTexSubImage2D(t, l, x, y, w, h, f, ty, p); err_check("glTexSubImage2D"); }

static void errcheck_install(void)
{
    eDrawArrays = p4gl_glDrawArrays; p4gl_glDrawArrays = xDrawArrays;
    eDrawArraysInstanced = p4gl_glDrawArraysInstanced; p4gl_glDrawArraysInstanced = xDrawArraysInstanced;
    eDrawElements = p4gl_glDrawElements; p4gl_glDrawElements = xDrawElements;
    eClear = p4gl_glClear; p4gl_glClear = xClear;
    eCopyTexSubImage2D = p4gl_glCopyTexSubImage2D; p4gl_glCopyTexSubImage2D = xCopyTexSubImage2D;
    eBlitFramebuffer = p4gl_glBlitFramebuffer; p4gl_glBlitFramebuffer = xBlitFramebuffer;
    eTexImage2D = p4gl_glTexImage2D; p4gl_glTexImage2D = xTexImage2D;
    eTexSubImage2D = p4gl_glTexSubImage2D; p4gl_glTexSubImage2D = xTexSubImage2D;
}

/* Per-frame averages since the last report, with the time the caller spent in the swap and in the whole present. */
void p4gl_gl_stats_report(unsigned frames, double swapMs, double presentMs)
{
    if (!sStatsOn) {
        return;
    }
    fprintf(stderr,
            "p4gl stats: %u frames: per frame %.1f draws, %.1f buffer uploads (%.1f KB), %.1f tex params, %.1f tex uploads, %.1f fb copies, %.1f program switches, %.1f texture binds, %.1f uniforms; swap %.3f ms, present %.3f ms\n",
            frames, (double)sStats.draws / frames, (double)sStats.bufferData / frames, (double)sStats.bufferBytes / 1024.0 / frames,
            (double)sStats.texParam / frames, (double)sStats.texImage / frames, (double)sStats.copyTex / frames,
            (double)sStats.useProgram / frames, (double)sStats.bindTexture / frames, (double)sStats.uniforms / frames, swapMs / frames, presentMs / frames);
    if (sHitchTrace != NULL) fflush(sHitchTrace);
    memset(&sStats, 0, sizeof sStats);
}

/* Redundant-state filter. The 2D and 3D paths set the whole state of a draw (program, uniforms, texture and its four sampler
 * parameters, blend, depth, colour mask) every time, 72 draws and 170 sampler parameters a frame in a menu; a GL driver pays for
 * each call whether or not the state changed (about 40% of the frame time in a menu, measured on an RTX 4080: 870 to 1500 flips/s).
 * These wrappers remember what the context holds and pass a call on only when it changes something (program, uniforms, textures,
 * sampler parameters, buffer and vertex array bindings, viewport). The cache starts as "unknown", so the first call always passes.
 * Only state this file's pointers set is tracked: nothing else in the process issues GL calls. Fixed-function state is lazy, below.
 * P4PC_GLCACHE=0 turns the whole layer off.
 *
 * Tried and dropped, no gain on the same measurement: a ring buffer with glBufferSubData instead of glBufferData per draw, and merging
 * consecutive draws of equal state into one (41 draws a frame instead of 72, but the extra vertex copies cost as much as the calls saved). */
enum { C_PROGS = 64, C_LOCS = 48, C_TEXS = 8192, C_UNITS = 4 };
static struct { uint32_t v[4]; unsigned char kind, valid; } sU[C_PROGS][C_LOCS];
static GLuint sProg = 0xFFFFFFFFu, sBoundVao = 0xFFFFFFFFu, sBoundArray = 0xFFFFFFFFu;
static GLuint sTexBound[C_UNITS] = {0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu};
static unsigned sUnit;
static struct { GLint s, t, min, mag; } sTexParams[C_TEXS];
static GLenum sCapName[8];
static GLint sViewport[4];
static int sViewportSet;
static void (P4GL_APIENTRY *kUseProgram)(GLuint), (P4GL_APIENTRY *kBindTexture)(GLenum, GLuint), (P4GL_APIENTRY *kActiveTexture)(GLenum);
static void (P4GL_APIENTRY *kTexParameteri)(GLenum, GLenum, GLint), (P4GL_APIENTRY *kDeleteTextures)(GLsizei, const GLuint *);
static void (P4GL_APIENTRY *kUniform1i)(GLint, GLint), (P4GL_APIENTRY *kUniform1f)(GLint, GLfloat), (P4GL_APIENTRY *kUniform2f)(GLint, GLfloat, GLfloat);
static void (P4GL_APIENTRY *kUniform3f)(GLint, GLfloat, GLfloat, GLfloat), (P4GL_APIENTRY *kUniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
static void (P4GL_APIENTRY *kBindBuffer)(GLenum, GLuint), (P4GL_APIENTRY *kBindVertexArray)(GLuint), (P4GL_APIENTRY *kLinkProgram)(GLuint);
static void (P4GL_APIENTRY *kDeleteBuffers)(GLsizei, const GLuint *);
static void (P4GL_APIENTRY *kEnable)(GLenum), (P4GL_APIENTRY *kDisable)(GLenum), (P4GL_APIENTRY *kBlendFunc)(GLenum, GLenum);
static void (P4GL_APIENTRY *kBlendFuncSeparate)(GLenum, GLenum, GLenum, GLenum), (P4GL_APIENTRY *kBlendEquation)(GLenum);
static void (P4GL_APIENTRY *kDepthFunc)(GLenum), (P4GL_APIENTRY *kDepthMask)(GLboolean), (P4GL_APIENTRY *kCullFace)(GLenum), (P4GL_APIENTRY *kFrontFace)(GLenum);
static void (P4GL_APIENTRY *kColorMask)(GLboolean, GLboolean, GLboolean, GLboolean), (P4GL_APIENTRY *kViewport)(GLint, GLint, GLsizei, GLsizei);

/* True when the uniform at the current program's `loc` already holds these n 32-bit words of this kind (and records them if not). */
static int uniform_same(GLint loc, int kind, const void *words, int n)
{
    uint32_t w[4] = {0, 0, 0, 0};
    unsigned char *kv;

    if (sProg >= C_PROGS || loc < 0 || loc >= C_LOCS) {
        return 0;
    }
    memcpy(w, words, (size_t)n * 4u);
    if (sU[sProg][loc].valid && sU[sProg][loc].kind == kind && memcmp(sU[sProg][loc].v, w, 16) == 0) {
        return 1;
    }
    memcpy(sU[sProg][loc].v, w, 16);
    kv = &sU[sProg][loc].kind;
    *kv = (unsigned char)kind;
    sU[sProg][loc].valid = 1;
    return 0;
}
static void P4GL_APIENTRY wUseProgram(GLuint p) { if (p != sProg) { sProg = p; kUseProgram(p); } }
static void P4GL_APIENTRY wActiveTexture(GLenum u) { if (u >= GL_TEXTURE0 && u < GL_TEXTURE0 + C_UNITS) { sUnit = u - GL_TEXTURE0; } kActiveTexture(u); }
static void P4GL_APIENTRY wBindTexture(GLenum target, GLuint tex)
{
    if (target == GL_TEXTURE_2D && sUnit < C_UNITS) {
        if (sTexBound[sUnit] == tex) {
            return;
        }
        sTexBound[sUnit] = tex;
    }
    
    kBindTexture(target, tex);
}
static void P4GL_APIENTRY wDeleteTextures(GLsizei n, const GLuint *tex)
{
    GLsizei i;
    unsigned u;

    
    for (i = 0; i < n; i++) {
        if (tex[i] < C_TEXS) {
            sTexParams[tex[i]].s = sTexParams[tex[i]].t = sTexParams[tex[i]].min = sTexParams[tex[i]].mag = -1;
        }
        for (u = 0; u < C_UNITS; u++) {
            if (sTexBound[u] == tex[i]) {
                sTexBound[u] = 0;
            }
        }
    }
    kDeleteTextures(n, tex);
}
static void P4GL_APIENTRY wTexParameteri(GLenum target, GLenum pname, GLint v)
{
    GLuint tex = sUnit < C_UNITS ? sTexBound[sUnit] : 0xFFFFFFFFu;

    if (target == GL_TEXTURE_2D && tex < C_TEXS) {
        GLint *slot = pname == GL_TEXTURE_WRAP_S ? &sTexParams[tex].s : pname == GL_TEXTURE_WRAP_T ? &sTexParams[tex].t
                    : pname == GL_TEXTURE_MIN_FILTER ? &sTexParams[tex].min : pname == GL_TEXTURE_MAG_FILTER ? &sTexParams[tex].mag : NULL;
        if (slot != NULL) {
            if (*slot == v) {
                return;
            }
            *slot = v;
        }
    }
    
    kTexParameteri(target, pname, v);
}
static void P4GL_APIENTRY wUniform1i(GLint l, GLint a) { if (!uniform_same(l, 1, &a, 1)) { kUniform1i(l, a); } }
static void P4GL_APIENTRY wUniform1f(GLint l, GLfloat a) { if (!uniform_same(l, 2, &a, 1)) { kUniform1f(l, a); } }
static void P4GL_APIENTRY wUniform2f(GLint l, GLfloat a, GLfloat b) { GLfloat v[2] = {a, b}; if (!uniform_same(l, 3, v, 2)) { kUniform2f(l, a, b); } }
static void P4GL_APIENTRY wUniform3f(GLint l, GLfloat a, GLfloat b, GLfloat c) { GLfloat v[3] = {a, b, c}; if (!uniform_same(l, 4, v, 3)) { kUniform3f(l, a, b, c); } }
static void P4GL_APIENTRY wUniform4f(GLint l, GLfloat a, GLfloat b, GLfloat c, GLfloat d) { GLfloat v[4] = {a, b, c, d}; if (!uniform_same(l, 5, v, 4)) { kUniform4f(l, a, b, c, d); } }
static void P4GL_APIENTRY wLinkProgram(GLuint p)
{
    
    if (p < C_PROGS) {
        memset(sU[p], 0, sizeof sU[p]);
    }
    kLinkProgram(p);
}
static void P4GL_APIENTRY wBindBuffer(GLenum target, GLuint b)
{
    if (target == GL_ARRAY_BUFFER) {
        if (sBoundArray == b) {
            return;
        }
        sBoundArray = b;
    }
    kBindBuffer(target, b);
}
/* Deleting the buffer that is bound unbinds it in the driver; the name may then come back from glGenBuffers, and its bind must not be mistaken for a
 * repeat of the one the cache remembers (the vertex data would go to buffer 0: tests/gl_cache_test.c). */
static void P4GL_APIENTRY wDeleteBuffers(GLsizei n, const GLuint *bufs)
{
    GLsizei i;

    for (i = 0; i < n; i++) {
        if (bufs[i] == sBoundArray) {
            sBoundArray = 0;
        }
    }
    kDeleteBuffers(n, bufs);
}
static void P4GL_APIENTRY wBindVertexArray(GLuint a) { if (a != sBoundVao) { sBoundVao = a; kBindVertexArray(a); } }
/* Fixed-function state is lazy: a call only records what the game wants (the D_ values) and nothing reaches the driver until a draw, a
 * clear or a copy needs it (sync_state, which applies what differs from what the driver holds, the A_ values). The 2D path sets
 * and then restores the depth mask, colour mask, blend and cull state around every single draw, so the eager cache saw 0, 1, 0, 1 and
 * passed every call on; here a restore followed by the same setting never reaches the driver. */
static signed char sCapD[8], sCapA[8];
static GLenum sBlendD[4], sBlendA[4], sBlendEqD, sBlendEqA, sDepthFuncD, sDepthFuncA, sCullD, sCullA, sFrontD, sFrontA;
static signed char sDepthMaskD = -1, sDepthMaskA = -1;
static GLfloat sBlendColorD[4], sBlendColorA[4];
static int sBlendColorSet;
static void (P4GL_APIENTRY *kBlendColor)(GLfloat, GLfloat, GLfloat, GLfloat);
static signed char sColorMaskD[4] = {-1, -1, -1, -1}, sColorMaskA[4] = {-1, -1, -1, -1};
static void cap_set(GLenum cap, int on)
{
    int i;

    for (i = 0; i < 8; i++) {
        if (sCapName[i] == cap || sCapName[i] == 0) {
            if (sCapName[i] == 0) {
                sCapName[i] = cap;
                sCapD[i] = sCapA[i] = -1;
            }
            sCapD[i] = (signed char)on;
            return;
        }
    }
    /* more caps than the table holds: eager */
    (on ? kEnable : kDisable)(cap);
}
static void P4GL_APIENTRY wEnable(GLenum cap) { cap_set(cap, 1); }
static void P4GL_APIENTRY wDisable(GLenum cap) { cap_set(cap, 0); }
static void P4GL_APIENTRY wBlendFunc(GLenum s, GLenum d) { sBlendD[0] = s; sBlendD[1] = d; sBlendD[2] = s; sBlendD[3] = d; }
static void P4GL_APIENTRY wBlendFuncSeparate(GLenum a, GLenum b, GLenum c, GLenum d) { sBlendD[0] = a; sBlendD[1] = b; sBlendD[2] = c; sBlendD[3] = d; }
static void P4GL_APIENTRY wBlendEquation(GLenum m) { sBlendEqD = m; }
static void P4GL_APIENTRY wBlendColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    sBlendColorD[0] = r; sBlendColorD[1] = g; sBlendColorD[2] = b; sBlendColorD[3] = a;
    sBlendColorSet = 1;
}
static void P4GL_APIENTRY wDepthFunc(GLenum f) { sDepthFuncD = f; }
static void P4GL_APIENTRY wDepthMask(GLboolean f) { sDepthMaskD = (signed char)f; }
static void P4GL_APIENTRY wCullFace(GLenum m) { sCullD = m; }
static void P4GL_APIENTRY wFrontFace(GLenum m) { sFrontD = m; }
static void P4GL_APIENTRY wColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
    sColorMaskD[0] = (signed char)r; sColorMaskD[1] = (signed char)g; sColorMaskD[2] = (signed char)b; sColorMaskD[3] = (signed char)a;
}
static int state_pending(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        if (sCapName[i] != 0 && sCapD[i] >= 0 && sCapD[i] != sCapA[i]) {
            return 1;
        }
    }
    return (sBlendColorSet == 1 && memcmp(sBlendColorD, sBlendColorA, sizeof sBlendColorD) != 0) ||
           (sBlendD[0] != 0 && memcmp(sBlendD, sBlendA, sizeof sBlendD) != 0) || (sBlendEqD != 0 && sBlendEqD != sBlendEqA) ||
           (sDepthFuncD != 0 && sDepthFuncD != sDepthFuncA) || (sCullD != 0 && sCullD != sCullA) || (sFrontD != 0 && sFrontD != sFrontA) ||
           (sDepthMaskD >= 0 && sDepthMaskD != sDepthMaskA) ||
           (sColorMaskD[0] >= 0 && memcmp(sColorMaskD, sColorMaskA, sizeof sColorMaskD) != 0);
}
/* Applies the recorded fixed-function state that differs from the driver's. */
static void sync_state(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        if (sCapName[i] != 0 && sCapD[i] >= 0 && sCapD[i] != sCapA[i]) {
            (sCapD[i] ? kEnable : kDisable)(sCapName[i]);
            sCapA[i] = sCapD[i];
        }
    }
    if (sBlendD[0] != 0 && memcmp(sBlendD, sBlendA, sizeof sBlendD) != 0) {
        kBlendFuncSeparate(sBlendD[0], sBlendD[1], sBlendD[2], sBlendD[3]);
        memcpy(sBlendA, sBlendD, sizeof sBlendD);
    }
    if (sBlendColorSet == 1 && memcmp(sBlendColorD, sBlendColorA, sizeof sBlendColorD) != 0) {
        kBlendColor(sBlendColorD[0], sBlendColorD[1], sBlendColorD[2], sBlendColorD[3]);
        memcpy(sBlendColorA, sBlendColorD, sizeof sBlendColorD);
    }
    if (sBlendEqD != 0 && sBlendEqD != sBlendEqA) { kBlendEquation(sBlendEqD); sBlendEqA = sBlendEqD; }
    if (sDepthFuncD != 0 && sDepthFuncD != sDepthFuncA) { kDepthFunc(sDepthFuncD); sDepthFuncA = sDepthFuncD; }
    if (sCullD != 0 && sCullD != sCullA) { kCullFace(sCullD); sCullA = sCullD; }
    if (sFrontD != 0 && sFrontD != sFrontA) { kFrontFace(sFrontD); sFrontA = sFrontD; }
    if (sDepthMaskD >= 0 && sDepthMaskD != sDepthMaskA) { kDepthMask((GLboolean)sDepthMaskD); sDepthMaskA = sDepthMaskD; }
    if (sColorMaskD[0] >= 0 && memcmp(sColorMaskD, sColorMaskA, sizeof sColorMaskD) != 0) {
        kColorMask((GLboolean)sColorMaskD[0], (GLboolean)sColorMaskD[1], (GLboolean)sColorMaskD[2], (GLboolean)sColorMaskD[3]);
        memcpy(sColorMaskA, sColorMaskD, sizeof sColorMaskD);
    }
}
static void P4GL_APIENTRY wViewport(GLint x, GLint y, GLsizei w, GLsizei h)
{
    if (sViewportSet && sViewport[0] == x && sViewport[1] == y && sViewport[2] == w && sViewport[3] == h) {
        return;
    }
    
    sViewportSet = 1; sViewport[0] = x; sViewport[1] = y; sViewport[2] = w; sViewport[3] = h;
    kViewport(x, y, w, h);
}

/* Draws, clears and copies read the fixed-function state, so they apply the recorded state first. */
static void (P4GL_APIENTRY *kDrawArrays)(GLenum, GLint, GLsizei);
static void (P4GL_APIENTRY *kDrawArraysInstanced)(GLenum, GLint, GLsizei, GLsizei);
static void (P4GL_APIENTRY *kDrawElements)(GLenum, GLsizei, GLenum, const void *);
static void (P4GL_APIENTRY *kClear)(GLbitfield);
static void (P4GL_APIENTRY *kCopyTexSubImage2D)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
static void (P4GL_APIENTRY *kBlitFramebuffer)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
static void P4GL_APIENTRY wDrawArrays(GLenum mode, GLint first, GLsizei count) { sync_state(); kDrawArrays(mode, first, count); }
static void P4GL_APIENTRY wDrawArraysInstanced(GLenum m, GLint f, GLsizei n, GLsizei instances)
{
    sync_state(); kDrawArraysInstanced(m, f, n, instances);
}
static void P4GL_APIENTRY wDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) { sync_state(); kDrawElements(mode, count, type, indices); }
static void P4GL_APIENTRY wClear(GLbitfield m) { sync_state(); kClear(m); }
static void P4GL_APIENTRY wCopyTexSubImage2D(GLenum t, GLint l, GLint xo, GLint yo, GLint x, GLint y, GLsizei w, GLsizei h) { sync_state(); kCopyTexSubImage2D(t, l, xo, yo, x, y, w, h); }
static void P4GL_APIENTRY wBlitFramebuffer(GLint a, GLint b, GLint c, GLint d, GLint e, GLint f, GLint g, GLint h, GLbitfield m, GLenum fl) { sync_state(); kBlitFramebuffer(a, b, c, d, e, f, g, h, m, fl); }

static void cache_install(void)
{
    memset(sTexParams, 0xFF, sizeof sTexParams);
    sUnit = 0;
#define WRAP(name) k##name = p4gl_gl##name; p4gl_gl##name = w##name
    WRAP(UseProgram); WRAP(ActiveTexture); WRAP(BindTexture); WRAP(DeleteTextures); WRAP(TexParameteri);
    WRAP(Uniform1i); WRAP(Uniform1f); WRAP(Uniform2f); WRAP(Uniform3f); WRAP(Uniform4f); WRAP(LinkProgram);
    WRAP(BindBuffer); WRAP(DeleteBuffers); WRAP(BindVertexArray); WRAP(Enable); WRAP(Disable); WRAP(BlendFunc); WRAP(BlendFuncSeparate);
    kDrawArrays = p4gl_glDrawArrays; p4gl_glDrawArrays = wDrawArrays;
    kDrawArraysInstanced = p4gl_glDrawArraysInstanced; p4gl_glDrawArraysInstanced = wDrawArraysInstanced;
    kDrawElements = p4gl_glDrawElements; p4gl_glDrawElements = wDrawElements;
    kClear = p4gl_glClear; p4gl_glClear = wClear;
    kCopyTexSubImage2D = p4gl_glCopyTexSubImage2D; p4gl_glCopyTexSubImage2D = wCopyTexSubImage2D;
    kBlitFramebuffer = p4gl_glBlitFramebuffer; p4gl_glBlitFramebuffer = wBlitFramebuffer;
        WRAP(BlendEquation); WRAP(BlendColor); WRAP(DepthFunc); WRAP(DepthMask); WRAP(CullFace); WRAP(FrontFace); WRAP(ColorMask); WRAP(Viewport);
#undef WRAP
}

/* Passes of the layer's own (p4gl_gl.h). Everything saved is the cache's record: the driver already holds what the pass set, and the
 * restored recorded state reaches it at the next draw (sync_state), as for any other change of the game's wishes. */
static struct {
    int active;
    GLenum capName[8];
    signed char capD[8];
    GLenum blendD[4], blendEqD, depthFuncD, cullD, frontD;
    signed char depthMaskD, colorMaskD[4];
    GLfloat blendColorD[4];
    int blendColorSet;
    GLint viewport[4];
    int viewportSet;
    unsigned unit;
    GLuint texBound[C_UNITS], prog, vao;
} sPass;

int p4gl_gl_pass_begin(void)
{
    if (kUseProgram == NULL || sPass.active) {
        return 0; /* no cache (P4PC_GLCACHE=0), or a pass inside a pass */
    }
    memcpy(sPass.capName, sCapName, sizeof sCapName);
    memcpy(sPass.capD, sCapD, sizeof sCapD);
    memcpy(sPass.blendD, sBlendD, sizeof sBlendD);
    sPass.blendEqD = sBlendEqD; sPass.depthFuncD = sDepthFuncD; sPass.cullD = sCullD; sPass.frontD = sFrontD;
    sPass.depthMaskD = sDepthMaskD;
    memcpy(sPass.colorMaskD, sColorMaskD, sizeof sColorMaskD);
    memcpy(sPass.blendColorD, sBlendColorD, sizeof sBlendColorD);
    sPass.blendColorSet = sBlendColorSet;
    memcpy(sPass.viewport, sViewport, sizeof sViewport);
    sPass.viewportSet = sViewportSet;
    sPass.unit = sUnit;
    memcpy(sPass.texBound, sTexBound, sizeof sTexBound);
    sPass.prog = sProg;
    sPass.vao = sBoundVao;
    sPass.active = 1;
    cap_set(GL_BLEND, 0);
    cap_set(GL_DEPTH_TEST, 0);
    cap_set(0x0B44u /* GL_CULL_FACE */, 0);
    cap_set(0x0C11u /* GL_SCISSOR_TEST */, 0);
    wColorMask(1, 1, 1, 1);
    return 1;
}

void p4gl_gl_pass_end(void)
{
    unsigned u;

    if (!sPass.active) {
        return;
    }
    sPass.active = 0;
    memcpy(sCapName, sPass.capName, sizeof sCapName);
    memcpy(sCapD, sPass.capD, sizeof sCapD);
    memcpy(sBlendD, sPass.blendD, sizeof sBlendD);
    sBlendEqD = sPass.blendEqD; sDepthFuncD = sPass.depthFuncD; sCullD = sPass.cullD; sFrontD = sPass.frontD;
    sDepthMaskD = sPass.depthMaskD;
    memcpy(sColorMaskD, sPass.colorMaskD, sizeof sColorMaskD);
    memcpy(sBlendColorD, sPass.blendColorD, sizeof sBlendColorD);
    sBlendColorSet = sPass.blendColorSet;
    if (sPass.viewportSet) {
        wViewport(sPass.viewport[0], sPass.viewport[1], sPass.viewport[2], sPass.viewport[3]);
    }
    for (u = 0; u < C_UNITS; u++) {
        if (sPass.texBound[u] != 0xFFFFFFFFu && sTexBound[u] != sPass.texBound[u]) {
            wActiveTexture(GL_TEXTURE0 + u);
            wBindTexture(GL_TEXTURE_2D, sPass.texBound[u]);
        }
    }
    wActiveTexture(GL_TEXTURE0 + sPass.unit);
    if (sPass.prog != 0xFFFFFFFFu) {
        wUseProgram(sPass.prog);
    }
    if (sPass.vao != 0xFFFFFFFFu) {
        wBindVertexArray(sPass.vao);
    }
}

void p4gl_gl_context_reset(void)
{
    int i;

    memset(sU, 0, sizeof sU);
    sProg = sBoundVao = sBoundArray = 0xFFFFFFFFu;
    for (i = 0; i < C_UNITS; i++) {
        sTexBound[i] = 0xFFFFFFFFu;
    }
    memset(sTexParams, 0xFF, sizeof sTexParams);
    sUnit = 0;
    sViewportSet = 0;
    /* the recorded (D) fixed-function state is what the game asked for and stays; what the new context holds (A) is unknown */
    for (i = 0; i < 8; i++) {
        sCapA[i] = -1;
    }
    memset(sBlendA, 0, sizeof sBlendA);
    memset(sBlendColorA, 0xFF, sizeof sBlendColorA);
    sBlendEqA = sDepthFuncA = sCullA = sFrontA = 0;
    sDepthMaskA = -1;
    memset(sColorMaskA, 0xFF, sizeof sColorMaskA);
}

void p4gl_gl_clear_black(void)
{
    /* the bound framebuffer, whatever colour mask the game has recorded; the recorded mask is applied again at the next draw */
    p4gl_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    if (kClear != NULL && kColorMask != NULL) {
        kColorMask(1, 1, 1, 1);
        kClear(GL_COLOR_BUFFER_BIT);
        memset(sColorMaskA, 0xFF, sizeof sColorMaskA);
    } else {
        p4gl_glColorMask(1, 1, 1, 1);
        p4gl_glClear(GL_COLOR_BUFFER_BIT);
    }
}

/* Draws a constant 127.6/255 into a 4x4 target and reads it back: 128 is rounding, 127 truncation. P4PC_OUTPUT_ROUNDING=nearest|trunc skips it. Runs on a
 * fresh context (the entry points just loaded, the recorded state not yet applied): the draw bypasses the state cache, and every binding it made is undone. */
static void probe_output_rounding(void)
{
    static const char kVs[] =
        "void main() {\n"
        "    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));\n"
        "    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);\n"
        "}\n";
    static const char kFs[] =
        "out vec4 o;\n"
        "void main() {\n"
        "    o = vec4(vec3(127.6 / 255.0), 1.0);\n"
        "}\n";
    const char *force = getenv("P4PC_OUTPUT_ROUNDING");
    GLuint vs, fs, prog = 0, tex = 0, fbo = 0, vao = 0;
    GLint ok = GL_FALSE, vp[4] = {0, 0, 0, 0};
    unsigned char px[4] = {0, 0, 0, 0};

    sRoundBias = 0;
    if (force != NULL && force[0] != '\0') {
        sRoundBias = strcmp(force, "nearest") == 0;
        return;
    }
    vs = p4gl_gl_compile_shader(GL_VERTEX_SHADER, kVs, "output rounding probe");
    fs = p4gl_gl_compile_shader(GL_FRAGMENT_SHADER, kFs, "output rounding probe");
    if (vs == 0 || fs == 0) {
        return;
    }
    prog = p4gl_glCreateProgram();
    p4gl_glAttachShader(prog, vs);
    p4gl_glAttachShader(prog, fs);
    p4gl_glLinkProgram(prog);
    p4gl_glGetProgramiv(prog, 0x8B82u /* GL_LINK_STATUS */, &ok);
    if (ok != GL_TRUE) {
        return;
    }
    p4gl_glGetIntegerv(0x0BA2u /* GL_VIEWPORT */, vp);
    p4gl_glGenTextures(1, &tex);
    p4gl_glBindTexture(GL_TEXTURE_2D, tex);
    p4gl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    p4gl_glGenFramebuffers(1, &fbo);
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    p4gl_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    if (p4gl_glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        p4gl_glGenVertexArrays(1, &vao);
        p4gl_glBindVertexArray(vao);
        p4gl_glUseProgram(prog);
        p4gl_glViewport(0, 0, 4, 4);
        (kDrawArrays != NULL ? kDrawArrays : p4gl_glDrawArrays)(GL_TRIANGLES, 0, 3);
        p4gl_glReadPixels(1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
        p4gl_glUseProgram(0);
        p4gl_glBindVertexArray(0);
        sRoundBias = px[0] >= 128;
    }
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    p4gl_glBindTexture(GL_TEXTURE_2D, 0);
    p4gl_glViewport(vp[0], vp[1], vp[2], vp[3]);
    fprintf(stderr, "p4gl: colour output %s (127.6/255 reads back as %u)\n", sRoundBias ? "rounds, shaders truncate as the GS does" : "truncates", (unsigned)px[0]);
}

/* Which side of an edge a pixel centre exactly on it falls on. A quad from (1, 1) to (3, 3) in a 4 x 4 target, mapped as the Im2D shader maps (a vertex at an
 * integer coordinate is a pixel centre), covers the pixels 1 and 2 in each direction under the GS's top-left rule (the centre of pixel 1 is on the quad's top
 * and left edges, that of pixel 3 on its bottom and right ones). Any other coverage turns the vertex shader's bias on. */
static void probe_edge_rule(void)
{
    static const char kVs[] =
        "const vec2 kP[6] = vec2[6](vec2(1.0, 1.0), vec2(3.0, 1.0), vec2(1.0, 3.0), vec2(3.0, 1.0), vec2(3.0, 3.0), vec2(1.0, 3.0));\n"
        "void main() {\n"
        "    vec2 p = kP[gl_VertexID];\n"
        "    gl_Position = vec4((p.x + 0.5) / 4.0 * 2.0 - 1.0, 1.0 - (p.y + 0.5) / 4.0 * 2.0, 0.0, 1.0);\n"
        "}\n";
    static const char kFs[] =
        "out vec4 o;\n"
        "void main() {\n"
        "    o = vec4(1.0);\n"
        "}\n";
    GLuint vs, fs, prog = 0, tex = 0, fbo = 0, vao = 0;
    GLint ok = GL_FALSE, vp[4] = {0, 0, 0, 0};
    unsigned char px[4 * 4 * 4];
    int x;
    int y;
    int mismatch = 0;

    sTieBias = 0;
    memset(px, 0, sizeof px);
    vs = p4gl_gl_compile_shader(GL_VERTEX_SHADER, kVs, "edge rule probe");
    fs = p4gl_gl_compile_shader(GL_FRAGMENT_SHADER, kFs, "edge rule probe");
    if (vs == 0 || fs == 0) {
        return;
    }
    prog = p4gl_glCreateProgram();
    p4gl_glAttachShader(prog, vs);
    p4gl_glAttachShader(prog, fs);
    p4gl_glLinkProgram(prog);
    p4gl_glGetProgramiv(prog, 0x8B82u /* GL_LINK_STATUS */, &ok);
    if (ok != GL_TRUE) {
        return;
    }
    p4gl_glGetIntegerv(0x0BA2u /* GL_VIEWPORT */, vp);
    p4gl_glGenTextures(1, &tex);
    p4gl_glBindTexture(GL_TEXTURE_2D, tex);
    p4gl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    p4gl_glGenFramebuffers(1, &fbo);
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    p4gl_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    if (p4gl_glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        p4gl_glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        p4gl_glClear(GL_COLOR_BUFFER_BIT);
        p4gl_glGenVertexArrays(1, &vao);
        p4gl_glBindVertexArray(vao);
        p4gl_glUseProgram(prog);
        p4gl_glViewport(0, 0, 4, 4);
        (kDrawArrays != NULL ? kDrawArrays : p4gl_glDrawArrays)(GL_TRIANGLES, 0, 6);
        p4gl_glReadPixels(0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, px);
        p4gl_glUseProgram(0);
        p4gl_glBindVertexArray(0);
        for (y = 0; y < 4; y++) {
            for (x = 0; x < 4; x++) {
                const int covered = px[((3 - y) * 4 + x) * 4] != 0; /* GL's first row is the bottom: image row y is GL row 3 - y */

                mismatch |= covered != (x >= 1 && x <= 2 && y >= 1 && y <= 2);
            }
        }
        sTieBias = mismatch;
        {
            char rows[4][5];

            for (y = 0; y < 4; y++) {
                for (x = 0; x < 4; x++) {
                    rows[y][x] = px[((3 - y) * 4 + x) * 4] != 0 ? '#' : '.';
                }
                rows[y][4] = '\0';
            }
            fprintf(stderr, "p4gl: edge rule probe, image rows top to bottom: %s %s %s %s\n", rows[0], rows[1], rows[2], rows[3]);
        }
    }
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    p4gl_glBindTexture(GL_TEXTURE_2D, 0);
    p4gl_glViewport(vp[0], vp[1], vp[2], vp[3]);
    fprintf(stderr, "p4gl: a pixel centre on a quad's edge %s\n", sTieBias ? "falls on the other side than the GS's (vertices move 1/128 pixel up and left)" : "falls as the GS's top-left rule has it");
}

int p4gl_gl_load(void)
{
    int missing = 0;

    missing |= p4gl_resolve((void **)&p4gl_glGetString, "glGetString");
    missing |= p4gl_resolve((void **)&p4gl_glGetStringi, "glGetStringi");
    missing |= p4gl_resolve((void **)&p4gl_glGetIntegerv, "glGetIntegerv");
    missing |= p4gl_resolve((void **)&p4gl_glGenTextures, "glGenTextures");
    missing |= p4gl_resolve((void **)&p4gl_glDeleteTextures, "glDeleteTextures");
    missing |= p4gl_resolve((void **)&p4gl_glBindTexture, "glBindTexture");
    missing |= p4gl_resolve((void **)&p4gl_glTexImage2D, "glTexImage2D");
    missing |= p4gl_resolve((void **)&p4gl_glTexSubImage2D, "glTexSubImage2D");
    missing |= p4gl_resolve((void **)&p4gl_glCopyTexSubImage2D, "glCopyTexSubImage2D");
    missing |= p4gl_resolve((void **)&p4gl_glActiveTexture, "glActiveTexture");
    missing |= p4gl_resolve((void **)&p4gl_glTexParameteri, "glTexParameteri");
    missing |= p4gl_resolve((void **)&p4gl_glPixelStorei, "glPixelStorei");
    missing |= p4gl_resolve((void **)&p4gl_glClearColor, "glClearColor");
    missing |= p4gl_resolve((void **)&p4gl_glClear, "glClear");
    missing |= p4gl_resolve((void **)&p4gl_glFlush, "glFlush");
    missing |= p4gl_resolve((void **)&p4gl_glViewport, "glViewport");
    missing |= p4gl_resolve((void **)&p4gl_glGetError, "glGetError");
    missing |= p4gl_resolve((void **)&p4gl_glEnable, "glEnable");
    missing |= p4gl_resolve((void **)&p4gl_glDisable, "glDisable");
    missing |= p4gl_resolve((void **)&p4gl_glBlendFunc, "glBlendFunc");
    missing |= p4gl_resolve((void **)&p4gl_glGenVertexArrays, "glGenVertexArrays");
    missing |= p4gl_resolve((void **)&p4gl_glBindVertexArray, "glBindVertexArray");
    missing |= p4gl_resolve((void **)&p4gl_glGenBuffers, "glGenBuffers");
    missing |= p4gl_resolve((void **)&p4gl_glBindBuffer, "glBindBuffer");
    missing |= p4gl_resolve((void **)&p4gl_glBufferData, "glBufferData");
    missing |= p4gl_resolve((void **)&p4gl_glEnableVertexAttribArray, "glEnableVertexAttribArray");
    missing |= p4gl_resolve((void **)&p4gl_glVertexAttribPointer, "glVertexAttribPointer");
    missing |= p4gl_resolve((void **)&p4gl_glVertexAttribDivisor, "glVertexAttribDivisor");
    missing |= p4gl_resolve((void **)&p4gl_glCreateShader, "glCreateShader");
    missing |= p4gl_resolve((void **)&p4gl_glDeleteShader, "glDeleteShader");
    missing |= p4gl_resolve((void **)&p4gl_glShaderSource, "glShaderSource");
    missing |= p4gl_resolve((void **)&p4gl_glCompileShader, "glCompileShader");
    missing |= p4gl_resolve((void **)&p4gl_glGetShaderiv, "glGetShaderiv");
    missing |= p4gl_resolve((void **)&p4gl_glGetShaderInfoLog, "glGetShaderInfoLog");
    missing |= p4gl_resolve((void **)&p4gl_glCreateProgram, "glCreateProgram");
    missing |= p4gl_resolve((void **)&p4gl_glDeleteProgram, "glDeleteProgram");
    missing |= p4gl_resolve((void **)&p4gl_glAttachShader, "glAttachShader");
    missing |= p4gl_resolve((void **)&p4gl_glLinkProgram, "glLinkProgram");
    missing |= p4gl_resolve((void **)&p4gl_glGetProgramiv, "glGetProgramiv");
    missing |= p4gl_resolve((void **)&p4gl_glUseProgram, "glUseProgram");
    missing |= p4gl_resolve((void **)&p4gl_glGetUniformLocation, "glGetUniformLocation");
    missing |= p4gl_resolve((void **)&p4gl_glUniform2f, "glUniform2f");
    missing |= p4gl_resolve((void **)&p4gl_glUniform1i, "glUniform1i");
    missing |= p4gl_resolve((void **)&p4gl_glUniform1f, "glUniform1f");
    missing |= p4gl_resolve((void **)&p4gl_glDrawArrays, "glDrawArrays");
    missing |= p4gl_resolve((void **)&p4gl_glDrawArraysInstanced, "glDrawArraysInstanced");
    missing |= p4gl_resolve((void **)&p4gl_glDrawElements, "glDrawElements");
    missing |= p4gl_resolve((void **)&p4gl_glGenFramebuffers, "glGenFramebuffers");
    missing |= p4gl_resolve((void **)&p4gl_glBindFramebuffer, "glBindFramebuffer");
    missing |= p4gl_resolve((void **)&p4gl_glFramebufferTexture2D, "glFramebufferTexture2D");
    missing |= p4gl_resolve((void **)&p4gl_glCheckFramebufferStatus, "glCheckFramebufferStatus");
    missing |= p4gl_resolve((void **)&p4gl_glReadPixels, "glReadPixels");
    missing |= p4gl_resolve((void **)&p4gl_glGenRenderbuffers, "glGenRenderbuffers");
    missing |= p4gl_resolve((void **)&p4gl_glDeleteFramebuffers, "glDeleteFramebuffers");
    missing |= p4gl_resolve((void **)&p4gl_glDeleteRenderbuffers, "glDeleteRenderbuffers");
    missing |= p4gl_resolve((void **)&p4gl_glBindRenderbuffer, "glBindRenderbuffer");
    missing |= p4gl_resolve((void **)&p4gl_glRenderbufferStorage, "glRenderbufferStorage");
    missing |= p4gl_resolve((void **)&p4gl_glFramebufferRenderbuffer, "glFramebufferRenderbuffer");
    missing |= p4gl_resolve((void **)&p4gl_glDeleteBuffers, "glDeleteBuffers");
    missing |= p4gl_resolve((void **)&p4gl_glDepthFunc, "glDepthFunc");
    missing |= p4gl_resolve((void **)&p4gl_glDepthMask, "glDepthMask");
    missing |= p4gl_resolve((void **)&p4gl_glCullFace, "glCullFace");
    missing |= p4gl_resolve((void **)&p4gl_glBlitFramebuffer, "glBlitFramebuffer");
    missing |= p4gl_resolve((void **)&p4gl_glFrontFace, "glFrontFace");
    missing |= p4gl_resolve((void **)&p4gl_glColorMask, "glColorMask");
    missing |= p4gl_resolve((void **)&p4gl_glBlendEquation, "glBlendEquation");
    missing |= p4gl_resolve((void **)&p4gl_glBlendFuncSeparate, "glBlendFuncSeparate");
    missing |= p4gl_resolve((void **)&p4gl_glBlendColor, "glBlendColor");
    missing |= p4gl_resolve((void **)&p4gl_glUniform3f, "glUniform3f");
    missing |= p4gl_resolve((void **)&p4gl_glUniform3fv, "glUniform3fv");
    missing |= p4gl_resolve((void **)&p4gl_glUniform4f, "glUniform4f");
    missing |= p4gl_resolve((void **)&p4gl_glUniformMatrix3fv, "glUniformMatrix3fv");
    missing |= p4gl_resolve((void **)&p4gl_glUniformMatrix4fv, "glUniformMatrix4fv");
    if (!missing) {
        context_probe();
        /* CI must never turn an unavailable GPU into a passing software run.
         * A host may additionally require its reviewed renderer by substring. */
        const char *hardware = getenv("P4PC_CI_HARDWARE_GL");
        const char *ciRenderer = getenv("P4PC_CI_GL_RENDERER");
        if ((hardware != NULL && strcmp(hardware, "1") == 0 &&
             (sRenderer[0] == '\0' || contains_nocase(sRenderer, "llvmpipe") ||
              contains_nocase(sRenderer, "softpipe") || contains_nocase(sRenderer, "swiftshader") ||
              contains_nocase(sRenderer, "warp") || contains_nocase(sRenderer, "software") ||
              contains_nocase(sRenderer, "basic render") || contains_nocase(sRenderer, "gdi generic"))) ||
            (ciRenderer != NULL && ciRenderer[0] != '\0' && !contains_nocase(sRenderer, ciRenderer))) {
            fprintf(stderr, "p4gl: CI requires hardware renderer%s%s; got '%s'\n",
                    ciRenderer != NULL && ciRenderer[0] != '\0' ? " containing " : "",
                    ciRenderer != NULL ? ciRenderer : "", sRenderer);
            exit(EXIT_FAILURE);
        }
        /* Port-frame goldens are scoped to the renderer which produced them.
         * Do not judge another GPU's colour quantization against that image.
         * Ordinary player runs and unscoped retail gates are unchanged. */
        const char *required = getenv("P4PC_GATE_RENDERER");
        if (required != NULL && required[0] != '\0' && sRenderer[0] == '\0') {
            fprintf(stderr, "p4gl: reference scope requires a valid loaded GL renderer\n");
            return 1;
        }
        if (required != NULL && required[0] != '\0' && SDL_strcasecmp(sRenderer, required) != 0) {
            fprintf(stderr, "p4gl: reference renderer mismatch: requires '%s', got '%s'; skipped\n",
                    required, sRenderer);
            exit(77);
        }
    }
    if (!missing) p4gl_profile_init(sIsEs);
    if (!missing && (getenv("P4PC_GLSTATS") != NULL || getenv("P4PC_GL_HITCH_TRACE") != NULL || getenv("P4PC_GPU_TRACE") != NULL)) {
        stats_install();
    }
    if (!missing && (getenv("P4PC_GLCACHE") == NULL || getenv("P4PC_GLCACHE")[0] != '0')) {
        cache_install();
    }
    if (!missing) {
        probe_output_rounding();
        probe_edge_rule();
    }
    if (!missing && getenv("P4PC_GLERR") != NULL) {
        errcheck_install();
    }
    return missing ? 1 : 0;
}
