/* The window system's GL calls, as p4gl_context.c and p4gl_gl.c use them: SDL's (SDL_GL_*: WGL, GLX, EGL on Android and Linux, Cocoa) or, with
 * P4PC_GL_VIA_EGL (iOS), the port's own EGL on a CAMetalLayer (p4gl_egl.c). */
#ifndef P4PC_RENDER_P4GL_WIN_H
#define P4PC_RENDER_P4GL_WIN_H

#include <SDL3/SDL.h>

#ifdef P4PC_GL_VIA_EGL
#include "p4gl_egl.h"

typedef void *P4GlContext;
#define P4GL_WIN_FLAG SDL_WINDOW_METAL
#define p4gl_win_create_context(w) p4gl_egl_create_context(w)
#define p4gl_win_make_current(w, c) p4gl_egl_make_current(w, c)
#define p4gl_win_destroy_context(c) p4gl_egl_destroy_context(c)
#define p4gl_win_current_context() p4gl_egl_current_context()
#define p4gl_win_swap(w) p4gl_egl_swap(w)
#define p4gl_win_set_swap_interval(n) p4gl_egl_set_swap_interval(n)
#define p4gl_win_proc(name) p4gl_egl_proc(name)
#define p4gl_win_error() p4gl_egl_error()
#else
typedef SDL_GLContext P4GlContext;
#define P4GL_WIN_FLAG SDL_WINDOW_OPENGL
#define p4gl_win_create_context(w) SDL_GL_CreateContext(w)
#define p4gl_win_make_current(w, c) SDL_GL_MakeCurrent(w, c)
#define p4gl_win_destroy_context(c) SDL_GL_DestroyContext(c)
#define p4gl_win_current_context() SDL_GL_GetCurrentContext()
#define p4gl_win_swap(w) SDL_GL_SwapWindow(w)
#define p4gl_win_set_swap_interval(n) SDL_GL_SetSwapInterval(n)
#define p4gl_win_proc(name) ((void (*)(void))SDL_GL_GetProcAddress(name))
#define p4gl_win_error() SDL_GetError()
#endif

#endif
