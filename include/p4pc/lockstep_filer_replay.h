/* Lockstep-only filer replay (src/platform/lockstep_filer.c, docs/lockstep.md "Lockstep disc
 * replay"). After the replay anchor, each FilerRequest is reported complete to its consumer on
 * the anchor-relative update at which retail's take first found it complete. Nothing changes
 * unless P4PC_LOCKSTEP_FILER_REPLAY is set. */
#ifndef P4PC_LOCKSTEP_FILER_REPLAY_H
#define P4PC_LOCKSTEP_FILER_REPLAY_H

#include <stdint.h>

/* Set at activation; the mirrored filerEnqueueRequest/filerTakeCompletedRequest test it. */
extern int p4pc_filer_replay_enabled;
/* Loads the retail table named by P4PC_LOCKSTEP_FILER_REPLAY and opens
 * DIRECTORY/port-filer-holds.tsv. Returns 1 when configured, 0 when the variable is unset;
 * a malformed table fails the lockstep run. */
int p4pc_lockstep_filer_open(const char *directory);
/* At the replay anchor boundary: from the next update on, every queued request must match a
 * table row and its take is held to retail's completion update. */
void p4pc_lockstep_filer_activate(uint32_t anchor_frame);
/* filerEnqueueRequest's queued request (host pointer). */
void p4pc_filer_replay_enqueue(const void *request);
/* filerTakeCompletedRequest's entry: 0 reports "not complete" for a held request. */
int p4pc_filer_replay_take(const void *request);

#endif
