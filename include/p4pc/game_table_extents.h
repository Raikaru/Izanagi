#ifndef P4PC_GAME_TABLE_EXTENTS_H
#define P4PC_GAME_TABLE_EXTENTS_H
#include <stdint.h>
/* The table service's registry of packed battle-bank extents: the exact byte
 * size behind each GP pointer slot, never inferred from padding or RAM bounds.
 * Every slot writer publishes in the same step as its pointer store: the native
 * copy at 0x00193840, relocating commits at 0x0010d480 and session-end
 * restores. Port-owned metadata only; no guest memory or RNG change. */
/* For a writer that has just stored *slot = base itself (the native copy). */
void p4_game_table_extent_note(uint32_t slot, uint32_t base, uint32_t bytes);
/* Store *slot = base and record its extent in one step. */
void p4_game_table_slot_publish(uint32_t slot, uint32_t base, uint32_t bytes);
/* Readers: 1 with the slot's base and whole rows, 0 unless the native boot
 * catalog completed (all eight files), the slot still holds the recorded base,
 * the extent lies in RAM and the stride divides it exactly. */
int p4_game_table_extent_rows(uint32_t slot, uint32_t stride, uint32_t *base, uint32_t *rows);
/* Content identity of the table catalog the banks were built from. 0 with
 * SHA-256("p4pc-table-catalog-v1:retail") while no expanded catalog is active;
 * 1 with a v1-tagged SHA-256 over every package table source (key, package
 * id/version/kind, schema, revision and the SHA-256 of its decoded bytes),
 * every native declaration origin and every sealed original input's SHA-256
 * once one is; negative on invalid arguments. */
int p4_game_table_catalog_identity(uint8_t sha256[32]);
#endif
