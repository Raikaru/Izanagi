#include "game_elements_ai_state.h"
#include "p4pc/gmem.h"
#include <string.h>

static uint32_t all_candidates(const P4AiCarrier *carrier)
{
    return (1u << carrier->candidate_count) - 1u;
}
static uint32_t read16(const uint8_t *bytes)
{
    return bytes[0] | (uint32_t)bytes[1] << 8;
}
int p4_game_elements_ai_native_root_begin(void *source, void *work)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier || !carrier->candidate_count) return P4_TABLE_NOT_READY;
    if (carrier->table_active || carrier->table_work_guest || carrier->weighted_valid) return P4_TABLE_CONFLICT;
    if (work) {
        uint32_t role;
        int result = p4_game_elements_ai_vm_query_role(work, source, &role);
        if (result) return result;
    }
    carrier->table_work_guest = work ? P4_H2G(work) : 0u;
    carrier->condition_active = carrier->condition_seen = 0u;
    carrier->condition_epoch = carrier->candidate_epoch;
    carrier->selected.valid = carrier->decision.valid = carrier->weighted_valid = 0u;
    carrier->selected_permission = all_candidates(carrier);
    memset(carrier->condition_masks, 0, sizeof carrier->condition_masks);
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_decision_begin(void *source, void *work)
{
    int result = p4_game_elements_ai_native_root_begin(source, work);
    if (!result) p4_ai_native_carrier(source)->table_active = 1u;
    return result;
}
int p4_game_elements_ai_native_condition_begin(void *source, uint32_t row,
    uint32_t slot, uint32_t packed)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier || !carrier->table_active || carrier->condition_active || row >= 2u || slot >= 3u ||
        carrier->condition_epoch != carrier->candidate_epoch) return P4_TABLE_CONFLICT;
    if (!slot) {
        carrier->decision = (P4GameAiDecision){.valid = 1u, .row = row};
        memset(carrier->condition_masks, 0, sizeof carrier->condition_masks);
    }
    if (carrier->decision.row != row) return P4_TABLE_CONFLICT;
    carrier->condition_active = 1u;
    carrier->condition_seen = 0u;
    carrier->condition_slot = slot;
    carrier->decision.conditions[slot] = packed;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_condition_capture(void *source, const uint32_t *words)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier || !words) return P4_TABLE_INVALID;
    if (!carrier->table_active || !carrier->condition_active) return P4_TABLE_OK;
    if (carrier->condition_seen || carrier->condition_epoch != carrier->candidate_epoch) return P4_TABLE_CONFLICT;
    for (uint32_t i = 0; i < carrier->candidate_count; ++i)
        carrier->condition_words[i] = !!words[i];
    carrier->condition_seen = 1u;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_condition_end(void *source, uint32_t scalar)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier || !carrier->table_active || !carrier->condition_active ||
        carrier->condition_epoch != carrier->candidate_epoch) return P4_TABLE_CONFLICT;
    uint32_t mask = scalar ? all_candidates(carrier) : 0u;
    if (carrier->condition_seen) {
        mask = 0u;
        for (uint32_t i = 0; i < carrier->candidate_count; ++i)
            if (carrier->condition_words[i]) mask |= 1u << i;
        if (!!mask != !!scalar) return P4_TABLE_CONFLICT;
    }
    carrier->condition_masks[carrier->condition_slot] = mask;
    carrier->condition_active = carrier->condition_seen = 0u;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_decision_group(void *source, uint32_t row,
    uint32_t condition_mask, uint32_t group, uint32_t *eligible)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!eligible || !carrier || !carrier->table_active || carrier->condition_active ||
        carrier->condition_epoch != carrier->candidate_epoch || row > 2u || condition_mask > 7u ||
        group >= P4_GAME_AI_WEIGHT_GROUPS) return P4_TABLE_CONFLICT;
    uint32_t permission = all_candidates(carrier);
    for (uint32_t k = 0; k < 3u; ++k)
        if (condition_mask & (1u << k)) permission &= carrier->condition_masks[k];
    const uint8_t *record;
    int result = p4_game_elements_ai_native_source_record(source, &record);
    if (result) return result;
    uint32_t mask = 0u, has_weight = 0u, candidates[P4_GAME_AI_WEIGHT_ENTRIES];
    for (uint32_t i = 0; i < P4_GAME_AI_WEIGHT_ENTRIES; ++i) {
        candidates[i] = UINT32_MAX;
        const uint8_t *entry = record + 0x2cu + group * 40u + i * 8u;
        if (!entry[0]) continue;
        has_weight = 1u;
        uint32_t code = read16(entry + 2u), command = code == 0x8000u ? 0u : code;
        if (command & 0xf000u) {
            /* Plans/guard/escape/etc. keep the original decoder. They cannot
             * acquire permission from a nonexistent attack frame. */
            if (permission) mask |= 1u << i;
            continue;
        }
        uint32_t index;
        for (index = 0; index < carrier->candidate_count; ++index)
            if (carrier->available[index].command == command) break;
        if (index == carrier->candidate_count) return P4_TABLE_CONFLICT;
        if (!(permission & (1u << index))) continue;
        if (carrier->table_work_guest) {
            int allowed = p4_game_elements_ai_vm_candidate_allowed(P4_G2H(carrier->table_work_guest),
                source, &carrier->available[index].frame);
            if (allowed < 0) return allowed;
            if (!allowed) continue;
        }
        candidates[i] = index;
        mask |= 1u << i;
    }
    uint32_t zero_allowed = !has_weight && (permission & 1u);
    if (zero_allowed && carrier->table_work_guest) {
        int allowed = p4_game_elements_ai_vm_candidate_allowed(P4_G2H(carrier->table_work_guest),
            source, &carrier->available[0].frame);
        if (allowed < 0) return allowed;
        zero_allowed = !!allowed;
    }
    /* ELF0x763398 is {weight100, command8000, target0}. A zero-total
     * group returns THAT original basic-attack sentinel without drawing RNG;
     * it cannot borrow a paid candidate's condition permission. */
    *eligible = !!mask || zero_allowed;
    if (!*eligible) return P4_TABLE_OK;
    carrier->weighted_valid = 1u;
    carrier->weighted_group = group;
    carrier->weighted_mask = mask;
    memcpy(carrier->weighted_candidate, candidates, sizeof candidates);
    carrier->selected_permission = permission;
    if (row == 2u) carrier->decision = (P4GameAiDecision){.valid = 1u, .row = row};
    carrier->decision.condition_mask = condition_mask;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_decision_end(void *source)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier || !carrier->table_active || carrier->condition_active || !carrier->weighted_valid)
        return P4_TABLE_CONFLICT;
    carrier->table_active = 0u;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_decision_work(void *source, void **work)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier || !work) return P4_TABLE_INVALID;
    *work = NULL;
    if (!carrier->table_work_guest) return P4_TABLE_OK;
    uint32_t role;
    int result = p4_game_elements_ai_vm_query_role(P4_G2H(carrier->table_work_guest), source, &role);
    if (result) return result;
    *work = P4_G2H(carrier->table_work_guest);
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_root_end(void *source)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier || carrier->table_active || carrier->condition_active) return P4_TABLE_CONFLICT;
    carrier->table_work_guest = carrier->weighted_valid = carrier->decision.valid = 0u;
    carrier->selected_permission = 0u;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_weight(void *source, uint32_t group, uint32_t entry,
    uint32_t original_weight, uint32_t *weight)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier || !weight || entry >= P4_GAME_AI_WEIGHT_ENTRIES || group >= P4_GAME_AI_WEIGHT_GROUPS ||
        original_weight > 255u) return P4_TABLE_INVALID;
    if (carrier->weighted_valid && carrier->weighted_group != group) return P4_TABLE_CONFLICT;
    *weight = !carrier->weighted_valid || (carrier->weighted_mask & (1u << entry)) ? original_weight : 0u;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_weight_pick(void *source, uint32_t group, uint32_t entry)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier || group >= P4_GAME_AI_WEIGHT_GROUPS || entry >= P4_GAME_AI_WEIGHT_ENTRIES)
        return P4_TABLE_INVALID;
    if (!carrier->weighted_valid) return P4_TABLE_OK;
    if (carrier->weighted_group != group || !(carrier->weighted_mask & (1u << entry))) return P4_TABLE_CONFLICT;
    uint32_t index = carrier->weighted_candidate[entry];
    if (index == UINT32_MAX) { carrier->selected.valid = 0u; return P4_TABLE_OK; }
    if (index >= carrier->candidate_count) return P4_TABLE_CONFLICT;
    return p4_game_elements_ai_native_select(source, NULL, &carrier->available[index], &carrier->decision);
}
int p4_game_elements_ai_native_decoded(void *source, uint32_t kind, uint32_t command)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier) return P4_TABLE_NOT_READY;
    if (++carrier->choice_generation == 0u) return P4_TABLE_CONFLICT;
    if (kind != 1u && kind != 2u && kind != 3u) { carrier->selected.valid = 0u; return P4_TABLE_OK; }
    uint32_t index;
    for (index = 0; index < carrier->candidate_count; ++index)
        if (carrier->available[index].command == command) break;
    if (carrier->weighted_valid &&
        (index == carrier->candidate_count || !(carrier->selected_permission & (1u << index))))
        return P4_TABLE_CONFLICT;
    /* Preserve the actual collector's target-local prediction when decoding
     * its returned command. An existence-only collector must not publish here. */
    if (carrier->selected.valid && carrier->selected.candidate.command == command) {
        if (carrier->weighted_valid) carrier->selected.decision = carrier->decision;
        return P4_TABLE_OK;
    }
    P4GameAiSelection selection;
    int result = p4_game_elements_ai_native_command(source, command, &selection);
    if (result) return result;
    return p4_game_elements_ai_native_select(source, NULL, &selection.candidate,
        carrier->weighted_valid ? &carrier->decision : NULL);
}
int p4_game_elements_ai_native_condition_query(void *source, int32_t selector, uint32_t policy, uint32_t *value)
{
    if (!value) return P4_TABLE_INVALID;
    P4GameAiQueryPlan plan;
    int result = p4_game_elements_ai_native_plan_begin(NULL, source, 0u, selector, &plan);
    if (result) return result;
    uint32_t words[P4_GAME_AI_CANDIDATE_COUNT], invalid;
    result = p4_game_elements_ai_native_plan_predict(&plan, source, 0u, policy, 0, words, &invalid, value);
    if (result) return result;
    return p4_game_elements_ai_native_condition_capture(source, words);
}
