/* A no-device rendering backend, not an alternate game update path. CPU-side
 * RW objects/rasters and every game draw callback continue to run. Graphics
 * resource names/state are local bookkeeping; no shaders/textures/framebuffers
 * are sent to a driver, no window/context exists, and pixels cannot be read.
 * Fidelity qualification must prove that omitted device work has no effect on
 * battle state/RNG. A successful run is not graphics/shader qualification. */
#include "p4pc/battle_sim.h"
#include "../../render/p4gl_gl.h"
#include <stdio.h>
#include <stdlib.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#define SINK(name, params) static void P4GL_APIENTRY sink_##name params {}
SINK(DeleteTextures, (GLsizei n, const GLuint *objects))
SINK(TexImage2D, (GLenum target, GLint level, GLint internal_format, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels))
SINK(CopyTexSubImage2D, (GLenum target, GLint level, GLint xo, GLint yo, GLint x, GLint y, GLsizei width, GLsizei height))
SINK(ActiveTexture, (GLenum texture))
SINK(TexSubImage2D, (GLenum target, GLint level, GLint xo, GLint yo, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels))
SINK(TexParameteri, (GLenum target, GLenum name, GLint value))
SINK(PixelStorei, (GLenum name, GLint value))
SINK(ClearColor, (GLfloat r, GLfloat g, GLfloat b, GLfloat a))
SINK(Clear, (GLbitfield mask))
SINK(Flush, (void))
SINK(Enable, (GLenum capability))
SINK(Disable, (GLenum capability))
SINK(BlendFunc, (GLenum source, GLenum destination))
SINK(BindVertexArray, (GLuint object))
SINK(BindBuffer, (GLenum target, GLuint object))
SINK(BufferData, (GLenum target, GLsizeiptr size, const void *data, GLenum usage))
SINK(EnableVertexAttribArray, (GLuint index))
SINK(VertexAttribPointer, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer))
SINK(VertexAttribDivisor, (GLuint index, GLuint divisor))
SINK(DeleteShader, (GLuint shader))
SINK(ShaderSource, (GLuint shader, GLsizei count, const GLchar *const *strings, const GLint *lengths))
SINK(CompileShader, (GLuint shader))
SINK(DeleteProgram, (GLuint program))
SINK(AttachShader, (GLuint program, GLuint shader))
SINK(LinkProgram, (GLuint program))
SINK(UseProgram, (GLuint program))
SINK(Uniform2f, (GLint location, GLfloat a, GLfloat b))
SINK(Uniform1i, (GLint location, GLint value))
SINK(Uniform1f, (GLint location, GLfloat value))
SINK(DrawArrays, (GLenum mode, GLint first, GLsizei count))
SINK(DrawArraysInstanced, (GLenum mode, GLint first, GLsizei count, GLsizei instances))
SINK(DrawElements, (GLenum mode, GLsizei count, GLenum type, const void *indices))
SINK(DeleteBuffers, (GLsizei count, const GLuint *objects))
SINK(DepthFunc, (GLenum function))
SINK(DepthMask, (GLboolean enabled))
SINK(CullFace, (GLenum mode))
SINK(BlitFramebuffer, (GLint sx0, GLint sy0, GLint sx1, GLint sy1, GLint dx0, GLint dy0, GLint dx1, GLint dy1, GLbitfield mask, GLenum filter))
SINK(FrontFace, (GLenum mode))
SINK(ColorMask, (GLboolean r, GLboolean g, GLboolean b, GLboolean a))
SINK(BlendEquation, (GLenum mode))
SINK(BlendFuncSeparate, (GLenum source_rgb, GLenum destination_rgb, GLenum source_alpha, GLenum destination_alpha))
SINK(BlendColor, (GLfloat r, GLfloat g, GLfloat b, GLfloat a))
SINK(Uniform3f, (GLint location, GLfloat a, GLfloat b, GLfloat c))
SINK(Uniform3fv, (GLint location, GLsizei count, const GLfloat *value))
SINK(Uniform4f, (GLint location, GLfloat a, GLfloat b, GLfloat c, GLfloat d))
SINK(UniformMatrix3fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))
SINK(UniformMatrix4fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))
SINK(FramebufferTexture2D, (GLenum target, GLenum attachment, GLenum texture_target, GLuint texture, GLint level))
SINK(DeleteFramebuffers, (GLsizei count, const GLuint *objects))
SINK(DeleteRenderbuffers, (GLsizei count, const GLuint *objects))
SINK(RenderbufferStorage, (GLenum target, GLenum format, GLsizei width, GLsizei height))
SINK(FramebufferRenderbuffer, (GLenum target, GLenum attachment, GLenum renderbuffer_target, GLuint object))
#undef SINK

static GLuint next_name = 1, texture, depth, read_framebuffer, draw_framebuffer;
static GLint viewport[4] = {0, 0, 640, 448};
static void P4GL_APIENTRY names(GLsizei n, GLuint *objects)
{
    for (GLsizei i = 0; i < n; i++) {
        if (!next_name) abort();
        objects[i] = next_name++;
    }
}
static GLuint P4GL_APIENTRY create_program(void) { GLuint name; names(1, &name); return name; }
static GLuint P4GL_APIENTRY create_shader(GLenum type) { return create_program(); }
static void P4GL_APIENTRY bind_texture(GLenum target, GLuint object) { texture = object; }
static void P4GL_APIENTRY bind_depth(GLenum target, GLuint object) { depth = object; }
static void P4GL_APIENTRY bind_framebuffer(GLenum target, GLuint object)
{
    if (target == GL_READ_FRAMEBUFFER || target == GL_FRAMEBUFFER) read_framebuffer = object;
    if (target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER) draw_framebuffer = object;
}
static void P4GL_APIENTRY set_viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    viewport[0] = x; viewport[1] = y; viewport[2] = width; viewport[3] = height;
}
static GLenum P4GL_APIENTRY no_error(void) { return GL_NO_ERROR; }
static GLenum P4GL_APIENTRY framebuffer_status(GLenum target) { return GL_FRAMEBUFFER_COMPLETE; }
static GLint P4GL_APIENTRY uniform_location(GLuint program, const GLchar *name) { return -1; }
static void P4GL_APIENTRY object_status(GLuint object, GLenum name, GLint *value)
{
    *value = name == GL_COMPILE_STATUS || name == GL_LINK_STATUS ? 1 : 0;
}
static void P4GL_APIENTRY object_log(GLuint object, GLsizei size, GLsizei *length, GLchar *log)
{
    if (length) *length = 0;
    if (size > 0 && log) log[0] = 0;
}
static const GLubyte *P4GL_APIENTRY description(GLenum name)
{
    return (const GLubyte *)(name == GL_RENDERER ? "battle-sim/no-device" : name == GL_VERSION ? "3.3 no-device" : "");
}
static const GLubyte *P4GL_APIENTRY extension(GLenum name, GLuint index) { return (const GLubyte *)""; }
static void P4GL_APIENTRY integer(GLenum name, GLint *value)
{
    if (name == 0x0ba2u) { for (unsigned i = 0; i < 4; i++) value[i] = viewport[i]; }
    else if (name == 0x0d33u || name == 0x84e8u) *value = 16384;
    else if (name == 0x8caau) *value = (GLint)read_framebuffer;
    else if (name == 0x8ca6u) *value = (GLint)draw_framebuffer;
    else if (name == 0x8069u) *value = (GLint)texture;
    else if (name == 0x8ca7u) *value = (GLint)depth;
    else *value = 0;
}
static void P4GL_APIENTRY forbidden_read(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels)
{
    fprintf(stderr, "p4pc: battle-sim: unexpected pixel read in no-device mode\n");
    exit(4);
}

int p4_battle_sim_graphics_open(void)
{
    if (!p4_battle_sim_renderless) return -1;
#define BIND(name) p4gl_gl##name = sink_##name
    BIND(DeleteTextures); BIND(TexImage2D); BIND(CopyTexSubImage2D); BIND(ActiveTexture); BIND(TexSubImage2D);
    BIND(TexParameteri); BIND(PixelStorei); BIND(ClearColor); BIND(Clear); BIND(Flush); BIND(Enable); BIND(Disable);
    BIND(BlendFunc); BIND(BindVertexArray); BIND(BindBuffer); BIND(BufferData); BIND(EnableVertexAttribArray);
    BIND(VertexAttribPointer); BIND(VertexAttribDivisor); BIND(DeleteShader); BIND(ShaderSource); BIND(CompileShader);
    BIND(DeleteProgram); BIND(AttachShader); BIND(LinkProgram); BIND(UseProgram); BIND(Uniform2f); BIND(Uniform1i);
    BIND(Uniform1f); BIND(DrawArrays); BIND(DrawArraysInstanced); BIND(DrawElements); BIND(DeleteBuffers); BIND(DepthFunc);
    BIND(DepthMask); BIND(CullFace); BIND(BlitFramebuffer); BIND(FrontFace); BIND(ColorMask); BIND(BlendEquation);
    BIND(BlendFuncSeparate); BIND(BlendColor); BIND(Uniform3f); BIND(Uniform3fv); BIND(Uniform4f);
    BIND(UniformMatrix3fv); BIND(UniformMatrix4fv); BIND(FramebufferTexture2D); BIND(DeleteFramebuffers);
    BIND(DeleteRenderbuffers); BIND(RenderbufferStorage); BIND(FramebufferRenderbuffer);
#undef BIND
    p4gl_glGenTextures = p4gl_glGenVertexArrays = p4gl_glGenBuffers = p4gl_glGenFramebuffers = p4gl_glGenRenderbuffers = names;
    p4gl_glBindTexture = bind_texture;
    p4gl_glBindRenderbuffer = bind_depth;
    p4gl_glBindFramebuffer = bind_framebuffer;
    p4gl_glViewport = set_viewport;
    p4gl_glGetError = no_error;
    p4gl_glCheckFramebufferStatus = framebuffer_status;
    p4gl_glCreateProgram = create_program;
    p4gl_glCreateShader = create_shader;
    p4gl_glGetUniformLocation = uniform_location;
    p4gl_glGetShaderiv = p4gl_glGetProgramiv = object_status;
    p4gl_glGetShaderInfoLog = object_log;
    p4gl_glGetString = description;
    p4gl_glGetStringi = extension;
    p4gl_glGetIntegerv = integer;
    p4gl_glReadPixels = forbidden_read;
    p4gl_gl_set_draw(0);
    fprintf(stderr, "p4pc: battle-sim: no-device graphics (no window, context, submission or readback)\n");
    return 0;
}
#pragma clang diagnostic pop
