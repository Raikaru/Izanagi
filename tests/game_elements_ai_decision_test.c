/* Production decision/weight carrier with labeled scalar boundaries. This does
 * not certify native BF/bank admission, actual RNG, or gameplay parity. */
#include "../src/platform/mods/game_elements_ai_state.h"
#include "guest_test.h"
#include "p4pc/game_skills_native.h"
#include <assert.h>
#include <string.h>

static P4AiCarrier carrier;
static unsigned char actor, record[P4_GAME_AI_RECORD_BYTES];
static unsigned selections, constructions, plan_calls, prediction_calls;
static uint32_t prediction[P4_GAME_AI_CANDIDATE_COUNT];
static int expanded = 1;
enum { SELF_SKILL = 227, ALL_SKILL = 145, SINGLE_SKILL = 7 };
int p4_game_elements_ai_native_enabled(void) { return expanded; }
int p4_game_skill_active(uint32_t id) { return id < P4_SKILL_RETAIL_ACTIVE_COUNT; }
P4AiCarrier *p4_ai_native_carrier(void *source)
{
    return source == &actor ? &carrier : NULL;
}
int p4_game_elements_ai_vm_query_role(void *work, void *source, uint32_t *role)
{
    (void)work; assert(source == &actor); *role = P4_GAME_AI_QUERY_UNQUALIFIED; return 0;
}
int p4_game_elements_ai_vm_candidate_allowed(void *work, void *source, const P4GameActionFrame *frame)
{
    (void)work; (void)frame; assert(source == &actor); return 1;
}
int p4_game_elements_ai_native_source_record(void *source, const uint8_t **out)
{
    assert(source == &actor); *out = record; return 0;
}
int p4_game_elements_ai_native_select(void *source, void *target,
    const P4GameAiCandidate *candidate, const P4GameAiDecision *decision)
{
    assert(source == &actor && !target);
    ++selections;
    carrier.selected = (P4GameAiSelection){.valid = 1, .candidate = *candidate};
    if (decision) carrier.selected.decision = *decision;
    return 0;
}
int p4_game_elements_ai_native_command(void *source, uint32_t command, P4GameAiSelection *out)
{
    assert(source == &actor); ++constructions;
    for (uint32_t i = 0; i < carrier.candidate_count; ++i)
        if (carrier.available[i].command == command) {
            *out = (P4GameAiSelection){.valid = 1, .candidate = carrier.available[i]}; return 0;
        }
    /* Any active skill outside the actor's sources, as the real constructor builds it. */
    *out = (P4GameAiSelection){.valid = 1, .candidate = {.command = command,
        .eligibility = P4_GAME_AI_CANDIDATE_TABLE}};
    out->candidate.frame.skill_id = command;
    return 0;
}
int p4_game_elements_ai_native_plan_begin(void *work, void *source, uint32_t parameter,
    int32_t selector, P4GameAiQueryPlan *out)
{
    assert(!work && source == &actor && !parameter && !selector);
    ++plan_calls; *out = (P4GameAiQueryPlan){0}; return 0;
}
int p4_game_elements_ai_native_plan_predict(const P4GameAiQueryPlan *plan, void *target,
    uint32_t known, uint32_t policy, int32_t rejected, uint32_t *words, uint32_t *invalid, uint32_t *value)
{
    (void)plan;
    assert(target == &actor && !known && policy == P4_GAME_AI_TEST_NULL && !rejected);
    ++prediction_calls; *value = *invalid = 0;
    for (uint32_t i = 0; i < carrier.candidate_count; ++i) {
        words[i] = prediction[i]; *value |= !!prediction[i];
    }
    return 0;
}
static void reset(void)
{
    memset(&carrier, 0, sizeof carrier); memset(record, 0, sizeof record);
    memset(prediction, 0, sizeof prediction);
    selections = constructions = plan_calls = prediction_calls = 0;
    carrier.candidate_epoch = 7; carrier.candidate_count = 3;
    const uint32_t commands[] = {0, 7, 300};
    const uint32_t leaves[] = {65535, 0x80000000u, UINT32_MAX};
    for (uint32_t i = 0; i < carrier.candidate_count; ++i) {
        P4GameAiCandidate *c = &carrier.available[i];
        c->command = c->frame.skill_id = commands[i];
        c->frame.element = (P4GameActionElement){1, leaves[i], 0,
            P4_ELEMENT_PHYSICAL | P4_ELEMENT_AFFINITY | P4_ELEMENT_OFFENSIVE, 0, P4_ACTION_COMPONENT_DAMAGE};
        c->eligibility = i == 2 ? P4_GAME_AI_CANDIDATE_TABLE : P4_GAME_AI_CANDIDATE_EQUIPPED;
    }
}
static void entry(uint32_t group, uint32_t index, uint32_t command, uint8_t weight)
{
    uint8_t *p = record + 0x2cu + group * 40u + index * 8u;
    p[0] = weight; p[2] = (uint8_t)command; p[3] = (uint8_t)(command >> 8);
}
static void condition(uint32_t slot, uint32_t mask, uint32_t scalar)
{
    uint32_t words[P4_GAME_AI_CANDIDATE_COUNT] = {0};
    assert(!p4_game_elements_ai_native_condition_begin(&actor, 0, slot, 0x80000000u | slot));
    for (uint32_t i = 0; i < carrier.candidate_count; ++i) words[i] = !!(mask & (1u << i));
    assert(!p4_game_elements_ai_native_condition_capture(&actor, words));
    assert(!p4_game_elements_ai_native_condition_end(&actor, scalar));
}
static void different_frames_do_not_conjoin(void)
{
    reset(); entry(0, 0, 0x8000, 60); entry(0, 1, 7, 40);
    assert(!p4_game_elements_ai_native_decision_begin(&actor, NULL));
    condition(0, 1, 1); condition(1, 2, 1);
    uint32_t eligible = 99;
    assert(!p4_game_elements_ai_native_decision_group(&actor, 0, 3, 0, &eligible) && !eligible);
    assert(!carrier.weighted_valid && !selections);
    assert(!p4_game_elements_ai_native_decision_group(&actor, 0, 1, 0, &eligible) && eligible);
    assert(carrier.weighted_mask == 1 && carrier.selected_permission == 1);
    uint32_t weight;
    assert(!p4_game_elements_ai_native_weight(&actor, 0, 0, 60, &weight) && weight == 60);
    assert(!p4_game_elements_ai_native_weight(&actor, 0, 1, 40, &weight) && !weight);
    assert(p4_game_elements_ai_native_weight_pick(&actor, 0, 1) == P4_TABLE_CONFLICT);
    assert(!p4_game_elements_ai_native_decision_end(&actor));
    assert(!p4_game_elements_ai_native_weight_pick(&actor, 0, 0));
    assert(carrier.selected.candidate.command == 0 && carrier.selected.candidate.frame.element.element_id == 65535);
    assert(carrier.selected.decision.conditions[0] == 0x80000000u && carrier.selected.decision.condition_mask == 1);
    assert(!p4_game_elements_ai_native_decoded(&actor, 1, 0) && !constructions);
    assert(!p4_game_elements_ai_native_root_end(&actor));
    assert(carrier.selected.valid && !carrier.weighted_valid && !carrier.table_work_guest);
}
static void zero_total_is_only_the_original_basic_sentinel(void)
{
    reset(); assert(!p4_game_elements_ai_native_decision_begin(&actor, NULL));
    condition(0, 2, 1);
    uint32_t eligible;
    assert(!p4_game_elements_ai_native_decision_group(&actor, 0, 1, 0, &eligible) && !eligible);
    assert(!p4_game_elements_ai_native_decision_group(&actor, 2, 0, 0, &eligible) && eligible);
    assert(!carrier.weighted_mask && carrier.selected_permission == 7 && !selections);
    assert(!p4_game_elements_ai_native_decision_end(&actor));
    assert(!p4_game_elements_ai_native_decoded(&actor, 1, 0));
    assert(constructions == 1 && selections == 1 && carrier.selected.candidate.command == 0);
    assert(!p4_game_elements_ai_native_root_end(&actor));
}
static void table_command_keeps_full_identity(void)
{
    reset(); entry(1, 4, 300, 100);
    assert(!p4_game_elements_ai_native_decision_begin(&actor, NULL)); condition(0, 4, 1);
    uint32_t eligible;
    assert(!p4_game_elements_ai_native_decision_group(&actor, 0, 1, 1, &eligible) && eligible);
    assert(carrier.weighted_mask == 16 && carrier.weighted_candidate[4] == 2);
    assert(!p4_game_elements_ai_native_decision_end(&actor));
    assert(!p4_game_elements_ai_native_weight_pick(&actor, 1, 4));
    assert(carrier.selected.candidate.frame.element.element_id == UINT32_MAX);
    assert(!p4_game_elements_ai_native_decoded(&actor, 2, 300) && !constructions);
    assert(!p4_game_elements_ai_native_root_end(&actor));
    assert(!p4_game_elements_ai_native_decoded(&actor, 9, 0x111) && !carrier.selected.valid);
}
static void one_query_and_epoch_bound_capture(void)
{
    reset(); assert(!p4_game_elements_ai_native_decision_begin(&actor, NULL));
    assert(!p4_game_elements_ai_native_condition_begin(&actor, 0, 0, 17)); prediction[2] = 1;
    uint32_t value;
    assert(!p4_game_elements_ai_native_condition_query(&actor, 0, P4_GAME_AI_TEST_NULL, &value) && value);
    assert(plan_calls == 1 && prediction_calls == 1);
    assert(!p4_game_elements_ai_native_condition_end(&actor, value) && carrier.condition_masks[0] == 4);
    assert(!p4_game_elements_ai_native_condition_begin(&actor, 0, 1, 18));
    assert(!p4_game_elements_ai_native_condition_end(&actor, 1) && carrier.condition_masks[1] == 7);
    ++carrier.candidate_epoch;
    uint32_t eligible = 42;
    assert(p4_game_elements_ai_native_decision_group(&actor, 0, 1, 0, &eligible) == P4_TABLE_CONFLICT && eligible == 42);
    assert(!selections && !constructions);
}
static void weighted_pick_of_seven(void)
{
    reset(); entry(0, 0, SINGLE_SKILL, 100);
    assert(!p4_game_elements_ai_native_decision_begin(&actor, NULL)); condition(0, 2, 1);
    uint32_t eligible;
    assert(!p4_game_elements_ai_native_decision_group(&actor, 0, 1, 0, &eligible) && eligible);
    assert(!p4_game_elements_ai_native_decision_end(&actor));
    assert(!p4_game_elements_ai_native_weight_pick(&actor, 0, 0));
    assert(carrier.selected.candidate.command == SINGLE_SKILL);
}
static void authored_substitution_replaces_only_its_decode(void)
{
    weighted_pick_of_seven();
    unsigned built = constructions;
    assert(p4_game_elements_ai_native_substitute(&actor, SINGLE_SKILL) == P4_TABLE_CONFLICT);
    assert(p4_game_elements_ai_native_substitute(&actor, P4_SKILL_RETAIL_ACTIVE_COUNT) == P4_TABLE_NOT_FOUND);
    assert(!carrier.substitute_valid);
    /* Unauthorized, a command outside the drawn group's permission is refused. */
    assert(p4_game_elements_ai_native_decoded(&actor, 2, SELF_SKILL) == P4_TABLE_CONFLICT);
    assert(!p4_game_elements_ai_native_substitute(&actor, SELF_SKILL) && carrier.substitute_valid);
    assert(!p4_game_elements_ai_native_decoded(&actor, 2, SELF_SKILL) && !carrier.substitute_valid);
    assert(carrier.selected.candidate.command == SELF_SKILL && constructions == built + 1);
    assert(carrier.selected.candidate.eligibility & P4_GAME_AI_CANDIDATE_AUTHORED);
    assert(carrier.selected.decision.conditions[0] == 0x80000000u); /* The original draw's decision. */
    /* One shot: the next decode of that command is ordinary again. */
    assert(p4_game_elements_ai_native_decoded(&actor, 2, SELF_SKILL) == P4_TABLE_CONFLICT);
    /* It covers only the command it named, and that decode discards it. */
    assert(!p4_game_elements_ai_native_substitute(&actor, ALL_SKILL));
    assert(p4_game_elements_ai_native_decoded(&actor, 2, SELF_SKILL) == P4_TABLE_CONFLICT && !carrier.substitute_valid);
    assert(!p4_game_elements_ai_native_substitute(&actor, ALL_SKILL));
    assert(!p4_game_elements_ai_native_root_end(&actor) && !carrier.substitute_valid);
}
static void script_decode_window(void)
{
    reset();
    void *work = p4t_galloc(P4_GAME_AI_VM_WORK_PAYLOAD, 16), *found = &actor;
    uint32_t parameter = 0;
    assert(work);
    assert(!p4_game_elements_ai_native_decode_work(&actor, &found, &parameter) && !found && parameter == UINT32_MAX);
    assert(!p4_game_elements_ai_native_substitute(&actor, ALL_SKILL));
    assert(!p4_game_elements_ai_native_decode_begin(&actor, work, 1) && !carrier.substitute_valid);
    assert(p4_game_elements_ai_native_decode_begin(&actor, work, 1) == P4_TABLE_CONFLICT);
    assert(!p4_game_elements_ai_native_decode_work(&actor, &found, &parameter) && found == work && parameter == 1);
    assert(!p4_game_elements_ai_native_substitute(&actor, ALL_SKILL) && carrier.substitute_valid);
    /* A replacing hook may skip the decode: the end of the call discards it. */
    assert(!p4_game_elements_ai_native_decode_end(&actor) && !carrier.substitute_valid);
    assert(p4_game_elements_ai_native_decode_end(&actor) == P4_TABLE_CONFLICT);
    assert(!p4_game_elements_ai_native_decode_work(&actor, &found, &parameter) && !found && parameter == UINT32_MAX);
}
static void retail_mode_checks_scope_only(void)
{
    reset(); expanded = 0;
    assert(!p4_game_elements_ai_native_substitute(&actor, SELF_SKILL) && !carrier.substitute_valid);
    assert(p4_game_elements_ai_native_substitute(&actor, SINGLE_SKILL) == P4_TABLE_CONFLICT);
    expanded = 1;
}
static void decide_rebinds_any_scope_after_release(void)
{
    weighted_pick_of_seven();
    unsigned built = constructions;
    /* p4.battle.decide: the released record names a single-target skill the
     * drawn group never permitted; the selected frame follows it, authored,
     * keeping the original draw's decision, and no authorization is left. */
    assert(!p4_game_elements_ai_native_decide(&actor, 2, ALL_SKILL + 1));
    assert(carrier.selected.valid && carrier.selected.candidate.command == ALL_SKILL + 1);
    assert(carrier.selected.candidate.eligibility & P4_GAME_AI_CANDIDATE_AUTHORED);
    assert(carrier.selected.decision.conditions[0] == 0x80000000u && constructions == built + 1);
    assert(!carrier.substitute_valid);
    assert(p4_game_elements_ai_native_decide(&actor, 2, P4_SKILL_RETAIL_ACTIVE_COUNT) == P4_TABLE_NOT_FOUND);
    assert(p4_game_elements_ai_native_decide(&actor, 3, SELF_SKILL) == P4_TABLE_INVALID);
    /* A pass selects no frame. */
    assert(!p4_game_elements_ai_native_decide(&actor, 7, 0) && !carrier.selected.valid);
    /* Without the expanded carrier there is nothing to bind. */
    expanded = 0;
    assert(!p4_game_elements_ai_native_decide(&actor, 2, SINGLE_SKILL));
    expanded = 1;
}
int main(void)
{
    if (p4t_guest_map(0x00764000u, 0x2000u)) return 77;
    uint8_t *rows = p4t_galloc(P4_SKILL_RETAIL_ACTIVE_COUNT * P4_SKILL_ROW_SIZE, 16);
    assert(rows);
    rows[SELF_SKILL * P4_SKILL_ROW_SIZE + 0x0a] = 1; /* Target filter: the actor itself. */
    rows[ALL_SKILL * P4_SKILL_ROW_SIZE + 0x08] = 1; /* Target class: every collected unit. */
    P4_G32(0x007644a8u) = P4_H2G(rows);
    different_frames_do_not_conjoin();
    zero_total_is_only_the_original_basic_sentinel();
    table_command_keeps_full_identity();
    one_query_and_epoch_bound_capture();
    authored_substitution_replaces_only_its_decode();
    script_decode_window();
    retail_mode_checks_scope_only();
    decide_rebinds_any_scope_after_release();
    return 0;
}
