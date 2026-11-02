/* Pure owned-proof algebra witnesses, not native AI/RNG/gameplay parity. */
#include "../src/platform/mods/game_elements_ai_proof.h"
#include "p4pc/game_tables.h"
#include <assert.h>
#include <stdint.h>

static P4AiProofNode nodes[512];
static uint32_t scratch[512];
static P4AiProofPool pool = {.nodes = nodes, .scratch = scratch, .capacity = 512u};
static P4AiTargetFact fact(uint32_t id)
{
    P4AiTargetFact value = {{0x100u + id * 16u, id, id ^ 0xabcdef01u, UINT32_MAX},
        {1u, UINT32_MAX - id, UINT32_MAX}};
    return value;
}
static uint32_t target(const P4AiTargetFact *value)
{
    uint32_t result;
    assert(!p4_ai_proof_target(&pool, value, 0u, 0u, &result));
    return result;
}
static uint32_t join(uint32_t kind, uint32_t left, uint32_t right)
{
    uint32_t result;
    assert(!p4_ai_proof_join(&pool, kind, left, right, &result));
    return result;
}
static uint32_t evaluate(uint32_t proof, const P4AiTargetFact *values, uint32_t count)
{
    uint32_t value;
    assert(!p4_ai_proof_evaluate(&pool, proof, values, count, &value));
    return value;
}
static void exact_uid_profile_and_domains(void)
{
    p4_ai_proof_reset(&pool);
    P4AiTargetFact values[2] = {fact(1u), fact(2u)};
    uint32_t b = target(&values[0]), c = target(&values[1]);
    uint32_t either = join(P4_AI_PROOF_OR, b, c), both = join(P4_AI_PROOF_AND, b, c);
    assert(evaluate(either, values, 1u) && evaluate(either, values + 1u, 1u));
    assert(!evaluate(both, values, 1u) && evaluate(both, values, 2u));
    assert(!evaluate(either, NULL, 0u));
    assert(evaluate(P4_AI_PROOF_TRUE, NULL, 0u)); /* MY/global ANY do not bind a later target. */
    P4AiTargetFact changed = values[0]; changed.identity.native_profile--;
    assert(!evaluate(b, &changed, 1u));
    changed = values[0]; changed.owner.uid_high--;
    assert(!evaluate(b, &changed, 1u));
    uint32_t generation = nodes[b].generation, copy;
    assert(!p4_ai_proof_target(&pool, &values[0], b, generation, &copy) && copy == b);
    p4_ai_proof_release(&pool, copy);
    uint32_t absorbed = join(P4_AI_PROOF_AND, b, either);
    assert(absorbed == b);
    p4_ai_proof_release(&pool, absorbed);
    p4_ai_proof_release(&pool, b); p4_ai_proof_release(&pool, c);
    p4_ai_proof_release(&pool, either); p4_ai_proof_release(&pool, both);
    for (uint32_t i = 2u; i < pool.high_water; ++i) assert(!nodes[i].references);
}
static void iterative_lifetime_without_actor_count_guess(void)
{
    p4_ai_proof_reset(&pool);
    P4AiTargetFact values[100];
    uint32_t root = P4_AI_PROOF_TRUE;
    for (uint32_t i = 0; i < 100u; ++i) {
        values[i] = fact(i + 1u);
        uint32_t leaf = target(&values[i]), next = join(P4_AI_PROOF_AND, root, leaf);
        p4_ai_proof_release(&pool, root); p4_ai_proof_release(&pool, leaf); root = next;
    }
    assert(evaluate(root, values, 100u));
    assert(!evaluate(root, values, 99u));
    p4_ai_proof_release(&pool, root);
    for (uint32_t i = 2u; i < pool.high_water; ++i) assert(!nodes[i].references);
}
static void bounded_storage_failure_is_not_a_target_fallback(void)
{
    p4_ai_proof_reset(&pool); pool.capacity = 4u;
    P4AiTargetFact b = fact(1u), c = fact(2u);
    uint32_t left = target(&b), right = target(&c), out = UINT32_MAX;
    assert(p4_ai_proof_join(&pool, P4_AI_PROOF_AND, left, right, &out) == P4_TABLE_NOMEM);
    assert(out == UINT32_MAX && evaluate(left, &b, 1u) && !evaluate(left, &c, 1u));
    p4_ai_proof_release(&pool, left); p4_ai_proof_release(&pool, right);
    pool.capacity = 512u;
}
int main(void)
{
    exact_uid_profile_and_domains();
    iterative_lifetime_without_actor_count_guess();
    bounded_storage_failure_is_not_a_target_fallback();
    return 0;
}
