/* Boundary witnesses for the actual translated-unit AI body. The canonical
 * frame constructor/amount formula have their own element-cohort qualification;
 * these fixtures prove retained refs/queries, gates and ranking, not gameplay. */
#include "p4pc/game_knowledge_native.h"
#include "p4pc/game_skills_native.h"
#include "p4pc/game_elements_ai_native.h"
#include "p4pc/game_tables.h"
#include "p4pc/gmem.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* This boundary fixture must never reach the production controlled-failure
 * frontend; native recovery itself is qualified by the composed Core cohort. */
_Noreturn void p4_game_elements_ai_native_fail(void) { abort(); }

typedef struct DatUnit {
    uint32_t flags, hp;
    uint16_t skills[8];
    int8_t buffs[24];
} DatUnit;
static _Alignas(16) uint8_t arena[1024];
static DatUnit *source, *target;
static uint8_t action;
static uint8_t rows[(P4_SKILL_MAX_ID + 1u) * P4_SKILL_ROW_SIZE];
static P4GameActionFrame authored[P4_SKILL_MAX_ID + 1u];
static int32_t deltas[P4_SKILL_MAX_ID + 1u];
static uint32_t frame_calls, query_calls, effective_calls, amount_calls, clear_calls;
static uint32_t provided_amounts, actual_amounts;
static int unknown, usable;
static P4GameAiCandidate available[P4_GAME_AI_CANDIDATE_COUNT];
static uint32_t available_count, allowed_mask;
static P4GameAiSelection selected_snapshot, returned_snapshot;
static uint32_t base_calls, base_word;
static uint8_t table_record[P4_GAME_AI_RECORD_BYTES];
static uint32_t preview_calls;

static uint32_t datCalcGetTableLength(DatUnit *unit) { assert(unit); return 8; }
static uint8_t *datCalcSkillList(DatUnit *unit) { return (uint8_t *)unit->skills; }
static int8_t datCalcGetBuffStatus(uint8_t *unit, int32_t buff)
{
    assert(buff >= 0 && buff < 24);
    return ((DatUnit *)unit)->buffs[buff];
}
static void datCalcClearField18(uint32_t unit)
{
    assert(unit == P4_H2G(source) || unit == P4_H2G(target));
    ++clear_calls;
}
static int32_t btlCommandUsable(uint8_t *subject, uint32_t command)
{
    assert(subject == &action && command && command <= P4_SKILL_MAX_ID);
    return usable;
}
int p4_game_skill_active(uint32_t command)
{
    return command && command <= P4_SKILL_MAX_ID;
}
void p4_game_elements_native_primary_frame(void *unit, uint32_t command,
    uint32_t options, P4GameActionFrame *frame)
{
    assert(unit == source && command <= P4_SKILL_MAX_ID && options == 0);
    ++frame_calls;
    *frame = authored[command];
}
int p4_game_elements_ai_native_source_record(void *subject, const uint8_t **record)
{
    assert(subject == &action);
    *record = table_record;
    return 0;
}
int p4_game_elements_ai_native_preview_command(void *subject, uint32_t command, P4GameAiCandidate *candidate)
{
    assert(subject == &action && command <= P4_SKILL_MAX_ID);
    ++preview_calls;
    *candidate = (P4GameAiCandidate){.frame = authored[command], .command = command,
        .cost_kind = rows[command * P4_SKILL_ROW_SIZE + 8u], .eligibility = P4_GAME_AI_CANDIDATE_TABLE};
    return 0;
}
int p4_game_elements_ai_native_candidate_use(void *subject, uint32_t command, const P4GameActionFrame *frame)
{
    assert(subject == &action);
    P4GameActionFrame actual;
    p4_game_elements_native_primary_frame(source, command, frame->native_options, &actual);
    assert(!memcmp(&actual, frame, sizeof actual));
    return 0;
}
int p4_game_knowledge_native_ai_query(void *unit, const P4GameActionFrame *frame,
    P4GameAffinityQuery *query)
{
    assert(unit == target);
    assert(!memcmp(frame, &authored[frame->skill_id], sizeof *frame));
    ++query_calls;
    *query = unknown && p4_game_action_element_has_affinity(&frame->element) ?
        (P4GameAffinityQuery){P4_AFFINITY_QUERY_PROVIDED, 100} :
        (P4GameAffinityQuery){P4_AFFINITY_QUERY_ACTUAL, 0};
    return 0;
}
uint32_t p4_game_elements_native_element_affinity(void *unit, const P4GameActionFrame *frame)
{
    assert(unit == target && frame->element.valid == 1 && !unknown);
    ++effective_calls;
    return P4_AFF_REPEL | 100u;
}
uint32_t p4_game_elements_native_affinity(void *unit, const P4GameActionFrame *frame)
{
    assert(unit == target && frame->element.valid == 1);
    ++base_calls;
    return base_word;
}
int p4_game_elements_native_profile_base_affinity(void *unit, uint32_t leaf,
    uint32_t channel, uint32_t component, uint32_t *word)
{
    (void)leaf; (void)channel; (void)component;
    assert(unit == target); ++base_calls; *word = base_word; return 0;
}
int p4_game_elements_native_profile_affinity(void *unit, uint32_t leaf,
    uint32_t channel, uint32_t component, uint32_t *word)
{
    (void)leaf; (void)channel; (void)component;
    assert(unit == target); ++effective_calls; *word = P4_AFF_REPEL | 100u; return 0;
}
#include "../src/platform/mods/game_elements_ai_predicate.inc"
int p4_game_elements_ai_native_test(void *unit, const P4GameActionFrame *frame,
    uint32_t genus, uint32_t policy, uint32_t *value, P4GameAffinityQuery *query)
{
    return p4_ai_test_frame(unit, frame, genus, policy, 0u, value, query);
}
int p4_game_elements_ai_native_available_set(void *subject, const P4GameAiCandidate *items, uint32_t count)
{
    assert(subject == &action && count && count <= P4_GAME_AI_CANDIDATE_COUNT);
    memcpy(available, items, count * sizeof *items); available_count = count;
    return 0;
}
int p4_game_elements_ai_native_available(void *subject, const P4GameAiCandidate **items, uint32_t *count)
{
    assert(subject == &action);
    *items = available; *count = available_count; return 0;
}
int p4_game_elements_ai_vm_candidate_allowed(void *work, void *subject, const P4GameActionFrame *frame)
{
    assert(subject == &action);
    if (!work) return 1;
    for (uint32_t i = 0; i < available_count; ++i)
        if (!memcmp(frame, &available[i].frame, sizeof *frame)) return !!(allowed_mask & (1u << i));
    return 0;
}
int p4_game_elements_ai_native_select(void *subject, void *defender,
    const P4GameAiCandidate *candidate, const P4GameAiDecision *decision)
{
    assert(subject == &action && defender == target && !decision);
    selected_snapshot = (P4GameAiSelection){.valid = 1u, .candidate = *candidate};
    return 0;
}
int p4_game_elements_ai_native_selected(void *subject, P4GameAiSelection *out)
{
    assert(subject == &action && selected_snapshot.valid);
    *out = selected_snapshot; return 0;
}
int p4_game_elements_ai_native_clear_selection(void *subject)
{
    assert(subject == &action);
    selected_snapshot.valid = 0; return 0;
}
int p4_game_elements_ai_vm_pending_selection(void *work, const P4GameAiSelection *out, uint32_t scalar)
{
    assert(work && out->candidate.command == scalar);
    returned_snapshot = *out; return 0;
}
int32_t p4_game_elements_native_amount(uint32_t command, void *from, void *to,
    uint32_t hit_count, uint32_t outcome, uint32_t hit_kind, int32_t options,
    uint32_t amount_channel, const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{
    assert(from == source && to == target);
    assert(hit_count == 1 && outcome == 1 && hit_kind == 1 && options == 0 && amount_channel == 1);
    assert(command == frame->skill_id && !memcmp(frame, &authored[command], sizeof *frame));
    assert(frame->owned_binding == 1 && frame->source_id == command);
    if (unknown) {
        assert(query->mode == P4_AFFINITY_QUERY_PROVIDED && query->value == 100);
        ++provided_amounts;
    } else {
        assert(query->mode == P4_AFFINITY_QUERY_ACTUAL);
        ++actual_amounts;
    }
    ++amount_calls;
    return deltas[command];
}
#include "p4pc/game_knowledge_ai_body.inc"
#include "p4pc/game_elements_ai_script_body.inc"

static void reset(void)
{
    uint32_t i;
    memset(arena, 0, sizeof arena);
#ifdef P4_GUEST64
    p4_guest_base = arena;
#endif
    source = (DatUnit *)(arena + 0x100);
    target = (DatUnit *)(arena + 0x200);
    source->hp = 300; target->hp = 50;
    memset(rows, 0, sizeof rows);
    memset(authored, 0, sizeof authored);
    memset(deltas, 0, sizeof deltas);
    memset(table_record, 0, sizeof table_record);
    preview_calls = 0;
    for (i = 0; i <= P4_SKILL_MAX_ID; ++i) {
        authored[i] = (P4GameActionFrame){
            {1, i == P4_SKILL_MAX_ID ? UINT32_MAX : 65535u + i, UINT32_MAX,
             P4_ELEMENT_PHYSICAL | P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY,
             0, P4_ACTION_COMPONENT_DAMAGE}, i, 1, i, 0, 1};
        deltas[i] = -(int32_t)(10u + i);
    }
    for (i = 0; i < 8; ++i) source->skills[i] = (uint16_t)(i + 1u);
    source->skills[7] = P4_SKILL_MAX_ID;
    frame_calls = query_calls = effective_calls = amount_calls = clear_calls = 0;
    provided_amounts = actual_amounts = 0;
    unknown = usable = 1;
    available_count = base_calls = 0; allowed_mask = 0x1ffu; base_word = P4_AFF_NULL | 100u;
    selected_snapshot.valid = returned_snapshot.valid = 0;
}
static void all_nine_and_neutral_prediction(void)
{
    reset();
    /* Unknown observations precede even the native physical/buff block. */
    target->flags = 0x100; target->buffs[16] = 1;
    assert(p4_knowledge_ai_collect(NULL, &action, (uint8_t *)target, source, target, 1, 1, 0, 0, 0, NULL) == P4_SKILL_MAX_ID);
    assert(frame_calls == 9 && query_calls == 9 && amount_calls == 9);
    assert(provided_amounts == 9 && actual_amounts == 0 && effective_calls == 0 && clear_calls == 2);
    assert(authored[P4_SKILL_MAX_ID].element.element_id == UINT32_MAX);
    assert(authored[P4_SKILL_MAX_ID].element.channel_id == UINT32_MAX);
}
static void magical_filter_and_legacy_selectors(void)
{
    reset();
    authored[2].element.flags &= ~P4_ELEMENT_PHYSICAL;
    authored[2].element.family = UINT32_MAX;
    authored[2].element.element_id = UINT32_MAX;
    assert(p4_knowledge_ai_collect(NULL, &action, (uint8_t *)target, source, target, 1, 1, 1, 0, 0, NULL) == 2);
    assert(query_calls == 1 && amount_calls == 1 && effective_calls == 0);
    assert(!p4_knowledge_ai_legacy_match(&authored[P4_SKILL_MAX_ID], -1));
    assert(p4_knowledge_ai_legacy_match(&authored[P4_SKILL_MAX_ID], 0));
    authored[2].element.element_id = 6;
    assert(!p4_knowledge_ai_legacy_match(&authored[2], 6));
    assert(p4_knowledge_ai_effect_match(&authored[2], 0));
    authored[2].element.component = P4_ACTION_COMPONENT_INSTANT_KO;
    assert(p4_knowledge_ai_legacy_match(&authored[2], 6));
    assert(!p4_knowledge_ai_effect_match(&authored[2], 0));
    authored[2].element.valid = 0;
    assert(p4_knowledge_ai_legacy_match(&authored[2], -1));
    assert(!p4_knowledge_ai_legacy_match(&authored[2], 0));
}
static void native_gates_precede_real_rows(void)
{
    P4GameAiCandidate candidate;
    reset(); unknown = 0;
    candidate.frame = authored[0];
    target->buffs[16] = 1;
    assert(!p4_knowledge_ai_normal(target, 1, &candidate) && effective_calls == 0);
    target->buffs[16] = 0; target->flags = 0x100;
    assert(!p4_knowledge_ai_normal(target, 1, &candidate) && effective_calls == 0);
    target->flags = 0; target->skills[0] = 0x1f6;
    assert(!p4_knowledge_ai_normal(target, 0, &candidate) && effective_calls == 0);
    target->skills[0] = 0;
    assert(!p4_knowledge_ai_normal(target, 0, &candidate) && effective_calls == 1);
    authored[0].element.flags &= ~P4_ELEMENT_PHYSICAL;
    authored[0].element.family = 1;
    candidate.frame = authored[0]; target->buffs[17] = 1;
    assert(!p4_knowledge_ai_normal(target, 1, &candidate) && effective_calls == 1);
    authored[0].element.family = 5;
    candidate.frame = authored[0];
    assert(!p4_knowledge_ai_normal(target, 1, &candidate) && effective_calls == 2);
}
static void ranking_ties_and_cost_fallback(void)
{
    reset();
    memset(source->skills, 0, sizeof source->skills);
    source->skills[0] = 1; source->skills[1] = 2; source->skills[2] = 3;
    deltas[0] = -100; deltas[1] = -50; deltas[2] = -20; deltas[3] = -50;
    assert(p4_knowledge_ai_collect(NULL, &action, (uint8_t *)target, source, target, 1, 0, 0, 0, 0, NULL) == 1);
    /* First HP-cost pass has no matching class, then the original SP fallback. */
    assert(amount_calls == 4 && clear_calls == 4 && frame_calls == 4);
    reset();
    memset(source->skills, 0, sizeof source->skills);
    source->skills[0] = 1;
    deltas[0] = deltas[1] = -20;
    assert(p4_knowledge_ai_collect(NULL, &action, (uint8_t *)target, source, target, 1, 7, 0, 0, 0, NULL) == 0);
    assert(amount_calls == 2 && clear_calls == 2);
}
static void physical_family_skill_scan(void)
{
    reset();
    authored[0].element.flags &= ~P4_ELEMENT_PHYSICAL;
    authored[0].element.element_id = 1; authored[0].element.family = 1;
    memset(source->skills, 0, sizeof source->skills);
    source->skills[0] = P4_SKILL_MAX_ID;
    assert(p4_knowledge_ai_has_skill(&action, source, 0));
    usable = 0;
    assert(!p4_knowledge_ai_has_skill(&action, source, 0));
    assert(!p4_knowledge_ai_has_skill(&action, source, -1));
}
static void exact_selected_frame_and_permission(void)
{
    reset();
    p4_knowledge_ai_available_refresh(&action, source); /* Pure actual vm_before source hand-off. */
    allowed_mask = 1u << 1;
    assert(p4_knowledge_ai_collect(&action, &action, (uint8_t *)target, source, target,
        1, 1, 0, 0, 0, NULL) == 1);
    assert(amount_calls == 1 && returned_snapshot.valid && returned_snapshot.candidate.command == 1);
    assert(!memcmp(&returned_snapshot.candidate.frame, &authored[1], sizeof authored[1]));
    assert(returned_snapshot.candidate.query.mode == P4_AFFINITY_QUERY_PROVIDED);
    allowed_mask = 0;
    assert(p4_knowledge_ai_collect(&action, &action, (uint8_t *)target, source, target,
        1, 1, 0, 0, 0, NULL) == -1);
    assert(!selected_snapshot.valid && amount_calls == 1);
}
static void own_null_uses_base_not_normal_wrapper(void)
{
    uint32_t value;
    reset(); unknown = 0;
    assert(!p4_ai_test_frame(target, &authored[0], 1, P4_GAME_AI_TEST_NULL, 0u, &value, NULL));
    assert(value == 1 && base_calls == 1 && effective_calls == 0);
    assert(!p4_ai_test_frame(target, &authored[0], 1, P4_GAME_AI_TEST_REPEL, 0u, &value, NULL));
    assert(value == 0 && base_calls == 2 && effective_calls == 0);
}
static void side_barriers_precede_unknown_shortcuts(void)
{
    reset();
    uint32_t test, shortcut;
    target->buffs[0x10] = 1;
    assert(!p4_ai_side_frame_gate(target, &authored[0], 1, 0xA, &test, &shortcut));
    assert(!test && !shortcut && !query_calls && !base_calls && !effective_calls);
    target->buffs[0x10] = 0;
    assert(!p4_ai_side_frame_gate(target, &authored[0], 1, 0xA, &test, &shortcut));
    assert(!test && shortcut && query_calls == 1 && !base_calls && !effective_calls);
    assert(!p4_ai_side_frame_gate(target, &authored[0], 1, 6, &test, &shortcut));
    assert(!test && !shortcut && query_calls == 2);
    /* Both flags retain option4's exclusion priority. */
    assert(!p4_ai_side_frame_gate(target, &authored[0], 1, 14, &test, &shortcut));
    assert(!test && !shortcut && query_calls == 3);
    assert(!p4_ai_side_frame_gate(target, &authored[0], 0, 0xA, &test, &shortcut));
    assert(test && !shortcut && query_calls == 3);
    unknown = 0;
    assert(!p4_ai_side_frame_gate(target, &authored[0], 1, 0xA, &test, &shortcut));
    assert(test && !shortcut && query_calls == 4 && !base_calls && !effective_calls);
}
static void table_only_commands_keep_collector_domain(void)
{
    reset();
    for (uint32_t i = 0; i < 15u; ++i) {
        uint8_t *entry = table_record + 0x2cu + i * 8u;
        entry[0] = 1;
        entry[2] = (uint8_t)(100u + i);
    }
    p4_knowledge_ai_available_refresh(&action, source);
    assert(available_count == P4_GAME_AI_CANDIDATE_COUNT && preview_calls == 24);
    assert(!frame_calls && !query_calls && !amount_calls);
    for (uint32_t i = 9; i < available_count; ++i)
        assert(available[i].eligibility == P4_GAME_AI_CANDIDATE_TABLE);
    assert(p4_knowledge_ai_collect(NULL, &action, (uint8_t *)target, source, target,
        1, 0, 0, 0, 0, NULL) == P4_SKILL_MAX_ID);
    assert(amount_calls == 9); /* Lookahead is not another equipped skill. */
}
static void existence_probe_never_replaces_decoded_choice(void)
{
    reset();
    assert(p4_knowledge_ai_collect(NULL, &action, (uint8_t *)target, source, target,
        1, 0, 0, 0, 0, NULL) == P4_SKILL_MAX_ID);
    P4GameAiSelection saved = selected_snapshot;
    returned_snapshot = saved;
    uint32_t before = amount_calls;
    allowed_mask = 1u;
    assert(p4_knowledge_ai_collect_body(&action, &action, (uint8_t *)target, source, target,
        1, 1, 0, 0, 0, NULL, 0) == 0);
    assert(amount_calls == before + 1u);
    assert(!memcmp(&saved, &selected_snapshot, sizeof saved));
    assert(!memcmp(&saved, &returned_snapshot, sizeof saved));
    allowed_mask = 0;
    assert(p4_knowledge_ai_collect_body(&action, &action, (uint8_t *)target, source, target,
        1, 1, 0, 0, 0, NULL, 0) == -1);
    assert(!memcmp(&saved, &selected_snapshot, sizeof saved));
}
static void scoring_frame_equality_covers_every_member(void)
{
    P4GameActionFrame a = {0}, b = {0};
    assert(p4_ai_score_same_frame(&a, &b));
#define DIFFERENT_MEMBER(member) do { b = a; ++b.member; \
    assert(!p4_ai_score_same_frame(&a, &b)); } while (0)
    DIFFERENT_MEMBER(element.valid);
    DIFFERENT_MEMBER(element.element_id);
    DIFFERENT_MEMBER(element.channel_id);
    DIFFERENT_MEMBER(element.flags);
    DIFFERENT_MEMBER(element.family);
    DIFFERENT_MEMBER(element.component);
    DIFFERENT_MEMBER(skill_id);
    DIFFERENT_MEMBER(source_kind);
    DIFFERENT_MEMBER(source_id);
    DIFFERENT_MEMBER(native_options);
    DIFFERENT_MEMBER(owned_binding);
#undef DIFFERENT_MEMBER
}
int main(void)
{
    scoring_frame_equality_covers_every_member();
    all_nine_and_neutral_prediction();
    magical_filter_and_legacy_selectors();
    native_gates_precede_real_rows();
    ranking_ties_and_cost_fallback();
    physical_family_skill_scan();
    exact_selected_frame_and_permission();
    own_null_uses_base_not_normal_wrapper();
    side_barriers_precede_unknown_shortcuts();
    table_only_commands_keep_collector_domain();
    existence_probe_never_replaces_decoded_choice();
    return 0;
}
