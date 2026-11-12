/* RenderWare chunks the runtime builder feeds RenderWare's own stream readers: the geometry chunk of a
 * cooked skinned mesh (p4_model_geometry_stream) and the RtAnimAnimation chunk of a cooked slot animation
 * (p4_model_animation_stream, after the geometry writer below).
 *
 * Retail skinned RMD geometries are generic (not PS2-native) geometry chunks whose extension carries the
 * bin mesh (0x50E) and the skin (0x116); RpGeometryStreamRead and the skin plugin's stream reader build
 * the RpSkin from them. RenderWare 3.7 keeps no RpSkinCreate in P4's executable, so the runtime builder
 * (p4model_build.c) builds a skinned geometry the way retail does: from these bytes, through
 * RwStreamOpen(rwSTREAMMEMORY) and RpGeometryStreamRead. The layout follows bageomet.c
 * RpGeometryStreamRead, the material list reader, and the skin reader (decomp/src/rw/rpskin.c
 * FUN_003B7E00): one u32 of bone count, used-bone count and max weights, the used bones, four bone
 * indices and four weights per vertex, one skin-to-bone matrix per bone, then the split data (bone
 * limit, mesh count, RLE count; zero: not split). Pure C: no RenderWare or guest memory. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "p4model.h"

#define RW_VERSION 0x1C020037u /* the library stamp of P4's RMD chunks: RenderWare 3.7.0.2, build 0x37 */
enum {
    ID_STRUCT = 0x01, ID_EXTENSION = 0x03, ID_MATERIAL = 0x07, ID_MATLIST = 0x08, ID_GEOMETRY = 0x0F,
    ID_ANIMATION = 0x1B, ID_SKIN = 0x116, ID_BINMESH = 0x50E
};

typedef struct Out {
    uint8_t *p;
    size_t at, cap;
} Out;

static void put(Out *o, const void *bytes, size_t n)
{
    if (o->p && o->at + n <= o->cap) memcpy(o->p + o->at, bytes, n);
    o->at += n;
}

static void put32(Out *o, uint32_t v)
{
    uint8_t b[4] = {(uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24)};
    put(o, b, 4);
}

static void putf(Out *o, float f)
{
    uint32_t v;
    memcpy(&v, &f, 4);
    put32(o, v);
}

static size_t begin(Out *o, uint32_t id)
{
    put32(o, id);
    size_t at = o->at;
    put32(o, 0);
    put32(o, RW_VERSION);
    return at;
}

static void end(Out *o, size_t at)
{
    uint32_t size = (uint32_t)(o->at - at - 8);
    if (o->p && at + 4 <= o->cap) {
        Out patch = {o->p, at, o->cap};
        put32(&patch, size);
    }
}

static int refuse(char *error, size_t error_size, const char *format, int value)
{
    if (error && error_size) snprintf(error, error_size, format, value);
    return 0;
}

size_t p4_model_geometry_stream(const uint8_t *blob, const P4ModelCookedMesh *mesh, const int32_t *node_ids,
                                uint32_t nodes, uint8_t *out, size_t capacity, char *error, size_t error_size)
{
    const P4ModelCookedSkin *skin = p4_model_skin(blob, mesh);
    /* The skin stream counts bones in one byte. */
    if (!skin || !nodes || nodes > 255) return (size_t)refuse(error, error_size, "a skin needs 1 to 255 hierarchy nodes, not %d", (int)nodes);
    const int32_t *ids = (const int32_t *)(const void *)(blob + skin->jointIdsOffset);
    const uint32_t *matrices = (const uint32_t *)(const void *)(blob + skin->matricesOffset);
    const uint8_t *slots = blob + skin->indicesOffset;
    const float *weights = (const float *)(const void *)(blob + skin->weightsOffset);
    const uint32_t vertices = mesh->vertices;

    /* Joint -> hierarchy node index; only weighted joints need a node. */
    int16_t node_of[256];
    uint8_t used[256];
    memset(used, 0, sizeof used);
    for (uint32_t j = 0; j < skin->joints; ++j) {
        node_of[j] = -1;
        for (uint32_t i = 0; i < nodes; ++i)
            if (node_ids[i] == ids[j]) {
                node_of[j] = (int16_t)i;
                break;
            }
    }
    for (uint64_t k = 0; k < (uint64_t)vertices * 4; ++k) {
        if (!(weights[k] > 0.0f)) continue;
        if (node_of[slots[k]] < 0)
            return (size_t)refuse(error, error_size, "the skin weights HAnim node %d, which the replaced model's hierarchy lacks",
                                  (int)ids[slots[k]]);
        used[node_of[slots[k]]] = 1;
    }
    uint32_t used_count = 0;
    for (uint32_t i = 0; i < nodes; ++i) used_count += used[i];
    /* The used bones in ascending order, or in the retail order the cooked skin carries when that order
     * lists exactly the used bones (a re-exported retail skin stays byte-identical). */
    uint8_t order[256];
    uint32_t n = 0;
    if (skin->orderCount == used_count) {
        uint8_t seen[256];
        memset(seen, 0, sizeof seen);
        const uint8_t *slot_order = blob + skin->orderOffset;
        for (; n < used_count; ++n) {
            int16_t node = node_of[slot_order[n]];
            if (node < 0 || !used[node] || seen[node]) break;
            seen[node] = 1;
            order[n] = (uint8_t)node;
        }
    }
    if (n != used_count)
        for (uint32_t i = n = 0; i < nodes; ++i)
            if (used[i]) order[n++] = (uint8_t)i;

    Out o = {out, 0, capacity};
    size_t geometry = begin(&o, ID_GEOMETRY);
    {
        size_t s = begin(&o, ID_STRUCT);
        put32(&o, mesh->format);
        put32(&o, mesh->triangles);
        put32(&o, vertices);
        put32(&o, 1); /* morph targets */
        if (mesh->colorsOffset) put(&o, blob + mesh->colorsOffset, (size_t)vertices * 4);
        for (uint32_t set = 0; set < mesh->uvSets; ++set)
            put(&o, blob + (set ? mesh->uv1Offset : mesh->uv0Offset), (size_t)vertices * 8);
        const uint16_t *t = (const uint16_t *)(const void *)(blob + mesh->trianglesOffset);
        for (uint32_t i = 0; i < mesh->triangles; ++i, t += 4) {
            put32(&o, ((uint32_t)t[0] << 16) | t[1]); /* vertex01 */
            put32(&o, ((uint32_t)t[2] << 16) | t[3]); /* vertex2Mat */
        }
        /* The morph target: bounding sphere (the builder recalculates it), points, normals. */
        const float *p = (const float *)(const void *)(blob + mesh->positionsOffset);
        float lo[3] = {p[0], p[1], p[2]}, hi[3] = {p[0], p[1], p[2]};
        for (uint32_t v = 0; v < vertices; ++v)
            for (int a = 0; a < 3; ++a) {
                if (p[3 * v + a] < lo[a]) lo[a] = p[3 * v + a];
                if (p[3 * v + a] > hi[a]) hi[a] = p[3 * v + a];
            }
        float centre[3] = {(lo[0] + hi[0]) * 0.5f, (lo[1] + hi[1]) * 0.5f, (lo[2] + hi[2]) * 0.5f}, radius = 0.0f;
        for (uint32_t v = 0; v < vertices; ++v) {
            float dx = p[3 * v] - centre[0], dy = p[3 * v + 1] - centre[1], dz = p[3 * v + 2] - centre[2];
            float d = sqrtf(dx * dx + dy * dy + dz * dz);
            if (d > radius) radius = d;
        }
        putf(&o, centre[0]);
        putf(&o, centre[1]);
        putf(&o, centre[2]);
        putf(&o, radius);
        put32(&o, 1);
        put32(&o, mesh->normalsOffset ? 1u : 0u);
        put(&o, p, (size_t)vertices * 12);
        if (mesh->normalsOffset) put(&o, blob + mesh->normalsOffset, (size_t)vertices * 12);
        end(&o, s);
    }
    {
        /* Material list: one new material per slot, untextured; the builder sets textures and user data. */
        const uint32_t *records = (const uint32_t *)(const void *)(blob + mesh->materialsOffset);
        size_t list = begin(&o, ID_MATLIST);
        size_t s = begin(&o, ID_STRUCT);
        put32(&o, mesh->materialCount);
        for (uint32_t i = 0; i < mesh->materialCount; ++i) put32(&o, 0xFFFFFFFFu);
        end(&o, s);
        for (uint32_t i = 0; i < mesh->materialCount; ++i) {
            const P4ModelCookedMaterial *m = p4_model_material(blob, records[i]);
            size_t material = begin(&o, ID_MATERIAL);
            size_t ms = begin(&o, ID_STRUCT);
            put32(&o, 0);           /* flags */
            put(&o, m->color, 4);   /* RwRGBA */
            put32(&o, 0);           /* unused */
            put32(&o, 0);           /* textured */
            putf(&o, m->ambient);
            putf(&o, m->specular);
            putf(&o, m->diffuse);
            end(&o, ms);
            end(&o, begin(&o, ID_EXTENSION));
            end(&o, material);
        }
        end(&o, list);
    }
    {
        size_t extension = begin(&o, ID_EXTENSION);
        /* Bin mesh: a triangle list per material slot. RpGeometryUnlock rebuilds the meshes from the
         * triangles of a geometry it created, as for a retail geometry. */
        size_t bm = begin(&o, ID_BINMESH);
        uint32_t meshes = 0;
        for (uint32_t slot = 0; slot < mesh->materialCount; ++slot) {
            const uint16_t *t = (const uint16_t *)(const void *)(blob + mesh->trianglesOffset);
            for (uint32_t i = 0; i < mesh->triangles; ++i, t += 4)
                if (t[3] == slot) {
                    ++meshes;
                    break;
                }
        }
        put32(&o, 0); /* flags: triangle lists */
        put32(&o, meshes);
        put32(&o, mesh->triangles * 3);
        for (uint32_t slot = 0; slot < mesh->materialCount; ++slot) {
            uint32_t count = 0;
            const uint16_t *t = (const uint16_t *)(const void *)(blob + mesh->trianglesOffset);
            for (uint32_t i = 0; i < mesh->triangles; ++i) count += t[4 * i + 3] == slot;
            if (!count) continue;
            put32(&o, count * 3);
            put32(&o, slot);
            for (uint32_t i = 0; i < mesh->triangles; ++i, t += 4)
                if (t[3] == slot) {
                    put32(&o, t[0]);
                    put32(&o, t[1]);
                    put32(&o, t[2]);
                }
        }
        end(&o, bm);
        size_t sk = begin(&o, ID_SKIN);
        uint8_t header[4] = {(uint8_t)nodes, (uint8_t)used_count, (uint8_t)skin->maxWeights, 0};
        put(&o, header, 4);
        put(&o, order, used_count);
        for (uint32_t v = 0; v < vertices; ++v) {
            uint8_t b[4] = {0, 0, 0, 0};
            for (int k = 0; k < 4; ++k)
                if (weights[4 * v + k] > 0.0f) b[k] = (uint8_t)node_of[slots[4 * v + k]];
            put(&o, b, 4);
        }
        for (uint64_t k = 0; k < (uint64_t)vertices * 4; ++k) putf(&o, weights[k]);
        static const uint32_t identity[16] = {0x3F800000u, 0, 0, 0, 0, 0x3F800000u, 0, 0, 0, 0, 0x3F800000u, 0, 0, 0, 0, 0};
        for (uint32_t i = 0; i < nodes; ++i) {
            const uint32_t *m = identity;
            for (uint32_t j = 0; j < skin->joints; ++j)
                if (node_of[j] == (int16_t)i) {
                    m = matrices + 16 * j;
                    break;
                }
            for (int w = 0; w < 16; ++w) put32(&o, m[w]);
        }
        put32(&o, 0); /* split data: bone limit */
        put32(&o, 0); /* mesh count */
        put32(&o, 0); /* RLE count */
        end(&o, sk);
        end(&o, extension);
    }
    end(&o, geometry);
    return o.at;
}

/* ------------------------------------------------------------------------------------------------
 * The RtAnimAnimation chunk of a cooked slot animation (p4_model_animation_stream). The layout follows
 * RtAnimAnimationStreamRead and the compressed-keyframe scheme's reader (tools/models/rwmodel.py
 * Animation): version 0x100, scheme 2, keyframe count, flags, duration; per keyframe the time, four
 * quaternion and three translation halves, and the in-memory byte offset of the node's previous keyframe
 * (24-byte RtCompressedKeyFrames); then the translation offset and scale. A node's first keyframe has no
 * previous one: retail leaves those words undefined, this writes 0. */

#define ANIM_VERSION 0x100u
#define ANIM_SCHEME_COMPRESSED 2u
#define ANIM_KEYFRAME_BYTES 24u
#define ANIM_NODES_MAX 255u

typedef struct Cursor {
    const P4ModelCookedKey *keys;
    uint32_t count, next, last; /* keys, the next one to write, the output index of the last written */
} Cursor;

static void put16(Out *o, uint16_t v)
{
    uint8_t b[2] = {(uint8_t)v, (uint8_t)(v >> 8)};
    put(o, b, 2);
}

static void emit(Out *o, Cursor *c, uint32_t index, int first)
{
    const P4ModelCookedKey *k = &c->keys[c->next++];
    putf(o, k->time);
    for (int a = 0; a < 4; ++a) put16(o, k->rotation[a]);
    for (int a = 0; a < 3; ++a) put16(o, k->translation[a]);
    put32(o, first ? 0u : c->last * ANIM_KEYFRAME_BYTES);
    c->last = index;
}

/* RtAnim's order after every node's first two keys: by the time of the node's previous key (when the
 * interpolator moves past it and needs the next), ties by node index. */
static int needed_before(const Cursor *c, uint32_t a, uint32_t b)
{
    const float ta = c[a].keys[c[a].next - 1].time, tb = c[b].keys[c[b].next - 1].time;
    return ta < tb || (ta == tb && a < b);
}

static void sift_down(const Cursor *c, uint32_t *heap, uint32_t n, uint32_t at)
{
    for (;;) {
        uint32_t best = at, l = 2 * at + 1, r = l + 1;
        if (l < n && needed_before(c, heap[l], heap[best])) best = l;
        if (r < n && needed_before(c, heap[r], heap[best])) best = r;
        if (best == at) return;
        uint32_t t = heap[at];
        heap[at] = heap[best];
        heap[best] = t;
        at = best;
    }
}

size_t p4_model_animation_stream(const uint8_t *blob, const P4ModelCookedAnimation *animation, const int32_t *node_ids,
                                 uint32_t nodes, uint8_t *out, size_t capacity, char *error, size_t error_size)
{
    if (!nodes || nodes > ANIM_NODES_MAX)
        return (size_t)refuse(error, error_size, "an animation needs 1 to 255 hierarchy nodes, not %d", (int)nodes);
    const P4ModelCookedTrack *tracks = (const P4ModelCookedTrack *)(const void *)(blob + animation->tracksOffset);
    Cursor node[ANIM_NODES_MAX];
    uint64_t frames = 0;
    for (uint32_t i = 0; i < nodes; ++i) {
        const P4ModelCookedTrack *track = NULL;
        for (uint32_t t = 0; t < animation->tracks && !track; ++t)
            if (tracks[t].nodeId == node_ids[i]) track = &tracks[t];
        if (!track)
            return (size_t)refuse(error, error_size, "it has no track for HAnim node %d of the replaced model's hierarchy",
                                  (int)node_ids[i]);
        node[i].keys = (const P4ModelCookedKey *)(const void *)(blob + track->keysOffset);
        node[i].count = track->keys;
        node[i].next = node[i].last = 0;
        frames += track->keys;
    }
    if (frames > 0x7FFFFFFFu / ANIM_KEYFRAME_BYTES)
        return (size_t)refuse(error, error_size, "%d keyframes are more than RtAnim holds", (int)(frames > 0x7FFFFFFF ? 0x7FFFFFFF : frames));
    const size_t size = 12 + 20 + (size_t)frames * 22 + 24;
    if (!out || capacity < size) return size;
    Out o = {out, 0, capacity};
    size_t chunk = begin(&o, ID_ANIMATION);
    put32(&o, ANIM_VERSION);
    put32(&o, ANIM_SCHEME_COMPRESSED);
    put32(&o, (uint32_t)frames);
    put32(&o, animation->flags);
    putf(&o, animation->duration);
    uint32_t index = 0, heap[ANIM_NODES_MAX], heap_n = 0;
    for (uint32_t i = 0; i < nodes; ++i) emit(&o, &node[i], index++, 1);
    for (uint32_t i = 0; i < nodes; ++i) emit(&o, &node[i], index++, 0);
    for (uint32_t i = 0; i < nodes; ++i)
        if (node[i].next < node[i].count) heap[heap_n++] = i;
    for (uint32_t at = heap_n / 2; at-- > 0;) sift_down(node, heap, heap_n, at);
    while (heap_n) {
        Cursor *c = &node[heap[0]];
        emit(&o, c, index++, 0);
        if (c->next == c->count) heap[0] = heap[--heap_n];
        sift_down(node, heap, heap_n, 0);
    }
    for (int a = 0; a < 3; ++a) putf(&o, animation->translationOffset[a]);
    for (int a = 0; a < 3; ++a) putf(&o, animation->translationScale[a]);
    end(&o, chunk);
    return o.at;
}
