/* Runtime glTF models: cooked model (src/platform/models/p4model.h) -> RenderWare objects in a loaded
 * clump. This is the one adapter between the model override service and RenderWare: every RW call and
 * every retail struct offset the service uses is here, so the RenderWare layer can change under it
 * (PortRWClean keeps the RW 3.7 names, signatures and retail layouts this file relies on).
 *
 * mdlReadAuxChunks (overrides/0047b0c0.c) calls p4_model_override_apply right after it read the clump
 * of a model whose file has an override, before any later chunk binds to the clump's materials. The
 * clump keeps its frames, HAnim hierarchy and every Atlus chunk of the file (animation slots, attach
 * table, effects); its atomics are replaced by atomics built from the cooked meshes, on the frames
 * they name (docs/modding/models.md, "Runtime glTF models"). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/gmem.h"
#include "p4pc/guest.h"
#include "p4pc/mod_models.h"
#include "../files/vfs.h"
#include "p4model.h"

/* Retail layouts (decomp/include/rw/sky2/rwcore.h, rpworld.h; the same offsets src/render/p4gl_atomic.c reads). */
#define MODEL_AUXILIARY 0x30C        /* Model.auxiliary (MdlFileAuxiliary / MdlLoaderState) */
#define AUX_DATA 0x2C                /* MdlFileAuxiliary.data: the RMD bytes (a pack's entry 0 payload) */
#define AUX_FILE 0x38                /* MdlFileAuxiliary.file: the HCdvd request, NULL for memory models */
#define AUX_ARCHIVED 0x40            /* MdlFileAuxiliary.archived */
#define HCDVD_PATH 0x10              /* HCdvd.path[256] */
#define OBJECT_PARENT 4              /* RwObject.parent: a clump's root frame, an atomic's frame */
#define ATOMIC_GEOMETRY 24           /* RpAtomic.geometry */
#define GEOM_MATERIALS 32            /* RpGeometry.matList.materials */
#define GEOM_TRIANGLES 44            /* RpGeometry.triangles */
#define GEOM_PRELIT 48               /* RpGeometry.preLitLum */
#define GEOM_TEXCOORDS 52            /* RpGeometry.texCoords[8] */
#define GEOM_MORPHTARGET 92          /* RpGeometry.morphTarget */
#define MORPH_SPHERE 4               /* RpMorphTarget.boundingSphere: RpMorphTargetSetBoundingSphere is this store (rpworld.h macro) */
#define MORPH_VERTS 20               /* RpMorphTarget.verts */
#define MORPH_NORMALS 24             /* RpMorphTarget.normals */
#define MATERIAL_COLOR 4             /* RpMaterial.color */
#define MATERIAL_SURFACE 12          /* RpMaterial.surfaceProps: ambient, specular, diffuse */
#define TEXTURE_FILTER 0x50          /* RwTexture.filterAddressing */
#define RASTER_STRIDE 24             /* RwRaster.stride */
#define USERDATA_DATA 12             /* RpUserDataArray.data */
#define TRIANGLE_BYTES 8             /* RpTriangle: vertIndex[3], matIndex */
#define RASTER_TEXTURE 0x04u         /* rwRASTERTYPETEXTURE */
#define RASTER_8888 0x0500u          /* rwRASTERFORMAT8888 */
#define RASTER_PAL8 0x2000u          /* rwRASTERFORMATPAL8 */
#define RASTER_LOCK_WRITE 0x01       /* rwRASTERLOCKWRITE */
#define USERDATA_INT 1               /* rpINTUSERDATA */
#define USERDATA_STRING 3            /* rpSTRINGUSERDATA */
#define HIERARCHY_NODES 4            /* RpHAnimHierarchy.numNodes */
#define HIERARCHY_NODE_INFO 16       /* RpHAnimHierarchy.pNodeInfo: RpHAnimNodeInfo[numNodes] */
#define NODE_INFO_BYTES 16           /* RpHAnimNodeInfo: nodeID, nodeIndex, flags, pFrame */
#define STREAM_MEMORY 3              /* rwSTREAMMEMORY */
#define STREAM_READ 1                /* rwSTREAMREAD */
#define CHUNK_HEADER 12              /* a chunk header: type, size, library stamp */
#define SKIN_TYPE_GENERIC 1          /* rpSKINTYPEGENERIC */

/* RenderWare 3.7 API, compiled from decomp/src/renderware and decomp/src/rw (pointer parameters are
 * host pointers). Names #defined to an address in the SDK sources are called by that address. */
extern void *func_003c2630(int32_t numVertices, int32_t numTriangles, uint32_t format); /* RpGeometryCreate */
extern void *RwStreamOpen(int32_t type, int32_t access, const void *memory);
extern int32_t RwStreamClose(void *stream, void *data);
extern void *RpGeometryStreamRead(void *stream);
extern uint8_t *func_003d53c0(int32_t stream);                                          /* RtAnimAnimationStreamRead (guest stream) */
/* RenderWare skin and HAnim plugins (decomp/src/rw); those taking or returning s32 work on guest addresses. */
extern uint32_t RpHAnimFrameGetHierarchy(int32_t frame);
extern int32_t func_003b83d0(int32_t atomic, int32_t hierarchy);                        /* RpSkinAtomicSetHAnimHierarchy */
extern uint32_t func_003b83f0(int32_t geometry);                                       /* RpSkinGeometryGetSkin */
extern void func_003b8520(uint8_t *atomic, int32_t type);                               /* RpSkinAtomicSetType */
extern void *func_003c22f0(void *geometry);                                            /* RpGeometryUnlock */
extern int32_t func_003c2a80(void *geometry);                                          /* RpGeometryDestroy */
extern const void *RpGeometryTriangleSetVertexIndices(const void *geometry, void *triangle, uint16_t v0, uint16_t v1, uint16_t v2);
extern void *RpGeometryTriangleSetMaterial(void *geometry, void *triangle, void *material);
extern const void *RpMorphTargetCalcBoundingSphere(const void *morphTarget, float sphere[4]);
extern void *RpMaterialCreate(void);
extern int32_t RpMaterialDestroy(void *material);
extern void *RpMaterialSetTexture(void *material, void *texture);
extern void *func_003c00e0(void);                                                      /* RpAtomicCreate */
extern void *func_003c1b90(void *atomic, void *frame);                                 /* RpAtomicSetFrame */
extern void *func_003c0210(void *atomic, void *geometry, uint32_t flags);              /* RpAtomicSetGeometry */
extern int32_t func_003c02e0(void *atomic);                                            /* RpAtomicDestroy */
extern void *RpClumpAddAtomic(void *clump, void *atomic);
extern void *RpClumpRemoveAtomic(void *clump, void *atomic);
extern void *RpClumpForAllAtomics(void *clump, void *(*callback)(void *atomic, void *data), void *data);
extern void *func_003e9af0(void *frame, void *(*callback)(void *frame, void *data), void *data); /* RwFrameForAllChildren */
extern int32_t func_00397470(uint8_t *frame);                                          /* the frame's HAnim node id (-1: none) */
extern void *K_Clump_FindIntUsrDataFrame(void *clump, const char *name, int32_t value);
extern void *RwRasterCreate(int32_t width, int32_t height, int32_t depth, uint32_t flags);
extern uint8_t *RwRasterLock(void *raster, uint8_t level, int32_t lockMode);
extern void *RwRasterUnlock(void *raster);
extern uint8_t *RwRasterLockPalette(void *raster, int32_t lockMode);
extern void *RwRasterUnlockPalette(void *raster);
extern void func_003ec330(void *raster);                                               /* RwRasterDestroy */
extern void *func_003ef2e0(void *raster);                                              /* RwTextureCreate */
extern void func_003ef470(void *texture, const void *name);                            /* RwTextureSetName */
extern int32_t func_003ef3a0(void *texture);                                           /* RwTextureDestroy */
extern int32_t func_003bcfa0(void *material, char *name, int32_t format, int32_t numElements); /* RpMaterialAddUserDataArray */
extern void *func_003bd000(void *material, int32_t index);                             /* RpMaterialGetUserDataArray */
extern void func_003bd0d0(void *array, int32_t index, int32_t value);                  /* RpUserDataArraySetInt */

/* The RenderWare allocator slot (0x008873E8[0]): RwMalloc(size, hint). */
typedef void *(*P4ModelAllocFn)(int size, unsigned int hint);

static const void *ptr_at(const void *base, size_t offset)
{
    uint32_t v;
    memcpy(&v, (const uint8_t *)base + offset, sizeof v);
    return P4_G2H(v);
}

static void *rw_malloc(size_t size)
{
    P4ModelAllocFn alloc = (P4ModelAllocFn)p4_fn_g2h_sig(P4_G32(0x008873E8u), P4_SIG(2, 1, 0, 0, 0, 0, 0, 0));
    return alloc((int)size, 0x40000u);
}

/* The RenderWare free slot (0x008873EC[0]): RwFree(pointer). */
static void rw_free(void *p)
{
    ((void (*)(void *))p4_fn_g2h_sig(P4_G32(0x008873ECu), P4_SIG(1, 0, 1, 0, 0, 0, 0, 0)))(p);
}

/* ------------------------------------------------------------------------------------------------
 * Which override a model's file has */

static char upper(char c)
{
    return c >= 'a' && c <= 'z' ? (char)(c - 'a' + 'A') : c;
}

/* The game path as the VFS keys it (src/platform/files/vfs.c normalise): no device, '/', upper case,
 * no version suffix; an unqualified key resolves to the first container holding it. */
int p4_model_disc_key(const char *path, char *out, size_t capacity)
{
    const char *colon = strchr(path, ':');
    if (colon) path = colon + 1;
    size_t n = 0;
    for (const char *p = path; *p && *p != ';'; ++p) {
        char c = *p == '\\' ? '/' : *p;
        if (c == '/' && (n == 0 || out[n - 1] == '/')) continue;
        if (n + 1 >= capacity) return -1;
        out[n++] = upper(c);
    }
    while (n && out[n - 1] == '/') --n;
    out[n] = 0;
    if (!n) return -1;
    static const char *const labels[] = {"DATA/", "BTL/", "BGM/", "ENV/", "ZZZZZ/"};
    for (size_t i = 0; i < sizeof labels / sizeof *labels; ++i)
        if (!strncmp(out, labels[i], strlen(labels[i]))) return 0;
    for (size_t i = 0; i < sizeof labels / sizeof *labels; ++i) {
        char qualified[512];
        int is_dir = 0;
        long long size = 0;
        snprintf(qualified, sizeof qualified, "%s%s", labels[i], out);
        if (vfs_stat(qualified, &is_dir, &size) == 0 && !is_dir) {
            if (strlen(qualified) >= capacity) return -1;
            strcpy(out, qualified);
            return 0;
        }
    }
    return 0;
}

const P4ModelOverride *p4_model_override_for_model(const void *model)
{
    if (!p4_model_overrides_active() || !model) return NULL;
    const uint8_t *aux = ptr_at(model, MODEL_AUXILIARY);
    if (!aux) return NULL;
    const uint8_t *file = ptr_at(aux, AUX_FILE);
    if (!file) return NULL; /* created from memory: no disc key */
    char path[257];
    memcpy(path, file + HCDVD_PATH, 256);
    path[256] = 0;
    char key[600];
    if (p4_model_disc_key(path, key, 512)) return NULL;
    int32_t archived;
    memcpy(&archived, aux + AUX_ARCHIVED, sizeof archived);
    if (archived) {
        /* A pack's entry 0 is the RMD: its 252-byte name sits 256 bytes before the payload. */
        const uint8_t *data = ptr_at(aux, AUX_DATA);
        if (!data) return NULL;
        char member[253];
        memcpy(member, data - 256, 252);
        member[252] = 0;
        size_t used = strlen(key);
        snprintf(key + used, sizeof key - used, "#%s", member);
    }
    return p4_model_override_find(key);
}

/* ------------------------------------------------------------------------------------------------
 * Frames */

typedef struct FrameSearch {
    int32_t id;
    void *found;
} FrameSearch;

static void *find_hanim(void *frame, void *data)
{
    FrameSearch *search = data;
    if (search->found) return NULL;
    if (func_00397470(frame) == search->id) {
        search->found = frame;
        return NULL;
    }
    func_003e9af0(frame, find_hanim, data);
    return search->found ? NULL : frame;
}

/* The replaced model's HAnim hierarchy: the first frame, root first, whose HAnim extension holds one. */
static void *hierarchy_of(void *frame)
{
    uint32_t hierarchy = RpHAnimFrameGetHierarchy((int32_t)P4_H2G(frame));
    return hierarchy ? P4_G2H(hierarchy) : NULL;
}

static void *find_hierarchy(void *frame, void *data)
{
    void **found = data;
    if (*found) return NULL;
    if ((*found = hierarchy_of(frame)) != NULL) return NULL;
    func_003e9af0(frame, find_hierarchy, data);
    return *found ? NULL : frame;
}

/* `skin_frame`: the frame of the replaced model's first skinned atomic, for skinned meshes. */
static void *mesh_frame(void *clump, const P4ModelCookedMesh *mesh, void *skin_frame)
{
    void *root = (void *)ptr_at(clump, OBJECT_PARENT);
    if (!root) return NULL;
    if (mesh->frameKind == P4_MODEL_FRAME_ROOT) return root;
    if (mesh->frameKind == P4_MODEL_FRAME_SKIN) return skin_frame ? skin_frame : root;
    if (mesh->frameKind == P4_MODEL_FRAME_HELPER) return K_Clump_FindIntUsrDataFrame(clump, "per3HelperId", mesh->frameId);
    FrameSearch search = {mesh->frameId, NULL};
    if (func_00397470(root) == mesh->frameId) return root;
    func_003e9af0(root, find_hanim, &search);
    return search.found;
}

/* ------------------------------------------------------------------------------------------------
 * Textures, materials, geometry */

static void *build_texture(const uint8_t *blob, const P4ModelCookedTexture *t)
{
    const int paletted = t->depth == 8;
    void *raster = RwRasterCreate((int32_t)t->width, (int32_t)t->height, (int32_t)t->depth,
                                  RASTER_TEXTURE | RASTER_8888 | (paletted ? RASTER_PAL8 : 0u));
    if (!raster) return NULL;
    uint8_t *pixels = RwRasterLock(raster, 0, RASTER_LOCK_WRITE);
    if (!pixels) {
        func_003ec330(raster);
        return NULL;
    }
    int32_t stride;
    memcpy(&stride, (const uint8_t *)raster + RASTER_STRIDE, sizeof stride);
    const size_t row = (size_t)t->width * (paletted ? 1u : 4u);
    if (stride < (int32_t)row) stride = (int32_t)row;
    for (uint32_t y = 0; y < t->height; ++y)
        memcpy(pixels + (size_t)y * (size_t)stride, blob + t->pixelsOffset + (size_t)y * row, row);
    int ok = 1;
    if (paletted) {
        /* A raster palette is the CLUT in GS memory order with PS2 alpha (0x80 opaque): entry j holds logical
         * entry csm1(j), bits 3 and 4 swapped (src/render/p4gl_native.c, p4gl_raster.c paletteEntry). */
        uint8_t *palette = RwRasterLockPalette(raster, RASTER_LOCK_WRITE);
        if (!palette) {
            ok = 0;
        } else {
            const uint8_t *logical = blob + t->paletteOffset;
            for (uint32_t j = 0; j < 256; ++j) {
                uint32_t src = (j & ~0x18u) | ((j & 8u) << 1) | ((j & 16u) >> 1);
                palette[4 * j] = logical[4 * src];
                palette[4 * j + 1] = logical[4 * src + 1];
                palette[4 * j + 2] = logical[4 * src + 2];
                palette[4 * j + 3] = (uint8_t)((logical[4 * src + 3] + 1u) / 2u);
            }
            RwRasterUnlockPalette(raster);
        }
    }
    RwRasterUnlock(raster);
    if (!ok) {
        func_003ec330(raster);
        return NULL;
    }
    void *texture = func_003ef2e0(raster);
    if (!texture) {
        func_003ec330(raster);
        return NULL;
    }
    func_003ef470(texture, t->name);
    memcpy((uint8_t *)texture + TEXTURE_FILTER, &t->filterAddressing, sizeof t->filterAddressing);
    return texture;
}

static int user_int(void *material, const char *name, int32_t value)
{
    char key[64];
    snprintf(key, sizeof key, "%s", name);
    int32_t index = func_003bcfa0(material, key, USERDATA_INT, 1);
    void *array = index >= 0 ? func_003bd000(material, index) : NULL;
    if (!array) return -1;
    func_003bd0d0(array, 0, value);
    return 0;
}

static int user_string(void *material, const char *name, const char *value)
{
    char key[64];
    snprintf(key, sizeof key, "%s", name);
    int32_t index = func_003bcfa0(material, key, USERDATA_STRING, 1);
    void *array = index >= 0 ? func_003bd000(material, index) : NULL;
    if (!array) return -1;
    const void *data = ptr_at(array, USERDATA_DATA);
    char *copy = rw_malloc(strlen(value) + 1);
    if (!data || !copy) return -1;
    strcpy(copy, value);
    uint32_t guest = P4_H2G(copy);
    memcpy((void *)data, &guest, sizeof guest);
    return 0;
}

/* A material's state from its cooked record: colour, surface properties, texture and the user data P4
 * reads from retail materials (docs/modding/models.md, "P4 user data conventions"). 0, or -1. */
static int setup_material(void *material, const P4ModelCookedMaterial *m, void *const *textures)
{
    memcpy((uint8_t *)material + MATERIAL_COLOR, m->color, 4);
    float surface[3] = {m->ambient, m->specular, m->diffuse};
    memcpy((uint8_t *)material + MATERIAL_SURFACE, surface, sizeof surface);
    if (m->texture >= 0 && textures[m->texture]) RpMaterialSetTexture(material, textures[m->texture]);
    int failed = user_string(material, "name", m->name[0] ? m->name : "default");
    if (m->drawCategory >= 0) failed |= user_int(material, "per3MatAlpha", m->drawCategory);
    if (m->flags & P4_MODEL_MATERIAL_FOG_DISABLE) failed |= user_int(material, "per3FogDisable", 1);
    if (m->flags & P4_MODEL_MATERIAL_TINT) failed |= user_int(material, "per3modelMatColor", (int32_t)m->tint);
    return failed ? -1 : 0;
}

static void *build_material(const P4ModelCookedMaterial *m, void *const *textures)
{
    void *material = RpMaterialCreate();
    if (!material) return NULL;
    if (setup_material(material, m, textures)) {
        RpMaterialDestroy(material);
        return NULL;
    }
    return material;
}

static void *build_geometry(const uint8_t *blob, const P4ModelCookedMesh *mesh, void *const *materials)
{
    void *geometry = func_003c2630((int32_t)mesh->vertices, (int32_t)mesh->triangles, mesh->format);
    if (!geometry) return NULL;
    void *morph = (void *)ptr_at(geometry, GEOM_MORPHTARGET);
    void *verts = morph ? (void *)ptr_at(morph, MORPH_VERTS) : NULL;
    uint8_t *triangles = (uint8_t *)ptr_at(geometry, GEOM_TRIANGLES);
    if (!verts || !triangles) {
        func_003c2a80(geometry);
        return NULL;
    }
    memcpy(verts, blob + mesh->positionsOffset, (size_t)mesh->vertices * 12);
    if (mesh->normalsOffset) {
        void *normals = (void *)ptr_at(morph, MORPH_NORMALS);
        if (normals) memcpy(normals, blob + mesh->normalsOffset, (size_t)mesh->vertices * 12);
    }
    for (uint32_t set = 0; set < mesh->uvSets; ++set) {
        void *uv = (void *)ptr_at(geometry, GEOM_TEXCOORDS + 4u * set);
        if (uv) memcpy(uv, blob + (set ? mesh->uv1Offset : mesh->uv0Offset), (size_t)mesh->vertices * 8);
    }
    if (mesh->colorsOffset) {
        void *prelit = (void *)ptr_at(geometry, GEOM_PRELIT);
        if (prelit) memcpy(prelit, blob + mesh->colorsOffset, (size_t)mesh->vertices * 4);
    }
    const uint16_t *src = (const uint16_t *)(const void *)(blob + mesh->trianglesOffset);
    const uint32_t *slots = (const uint32_t *)(const void *)(blob + mesh->materialsOffset);
    for (uint32_t t = 0; t < mesh->triangles; ++t, src += 4) {
        void *triangle = triangles + (size_t)t * TRIANGLE_BYTES;
        RpGeometryTriangleSetVertexIndices(geometry, triangle, src[0], src[1], src[2]);
        RpGeometryTriangleSetMaterial(geometry, triangle, materials[slots[src[3]]]);
    }
    float sphere[4];
    RpMorphTargetCalcBoundingSphere(morph, sphere);
    memcpy((uint8_t *)morph + MORPH_SPHERE, sphere, sizeof sphere);
    func_003c22f0(geometry);
    return geometry;
}

/* The HAnim node id of each node of `hierarchy`, in index order. Returns the node count, or 0. */
static uint32_t hierarchy_ids(void *hierarchy, int32_t ids[255])
{
    int32_t nodes;
    memcpy(&nodes, (const uint8_t *)hierarchy + HIERARCHY_NODES, sizeof nodes);
    const uint8_t *info = ptr_at(hierarchy, HIERARCHY_NODE_INFO);
    if (nodes <= 0 || nodes > 255 || !info) return 0;
    for (int32_t i = 0; i < nodes; ++i) memcpy(&ids[i], info + (size_t)i * NODE_INFO_BYTES, sizeof ids[i]);
    return (uint32_t)nodes;
}

/* The replaced model's HAnim hierarchy (the first frame, root first, holding one) and its node ids.
 * Returns the node count, or 0 without one. */
static uint32_t clump_hierarchy(void *clump, void **hierarchy, int32_t ids[255])
{
    void *root = (void *)ptr_at(clump, OBJECT_PARENT);
    *hierarchy = NULL;
    if (root && !(*hierarchy = hierarchy_of(root))) func_003e9af0(root, find_hierarchy, hierarchy);
    return *hierarchy ? hierarchy_ids(*hierarchy, ids) : 0;
}

/* Reads the chunk at `bytes` (`size` bytes with its header) with `reader` from a memory stream that starts
 * inside the chunk, as RenderWare's readers start after the chunk header their caller found. */
static void *read_chunk(const uint8_t *bytes, size_t size, void *(*reader)(void *stream))
{
    struct {
        uint32_t start, length; /* RwMemory: a guest address and a length */
    } memory = {P4_H2G(bytes + CHUNK_HEADER), (uint32_t)(size - CHUNK_HEADER)};
    void *object = NULL;
    void *stream = RwStreamOpen(STREAM_MEMORY, STREAM_READ, &memory);
    if (stream) {
        object = reader(stream);
        RwStreamClose(stream, &memory);
    }
    return object;
}

/* A skinned mesh is built the way retail skinned geometry is: RpGeometryStreamRead reads its geometry
 * chunk (p4model_stream.c) from a memory stream, and the skin plugin's reader builds the RpSkin. Its
 * materials come from the stream with their colour and surface properties; textures and user data
 * follow. */
static void *build_skinned_geometry(const uint8_t *blob, const P4ModelCookedMesh *mesh, const int32_t *ids,
                                    uint32_t nodes, void *const *textures)
{
    size_t size = p4_model_geometry_stream(blob, mesh, ids, nodes, NULL, 0, NULL, 0);
    uint8_t *bytes = size ? rw_malloc(size) : NULL;
    if (!bytes) return NULL;
    p4_model_geometry_stream(blob, mesh, ids, nodes, bytes, size, NULL, 0);
    /* RpGeometryStreamRead starts inside the geometry chunk, as when the clump reader calls it after
     * finding the chunk's header. */
    void *geometry = read_chunk(bytes, size, RpGeometryStreamRead);
    rw_free(bytes);
    if (!geometry) return NULL;
    const uint32_t *records = (const uint32_t *)(const void *)(blob + mesh->materialsOffset);
    const uint8_t *list = ptr_at(geometry, GEOM_MATERIALS);
    for (uint32_t i = 0; i < mesh->materialCount; ++i) {
        void *material = list ? (void *)ptr_at(list, (size_t)i * 4u) : NULL;
        if (!material || setup_material(material, p4_model_material(blob, records[i]), textures)) {
            func_003c2a80(geometry);
            return NULL;
        }
    }
    return geometry;
}

/* ------------------------------------------------------------------------------------------------ */

typedef struct AtomicList {
    void **items;
    size_t count, capacity;
} AtomicList;

static void *collect_atomic(void *atomic, void *data)
{
    AtomicList *list = data;
    if (list->count == list->capacity) {
        size_t next = list->capacity ? list->capacity * 2 : 16;
        void **grown = realloc(list->items, next * sizeof *grown);
        if (!grown) return NULL;
        list->items = grown;
        list->capacity = next;
    }
    list->items[list->count++] = atomic;
    return atomic;
}

int p4_model_override_apply(const P4ModelOverride *override, void *clump)
{
    size_t size = 0;
    const uint8_t *blob = p4_model_override_blob(override, &size);
    const char *name = p4_model_override_name(override);
    /* Checked when the table took it (the boot handler's cook or cache load, or the viewer's cook). */
    if (!blob || !clump || !size) return -1;
    const P4ModelCookedHeader *h = p4_model_header(blob);
    /* Resolve every frame and skin first: a model that cannot be placed keeps its retail look. */
    void **frames = calloc(h->meshes, sizeof *frames);
    void **textures = calloc(h->textures ? h->textures : 1, sizeof *textures);
    void **materials = calloc(h->materials, sizeof *materials);
    AtomicList retail = {NULL, 0, 0};
    void *hierarchy = NULL, *skin_frame = NULL;
    int32_t ids[255];
    uint32_t nodes = 0, skinned = 0;
    int result = -1;
    if (!frames || !textures || !materials) goto done;
    RpClumpForAllAtomics(clump, collect_atomic, &retail);
    for (size_t i = 0; i < retail.count && !skin_frame; ++i) {
        const void *geometry = ptr_at(retail.items[i], ATOMIC_GEOMETRY);
        if (geometry && func_003b83f0((int32_t)P4_H2G(geometry))) skin_frame = (void *)ptr_at(retail.items[i], OBJECT_PARENT);
    }
    for (uint32_t i = 0; i < h->meshes; ++i) {
        const P4ModelCookedMesh *mesh = p4_model_mesh(blob, i);
        frames[i] = mesh_frame(clump, mesh, skin_frame);
        if (!frames[i]) {
            fprintf(stderr, "p4pc: resource-model %s: the replaced model has no %s %d frame; keeping its retail look\n", name,
                    mesh->frameKind == P4_MODEL_FRAME_HANIM ? "HAnim bone" : "per3HelperId", (int)mesh->frameId);
            goto done;
        }
        if (!mesh->skinOffset) continue;
        if (!nodes && !(nodes = clump_hierarchy(clump, &hierarchy, ids))) {
            fprintf(stderr, "p4pc: resource-model %s: the replaced model has no HAnim hierarchy for skinned mesh %u; "
                    "keeping its retail look\n", name, (unsigned)i);
            goto done;
        }
        char error[160];
        if (!p4_model_geometry_stream(blob, mesh, ids, nodes, NULL, 0, error, sizeof error)) {
            fprintf(stderr, "p4pc: resource-model %s: skinned mesh %u: %s; keeping its retail look\n", name, (unsigned)i, error);
            goto done;
        }
    }
    for (uint32_t i = 0; i < h->textures; ++i)
        if (!(textures[i] = build_texture(blob, p4_model_texture(blob, i)))) {
            fprintf(stderr, "p4pc: resource-model %s: cannot create texture %u\n", name, (unsigned)i);
            goto done;
        }
    for (uint32_t i = 0; i < h->materials; ++i)
        if (!(materials[i] = build_material(p4_model_material(blob, i), textures))) {
            fprintf(stderr, "p4pc: resource-model %s: cannot create material %u\n", name, (unsigned)i);
            goto done;
        }
    for (uint32_t i = 0; i < h->meshes; ++i) {
        const P4ModelCookedMesh *mesh = p4_model_mesh(blob, i);
        void *geometry = mesh->skinOffset ? build_skinned_geometry(blob, mesh, ids, nodes, textures)
                                          : build_geometry(blob, mesh, materials);
        void *atomic = geometry ? func_003c00e0() : NULL;
        if (!atomic) {
            if (geometry) func_003c2a80(geometry);
            fprintf(stderr, "p4pc: resource-model %s: cannot create mesh %u\n", name, (unsigned)i);
            /* Atomics added so far stay: the clump is consistent, only incomplete. */
            break;
        }
        func_003c0210(atomic, geometry, 0);
        func_003c2a80(geometry); /* the atomic holds the geometry now */
        func_003c1b90(atomic, frames[i]);
        if (mesh->skinOffset) {
            /* What the skin plugin's atomic stream callbacks do for a retail skinned atomic; the game attaches
             * the hierarchy to every atomic of the clump again when it sets the model up. */
            func_003b8520(atomic, SKIN_TYPE_GENERIC);
            func_003b83d0((int32_t)P4_H2G(atomic), (int32_t)P4_H2G(hierarchy));
            ++skinned;
        }
        RpClumpAddAtomic(clump, atomic);
    }
    for (size_t i = 0; i < retail.count; ++i) {
        RpClumpRemoveAtomic(clump, retail.items[i]);
        func_003c02e0(retail.items[i]);
    }
    fprintf(stderr, "p4pc: resource-model %s (%s) applied: %u meshes (%u skinned), %u triangles, %u textures replace %zu atomics\n",
            name, p4_model_override_provider(override), (unsigned)h->meshes, (unsigned)skinned, (unsigned)h->triangles,
            (unsigned)h->textures, retail.count);
    result = 0;
done:
    /* Drop the creation references: geometries hold their materials, materials their textures. */
    for (uint32_t i = 0; materials && i < h->materials; ++i)
        if (materials[i]) RpMaterialDestroy(materials[i]);
    for (uint32_t i = 0; textures && i < h->textures; ++i)
        if (textures[i]) func_003ef3a0(textures[i]);
    free(retail.items);
    free(materials);
    free(textures);
    free(frames);
    return result;
}

static void *read_animation(void *stream)
{
    return func_003d53c0((int32_t)P4_H2G(stream));
}

/* A slot animation is built the way retail ones are: RtAnimAnimationStreamRead reads its RtAnimAnimation
 * chunk (p4model_stream.c), here from a memory stream instead of the RMD. */
void *p4_model_override_animation(const P4ModelOverride *override, void *clump, uint32_t layer, uint32_t slot)
{
    size_t size = 0;
    const uint8_t *blob = p4_model_override_blob(override, &size);
    if (!blob || !size || !clump || p4_model_override_schema(override) < 3) return NULL;
    const P4ModelCookedAnimation *animation = NULL;
    for (uint32_t i = 0; i < p4_model_header(blob)->animations && !animation; ++i) {
        const P4ModelCookedAnimation *a = p4_model_animation(blob, i);
        if (a->layer == layer && a->slot == slot) animation = a;
    }
    if (!animation) return NULL;
    void *hierarchy;
    int32_t ids[255];
    char error[160] = "the replaced model has no HAnim hierarchy";
    const uint32_t nodes = clump_hierarchy(clump, &hierarchy, ids);
    const size_t bytes_size = nodes ? p4_model_animation_stream(blob, animation, ids, nodes, NULL, 0, error, sizeof error) : 0;
    uint8_t *bytes = bytes_size ? rw_malloc(bytes_size) : NULL;
    void *result = NULL;
    if (bytes) {
        p4_model_animation_stream(blob, animation, ids, nodes, bytes, bytes_size, NULL, 0);
        result = read_chunk(bytes, bytes_size, read_animation);
        rw_free(bytes);
        snprintf(error, sizeof error, "RtAnimAnimationStreamRead refused it");
    } else if (bytes_size) {
        snprintf(error, sizeof error, "out of memory");
    }
    if (result)
        fprintf(stderr, "p4pc: resource-model %s: animation L%uS%02u replaces the file's (%u nodes, %.4g s)\n",
                p4_model_override_name(override), (unsigned)layer, (unsigned)slot, (unsigned)nodes, (double)animation->duration);
    else
        fprintf(stderr, "p4pc: resource-model %s: animation L%uS%02u: %s; the slot keeps the file's animation\n",
                p4_model_override_name(override), (unsigned)layer, (unsigned)slot, error);
    return result;
}

/* ------------------------------------------------------------------------------------------------
 * p4pc_viewer's P4PC_VIEWER_DUMP seam: the loaded clump's atomics as RenderWare holds them in memory
 * (frame HAnim id, geometry flags, vertices, normals, UV set 0, prelit colours, triangles, materials),
 * for comparing a glTF override with the retail model it replaces (tests/models_roundtrip_test.py). */

#define ATOMIC_FRAME 4               /* RpAtomic.object.object.parent */
#define GEOM_FLAGS 8                 /* RpGeometry.flags */
#define GEOM_NUM_TRIANGLES 16        /* RpGeometry.numTriangles */
#define GEOM_NUM_VERTICES 20         /* RpGeometry.numVertices */
#define GEOM_NUM_MATERIALS 36        /* RpGeometry.matList.numMaterials */
#define MATERIAL_TEXTURE 0           /* RpMaterial.texture */
#define TEXTURE_RASTER 0             /* RwTexture.raster */
#define TEXTURE_NAME 0x10            /* RwTexture.name[32] */
#define RASTER_SIZE 12               /* RwRaster.width, height, depth */
#define SKIN_BONES 0                 /* RpSkin.numBones */
#define SKIN_USED 4                  /* RpSkin.numBoneIds */
#define SKIN_USED_IDS 8              /* RpSkin.boneIndices: the used bones (u8) */
#define SKIN_MATRICES 0x0C           /* RpSkin.skinToBoneMatrices */
#define SKIN_MAX_WEIGHTS 0x10        /* RpSkin.maxNumWeightsForVertex */
#define SKIN_INDICES 0x14            /* RpSkin.vertexBoneIndices: 4 bytes per vertex */
#define SKIN_WEIGHTS 0x18            /* RpSkin.vertexBoneWeights: 4 floats per vertex */
#define SKIN_SPLIT 0x2C              /* RpSkin split data: bone limit, mesh count, RLE count (sky2 layout) */
#define SKIN_ATOMIC_PLUGIN 0x00886494u /* the skin plugin's atomic offset: the atomic's hierarchy */

static uint32_t u32_at(const void *base, size_t offset)
{
    uint32_t v;
    memcpy(&v, (const uint8_t *)base + offset, sizeof v);
    return v;
}

static void dump_floats(FILE *f, const void *base, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        float v;
        memcpy(&v, (const uint8_t *)base + (size_t)i * 4u, sizeof v);
        fprintf(f, "%s%.9g", i ? "," : "", (double)v);
    }
}

typedef struct DumpState {
    FILE *file;
    int first;
} DumpState;

static void *dump_atomic(void *atomic, void *data)
{
    DumpState *state = data;
    FILE *f = state->file;
    const uint8_t *frame = ptr_at(atomic, ATOMIC_FRAME);
    const uint8_t *geo = ptr_at(atomic, ATOMIC_GEOMETRY);
    if (!geo) return atomic;
    uint32_t triangles = u32_at(geo, GEOM_NUM_TRIANGLES), vertices = u32_at(geo, GEOM_NUM_VERTICES);
    uint32_t material_count = u32_at(geo, GEOM_NUM_MATERIALS);
    const uint8_t *morph = ptr_at(geo, GEOM_MORPHTARGET);
    fprintf(f, "%s{\"hanim\":%d,\"flags\":%u,\"vertices\":[", state->first ? "" : ",\n",
            frame ? (int)func_00397470((uint8_t *)frame) : -2, (unsigned)u32_at(geo, GEOM_FLAGS));
    state->first = 0;
    if (morph && ptr_at(morph, MORPH_VERTS)) dump_floats(f, ptr_at(morph, MORPH_VERTS), vertices * 3u);
    fprintf(f, "],\"normals\":[");
    if (morph && ptr_at(morph, MORPH_NORMALS)) dump_floats(f, ptr_at(morph, MORPH_NORMALS), vertices * 3u);
    fprintf(f, "],\"uv\":[");
    if (ptr_at(geo, GEOM_TEXCOORDS)) dump_floats(f, ptr_at(geo, GEOM_TEXCOORDS), vertices * 2u);
    fprintf(f, "],\"prelit\":[");
    const uint8_t *prelit = ptr_at(geo, GEOM_PRELIT);
    for (uint32_t i = 0; prelit && i < vertices * 4u; i++) fprintf(f, "%s%u", i ? "," : "", (unsigned)prelit[i]);
    fprintf(f, "],\"triangles\":[");
    const uint8_t *tri = ptr_at(geo, GEOM_TRIANGLES);
    for (uint32_t i = 0; tri && i < triangles; i++) {
        uint16_t t[4];
        memcpy(t, tri + (size_t)i * TRIANGLE_BYTES, sizeof t);
        fprintf(f, "%s%u,%u,%u,%u", i ? "," : "", t[0], t[1], t[2], t[3]);
    }
    fprintf(f, "],\"materials\":[");
    const uint8_t *materials = ptr_at(geo, GEOM_MATERIALS);
    for (uint32_t i = 0; materials && i < material_count; i++) {
        const uint8_t *mat = ptr_at(materials, (size_t)i * 4u);
        const uint8_t *tex = mat ? ptr_at(mat, MATERIAL_TEXTURE) : NULL;
        const uint8_t *raster = tex ? ptr_at(tex, TEXTURE_RASTER) : NULL;
        int32_t size[3] = {0, 0, 0};
        if (raster) memcpy(size, raster + RASTER_SIZE, sizeof size);
        fprintf(f, "%s{\"color\":[%u,%u,%u,%u],\"texture\":\"%.31s\",\"raster\":[%d,%d,%d]}", i ? "," : "",
                mat ? mat[MATERIAL_COLOR] : 0, mat ? mat[MATERIAL_COLOR + 1] : 0, mat ? mat[MATERIAL_COLOR + 2] : 0,
                mat ? mat[MATERIAL_COLOR + 3] : 0, tex ? (const char *)tex + TEXTURE_NAME : "", (int)size[0], (int)size[1],
                (int)size[2]);
    }
    fprintf(f, "]");
    /* The skin as the skin plugin holds it (sky2 RpSkin): bone count, used bones, max weights, the native
     * init's per-weight-count vertex counters, the split data counts, then the arrays; the weights keep the
     * VU1 matrix offsets the native init writes into their low ten bits. Then the atomic's hierarchy. */
    uint32_t skin_guest = func_003b83f0((int32_t)P4_H2G(geo));
    if (skin_guest) {
        const uint8_t *skin = P4_G2H(skin_guest);
        uint32_t bones = u32_at(skin, SKIN_BONES), used = u32_at(skin, SKIN_USED);
        fprintf(f, ",\"skin\":{\"bones\":%u,\"maxWeights\":%u,\"counters\":[%u,%u,%u,%u],\"split\":[%u,%u,%u],\"used\":[",
                (unsigned)bones, (unsigned)u32_at(skin, SKIN_MAX_WEIGHTS), (unsigned)u32_at(skin, 0x1C), (unsigned)u32_at(skin, 0x20),
                (unsigned)u32_at(skin, 0x24), (unsigned)u32_at(skin, 0x28), (unsigned)u32_at(skin, SKIN_SPLIT),
                (unsigned)u32_at(skin, SKIN_SPLIT + 4), (unsigned)u32_at(skin, SKIN_SPLIT + 8));
        const uint8_t *ids = ptr_at(skin, SKIN_USED_IDS);
        for (uint32_t i = 0; ids && i < used; i++) fprintf(f, "%s%u", i ? "," : "", (unsigned)ids[i]);
        const char *arrays[3] = {"indices", "weights", "matrices"};
        const size_t offsets[3] = {SKIN_INDICES, SKIN_WEIGHTS, SKIN_MATRICES};
        const uint32_t counts[3] = {vertices, vertices * 4u, bones * 16u};
        for (int a = 0; a < 3; a++) {
            fprintf(f, "],\"%s\":[", arrays[a]);
            const uint8_t *words = ptr_at(skin, offsets[a]);
            for (uint32_t i = 0; words && i < counts[a]; i++) fprintf(f, "%s%u", i ? "," : "", (unsigned)u32_at(words, (size_t)i * 4u));
        }
        fprintf(f, "]}");
    }
    uint32_t offset = u32_at(P4_G2H(SKIN_ATOMIC_PLUGIN), 0);
    uint32_t hierarchy_guest = u32_at(atomic, offset);
    if (hierarchy_guest) {
        int32_t ids[255];
        uint32_t nodes = hierarchy_ids(P4_G2H(hierarchy_guest), ids);
        fprintf(f, ",\"hierarchy\":[");
        for (uint32_t i = 0; i < nodes; i++) fprintf(f, "%s%d", i ? "," : "", (int)ids[i]);
        fprintf(f, "]");
    }
    fprintf(f, "}");
    return atomic;
}

int p4_model_viewer_dump(void *clump)
{
    const char *path = getenv("P4PC_VIEWER_DUMP");
    if (!path || !*path) return 0;
    FILE *f = clump ? fopen(path, "w") : NULL;
    if (!f) {
        fprintf(stderr, "p4pc_viewer: cannot write %s\n", path);
        return -1;
    }
    DumpState state = {f, 1};
    fprintf(f, "{\"atomics\":[\n");
    RpClumpForAllAtomics(clump, dump_atomic, &state);
    fprintf(f, "\n]}\n");
    if (fclose(f)) {
        fprintf(stderr, "p4pc_viewer: cannot write %s\n", path);
        return -1;
    }
    return 0;
}
