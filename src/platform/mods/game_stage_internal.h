#ifndef P4PC_GAME_STAGE_INTERNAL_H
#define P4PC_GAME_STAGE_INTERNAL_H
#include <stddef.h>
#include <stdint.h>
/* One ordered modifier registry shared by the native amount stages: final
 * damage, recovery, skill cost and status/KO chance (docs/modding/tables.md).
 * Each stage owns one GameStage and its typed public API; this file owns the
 * order, the single composition in double precision and the refusals. Every
 * stage is main-thread battle/menu code. */
#define GAME_STAGE_OWNER_SIZE 64u
enum { GAME_STAGE_OK = 0, GAME_STAGE_INVALID = -1, GAME_STAGE_BUSY = -2, GAME_STAGE_NOMEM = -3,
    GAME_STAGE_NOT_FOUND = -4 };
enum { GAME_STAGE_MULTIPLY = 0, GAME_STAGE_ADD = 1 };
/* The stage's typed modifier pointer travels as a generic function pointer
 * and is converted back by the stage's own invoke thunk, never called through
 * a mismatched type. */
typedef void (*GameStageFn)(void);
typedef int (*GameStageInvoke)(GameStageFn fn, void *user, const void *hit, double *value);
typedef struct GameStageEntry {
    char owner[GAME_STAGE_OWNER_SIZE];
    int32_t priority;
    uint32_t load_order, kind, handle, sequence, reported;
    GameStageFn fn;
    void *user;
} GameStageEntry;
typedef struct GameStage {
    const char *name;          /* trace and report prefix, e.g. "damage stage" */
    const char *trace_variable; /* environment switch for the per-hit trace */
    GameStageInvoke invoke;
    GameStageEntry *entries;
    size_t count, capacity;
    uint32_t next_handle, next_sequence;
    int composing, trace;
} GameStage;
/* Order: priority ascending, then MULTIPLY before ADD, then load order, then
 * registration order. INVALID for a missing function, an unbounded owner or
 * an unknown kind; BUSY while this stage composes. */
int game_stage_register(GameStage *stage, const char *owner, int32_t priority, uint32_t load_order,
    uint32_t kind, GameStageFn fn, void *user, uint32_t *handle);
int game_stage_unregister(GameStage *stage, uint32_t handle);
size_t game_stage_count(const GameStage *stage);
int game_stage_trace(GameStage *stage);
/* Composes every entry in order over stock in double precision. Returns 0
 * with *value = stock when the stage is already composing (a modifier
 * re-entered it), else 1. Non-finite steps and negative factors are skipped
 * and reported once per entry; the intermediate stays within +-1e18. */
int game_stage_compose(GameStage *stage, const void *hit, double stock, double *value);
/* The amount equation's per-call facts, shared by the damage and recovery
 * stages: whether 001f14f0's bracket is resolving this skill, and then its
 * P4_DAMAGE_ORIGIN_* command, aimed-at unit and repel. Unbracketed calls are
 * estimates with origin unknown. Defined in game_damage_stage.c. */
typedef struct GameAmountContext {
    uint32_t resolved, origin, origin_side, origin_id, reflected;
} GameAmountContext;
void game_amount_bracket(uint32_t skill, GameAmountContext *out);
/* The persona fields every stage reports: the party source's active persona
 * and its level for a skill or an unbracketed estimate, else 0 and 0. */
void game_amount_source_persona(uint32_t *persona, uint32_t *level, uint32_t source_enemy, uint32_t origin,
    uint32_t record_persona, uint32_t record_level);
const char *game_stage_origin_name(uint32_t origin);
#endif