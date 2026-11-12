/* PS2PTank on OpenGL 3.3 (the node body 0x003a8840, docs/renderer_port.md section 9).
 *
 * Derived from sky2hook src/pipelines/atomic/ptank.cpp (parse and submitQuads), tge-was-taken,
 * reused with permission; see third_party/README.md. sky2hook measured this against PCSX2; the
 * record layout below was cross-checked against the retail create code (0x003a1730) and the node
 * body (0x003a8840), and the semantics (corner order, half-size quads, rotation, texture
 * coordinate pairing, colour precedence, the tank's own GS registers) are sky2hook's.
 *
 * The retail node body sends DMA chunks that the PTank instance functions (rpPTankSkyIns*,
 * 0x003a5740..0x003b4120) built from the particle arrays, and a VU1 program expands each particle
 * into a quad. The port draws the particle arrays directly, so it never needs the instanced
 * chunks, the DMA tags or the GS register packet.
 *
 * The atomic's PTank record (RpPTankAtomicExtPrv) is the pointer at atomic + iGpffffb610 (the
 * plugin offset). Record offsets:
 *   +0x00 maxPCount  +0x04 actPCount  +0x3c lockFlags  +0x40 instFlags  +0x44 platFlags
 *   +0x50 publicData (RpPTankData, offsets below relative to it)
 * publicData:
 *   +0x04 + 8 * c  cluster c: data pointer, stride (c = 0 position, 1 matrix, 2 normal, 3 size,
 *                  4 colour, 5 vertex colour, 6 2D rotation, 7 two texture coordinates,
 *                  8 four texture coordinates). A cluster is live only when its data flag is set;
 *                  the others hold stale allocation bytes.
 *   +0x4c userData (RpPTankSkyData: GS ALPHA at +0xb8, CLAMP +0xc0, TEST +0xc8, TEX1 +0xd0)
 *   +0x58 dataFlags   +0x5c srcBlend   +0x60 dstBlend (the sky path never sets them)
 *   +0x64 vertexAlphaBlend (becomes PRIM.ABE: a tank without it does not blend)
 *   +0x68 cCenter  +0x70 cSize  +0x78 cRotate  +0x7c cColor  +0x80 cVtxColor[4]  +0x90 cUV[4]
 *   +0xb0 cMatrix
 *
 * Generators: the 3D sprite generator (platFlags 2) places each particle by its position, which is
 * object space. The 2D generator (platFlags 0) with only a position places it in screen space under
 * a half-scale VU1 projection, which putting through the camera draws wrongly (sky2hook skips it
 * too); such tanks are skipped with one log line. A tank with a matrix per particle (the field rain,
 * data flags colour | matrix, built by the 2D generator) is placed by the matrix whichever generator
 * made it, so it is drawn: the matrix's row 3 is the centre unless a position cluster overrides it.
 * Billboards: a quad of width and height `size` centred on the position in the camera plane,
 * rotated by the 2D rotation; the corners are top-left, top-right, bottom-right, bottom-left.
 * A matrix supplies the quad's full right and up dimensions (the size cluster is dead then).
 * rpPTANKDFLAGUSECENTER (cCenter) is not applied; no tank on this disc sets it. The clone pipeline
 * is not drawn. Colour is material colour x texel x particle colour, where the particle colour is
 * the per-particle one, else the four corner colours (per-particle or constant), else cColor. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4gl.h"
#include "p4pc/gmem.h"

/* Retail PTank rotation: jal 003abdac -> sinf, 003abdb8 -> cosf, both
 * taking the same float angle in f12. Host double trig changes quad corners. */
extern float func_0044b7b0(float angle);
extern float func_0044b610(float angle);

/* RpPTank plugin offset in RpAtomic (retail gp-global, config/symbol_data_addrs.txt). */
#define iGpffffb610 P4_GPDATA(int32_t, 0xb610)

#define P4_ATOMIC_PARENT 4          /* RwObject.parent: the RwFrame */
#define P4_ATOMIC_GEOMETRY 24       /* RpAtomic.geometry */
#define P4_FRAME_LTM 80             /* RwFrame.ltm (RwMatrix) */
#define P4_GEOM_MATERIALS 32        /* RpGeometry.matList.materials */
#define P4_MATERIAL_TEXTURE 0       /* RpMaterial.texture */
#define P4_MATERIAL_COLOR 4         /* RpMaterial.color (RwRGBA) */
#define P4_TEXTURE_RASTER 0         /* RwTexture.raster */

#define PT_MAXCOUNT 0x00
#define PT_ACTCOUNT 0x04
#define PT_PLATFLAGS 0x44
#define PT_PUBLIC 0x50
#define PT_PLAT_SPR3DGEN 2u

#define PD_CLUSTER 0x04
#define PD_USERDATA 0x4c
#define PD_DATAFLAGS 0x58
#define PD_VALPHA 0x64
#define PD_CSIZE 0x70
#define PD_CROTATE 0x78
#define PD_CCOLOR 0x7c
#define PD_CVTXCOLOR 0x80
#define PD_CUV 0x90
#define PD_CMATRIX 0xb0

#define SKY_ALPHA 0xb8
#define SKY_TEST 0xc8

#define CL_POSITION 0
#define CL_MATRIX 1
#define CL_SIZE 3
#define CL_COLOR 4
#define CL_VTXCOLOR 5
#define CL_ROTATE 6
#define CL_UV2 7
#define CL_UV4 8

/* The data flag that instances each cluster slot (rpPTANKDFLAG*, rpptank.h). The bits are not in
 * slot order. */
static const uint32_t kClusterFlag[9] = {
    0x00000001u, /* position */
    0x00000008u, /* matrix */
    0x00000010u, /* normal */
    0x00000004u, /* size */
    0x00000002u, /* colour */
    0x00000040u, /* vertex colour */
    0x00000020u, /* 2D rotation */
    0x00000080u, /* two texture coordinates */
    0x00000100u, /* four texture coordinates */
};
#define DF_CNSVTXCOLOR 0x00040000u
#define DF_CNSMATRIX 0x00008000u

typedef struct Cluster {
    const uint8_t *data;
    int32_t stride;
} Cluster;

static P4ClipVertex *sVerts;
static uint32_t *sIndices;
static uint32_t sCapacity;   /* particles the buffers hold */
static uint32_t sWarnedPlat = 0xffffffffu;

/* A pointer field of a retail struct is a 4-byte guest address (read through memcpy: the offsets are not all aligned). */
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

static uint64_t u64_at(const void *base, size_t off)
{
    uint64_t value;

    memcpy(&value, (const uint8_t *)base + off, sizeof value);
    return value;
}

static float f32_at(const void *base, size_t off)
{
    float value;

    memcpy(&value, (const uint8_t *)base + off, sizeof value);
    return value;
}

/* The cluster in slot c, empty when its data flag is not set. */
static Cluster cluster_at(const uint8_t *pd, uint32_t flags, int c)
{
    Cluster out = {NULL, 0};

    if (flags & kClusterFlag[c]) {
        out.data = ptr_at(pd, PD_CLUSTER + 8u * (unsigned)c);
        out.stride = i32_at(pd, PD_CLUSTER + 8u * (unsigned)c + 4u);
    }
    return out;
}

static const uint8_t *element(const Cluster *c, int32_t i)
{
    return c->data + (ptrdiff_t)i * c->stride;
}

static void rgba_to_float(const uint8_t *rgba, float out[4])
{
    out[0] = rgba[0] / 255.0f;
    out[1] = rgba[1] / 255.0f;
    out[2] = rgba[2] / 255.0f;
    out[3] = rgba[3] / 255.0f;
}

static int ensure_capacity(uint32_t particles)
{
    P4ClipVertex *verts;
    uint32_t *indices;

    if (particles <= sCapacity) {
        return 1;
    }
    verts = realloc(sVerts, (size_t)particles * 4u * sizeof *verts);
    if (verts == NULL) {
        return 0;
    }
    sVerts = verts;
    indices = realloc(sIndices, (size_t)particles * 6u * sizeof *indices);
    if (indices == NULL) {
        return 0;
    }
    sIndices = indices;
    sCapacity = particles;
    return 1;
}

/* Camera-space point of object-space (x, y, z) under m (object to camera). */
static void to_camera(float m[4][4], float x, float y, float z, float out[3])
{
    int r;

    for (r = 0; r < 3; r++) {
        out[r] = m[r][0] * x + m[r][1] * y + m[r][2] * z + m[r][3];
    }
}

/* Camera-space direction of the object-space vector (x, y, z): the rotation part of m. */
static void dir_to_camera(float m[4][4], const float v[3], float out[3])
{
    int r;

    for (r = 0; r < 3; r++) {
        out[r] = m[r][0] * v[0] + m[r][1] * v[1] + m[r][2] * v[2];
    }
}

static void to_clip(const float proj[6], const float cam[3], float out[4])
{
    out[0] = cam[0] * proj[0] + cam[2] * proj[4];
    out[1] = cam[1] * proj[1] + cam[2] * proj[5];
    out[2] = cam[2] * proj[2] + proj[3];
    out[3] = cam[2];
}

int p4gl_ptank_draw(void *atomic)
{
    /* Top-left, top-right, bottom-right, bottom-left. */
    static const float corner[4][2] = {{-1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, -1.0f}, {-1.0f, -1.0f}};
    static const uint32_t quad[6] = {0, 1, 2, 0, 2, 3};
    const uint8_t *record;
    const uint8_t *pd;
    const uint8_t *userData;
    const void *camera;
    const void *frame;
    const void *geometry;
    const void *materials;
    const void *material = NULL;
    P4ClipDrawParams params;
    uint32_t flags;
    uint32_t platFlags;
    int32_t count;
    int32_t i;
    int k;
    int j;
    float m[4][4];
    float mvp[16];
    float proj[6];
    Cluster position;
    Cluster matrix;
    Cluster size;
    Cluster color;
    Cluster vtxColor;
    Cluster rotate;
    Cluster uv2;
    Cluster uv4;
    int cnsVtxColor;

    if (atomic == NULL) {
        return 0;
    }
    record = ptr_at(atomic, (size_t)(uint32_t)iGpffffb610);
    if (record == NULL) {
        return 0;
    }
    count = i32_at(record, PT_ACTCOUNT);
    if (count <= 0 || count > i32_at(record, PT_MAXCOUNT)) {
        return 1;
    }
    platFlags = (uint32_t)i32_at(record, PT_PLATFLAGS);
    camera = p4gl_camera_current();
    if (camera == NULL) {
        fprintf(stderr, "p4gl_ptank: PTank drawn outside a camera update\n");
        return 0;
    }

    pd = record + PT_PUBLIC;
    flags = (uint32_t)i32_at(pd, PD_DATAFLAGS);
    position = cluster_at(pd, flags, CL_POSITION);
    matrix = cluster_at(pd, flags, CL_MATRIX);
    size = cluster_at(pd, flags, CL_SIZE);
    color = cluster_at(pd, flags, CL_COLOR);
    vtxColor = cluster_at(pd, flags, CL_VTXCOLOR);
    rotate = cluster_at(pd, flags, CL_ROTATE);
    uv2 = cluster_at(pd, flags, CL_UV2);
    uv4 = cluster_at(pd, flags, CL_UV4);
    cnsVtxColor = (flags & DF_CNSVTXCOLOR) != 0u;
    /* The 2D sprite generator (platFlags 0) places a particle in screen space under a half-scale VU1
     * projection when it has only a position; sky2hook skips those, and so does this. A tank with a
     * matrix per particle (the field rain, dataFlags 0xA) carries its world placement in the matrix
     * whichever generator built it, so it is drawn. */
    if (platFlags != PT_PLAT_SPR3DGEN && matrix.data == NULL && !(flags & DF_CNSMATRIX)) {
        if (sWarnedPlat != platFlags) {
            fprintf(stderr, "p4gl_ptank: platFlags 0x%x without a matrix is the screen-space generator; PTank skipped\n",
                    (unsigned)platFlags);
            sWarnedPlat = platFlags;
        }
        return 0;
    }
    if (position.data == NULL && matrix.data == NULL && !(flags & DF_CNSMATRIX)) {
        return 0;
    }

    frame = ptr_at(atomic, P4_ATOMIC_PARENT);
    if (!p4gl_camera_object_matrix(frame != NULL ? (const uint8_t *)frame + P4_FRAME_LTM : NULL,
                                   camera, m, mvp) ||
        !p4gl_camera_projection(camera, proj)) {
        return 0;
    }
    if (!ensure_capacity((uint32_t)count)) {
        fprintf(stderr, "p4gl_ptank: cannot allocate %d particles\n", (int)count);
        return 0;
    }

    memset(&params, 0, sizeof params);
    params.color[0] = params.color[1] = params.color[2] = params.color[3] = 1.0f;
    geometry = ptr_at(atomic, P4_ATOMIC_GEOMETRY);
    materials = geometry != NULL ? ptr_at(geometry, P4_GEOM_MATERIALS) : NULL;
    if (materials != NULL) {
        material = ptr_at(materials, 0);
    }
    if (material != NULL) {
        const void *texture = ptr_at(material, P4_MATERIAL_TEXTURE);

        rgba_to_float((const uint8_t *)material + P4_MATERIAL_COLOR, params.color);
        if (texture != NULL) {
            params.raster = P4_H2G(ptr_at(texture, P4_TEXTURE_RASTER));
        }
    }

    for (i = 0; i < count; i++) {
        P4ClipVertex *vq = &sVerts[(size_t)i * 4u];
        const uint8_t *mat = NULL;
        const uint8_t *pos = NULL;
        const uint8_t *particleColor = color.data != NULL ? element(&color, i) : NULL;
        const uint8_t *vtxColors = NULL;
        float centerCam[3];
        float rightCam[3];
        float upCam[3];
        float colorF[4];
        float uv[4][2];

        if (matrix.data != NULL) {
            mat = element(&matrix, i);
        } else if (flags & DF_CNSMATRIX) {
            mat = (const uint8_t *)pd + PD_CMATRIX;
        }
        /* A particle with a matrix is placed by it (pos is its row 3); the position cluster, when
         * there is one, overrides that. */
        pos = position.data != NULL ? element(&position, i) : mat + 48;
        to_camera(m, f32_at(pos, 0), f32_at(pos, 4), f32_at(pos, 8), centerCam);
        if (mat != NULL) {
            /* The matrix, like cSize, contains full dimensions. Retail's matrix generator
             * expands +/- half of each row; using the full row quadruples a rain quad's area. */
            const float right[3] = {0.5f * f32_at(mat, 0), 0.5f * f32_at(mat, 4), 0.5f * f32_at(mat, 8)};
            const float up[3] = {0.5f * f32_at(mat, 16), 0.5f * f32_at(mat, 20), 0.5f * f32_at(mat, 24)};

            dir_to_camera(m, right, rightCam);
            dir_to_camera(m, up, upCam);
        } else {
            float sx = f32_at(pd, PD_CSIZE);
            float sy = f32_at(pd, PD_CSIZE + 4);
            float rot = f32_at(pd, PD_CROTATE);
            float cosR = 1.0f;
            float sinR = 0.0f;

            if (size.data != NULL) {
                const uint8_t *s = element(&size, i);

                sx = f32_at(s, 0);
                sy = f32_at(s, 4);
            }
            if (rotate.data != NULL) {
                rot = f32_at(element(&rotate, i), 0);
            }
            if (rot != 0.0f) {
                sinR = func_0044b7b0(rot);
                cosR = func_0044b610(rot);
            }
            /* The camera's right and up, rotated by rot, scaled to half the size. */
            rightCam[0] = cosR * sx * 0.5f;
            rightCam[1] = sinR * sx * 0.5f;
            rightCam[2] = 0.0f;
            upCam[0] = -sinR * sy * 0.5f;
            upCam[1] = cosR * sy * 0.5f;
            upCam[2] = 0.0f;
        }

        if (particleColor != NULL) {
            rgba_to_float(particleColor, colorF);
        } else {
            rgba_to_float((const uint8_t *)pd + PD_CCOLOR, colorF);
            if (vtxColor.data != NULL) {
                vtxColors = element(&vtxColor, i);
            } else if (cnsVtxColor) {
                vtxColors = (const uint8_t *)pd + PD_CVTXCOLOR;
            }
        }

        /* Two coordinates are the sprite rectangle's opposite corners, so the other two corners
         * take the mixed pair. A constant pair stands in when the cluster is absent. */
        if (uv4.data != NULL) {
            const uint8_t *u = element(&uv4, i);

            for (k = 0; k < 4; k++) {
                uv[k][0] = f32_at(u, 8 * k);
                uv[k][1] = f32_at(u, 8 * k + 4);
            }
        } else {
            const uint8_t *u = uv2.data != NULL ? element(&uv2, i) : (const uint8_t *)pd + PD_CUV;
            const float u0 = f32_at(u, 0);
            const float v0 = f32_at(u, 4);
            const float u1 = f32_at(u, 8);
            const float v1 = f32_at(u, 12);

            uv[0][0] = u0; uv[0][1] = v0;
            uv[1][0] = u1; uv[1][1] = v0;
            uv[2][0] = u1; uv[2][1] = v1;
            uv[3][0] = u0; uv[3][1] = v1;
        }

        for (k = 0; k < 4; k++) {
            float cam[3];
            P4ClipVertex *v = &vq[k];

            for (j = 0; j < 3; j++) {
                cam[j] = centerCam[j] + corner[k][0] * rightCam[j] + corner[k][1] * upCam[j];
            }
            to_clip(proj, cam, v->pos);
            if (vtxColors != NULL) {
                rgba_to_float(vtxColors + 4 * k, v->col);
            } else {
                memcpy(v->col, colorF, sizeof v->col);
            }
            v->uv[0] = uv[k][0];
            v->uv[1] = uv[k][1];
        }
        for (k = 0; k < 6; k++) {
            sIndices[(size_t)i * 6u + (size_t)k] = (uint32_t)i * 4u + quad[k];
        }
    }

    /* The tank carries its own GS registers, which the retail node sends ahead of the particles:
     * ALPHA and TEST at userData + 0xb8 and + 0xc8. sky2hook accepts them only when the undefined
     * bits are clear (a wrong layout would read as unrelated bytes); otherwise the draw follows the
     * RpSky state. */
    userData = ptr_at(pd, PD_USERDATA);
    if (userData != NULL) {
        const uint64_t alpha = u64_at(userData, SKY_ALPHA);
        const uint64_t test = u64_at(userData, SKY_TEST);

        if ((alpha & ~0x000000ff000000ffull) == 0u && (test >> 19) == 0u) {
            const uint32_t t = (uint32_t)test;
            const uint32_t atst = (t >> 1) & 7u;
            const uint32_t afail = (t >> 12) & 3u;
            const uint32_t zte = (t >> 16) & 1u;
            const uint32_t ztst = (t >> 17) & 3u;

            params.own = 1;
            params.blend = i32_at(pd, PD_VALPHA) != 0;
            params.alpha = alpha;
            params.test = t;
            /* Depth test on only for ZTE with a GEQUAL or GREATER test, as a GS draw reads it. ATST
             * NEVER with FB_ONLY writes colour and no depth. */
            params.zTest = zte != 0u && ztst >= 2u;
            params.zWrite = p4gl_render_state(P4GL_RS_ZWRITEENABLE) != 0u &&
                            !((t & 1u) != 0u && atst == 0u && afail == 1u);
        }
    }
    return p4gl_clip_draw_triangles(sVerts, (uint32_t)count * 4u, sIndices, (uint32_t)count * 6u,
                                    &params);
}
