/* Which OpenGL API the renderer runs on: desktop OpenGL 3.3 core, or the OpenGL ES 3.0 subset that
 * Android, iOS and macOS (through ANGLE) offer. One source serves both: the shader headers
 * (p4gl_gl_compile_shader) and the few feature differences (p4gl_gl_border_native) follow the
 * context that was actually created.
 *
 * The choice is made before the window exists: SDL picks the pixel format or EGL config of a window
 * from the GL attributes set at that time (src/platform/pad/pad_sdl.c calls p4gl_api_set_attributes
 * before SDL_CreateWindow, p4gl_context_open calls it again). The default is the CMake option
 * P4PC_GLES (it defines P4PC_GL_API_ES_DEFAULT); P4PC_GL_API=gl|gles overrides it per run. */
#ifndef P4PC_RENDER_P4GL_API_H
#define P4PC_RENDER_P4GL_API_H

#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#ifndef P4PC_GL_API_ES_DEFAULT
#define P4PC_GL_API_ES_DEFAULT 0
#endif

/* P4PC_ANGLE=1|<backend>: run the ES path through ANGLE (libEGL and libGLESv2 next to the exe, or where the loader finds them) instead
 * of the platform's own ES driver. SDL then uses EGL (SDL_HINT_OPENGL_ES_DRIVER), which on Windows would otherwise be WGL's ES profile
 * (the NVIDIA driver's own ES). <backend> picks ANGLE's renderer through ANGLE_DEFAULT_PLATFORM: d3d11, d3d9, gl, vulkan, metal,
 * swiftshader or null (its default is d3d11 on Windows, metal on macOS, vulkan or gl on Linux). Setting it implies the ES API. */
static inline const char *p4gl_api_angle(void)
{
    const char *a = getenv("P4PC_ANGLE");

    return a != NULL && a[0] != '\0' && strcmp(a, "0") != 0 ? a : NULL;
}

/* 1 when the renderer should create an OpenGL ES 3.0 context, 0 for desktop OpenGL 3.3 core. */
static inline int p4gl_api_wants_es(void)
{
    const char *api = getenv("P4PC_GL_API");

    if (api != NULL && api[0] != '\0') {
        return strcmp(api, "gles") == 0 || strcmp(api, "es") == 0;
    }
    return P4PC_GL_API_ES_DEFAULT || p4gl_api_angle() != NULL;
}

/* 1 when the API was asked for by the run (P4PC_GL_API or P4PC_ANGLE) rather than taken from the build's default. A default that cannot be
 * had (a driver with no ES profile, no ES library) falls back to desktop OpenGL; an explicit request fails instead (p4gl_context_open). */
static inline int p4gl_api_explicit(void)
{
    const char *api = getenv("P4PC_GL_API");

    return (api != NULL && api[0] != '\0') || p4gl_api_angle() != NULL;
}

/* The SDL attributes of the desktop OpenGL 3.3 core context. */
static inline void p4gl_api_set_attributes_desktop(void)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
}

/* The SDL GL attributes of the chosen API. The ES path asks for an 8-bit-per-channel window with alpha: the GS blend state lives in the
 * frame buffer's alpha channel (docs/renderer_port.md), and glCopyTexSubImage2D / glBlitFramebuffer between the window and the RGBA8
 * textures need matching formats in ES. */
static inline void p4gl_api_set_attributes(void)
{
    const char *angle = p4gl_api_angle();

    if (!p4gl_api_wants_es()) {
        p4gl_api_set_attributes_desktop();
        return;
    }
    if (angle != NULL) {
        SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
        if (strcmp(angle, "1") != 0) {
            SDL_setenv_unsafe("ANGLE_DEFAULT_PLATFORM", angle, 1);
        }
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
}

#endif
