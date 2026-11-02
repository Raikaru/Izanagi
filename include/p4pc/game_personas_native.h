#ifndef P4PC_GAME_PERSONAS_NATIVE_H
#define P4PC_GAME_PERSONAS_NATIVE_H
#include <stdint.h>
#include "p4pc/game_table_admission_native.h"
/* Scalar-only bridge for translated game units; no host registry layout. */
/* One past the highest persona ID the published banks hold: 256 until an
 * expanded snapshot's five slots are committed at battle-table completion. */
uint32_t p4_game_personas_native_limit(void);
int p4_game_personas_expanded(void);
/* Packed-load participation through p4_game_tables_observe_packed only. Scan
 * returns the persona banks among the actual loader's blocks (0 without staging);
 * accept records them after the shared authentication. */
int p4_game_personas_packed_pending(void);
uint32_t p4_game_personas_packed_scan(const P4GameTablePackedBlock *blocks, unsigned count);
void p4_game_personas_packed_accept(uint32_t mask);
#endif
