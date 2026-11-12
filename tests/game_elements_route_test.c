/* The real 001F14F0 knowledge-gate route body with scripted frames and a
 * scripted knowledge owner: call order, KO gating and refusal only. */
#include "p4pc/game_elements_native.h"
#include "p4pc/game_knowledge_native.h"
#include "p4pc/table_result.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

static int defender, attacker;
static unsigned primary_calls, ko_calls, observed;
static uint32_t observed_leaves[4], observe_result, observe_fail_at;
static jmp_buf refused;
static int32_t refusal;

void p4_game_elements_native_primary_frame(void *unit, uint32_t skill, uint32_t options, P4GameActionFrame *frame)
{
    assert(unit == &attacker && skill == 120 && !options);
    ++primary_calls;
    memset(frame, 0, sizeof *frame);
    frame->element = (P4GameActionElement){1, 19, P4_CHANNEL_DAMAGE, P4_ELEMENT_PHYSICAL | P4_ELEMENT_OFFENSIVE |
        P4_ELEMENT_AFFINITY, 0, P4_ACTION_COMPONENT_DAMAGE};
    frame->skill_id = skill;
}
void p4_game_elements_native_action_frame(void *unit, uint32_t skill, uint32_t component,
    uint32_t status, uint32_t options, P4GameActionFrame *frame)
{
    assert(unit == &attacker && skill == 120 && component == P4_ACTION_COMPONENT_INSTANT_KO &&
        status == 0x80000u && !options);
    ++ko_calls;
    memset(frame, 0, sizeof *frame);
    frame->element = (P4GameActionElement){1, 6, P4_CHANNEL_INSTANT_KO, P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY,
        6, P4_ACTION_COMPONENT_INSTANT_KO};
    frame->skill_id = skill;
}
int p4_game_knowledge_native_observe(void *unit, const P4GameActionFrame *frame)
{
    assert(unit == &defender && frame->skill_id == 120 && observed < 4);
    observed_leaves[observed++] = frame->element.element_id;
    return observed == observe_fail_at ? (int)observe_result : P4_TABLE_OK;
}
_Noreturn void p4_game_elements_native_calculation_refuse(int32_t result)
{
    refusal = result;
    longjmp(refused, 1);
}
static void reset(void)
{
    primary_calls = ko_calls = observed = 0; observe_fail_at = 0; refusal = 0;
    memset(observed_leaves, 0, sizeof observed_leaves);
}
int main(void)
{
    reset();
    p4_game_elements_native_learn(&defender, &attacker, 120, 0);
    assert(primary_calls == 1 && !ko_calls && observed == 1 && observed_leaves[0] == 19);
    reset();
    p4_game_elements_native_learn(&defender, &attacker, 120, 0x80000u);
    assert(primary_calls == 1 && ko_calls == 1 && observed == 2);
    assert(observed_leaves[0] == 19 && observed_leaves[1] == 6);
    reset(); /* A failed primary observation refuses before any KO query. */
    observe_fail_at = 1; observe_result = P4_TABLE_NOT_FOUND;
    if (!setjmp(refused)) { p4_game_elements_native_learn(&defender, &attacker, 120, 0x80000u); assert(!"refusal expected"); }
    assert(refusal == P4_TABLE_NOT_FOUND && observed == 1 && !ko_calls);
    reset(); /* The KO observation is refused, never treated as learned. */
    observe_fail_at = 2; observe_result = P4_TABLE_INVALID;
    if (!setjmp(refused)) { p4_game_elements_native_learn(&defender, &attacker, 120, 0x80000u); assert(!"refusal expected"); }
    assert(refusal == P4_TABLE_INVALID && observed == 2 && ko_calls == 1);
    puts("game_elements_route: knowledge gate observes resolved primary/KO frames and refuses failures (scripted; not battle qualification)");
    return 0;
}
