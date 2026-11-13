#ifndef P4PC_GAME_PERSONAS_H
#define P4PC_GAME_PERSONAS_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_tables.h"
#include "p4pc/game_text.h"
#include "p4pc/game_table_use.h"
/* Five parallel persona banks, each indexed by persona ID:
 * PERSONA0 metadata (0x007644C4), PERSONA1 growth (0x007644CC), MODEL2 battle
 * model parameters (0x007644D0), PERSONA5 replacement candidates (0x007644E0)
 * and MSG3 names (0x00764530). The party-growth, EXP-curve and skill-pair
 * blocks are not indexed by an added persona's ID; added rows leave them retail. */
#define P4_PERSONA_RETAIL_ROWS 256u
#define P4_PERSONA_METADATA_SIZE 14u
#define P4_PERSONA_GROWTH_SIZE 70u
#define P4_PERSONA_MODEL_SIZE 88u
#define P4_PERSONA_CANDIDATES_SIZE 4u
#define P4_PERSONA_NAME_SIZE 17u
/* UNIT2 at 0x007644C8: sixteen u16 affinity words per persona, read by
 * datPersonaRunSkillSlot 0x001097C0 and datPersonaGetSkillSlot 0x00109870. */
#define P4_PERSONA_AFFINITY_SIZE 32u
/* PERSONA4 at 0x007644DC (iGpffffb3ec): skill-replacement pairs, u16 original
 * and u16 replacement, read by Shuffle Time's skill change (0x0036F880), roster
 * fusion's skill change (0x003096D0, 0x00304580) and the level-up pick
 * (0x0011E490). Every reader stops at a zero pair half, never at a count:
 * retail is 40 pairs and a (0, 0) terminator. Only a derived source replaces
 * the list, with any length up to P4_PERSONA_SKILL_PAIR_MAX pairs
 * (game_table_derived.h); 0x003096D0's index is a signed halfword. */
#define P4_PERSONA_SKILL_PAIR_SPAN 4u
#define P4_PERSONA_SKILL_PAIR_SIZE 4u
#define P4_PERSONA_SKILL_PAIR_MAX 4096u
/* Public added persona IDs are 256..511: fusion recipe material selectors are
 * signed 10-bit and name/list APIs signed16, so every carrier stays exact. */
#define P4_PERSONA_MAX_ID 511u
/* Stable keys use the shared owner:local-name convention. Retail rows cannot be
 * registered. model_source names the retail persona whose model resource the
 * new persona loads: its ID still selects every table row, never that model. */
typedef struct P4GamePersonaDesc {
    uint32_t id, model_source, schema, revision;
    char key[P4_TABLE_KEY_SIZE];
    uint8_t metadata[P4_PERSONA_METADATA_SIZE];
    uint8_t growth[P4_PERSONA_GROWTH_SIZE];
    uint8_t model[P4_PERSONA_MODEL_SIZE];
    uint8_t candidates[P4_PERSONA_CANDIDATES_SIZE];
    uint8_t affinity[P4_PERSONA_AFFINITY_SIZE];
} P4GamePersonaDesc;
typedef struct P4GamePersonaView {
    P4GamePersonaDesc desc;
    char owner[64];
} P4GamePersonaView;
typedef struct P4GamePersonas P4GamePersonas;
/* Root attests this exact sealed view/id against its captured ledger entry. */
typedef int (*P4GamePersonaRowUseFn)(const P4GamePersonas *exact_view,
    uint32_t persona_id, void *immutable_source_context);
/* Inject real source transports before sealing. The original input context is
 * the actual sealed PERSONA span owner, never supplied or reconstructed. */
int p4_game_personas_bind_runtime_use(P4GamePersonas *personas,
    P4GameTableSourceUseFn original_use, P4GamePersonaRowUseFn authored_use,
    void *immutable_source_context);
/* Pure declaration prerequisite for boot staging, not origin authority. */
int p4_game_personas_validate_runtime_sources(const P4GamePersonas *personas);
/* Actual runtime consumption only, after materialization: retail rows use the
 * issued PERSONA0 span, authored rows Root's exact-view/id attestation. Only 0
 * permits consumption; positive callback statuses are errors. */
int p4_game_personas_runtime_use(const P4GamePersonas *personas, uint32_t id);
P4GamePersonas *p4_game_personas_create(const P4GameTableAllocator *allocator);
/* Destroy only after publication is detached and native readers drain. */
void p4_game_personas_destroy(P4GamePersonas *personas);
int p4_game_personas_register(P4GamePersonas *personas, const char *owner, const P4GamePersonaDesc *desc);
/* Native allocator declaration admission: added rows need an actual allocator
 * callback (NOT_FOUND if absent). No work. */
int p4_game_personas_native_admit(const P4GamePersonas *personas);
struct P4GameTableInputSpan;
/* CPU-only immutable seal against the actual producer-owned spans (PERSONA0,
 * PERSONA1, MODEL2, PERSONA5, MSG3 and UNIT2), each exactly 256 retail rows. Added IDs
 * must be dense from 256 (no zero row is ever invented between them), name a
 * retail model_source, and have a P4_TEXT_PERSONA name of at most 17 bytes
 * including NUL in the sealed text view. Same revision and exact inputs are
 * idempotent; registration never reopens after sealing. */
int p4_game_personas_seal(P4GamePersonas *personas,
    const struct P4GameTableInputSpan *metadata,
    const struct P4GameTableInputSpan *growth,
    const struct P4GameTableInputSpan *model,
    const struct P4GameTableInputSpan *candidates,
    const struct P4GameTableInputSpan *original_names,
    const struct P4GameTableInputSpan *affinity,
    const P4GameText *names, uint32_t revision);
uint32_t p4_game_personas_semantic_revision(const P4GamePersonas *personas);
/* Actual runtime phase only: verify all six loaded banks against the sealed
 * source bytes, then allocate/copy (retail prefix plus authored rows). Failure
 * leaves the definitions sealed and every game pointer unchanged; repeated
 * calls verify the same bytes without another allocation. Empty snapshots
 * allocate nothing. */
int p4_game_personas_materialize(P4GamePersonas *personas,
    const uint8_t *metadata, size_t metadata_size,
    const uint8_t *growth, size_t growth_size,
    const uint8_t *model, size_t model_size,
    const uint8_t *candidates, size_t candidates_size,
    const uint8_t *original_names, size_t original_names_size,
    const uint8_t *affinity, size_t affinity_size);
int p4_game_personas_validate_original(const P4GamePersonas *personas,
    const uint8_t *metadata, size_t metadata_size,
    const uint8_t *growth, size_t growth_size,
    const uint8_t *model, size_t model_size,
    const uint8_t *candidates, size_t candidates_size,
    const uint8_t *original_names, size_t original_names_size,
    const uint8_t *affinity, size_t affinity_size);
uint32_t p4_game_personas_revision(const P4GamePersonas *personas);
size_t p4_game_personas_count(const P4GamePersonas *personas);
/* One past the highest valid persona ID: 256 without added rows. */
uint32_t p4_game_personas_limit(const P4GamePersonas *personas);
const P4GamePersonaView *p4_game_personas_at(const P4GamePersonas *personas, size_t index);
const P4GamePersonaView *p4_game_personas_find(const P4GamePersonas *personas, uint32_t id);
/* Model resource ID for a persona: the ID itself for retail rows, the declared
 * model_source for added rows; NOT_FOUND beyond the sealed domain. */
int p4_game_personas_model_source(const P4GamePersonas *personas, uint32_t id, uint32_t *model_id);
/* Guest bases for one atomic game-thread publication of all six slots; zero
 * before materialization or for an empty snapshot. */
uint32_t p4_game_personas_metadata(const P4GamePersonas *personas);
uint32_t p4_game_personas_growth(const P4GamePersonas *personas);
uint32_t p4_game_personas_model(const P4GamePersonas *personas);
uint32_t p4_game_personas_candidates(const P4GamePersonas *personas);
uint32_t p4_game_personas_names(const P4GamePersonas *personas);
uint32_t p4_game_personas_affinity(const P4GamePersonas *personas);

/* Stage the CPU-sealed snapshot before native raw-table exposure. Added rows
 * register the `persona` completion participant, which materializes and
 * relocates the five banks only after their actual loads were observed; an
 * empty snapshot changes nothing. */
int p4_game_personas_boot_stage(P4GamePersonas *personas);
/* Exact EARLY rollback only; raw observations/materialization lock ownership. */
int p4_game_personas_boot_unstage(P4GamePersonas *exact);
P4GamePersonas *p4_game_personas_active(void);
/* Caller supplies actual RAM lifetime after tasks and queues drain; a live
 * image gets its original five slot values back. */
int p4_game_personas_session_end(int image_live);
#endif
