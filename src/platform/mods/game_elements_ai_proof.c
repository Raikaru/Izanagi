#include "game_elements_ai_proof.h"
#include "p4pc/game_tables.h"
#include <limits.h>
#include <stdlib.h>

int p4_ai_target_fact_equal(const P4AiTargetFact *a, const P4AiTargetFact *b)
{
    return a->owner.action_guest == b->owner.action_guest && a->owner.action_id == b->owner.action_id &&
        a->owner.uid_low == b->owner.uid_low && a->owner.uid_high == b->owner.uid_high &&
        a->identity.entity_kind == b->identity.entity_kind && a->identity.entity_id == b->identity.entity_id &&
        a->identity.native_profile == b->identity.native_profile;
}
void p4_ai_proof_reset(P4AiProofPool *pool)
{
    /* Every owner is in this exact VM generation. No proof handle escapes its
     * pool: the final decision is checked before the native work is released. */
    pool->high_water = 2u; pool->free_head = pool->serial = pool->evaluation = 0u;
}
static int live(const P4AiProofPool *pool, uint32_t proof)
{
    return proof < 2u || (proof < pool->high_water && pool->nodes[proof].references != 0u);
}
uint32_t p4_ai_proof_retain(P4AiProofPool *pool, uint32_t proof)
{
    if (!live(pool, proof)) abort();
    if (proof >= 2u && ++pool->nodes[proof].references == 0u) abort();
    return proof;
}
void p4_ai_proof_release(P4AiProofPool *pool, uint32_t proof)
{
    if (!live(pool, proof)) abort();
    if (proof < 2u || --pool->nodes[proof].references) return;
    uint32_t pending = proof;
    pool->nodes[proof].next = 0u;
    while (pending) {
        uint32_t current = pending;
        P4AiProofNode *node = &pool->nodes[current];
        pending = node->next;
        if (node->kind == P4_AI_PROOF_AND || node->kind == P4_AI_PROOF_OR) {
            const uint32_t children[2] = {node->left, node->right};
            for (uint32_t i = 0; i < 2u; ++i) {
                uint32_t child = children[i];
                if (child >= 2u && --pool->nodes[child].references == 0u) {
                    pool->nodes[child].next = pending; pending = child;
                }
            }
        }
        node->kind = P4_AI_PROOF_FREE;
        node->next = pool->free_head; pool->free_head = current;
    }
}
static int allocate(P4AiProofPool *pool, uint32_t kind, uint32_t *out)
{
    if (!pool || !pool->nodes || !pool->scratch || pool->capacity < 2u || !out) return P4_TABLE_INVALID;
    if (pool->serial == UINT32_MAX) return P4_TABLE_CONFLICT;
    uint32_t index;
    if (pool->free_head) {
        index = pool->free_head; pool->free_head = pool->nodes[index].next;
    } else {
        if (pool->high_water == pool->capacity) return P4_TABLE_NOMEM;
        index = pool->high_water++;
    }
    P4AiProofNode *node = &pool->nodes[index];
    node->kind = kind; node->generation = ++pool->serial;
    node->references = 1u; node->evaluation = 0u;
    *out = index;
    return P4_TABLE_OK;
}
int p4_ai_proof_target(P4AiProofPool *pool, const P4AiTargetFact *target,
    uint32_t cached, uint32_t generation, uint32_t *out)
{
    if (!pool || !target || !out) return P4_TABLE_INVALID;
    if (cached >= 2u && live(pool, cached)) {
        const P4AiProofNode *node = &pool->nodes[cached];
        if (node->kind == P4_AI_PROOF_TARGET && node->generation == generation &&
            p4_ai_target_fact_equal(&node->target, target)) {
            *out = p4_ai_proof_retain(pool, cached); return P4_TABLE_OK;
        }
    }
    uint32_t result;
    int status = allocate(pool, P4_AI_PROOF_TARGET, &result);
    if (status) return status;
    pool->nodes[result].target = *target;
    *out = result;
    return P4_TABLE_OK;
}
static int contains_direct(const P4AiProofPool *pool, uint32_t tree, uint32_t child, uint32_t kind)
{
    return tree >= 2u && pool->nodes[tree].kind == kind &&
        (pool->nodes[tree].left == child || pool->nodes[tree].right == child);
}
int p4_ai_proof_join(P4AiProofPool *pool, uint32_t kind, uint32_t left, uint32_t right, uint32_t *out)
{
    if (!pool || !out || (kind != P4_AI_PROOF_AND && kind != P4_AI_PROOF_OR) ||
        !live(pool, left) || !live(pool, right)) return P4_TABLE_INVALID;
    uint32_t absorbing = kind == P4_AI_PROOF_AND ? P4_AI_PROOF_FALSE : P4_AI_PROOF_TRUE;
    uint32_t identity = kind == P4_AI_PROOF_AND ? P4_AI_PROOF_TRUE : P4_AI_PROOF_FALSE;
    if (left == absorbing || right == absorbing) { *out = absorbing; return P4_TABLE_OK; }
    if (left == identity) { *out = p4_ai_proof_retain(pool, right); return P4_TABLE_OK; }
    if (right == identity || left == right) { *out = p4_ai_proof_retain(pool, left); return P4_TABLE_OK; }
    if (contains_direct(pool, left, right, kind)) { *out = p4_ai_proof_retain(pool, left); return P4_TABLE_OK; }
    if (contains_direct(pool, right, left, kind)) { *out = p4_ai_proof_retain(pool, right); return P4_TABLE_OK; }
    uint32_t other = kind == P4_AI_PROOF_AND ? P4_AI_PROOF_OR : P4_AI_PROOF_AND;
    if (contains_direct(pool, left, right, other)) { *out = p4_ai_proof_retain(pool, right); return P4_TABLE_OK; }
    if (contains_direct(pool, right, left, other)) { *out = p4_ai_proof_retain(pool, left); return P4_TABLE_OK; }
    uint32_t result;
    int status = allocate(pool, kind, &result);
    if (status) return status;
    pool->nodes[result].left = p4_ai_proof_retain(pool, left);
    pool->nodes[result].right = p4_ai_proof_retain(pool, right);
    *out = result;
    return P4_TABLE_OK;
}
static uint32_t evaluated(const P4AiProofPool *pool, uint32_t proof)
{
    return proof < 2u ? proof : pool->nodes[proof].value;
}
int p4_ai_proof_evaluate(P4AiProofPool *pool, uint32_t proof,
    const P4AiTargetFact *targets, uint32_t count, uint32_t *out)
{
    if (!pool || !out || (count && !targets) || !live(pool, proof)) return P4_TABLE_INVALID;
    if (proof < 2u) { *out = proof; return P4_TABLE_OK; }
    if (++pool->evaluation == 0u) {
        for (uint32_t i = 2u; i < pool->high_water; ++i) pool->nodes[i].evaluation = 0u;
        pool->evaluation = 1u;
    }
    uint32_t depth = 1u;
    pool->scratch[0] = proof;
    while (depth) {
        uint32_t index = pool->scratch[depth - 1u];
        P4AiProofNode *node = &pool->nodes[index];
        if (node->evaluation == pool->evaluation) { --depth; continue; }
        if (node->kind == P4_AI_PROOF_TARGET) {
            node->value = 0u;
            for (uint32_t i = 0; i < count; ++i) {
                if (p4_ai_target_fact_equal(&node->target, &targets[i])) { node->value = 1u; break; }
            }
        } else {
            if (node->left >= 2u && pool->nodes[node->left].evaluation != pool->evaluation) {
                if (depth == pool->capacity) return P4_TABLE_INVALID;
                pool->scratch[depth++] = node->left; continue;
            }
            uint32_t left = evaluated(pool, node->left);
            if ((node->kind == P4_AI_PROOF_AND && !left) || (node->kind == P4_AI_PROOF_OR && left)) {
                node->value = left;
            } else {
                if (node->right >= 2u && pool->nodes[node->right].evaluation != pool->evaluation) {
                    if (depth == pool->capacity) return P4_TABLE_INVALID;
                    pool->scratch[depth++] = node->right; continue;
                }
                node->value = evaluated(pool, node->right);
            }
        }
        node->evaluation = pool->evaluation; --depth;
    }
    *out = pool->nodes[proof].value;
    return P4_TABLE_OK;
}
