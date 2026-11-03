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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#if defined(__APPLE__) && !defined(P4PC_GL_VIA_EGL)
#include "p4pc/os.h"
#endif

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

/* The SDL attributes of the desktop OpenGL 3.3 core context. On macOS the library hint the ES path set (ANGLE's libGLESv2, below) is dropped,
 * so SDL loads Apple's OpenGL for it, also when it is the fallback after ES failed (SDL_OPENGL_LIBRARY in the environment still wins). */
static inline void p4gl_api_set_attributes_desktop(void)
{
#if defined(__APPLE__) && !defined(P4PC_GL_VIA_EGL)
    SDL_ResetHint(SDL_HINT_OPENGL_LIBRARY);
#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
}

#if defined(__APPLE__) && !defined(P4PC_GL_VIA_EGL)
/* macOS: SDL's Cocoa driver makes an ES context through EGL (Cocoa_GLES_*), and the EGL is ANGLE's (docs/platform/macos.md), which the app
 * bundle carries in Contents/Frameworks (tools/package_macos.sh) and a build tree beside its executables (cmake/sources/060-platform-libraries.cmake).
 * SDL itself would dlopen("libEGL.dylib"), which searches the working directory and the fallback paths, not the bundle, so the two libraries
 * are named by path here (p4_os_runtime_library); SDL_EGL_LIBRARY / SDL_OPENGL_LIBRARY in the environment still win over these hints.
 * ANGLE's back end is Metal unless ANGLE_DEFAULT_PLATFORM or P4PC_ANGLE picks another. Without ANGLE, or without a display for it (no Metal
 * device: a virtual machine), the window (p4gl_api_create_window) and the context (p4gl_context_open) fall back to Apple's desktop OpenGL. */
static inline void p4gl_api_apple_angle(void)
{
    char egl[1024], gles[1024];

    SDL_setenv_unsafe("ANGLE_DEFAULT_PLATFORM", "metal", 0);
    if (p4_os_runtime_library("libEGL.dylib", egl, sizeof egl) == 0 && p4_os_runtime_library("libGLESv2.dylib", gles, sizeof gles) == 0) {
        SDL_SetHint(SDL_HINT_EGL_LIBRARY, egl);
        SDL_SetHint(SDL_HINT_OPENGL_LIBRARY, gles);
    }
}
#endif

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
#if defined(__APPLE__) && !defined(P4PC_GL_VIA_EGL)
    p4gl_api_apple_angle();
#endif
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

/* SDL_CreateWindow for the GL attributes set (p4gl_api_set_attributes). macOS: SDL's Cocoa driver sets up the ES path's EGL display when it
 * creates the window, so a Mac where ANGLE has no display fails here, before p4gl_context_open could fall back to desktop OpenGL; with the
 * build's default ES the window is made again for desktop OpenGL then, and the rest of the run takes desktop OpenGL as P4PC_GL_API=gl would
 * (an ES context attempt on that window leaves SDL without Apple's OpenGL entry points). A run that asked for ES (P4PC_GL_API, P4PC_ANGLE)
 * fails instead. */
static inline SDL_Window *p4gl_api_create_window(const char *title, int width, int height, SDL_WindowFlags flags)
{
    SDL_Window *window = SDL_CreateWindow(title, width, height, flags);

#if defined(__APPLE__) && !defined(P4PC_GL_VIA_EGL)
    if (window == NULL && p4gl_api_wants_es() && !p4gl_api_explicit()) {
        fprintf(stderr, "p4gl: SDL_CreateWindow (OpenGL ES 3.0) failed: %s; trying desktop OpenGL 3.3 core\n", SDL_GetError());
        SDL_setenv_unsafe("P4PC_GL_API", "gl", 1);
        SDL_GL_ResetAttributes();
        p4gl_api_set_attributes_desktop();
        window = SDL_CreateWindow(title, width, height, flags);
    }
#endif
    return window;
}

#endif
