/* Clean-room native Golden model resources. Format source: model-formats*.md. */
#ifndef P4PC_GOLDEN_MODEL_H
#define P4PC_GOLDEN_MODEL_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/golden_texture.h"
#define P4_GOLDEN_MODEL_CONVERTER "amd-gmo-motion@2"
#define P4_GOLDEN_MODEL_NONE UINT32_MAX
#define P4_GOLDEN_MODEL_BUDGET (256u << 20)
enum { P4_GM_AMD=0x10000, P4_GM_PAC2, P4_GM_PAC1, P4_GM_NECK, P4_GM_MEMBER, P4_GM_GMO };
/* All byte offsets address the owned source, not guest memory. Names remain raw bytes.
 * Every command, motion, adjunct, option and suffix survives parsing unchanged. */
typedef struct P4GoldenModelNode {
    uint32_t type, parent, first, next;
    uint32_t offset, size, name, name_size, args, data, children, suffix;
} P4GoldenModelNode;
typedef struct P4GoldenModel {
    uint8_t *bytes;
    size_t size;
    P4GoldenModelNode *nodes;
    uint32_t count;
} P4GoldenModel;
typedef struct P4GoldenVertexLayout {
    uint32_t format, count, stride, uv_sets, weights;
    uint32_t weight, uv, color, normal, position;
} P4GoldenVertexLayout;
typedef struct P4GoldenVertex {
    float position[3], normal[3], uv[4][2], weights[8];
    uint8_t color[4];
} P4GoldenVertex;
/* Failure leaves zero-initialized output unowned; errors name the offending byte offset. */
int p4_golden_model_parse(const uint8_t *bytes, size_t size, P4GoldenModel *out, char *error, size_t capacity);
void p4_golden_model_free(P4GoldenModel *model);
uint32_t p4_golden_model_child(const P4GoldenModel *model, uint32_t parent, uint32_t type, uint32_t ordinal);
uint32_t p4_golden_model_resolve(const P4GoldenModel *model, uint32_t scope, uint32_t reference);
int p4_golden_model_layout(uint32_t format, uint32_t count, uint32_t extra_uv, P4GoldenVertexLayout *out);
int p4_golden_model_vertex(const P4GoldenModel *model, uint32_t arrays, uint32_t vertex, P4GoldenVertex *out);
int p4_golden_model_vertices(const P4GoldenModel *model, uint32_t arrays, P4GoldenVertex *out, uint32_t count);
/* Embedded TIM2/BMP only. No guessed DDS association or PS2 alpha expansion. */
int p4_golden_model_image(const uint8_t *bytes, size_t size, P4GoldenTexture *out);
/* Triangulates all measured primitive forms, preserving strip winding and degenerates.
 * NULL out obtains count; indices have three words per triangle. Lines are a named refusal. */
int p4_golden_model_triangles(const P4GoldenModel *model, uint32_t draw, uint32_t *out, size_t capacity, size_t *triangles);
enum { P4_GM_CHANNEL_SENTINEL=1, P4_GM_CHANNEL_EXTERNAL=2, P4_GM_CHANNEL_NECK=4,
       P4_GM_CHANNEL_TIME_SENTINEL=8, P4_GM_MOTION_NECK=1 };
typedef struct P4GoldenChannel {
    uint32_t node, reference, target, command, option, curve, flags;
} P4GoldenChannel;
typedef struct P4GoldenMotion {
    uint32_t node, model, ordinal, first, count, flags;
    float start, end, fps;
} P4GoldenMotion;
typedef struct P4GoldenMotions {
    P4GoldenMotion *clips; uint32_t clip_count;
    P4GoldenChannel *channels; uint32_t channel_count;
} P4GoldenMotions;
/* All physical clips (including empty/neck clips and naming exceptions) remain in order.
 * Flags distinguish semantic exceptions from corrupt framing. Keys remain source-owned. */
int p4_golden_model_motions(const P4GoldenModel *model, P4GoldenMotions *out, char *error, size_t capacity);
void p4_golden_model_motions_free(P4GoldenMotions *bank);
int p4_golden_model_curve_key(const P4GoldenModel *model, uint32_t curve, uint32_t key, float *time, float value[4]);
int p4_golden_model_curve(const P4GoldenModel *model, uint32_t curve, float frame, float value[4]);
/* U08 conservative file-order mapping: -1 is base, 0..31 map to ordinal slot+1.
 * No invented clip, suffix mapping, id34 alias or empty-entry alias. */
uint32_t p4_golden_model_clip(const P4GoldenMotions *bank, uint32_t model, int32_t slot);
float p4_golden_model_motion_frame(const P4GoldenMotion *clip, double seconds, int loop);
/* Native host scene; world uses column vectors, translation at 12..14.
 * Rest vertices/bindings are retained for all skin lanes, never the PS2 four-lane skin. */
typedef struct P4GoldenModelBone {
    uint32_t node, parent;
    float local[16], world[16], rest[10], pose[10]; /* T XYZ, Q XYZW, S XYZ */
} P4GoldenModelBone;
typedef struct P4GoldenModelMesh {
    uint32_t node, bone, material, vertices, triangles, uv_sets, weight_count;
    P4GoldenVertex *vertex;
    uint32_t *indices;
    P4GoldenVertex *rest;
    uint32_t joints[8];
    float offsets[8][16];
} P4GoldenModelMesh;
typedef struct P4GoldenModelHelper { uint32_t node, bone; int32_t id; float world[16]; } P4GoldenModelHelper;
typedef struct P4GoldenModelScene {
    P4GoldenModel source;
    P4GoldenModelBone *bones; uint32_t bone_count;
    P4GoldenModelMesh *meshes; uint32_t mesh_count;
    P4GoldenModelHelper *helpers; uint32_t helper_count;
    uint8_t *bone_state;
    uint32_t *bone_index; /* source-node -> native bone, or NONE */
} P4GoldenModelScene;
/* Evaluate one physical clip at source-frame time. Empty clips restore the rest pose.
 * Unresolved target/neck/effect semantics cause a named refusal before changing pose. */
int p4_golden_model_pose(P4GoldenModelScene *scene, const P4GoldenMotions *bank, uint32_t clip, float frame, char *error, size_t capacity);
int p4_golden_model_scene(const uint8_t *bytes, size_t size, P4GoldenModelScene *out, char *error, size_t capacity);
void p4_golden_model_scene_free(P4GoldenModelScene *scene);
/* Pure CPU readiness check; named material/UV/source refusals do not lose parsed data. */
int p4_golden_model_static_check(const P4GoldenModelScene *scene, char *error, size_t capacity);
/* Golden-only native source seam. The returned genuine RW clump owns its hierarchy,
 * geometry and textures. No PS2 model/skeleton/texture is consulted. */
void *p4_golden_model_build(const P4GoldenModelScene *scene, char *error, size_t capacity);
void *p4_golden_model_load(const char *path, char *error, size_t capacity);
void p4_golden_model_destroy(void *clump);
/* Draw with the provisional Golden static alpha/depth policy; restores all state. */
void p4_golden_model_draw(void *clump);
/* Bind two owned Golden resources by helper id on the explicit native bone index.
 * No HAnim/traversal/helper-id alias. Child/owner lifetime and cycle checks are enforced. */
int p4_golden_model_attach(void *owner, uint32_t bone, int32_t helper_id, void *child, char *error, size_t capacity);
/* Absolute playback time avoids drift. Full MODEL0 record is forwarded byte-for-byte;
 * its fifth halfword is not interpreted as a clip id. NULL record is explicit viewer use. */
int p4_golden_model_play(void *clump, uint32_t model_ordinal, int32_t slot, const uint8_t record[10], double seconds, int loop, char *error, size_t capacity);
/* Explicit physical clip selection also admits ordinals beyond the 32-slot bank.
 * Physical zero is base. It carries no MODEL0 record. */
int p4_golden_model_play_physical(void *clump, uint32_t model_ordinal, uint32_t physical, double seconds, int loop, char *error, size_t capacity);
#endif
