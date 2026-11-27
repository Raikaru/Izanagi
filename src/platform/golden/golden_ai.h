#ifndef P4_GOLDEN_AI_H
#define P4_GOLDEN_AI_H
#include "golden_ai_rng.h"
#include "p4pc/game_mode.h"
#include <stddef.h>
#include <stdint.h>

/* Semantic input views, not guest layouts. Borrowed tables and records must
 * remain valid for the complete command. IDs in this interface are Golden IDs. */
typedef struct P4GoldenAiItem {
    uint32_t granted_skill;
    uint16_t effect, element;
} P4GoldenAiItem;
typedef struct P4GoldenAiUnit {
    uint16_t flags, id, affinity_id, hp, sp;
    uint32_t status;
    uint8_t side, target_flags, secondary[2];
    uint16_t character_id;
    uint16_t base_hp, base_sp;
    const uint16_t *skills; /* exactly eight slots, including repeated entries */
    const P4GoldenAiItem *equipment[3];
    int16_t equipment_ids[3];
    const uint16_t *affinity; /* sixteen raw entries; affinity_id is enemy id2 or persona ID */
    uint8_t enemy_flags, enemy_element;
} P4GoldenAiUnit;
typedef struct P4GoldenAiEntity P4GoldenAiEntity;
struct P4GoldenAiEntity {
    uint32_t id, flags;
    P4GoldenAiUnit *unit;
    P4GoldenAiEntity *next;
    uint16_t message_parameter, action_kind, selection;
    uint8_t message_flags;
    P4GoldenAiEntity *selected;
    P4GoldenAiEntity *candidates[12];
    uint16_t candidate_count;
    uint8_t candidates_initialized;
};
/* Golden-only battle sidecar. Writers, not consumers, supply state categories.
 * Native lifecycle mapping is a separate contract; no guessed PS2 offsets. */
typedef struct P4GoldenAiBattle {
    uint32_t flags;
    int32_t category_guard;
    uint16_t category, assist_choreography;
} P4GoldenAiBattle;
void p4_golden_ai_battle_categories(P4GoldenAiBattle *state, uint32_t flags,
                                   int32_t guard, uint16_t category);
void p4_golden_ai_battle_assist(P4GoldenAiBattle *state, uint16_t parameter);
void p4_golden_ai_battle_eligible_start(P4GoldenAiBattle *state);
void p4_golden_ai_battle_continuous_entry(P4GoldenAiBattle *state);
uint16_t p4_golden_ai_assist_selector(const P4GoldenAiBattle *state);
/* Consume only the nonzero-message wait branch's latch, not its packets. */
int p4_golden_ai_message_wait_latch(P4GoldenAiEntity *actor);
typedef struct P4GoldenAi {
    P4GoldenAiEntity *head, *actor;
    P4GoldenAiBattle *battle;
    const uint8_t *skill_rows; /* 472 x 44, unprojected SKILL1 */
    const uint8_t *skill_elements; /* 624 x 2, unprojected SKILL0 */
    const uint32_t *knowledge;
    size_t knowledge_words;
    P4GoldenAiRng *additive;
    P4GoldenSfmt *sfmt;
} P4GoldenAi;
typedef struct P4GoldenAiResult {
    int64_t value;
    uint8_t type;
} P4GoldenAiResult;
/* Returns -1 for an unregistered ID. Counts are top-of-stack-first. */
int p4_golden_ai_arity(P4GameMode mode, uint32_t command);
/* 1 handled; 0 means mode/command not handled, without any access or write.
 * Infrastructure failures are fatal, never a fabricated successful BF value. */
int p4_golden_ai_execute(P4GameMode mode, P4GoldenAi *ai, uint32_t command,
                         const int64_t *args, P4GoldenAiResult *result);
uint32_t p4_golden_ai_maximum(const P4GoldenAiUnit *unit, int sp);
uint32_t p4_golden_ai_cost(const P4GoldenAi *ai, const P4GoldenAiUnit *unit, uint16_t skill);
uint32_t p4_golden_ai_trigger(const P4GoldenAi *ai, const P4GoldenAiUnit *unit, uint16_t skill);
uint32_t p4_golden_ai_recovery_mask(const P4GoldenAiUnit *unit);
int p4_golden_ai_affinity_bit29(const P4GoldenAiUnit *unit, int16_t index);
#endif
