#ifndef P4_TEST_ELEMENT_SKILL_INPUTS_H
#define P4_TEST_ELEMENT_SKILL_INPUTS_H
#include "p4pc/game_skills.h"
#include "p4pc/game_table_inputs.h"
#include "p4pc/packages.h"
#include <assert.h>
/* Actual Core/VFS transport over synthetic ISO banks, not runtime provenance
 * authority. Callbacks in the individual fixtures are explicit dispatch probes. */
typedef struct P4TestElementSkillInputs {
    P4GameSkills *skills;
    const P4GameTables *tables;
    P4GameText *names, *descriptions;
    P4GameTableInput *skill, *effect, *unit, *message;
} P4TestElementSkillInputs;
static void p4_test_element_inputs_release(void *opaque)
{
    P4TestElementSkillInputs *state = opaque;
    p4_game_table_input_destroy(state->skill);
    p4_game_table_input_destroy(state->effect);
    p4_game_table_input_destroy(state->unit);
    p4_game_table_input_destroy(state->message);
    state->skill = state->effect = state->unit = state->message = NULL;
}
static int p4_test_element_inputs_prepare(void *opaque)
{
    P4TestElementSkillInputs *state = opaque;
    if (p4_game_table_input_prepare(P4_TABLE_INPUT_SKILL, 1048576, &state->skill, NULL) ||
        p4_game_table_input_prepare(P4_TABLE_INPUT_EFFECT, 1048576, &state->effect, NULL) ||
        p4_game_table_input_prepare(P4_TABLE_INPUT_UNIT, 1048576, &state->unit, NULL) ||
        p4_game_table_input_prepare(P4_TABLE_INPUT_MSG, 1048576, &state->message, NULL)) return -1;
    if (p4_game_table_input_seal(state->skill) || p4_game_table_input_seal(state->effect) ||
        p4_game_table_input_seal(state->unit) ||
        p4_game_table_input_seal(state->message)) return -1;
    return p4_game_skills_seal(state->skills, p4_game_table_input_span(state->skill, 1),
        p4_game_table_input_span(state->skill, 0), p4_game_table_input_span(state->effect, 0),
        p4_game_table_input_span(state->message, 1), state->tables, state->names, state->descriptions, 1);
}
static void p4_test_element_inputs_publish(void *opaque)
{ assert(p4_game_skills_semantic_revision(((P4TestElementSkillInputs *)opaque)->skills) == 1); }
static void p4_test_element_inputs_begin(P4TestElementSkillInputs *state)
{
    const P4PackageHandler handler = {.name = "element-skill-fixture", .user = state,
        .prepare = p4_test_element_inputs_prepare, .publish = p4_test_element_inputs_publish,
        .rollback = p4_test_element_inputs_release, .boot_phase = P4_PACKAGE_BOOT_TABLES};
    assert(!p4_package_handler_register(&handler));
    assert(!p4_package_registry_publish());
}
static void p4_test_element_inputs_materialize(P4TestElementSkillInputs *state)
{
    const P4GameTableInputSpan *metadata = p4_game_table_input_span(state->skill, 1);
    const P4GameTableInputSpan *elements = p4_game_table_input_span(state->skill, 0);
    const P4GameTableInputSpan *presentation = p4_game_table_input_span(state->effect, 0);
    const P4GameTableInputSpan *names = p4_game_table_input_span(state->message, 1);
    assert(!p4_game_skills_materialize(state->skills,
        p4_game_table_input_span_bytes(metadata), p4_game_table_input_span_size(metadata),
        p4_game_table_input_span_bytes(elements), p4_game_table_input_span_size(elements),
        p4_game_table_input_span_bytes(presentation), p4_game_table_input_span_size(presentation),
        p4_game_table_input_span_bytes(names), p4_game_table_input_span_size(names)));
}
#endif
