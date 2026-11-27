/* The PS2All atomic node on OpenGL 3.3 (replaces the sky2 DMA path; docs/renderer_port.md §8).
 * p4gl_atomic_draw draws one RpAtomic from its RpGeometry: positions, normals, texture
 * coordinates and prelit colours come from morph target 0 and texCoords[0], one index buffer
 * per material, cached per geometry (a world sector, the BSP leaf the game renders through
 * RwCameraForAllSectorsInFrustum, is drawn the same way from its own vertex arrays and mesh
 * header: the dream battle's floor). The transform is the frame LTM times the current camera
 * view, then a perspective projection from the camera's view window and near/far planes.
 * Lighting uses the scene's RenderWare lights, prelit and material colours, and the texture.
 * GS Gouraud colour is screen-affine while texture STQ remains perspective-correct. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "p4gl.h"
#include "p4gl_gl.h"
#include "p4gl_passcull.h"
#include "p4gl_profile.h"
#include "p4pc/gmem.h"

/* RwObject.type of an atomic (rpATOMIC in decomp/include/rw/sky2/rpworld.h). */
#define P4_OBJECT_ATOMIC 1u

/* Retail layout offsets (decomp/include/rw/sky2/rpworld.h, rwcore.h, rwplcore.h). */
#define P4_ATOMIC_PARENT 4         /* RwObject.parent: the RwFrame */
#define P4_ATOMIC_GEOMETRY 24      /* RpAtomic.geometry */
#define P4_FRAME_LTM 80            /* RwFrame.ltm (RwMatrix) */
#define P4_GEOM_FLAGS 8           /* RpGeometry.flags: rpGEOMETRYLIGHT 0x20, rpGEOMETRYNORMALS 0x10 */
#define P4_GEOM_LOCKED 12          /* RpGeometry.lockedSinceLastInst (u16) */
#define P4_GEOM_NUMTRIANGLES 16    /* RpGeometry.numTriangles */
#define P4_GEOM_NUMVERTICES 20     /* RpGeometry.numVertices */
#define P4_GEOM_MATERIALS 32       /* RpGeometry.matList.materials (RpMaterial **) */
#define P4_GEOM_NUMMATERIALS 36    /* RpGeometry.matList.numMaterials */
#define P4_GEOM_TRIANGLES 44       /* RpGeometry.triangles (RpTriangle *) */
#define P4_GEOM_PRELIT 48          /* RpGeometry.preLitLum (RwRGBA *) */
#define P4_GEOM_TEXCOORDS 52       /* RpGeometry.texCoords[0] (RwTexCoords *) */
#define P4_GEOM_MORPHTARGET 92     /* RpGeometry.morphTarget[0] (RpMorphTarget *) */
#define P4_MORPH_VERTS 20          /* RpMorphTarget.verts (RwV3d *) */
#define P4_MORPH_NORMALS 24        /* RpMorphTarget.normals (RwV3d *) */
#define P4_MATERIAL_TEXTURE 0      /* RpMaterial.texture (RwTexture *) */
#define P4_MATERIAL_COLOR 4        /* RpMaterial.color (RwRGBA) */
#define P4_TEXTURE_RASTER 0        /* RwTexture.raster (RwRaster *) */
#define P4_TEXTURE_SAMPLER 80      /* RwTexture.filterAddressing */
#define P4_TRIANGLE_SIZE 8         /* RpTriangle: vertIndex[3] u16, matIndex i16 */
#define P4_TRIANGLE_MAT 6
#define P4_CAMERA_FAR 132          /* RwCamera.farPlane */
#define P4_CAMERA_FOGPLANE 136     /* RwCamera.fogPlane */

#define P4_ATOMIC_CACHE_SIZE 512
#define P4_ATOMIC_MAX_MATERIALS 4096

typedef struct P4AtomicVertex {
    float pos[3];
    float nrm[3];
    float uv[2];
    uint8_t rgba[4];
} P4AtomicVertex;
_Static_assert(sizeof(P4AtomicVertex) == 36, "P4AtomicVertex layout");

typedef struct P4AtomicCacheEntry {
    int valid;
    uint32_t locked;
    const void *verts;
    const void *normals;
    const void *prelit;
    const void *uv;
    const void *triangles;
    int32_t numVertices;
    int32_t numTriangles;
    int32_t numMaterials;
    GLuint vbo;
    GLuint ibo;
    GLuint vao; /* the entry's vertex array: its two buffers and the four attribute pointers, bound by one call (0: P4PC_ATOMIC_VAO=0) */
    uint32_t *ranges; /* per material bucket (numMaterials + 1): first index, index count */
    uint8_t *alphaRange; /* per material bucket: the smallest and largest vertex alpha of its triangles (alpha-test pass culling) */
    uint8_t skinAlphaLo, skinAlphaHi; /* the same over the whole mesh, of the last skinned upload */
    uint8_t modelEdgeCount, modelBasis[2]; /* 0: unprepared; 255: invalid mask */
    uint8_t modelEdges[8][3]; /* vertex pair, crop kind */
} P4AtomicCacheEntry;

/* The per-draw values of both shaders, packed in one vec4 array so that a draw costs one glUniform4fv instead of about a dozen uniform calls (the GL driver pays
 * per call, and on the render thread per recorded call). The macros keep the shaders' code as it was; every value is the float it was before (integers
 * and flags travel as small exact floats). Layout (sD below): 0-3 mvp columns, 4-6 normal matrix columns, 7-9 world rotation columns, 10 model uv xyz + lit mode,
 * 11 ambient + textured, 12 light 0 direction + border mode, 13 light 1 direction + lit, 14 light 0 colour + date, 15 light 1 colour + alpha function, 16 material colour,
 * 17 fog colour + fog on, 18 light direction + alpha reference, 19 fog start, fog end, alpha invert, 20-22 the material's UV transform columns (RpMatFX). */
#define P4_ATOMIC_PACKED_COUNT 23
#define P4_ATOMIC_PACKED_UNIFORMS \
    "uniform vec4 uD[23];\n" \
    "#define uMvp mat4(uD[0], uD[1], uD[2], uD[3])\n" \
    "#define uNrm mat3(uD[4].xyz, uD[5].xyz, uD[6].xyz)\n" \
    "#define uWorldRot mat3(uD[7].xyz, uD[8].xyz, uD[9].xyz)\n" \
    "#define uModelUv uD[10].xyz\n" \
    "#define uLitMode int(uD[10].w)\n" \
    "#define uAmb uD[11].xyz\n" \
    "#define uTextured int(uD[11].w)\n" \
    "#define uDirAt0 uD[12].xyz\n" \
    "#define uBorder int(uD[12].w)\n" \
    "#define uDirAt1 uD[13].xyz\n" \
    "#define uLit int(uD[13].w)\n" \
    "#define uDirCol0 uD[14].xyz\n" \
    "#define uDate int(uD[14].w)\n" \
    "#define uDirCol1 uD[15].xyz\n" \
    "#define uAlphaFunc int(uD[15].w)\n" \
    "#define uMatColor uD[16]\n" \
    "#define uFogColor uD[17].xyz\n" \
    "#define uFogOn int(uD[17].w)\n" \
    "#define uLightDir uD[18].xyz\n" \
    "#define uAlphaRef uD[18].w\n" \
    "#define uFogStart uD[19].x\n" \
    "#define uFogEnd uD[19].y\n" \
    "#define uAlphaInvert int(uD[19].z)\n" \
    "#define uUvM mat3(uD[20].xyz, uD[21].xyz, uD[22].xyz)\n"

static const char kAtomicVertexSource[] =
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 1) in vec3 aNrm;\n"
    "layout(location = 2) in vec2 aUv;\n"
    "layout(location = 3) in vec4 aCol;\n"
    P4_ATOMIC_PACKED_UNIFORMS
    "out vec3 vNrm;\n"
    "out vec2 vUv;\n"
    "out vec4 vCol;\n"
    "out float vFogZ;\n"
    "out float vFog;\n"
    "void main() {\n"
    "    gl_Position = uMvp * vec4(aPos, 1.0);\n"
    "    vFogZ = gl_Position.w;\n"
    "    /* The retail vertex fog byte is linear in depth and clamped per vertex; the GS interpolates it affinely in screen space (like the colour below). */\n"
    "    vFog = clamp((uFogEnd - gl_Position.w) / max(uFogEnd - uFogStart, 1e-3), 0.0, 1.0) * gl_Position.w;\n"
    "    vNrm = uNrm * aNrm;\n"
    "    vUv = (uUvM * vec3(aUv, 1.0)).xy;\n"
    "    if (uModelUv.x > 1.0) vUv = aUv * uModelUv.x + uModelUv.yz;\n"
    "    vCol = aCol;\n"
    "    if (uLitMode != 0) {\n"
    "        /* RenderWare lighting of a lit geometry: the prelit colour plus the ambient light and the directional\n"
    "         * lights (surfaces facing against the light's at vector), each scaled by the material's ambient or\n"
    "         * diffuse coefficient (uAmb and uDirCol arrive scaled). */\n"
    "        vec3 n = normalize(uWorldRot * aNrm);\n"
    "        vec3 l = uAmb + uDirCol0 * max(-dot(n, uDirAt0), 0.0) + uDirCol1 * max(-dot(n, uDirAt1), 0.0);\n"
    "        vCol.rgb = min(aCol.rgb + l, vec3(1.0));\n"
    "    }\n"
    "    /* GS Gouraud RGBA is affine in screen space; only STQ is perspective-correct.\n"
    "     * Multiplying by clip W then dividing by interpolated W works on GLES 3.0,\n"
    "     * which has no core noperspective qualifier. */\n"
    "    vCol *= gl_Position.w;\n"
    "}\n";

static const char kAtomicFragmentSource[] =
    "in vec3 vNrm;\n"
    "in vec2 vUv;\n"
    "in vec4 vCol;\n"
    "in float vFogZ;\n"
    "in float vFog;\n"
    P4_ATOMIC_PACKED_UNIFORMS
    "uniform sampler2D uTex;\n"
    "uniform sampler2D uDest;\n"
    "bool alphaPasses(float a) {\n"
    "    float t = p4GsAlpha(a);\n"
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
    "    if (uDate != 0) {\n"
    "        bool set = texelFetch(uDest, ivec2(gl_FragCoord.xy), 0).a >= (254.5 / 255.0);\n"
    "        if (set != (uDate == 2)) discard;\n"
    "    }\n"
    "    /* Expanded portrait masks may sample beyond their image, never tile it. */\n"
    "    if (uModelUv.x > 1.0 && (any(lessThan(vUv, vec2(0.0))) || any(greaterThan(vUv, vec2(1.0))))) discard;\n"
    "    vec4 c = (vCol / vFogZ) * uMatColor;\n"
    "    if (uTextured != 0) {\n"
    "        if (uModelUv.x > 1.0) {\n"
    "            vec2 halfTexel = 0.5 / vec2(textureSize(uTex, 0));\n"
    "            c *= texture(uTex, clamp(vUv, halfTexel, vec2(1.0) - halfTexel));\n"
    "        } else c *= (uBorder != 0) ? p4SampleBorder(uTex, vUv, uBorder) : texture(uTex, vUv);\n"
    "    }\n"
    "    if (uAlphaFunc != 0 && alphaPasses(c.a) == (uAlphaInvert != 0)) {\n"
    "        discard;\n"
    "    }\n"
    "    if (uLit != 0) {\n"
    "        float d = max(dot(normalize(vNrm), uLightDir), 0.0);\n"
    "        c.rgb *= 0.35 + 0.65 * d;\n"
    "    }\n"
    "    if (uFogOn != 0) {\n"
    "        c.rgb = mix(uFogColor, c.rgb,\n"
    "                    clamp(vFog / vFogZ, 0.0, 1.0));\n"
    "    }\n"
    "#ifdef P4_ROUND_BIAS\n"
    "    c.rgb = p4GsTruncate(c.rgb);\n"
    "#endif\n"
    "    fragColor = c;\n"
    "}\n";

/* Normalised direction towards the light, camera space. */
static const float kLightDir[3] = { -0.398f, 0.597f, -0.697f };

/* The viewer's stand-in headlight for meshes with normals, off by default: the PS2 pipeline lights from the game's
 * RpLights (none in the title: its silhouettes are vertex colour times material colour, flat), so a game-driven
 * scene must not get one. Only p4pc_viewer's single-RMD mode turns it on to make a bare model readable. */
static int sViewerLight = 0;

void p4gl_atomic_set_viewer_light(int on)
{
    sViewerLight = on != 0;
}

static P4AtomicCacheEntry sCache[P4_ATOMIC_CACHE_SIZE];
static uint32_t sCacheNext;
static int sReady;
static GLuint sVao;
static GLuint sProgram;
static GLint sLocD; /* uD[0]: the packed per-draw values (P4_ATOMIC_PACKED_UNIFORMS) */
static float sD[P4_ATOMIC_PACKED_COUNT][4]; /* what the draw wants */
static float sDSent[P4_ATOMIC_PACKED_COUNT][4]; /* what the program holds */
static int sDSentValid;
static int sModelUvActive;
static float sModelAnchor[4][2];
static unsigned sModelAnchorValid;

static GLint sLocTexture;
static GLint sLocDest;

/* Sends the packed per-draw values when they differ from what the program holds (one call; the draw's own order of setting them does not matter). */
static int d_differs(void)
{
    for (size_t i = 0; i < sizeof sD / sizeof(uint64_t); i++) {
        uint64_t a, b;

        memcpy(&a, (const uint8_t *)sD + i * sizeof a, sizeof a);
        memcpy(&b, (const uint8_t *)sDSent + i * sizeof b, sizeof b);
        if (a != b) {
            return 1; /* bit patterns, so -0 and +0 differ as they would to the shader */
        }
    }
    return 0;
}

static void d_flush(void)
{
    if (!sDSentValid || d_differs()) {
        p4gl_glUniform4fv(sLocD, P4_ATOMIC_PACKED_COUNT, &sD[0][0]);
        memcpy(sDSent, sD, sizeof sD);
        sDSentValid = 1;
    }
}

void p4gl_atomic_model_scope_begin(void)
{
    sModelAnchorValid = 0;
}
/* Retail structs are read at byte offsets through memcpy: the offsets are not all aligned to the host type,
 * and a pointer field is a 4-byte guest address that P4_G2H turns into a host pointer. */
static const void *ptr_at(const void *base, size_t off)
{
    uint32_t value;

    memcpy(&value, (const uint8_t *)base + off, sizeof value);
    return P4_G2H(value);
}

static int32_t i32_at(const void *base, size_t off)
{
    int32_t value;

    memcpy(&value, (const uint8_t *)base + off, sizeof value);
    return value;
}

static uint16_t u16_at(const void *base, size_t off)
{
    uint16_t value;

    memcpy(&value, (const uint8_t *)base + off, sizeof value);
    return value;
}

static float f32_at(const void *base, size_t off)
{
    float value;

    memcpy(&value, (const uint8_t *)base + off, sizeof value);
    return value;
}

static GLuint compile_stage(GLenum type, const char *source)
{
    return p4gl_gl_compile_shader(type, source, "atomic");
}

static int ensure_ready(void)
{
    GLuint vs;
    GLuint fs;
    GLint ok = 0;

    if (sReady) {
        return 1;
    }
    vs = compile_stage(GL_VERTEX_SHADER, kAtomicVertexSource);
    fs = compile_stage(GL_FRAGMENT_SHADER, kAtomicFragmentSource);
    if (vs == 0 || fs == 0) {
        return 0;
    }
    sProgram = p4gl_glCreateProgram();
    p4gl_glAttachShader(sProgram, vs);
    p4gl_glAttachShader(sProgram, fs);
    p4gl_glLinkProgram(sProgram);
    p4gl_glGetProgramiv(sProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        fprintf(stderr, "p4gl_atomic: program link failed\n");
        return 0;
    }

    sLocD = p4gl_glGetUniformLocation(sProgram, "uD[0]");
    sLocTexture = p4gl_glGetUniformLocation(sProgram, "uTex");
    sLocDest = p4gl_glGetUniformLocation(sProgram, "uDest");
    sDSentValid = 0; /* a new program holds zeros */

    p4gl_glGenVertexArrays(1, &sVao);
    p4gl_glBindVertexArray(sVao);
    p4gl_glEnableVertexAttribArray(0);
    p4gl_glEnableVertexAttribArray(1);
    p4gl_glEnableVertexAttribArray(2);
    p4gl_glEnableVertexAttribArray(3);

    p4gl_glUseProgram(sProgram);
    p4gl_glUniform1i(sLocTexture, 0);
    p4gl_glUniform1i(sLocDest, 1);
    sD[20][0] = 1.0f; sD[20][1] = 0.0f; sD[20][2] = 0.0f; sD[20][3] = 0.0f;
    sD[21][0] = 0.0f; sD[21][1] = 1.0f; sD[21][2] = 0.0f; sD[21][3] = 0.0f;
    sD[22][0] = 0.0f; sD[22][1] = 0.0f; sD[22][2] = 1.0f; sD[22][3] = 0.0f;
    sReady = 1;
    return 1;
}

/* Vertex arrays of freed entries wait here for the next entry to build into (glDeleteVertexArrays is not in the GL table); every use rebinds both
 * buffers and the four pointers, so nothing of the previous entry survives. */
static GLuint sVaoPool[P4_ATOMIC_CACHE_SIZE];
static unsigned sVaoPoolCount;
static int sVaoOn = -1;

static void entry_attribs(void)
{
    p4gl_glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(P4AtomicVertex),
                               (const void *)offsetof(P4AtomicVertex, pos));
    p4gl_glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(P4AtomicVertex),
                               (const void *)offsetof(P4AtomicVertex, nrm));
    p4gl_glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(P4AtomicVertex),
                               (const void *)offsetof(P4AtomicVertex, uv));
    p4gl_glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(P4AtomicVertex),
                               (const void *)offsetof(P4AtomicVertex, rgba));
}

static void cache_free_entry(P4AtomicCacheEntry *e)
{
    if (e->valid) {
        GLuint buffers[2];

        if (e->vao != 0 && sVaoPoolCount < P4_ATOMIC_CACHE_SIZE) {
            sVaoPool[sVaoPoolCount++] = e->vao;
        }
        buffers[0] = e->vbo;
        buffers[1] = e->ibo;
        p4gl_glDeleteBuffers(2, buffers);
        free(e->ranges);
        free(e->alphaRange);
    }
    memset(e, 0, sizeof *e);
}

void p4gl_atomic_context_lost(void)
{
    for (int i = 0; i < P4_ATOMIC_CACHE_SIZE; i++) {
        if (sCache[i].valid) {
            free(sCache[i].alphaRange);
            free(sCache[i].ranges); /* the buffers went with the context: no glDeleteBuffers, the names are other objects' now */
        }
        memset(&sCache[i], 0, sizeof sCache[i]);
    }
    sCacheNext = 0;
    sVaoPoolCount = 0;
    sVao = 0;
    sProgram = 0;
    sModelUvActive = 0;
    sModelAnchorValid = 0;
    sDSentValid = 0;
    sReady = 0;
}

/* Build the vertex and per-material index buffers for one geometry. Triangles whose vertex
 * indices fall outside numVertices are dropped. A triangle whose matIndex is outside the
 * material list goes into the trailing bucket, drawn untextured and white. */
/* What the cache and the draw read of an RpGeometry (with its morph target 0) or of an RpWorldSector. A world sector holds the same
 * arrays (vertices, normals, texCoords[0], preLitLum) but no triangle list worth bucketing by material: its RpMeshHeader groups the
 * strips (rpMESHHEADERTRISTRIP) or lists per material, and the RpMesh entries carry the RpMaterial. */
typedef struct {
    int32_t numVertices;
    int32_t numTriangles;
    int32_t numMaterials;  /* RpGeometry.matList.numMaterials, or the number of meshes of a sector */
    const void *triangles; /* RpGeometry.triangles; for a sector the mesh header, the cache key */
    const void *mesh;      /* the sector's RpMeshHeader, NULL for a geometry */
    const void *prelit;
    const void *uv;
    const void *verts;
    const void *normals;
    int32_t flags;
    uint32_t locked;
    const void *matBase; /* material k is the guest pointer at matBase + matStride * k + matOff */
    size_t matStride;
    size_t matOff;
} P4GeomView;

#define P4_SECTOR_TYPE_ATOMIC 0xFF /* RpWorldSector.type = -1 (a leaf), the first byte of the object */
#define P4_SECTOR_VERTICES 8
#define P4_SECTOR_NORMALS 12
#define P4_SECTOR_TEXCOORDS 16
#define P4_SECTOR_PRELIT 48
#define P4_SECTOR_MESH 120
#define P4_SECTOR_NUMVERTICES 130
#define P4_SECTOR_NUMTRIANGLES 132
#define P4_MESHHEADER_FLAGS 0
#define P4_MESHHEADER_NUMMESHES 4
#define P4_MESHHEADER_FIRSTMESH 12
#define P4_MESHHEADER_SIZE 16
#define P4_MESH_INDICES 0
#define P4_MESH_NUMINDICES 4
#define P4_MESH_MATERIAL 8
#define P4_MESH_SIZE 12
#define P4_MESHHEADER_TRISTRIP 1
#define P4_MESHHEADER_TRIFAN 2

static void view_of_geometry(P4GeomView *v, const void *geometry, const void *morph)
{
    memset(v, 0, sizeof *v);
    v->numVertices = i32_at(geometry, P4_GEOM_NUMVERTICES);
    v->numTriangles = i32_at(geometry, P4_GEOM_NUMTRIANGLES);
    v->numMaterials = i32_at(geometry, P4_GEOM_NUMMATERIALS);
    v->triangles = ptr_at(geometry, P4_GEOM_TRIANGLES);
    v->prelit = ptr_at(geometry, P4_GEOM_PRELIT);
    v->uv = ptr_at(geometry, P4_GEOM_TEXCOORDS);
    v->verts = ptr_at(morph, P4_MORPH_VERTS);
    v->normals = ptr_at(morph, P4_MORPH_NORMALS);
    v->flags = i32_at(geometry, P4_GEOM_FLAGS);
    v->locked = u16_at(geometry, P4_GEOM_LOCKED);
    v->matBase = ptr_at(geometry, P4_GEOM_MATERIALS);
    v->matStride = sizeof(uint32_t);
}

/* Returns 0 when the sector has no vertices or no mesh header to draw from. */
static int view_of_sector(P4GeomView *v, const void *sector)
{
    const uint8_t *mesh = ptr_at(sector, P4_SECTOR_MESH);

    memset(v, 0, sizeof *v);
    v->numVertices = u16_at(sector, P4_SECTOR_NUMVERTICES);
    v->numTriangles = u16_at(sector, P4_SECTOR_NUMTRIANGLES);
    v->verts = ptr_at(sector, P4_SECTOR_VERTICES);
    v->normals = ptr_at(sector, P4_SECTOR_NORMALS);
    v->uv = ptr_at(sector, P4_SECTOR_TEXCOORDS);
    v->prelit = ptr_at(sector, P4_SECTOR_PRELIT);
    if (mesh == NULL || v->numVertices <= 0 || v->verts == NULL) {
        return 0;
    }
    v->mesh = mesh;
    v->triangles = mesh;
    v->numMaterials = u16_at(mesh, P4_MESHHEADER_NUMMESHES);
    v->matBase = mesh + P4_MESHHEADER_SIZE + i32_at(mesh, P4_MESHHEADER_FIRSTMESH);
    v->matStride = P4_MESH_SIZE;
    v->matOff = P4_MESH_MATERIAL;
    return v->numMaterials > 0;
}

static const void *view_material(const P4GeomView *v, uint32_t b)
{
    if (v->matBase == NULL) {
        return NULL;
    }
    return ptr_at(v->matBase, v->matStride * b + v->matOff);
}

/* A prelit colour is a GS vertex colour: a textured GS draw computes texel * colour / 128, so the byte 0x80 is 1.0 and 0xFF is
 * 2.0 (the GS dump of the Dojima room: its vertex colours peak at 0x80 after the game's lighting). The shader's vertex colours
 * are c / 255, so the colour channels are scaled by 255 / 128 and clamped; alpha is kept. Geometry without prelit colours keeps
 * the white default (the protagonist is unlit and drew washed out when every textured colour was scaled). The unscaled field
 * came out 1.3 to 1.9 times darker than the PCSX2 frame, scaled 1.0 to 1.2. */
static void prelit_colour(uint8_t out[4], const uint8_t in[4])
{
    int i;

    for (i = 0; i < 3; i++) {
        unsigned v = ((unsigned)in[i] * 255u + 64u) / 128u;

        out[i] = (uint8_t)(v > 255u ? 255u : v);
    }
    out[3] = in[3];
}

/* A geometry that RenderWare lights (rpGEOMETRYLIGHT with normals): its vertex colours are the prelit colours (black without)
 * and the shader adds the lights; its prelit colours are not scaled (the lighting brings the 255/128 of the GS). */
static int geometry_lit(const P4GeomView *v)
{
    return (v->flags & 0x20) != 0 && (v->flags & 0x10) != 0 && v->normals != NULL;
}

static void vertex_colour(uint8_t out[4], const uint8_t *prelit, int32_t i, int lit)
{
    if (prelit != NULL) {
        if (lit) {
            memcpy(out, prelit + (size_t)i * 4u, 4);
        } else {
            prelit_colour(out, prelit + (size_t)i * 4u);
        }
    } else if (lit) {
        out[0] = out[1] = out[2] = 0;
        out[3] = 0xFF;
    } else {
        memset(out, 0xFF, 4);
    }
}

typedef struct {
    uint32_t idx[3];
    uint32_t bucket;
} P4Tri;

/* The triangles of a geometry, each with its material bucket (a matIndex outside the list goes to the trailing bucket, drawn
 * untextured and white). Triangles whose vertex indices fall outside numVertices are dropped. Returns the count, or -1. */
static int32_t gather_triangles_geometry(const P4GeomView *g, P4Tri **out)
{
    int32_t n = 0;
    int32_t t;
    P4Tri *tris = malloc((size_t)g->numTriangles * sizeof *tris);

    if (tris == NULL) {
        return -1;
    }
    for (t = 0; t < g->numTriangles; t++) {
        const uint8_t *tri = (const uint8_t *)g->triangles + (size_t)t * P4_TRIANGLE_SIZE;
        uint16_t a;
        uint16_t bv;
        uint16_t c;
        int16_t mat;

        memcpy(&a, tri, 2);
        memcpy(&bv, tri + 2, 2);
        memcpy(&c, tri + 4, 2);
        memcpy(&mat, tri + P4_TRIANGLE_MAT, 2);
        if (a >= g->numVertices || bv >= g->numVertices || c >= g->numVertices) {
            continue;
        }
        tris[n].idx[0] = a;
        tris[n].idx[1] = bv;
        tris[n].idx[2] = c;
        tris[n].bucket = (mat >= 0 && mat < g->numMaterials) ? (uint32_t)mat : (uint32_t)g->numMaterials;
        n++;
    }
    *out = tris;
    return n;
}

/* The triangles of a sector's meshes: lists, strips (a strip's odd triangles flip their winding; the strippers join strips with
 * repeated indices, so a triangle with two equal indices is dropped) or fans. The bucket is the mesh number. */
static int32_t gather_triangles_sector(const P4GeomView *g, P4Tri **out)
{
    const uint32_t flags = (uint32_t)i32_at(g->mesh, P4_MESHHEADER_FLAGS);
    const uint8_t *meshes = g->matBase;
    size_t cap = 0;
    int32_t n = 0;
    int32_t k;
    P4Tri *tris;

    for (k = 0; k < g->numMaterials; k++) {
        const uint8_t *m = meshes + (size_t)k * P4_MESH_SIZE;
        int32_t ni = i32_at(m, P4_MESH_NUMINDICES);

        if (ni > 2) {
            cap += (size_t)ni;
        }
    }
    tris = malloc((cap + 1u) * sizeof *tris);
    if (tris == NULL) {
        return -1;
    }
    for (k = 0; k < g->numMaterials; k++) {
        const uint8_t *m = meshes + (size_t)k * P4_MESH_SIZE;
        const uint8_t *ix = ptr_at(m, P4_MESH_INDICES);
        int32_t ni = i32_at(m, P4_MESH_NUMINDICES);
        int32_t i;

        if (ix == NULL || ni < 3) {
            continue;
        }
        for (i = 0; i + 2 < ni; i += (flags & (P4_MESHHEADER_TRISTRIP | P4_MESHHEADER_TRIFAN)) ? 1 : 3) {
            uint16_t w[3];
            uint32_t a;
            uint32_t b;
            uint32_t c;

            if (flags & P4_MESHHEADER_TRIFAN) {
                memcpy(&w[0], ix, 2);
                memcpy(&w[1], ix + 2u * (size_t)(i + 1), 2);
                memcpy(&w[2], ix + 2u * (size_t)(i + 2), 2);
            } else {
                memcpy(w, ix + 2u * (size_t)i, 6);
            }
            a = w[0];
            b = w[1];
            c = w[2];
            if ((flags & P4_MESHHEADER_TRISTRIP) && (i & 1)) {
                uint32_t tmp = b;

                b = c;
                c = tmp;
            }
            if (a == b || b == c || a == c || a >= (uint32_t)g->numVertices || b >= (uint32_t)g->numVertices || c >= (uint32_t)g->numVertices) {
                continue;
            }
            tris[n].idx[0] = a;
            tris[n].idx[1] = b;
            tris[n].idx[2] = c;
            tris[n].bucket = (uint32_t)k;
            n++;
        }
    }
    *out = tris;
    return n;
}

static int build_entry(P4AtomicCacheEntry *e, const P4GeomView *g)
{
    const int32_t numVertices = g->numVertices;
    const int32_t numMaterials = g->numMaterials;
    P4AtomicVertex *vertices = NULL;
    uint32_t *indices = NULL;
    uint32_t *cursor = NULL;
    uint32_t *ranges = NULL;
    uint8_t *alphaRange = NULL;
    P4Tri *tris = NULL;
    int32_t numTris = 0;
    uint32_t bucketCount;
    uint32_t total = 0;
    int32_t t;
    int32_t i;
    uint32_t b;
    GLuint buffers[2];
    GLuint vao = 0;

    if (numVertices <= 0 || numMaterials < 0 || numMaterials > P4_ATOMIC_MAX_MATERIALS || g->verts == NULL ||
        (g->mesh == NULL && (g->numTriangles <= 0 || g->triangles == NULL))) {
        return 0;
    }
    numTris = g->mesh != NULL ? gather_triangles_sector(g, &tris) : gather_triangles_geometry(g, &tris);
    if (numTris < 0) {
        return 0;
    }

    bucketCount = (uint32_t)numMaterials + 1u;
    ranges = calloc(bucketCount * 2u, sizeof *ranges);
    alphaRange = malloc((size_t)bucketCount * 2u);
    cursor = calloc(bucketCount, sizeof *cursor);
    vertices = malloc((size_t)numVertices * sizeof *vertices);
    if (!ranges || !cursor || !vertices || !alphaRange) {
        goto fail;
    }
    for (b = 0; b < bucketCount; b++) {
        alphaRange[2u * b] = 255;
        alphaRange[2u * b + 1u] = 0;
    }

    for (i = 0; i < numVertices; i++) {
        P4AtomicVertex *v = &vertices[i];

        memcpy(v->pos, (const uint8_t *)g->verts + (size_t)i * 12u, 12);
        if (g->normals) {
            memcpy(v->nrm, (const uint8_t *)g->normals + (size_t)i * 12u, 12);
        } else {
            memset(v->nrm, 0, 12);
        }
        if (g->uv) {
            memcpy(v->uv, (const uint8_t *)g->uv + (size_t)i * 8u, 8);
        } else {
            memset(v->uv, 0, 8);
        }
        vertex_colour(v->rgba, g->prelit, i, geometry_lit(g));
    }

    for (t = 0; t < numTris; t++) {
        uint8_t *ar = alphaRange + 2u * tris[t].bucket;
        int k;

        ranges[2u * tris[t].bucket + 1u] += 3;
        total += 3;
        for (k = 0; k < 3; k++) {
            const uint8_t a = vertices[tris[t].idx[k]].rgba[3];

            ar[0] = a < ar[0] ? a : ar[0];
            ar[1] = a > ar[1] ? a : ar[1];
        }
    }

    indices = malloc((size_t)total * sizeof *indices + sizeof *indices);
    if (!indices) {
        goto fail;
    }

    /* Prefix sums give each bucket's first index; cursor walks each bucket while filling. */
    {
        uint32_t start = 0;

        for (b = 0; b < bucketCount; b++) {
            ranges[2u * b] = start;
            cursor[b] = start;
            start += ranges[2u * b + 1u];
        }
    }

    for (t = 0; t < numTris; t++) {
        uint32_t slot = cursor[tris[t].bucket];

        indices[slot] = tris[t].idx[0];
        indices[slot + 1u] = tris[t].idx[1];
        indices[slot + 2u] = tris[t].idx[2];
        cursor[tris[t].bucket] = slot + 3u;
    }

    if (sVaoOn < 0) {
        sVaoOn = p4gl_gl_feature("P4PC_ATOMIC_VAO");
    }
    if (sVaoOn) {
        if (sVaoPoolCount != 0) {
            vao = sVaoPool[--sVaoPoolCount];
        } else {
            p4gl_glGenVertexArrays(1, &vao);
        }
        p4gl_glBindVertexArray(vao); /* the element buffer below becomes this array's */
    }
    p4gl_glGenBuffers(2, buffers);
    p4gl_glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    p4gl_glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)numVertices * (GLsizeiptr)sizeof *vertices,
                      vertices, GL_STATIC_DRAW);
    p4gl_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[1]);
    p4gl_glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)total * (GLsizeiptr)sizeof *indices,
                      indices, GL_STATIC_DRAW);

    if (vao != 0) {
        p4gl_glEnableVertexAttribArray(0);
        p4gl_glEnableVertexAttribArray(1);
        p4gl_glEnableVertexAttribArray(2);
        p4gl_glEnableVertexAttribArray(3);
        entry_attribs(); /* the array buffer bound last is this entry's vertex buffer */
    }
    e->vao = vao;
    e->vbo = buffers[0];
    e->ibo = buffers[1];
    e->ranges = ranges;
    e->alphaRange = alphaRange;
    e->numVertices = numVertices;
    e->numTriangles = g->numTriangles;
    e->numMaterials = numMaterials;
    e->verts = g->verts;
    e->normals = g->normals;
    e->prelit = g->prelit;
    e->uv = g->uv;
    e->triangles = g->triangles;
    e->locked = g->locked;
    e->modelEdgeCount = 0;
    e->valid = 1;

    free(tris);
    free(indices);
    free(cursor);
    free(vertices);
    return 1;

fail:
    free(tris);
    free(indices);
    free(ranges);
    free(alphaRange);
    free(cursor);
    free(vertices);
    return 0;
}

/* Find the cache entry for this geometry or sector, or build one into the next round-robin slot. The
 * key is the array pointers and counts plus lockedSinceLastInst. */
static P4AtomicCacheEntry *cache_lookup_or_build(const P4GeomView *g)
{
    P4AtomicCacheEntry *e;
    uint32_t slot;
    int i;

    for (i = 0; i < P4_ATOMIC_CACHE_SIZE; i++) {
        e = &sCache[i];
        if (e->valid && e->verts == g->verts && e->normals == g->normals && e->prelit == g->prelit &&
            e->uv == g->uv && e->triangles == g->triangles && e->numVertices == g->numVertices &&
            e->numTriangles == g->numTriangles && e->numMaterials == g->numMaterials &&
            e->locked == g->locked) {
            return e;
        }
    }

    slot = sCacheNext;
    sCacheNext = (sCacheNext + 1u) % P4_ATOMIC_CACHE_SIZE;
    e = &sCache[slot];
    cache_free_entry(e);
    if (!build_entry(e, g)) {
        cache_free_entry(e);
        return NULL;
    }
    return e;
}

/* CPU skinning (the skin plugin 0x116 and the HAnim hierarchy). The skin of a geometry is the pointer at
 * geometry + the plugin offset the retail registration stored at 0x00886498; the hierarchy of an atomic is
 * the pointer at atomic + the offset at 0x00886494. RpSkin: +0 numBones, +0x0C the skin-to-bone matrices
 * (RwMatrix, 64 bytes each), +0x14 the vertex bone index words (four bytes), +0x18 the vertex weights (four
 * floats; the sky preparation, func_003b85e0, put a matrix-cache offset in the low ten bits of each, which is an
 * error of about 1e-4). RpHAnimHierarchy: +4 numNodes, +8 the node matrices (world space: the game sets the
 * update-LTMs flag and not the local-space one). A bone index is a node index. A vertex is the weighted sum of
 * pos * skinToBone[i] * node[i] (row vectors), so the skinned geometry is in world space and is drawn with an
 * identity object matrix. */
#define P4_SKIN_PLUGIN_GEOM_ADDR 0x00886498u
#define P4_SKIN_PLUGIN_ATOMIC_ADDR 0x00886494u
#define P4_SKIN_NUMBONES 0
#define P4_SKIN_MATRICES 0x0C
#define P4_SKIN_INDICES 0x14
#define P4_SKIN_WEIGHTS 0x18
#define P4_HANIM_NUMNODES 4
#define P4_HANIM_MATRICES 8
#define P4_MAX_BONES 256

static const void *skin_of(const void *geometry)
{
    uint32_t off = *(const uint32_t *)P4_G2H(P4_SKIN_PLUGIN_GEOM_ADDR);

    return off != 0 ? ptr_at(geometry, off) : NULL;
}

static const void *hierarchy_of(const void *atomic)
{
    uint32_t off = *(const uint32_t *)P4_G2H(P4_SKIN_PLUGIN_ATOMIC_ADDR);

    return off != 0 ? ptr_at(atomic, off) : NULL;
}

/* out = a * b for RwMatrix (row vectors: right, up, at, pos rows of 16 bytes). */
static void mat_mul(const float *a, const float *b, float *out)
{
    int r;
    int c;

    for (r = 0; r < 4; r++) {
        for (c = 0; c < 3; c++) {
            out[r * 4 + c] = a[r * 4 + 0] * b[0 * 4 + c] + a[r * 4 + 1] * b[1 * 4 + c] + a[r * 4 + 2] * b[2 * 4 + c] +
                             (r == 3 ? b[3 * 4 + c] : 0.0f);
        }
        out[r * 4 + 3] = 0.0f;
    }
}

/* Rebuilds the entry's vertex buffer from the skinned geometry. Returns 1 when the skin applied. */
static int skin_upload(P4AtomicCacheEntry *e, const void *geometry, const void *morph, const void *skin,
                       const void *hier)
{
    static P4AtomicVertex *scratch;
    static int32_t scratchCap;
    const int32_t numVertices = i32_at(geometry, P4_GEOM_NUMVERTICES);
    const int32_t numBones = i32_at(skin, P4_SKIN_NUMBONES);
    const int32_t numNodes = i32_at(hier, P4_HANIM_NUMNODES);
    const float *skinToBone = ptr_at(skin, P4_SKIN_MATRICES);
    const uint32_t *indices = ptr_at(skin, P4_SKIN_INDICES);
    const float *weights = ptr_at(skin, P4_SKIN_WEIGHTS);
    const float *nodes = ptr_at(hier, P4_HANIM_MATRICES);
    const uint8_t *verts = ptr_at(morph, P4_MORPH_VERTS);
    const uint8_t *normals = ptr_at(morph, P4_MORPH_NORMALS);
    const uint8_t *uv = ptr_at(geometry, P4_GEOM_TEXCOORDS);
    const uint8_t *prelit = ptr_at(geometry, P4_GEOM_PRELIT);
    static float bones[P4_MAX_BONES][16];
    int32_t i;
    int32_t k;
    int32_t usable;
    uint8_t aLo = 255, aHi = 0;
    P4GeomView gv;

    view_of_geometry(&gv, geometry, morph);
    if (numBones <= 0 || numNodes <= 0 || skinToBone == NULL || indices == NULL || weights == NULL || nodes == NULL ||
        verts == NULL || numVertices <= 0) {
        return 0;
    }
    usable = numBones < numNodes ? numBones : numNodes;
    if (usable > P4_MAX_BONES) {
        usable = P4_MAX_BONES;
    }
    for (i = 0; i < usable; i++) {
        mat_mul(skinToBone + 16 * i, nodes + 16 * i, bones[i]);
    }
    if (scratchCap < numVertices) {
        free(scratch);
        scratch = malloc((size_t)numVertices * sizeof *scratch);
        scratchCap = scratch != NULL ? numVertices : 0;
        if (scratch == NULL) {
            return 0;
        }
    }
    for (i = 0; i < numVertices; i++) {
        P4AtomicVertex *v = &scratch[i];
        float p[3];
        float n[3] = {0.0f, 0.0f, 0.0f};
        float acc[3] = {0.0f, 0.0f, 0.0f};
        float wsum = 0.0f;
        float w[4];

        memcpy(p, verts + (size_t)i * 12u, 12);
        if (normals != NULL) {
            memcpy(n, normals + (size_t)i * 12u, 12);
        }
        memcpy(w, weights + 4 * i, sizeof w);
        v->nrm[0] = v->nrm[1] = v->nrm[2] = 0.0f;
        for (k = 0; k < 4; k++) {
            const uint32_t bi = (indices[i] >> (8 * k)) & 0xFFu;
            const float *m;

            if (w[k] == 0.0f || (int32_t)bi >= usable) {
                continue;
            }
            m = bones[bi];
            acc[0] += w[k] * (p[0] * m[0] + p[1] * m[4] + p[2] * m[8] + m[12]);
            acc[1] += w[k] * (p[0] * m[1] + p[1] * m[5] + p[2] * m[9] + m[13]);
            acc[2] += w[k] * (p[0] * m[2] + p[1] * m[6] + p[2] * m[10] + m[14]);
            v->nrm[0] += w[k] * (n[0] * m[0] + n[1] * m[4] + n[2] * m[8]);
            v->nrm[1] += w[k] * (n[0] * m[1] + n[1] * m[5] + n[2] * m[9]);
            v->nrm[2] += w[k] * (n[0] * m[2] + n[1] * m[6] + n[2] * m[10]);
            wsum += w[k];
        }
        if (wsum > 0.0f) {
            v->pos[0] = acc[0] / wsum;
            v->pos[1] = acc[1] / wsum;
            v->pos[2] = acc[2] / wsum;
            v->nrm[0] /= wsum;
            v->nrm[1] /= wsum;
            v->nrm[2] /= wsum;
        } else {
            memcpy(v->pos, p, 12);
            memcpy(v->nrm, n, 12);
        }
        if (uv != NULL) {
            memcpy(v->uv, uv + (size_t)i * 8u, 8);
        } else {
            v->uv[0] = v->uv[1] = 0.0f;
        }
        vertex_colour(v->rgba, prelit, i, geometry_lit(&gv));
        aLo = v->rgba[3] < aLo ? v->rgba[3] : aLo;
        aHi = v->rgba[3] > aHi ? v->rgba[3] : aHi;
    }
    e->skinAlphaLo = aLo;
    e->skinAlphaHi = aHi;
    p4gl_glBindBuffer(GL_ARRAY_BUFFER, e->vbo);
    p4gl_glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)numVertices * (GLsizeiptr)sizeof *scratch, scratch, GL_DYNAMIC_DRAW);
    return 1;
}

/* Upload MVP and the normal matrix (upper-left 3x3 of the camera-space object matrix, transposed
 * from the row-vector rotation). A NULL frame is the identity LTM. Returns 0 for a degenerate
 * camera (src/render/p4gl_camera.c). */
static int upload_transforms(const void *atomicFrame, const void *camera, float mvpCol[16])
{
    const void *ltm = atomicFrame ? (const uint8_t *)atomicFrame + P4_FRAME_LTM : NULL;
    float m[4][4];
    float nrmCol[9];
    int r;
    int c;

    if (!p4gl_camera_object_matrix(ltm, camera, m, mvpCol)) {
        return 0;
    }
    for (c = 0; c < 3; c++) {
        for (r = 0; r < 3; r++) {
            nrmCol[c * 3 + r] = m[r][c];
        }
    }

    memcpy(sD[0], mvpCol, 16 * sizeof(float));
    for (c = 0; c < 3; c++) {
        memcpy(sD[4 + c], nrmCol + 3 * c, 3 * sizeof(float));
        sD[4 + c][3] = 0.0f;
    }
    {
        /* The object-to-world rotation (the LTM's right, up and at rows as the columns): world normals for the lights. */
        float rot[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };

        if (ltm != NULL) {
            int row;

            for (row = 0; row < 3; row++) {
                memcpy(rot + 3 * row, (const uint8_t *)ltm + 16 * row, 12);
            }
        }
        for (c = 0; c < 3; c++) {
            memcpy(sD[7 + c], rot + 3 * c, 3 * sizeof(float));
            sD[7 + c][3] = 0.0f;
        }
    }
    return 1;
}

enum { MODEL_DIAGONAL, MODEL_VERTICAL_CROP, MODEL_HORIZONTAL_CROP };

/* The polygon topology and UV basis belong to the cached geometry, not to an
 * animation frame. Prepare once, without heap allocation or guest mutation. */
static int prepare_model_mask(P4AtomicCacheEntry *e)
{
    float p[8][2], tex[8][2], best = 0.0f;
    const int n = e->numVertices;
    unsigned count = 0;
    e->modelEdgeCount = 255;
    if (n < 3 || n > 8 || e->uv == NULL) return 0;
    for (int i = 0; i < n; i++) {
        memcpy(p[i], (const uint8_t *)e->verts + 12u * i, sizeof p[i]);
        memcpy(tex[i], (const uint8_t *)e->uv + 8u * i, sizeof tex[i]);
    }
    for (int a = 1; a < n; a++) for (int b = a + 1; b < n; b++) {
        const float d = (p[a][0] - p[0][0]) * (p[b][1] - p[0][1]) -
                        (p[b][0] - p[0][0]) * (p[a][1] - p[0][1]);
        if (fabsf(d) > fabsf(best)) {
            best = d;
            e->modelBasis[0] = (uint8_t)a;
            e->modelBasis[1] = (uint8_t)b;
        }
    }
    if (fabsf(best) < 0.000001f) return 0;
    for (int a = 0; a < n; a++) for (int b = a + 1; b < n; b++) {
        const float dx = p[b][0] - p[a][0], dy = p[b][1] - p[a][1];
        int side = 0, boundary = 1;
        for (int c = 0; c < n; c++) {
            const float cross = dx * (p[c][1] - p[a][1]) - dy * (p[c][0] - p[a][0]);
            const int current = cross > 0.000001f ? 1 : cross < -0.000001f ? -1 : 0;
            if (current != 0 && side != 0 && current != side) boundary = 0;
            if (current != 0) side = current;
        }
        if (!boundary) continue;
        if (count == 8) return 0;
        e->modelEdges[count][0] = (uint8_t)a;
        e->modelEdges[count][1] = (uint8_t)b;
        e->modelEdges[count][2] =
            fabsf(dx) <= 0.01f * fabsf(dy) && fabsf(tex[b][0] - tex[a][0]) < 0.01f
            ? MODEL_VERTICAL_CROP :
            fabsf(dy) <= 0.01f * fabsf(dx) && fabsf(tex[b][1] - tex[a][1]) < 0.01f
            ? MODEL_HORIZONTAL_CROP : MODEL_DIAGONAL;
        count++;
    }
    e->modelEdgeCount = (uint8_t)count;
    return count != 0;
}

/* EPL all-out art is already triangulated to a diagonal panel, not a rectangular
 * sprite. Cover that authored mask, then invert its scale in UV space so faces
 * retain their pixel size. Side portraits follow the retail outer screen edge;
 * their vertical translation follows the shared diagonal, not an arbitrary Y
 * offset. The attached glasses sparkle uses precisely the same translation. */
static int model_portrait_uv(P4AtomicCacheEntry *e, const float xy[8],
                             const float mvp[16], int portrait, float uv[3])
{
    float p[8][2];
    const int n = e->numVertices;
    const float ratio = p4gl_aspect_width();
    if (e->modelEdgeCount == 0 && !prepare_model_mask(e)) return 0;
    if (e->modelEdgeCount == 255) return 0;
    const int basisA = e->modelBasis[0], basisB = e->modelBasis[1];
    for (int i = 0; i < n; i++) {
        const float x = f32_at(e->verts, 12u * i);
        const float y = f32_at(e->verts, 12u * i + 4);
        const float z = f32_at(e->verts, 12u * i + 8);
        const float w = mvp[3] * x + mvp[7] * y + mvp[11] * z + mvp[15];
        if (fabsf(w) < 0.000001f) return 0;
        p[i][0] = (xy[0] * x + xy[2] * y + xy[4] * z + xy[6]) / w;
        p[i][1] = (xy[1] * x + xy[3] * y + xy[5] * z + xy[7]) / w;
    }
    const float determinant =
        (p[basisA][0] - p[0][0]) * (p[basisB][1] - p[0][1]) -
        (p[basisB][0] - p[0][0]) * (p[basisA][1] - p[0][1]);
    if (fabsf(determinant) < 0.000001f) return 0;
    float tx = 0.0f, ty = 0.0f;
    float cropLength = 0.0f, cropSide = 0.0f, diagonalLength = 0.0f;
    int diagonalA = 0, diagonalB = 0;
    const float canvasRatio = 640.0f * ratio / 448.0f;
    for (unsigned edge = 0; edge < e->modelEdgeCount; edge++) {
        const int a = e->modelEdges[edge][0], b = e->modelEdges[edge][1];
        const int crop = e->modelEdges[edge][2];
        const float dx = p[b][0] - p[a][0], dy = p[b][1] - p[a][1];
        const float length = dx * dx * canvasRatio * canvasRatio + dy * dy;
        if (crop != MODEL_DIAGONAL && length > cropLength) {
            cropLength = length;
            cropSide = crop == MODEL_VERTICAL_CROP ? p[a][0] + p[b][0] : 0.0f;
        }
        /* Three-member panels have steep dividers and retain their Y placement. */
        if (crop == MODEL_DIAGONAL && fabsf(dy) < fabsf(dx) * canvasRatio &&
            length > diagonalLength) {
            diagonalLength = length;
            diagonalA = a;
            diagonalB = b;
        }
    }
    if (cropSide != 0.0f) {
        tx = (cropSide < 0.0f ? -1.0f : 1.0f) * (1.0f - 1.0f / ratio);
        if (diagonalLength != 0.0f) {
            const float dx = p[diagonalB][0] - p[diagonalA][0];
            const float dy = p[diagonalB][1] - p[diagonalA][1];
            const float distance = dy * p[diagonalA][0] - dx * p[diagonalA][1];
            ty = (dy * tx - (ratio - 1.0f) * distance) / dx;
        }
    }
    uv[0] = ratio;
    for (int component = 0; component < 2; component++) {
        const float base = f32_at(e->uv, 4u * component);
        const float da = f32_at(e->uv, 8u * basisA + 4u * component) - base;
        const float db = f32_at(e->uv, 8u * basisB + 4u * component) - base;
        const float ax = (da * (p[basisB][1] - p[0][1]) -
                          db * (p[basisA][1] - p[0][1])) / determinant;
        const float ay = ((p[basisA][0] - p[0][0]) * db -
                          (p[basisB][0] - p[0][0]) * da) / determinant;
        const float origin = base - ax * p[0][0] - ay * p[0][1];
        uv[component + 1] = (1.0f - ratio) * origin - ax * tx - ay * ty;
    }
    sModelAnchor[portrait][0] = tx;
    sModelAnchor[portrait][1] = ty;
    sModelAnchorValid |= 1u << portrait;
    return 1;
}

/* The lights of the scene: the game's ambient light and up to two directional lights (K_Clump_GetLightA/B/C, added to the world
 * by the field loader). RpLight: type 3 at +0, subtype at +1 (1 directional, 2 ambient), flags at +2 (bit 0 lights atomics),
 * parent frame at +4, colour (r, g, b) floats at +24. A directional light shines along its frame's at vector. */
extern uint8_t *K_Clump_GetLightA(void);
extern uint8_t *K_Clump_GetLightB(void);
extern uint8_t *K_Clump_GetLightC(void);

typedef struct P4Lights {
    float amb[3];
    int numDir;
    float at[2][3];
    float col[2][3];
} P4Lights;

static P4Lights sLights;

static void gather_lights(void)
{
    uint8_t *(*const get[3])(void) = { K_Clump_GetLightA, K_Clump_GetLightB, K_Clump_GetLightC };
    int i;

    memset(&sLights, 0, sizeof sLights);
    for (i = 0; i < 3; i++) {
        const uint8_t *light = get[i]();
        float c[3];

        if (light == NULL || light[0] != 3 || (light[2] & 1) == 0) {
            continue;
        }
        {
            /* A light counts only while it is linked into the world's light list (RpWorldAddLight / RpWorldRemoveLight leave the
             * light's own link pointers stale after a removal): its neighbours must point back at its link. */
            uint32_t link = P4_H2G(light) + 52u;
            uint32_t nx;
            uint32_t pv;
            uint32_t back;
            uint32_t fwd;

            memcpy(&nx, light + 52, 4);
            memcpy(&pv, light + 56, 4);
            if (nx == 0 || pv == 0) {
                continue;
            }
            memcpy(&back, (const uint8_t *)P4_G2H(nx) + 4, 4);
            memcpy(&fwd, (const uint8_t *)P4_G2H(pv), 4);
            if (back != link || fwd != link) {
                continue;
            }
        }
        memcpy(c, light + 24, sizeof c);
        if (light[1] == 2) {
            sLights.amb[0] += c[0];
            sLights.amb[1] += c[1];
            sLights.amb[2] += c[2];
        } else if (light[1] == 1 && sLights.numDir < 2) {
            const void *frame = ptr_at(light, 4);
            float at[3];

            if (frame == NULL) {
                continue;
            }
            memcpy(at, (const uint8_t *)frame + P4_FRAME_LTM + 32, sizeof at);
            memcpy(sLights.at[sLights.numDir], at, sizeof at);
            memcpy(sLights.col[sLights.numDir], c, sizeof c);
            sLights.numDir++;
        }
    }
}


/* RpMatFX material data (RpMatFXMaterialGetEffects 0x00399d80 and the slot-search family at 0x0039a0xx): the record is the pointer at
 * material + the plugin offset the retail registration stored at 0x007646D0. Its effect kind is the word at +0x80 (1 bump, 2 env map,
 * 3 bump env, 4 dual texture, 5 UV transform, 6 dual UV transform) and two 0x40-byte slots hold the effect data, each tagged by an effect
 * type word at +0x20. The UV transform slot (type 5) holds the base RwMatrix pointer at +0 and the dual one at +4: the game's UVAnim
 * interpolators (mdlSetAnimControlTime, rpuvanim.c) rewrite those matrices every frame, which is how retail blinks, scrolls the TV
 * portal and moves the arena backdrops. The GS pipe applies a texture matrix to the vertex UVs before it draws:
 * u' = u * right.x + v * up.x + pos.x, v' = u * right.y + v * up.y + pos.y. */
#define P4_MATFX_PLUGIN_ADDR 0x007646D0u
#define P4_MATFX_SLOT_SIZE 0x40
#define P4_MATFX_SLOT_TYPE 0x20
#define P4_MATFX_EFFECTS 0x80
#define P4_MATFX_UVTRANSFORM 5
#define P4_MATFX_DUALUVTRANSFORM 6

static const uint8_t *matfx_record(const void *mat)
{
    const uint32_t off = *(const uint32_t *)P4_G2H(P4_MATFX_PLUGIN_ADDR);

    return mat != NULL && off != 0 ? ptr_at(mat, off) : NULL;
}

/* The material's base UV transform as the three columns of a mat3 (applied to (u, v, 1)) in the packed rows 20-22; identity when it has none. */
static void matfx_uv_columns(const void *mat)
{
    const uint8_t *rec = matfx_record(mat);
    int slot;

    sD[20][0] = 1.0f; sD[20][1] = 0.0f;
    sD[21][0] = 0.0f; sD[21][1] = 1.0f;
    sD[22][0] = 0.0f; sD[22][1] = 0.0f;
    if (rec == NULL) {
        return;
    }
    {
        const int32_t effects = i32_at(rec, P4_MATFX_EFFECTS);

        if (effects != P4_MATFX_UVTRANSFORM && effects != P4_MATFX_DUALUVTRANSFORM) {
            return;
        }
    }
    for (slot = 0; slot < 2; slot++) {
        const uint8_t *entry = rec + (size_t)slot * P4_MATFX_SLOT_SIZE;

        if (i32_at(entry, P4_MATFX_SLOT_TYPE) == P4_MATFX_UVTRANSFORM) {
            const uint8_t *m = ptr_at(entry, 0);

            if (m == NULL) {
                return;
            }
            sD[20][0] = f32_at(m, 0);   sD[20][1] = f32_at(m, 4);
            sD[21][0] = f32_at(m, 16);  sD[21][1] = f32_at(m, 20);
            sD[22][0] = f32_at(m, 48);  sD[22][1] = f32_at(m, 52);
            return;
        }
    }
}

static char sLogMat[160];

/* Alpha-test pass culling (p4gl_passcull.h): the bounds of this draw's fragment alpha, then the decision. P4PC_PASSCULL=0 keeps both passes. Returns bit 0: leave
 * out the first pass, bit 1: leave out the second. */
static unsigned fb_pass_cull(const P4AtomicCacheEntry *e, uint32_t bucket, int skinned, const void *mat, int32_t func, float ref)
{
    static int enabled = -1;
    unsigned tlo = 255, thi = 255;
    float matAlpha = 1.0f;

    if (enabled < 0) {
        enabled = p4gl_gl_feature("P4PC_PASSCULL");
    }
    if (!enabled) {
        return 0u;
    }
    if (func == 8) {
        return 1u; /* NEVER: the first pass discards every fragment */
    }
    if (mat != NULL) {
        const void *texture = ptr_at(mat, P4_MATERIAL_TEXTURE);
        const void *raster = texture != NULL ? ptr_at(texture, P4_TEXTURE_RASTER) : NULL;

        matAlpha = ((const uint8_t *)mat)[P4_MATERIAL_COLOR + 3] / 255.0f;
        if (raster != NULL) {
            const uint32_t sampler = (uint32_t)i32_at(texture, P4_TEXTURE_SAMPLER);

            /* border addressing samples a transparent border; a raster whose pixels are not all known may hold any alpha */
            if (((sampler >> 8) & 15u) == 4u || ((sampler >> 12) & 15u) == 4u || !p4gl_raster_alpha_range((const P4RwRaster *)raster, &tlo, &thi)) {
                return 0u;
            }
        }
    }
    return p4gl_alpha_pass_cull(func, ref, skinned ? e->skinAlphaLo : e->alphaRange[2u * bucket], skinned ? e->skinAlphaHi : e->alphaRange[2u * bucket + 1u], matAlpha,
                                tlo, thi);
}

static void bind_material(const void *mat)
{
    const int logWanted = p4gl_drawlog_wanted();

    sLogMat[0] = '\0';
    float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    int textured = 0;

    if (mat) {
        const uint8_t *rgba = (const uint8_t *)mat + P4_MATERIAL_COLOR;
        const void *texture = ptr_at(mat, P4_MATERIAL_TEXTURE);

        color[0] = rgba[0] / 255.0f;
        color[1] = rgba[1] / 255.0f;
        color[2] = rgba[2] / 255.0f;
        color[3] = rgba[3] / 255.0f;
        if (logWanted) {
            snprintf(sLogMat, sizeof sLogMat, "material rgba %02x%02x%02x%02x", rgba[0], rgba[1], rgba[2], rgba[3]);
        }
        if (texture) {
            const void *raster = ptr_at(texture, P4_TEXTURE_RASTER);

            if (raster) {
                const P4RwRaster *rr = (const P4RwRaster *)raster;

                textured = p4gl_texture_bind_sampler(P4_H2G(raster), (uint32_t)i32_at(texture, P4_TEXTURE_SAMPLER));
                if (logWanted) {
                    size_t l = strlen(sLogMat);
                    snprintf(sLogMat + l, sizeof sLogMat - l, " texture %dx%d d%d fmt%02x", (int)rr->width, (int)rr->height, (int)rr->depth, (unsigned)rr->cFormat);
                }
            }
        }
    }
    matfx_uv_columns(mat);
    memcpy(sD[16], color, sizeof color);
    sD[11][3] = (float)textured;
    sD[12][3] = (float)(textured ? p4gl_texture_border_mode() : 0);
    {
        /* RpMaterial.surfaceProps (ambient, specular, diffuse) at +12 scale the ambient and the directional lights. */
        float sp[3] = { 1.0f, 0.0f, 1.0f };
        float amb[3];
        float col[2][3];
        int d;
        int k;

        if (mat) {
            memcpy(sp, (const uint8_t *)mat + 12, sizeof sp);
        }
        for (k = 0; k < 3; k++) {
            amb[k] = sLights.amb[k] * sp[0];
            col[0][k] = sLights.col[0][k] * sp[2];
            col[1][k] = sLights.col[1][k] * sp[2];
        }
        for (d = sLights.numDir; d < 2; d++) {
            col[d][0] = col[d][1] = col[d][2] = 0.0f;
        }
        memcpy(sD[11], amb, sizeof amb);
        memcpy(sD[14], col[0], sizeof col[0]);
        memcpy(sD[15], col[1], sizeof col[1]);
    }
}

/* P4PC_GS_LOG (docs/renderer_port.md 11): one line per material draw, the material colour in rgba, the geometry flags in z, with the screen box of the geometry's bounding sphere (the whole screen for skinned meshes
 * and world sectors, whose vertices are already in world space) and the texture the material binds. */
static void atomic_gs_log(const void *frame, const void *camera, const void *morph, int skinned, const void *mat, uint32_t count, uint32_t flags)
{
    float box[4] = { 0.0f, 0.0f, 640.0f, 448.0f };
    float m[4][4];
    float mvp[16];
    uint32_t tex = 0;
    const void *texture = mat != NULL ? ptr_at(mat, P4_MATERIAL_TEXTURE) : NULL;
    const void *raster = texture != NULL ? ptr_at(texture, P4_TEXTURE_RASTER) : NULL;

    if (raster != NULL) {
        tex = P4_H2G(raster);
    }
    if (morph != NULL && !skinned && p4gl_camera_object_matrix(frame != NULL ? (const uint8_t *)frame + P4_FRAME_LTM : NULL, camera, m, mvp)) {
        float c[3];
        float radius;
        float cx, cy, cw, rx, ry;

        memcpy(c, (const uint8_t *)morph + 4, sizeof c);
        memcpy(&radius, (const uint8_t *)morph + 16, sizeof radius);
        cx = mvp[0] * c[0] + mvp[4] * c[1] + mvp[8] * c[2] + mvp[12];
        cy = mvp[1] * c[0] + mvp[5] * c[1] + mvp[9] * c[2] + mvp[13];
        cw = mvp[3] * c[0] + mvp[7] * c[1] + mvp[11] * c[2] + mvp[15];
        if (cw > 1e-3f) {
            rx = radius * sqrtf(mvp[0] * mvp[0] + mvp[1] * mvp[1] + mvp[2] * mvp[2]) / cw * 320.0f;
            ry = radius * sqrtf(mvp[4] * mvp[4] + mvp[5] * mvp[5] + mvp[6] * mvp[6]) / cw * 224.0f;
            box[0] = (cx / cw * 0.5f + 0.5f) * 640.0f - rx;
            box[2] = (cx / cw * 0.5f + 0.5f) * 640.0f + rx;
            box[1] = (0.5f - cy / cw * 0.5f) * 448.0f - ry;
            box[3] = (0.5f - cy / cw * 0.5f) * 448.0f + ry;
        }
    }
    {
        float rgba[4] = { 255.0f, 255.0f, 255.0f, 255.0f };
        int k;

        /* the material colour in the rgba field, the geometry flags (RpGeometry.flags, lit = 0x30) in z */
        for (k = 0; mat != NULL && k < 4; k++) {
            rgba[k] = (float)((const uint8_t *)mat)[P4_MATERIAL_COLOR + k];
        }
        p4gl_gs_log_draw("atomic", morph == NULL ? 2 : skinned, (int)count / 3, box, flags, rgba, tex,
                         p4gl_render_state_or(P4GL_RS_VERTEXALPHAENABLE, 0u) != 0u ? p4gl_sky_alpha_raw() : 0u, p4gl_sky_test_raw(),
                         (unsigned)p4gl_render_state_or(P4GL_RS_ZTESTENABLE, 1u), (unsigned)p4gl_render_state_or(P4GL_RS_ZWRITEENABLE, 1u));
    }
}

/* Draw one RpAtomic inside a camera update (p4gl_camera_current). Returns 1 when drawn, 0
 * when skipped or failed; failures print a diagnostic. */
int p4gl_atomic_draw(void *atomic)
{
    P4GeomView gv;
    const void *camera;
    const void *frame;
    P4AtomicCacheEntry *e;
    int32_t numMaterials;
    uint32_t b;
    int fbOnly;
    int skinned = 0;
    int32_t fbFunc = 0;
    float fbRef = 0.0f;
    const void *geometry = NULL;
    const void *morph = NULL;
    int sector;
    float mvp[16], originalXY[8];
    float canvasScale[2] = {1.0f, 1.0f}, canvasShift[2] = {0.0f, 0.0f};
    int savedXY = 0;

    if (atomic == NULL) {
        return 0;
    }
    sector = *(const uint8_t *)atomic == P4_SECTOR_TYPE_ATOMIC;
    if (!sector && *(const uint8_t *)atomic != P4_OBJECT_ATOMIC) {
        fprintf(stderr, "p4gl_atomic: node input is object type %u, not an atomic\n",
                (unsigned)*(const uint8_t *)atomic);
        return 0;
    }
    if (sector) {
        if (!view_of_sector(&gv, atomic)) {
            return 0;
        }
    } else {
        geometry = ptr_at(atomic, P4_ATOMIC_GEOMETRY);
        if (geometry == NULL) {
            return 0;
        }
        morph = ptr_at(geometry, P4_GEOM_MORPHTARGET);
        if (morph == NULL) {
            return 0;
        }
        view_of_geometry(&gv, geometry, morph);
    }
    camera = p4gl_camera_current();
    if (camera == NULL) {
        fprintf(stderr, "p4gl_atomic: atomic drawn outside a camera update\n");
        return 0;
    }
    /* Skinning, cache construction and uniforms below affect only this draw, not guest state.
     * The next drawn frame rebuilds/uploads from its current geometry and bone matrices. */
    if (!p4gl_gl_prepare_draw()) {
        return 1;
    }
    if (p4gl_profile_pass != NULL) p4gl_profile_pass(P4GL_PROFILE_SCENE);
    if (!ensure_ready()) {
        return 0;
    }

    if (sVaoOn < 0) {
        sVaoOn = p4gl_gl_feature("P4PC_ATOMIC_VAO");
    }
    if (!sVaoOn) {
        p4gl_glBindVertexArray(sVao);
    }
    e = cache_lookup_or_build(&gv);
    if (e == NULL) {
        return 0;
    }
    if (e->vao != 0) {
        p4gl_glBindVertexArray(e->vao);
    }

    if (sector) {
        frame = NULL; /* a world sector's vertices are in world space */
    } else {
        const void *skin = skin_of(geometry);
        const void *hier = hierarchy_of(atomic);

        frame = ptr_at(atomic, P4_ATOMIC_PARENT);
        if (skin != NULL && hier != NULL && skin_upload(e, geometry, morph, skin, hier)) {
            frame = NULL; /* skinned vertices are in world space */
            skinned = 1;
        }
    }
    p4gl_glUseProgram(sProgram); /* the uniforms below belong to this program, not the previous draw's */
    if (!upload_transforms(frame, camera, mvp)) {
        return 0;
    }

    p4gl_glUseProgram(sProgram);
    memcpy(sD[18], kLightDir, sizeof kLightDir);
    sD[13][3] = (float)(sViewerLight && gv.normals != NULL);
    gather_lights();
    sD[10][3] = (float)geometry_lit(&gv);
    memcpy(sD[12], sLights.at[0], sizeof sLights.at[0]);
    memcpy(sD[13], sLights.at[1], sizeof sLights.at[1]);

    /* Depth, blending and fog follow the render state the game set. Derived from sky2hook
     * src/render_queue.cpp (readRenderStateInto) and src/device.cpp (captureCameraState),
     * tge-was-taken, reused with permission; see third_party/README.md. Depth defaults on when the
     * game never set it; blending needs VERTEXALPHAENABLE; fog is linear from the camera's fog
     * plane to the FARFOGPLANE float (the camera far plane when that is not set), in FOGCOLOR
     * (a << 24 | r << 16 | g << 8 | b). */
    p4gl_depth_apply_values(p4gl_render_state_or(P4GL_RS_ZTESTENABLE, 1u) != 0u,
                            p4gl_render_state_or(P4GL_RS_ZWRITEENABLE, 1u) != 0u);
    p4gl_cull_apply();
    if (p4gl_render_state_or(P4GL_RS_VERTEXALPHAENABLE, 0u) != 0u) {
        p4gl_sky_apply_blend();
    } else {
        p4gl_glDisable(GL_BLEND);
    }
    {
        const uint32_t fogColor = p4gl_render_state_or(P4GL_RS_FOGCOLOR, 0u);
        const float fogStart = f32_at(camera, P4_CAMERA_FOGPLANE);
        float fogEnd = p4gl_sky_far_fog(fogStart);

        if (fogEnd == 0.0f) {
            fogEnd = f32_at(camera, P4_CAMERA_FAR);
        }
        sD[17][3] = (float)(p4gl_render_state_or(P4GL_RS_FOGENABLE, 0u) != 0u);
        sD[17][0] = (float)((fogColor >> 16) & 0xffu) / 255.0f;
        sD[17][1] = (float)((fogColor >> 8) & 0xffu) / 255.0f;
        sD[17][2] = (float)(fogColor & 0xffu) / 255.0f;
        sD[19][0] = fogStart;
        sD[19][1] = fogEnd;
    }
    sD[15][3] = (float)p4gl_sky_alpha_func();
    sD[18][3] = p4gl_sky_alpha_ref();
    sD[19][2] = 0.0f;
    sD[14][3] = (float)p4gl_sky_date();
    fbOnly = p4gl_gs_alpha_color_fail(p4gl_sky_test_raw(), &fbFunc, &fbRef);
    if (fbOnly) {
        sD[15][3] = (float)fbFunc;
        sD[18][3] = fbRef;
    }
    p4gl_frame_mask_apply();

    if (e->vao == 0) {
        p4gl_glBindBuffer(GL_ARRAY_BUFFER, e->vbo);
        p4gl_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, e->ibo);
        entry_attribs();
    }

    numMaterials = gv.numMaterials;
    const int modelCanvas = p4gl_layout_model_scale() != 1.0f;
    for (b = 0; b <= (uint32_t)numMaterials; b++) {
        uint32_t count = e->ranges[2u * b + 1u];
        const void *mat = NULL;

        if (count == 0) {
            continue;
        }
        if (b < (uint32_t)numMaterials) {
            mat = view_material(&gv, b); /* a guest array of 4-byte material addresses, or the meshes of a sector */
        }
        const int policy = modelCanvas ? p4gl_layout_model_material(mat) : P4GL_MODEL_FIT;
        const int portrait = policy >= P4GL_MODEL_PORTRAIT_A && policy <= P4GL_MODEL_PORTRAIT_D;
        const float targetScaleY = policy == P4GL_MODEL_COVER || portrait ? p4gl_aspect_width() : 1.0f;
        const float targetScaleX = policy == P4GL_MODEL_FILL_X ? p4gl_aspect_width() : targetScaleY;
        float shift[2] = {0.0f, 0.0f};
        if (policy >= P4GL_MODEL_GLINT_A && policy <= P4GL_MODEL_GLINT_D) {
            const unsigned slot = (unsigned)(policy - P4GL_MODEL_GLINT_A);
            if (sModelAnchorValid & (1u << slot)) memcpy(shift, sModelAnchor[slot], sizeof shift);
        }
        const int transformChanged = targetScaleX != canvasScale[0] || targetScaleY != canvasScale[1] ||
                                     shift[0] != canvasShift[0] || shift[1] != canvasShift[1];
        if ((portrait || transformChanged) && !savedXY) {
            for (unsigned column = 0; column < 4; column++) {
                originalXY[column * 2] = mvp[column * 4];
                originalXY[column * 2 + 1] = mvp[column * 4 + 1];
            }
            savedXY = 1;
        }
        if (portrait) {
            float uv[3];
            if (!model_portrait_uv(e, originalXY, mvp, policy - P4GL_MODEL_PORTRAIT_A, uv)) {
                fprintf(stderr, "p4gl_atomic: invalid all-out portrait mask\n");
                return 0;
            }
            memcpy(sD[10], uv, sizeof uv);
            sModelUvActive = 1;
        } else if (sModelUvActive) {
            sD[10][0] = sD[10][1] = sD[10][2] = 0.0f;
            sModelUvActive = 0;
        }
        if (transformChanged) {
            for (unsigned column = 0; column < 4; column++) {
                mvp[column * 4] = originalXY[column * 2] * targetScaleX + shift[0] * mvp[column * 4 + 3];
                mvp[column * 4 + 1] = originalXY[column * 2 + 1] * targetScaleY + shift[1] * mvp[column * 4 + 3];
            }
            memcpy(sD[0], mvp, sizeof mvp);
            canvasScale[0] = targetScaleX;
            canvasScale[1] = targetScaleY;
            memcpy(canvasShift, shift, sizeof canvasShift);
        }
        bind_material(mat);
        {
            int skipped = !p4gl_drawlog("atomic", 4, (int)count, NULL, NULL, sLogMat);

            if (p4gl_gs_log_on()) {
                atomic_gs_log(frame, camera, morph, skinned, mat, count, (uint32_t)gv.flags);
                skipped |= p4gl_gs_skip();
            }
            if (skipped) {
                continue;
            }
        }
        if (p4gl_sky_date()) {
            int width, height;
            if (!p4gl_target_pixel_size(&width, &height)) return 0;
            p4gl_dest_alpha_snapshot(width, height);
        }
        const unsigned cull = fbOnly ? fb_pass_cull(e, b, skinned, mat, fbFunc, fbRef) : 0u;

        if (!(cull & 1u)) {
            d_flush();
            p4gl_glDrawElements(GL_TRIANGLES, (GLsizei)count, GL_UNSIGNED_INT,
                                (const void *)(uintptr_t)(e->ranges[2u * b] * sizeof(uint32_t))); /* byte offset into the bound index buffer */
        }
        if (fbOnly && !(cull & 2u)) {
            /* Failed FB_ONLY/RGB_ONLY fragments never write Z; RGB_ONLY also keeps Ad. */
            sD[19][2] = 1.0f;
            d_flush();
            p4gl_glDepthMask(GL_FALSE);
            if (fbOnly == 3) {
                p4gl_frame_mask_rgb_only();
            }
            p4gl_glDrawElements(GL_TRIANGLES, (GLsizei)count, GL_UNSIGNED_INT,
                                (const void *)(uintptr_t)(e->ranges[2u * b] * sizeof(uint32_t)));
            if (fbOnly == 3) {
                p4gl_frame_mask_restore();
                p4gl_frame_mask_apply();
            }
            p4gl_glDepthMask(p4gl_render_state_or(P4GL_RS_ZWRITEENABLE, 1u) != 0u ? GL_TRUE : GL_FALSE);
            sD[19][2] = 0.0f;
        }
    }
    p4gl_frame_mask_restore();
    p4gl_depth_restore();
    return 1;
}
