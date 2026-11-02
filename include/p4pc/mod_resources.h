/* Authored image resources: game/context-thread-only registration and lifetime. */
#ifndef P4PC_MOD_RESOURCES_H
#define P4PC_MOD_RESOURCES_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/content_source.h"
typedef struct P4Resources P4Resources;
typedef enum P4ResourceResult {
    P4_RESOURCE_OK = 0, P4_RESOURCE_INVALID = -1, P4_RESOURCE_CONFLICT = -2,
    P4_RESOURCE_NOMEM = -3, P4_RESOURCE_FROZEN = -4, P4_RESOURCE_NOT_FOUND = -5,
    P4_RESOURCE_RENDER_ERROR = -6, P4_RESOURCE_RETIRED = -7,
    P4_RESOURCE_NOT_READY = -8
} P4ResourceResult;
#define P4_RESOURCE_PROVIDER_OVERRIDE 1u
typedef struct P4ResourceImage {
    uint32_t token, raster, pixels, stride, width, height, texture_width, texture_height, revision;
    char key[96], owner[64];
    uint32_t flags;
    P4ContentSource source; /* Actual winning bytes, distinct from descriptor owner. */
    /* pixels borrows the raster-owned padded RGBA8 shadow; never free or write it.
     * Its allocation is stride * texture_height bytes, valid while pinned. */
} P4ResourceImage;
/* budget charges the actual padded guest RGBA shadow and GPU level-0 backing.
 * Creation alone allocates no guest memory and changes no retail globals.
 * The resource service does not resolve filenames or decode images: use the common loader/codec on
 * a worker, then register its top-down straight RGBA8 result on the game thread.
 * Source is the real winning OVERRIDE provider/key, not a retail texture alias. */
P4Resources *p4_resources_create(uint32_t budget);
int p4_resources_image(P4Resources *resources, const char *owner, const char *key,
    const struct P4ContentSource *source, const uint8_t *rgba, size_t size,
    uint32_t width, uint32_t height, uint32_t revision, uint32_t *token);
/* Successful freeze publishes token queries; before that all rows are private
 * staging. Core activation still owns the atomic mod/table/resource boundary. */
int p4_resources_freeze(P4Resources *resources, uint32_t revision);
uint32_t p4_resources_revision(const P4Resources *resources);
size_t p4_resources_count(const P4Resources *resources);
/* The core reports these owner/provider conflicts through its normal admission
 * UI/diagnostics. A known higher provider is not silently bypassed or relabeled. */
size_t p4_resources_conflicts(const P4Resources *resources);
const P4ResourceImage *p4_resources_at(const P4Resources *resources, size_t index);
const P4ResourceImage *p4_resources_find(const P4Resources *resources, const char *key);
const P4ResourceImage *p4_resources_query(uint32_t token);
/* Immediate native Im2D image: authored logical pixels (640x448 game
 * coordinates) times explicit axis scales, nearest sampling, RGBA (R low).
 * Inherits the native layout, depth, Sky/FRAME/CLAMP policy; restores only the
 * three changed RenderState cells AND their written/unwritten bits. Caller
 * holds the generation/pin through the call. No foreign queue, per-image
 * host/guest allocation or image copy; the renderer keeps its normal caches. */
int p4_resources_draw(uint32_t token, float x, float y, float depth, uint32_t rgba,
    float scale_x, float scale_y);
/* Snapshots/menu tasks/queued draws pin before borrowing a token. Retire drops
 * the owning generation, forbids new pins and defers EVERY raster/guest pixel
 * release until its last existing pin is released. All calls stay on the
 * game/context thread; the core must quiesce pins and retire all generations
 * before RW-engine, guest-image or session teardown. Graphics-only context loss
 * preserves logical tokens and the raster-owned shadow for cache reupload.
 * Never persist tokens: serialize owner:key/revision, then resolve anew.
 * retire may destroy resources immediately; do not use its pointer afterward. */
int p4_resources_pin(uint32_t token);
int p4_resources_release(uint32_t token);
/* Core-proven quiescence is still required. Atomically refuse while any pin is
 * live; on success destroy synchronously and invalidate the resources pointer. */
int p4_resources_retire_unpinned(P4Resources *resources);
void p4_resources_retire(P4Resources *resources);
#endif
