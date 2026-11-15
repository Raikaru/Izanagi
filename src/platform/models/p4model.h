/* Cooked glTF models: the compact internal form a [resource-model] glTF is turned into once (cached by
 * content hash), and that the runtime builder turns into RenderWare objects (docs/modding/models.md,
 * "Runtime glTF models").
 *
 * A cooked model is one little-endian blob: a header, then fixed-size texture, material and mesh
 * records, then the arrays they name by byte offset from the blob start (4-byte aligned). It carries
 * only what the builder needs, already in P4's formats: paletted (PAL8) or 32-bit textures with
 * straight alpha, RenderWare material state, and per-mesh vertex arrays and triangles in the space of
 * the RenderWare frame the mesh hangs from. A skinned mesh's vertices are in skin space and its skin
 * names its bones by HAnim node id; the builder maps them onto the replaced model's hierarchy.
 * Pure C: no RenderWare, guest memory or package state. */
#ifndef P4PC_PLATFORM_MODELS_P4MODEL_H
#define P4PC_PLATFORM_MODELS_P4MODEL_H

#include <stddef.h>
#include <stdint.h>

#define P4_MODEL_COOKED_MAGIC 0x434D3450u /* "P4MC" */
#define P4_MODEL_COOKED_VERSION 3u
#define P4_MODEL_NAME_BYTES 32u
/* Bumped whenever the cooker's output for the same input changes: part of the cache key. */
#define P4_MODEL_COOKER_ID "p4model-cook-3"

enum {
    P4_MODEL_FRAME_ROOT = 0,   /* the clump's root frame */
    P4_MODEL_FRAME_HANIM = 1,  /* the frame whose HAnim node id is frameId (glTF node "bone_<id>") */
    P4_MODEL_FRAME_HELPER = 2, /* the frame whose per3HelperId user data is frameId ("helper_<id>") */
    P4_MODEL_FRAME_SKIN = 3    /* a skinned mesh: the frame of the replaced model's first skinned atomic */
};
enum {
    P4_MODEL_MATERIAL_FOG_DISABLE = 1u, /* per3FogDisable */
    P4_MODEL_MATERIAL_TINT = 2u,        /* per3modelMatColor = tint */
    P4_MODEL_MATERIAL_UNLIT = 4u        /* KHR_materials_unlit */
};
/* RenderWare geometry format bits the cooker sets (rpGEOMETRY*). */
enum {
    P4_MODEL_GEO_TRISTRIP = 0x1u, P4_MODEL_GEO_POSITIONS = 0x2u, P4_MODEL_GEO_TEXTURED = 0x4u,
    P4_MODEL_GEO_PRELIT = 0x8u, P4_MODEL_GEO_NORMALS = 0x10u, P4_MODEL_GEO_LIGHT = 0x20u,
    P4_MODEL_GEO_MODULATE = 0x40u, P4_MODEL_GEO_TEXTURED2 = 0x80u
};

typedef struct P4ModelCookedHeader {
    uint32_t magic, version, bytes, flags;
    uint32_t textures, materials, meshes, triangles;
    uint32_t texturesOffset, materialsOffset, meshesOffset, textureBytes;
    uint32_t bodyCrc;        /* CRC-32 of every byte after the header: a damaged cache file is a miss */
    uint32_t animations;     /* animation slot replacements (P4ModelCookedAnimation) */
    uint32_t animationsOffset;
    uint32_t reserved;
    char source[68]; /* SHA-256 hex of the cooker id and every input byte, NUL-terminated */
    uint32_t reserved2[3];
} P4ModelCookedHeader;

typedef struct P4ModelCookedTexture {
    char name[P4_MODEL_NAME_BYTES];     /* unique RenderWare texture name */
    uint32_t width, height, depth;      /* depth 8 (PAL8, 256-entry palette) or 32 (RGBA8) */
    uint32_t filterAddressing;          /* RwTexture filter (bits 0-7) and U/V addressing (8-11, 12-15) */
    uint32_t paletteOffset;             /* depth 8: 256 straight RGBA8 entries in logical index order */
    uint32_t pixelsOffset, pixelsBytes; /* rows top first: indices (8) or straight RGBA8 (32) */
    uint32_t reserved;
} P4ModelCookedTexture;

typedef struct P4ModelCookedMaterial {
    uint8_t color[4];                   /* RpMaterial colour, RGBA8 */
    float ambient, specular, diffuse;   /* surface properties */
    int32_t texture;                    /* texture record index, or -1 */
    int32_t drawCategory;               /* per3MatAlpha, or -1 for none */
    uint32_t flags;                     /* P4_MODEL_MATERIAL_* */
    uint32_t tint;                      /* per3modelMatColor when P4_MODEL_MATERIAL_TINT */
    char name[P4_MODEL_NAME_BYTES];     /* material "name" user data (material animations match it) */
} P4ModelCookedMaterial;

typedef struct P4ModelCookedMesh {
    int32_t frameKind, frameId;         /* P4_MODEL_FRAME_* and its id */
    uint32_t vertices, triangles;
    uint32_t format;                    /* P4_MODEL_GEO_* plus the UV set count in bits 16-23 */
    uint32_t uvSets;
    uint32_t positionsOffset;           /* float x, y, z per vertex */
    uint32_t normalsOffset;             /* float x, y, z per vertex, or 0 */
    uint32_t uv0Offset, uv1Offset;      /* float u, v per vertex, or 0 */
    uint32_t colorsOffset;              /* RGBA8 per vertex, or 0 */
    uint32_t trianglesOffset;           /* u16 v0, v1, v2, material slot per triangle */
    uint32_t materialCount;
    uint32_t materialsOffset;           /* u32 material record index per slot */
    uint32_t skinOffset;                /* a P4ModelCookedSkin, or 0 for a static mesh */
    uint32_t reserved;
} P4ModelCookedMesh;

/* A skinned mesh's skin (glTF skin, JOINTS_0 and WEIGHTS_0). Bones are HAnim node ids, so the skin
 * fits any model whose hierarchy has those nodes; RenderWare bone indices are hierarchy node indices,
 * which the builder resolves (src/platform/models/p4model_stream.c). */
typedef struct P4ModelCookedSkin {
    uint32_t joints;                    /* glTF skin joints: at most 256 */
    uint32_t maxWeights;                /* the most non-zero weights of one vertex, 1 to 4 */
    uint32_t jointIdsOffset;            /* int32 HAnim node id per joint ("bone_<id>") */
    uint32_t matricesOffset;            /* 16 words per joint: the inverse bind matrix as an RwMatrix (right,
                                         * flags, up, pad, at, pad, pos, pad), the skin-to-bone matrix */
    uint32_t indicesOffset;             /* 4 joint slots (u8) per vertex */
    uint32_t weightsOffset;             /* 4 floats per vertex, non-zero ones first; a zero weight's slot is 0 */
    uint32_t orderCount, orderOffset;   /* the used bones' order (joint slots, u8) from extras.p4.skin.usedBones, or 0 */
} P4ModelCookedSkin;

/* A glTF animation named L<layer>S<slot> (rw_to_gltf's name for a slot's animation): it replaces the animation
 * mdlReadAuxChunks reads into that slot. Keys are RtCompressedKeyFrame values (P4's retail animation
 * scheme): 16-bit packed floats for the quaternion and the translation, which decodes as
 * value * translationScale + translationOffset. One track per HAnim bone of the glTF. */
typedef struct P4ModelCookedAnimation {
    uint32_t layer, slot;
    float duration;                     /* seconds: every track's first key is at 0 and its last at duration */
    uint32_t flags;                     /* RtAnimAnimation flags (extras.p4.flags), usually 0 */
    uint32_t tracks, tracksOffset;      /* P4ModelCookedTrack[tracks] */
    float translationOffset[3], translationScale[3];
    uint32_t reserved[4];
} P4ModelCookedAnimation;

typedef struct P4ModelCookedTrack {
    int32_t nodeId;                     /* HAnim node id ("bone_<id>") */
    uint32_t keys, keysOffset;          /* P4ModelCookedKey[keys], times ascending, at least 2 */
    uint32_t reserved;
} P4ModelCookedTrack;

typedef struct P4ModelCookedKey {
    float time;
    uint16_t rotation[4];               /* x, y, z, w */
    uint16_t translation[3];
    uint16_t pad;
} P4ModelCookedKey;

/* Per-target bounds (docs/modding/models.md, "Budgets"). */
typedef struct P4ModelBudget {
    const char *category;
    uint32_t triangles;      /* whole model */
    uint32_t meshes;
    uint32_t vertices;       /* per mesh: RenderWare triangles index vertices with 16 bits */
    uint32_t textures;
    uint32_t textureSize;    /* largest width or height; both must be powers of two from 8 */
    uint32_t textureBytes;   /* decoded P4 texture bytes (palettes included) */
    uint32_t sourceBytes;    /* glTF input, all files */
} P4ModelBudget;

/* The budget for a model replacing disc key `key` (DATA/MODEL/WEAPON/... is a weapon). */
const P4ModelBudget *p4_model_budget_for(const char *key);

/* Reads a file the glTF names by URI (a .bin buffer or a PNG). 0 and a malloc'd copy, or -1. */
typedef int (*P4ModelReadFn)(void *user, const char *uri, uint8_t **data, size_t *size);

/* Cooks glTF 2.0 text or GLB bytes. `name` is used in messages and texture names. On success returns 0
 * and a malloc'd blob in *out (free with free()). On refusal returns -1 with a reason naming the glTF
 * object and the exceeded bound. */
int p4_model_cook(const char *name, const uint8_t *gltf, size_t size, P4ModelReadFn read, void *user,
                  const P4ModelBudget *budget, uint8_t **out, size_t *out_size, char *error, size_t error_size);

/* 0 when blob is a complete, self-consistent cooked model of this version (every record and array
 * inside it, every index in range); -1 otherwise. Cache loads use it before trusting a file. */
int p4_model_cooked_check(const uint8_t *blob, size_t size);

static inline const P4ModelCookedHeader *p4_model_header(const uint8_t *blob)
{
    return (const P4ModelCookedHeader *)(const void *)blob;
}
static inline const P4ModelCookedTexture *p4_model_texture(const uint8_t *blob, uint32_t i)
{
    return (const P4ModelCookedTexture *)(const void *)(blob + p4_model_header(blob)->texturesOffset) + i;
}
static inline const P4ModelCookedMaterial *p4_model_material(const uint8_t *blob, uint32_t i)
{
    return (const P4ModelCookedMaterial *)(const void *)(blob + p4_model_header(blob)->materialsOffset) + i;
}
static inline const P4ModelCookedMesh *p4_model_mesh(const uint8_t *blob, uint32_t i)
{
    return (const P4ModelCookedMesh *)(const void *)(blob + p4_model_header(blob)->meshesOffset) + i;
}
static inline const P4ModelCookedSkin *p4_model_skin(const uint8_t *blob, const P4ModelCookedMesh *mesh)
{
    return mesh->skinOffset ? (const P4ModelCookedSkin *)(const void *)(blob + mesh->skinOffset) : NULL;
}

/* The RenderWare geometry chunk (0x0F) of cooked skinned mesh `mesh`, byte for byte in the form retail
 * RMDs store and RpGeometryStreamRead reads: struct, material list, and an extension with the bin mesh
 * (0x50E) and the skin (0x116) whose bone indices are the hierarchy node indices of the skin's joints.
 * `node_ids` lists the HAnim node id of each hierarchy node in index order. Materials are untextured,
 * with the cooked colour and surface properties; the builder sets textures and user data afterwards.
 * Returns the chunk's size, writing it when `out` holds that many bytes, or 0 with a reason (a joint
 * that is not a node of the hierarchy). */
size_t p4_model_geometry_stream(const uint8_t *blob, const P4ModelCookedMesh *mesh, const int32_t *node_ids,
                                uint32_t nodes, uint8_t *out, size_t capacity, char *error, size_t error_size);

/* The RtAnimAnimation chunk (0x1B) of cooked animation `animation` for the hierarchy whose nodes have the
 * HAnim ids `node_ids` in index order, as retail RMDs store it and RtAnimAnimationStreamRead reads it:
 * header (version 0x100, scheme 2, keyframe count, flags, duration), RtCompressedKeyFrames in RtAnim's order
 * (every node's first key, every node's second key, then each later key by the time of its node's
 * previous key, ties by node index; previous-keyframe offsets as in-memory byte offsets), then the
 * translation offset and scale. Returns the chunk's size, writing it when `out` holds that many bytes, or
 * 0 with a reason (a hierarchy node the animation has no track for). */
size_t p4_model_animation_stream(const uint8_t *blob, const P4ModelCookedAnimation *animation, const int32_t *node_ids,
                                 uint32_t nodes, uint8_t *out, size_t capacity, char *error, size_t error_size);

static inline const P4ModelCookedAnimation *p4_model_animation(const uint8_t *blob, uint32_t i)
{
    return (const P4ModelCookedAnimation *)(const void *)(blob + p4_model_header(blob)->animationsOffset) + i;
}

/* Cache: `directory` NULL disables it. Returns 0 with the cooked blob (malloc'd) for the inputs whose
 * hash is `source` when a valid cached file exists, else -1. */
int p4_model_cache_load(const char *directory, const char *source, uint8_t **out, size_t *out_size);
/* Writes the blob atomically as <directory>/<source>.p4mc. 0, or -1 (the model still loads). */
int p4_model_cache_store(const char *directory, const uint8_t *blob, size_t size);
/* The hash a cook of these inputs would record (cooker id + glTF bytes + every external file the glTF
 * names, in document order). 0, or -1 with a reason when an external file cannot be read. */
int p4_model_source_hash(const char *name, const uint8_t *gltf, size_t size, P4ModelReadFn read, void *user,
                         char hex[68], char *error, size_t error_size);
#endif
