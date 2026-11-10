/* Lockstep-only disc replay (docs/lockstep.md, "Lockstep disc replay"). After the accepted
 * initial LOAD anchor of a state-aligned run (or the qualified anchor of a battle qualification),
 * each disc read stays busy until the game frame at which retail first saw the matching read
 * complete, or, online (P4PC_LOCKSTEP_DISC_ONLINE), until the update in which retail's own poll
 * saw it complete. Nothing changes unless one of them is set. */
#ifndef P4PC_LOCKSTEP_DISC_H
#define P4PC_LOCKSTEP_DISC_H

#include <stdint.h>

/* Loads the retail table named by P4PC_LOCKSTEP_DISC_REPLAY and opens
 * DIRECTORY/port-disc-holds.tsv. Returns 1 when configured, 0 when the variable is unset;
 * a malformed table fails the lockstep run. */
int p4pc_lockstep_disc_open(const char *directory);
/* At the replay anchor boundary: from the next update on, every disc read must match a
 * table row and is held until the anchor-relative frame retail observed its completion. */
void p4pc_lockstep_disc_activate(uint32_t anchor_frame);
/* A status poll of a disc handle that is still reading (the H_Cdvd path's 0x004c9820 and the
 * partition loader's 0x004c9858). In an online replay (P4PC_LOCKSTEP_DISC_ONLINE) a held game
 * read takes retail's next poll of it, which must be in this update, and is released when that
 * poll saw it complete. Nothing otherwise. */
void p4pc_lockstep_disc_poll(const void *handle);

#endif
