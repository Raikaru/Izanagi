#ifndef P4PC_GAME_PERSONAS_NATIVE_H
#define P4PC_GAME_PERSONAS_NATIVE_H
#include <stdint.h>
#include "p4pc/game_table_admission_native.h"
/* Scalar-only bridge for translated game units; no host registry layout. */
/* One past the highest persona ID the published banks hold: 256 until an
 * expanded snapshot's six slots are committed at battle-table completion. */
uint32_t p4_game_personas_native_limit(void);
int p4_game_personas_expanded(void);
/* One past the highest persona ID the staged or published snapshot declares:
 * what a completion participant must cover before the banks are published. */
uint32_t p4_game_personas_native_declared_limit(void);
/* Model resource ID for a persona: the ID itself for retail personas, the
 * declared retail model source for an added one (its use is attributed to the
 * owning source). Data banks stay indexed by the persona's own ID. */
uint32_t p4_game_personas_native_model(uint32_t id);
/* Guest address of a persona's 48-byte compendium row: the save block's cache
 * (0x007981CC + id * 0x30) for retail IDs and every ID without expansion, the
 * port-owned extension for added IDs below the limit. An expanded ID at or
 * above the limit is a controlled refusal. */
uint32_t p4_game_personas_native_compendium(uint32_t id);
/* New Game's cache clear (0x0010FC80) also clears the added rows. */
void p4_game_personas_native_compendium_clear(void);
/* Normal-fusion result scans (0x003130E0/0x003133B0) visit the retail IDs
 * below 0xC0, then the added IDs below the limit; metadata flags and arcana
 * decide eligibility exactly as for retail rows. */
#define P4_PERSONA_FUSION_RETAIL_END 0xC0u
int p4_game_personas_native_fusion_candidate(uint32_t id);
uint32_t p4_game_personas_native_fusion_next(uint32_t id);
/* Packed-load participation through p4_game_tables_observe_packed only. Scan
 * returns the persona banks among the actual loader's blocks (0 without staging);
 * accept records them after the shared authentication. */
int p4_game_personas_packed_pending(void);
uint32_t p4_game_personas_packed_scan(const P4GameTablePackedBlock *blocks, unsigned count);
void p4_game_personas_packed_accept(uint32_t mask);
#endif
