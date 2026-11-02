/* p4gl_yuv: the movie picture's colour conversion on the GPU, exactly.
 *
 * The game draws the movie from a texture holding the picture as RGBA8. The CPU converter (src/movie/mpeg2_dec.c,
 * p4mpeg2_frame_to_rgba_ipu: libswscale with the IPU's matrix and rounding) costs a conversion, a copy and an upload of a megabyte per
 * frame. Here the picture's three planes go to the GPU (a third of the bytes) and a fragment shader turns them into the raster's texture.
 *
 * The integer shader reproduces the pinned converter exactly; movie_colour_test checks all 2^24 triples.
 * Chroma is replicated 4:2:0. Unqualified native CPU backends retain CPU conversion.
 *
 * The pass runs in the middle of a frame, between draws that set their state only when the game's render state changes: p4gl_gl_pass_begin
 * and p4gl_gl_pass_end (p4gl_gl.c) save and restore what it touches. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4gl.h"
#include "p4gl_gl.h"
#include "p4gl_profile.h"
#include "p4gl_readback.h"
#include "p4gl_yuv_shader.h"

#ifndef GL_R8
#define GL_R8 0x8229
#endif
#ifndef GL_RED
#define GL_RED 0x1903
#endif

#define MAX_PICTURE_SIZE 4096

static int sFailed;                 /* a setup step failed: the CPU converts */
static GLuint sProgram, sVao, sFbo; /* the pass, built at the first picture */
static GLuint sPlane[3];            /* Y, Cb, Cr as R8 textures of the picture's size */
static int32_t sPlaneW, sPlaneH;
static int sFboChecked;

static const char kVertex[] =
    "const vec2 kCorner[6] = vec2[6](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0), vec2(-1.0, 1.0), vec2(1.0, -1.0), vec2(1.0, 1.0));\n"
    "void main() {\n"
    "    gl_Position = vec4(kCorner[gl_VertexID], 0.0, 1.0);\n"
    "}\n";

static void nearest(void)
{
    p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}


/* Inside a pass: the program, the empty vertex array, the framebuffer. The two triangles cover the viewport exactly: one big triangle
 * would reach outside it, and rasterisers that do not clip to the viewport (llvmpipe) would write the texture's padding. */
static int passSetup(void)
{
    GLuint vs;
    GLuint fs;
    GLint linked = GL_FALSE;

    if (sProgram != 0) {
        return 1;
    }
    vs = p4gl_gl_compile_shader(GL_VERTEX_SHADER, kVertex, "movie colour");
    fs = vs != 0 ? p4gl_gl_compile_shader(GL_FRAGMENT_SHADER, kP4YuvIntegerFragment, "movie colour") : 0;
    if (vs == 0 || fs == 0) {
        return 0;
    }
    sProgram = p4gl_glCreateProgram();
    p4gl_glAttachShader(sProgram, vs);
    p4gl_glAttachShader(sProgram, fs);
    p4gl_glLinkProgram(sProgram);
    p4gl_glGetProgramiv(sProgram, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        fprintf(stderr, "p4gl: movie colour program link failed\n");
        sProgram = 0;
        return 0;
    }
    p4gl_glUseProgram(sProgram);
    p4gl_glUniform1i(p4gl_glGetUniformLocation(sProgram, "uY"), 0);
    p4gl_glUniform1i(p4gl_glGetUniformLocation(sProgram, "uU"), 1);
    p4gl_glUniform1i(p4gl_glGetUniformLocation(sProgram, "uV"), 2);
    p4gl_glGenVertexArrays(1, &sVao);
    p4gl_glGenFramebuffers(1, &sFbo);
    return 1;
}

static void planeTexture(int i, int32_t w, int32_t h, int32_t stride, const uint8_t *data, int create)
{
    p4gl_glActiveTexture(GL_TEXTURE0 + (unsigned)i);
    if (sPlane[i] == 0) {
        p4gl_glGenTextures(1, &sPlane[i]);
        create = 1;
    }
    p4gl_glBindTexture(GL_TEXTURE_2D, sPlane[i]);
    if (create) {
        nearest();
        p4gl_glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    }
    p4gl_glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
    p4gl_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED, GL_UNSIGNED_BYTE, data);
    p4gl_glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

int p4gl_raster_set_yuv420(P4RwRaster *r, const uint8_t *const plane[3], const int32_t stride[3], int32_t w, int32_t h)
{
    const int32_t cw = (w + 1) / 2;
    const int32_t ch = (h + 1) / 2;
    GLuint dst;
    int resized;

    if (!P4GL_YUV_INTEGER_SUPPORTED) return -1;
    if (sFailed || r == NULL || plane == NULL || w != r->width || h != r->height || w <= 0 || h <= 0 || w > MAX_PICTURE_SIZE || h > MAX_PICTURE_SIZE) {
        return -1;
    }
    dst = p4gl_raster_image_texture(r);
    if (dst == 0 || !p4gl_gl_pass_begin()) {
        return -1;
    }
    if (p4gl_profile_pass != NULL) p4gl_profile_pass(P4GL_PROFILE_MOVIE);
    if (!passSetup()) {
        sFailed = 1;
        p4gl_gl_pass_end();
        return -1;
    }
    p4gl_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    resized = sPlaneW != w || sPlaneH != h;
    planeTexture(0, w, h, stride[0], plane[0], resized);
    planeTexture(1, cw, ch, stride[1], plane[1], resized);
    planeTexture(2, cw, ch, stride[2], plane[2], resized);
    sPlaneW = w;
    sPlaneH = h;

    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, sFbo);
    p4gl_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst, 0);
    if (!sFboChecked) {
        sFboChecked = 1;
        if (p4gl_glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "p4gl: movie colour framebuffer incomplete\n");
            sFailed = 1;
        }
    }
    if (!sFailed) {
        p4gl_glViewport(0, 0, w, h);
        p4gl_glUseProgram(sProgram);
        p4gl_glBindVertexArray(sVao);
        p4gl_glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, p4gl_readback_framebuffer());
    p4gl_gl_pass_end();
    return sFailed ? -1 : 0;
}
