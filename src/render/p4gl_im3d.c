/* RwIm3D on OpenGL 3.3 core (replaces the sky2 transform and render pipelines; docs/renderer_port.md
 * section 9). The six RwIm3D entry points (overrides/00410420.c, 004104d0.c, 00410520.c, 004106a0.c,
 * 00410800.c, 00410930.c) keep the retail state block and call this file for the draw.
 *
 * RwIm3DTransform copies the caller's RwIm3DVertex array (RxObjSpace3DVertex, 36 bytes: position
 * @0, RGBA bytes @12, normal @16, uv @28) and moves it to clip space with the object-to-world
 * matrix and the camera of the open update (p4gl_camera_object_matrix), exactly once. Every
 * RwIm3DRender* call between Transform and End draws from that buffer with its own program.
 * Triangles and native-density lines are clipped by the GPU; scaled lines are clipped before
 * expanding to virtual-pixel-wide quads, since GLES only guarantees one-physical-pixel lines.
 * ALLOPAQUE forces alpha to 1. The uv are always read from the vertex, whatever the VERTEXUV flag says: the
 * retail PS2 vertex always carries them and the sky pipeline passes them through (P4's shadow
 * projection passes flags without the bit and still needs its texcoords; sky2hook measured the
 * flag-gated read as worse, src/pipelines/im3d/im3d.cpp). Vertex colour modulates the texel of the
 * raster TEXTURERASTER names. Blending and the alpha test follow RpSky ALPHA_1 and TEST_1; the depth
 * test and depth write follow RwRenderState ZTESTENABLE and ZWRITEENABLE.
 *
 * The shared clip-space draw at the end also serves the PTank node (src/render/p4gl_ptank.c).
 * Draw-state choices here (uv always read, depth defaults, GS blend and test decoding) follow
 * sky2hook src/pipelines/im3d/im3d.cpp, tge-was-taken, reused with permission; see
 * third_party/README.md. */
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4gl.h"
#include "p4gl_gl.h"
#include "p4gl_profile.h"

#define P4_IM3D_VERTEX_BYTES 36u
#define P4_IM3D_OFF_COLOR 12u
#define P4_IM3D_OFF_UV 28u
#define P4_IM3D_ALLOPAQUE 2u

/* RwPrimitiveType (decomp/include/rw/sky2/rwplcore.h). */
#define P4_PRIM_LINELIST 1
#define P4_PRIM_POLYLINE 2
#define P4_PRIM_TRILIST 3
#define P4_PRIM_TRISTRIP 4
#define P4_PRIM_TRIFAN 5

static const char kIm3DVertexSource[] =
    "layout(location = 0) in vec4 aPos;\n"
    "layout(location = 1) in vec4 aCol;\n"
    "layout(location = 2) in vec2 aUv;\n"
    "out vec4 vCol;\n"
    "out float vColorW;\n"
    "out vec2 vUv;\n"
    "void main() {\n"
    "    gl_Position = aPos;\n"
    "    /* Screen-affine GS RGBA, without a GLES noperspective extension. */\n"
    "    vCol = aCol * aPos.w;\n"
    "    vColorW = aPos.w;\n"
    "    vUv = aUv;\n"
    "}\n";

static const char kIm3DFragmentSource[] =
    "in vec4 vCol;\n"
    "in float vColorW;\n"
    "in vec2 vUv;\n"
    "uniform sampler2D uTex;\n"
    "uniform int uTextured;\n"
    "uniform int uBorder;\n"
    "uniform vec4 uMatColor;\n"
    "uniform int uAlphaFunc;\n"
    "uniform float uAlphaRef;\n"
    "uniform int uAlphaInvert;\n"
    "bool alphaPasses(float a) {\n"
    "    float t = a * 128.0;\n"
    "    if (uAlphaFunc == 8) return false;\n"
    "    if (uAlphaFunc == 2) return t < uAlphaRef;\n"
    "    if (uAlphaFunc == 3) return t <= uAlphaRef;\n"
    "    if (uAlphaFunc == 4) return t == uAlphaRef;\n"
    "    if (uAlphaFunc == 5) return t >= uAlphaRef;\n"
    "    if (uAlphaFunc == 6) return t > uAlphaRef;\n"
    "    if (uAlphaFunc == 7) return t != uAlphaRef;\n"
    "    return true;\n"
    "}\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    vec4 c = (vCol / vColorW) * uMatColor;\n"
    "    if (uTextured != 0) {\n"
    "        c *= (uBorder != 0) ? p4SampleBorder(uTex, vUv, uBorder) : texture(uTex, vUv);\n"
    "    }\n"
    "    if (uAlphaFunc != 0 && alphaPasses(c.a) == (uAlphaInvert != 0)) {\n"
    "        discard;\n"
    "    }\n"
    "#ifdef P4_ROUND_BIAS\n"
    "    c.rgb = max(c.rgb - 0.5 / 255.0, 0.0);\n"
    "#endif\n"
    "    fragColor = c;\n"
    "}\n";

static P4ClipVertex *sClip;   /* transformed vertices of the open Transform/End pair */
static uint32_t sCount;           /* vertices in sClip */
static uint32_t sCapacity;        /* allocated vertices in sClip */
static int sUploaded;             /* sVbo holds sClip */

static int sReady;
static int sFailed;
static GLuint sProgram;
static GLint sLocTex;
static GLint sLocTextured;
static GLint sLocBorder;
static GLint sLocMatColor;
static GLint sLocAlphaFunc;
static GLint sLocAlphaRef;
static GLint sLocAlphaInvert;
static GLuint sVao;
static GLuint sVbo;
static GLuint sEbo;
static P4ClipVertex *sLineVertices;
static uint32_t *sLineIndices;
static uint32_t sLineCapacity; /* segments; scratch and immutable quad index pattern */
static GLuint sLineEbo;
static uint32_t sLineGpuCapacity;

static float f32_at(const uint8_t *base, size_t off)
{
    float value;

    memcpy(&value, base + off, sizeof value);
    return value;
}

/* Copies numVerts vertices and moves them to clip space. numVerts is the 16-bit count the retail
 * transform node reads, so the caller passes the truncated value. */
int p4gl_im3d_transform(const void *verts, uint32_t numVerts, const void *ltm, uint32_t flags)
{
    const uint8_t *src = verts;
    const void *camera = p4gl_camera_current();
    float m[4][4];
    float mvp[16];
    uint32_t i;

    sCount = 0;
    sUploaded = 0;
    if (camera == NULL) {
        fprintf(stderr, "p4gl_im3d: RwIm3DTransform outside a camera update\n");
        return 0;
    }
    if (!p4gl_camera_object_matrix(ltm, camera, m, mvp)) {
        return 0;
    }
    if (numVerts > sCapacity) {
        P4ClipVertex *grown = realloc(sClip, (size_t)numVerts * sizeof *grown);

        if (grown == NULL) {
            fprintf(stderr, "p4gl_im3d: cannot allocate %u vertices\n", (unsigned)numVerts);
            return 0;
        }
        sClip = grown;
        sCapacity = numVerts;
    }
    for (i = 0; i < numVerts; i++) {
        const uint8_t *v = src + (size_t)i * P4_IM3D_VERTEX_BYTES;
        P4ClipVertex *out = &sClip[i];
        const float x = f32_at(v, 0);
        const float y = f32_at(v, 4);
        const float z = f32_at(v, 8);
        int r;

        for (r = 0; r < 4; r++) {
            out->pos[r] = mvp[r] * x + mvp[4 + r] * y + mvp[8 + r] * z + mvp[12 + r];
        }
        out->col[0] = v[P4_IM3D_OFF_COLOR + 0] / 255.0f;
        out->col[1] = v[P4_IM3D_OFF_COLOR + 1] / 255.0f;
        out->col[2] = v[P4_IM3D_OFF_COLOR + 2] / 255.0f;
        out->col[3] = (flags & P4_IM3D_ALLOPAQUE) ? 1.0f : v[P4_IM3D_OFF_COLOR + 3] / 255.0f;
        out->uv[0] = f32_at(v, P4_IM3D_OFF_UV);
        out->uv[1] = f32_at(v, P4_IM3D_OFF_UV + 4);
    }
    sCount = numVerts;
    return 1;
}

void p4gl_im3d_end(void)
{
    sCount = 0;
    sUploaded = 0;
}

static GLuint compile_stage(GLenum type, const char *source)
{
    return p4gl_gl_compile_shader(type, source, "Im3D");
}

static int ensure_ready(void)
{
    GLuint vs;
    GLuint fs;
    GLint ok = 0;

    if (sReady) {
        return 1;
    }
    if (sFailed || p4gl_glCreateShader == NULL) {
        return 0;
    }
    vs = compile_stage(GL_VERTEX_SHADER, kIm3DVertexSource);
    fs = compile_stage(GL_FRAGMENT_SHADER, kIm3DFragmentSource);
    if (vs == 0 || fs == 0) {
        sFailed = 1;
        return 0;
    }
    sProgram = p4gl_glCreateProgram();
    p4gl_glAttachShader(sProgram, vs);
    p4gl_glAttachShader(sProgram, fs);
    p4gl_glLinkProgram(sProgram);
    p4gl_glGetProgramiv(sProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        fprintf(stderr, "p4gl_im3d: program link failed\n");
        sFailed = 1;
        return 0;
    }
    sLocTex = p4gl_glGetUniformLocation(sProgram, "uTex");
    sLocTextured = p4gl_glGetUniformLocation(sProgram, "uTextured");
    sLocBorder = p4gl_glGetUniformLocation(sProgram, "uBorder");
    sLocMatColor = p4gl_glGetUniformLocation(sProgram, "uMatColor");
    sLocAlphaFunc = p4gl_glGetUniformLocation(sProgram, "uAlphaFunc");
    sLocAlphaRef = p4gl_glGetUniformLocation(sProgram, "uAlphaRef");
    sLocAlphaInvert = p4gl_glGetUniformLocation(sProgram, "uAlphaInvert");

    p4gl_glGenVertexArrays(1, &sVao);
    p4gl_glGenBuffers(1, &sVbo);
    p4gl_glGenBuffers(1, &sEbo);
    p4gl_glBindVertexArray(sVao);
    p4gl_glBindBuffer(GL_ARRAY_BUFFER, sVbo);
    p4gl_glEnableVertexAttribArray(0);
    p4gl_glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(P4ClipVertex),
                               (const void *)offsetof(P4ClipVertex, pos));
    p4gl_glEnableVertexAttribArray(1);
    p4gl_glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(P4ClipVertex),
                               (const void *)offsetof(P4ClipVertex, col));
    p4gl_glEnableVertexAttribArray(2);
    p4gl_glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(P4ClipVertex),
                               (const void *)offsetof(P4ClipVertex, uv));
    p4gl_glUseProgram(sProgram);
    p4gl_glUniform1i(sLocTex, 0);
    sReady = 1;
    return 1;
}

static GLenum prim_mode(int32_t primType)
{
    switch (primType) {
    case P4_PRIM_LINELIST:
        return GL_LINES;
    case P4_PRIM_POLYLINE:
        return GL_LINE_STRIP;
    case P4_PRIM_TRILIST:
        return GL_TRIANGLES;
    case P4_PRIM_TRISTRIP:
        return GL_TRIANGLE_STRIP;
    case P4_PRIM_TRIFAN:
        return GL_TRIANGLE_FAN;
    default:
        return 0;
    }
}

/* P4PC_GS_LOG (docs/renderer_port.md 11): the clip-space vertices' screen box, the draw's texture and ALPHA / TEST (its own for PTank, the game's otherwise). */
static void im3d_gs_log(const P4ClipVertex *v, uint32_t n, const P4ClipDrawParams *dp, GLenum mode, int32_t count)
{
    float box[4] = { 1e9f, 1e9f, -1e9f, -1e9f };
    float rgba[4];
    uint32_t i;

    for (i = 0; i < n; i++) {
        if (v[i].pos[3] > 1e-4f) {
            const float x = (v[i].pos[0] / v[i].pos[3] * 0.5f + 0.5f) * 640.0f;
            const float y = (0.5f - v[i].pos[1] / v[i].pos[3] * 0.5f) * 448.0f;

            if (x < box[0]) box[0] = x;
            if (x > box[2]) box[2] = x;
            if (y < box[1]) box[1] = y;
            if (y > box[3]) box[3] = y;
        }
    }
    if (box[0] > box[2]) {
        box[0] = box[1] = box[2] = box[3] = 0.0f;
    }
    rgba[0] = dp->color[0] * 255.0f; rgba[1] = dp->color[1] * 255.0f; rgba[2] = dp->color[2] * 255.0f; rgba[3] = dp->color[3] * 255.0f;
    p4gl_gs_log_draw(dp->own ? "ptank" : "im3d", (int)mode, (int)count, box, 0, rgba, dp->raster, dp->own ? (dp->blend ? dp->alpha : 0u) : p4gl_sky_alpha_raw(),
                     dp->own ? dp->test : p4gl_sky_test_raw(), dp->own ? (unsigned)dp->zTest : (unsigned)p4gl_render_state_or(P4GL_RS_ZTESTENABLE, 1u),
                     dp->own ? (unsigned)dp->zWrite : (unsigned)p4gl_render_state_or(P4GL_RS_ZWRITEENABLE, 0u));
}

/* Homogeneous clipping must precede division by W. The shader makes colours
 * screen-affine (col * W / W), while UV remain perspective-correct, so clipped
 * endpoint colours interpolate col*W and UV interpolate directly. */
static void line_endpoint(P4ClipVertex *out, const P4ClipVertex *a,
                          const P4ClipVertex *b, float t)
{
    if (t == 0.0f) { *out = *a; return; }
    if (t == 1.0f) { *out = *b; return; }
    for (unsigned c = 0; c < 4; c++) out->pos[c] = a->pos[c] + t * (b->pos[c] - a->pos[c]);
    for (unsigned c = 0; c < 4; c++)
        out->col[c] = (a->col[c] * a->pos[3] * (1.0f - t) +
                       b->col[c] * b->pos[3] * t) / out->pos[3];
    for (unsigned c = 0; c < 2; c++) out->uv[c] = a->uv[c] + t * (b->uv[c] - a->uv[c]);
}

/* Returns visible segments, or -1 on allocation failure. The CPU scratch and
 * GPU index pattern grow only when necessary, and survive draws/context loss. */
static int expand_lines(GLenum mode, const P4ClipVertex *verts, uint32_t numVerts,
                        const uint16_t *indices, uint32_t count, int width, int height)
{
    const uint32_t sourceCount = indices != NULL ? count : numVerts;
    const uint32_t segments = mode == GL_LINES ? sourceCount / 2u :
                              sourceCount > 1u ? sourceCount - 1u : 0u;
    uint32_t visible = 0;
    if (segments == 0) return 0;
    if (segments > INT32_MAX / 6u || segments > PTRDIFF_MAX / (4u * sizeof *sLineVertices)) return -1;
    if (segments > sLineCapacity) {
        uint32_t capacity = segments > sLineCapacity * 2u ? segments : sLineCapacity * 2u;
        P4ClipVertex *vertices;
        if (capacity > INT32_MAX / 6u || capacity > PTRDIFF_MAX / (4u * sizeof *vertices)) capacity = segments;
        vertices = realloc(sLineVertices, (size_t)capacity * 4u * sizeof *vertices);
        uint32_t *pattern;
        if (vertices == NULL) return -1;
        sLineVertices = vertices;
        pattern = realloc(sLineIndices, (size_t)capacity * 6u * sizeof *pattern);
        if (pattern == NULL) return -1;
        sLineIndices = pattern;
        for (uint32_t i = sLineCapacity; i < capacity; i++) {
            const uint32_t base = i * 4u;
            pattern[i * 6u] = base; pattern[i * 6u + 1u] = base + 2u;
            pattern[i * 6u + 2u] = base + 1u; pattern[i * 6u + 3u] = base + 1u;
            pattern[i * 6u + 4u] = base + 2u; pattern[i * 6u + 5u] = base + 3u;
        }
        sLineCapacity = capacity;
    }
    for (uint32_t i = 0; i < segments; i++) {
        const uint32_t first = mode == GL_LINES ? i * 2u : i;
        const P4ClipVertex *a = verts + (indices != NULL ? indices[first] : first);
        const P4ClipVertex *b = verts + (indices != NULL ? indices[first + 1u] : first + 1u);
        float enter = 0.0f, leave = 1.0f;
        P4ClipVertex endpoints[2];
        float dx, dy, length, ox, oy;
        for (unsigned plane = 0; plane < 6; plane++) {
            const unsigned axis = plane / 2u;
            const float sign = (plane & 1u) ? -1.0f : 1.0f;
            const float da = a->pos[3] + sign * a->pos[axis];
            const float db = b->pos[3] + sign * b->pos[axis];
            if (da < 0.0f && db < 0.0f) { enter = 1.0f; leave = 0.0f; break; }
            if ((da < 0.0f) != (db < 0.0f)) {
                const float t = da / (da - db);
                if (da < 0.0f) { if (t > enter) enter = t; }
                else if (t < leave) leave = t;
            }
        }
        if (enter > leave) continue;
        line_endpoint(&endpoints[0], a, b, enter);
        line_endpoint(&endpoints[1], a, b, leave);
        if (!(endpoints[0].pos[3] > 0.0f) || !(endpoints[1].pos[3] > 0.0f)) continue;
        dx = (endpoints[1].pos[0] / endpoints[1].pos[3] -
              endpoints[0].pos[0] / endpoints[0].pos[3]) * (float)width;
        dy = (endpoints[1].pos[1] / endpoints[1].pos[3] -
              endpoints[0].pos[1] / endpoints[0].pos[3]) * (float)height;
        length = sqrtf(dx * dx + dy * dy);
        if (!(length > 0.0f)) continue;
        ox = -dy / (length * (float)width);
        oy = dx / (length * (float)height);
        for (unsigned corner = 0; corner < 4; corner++) {
            P4ClipVertex *out = sLineVertices + visible * 4u + corner;
            const float sign = (corner & 1u) ? 1.0f : -1.0f;
            *out = endpoints[corner / 2u];
            out->pos[0] += sign * ox * out->pos[3];
            out->pos[1] += sign * oy * out->pos[3];
        }
        visible++;
    }
    if (visible != 0) {
        if (sLineEbo == 0) p4gl_glGenBuffers(1, &sLineEbo);
        p4gl_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sLineEbo);
        if (sLineGpuCapacity < sLineCapacity) {
            p4gl_glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)sLineCapacity * 6 *
                              (GLsizeiptr)sizeof *sLineIndices, sLineIndices, GL_STATIC_DRAW);
            sLineGpuCapacity = sLineCapacity;
        }
    }
    return (int)visible;
}

/* Draws the vertex buffer (indices NULL) or indices of the given GL type with the Im3D program.
 * `*uploaded` records whether `verts` is already in the vertex buffer. */
static int draw_common(GLenum mode, const P4ClipVertex *verts, uint32_t numVerts, int *uploaded,
                       const void *indices, GLenum indexType, uint32_t indexSize, int32_t count,
                       const P4ClipDrawParams *dp)
{
    int skipped = 0;
    int32_t failFunc = 0;
    float failRef = 0.0f;
    int scaledLine = 0;
    int virtualWidth = 0, virtualHeight = 0, pixelWidth = 0, pixelHeight = 0;
    const int colorFail = p4gl_gs_alpha_color_fail(dp->own ? dp->test : p4gl_sky_test_raw(),
                                                &failFunc, &failRef);

    if (!p4gl_gl_prepare_draw()) {
        return 1;
    }
    if (p4gl_profile_pass != NULL) p4gl_profile_pass(P4GL_PROFILE_SCENE);

    if (!ensure_ready()) {
        return 0;
    }
    if ((mode == GL_LINES || mode == GL_LINE_STRIP) &&
        p4gl_target_size(&virtualWidth, &virtualHeight) &&
        p4gl_target_pixel_size(&pixelWidth, &pixelHeight)) {
        scaledLine = virtualWidth != pixelWidth || virtualHeight != pixelHeight;
    }

    p4gl_glBindVertexArray(sVao);
    p4gl_glBindBuffer(GL_ARRAY_BUFFER, sVbo);
    if (!*uploaded && !scaledLine) {
        p4gl_glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)numVerts * (GLsizeiptr)sizeof(P4ClipVertex),
                          verts, GL_STREAM_DRAW);
        *uploaded = 1;
    }
    p4gl_glUseProgram(sProgram);

    {
        const int textured = dp->raster != 0 && p4gl_texture_bind(dp->raster);

        p4gl_glUniform1i(sLocTextured, textured);
        p4gl_glUniform1i(sLocBorder, textured ? p4gl_texture_border_mode() : 0);
    }
    p4gl_glUniform4f(sLocMatColor, dp->color[0], dp->color[1], dp->color[2], dp->color[3]);
    if (dp->own) {
        if (dp->blend) {
            p4gl_gs_alpha_apply(dp->alpha);
        } else {
            p4gl_glDisable(GL_BLEND);
        }
        p4gl_glUniform1i(sLocAlphaFunc, p4gl_gs_test_func(dp->test));
        p4gl_glUniform1f(sLocAlphaRef, p4gl_gs_test_ref(dp->test));
        p4gl_depth_apply_values(dp->zTest, dp->zWrite);
    } else {
        p4gl_sky_apply_blend();
        p4gl_glUniform1i(sLocAlphaFunc, p4gl_sky_alpha_func());
        p4gl_glUniform1f(sLocAlphaRef, p4gl_sky_alpha_ref());
        p4gl_depth_apply();
    }
    p4gl_glUniform1i(sLocAlphaInvert, 0);
    if (colorFail) {
        p4gl_glUniform1i(sLocAlphaFunc, failFunc);
        p4gl_glUniform1f(sLocAlphaRef, failRef);
    }
    p4gl_frame_mask_apply();
    if (dp->own) {
        p4gl_glDisable(GL_CULL_FACE);
    } else {
        p4gl_cull_apply();
    }

    skipped = !p4gl_drawlog("im3d", (int)mode, indices != NULL ? (int)count : (int)numVerts, NULL, NULL, NULL);
    if (p4gl_gs_log_on()) {
        im3d_gs_log(verts, numVerts, dp, mode, indices != NULL ? count : (int32_t)numVerts);
        skipped |= p4gl_gs_skip();
    }
    if (skipped) {
        p4gl_frame_mask_restore();
        p4gl_depth_restore();
        return 1;
    }
    if (scaledLine) {
        const int visible = expand_lines(mode, verts, numVerts, indices, (uint32_t)count,
                                         virtualWidth, virtualHeight);
        if (visible <= 0) {
            p4gl_frame_mask_restore();
            p4gl_depth_restore();
            return visible == 0;
        }
        /* Logging/skipping above describe the original GS line draw, not
         * its resolution-dependent triangle transport. Lines ignore culling. */
        p4gl_glDisable(GL_CULL_FACE);
        p4gl_glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)visible * 4 *
                          (GLsizeiptr)sizeof *sLineVertices, sLineVertices, GL_STREAM_DRAW);
        *uploaded = 0;
        mode = GL_TRIANGLES;
        indices = sLineIndices;
        indexType = GL_UNSIGNED_INT;
        count = visible * 6;
    }
    if (indices == NULL) {
        p4gl_glDrawArrays(mode, 0, (GLsizei)numVerts);
    } else {
        p4gl_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scaledLine ? sLineEbo : sEbo);
        if (!scaledLine)
            p4gl_glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)count * (GLsizeiptr)indexSize,
                              indices, GL_STREAM_DRAW);
        p4gl_glDrawElements(mode, count, indexType, NULL);
    }
    if (colorFail) {
        p4gl_glUniform1i(sLocAlphaInvert, 1);
        p4gl_glDepthMask(GL_FALSE);
        if (colorFail == 3) {
            p4gl_frame_mask_rgb_only();
        }
        if (indices == NULL) {
            p4gl_glDrawArrays(mode, 0, (GLsizei)numVerts);
        } else {
            p4gl_glDrawElements(mode, count, indexType, NULL);
        }
        p4gl_glUniform1i(sLocAlphaInvert, 0);
    }

    p4gl_frame_mask_restore();
    p4gl_depth_restore();
    return 1;
}

int p4gl_im3d_draw(int32_t primType, const uint16_t *indices, int32_t numIndices)
{
    const GLenum mode = prim_mode(primType);
    P4ClipDrawParams dp;
    int32_t i;

    if (mode == 0) {
        return 0;
    }
    if (indices != NULL) {
        if (numIndices <= 0) {
            return 1;
        }
        for (i = 0; i < numIndices; i++) {
            if ((uint32_t)indices[i] >= sCount) {
                fprintf(stderr, "p4gl_im3d: index %u out of range (%u vertices)\n",
                        (unsigned)indices[i], (unsigned)sCount);
                return 0;
            }
        }
    } else if (sCount == 0) {
        return 1;
    }
    memset(&dp, 0, sizeof dp);
    dp.raster = p4gl_render_state(P4GL_RS_TEXTURERASTER);
    dp.color[0] = dp.color[1] = dp.color[2] = dp.color[3] = 1.0f;
    return draw_common(mode, sClip, sCount, &sUploaded, indices, GL_UNSIGNED_SHORT,
                       (uint32_t)sizeof(uint16_t), numIndices, &dp);
}

int p4gl_clip_draw_triangles(const P4ClipVertex *verts, uint32_t numVerts, const uint32_t *indices,
                             uint32_t numIndices, const P4ClipDrawParams *params)
{
    int uploaded = 0;
    uint32_t i;

    if (numIndices == 0 || numVerts == 0) {
        return 1;
    }
    for (i = 0; i < numIndices; i++) {
        if (indices[i] >= numVerts) {
            fprintf(stderr, "p4gl_clip_draw: index %u out of range (%u vertices)\n",
                    (unsigned)indices[i], (unsigned)numVerts);
            return 0;
        }
    }
    sUploaded = 0;   /* the draw replaced the shared vertex buffer */
    return draw_common(GL_TRIANGLES, verts, numVerts, &uploaded, indices, GL_UNSIGNED_INT,
                       (uint32_t)sizeof(uint32_t), (int32_t)numIndices, params);
}

/* func_003cc370 (retail 0x003cc370), the RwIm3D platform pipeline setup that func_003cc170 calls.
 * Retail builds the fast-transform pipe (func_003cc2c0) and takes the Im3D render pipes from the
 * RpPDS registry (RpPDSGetPipe ids 4, 6, 5 and 7), stores them at +0x1c..+0x3c of the Im3D state
 * block (0x8872E0 + *(gp-18000)) and registers them (0x410a40, 0x410ab0). The GL Im3D entry points
 * (overrides/00410420.c and the other five) never execute those pipelines, so nothing reads them and
 * the setup is a success without building them. The decomp's copy of this function is in the
 * excluded code1_003c_cw119.c, so this is a port-owned definition. */
int32_t func_003cc370(void)
{
    return 1;
}

void p4gl_im3d_context_lost(void)
{
    sProgram = 0;
    sVao = sVbo = sEbo = 0;
    sLineEbo = 0;
    sLineGpuCapacity = 0;
    sUploaded = 0;
    sReady = sFailed = 0;
}
