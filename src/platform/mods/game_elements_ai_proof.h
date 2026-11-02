#ifndef P4PC_GAME_ELEMENTS_AI_PROOF_H
#define P4PC_GAME_ELEMENTS_AI_PROOF_H
#include "p4pc/game_elements_ai_native.h"
#include <stddef.h>
/* A proof is about whether a stored value is valid for the eventual defender
 * set, not a replay of native conditions. MY and original eligible-side ANY
 * questions contribute TRUE. An ID question contributes its exact live UID +
 * profile. Numeric logical operators may prove a result from either operand;
 * this is why unrelated target IDs cannot be merged into one last-target cell.
 * Nodes live in the already prepared per-VM arena: no runtime malloc or map. */
#define P4_AI_PROOF_FALSE 0u
#define P4_AI_PROOF_TRUE 1u
#define P4_AI_PROOF_FREE 0u
#define P4_AI_PROOF_TARGET 1u
#define P4_AI_PROOF_AND 2u
#define P4_AI_PROOF_OR 3u

typedef struct P4AiTargetFact {
    P4GameAiOwner owner;
    P4GameUnitIdentity identity;
} P4AiTargetFact;
typedef struct P4AiProofNode {
    uint32_t kind, generation, references, left, right, next;
    P4AiTargetFact target;
    uint32_t evaluation, value;
} P4AiProofNode;
typedef struct P4AiProofPool {
    P4AiProofNode *nodes;
    uint32_t *scratch;
    uint32_t capacity, high_water, free_head, serial, evaluation;
} P4AiProofPool;
int p4_ai_target_fact_equal(const P4AiTargetFact *left, const P4AiTargetFact *right);
void p4_ai_proof_reset(P4AiProofPool *pool);
uint32_t p4_ai_proof_retain(P4AiProofPool *pool, uint32_t proof);
void p4_ai_proof_release(P4AiProofPool *pool, uint32_t proof);
int p4_ai_proof_target(P4AiProofPool *pool, const P4AiTargetFact *target,
    uint32_t cached, uint32_t cached_generation, uint32_t *proof);
int p4_ai_proof_join(P4AiProofPool *pool, uint32_t kind,
    uint32_t left, uint32_t right, uint32_t *proof);
int p4_ai_proof_evaluate(P4AiProofPool *pool, uint32_t proof,
    const P4AiTargetFact *targets, uint32_t count, uint32_t *value);
#endif
