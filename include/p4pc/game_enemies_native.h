#ifndef P4PC_GAME_ENEMIES_NATIVE_H
#define P4PC_GAME_ENEMIES_NATIVE_H
#include <stdint.h>
#include "p4pc/game_table_admission_native.h"
/* Scalar-only bridge for translated game units; no host registry layout. */
/* One past the highest enemy ID the published banks hold: 336 until an
 * expanded snapshot's five slots are committed at battle-table completion. */
uint32_t p4_game_enemies_native_limit(void);
int p4_game_enemies_expanded(void);
/* One past the highest enemy ID the staged or published snapshot declares:
 * what a completion participant must cover before the banks are published. */
uint32_t p4_game_enemies_native_declared_limit(void);
/* Presentation resource ID for an enemy: the ID itself for retail enemies, the
 * declared retail model source for an added one (its use is attributed to the
 * owning source). Data banks stay indexed by the enemy's own ID. */
uint32_t p4_game_enemies_native_model(uint32_t id);
/* Packed-load participation through p4_game_tables_observe_packed only. Scan
 * returns the enemy banks among the actual loader's blocks (0 without staging);
 * accept records them after the shared authentication. */
int p4_game_enemies_packed_pending(void);
uint32_t p4_game_enemies_packed_scan(const P4GameTablePackedBlock *blocks, unsigned count);
void p4_game_enemies_packed_accept(uint32_t mask);
#endif
