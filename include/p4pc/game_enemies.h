#ifndef P4PC_GAME_ENEMIES_H
#define P4PC_GAME_ENEMIES_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_tables.h"
#include "p4pc/game_text.h"
#include "p4pc/game_table_use.h"
/* Five parallel enemy banks, each indexed by enemy ID (affinity by the id2
 * selector, which datUnitInit 0x00231AF0 starts at the ID):
 * UNIT0 metadata (0x007644B4), UNIT1 affinity (0x007644B8), MODEL1 battle
 * parameters (0x007644BC), AICALC8 AI (0x007644C0) and MSG2 names (0x00764534). */
#define P4_ENEMY_RETAIL_ROWS 336u
#define P4_ENEMY_ROW_SIZE 60u
#define P4_ENEMY_AFFINITY_SIZE 32u
#define P4_ENEMY_MODEL_SIZE 232u
#define P4_ENEMY_AI_SIZE 164u
#define P4_ENEMY_NAME_SIZE 21u
/* Public expanded enemy IDs are 336..4095: every u16 carrier and signed s16
 * comparison stays exact while AI/script operands remain unaudited above it. */
#define P4_ENEMY_MAX_ID 4095u
/* Stable keys use the shared owner:local-name convention. Retail rows cannot be
 * registered. model_source names the retail enemy whose model resource the new
 * enemy loads: its ID still selects every table row, never that model file. */
typedef struct P4GameEnemyDesc {
    uint32_t id, model_source, schema, revision;
    char key[P4_TABLE_KEY_SIZE];
    uint8_t metadata[P4_ENEMY_ROW_SIZE];
    uint8_t affinity[P4_ENEMY_AFFINITY_SIZE];
    uint8_t model[P4_ENEMY_MODEL_SIZE];
    uint8_t ai[P4_ENEMY_AI_SIZE];
} P4GameEnemyDesc;
typedef struct P4GameEnemyView {
    P4GameEnemyDesc desc;
    char owner[64];
} P4GameEnemyView;
typedef struct P4GameEnemies P4GameEnemies;
/* Root attests this exact sealed view/id against its captured ledger entry. */
typedef int (*P4GameEnemyRowUseFn)(const P4GameEnemies *exact_view,
    uint32_t enemy_id, void *immutable_source_context);
/* Inject real source transports before sealing. The original input context is
 * the actual sealed UNIT span owner, never supplied or reconstructed. */
int p4_game_enemies_bind_runtime_use(P4GameEnemies *enemies,
    P4GameTableSourceUseFn original_use, P4GameEnemyRowUseFn authored_use,
    void *immutable_source_context);
/* Pure declaration prerequisite for boot staging, not origin authority. */
int p4_game_enemies_validate_runtime_sources(const P4GameEnemies *enemies);
/* Actual runtime consumption only, after materialization: retail rows use the
 * issued UNIT0 span, authored rows Root's exact-view/id attestation. Only 0
 * permits consumption; positive callback statuses are errors. */
int p4_game_enemies_runtime_use(const P4GameEnemies *enemies, uint32_t id);
P4GameEnemies *p4_game_enemies_create(const P4GameTableAllocator *allocator);
/* Destroy only after publication is detached and native readers drain. */
void p4_game_enemies_destroy(P4GameEnemies *enemies);
int p4_game_enemies_register(P4GameEnemies *enemies, const char *owner, const P4GameEnemyDesc *desc);
/* Native allocator declaration admission: added rows need an actual allocator
 * callback (NOT_FOUND if absent). No work. */
int p4_game_enemies_native_admit(const P4GameEnemies *enemies);
struct P4GameTableInputSpan;
/* CPU-only immutable seal against the actual producer-owned spans (UNIT0,
 * UNIT1, MODEL1, AICALC8 and MSG2), each exactly 336 retail rows. Added IDs
 * must be dense from 336 (no zero row is ever invented between them), name a
 * retail model_source, and have a P4_TEXT_ENEMY name of at most 21 bytes
 * including NUL in the sealed text view. Same revision and exact inputs are
 * idempotent; registration never reopens after sealing. */
int p4_game_enemies_seal(P4GameEnemies *enemies,
    const struct P4GameTableInputSpan *metadata,
    const struct P4GameTableInputSpan *affinity,
    const struct P4GameTableInputSpan *model,
    const struct P4GameTableInputSpan *ai,
    const struct P4GameTableInputSpan *original_names,
    const P4GameText *names, uint32_t revision);
uint32_t p4_game_enemies_semantic_revision(const P4GameEnemies *enemies);
/* Actual runtime phase only: verify all five loaded banks against the sealed
 * source bytes, then allocate/copy (retail prefix plus authored rows). Failure
 * leaves the definitions sealed and every game pointer unchanged; repeated
 * calls verify the same bytes without another allocation. Empty snapshots
 * allocate nothing. */
int p4_game_enemies_materialize(P4GameEnemies *enemies,
    const uint8_t *metadata, size_t metadata_size,
    const uint8_t *affinity, size_t affinity_size,
    const uint8_t *model, size_t model_size,
    const uint8_t *ai, size_t ai_size,
    const uint8_t *original_names, size_t original_names_size);
int p4_game_enemies_validate_original(const P4GameEnemies *enemies,
    const uint8_t *metadata, size_t metadata_size,
    const uint8_t *affinity, size_t affinity_size,
    const uint8_t *model, size_t model_size,
    const uint8_t *ai, size_t ai_size,
    const uint8_t *original_names, size_t original_names_size);
uint32_t p4_game_enemies_revision(const P4GameEnemies *enemies);
size_t p4_game_enemies_count(const P4GameEnemies *enemies);
/* One past the highest valid enemy ID: 336 without added rows. */
uint32_t p4_game_enemies_limit(const P4GameEnemies *enemies);
const P4GameEnemyView *p4_game_enemies_at(const P4GameEnemies *enemies, size_t index);
const P4GameEnemyView *p4_game_enemies_find(const P4GameEnemies *enemies, uint32_t id);
/* Model resource ID for an enemy: the ID itself for retail rows, the declared
 * model_source for added rows; NOT_FOUND beyond the sealed domain. */
int p4_game_enemies_model_source(const P4GameEnemies *enemies, uint32_t id, uint32_t *model_id);
/* Guest bases for one atomic game-thread publication of all five slots; zero
 * before materialization or for an empty snapshot. */
uint32_t p4_game_enemies_metadata(const P4GameEnemies *enemies);
uint32_t p4_game_enemies_affinity(const P4GameEnemies *enemies);
uint32_t p4_game_enemies_model(const P4GameEnemies *enemies);
uint32_t p4_game_enemies_ai(const P4GameEnemies *enemies);
uint32_t p4_game_enemies_names(const P4GameEnemies *enemies);

/* Stage the CPU-sealed snapshot before native raw-table exposure. Added rows
 * register the `enemy` completion participant, which materializes and
 * relocates the five banks only after their actual loads were observed; an
 * empty snapshot changes nothing. */
int p4_game_enemies_boot_stage(P4GameEnemies *enemies);
/* Exact EARLY rollback only; raw observations/materialization lock ownership. */
int p4_game_enemies_boot_unstage(P4GameEnemies *exact);
P4GameEnemies *p4_game_enemies_active(void);
/* Caller supplies actual RAM lifetime after tasks and queues drain; a live
 * image gets its original five slot values back. */
int p4_game_enemies_session_end(int image_live);
#endif
