/* Lockstep-only disc replay (docs/lockstep.md, "In-battle disc replay"). After a qualified
 * battle anchor, each disc read stays busy until the game frame at which retail first saw
 * the matching read complete. Nothing changes unless P4PC_LOCKSTEP_DISC_REPLAY is set. */
#ifndef P4PC_LOCKSTEP_DISC_H
#define P4PC_LOCKSTEP_DISC_H

#include <stdint.h>

/* Loads the retail table named by P4PC_LOCKSTEP_DISC_REPLAY and opens
 * DIRECTORY/port-disc-holds.tsv. Returns 1 when configured, 0 when the variable is unset;
 * a malformed table fails the lockstep run. */
int p4pc_lockstep_disc_open(const char *directory);
/* At the qualified anchor boundary: from the next update on, every disc read must match a
 * table row and is held until the anchor-relative frame retail observed its completion. */
void p4pc_lockstep_disc_activate(uint32_t anchor_frame);

#endif
