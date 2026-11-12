/* The renderer's GL context on this machine, with no game data: the port's own API choice and attributes (src/render/p4gl_api.h: OpenGL ES 3.0
 * by default; on macOS through ANGLE from the app bundle or beside the executable, Metal unless ANGLE_DEFAULT_PLATFORM says otherwise), a
 * hidden window, the context through the renderer's window-system calls (p4gl_win.h), the renderer's loader (p4gl_gl_load) and shader header
 * (p4gl_gl_compile_shader), then a clear and one triangle drawn into an RGBA8 framebuffer object, read back and checked at two pixels.
 * Prints the context's GL strings and writes the 64x64 picture to argv[1] as a PNG. A default ES request that fails falls back to desktop
 * OpenGL 3.3 core like p4gl_context_open does.
 *
 * Exit 77 (ctest skip) when the machine gives no context. P4PC_REQUIRE_GLES=1 turns that and a desktop fallback into a failure (a CI host
 * that must have the ES path); P4PC_REQUIRE_GL_RENDERER=<text> fails when GL_RENDERER does not contain <text> (for example ANGLE's "Metal"). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "p4gl_api.h"
#include "p4gl_gl.h"
#include "p4gl_win.h"
#include "p4png.h"

#ifndef GL_SHADING_LANGUAGE_VERSION
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#endif

enum { kSize = 64 };

static int sRequire;

static int unavailable(const char *what)
{
    printf("gles_context_smoke: %s: %s\n", what, SDL_GetError());
    return sRequire ? 1 : 77;
}

static const char kVertex[] =
    "layout(location = 0) in vec2 aPos;\n"
    "layout(location = 1) in vec3 aColor;\n"
    "out vec3 vColor;\n"
    "void main() { vColor = aColor; gl_Position = vec4(aPos, 0.0, 1.0); }\n";
static const char kFragment[] =
    "in vec3 vColor;\n"
    "out vec4 fragColor;\n"
    "void main() { fragColor = vec4(vColor, 1.0); }\n";

static int near(const unsigned char *px, int r, int g, int b)
{
    return abs(px[0] - r) <= 2 && abs(px[1] - g) <= 2 && abs(px[2] - b) <= 2 && px[3] == 255;
}

int main(int argc, char **argv)
{
    const char *require = getenv("P4PC_REQUIRE_GLES");
    const char *wantRenderer = getenv("P4PC_REQUIRE_GL_RENDERER");
    SDL_Window *window;
    P4GlContext ctx;
    int fallback = 0, bad = 0;
    GLuint tex = 0, fbo = 0, vao = 0, vbo = 0, vs, fs, program;
    GLint linked = GL_FALSE;
    static unsigned char pixels[kSize * kSize * 4], flipped[kSize * kSize * 4];
    static const GLfloat triangle[] = {
        -1.0f, -1.0f, 1.0f, 0.5f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.5f, 0.0f,
        -1.0f, 1.0f, 1.0f, 0.5f, 0.0f,
    };

    sRequire = require != NULL && require[0] == '1';
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return unavailable("SDL_Init failed");
    }
    printf("gles_context_smoke: SDL video driver %s\n", SDL_GetCurrentVideoDriver());
    p4gl_api_set_attributes();
    window = p4gl_api_create_window("gles_context_smoke", kSize, kSize, P4GL_WIN_FLAG | SDL_WINDOW_HIDDEN);
    if (window == NULL) {
        return unavailable("no window");
    }
    p4gl_api_set_attributes();      /* again, as p4gl_context_open does: the window may be the desktop fallback's (p4gl_api_create_window) */
    ctx = p4gl_win_create_context(window);
    if (ctx == NULL && p4gl_api_wants_es() && !p4gl_api_explicit()) {
        printf("gles_context_smoke: OpenGL ES 3.0 context failed (%s); trying desktop OpenGL 3.3 core like the game\n", p4gl_win_error());
        SDL_GL_ResetAttributes();
        p4gl_api_set_attributes_desktop();
        ctx = p4gl_win_create_context(window);
        fallback = 1;
    }
    if (ctx == NULL || !p4gl_win_make_current(window, ctx)) {
        printf("gles_context_smoke: no GL context: %s\n", p4gl_win_error());
        return sRequire ? 1 : 77;
    }
    if (p4gl_gl_load() != 0) {
        printf("gles_context_smoke: GL entry points missing\n");
        return sRequire ? 1 : 77;
    }
    printf("gles_context_smoke: GL_VENDOR   %s\n", (const char *)p4gl_glGetString(GL_VENDOR));
    printf("gles_context_smoke: GL_RENDERER %s\n", (const char *)p4gl_glGetString(GL_RENDERER));
    printf("gles_context_smoke: GL_VERSION  %s\n", (const char *)p4gl_glGetString(GL_VERSION));
    printf("gles_context_smoke: GLSL       %s\n", (const char *)p4gl_glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("gles_context_smoke: %s context%s\n", p4gl_gl_is_es() ? "OpenGL ES" : "desktop OpenGL", fallback ? " (fallback)" : "");
    if (sRequire && (fallback || !p4gl_gl_is_es())) {
        printf("gles_context_smoke: FAIL: P4PC_REQUIRE_GLES=1 and the context is not OpenGL ES\n");
        bad = 1;
    }
    if (wantRenderer != NULL && wantRenderer[0] != '\0' &&
        strstr((const char *)p4gl_glGetString(GL_RENDERER), wantRenderer) == NULL) {
        printf("gles_context_smoke: FAIL: GL_RENDERER does not contain \"%s\"\n", wantRenderer);
        bad = 1;
    }

    p4gl_glGenTextures(1, &tex);
    p4gl_glBindTexture(GL_TEXTURE_2D, tex);
    p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    p4gl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    p4gl_glGenFramebuffers(1, &fbo);
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    p4gl_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    if (p4gl_glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("gles_context_smoke: FAIL: the RGBA8 framebuffer is incomplete\n");
        return 1;
    }
    vs = p4gl_gl_compile_shader(GL_VERTEX_SHADER, kVertex, "smoke vertex");
    fs = p4gl_gl_compile_shader(GL_FRAGMENT_SHADER, kFragment, "smoke fragment");
    program = p4gl_glCreateProgram();
    if (vs == 0 || fs == 0) {
        printf("gles_context_smoke: FAIL: shader compile\n");
        return 1;
    }
    p4gl_glAttachShader(program, vs);
    p4gl_glAttachShader(program, fs);
    p4gl_glLinkProgram(program);
    p4gl_glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        printf("gles_context_smoke: FAIL: program link\n");
        return 1;
    }
    p4gl_glGenVertexArrays(1, &vao);
    p4gl_glBindVertexArray(vao);
    p4gl_glGenBuffers(1, &vbo);
    p4gl_glBindBuffer(GL_ARRAY_BUFFER, vbo);
    p4gl_glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof triangle, triangle, GL_STATIC_DRAW);
    p4gl_glEnableVertexAttribArray(0);
    p4gl_glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const void *)0);
    p4gl_glEnableVertexAttribArray(1);
    p4gl_glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const void *)(2 * sizeof(GLfloat)));

    p4gl_glViewport(0, 0, kSize, kSize);
    p4gl_glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    p4gl_glClear(GL_COLOR_BUFFER_BIT);
    p4gl_glUseProgram(program);
    p4gl_glDrawArrays(GL_TRIANGLES, 0, 3);
    p4gl_glReadPixels(0, 0, kSize, kSize, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    if (p4gl_glGetError() != GL_NO_ERROR) {
        printf("gles_context_smoke: FAIL: a GL error after the draw\n");
        bad = 1;
    }
    /* (8, 8) is inside the lower-left triangle, (56, 56) outside it: the triangle's colour and the clear colour */
    {
        const unsigned char *in = pixels + (8 * kSize + 8) * 4, *out = pixels + (56 * kSize + 56) * 4;

        printf("gles_context_smoke: pixel (8,8) = %u %u %u %u, pixel (56,56) = %u %u %u %u\n", in[0], in[1], in[2], in[3], out[0], out[1],
               out[2], out[3]);
        if (!near(in, 255, 128, 0) || !near(out, 26, 51, 77)) {
            printf("gles_context_smoke: FAIL: expected 255 128 0 255 inside the triangle and 26 51 77 255 outside\n");
            bad = 1;
        }
    }
    if (argc > 1) {
        for (int y = 0; y < kSize; y++) {
            memcpy(flipped + (size_t)y * kSize * 4, pixels + (size_t)(kSize - 1 - y) * kSize * 4, kSize * 4);
        }
        if (p4png_write_rgba(argv[1], kSize, kSize, flipped) != 0) {
            printf("gles_context_smoke: could not write %s\n", argv[1]);
            bad = 1;
        } else {
            printf("gles_context_smoke: wrote %s\n", argv[1]);
        }
    }
    p4gl_win_destroy_context(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    if (!bad) {
        printf("gles_context_smoke: OK\n");
    }
    return bad;
}
