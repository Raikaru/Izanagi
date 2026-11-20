#ifndef P4PC_LOCKSTEP_ALIGN_GUEST_H
#define P4PC_LOCKSTEP_ALIGN_GUEST_H

/* Host-only observation of the applied card restore; no guest state changes. */
void p4pc_align_restore_applied(void);
/* Read-only new-game reset and supporter playtime-use witnesses. */
extern int p4pc_align_playtime_enabled;
void p4pc_align_playtime_event(const char *event, const char *site);
/* Opt-in, read-only visual allocation/update provenance. */
extern int p4pc_align_rng_calls_enabled;
void p4pc_align_visual_begin(unsigned type, const void *instance);
void p4pc_align_visual_particle(unsigned particle);
void p4pc_align_visual_created(unsigned type, const void *instance, const void *caller);

#endif
