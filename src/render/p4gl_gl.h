/* Minimal OpenGL declarations for src/render: the subset shared by desktop OpenGL 3.3 core and OpenGL ES 3.0
 * (src/render/p4gl_api.h chooses which context is created; docs/renderer_port.md has the audit). The entry points are loaded at GL
 * context open with SDL_GL_GetProcAddress (p4gl_gl_load in p4gl_gl.c), so the port does not
 * link libGL or libGLESv2. Only the functions the device layer calls are declared. */
#ifndef P4PC_RENDER_P4GL_GL_H
#define P4PC_RENDER_P4GL_GL_H

#include <stddef.h>

/* GL entry points are __stdcall on 32-bit Windows (the callee pops its arguments) and plain C
 * elsewhere. A pointer declared without it corrupts the stack on every call. */
#ifdef _WIN32
#define P4GL_APIENTRY __stdcall
#else
#define P4GL_APIENTRY
#endif

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef unsigned int GLbitfield;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLboolean;
typedef float GLfloat;
typedef double GLdouble;
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef void GLvoid;

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_NO_ERROR 0
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_RGBA 0x1908
#define GL_RGBA8 0x8058
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_SHORT 0x1403
#define GL_FLOAT 0x1406
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_REPEAT 0x2901
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_MIRRORED_REPEAT 0x8370
#define GL_CLAMP_TO_BORDER 0x812D
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_ONE 0x0001
#define GL_LINES 0x0001
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STREAM_DRAW 0x88E0
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_LEQUAL 0x0203
#define GL_ALWAYS 0x0207
#define GL_ZERO 0
#define GL_SRC_COLOR 0x0300
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_DST_ALPHA 0x0304
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_DST_COLOR 0x0306
#define GL_ONE_MINUS_DST_COLOR 0x0307
#define GL_SRC_ALPHA_SATURATE 0x0308
#define GL_CONSTANT_ALPHA 0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#define GL_FUNC_ADD 0x8006
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
#define GL_CULL_FACE 0x0B44
#define GL_CW 0x0900
#define GL_CCW 0x0901
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_UNSIGNED_INT 0x1405
#define GL_DEPTH_COMPONENT 0x1902
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_RENDERBUFFER 0x8D41
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03
#define GL_NUM_EXTENSIONS 0x821D
#define GL_MAJOR_VERSION 0x821B
#define GL_MINOR_VERSION 0x821C
typedef unsigned char GLubyte;
extern const GLubyte *(P4GL_APIENTRY *p4gl_glGetString)(GLenum name);
extern const GLubyte *(P4GL_APIENTRY *p4gl_glGetStringi)(GLenum name, GLuint index);
extern void (P4GL_APIENTRY *p4gl_glGetIntegerv)(GLenum pname, GLint *data);

extern void (P4GL_APIENTRY *p4gl_glGenTextures)(GLsizei n, GLuint *textures);
extern void (P4GL_APIENTRY *p4gl_glDeleteTextures)(GLsizei n, const GLuint *textures);
extern void (P4GL_APIENTRY *p4gl_glBindTexture)(GLenum target, GLuint texture);
extern void (P4GL_APIENTRY *p4gl_glTexImage2D)(GLenum target, GLint level, GLint internalFormat,
                                 GLsizei width, GLsizei height, GLint border,
                                 GLenum format, GLenum type, const void *pixels);
extern void (P4GL_APIENTRY *p4gl_glCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y,
                                                      GLsizei width, GLsizei height);
extern void (P4GL_APIENTRY *p4gl_glActiveTexture)(GLenum texture);
extern void (P4GL_APIENTRY *p4gl_glTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                    GLsizei width, GLsizei height, GLenum format, GLenum type,
                                    const void *pixels);
extern void (P4GL_APIENTRY *p4gl_glTexParameteri)(GLenum target, GLenum pname, GLint param);
extern void (P4GL_APIENTRY *p4gl_glPixelStorei)(GLenum pname, GLint param);
extern void (P4GL_APIENTRY *p4gl_glClearColor)(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
extern void (P4GL_APIENTRY *p4gl_glClear)(GLbitfield mask);
extern void (P4GL_APIENTRY *p4gl_glFlush)(void);
extern void (P4GL_APIENTRY *p4gl_glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
extern GLenum (P4GL_APIENTRY *p4gl_glGetError)(void);

/* Im2D (src/render/p4gl_im2d.c). */
extern void (P4GL_APIENTRY *p4gl_glEnable)(GLenum cap);
extern void (P4GL_APIENTRY *p4gl_glDisable)(GLenum cap);
extern void (P4GL_APIENTRY *p4gl_glBlendFunc)(GLenum sfactor, GLenum dfactor);
extern void (P4GL_APIENTRY *p4gl_glGenVertexArrays)(GLsizei n, GLuint *arrays);
extern void (P4GL_APIENTRY *p4gl_glBindVertexArray)(GLuint array);
extern void (P4GL_APIENTRY *p4gl_glGenBuffers)(GLsizei n, GLuint *buffers);
extern void (P4GL_APIENTRY *p4gl_glBindBuffer)(GLenum target, GLuint buffer);
extern void (P4GL_APIENTRY *p4gl_glBufferData)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
extern void (P4GL_APIENTRY *p4gl_glEnableVertexAttribArray)(GLuint index);
extern void (P4GL_APIENTRY *p4gl_glVertexAttribPointer)(GLuint index, GLint size, GLenum type,
                                          GLboolean normalized, GLsizei stride,
                                          const void *pointer);
extern void (P4GL_APIENTRY *p4gl_glVertexAttribDivisor)(GLuint index, GLuint divisor);
extern GLuint (P4GL_APIENTRY *p4gl_glCreateShader)(GLenum type);
extern void (P4GL_APIENTRY *p4gl_glDeleteShader)(GLuint shader);
extern void (P4GL_APIENTRY *p4gl_glShaderSource)(GLuint shader, GLsizei count, const GLchar *const *string,
                                   const GLint *length);
extern void (P4GL_APIENTRY *p4gl_glCompileShader)(GLuint shader);
extern void (P4GL_APIENTRY *p4gl_glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
extern void (P4GL_APIENTRY *p4gl_glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei *length,
                                       GLchar *infoLog);
extern GLuint (P4GL_APIENTRY *p4gl_glCreateProgram)(void);
extern void (P4GL_APIENTRY *p4gl_glDeleteProgram)(GLuint program);
extern void (P4GL_APIENTRY *p4gl_glAttachShader)(GLuint program, GLuint shader);
extern void (P4GL_APIENTRY *p4gl_glLinkProgram)(GLuint program);
extern void (P4GL_APIENTRY *p4gl_glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
extern void (P4GL_APIENTRY *p4gl_glUseProgram)(GLuint program);
extern GLint (P4GL_APIENTRY *p4gl_glGetUniformLocation)(GLuint program, const GLchar *name);
extern void (P4GL_APIENTRY *p4gl_glUniform2f)(GLint location, GLfloat v0, GLfloat v1);
extern void (P4GL_APIENTRY *p4gl_glUniform1i)(GLint location, GLint v0);
extern void (P4GL_APIENTRY *p4gl_glUniform1f)(GLint location, GLfloat v0);
extern void (P4GL_APIENTRY *p4gl_glDrawArrays)(GLenum mode, GLint first, GLsizei count);
extern void (P4GL_APIENTRY *p4gl_glDrawArraysInstanced)(GLenum mode, GLint first, GLsizei count, GLsizei instances);
extern void (P4GL_APIENTRY *p4gl_glDrawElements)(GLenum mode, GLsizei count, GLenum type, const void *indices);
extern void (P4GL_APIENTRY *p4gl_glDeleteBuffers)(GLsizei n, const GLuint *buffers);
extern void (P4GL_APIENTRY *p4gl_glDepthFunc)(GLenum func);
extern void (P4GL_APIENTRY *p4gl_glDepthMask)(GLboolean flag);
extern void (P4GL_APIENTRY *p4gl_glCullFace)(GLenum mode);
extern void (P4GL_APIENTRY *p4gl_glBlitFramebuffer)(GLint sx0, GLint sy0, GLint sx1, GLint sy1, GLint dx0,
                                      GLint dy0, GLint dx1, GLint dy1, GLbitfield mask, GLenum filter);
extern void (P4GL_APIENTRY *p4gl_glFrontFace)(GLenum mode);
extern void (P4GL_APIENTRY *p4gl_glColorMask)(GLboolean r, GLboolean g, GLboolean b, GLboolean a);
extern void (P4GL_APIENTRY *p4gl_glBlendEquation)(GLenum mode);
extern void (P4GL_APIENTRY *p4gl_glBlendFuncSeparate)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
extern void (P4GL_APIENTRY *p4gl_glBlendColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void (P4GL_APIENTRY *p4gl_glUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
extern void (P4GL_APIENTRY *p4gl_glUniform3fv)(GLint location, GLsizei count, const GLfloat *value);
extern void (P4GL_APIENTRY *p4gl_glUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
extern void (P4GL_APIENTRY *p4gl_glUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose,
                                       const GLfloat *value);
extern void (P4GL_APIENTRY *p4gl_glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose,
                                       const GLfloat *value);

/* Offscreen target and pixel readback (src/render/p4gl_readback.c). */
#define GL_FRAMEBUFFER 0x8D40
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
extern void (P4GL_APIENTRY *p4gl_glGenFramebuffers)(GLsizei n, GLuint *framebuffers);
extern void (P4GL_APIENTRY *p4gl_glBindFramebuffer)(GLenum target, GLuint framebuffer);
extern void (P4GL_APIENTRY *p4gl_glFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget,
                                           GLuint texture, GLint level);
extern GLenum (P4GL_APIENTRY *p4gl_glCheckFramebufferStatus)(GLenum target);
extern void (P4GL_APIENTRY *p4gl_glReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height,
                                 GLenum format, GLenum type, void *pixels);
extern void (P4GL_APIENTRY *p4gl_glGenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
extern void (P4GL_APIENTRY *p4gl_glDeleteFramebuffers)(GLsizei n, const GLuint *framebuffers);
extern void (P4GL_APIENTRY *p4gl_glDeleteRenderbuffers)(GLsizei n, const GLuint *renderbuffers);
extern void (P4GL_APIENTRY *p4gl_glBindRenderbuffer)(GLenum target, GLuint renderbuffer);
extern void (P4GL_APIENTRY *p4gl_glRenderbufferStorage)(GLenum target, GLenum internalformat,
                                          GLsizei width, GLsizei height);
extern void (P4GL_APIENTRY *p4gl_glFramebufferRenderbuffer)(GLenum target, GLenum attachment,
                                              GLenum renderbuffertarget, GLuint renderbuffer);

/* Which API the loaded context is: 1 for OpenGL ES (the version string starts with "OpenGL ES"), 0 for desktop OpenGL. Valid after p4gl_gl_load. */
int p4gl_gl_is_es(void);
/* 1 when GL_CLAMP_TO_BORDER can be used: desktop OpenGL, ES 3.2, or GL_EXT_texture_border_clamp / GL_OES_texture_border_clamp. Else the
 * fragment shaders emulate the border (p4SampleBorder; p4gl_texture_border_mode in p4gl_im2d.c). P4PC_GLES_BORDER=shader forces the
 * emulation on any context, =native the hardware mode. Valid after p4gl_gl_load. */
int p4gl_gl_border_native(void);
/* Compiles one shader stage. `body` is GLSL without the #version line: this prepends the header of the loaded context (`#version 330 core`,
 * or `#version 300 es` with highp default precision) and, for fragment shaders, p4SampleBorder. Prints the log with `what` and returns 0 on failure. */
GLuint p4gl_gl_compile_shader(GLenum stage, const char *body, const char *what);
/* Existing optional hitch stream: zero clock reads when disabled. */
uint64_t p4gl_gl_hitch_begin(void);
/* Potentially blocking API records are emitted only at >=1 ms, avoiding a CSV
 * write for every timestamp/draw. Compile/link/upload events stay full. */
void p4gl_gl_hitch_api(const char *kind, uint64_t start, GLuint object, int width, int height);

/* Load every entry point above. Returns 0 when all are present and 1 otherwise. Needs a
 * current GL context. */
int p4gl_gl_load(void);
/* The limit a gate judges a frame by: the environment variable `envName` (the ctest limit), or the row of tests/gate_limits.txt for this renderer. */
const char *p4gl_gate_limit(const char *envName);
/* Enables (1) or drops (0) glClear / glDrawArrays / glDrawElements; P4PC_TURBO=2 (p4gl_context.c). Call after p4gl_gl_load. */
void p4gl_gl_set_draw(int on);
/* Drop clears/draws inside hidden ordering-table scopes (p4gl_im2d.c). */
void p4gl_gl_suppress_draw(int suppressed);
/* 0 while P4PC_TURBO=2 drops the draw calls: nothing was drawn, so a copy of the framebuffer would copy the same stale pixels. */
int p4gl_gl_draw_enabled(void);
/* Whether a draw needs CPU/GL preparation. Undrawn turbo frames can omit port-only draw work,
 * but explicit draw/GS observers and GL statistics keep the complete submission path. */
int p4gl_gl_prepare_draw(void);
void p4gl_gl_stats_report(unsigned frames, double swapMs, double presentMs); /* P4PC_GLSTATS */
/* A render pass of this layer's own, in the middle of a frame (src/render/p4gl_yuv.c, the movie picture conversion). The draws around it
 * set the state they need only when the game's render state changes, so the pass saves what it touches and puts it back: the recorded
 * fixed-function state, viewport, program, vertex array and the textures of units 0..3 with the active unit. begin leaves blend, depth test,
 * culling and the colour mask off/full for the pass. Returns 0 when the pass cannot be undone (P4PC_GLCACHE=0: no record of the state). */
int p4gl_gl_pass_begin(void);
void p4gl_gl_pass_end(void);

/* Context loss (p4gl_context_recover in p4gl_context.c; Android drops the EGL context when the system takes the GPU away, and ANGLE can lose its device).
 * The new context holds none of the old one's objects, so each module forgets its GL names (never deleting them: the names mean other objects in the new
 * context) and rebuilds lazily, and the call wrappers' caches of what the context holds go back to "unknown". p4gl_gl_load has re-resolved the entry points. */
void p4gl_gl_context_reset(void);
void p4gl_im2d_context_lost(void);
void p4gl_im3d_context_lost(void);
void p4gl_atomic_context_lost(void);
/* Starts a screen-model queue; attached portrait glints share only this queue's anchors. */
void p4gl_atomic_model_scope_begin(void);
/* Textures are rebuilt from the rasters' CPU shadows (the same upload as at unlock); render targets and frame copies are recreated by their next use. */
void p4gl_raster_context_lost(void);
/* 1 when the context rounds the shaders' float colours to 8 bits (and the shaders truncate them as the GS does, P4_ROUND_BIAS), 0 when it truncates like the GS. */
int p4gl_gl_round_bias(void);
/* Clears the bound framebuffer's colour to black, ignoring the colour mask the game recorded (the letterbox bars of the present blit). */
void p4gl_gl_clear_black(void);

#endif
