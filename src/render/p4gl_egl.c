/* EGL on a CAMetalLayer through ANGLE's Metal back end (p4gl_egl.h). Not compiled into the Android, Linux or Windows builds. */
#ifdef P4PC_GL_VIA_EGL
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <SDL3/SDL_egl.h>
#include <SDL3/SDL_metal.h>

#include "p4gl_egl.h"

/* ANGLE's EGL extension tokens (EGL_ANGLE_platform_angle, ..._metal), which the Khronos headers do not carry. */
#define P4_EGL_PLATFORM_ANGLE_ANGLE 0x3202
#define P4_EGL_PLATFORM_ANGLE_TYPE_ANGLE 0x3203
#define P4_EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE 0x3489

typedef EGLDisplay (*GetPlatformDisplayFn)(EGLenum platform, void *native, const EGLint *attribs);

static struct {
    void *lib;
    void (*(*GetProcAddress)(const char *))(void);
    EGLBoolean (*Initialize)(EGLDisplay, EGLint *, EGLint *);
    EGLBoolean (*ChooseConfig)(EGLDisplay, const EGLint *, EGLConfig *, EGLint, EGLint *);
    EGLContext (*CreateContext)(EGLDisplay, EGLConfig, EGLContext, const EGLint *);
    EGLSurface (*CreateWindowSurface)(EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint *);
    EGLBoolean (*MakeCurrent)(EGLDisplay, EGLSurface, EGLSurface, EGLContext);
    EGLBoolean (*SwapBuffers)(EGLDisplay, EGLSurface);
    EGLBoolean (*SwapInterval)(EGLDisplay, EGLint);
    EGLBoolean (*DestroyContext)(EGLDisplay, EGLContext);
    EGLContext (*GetCurrentContext)(void);
    EGLint (*GetError)(void);
    GetPlatformDisplayFn GetPlatformDisplayEXT;
} sEgl;

static EGLDisplay sDisplay = EGL_NO_DISPLAY;
static EGLConfig sConfig;
static EGLSurface sSurface = EGL_NO_SURFACE;
static SDL_MetalView sView;
static char sError[160];

static void fail(const char *what)
{
    const EGLint code = sEgl.GetError != NULL ? sEgl.GetError() : 0;

    snprintf(sError, sizeof sError, "%s (EGL error 0x%x%s)", what, (unsigned)code, code == EGL_CONTEXT_LOST ? " EGL_CONTEXT_LOST" : "");
}

const char *p4gl_egl_error(void)
{
    return sError;
}

#define LOAD(field, name) \
    do { \
        *(void **)&sEgl.field = dlsym(sEgl.lib, "egl" name); \
        if (sEgl.field == NULL) { \
            snprintf(sError, sizeof sError, "ANGLE's libEGL has no egl%s", name); \
            return false; \
        } \
    } while (0)

static bool load_library(void)
{
    /* the frameworks of the app bundle (Frameworks/libEGL.framework), then a plain dylib (a simulator or macOS build) */
    static const char *const kNames[] = {"@rpath/libEGL.framework/libEGL", "libEGL.framework/libEGL", "@rpath/libEGL.dylib", "libEGL.dylib"};

    for (size_t i = 0; i < sizeof kNames / sizeof kNames[0] && sEgl.lib == NULL; i++) {
        sEgl.lib = dlopen(kNames[i], RTLD_NOW | RTLD_LOCAL);
    }
    if (sEgl.lib == NULL) {
        snprintf(sError, sizeof sError, "ANGLE's libEGL is not in the app bundle (%s)", dlerror());
        return false;
    }
    LOAD(GetProcAddress, "GetProcAddress");
    LOAD(Initialize, "Initialize");
    LOAD(ChooseConfig, "ChooseConfig");
    LOAD(CreateContext, "CreateContext");
    LOAD(CreateWindowSurface, "CreateWindowSurface");
    LOAD(MakeCurrent, "MakeCurrent");
    LOAD(SwapBuffers, "SwapBuffers");
    LOAD(SwapInterval, "SwapInterval");
    LOAD(DestroyContext, "DestroyContext");
    LOAD(GetCurrentContext, "GetCurrentContext");
    LOAD(GetError, "GetError");
    sEgl.GetPlatformDisplayEXT = (GetPlatformDisplayFn)sEgl.GetProcAddress("eglGetPlatformDisplayEXT");
    if (sEgl.GetPlatformDisplayEXT == NULL) {
        snprintf(sError, sizeof sError, "ANGLE's libEGL lacks EGL_EXT_platform_base");
        return false;
    }
    return true;
}

/* Display, config and window surface: once. */
static bool setup(SDL_Window *window)
{
    static const EGLint kConfigAttribs[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RED_SIZE, 8,
                                            EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 24, EGL_NONE};
    EGLint attribs[3];
    EGLint major = 0, minor = 0, count = 0;
    void *layer;

    if (sSurface != EGL_NO_SURFACE) {
        return true;
    }
    if (sEgl.lib == NULL && !load_library()) {
        return false;
    }
    attribs[0] = P4_EGL_PLATFORM_ANGLE_TYPE_ANGLE;
    attribs[1] = P4_EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE;
    attribs[2] = EGL_NONE;
    sDisplay = sEgl.GetPlatformDisplayEXT(P4_EGL_PLATFORM_ANGLE_ANGLE, NULL, attribs);
    if (sDisplay == EGL_NO_DISPLAY || !sEgl.Initialize(sDisplay, &major, &minor)) {
        fail("eglInitialize (ANGLE Metal)");
        return false;
    }
    if (!sEgl.ChooseConfig(sDisplay, kConfigAttribs, &sConfig, 1, &count) || count < 1) {
        fail("eglChooseConfig (RGBA8, 24-bit depth, ES 3)");
        return false;
    }
    sView = SDL_Metal_CreateView(window);
    layer = sView != NULL ? SDL_Metal_GetLayer(sView) : NULL;
    if (layer == NULL) {
        snprintf(sError, sizeof sError, "no CAMetalLayer for the window (was it created with SDL_WINDOW_METAL?): %s", SDL_GetError());
        return false;
    }
    sSurface = sEgl.CreateWindowSurface(sDisplay, sConfig, (EGLNativeWindowType)layer, NULL);
    if (sSurface == EGL_NO_SURFACE) {
        fail("eglCreateWindowSurface (CAMetalLayer)");
        return false;
    }
    return true;
}

void *p4gl_egl_create_context(SDL_Window *window)
{
    static const EGLint kContextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context;

    if (!setup(window)) {
        return NULL;
    }
    context = sEgl.CreateContext(sDisplay, sConfig, EGL_NO_CONTEXT, kContextAttribs);
    if (context == EGL_NO_CONTEXT) {
        fail("eglCreateContext (ES 3.0)");
        return NULL;
    }
    return context;
}

bool p4gl_egl_make_current(SDL_Window *window, void *context)
{
    (void)window;
    if (sDisplay == EGL_NO_DISPLAY) {
        return false;
    }
    if (!sEgl.MakeCurrent(sDisplay, context != NULL ? sSurface : EGL_NO_SURFACE, context != NULL ? sSurface : EGL_NO_SURFACE,
                          context != NULL ? (EGLContext)context : EGL_NO_CONTEXT)) {
        fail("eglMakeCurrent");
        return false;
    }
    return true;
}

void p4gl_egl_destroy_context(void *context)
{
    if (context != NULL && sDisplay != EGL_NO_DISPLAY) {
        sEgl.DestroyContext(sDisplay, (EGLContext)context);
    }
}

void *p4gl_egl_current_context(void)
{
    return sEgl.GetCurrentContext != NULL ? (void *)sEgl.GetCurrentContext() : NULL;
}

bool p4gl_egl_swap(SDL_Window *window)
{
    (void)window;
    if (!sEgl.SwapBuffers(sDisplay, sSurface)) {
        fail("eglSwapBuffers");
        return false;
    }
    return true;
}

bool p4gl_egl_set_swap_interval(int interval)
{
    return sDisplay != EGL_NO_DISPLAY && sEgl.SwapInterval(sDisplay, interval);
}

void (*p4gl_egl_proc(const char *name))(void)
{
    return sEgl.GetProcAddress != NULL ? sEgl.GetProcAddress(name) : NULL;
}

#endif /* P4PC_GL_VIA_EGL */
