/* The GL call filter (src/render/p4gl_gl.c: redundant-state cache, lazy fixed-function state) against the driver's real state. A cache that believes an
 * object is bound after the driver unbound it skips the call that would bind it: glDeleteBuffers on the bound buffer, then a new buffer that gets the same
 * name and a bind of it, was skipped, the vertex data went to buffer 0 and a later glDrawElements read from an invalid client pointer (an access violation
 * inside nvoglv32.dll when a Marukyu floor change recycled the atomic cache's buffers). Each case asks the driver (glGetIntegerv, resolved directly) what is bound.
 * Offscreen window, ES 3.0 then desktop 3.3 core like the game. Exits 77 when no context can be made. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "p4gl_gl.h"
#include "p4gl_win.h"

#ifndef GL_ARRAY_BUFFER_BINDING
#define GL_ARRAY_BUFFER_BINDING 0x8894
#endif

static void (P4GL_APIENTRY *getIntegerv)(GLenum, GLint *);

static GLint query(GLenum what)
{
    GLint v = -1;

    getIntegerv(what, &v);
    return v;
}

static int bad;

static void expect(const char *what, GLint got, GLint want)
{
    if (got != want) {
        printf("gl_cache_test: %s: the driver has %d, expected %d\n", what, (int)got, (int)want);
        bad = 1;
    }
}

int main(void)
{
    SDL_Window *window;
    SDL_GLContext ctx;
    GLuint a = 0, b = 0, c = 0;

    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, getenv("SDL_VIDEODRIVER") != NULL ? getenv("SDL_VIDEODRIVER") : "offscreen");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("gl_cache_test: SDL_Init failed: %s\n", SDL_GetError());
        return 77;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    window = SDL_CreateWindow("gl_cache_test", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    ctx = window != NULL ? SDL_GL_CreateContext(window) : NULL;
    if (ctx == NULL) {
        SDL_GL_ResetAttributes();
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        if (window != NULL) {
            SDL_DestroyWindow(window);
        }
        window = SDL_CreateWindow("gl_cache_test", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        ctx = window != NULL ? SDL_GL_CreateContext(window) : NULL;
    }
    if (ctx == NULL || !SDL_GL_MakeCurrent(window, ctx)) {
        printf("gl_cache_test: no GL context: %s\n", SDL_GetError());
        return 77;
    }
    getIntegerv = (void (P4GL_APIENTRY *)(GLenum, GLint *))SDL_GL_GetProcAddress("glGetIntegerv");
    if (p4gl_gl_load() != 0 || getIntegerv == NULL) {
        printf("gl_cache_test: GL entry points missing\n");
        return 77;
    }

    /* 1: a bound array buffer is deleted; the next buffer reuses the name (or not) and its bind must reach the driver */
    p4gl_glGenBuffers(1, &a);
    p4gl_glBindBuffer(GL_ARRAY_BUFFER, a);
    expect("array buffer after the first bind", query(GL_ARRAY_BUFFER_BINDING), (GLint)a);
    p4gl_glDeleteBuffers(1, &a);
    expect("array buffer after deleting the bound one", query(GL_ARRAY_BUFFER_BINDING), 0);
    p4gl_glGenBuffers(1, &b);
    p4gl_glBindBuffer(GL_ARRAY_BUFFER, b);
    expect("array buffer after binding the next buffer", query(GL_ARRAY_BUFFER_BINDING), (GLint)b);
    if (b != a) {
        printf("gl_cache_test: note: the driver did not reuse the buffer name (%u then %u), case 1 only checked the unbind\n", (unsigned)a, (unsigned)b);
    }

    /* 2: binding 0 and rebinding, and a bind to the buffer that is already bound, behave */
    p4gl_glBindBuffer(GL_ARRAY_BUFFER, 0);
    expect("array buffer unbound", query(GL_ARRAY_BUFFER_BINDING), 0);
    p4gl_glBindBuffer(GL_ARRAY_BUFFER, b);
    p4gl_glBindBuffer(GL_ARRAY_BUFFER, b);
    expect("array buffer bound twice", query(GL_ARRAY_BUFFER_BINDING), (GLint)b);

    /* 3: deleting a buffer that is not bound leaves the binding alone */
    p4gl_glGenBuffers(1, &c);
    p4gl_glDeleteBuffers(1, &c);
    expect("array buffer after deleting another buffer", query(GL_ARRAY_BUFFER_BINDING), (GLint)b);

    /* 4: two buffers deleted in one call, one of them bound */
    p4gl_glGenBuffers(1, &a);
    p4gl_glGenBuffers(1, &c);
    p4gl_glBindBuffer(GL_ARRAY_BUFFER, a);
    {
        const GLuint both[2] = {c, a};

        p4gl_glDeleteBuffers(2, both);
    }
    expect("array buffer after deleting two, one bound", query(GL_ARRAY_BUFFER_BINDING), 0);
    p4gl_glGenBuffers(1, &a);
    p4gl_glBindBuffer(GL_ARRAY_BUFFER, a);
    expect("array buffer after the rebind", query(GL_ARRAY_BUFFER_BINDING), (GLint)a);

    if (!bad) {
        printf("gl_cache_test: the filter tracks the driver's bindings through deletes\n");
    }
    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return bad;
}
