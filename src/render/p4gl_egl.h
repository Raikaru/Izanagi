/* OpenGL ES through EGL on a CAMetalLayer: the context layer for iOS (P4PC_GL_VIA_EGL), where SDL3 has no EGL path of its own (its UIKit
 * video driver offers only the deprecated EAGL, which has no ES 3.0 on a Metal-only future and no ANGLE). ANGLE's Metal back end is the ES 3.0
 * implementation: libEGL and libGLESv2 of the ANGLE build are shipped in the app bundle as frameworks, loaded here with dlopen, so the port links
 * neither and a build without them still starts (and says why it cannot draw). The window is an SDL window made with SDL_WINDOW_METAL; its
 * CAMetalLayer (SDL_Metal_CreateView / SDL_Metal_GetLayer) becomes the EGL window surface. docs/renderer_port.md section 10.
 *
 * The functions mirror the SDL_GL_* calls src/render/p4gl_context.c uses (p4gl_win.h maps one set or the other), one window per process. */
#ifndef P4PC_RENDER_P4GL_EGL_H
#define P4PC_RENDER_P4GL_EGL_H

#include <stdbool.h>

#include <SDL3/SDL.h>

/* Loads ANGLE, opens its Metal display, makes a window surface of the window's CAMetalLayer and an ES 3.0 context on it. NULL on failure (see
 * p4gl_egl_error). The first call does the setup; later calls (context loss) only make a new context on the same surface. */
void *p4gl_egl_create_context(SDL_Window *window);
/* Makes the context current on the window's surface; NULL releases the thread's context. */
bool p4gl_egl_make_current(SDL_Window *window, void *context);
void p4gl_egl_destroy_context(void *context);
void *p4gl_egl_current_context(void);
/* eglSwapBuffers. False on failure, with p4gl_egl_error naming the EGL error (EGL_CONTEXT_LOST is how p4gl_context.c knows to recover). */
bool p4gl_egl_swap(SDL_Window *window);
bool p4gl_egl_set_swap_interval(int interval);
/* An OpenGL ES entry point of the current context (eglGetProcAddress). */
void (*p4gl_egl_proc(const char *name))(void);
/* The last failure, as text. */
const char *p4gl_egl_error(void);

#endif
