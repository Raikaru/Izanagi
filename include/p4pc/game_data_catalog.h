/* p4.data runtime catalog: the effective rows the game is using, under keys
 * computed from the unmodded disc names (docs/modding/systems/data-and-edits.md).
 *
 * Keys and the unmodded names are captured once per process at package boot,
 * from the disc's own battle/MSG.TBL and the executable's item-name bank, and
 * only when an admitted package has a script. Values and current names are
 * read from the loaded banks (the table service's slot extents, the item
 * table and the active added-row snapshots), so they include every package
 * replacement, derived producer, table delta and added row. Reads write no
 * game state, call no game code that writes and draw no RNG. */
#ifndef P4PC_GAME_DATA_CATALOG_H
#define P4PC_GAME_DATA_CATALOG_H
#include "p4pc/game_data.h"
#include "p4pc/packages.h"

/* Captures the disc names when an admitted package has a script or a data
 * layer (data.luau, data/edits.xml). */
extern const P4PackageHandler p4_game_data_boot_provider;
/* The same capture, for the table seal: 0 captured (now or earlier), -1 with
 * the reason p4_data_catalog_ready reports. Package admission only. */
int p4_data_catalog_capture(void);
/* Selected disc UNIT.TBL's original enemy level, captured independently of
 * replacement and edit layers during package boot. source is the canonical
 * disc key and sha256 covers the complete packed file; neither attests that
 * the selected disc is an unmodified retail image. No added-row IDs. */
int p4_data_original_enemy_level(uint32_t id, uint8_t *level, const char **source,
    const char **sha256, const char **reason);

/* The working rows of the table seal, laid out as the loaded banks are (row
 * strides of game_data.h). While a view is installed every catalog read uses
 * it instead of the loaded banks; item names are the disc's. */
typedef struct P4DataBanks {
    const uint8_t *skill_element, *skill, *skill_name;   /* SKILL0, SKILL1, MSG1 */
    uint32_t skill_elements, skills, skill_names;
    const uint8_t *enemy, *enemy_affinity, *enemy_name;  /* UNIT0, UNIT1, MSG2 */
    uint32_t enemies, enemy_names;
    const uint8_t *persona, *persona_growth, *party, *persona_affinity, *persona_name; /* PERSONA0/1/2, UNIT2, MSG3 */
    uint32_t personas, parties, persona_names;
    const uint8_t *item;                                 /* ITEMTBL rows after the 16-byte header */
    uint32_t items;
    const uint8_t *floor, *formation, *encounters, *treasure; /* ENCOUNT 1, 0, 3, 4 (relocated when moved) */
    uint32_t floors, formations, encounter_groups, treasures;
} P4DataBanks;
void p4_data_catalog_seal_view(const P4DataBanks *banks);
/* After boot: the banks the game has loaded (session edits write them). The
 * item rows are absent when added items made them an immutable projection.
 * 0, or -1 when the catalog is not ready. */
int p4_data_catalog_live_banks(P4DataBanks *out);

/* 0 ready; -1 with a readable reason (no script package was admitted at
 * boot, the disc's names could not be read, or the game has not loaded its
 * tables yet). */
int p4_data_catalog_ready(const char **reason);
/* The current rows of one kind for lookup and enumeration. Valid until the
 * next catalog call. */
int p4_data_catalog_rows(P4DataKind kind, P4DataRows *rows);
uint32_t p4_data_catalog_id(P4DataKind kind, size_t index);
/* 0 and *index for a row ID in the current domain, -1 otherwise. */
int p4_data_catalog_index(P4DataKind kind, uint32_t id, size_t *index);
const char *p4_data_catalog_key(P4DataKind kind, size_t index);
/* Current display name copied into out (game text bytes, ASCII for the
 * English disc); NULL for an absent or placeholder row. */
const char *p4_data_catalog_name(P4DataKind kind, size_t index, char *out, size_t capacity);
/* Arcana display name from the disc (MSG0), NULL outside 1..31. */
const char *p4_data_catalog_arcana(uint32_t arcana);
int p4_data_catalog_arcana_parse(const char *name, uint32_t *arcana);

/* Effective typed rows; 0 or -1 when the row is outside the loaded banks. */
int p4_data_catalog_skill(size_t index, P4DataSkill *out);
int p4_data_catalog_enemy(size_t index, P4DataEnemy *out);
int p4_data_catalog_persona(size_t index, P4DataPersona *out);
int p4_data_catalog_item(size_t index, P4DataItem *out);
int p4_data_catalog_floor(size_t index, P4DataFloor *out);
int p4_data_catalog_formation(size_t index, P4DataFormation *out);
int p4_data_catalog_encounters(size_t index, P4DataEncounters *out);
int p4_data_catalog_treasure(size_t index, P4DataTreasure *out);

/* For native callers (semantic events, battle decisions): a row's key and
 * current display name by kind and ID, copied into out. No allocation and no
 * game-state access beyond the bank reads above; game thread. 0 found; -1
 * when the catalog is not ready or the ID is outside the loaded banks (out
 * then holds only kind and id). */
#define P4_DATA_KEY_SIZE 112u
#define P4_DATA_NAME_SIZE 32u
typedef struct P4DataRef {
    P4DataKind kind;
    uint32_t id;
    char key[P4_DATA_KEY_SIZE];
    char name[P4_DATA_NAME_SIZE]; /* empty for a placeholder row */
} P4DataRef;
int p4_data_ref(P4DataKind kind, uint32_t id, P4DataRef *out);
#endif
