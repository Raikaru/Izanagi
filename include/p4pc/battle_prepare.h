/* Native-only battle preparation transport. Public definitions remain JSON. */
#ifndef P4PC_BATTLE_PREPARE_H
#define P4PC_BATTLE_PREPARE_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum {
    P4_PREPARE_HEADER_BYTES = 160,
    P4_PREPARE_ACTOR_BYTES = 64,
    P4_PREPARE_PERSONA_BYTES = 112,
    P4_PREPARE_ITEM_BYTES = 8,
    P4_PREPARE_DECISION_BYTES = 24,
    P4_PREPARE_MAX_SKILL = 4095,
    P4_PREPARE_MAX_BYTES = 8 * 1024 * 1024
};

typedef struct P4BattlePreparation {
    const uint8_t *header, *source, *actors, *personas, *items, *decisions;
    uint32_t source_bytes, actor_count, persona_count, item_count, decision_count;
} P4BattlePreparation;

static inline uint32_t p4_prepare_word(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

#ifdef __cplusplus
extern "C" {
#endif
/* No allocation, guest mutation, progression calls or RNG. Returned views borrow
 * the immutable request bytes. Native semantic/catalog checks follow boot. */
int p4_battle_prepare_decode(const uint8_t *bytes, size_t size, P4BattlePreparation *view, const char **reason);
/* Active-catalog persona bound for native preparation: 1 when id lies below
 * the published persona-metadata extent (0x7644c4, 14-byte rows: 256 retail,
 * the committed expanded catalog's otherwise) and its native level is 1..99. */
int p4_battle_prepare_persona_catalogued(uint32_t id);
/* Called once after native restore and real field/step readiness. Returns a
 * stable failure reason or NULL. No source files are modified. */
const char *p4_battle_prepare_apply(const P4BattlePreparation *view);
/* Actual applied native records, not a prediction from the request. */
void p4_battle_prepare_report(FILE *stream, const P4BattlePreparation *view);
#ifdef __cplusplus
}
#endif
#endif
