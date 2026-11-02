/* Original-content texture dumping. No game/GL state is touched by the worker. */
#ifndef P4PC_TEXTURE_DUMP_H
#define P4PC_TEXTURE_DUMP_H
#include "p4pc/texture_key.h"

enum P4TextureOrigin {
    P4_TEXTURE_UNKNOWN, P4_TEXTURE_DISC, P4_TEXTURE_FONT_DISC,
    P4_TEXTURE_OVERRIDE, P4_TEXTURE_FRAMEBUFFER, P4_TEXTURE_MOVIE,
    P4_TEXTURE_PROCEDURAL, P4_TEXTURE_SYNTHETIC
};
typedef struct P4TextureDumpStats {
    uint64_t submitted, completed, excluded, errors;
    size_t queued_bytes, peak_bytes;
} P4TextureDumpStats;

/* One game-thread-owned service. Empty/NULL root means disabled and performs no
 * filesystem access. The explicit synthetic flag is for asset-free tools/tests;
 * player initialization never enables it. Returns 0/-1. */
int p4_texture_dump_start(const char *root, int allow_synthetic);
int p4_texture_dump_active(void);
/* Copies original decoded pixels for worker encoding. Backpressure is deliberate
 * in this opt-in diagnostic mode: eligible one-frame variants are never dropped.
 * Excluded sources produce only an inventory reason. name may be NULL.
 * Optional key_out receives the computed identity without a second hash;
 * disabled/excluded submissions do not touch it. */
int p4_texture_dump_submit(const P4TexturePixels *pixels, enum P4TextureOrigin origin,
                           const char *name, char key_out[65]);
void p4_texture_dump_stats(P4TextureDumpStats *stats);
/* Drains and joins. Returns -1 if any eligible image failed; disabled is 0. */
int p4_texture_dump_finish(void);
#endif
