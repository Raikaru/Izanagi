#ifndef P4PC_LOCKSTEP_ALIGN_RUNTIME_H
#define P4PC_LOCKSTEP_ALIGN_RUNTIME_H
#include "p4pc/lockstep_align_guest.h"
#include "p4pc/lockstep_alignment.h"
int p4pc_align_init(const char *directory, FILE *replay);
uint32_t *p4pc_align_rng(void);
uint32_t p4pc_align_profile(void);
extern int p4pc_align_rng_calls_enabled;
void p4pc_align_rng_call(uint32_t draw, const void *caller);
void p4pc_align_effect_call(uint32_t kind, const void *state, uint32_t argument, const void *caller);
extern int p4pc_align_emitters_enabled;
void p4pc_align_emitter_advance(const void *emitter);
void p4pc_align_prepare(const P4WalkHost *host, uint32_t marker[8]);
const uint8_t *p4pc_align_replay_pad(const P4WalkHost *host);
const uint8_t *p4pc_align_replay_frame(const P4WalkHost *host);
int p4pc_align_pad_gate(const P4WalkHost *host, uint32_t gate[8]);
void p4pc_align_pad_read(void);
void p4pc_align_meta(FILE *record, uint32_t frame, const uint32_t marker[8]);
void p4pc_align_boundary(const P4WalkHost *host, const uint32_t marker[8]);
#endif
