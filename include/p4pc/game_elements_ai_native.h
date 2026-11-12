#ifndef P4PC_GAME_ELEMENTS_AI_NATIVE_H
#define P4PC_GAME_ELEMENTS_AI_NATIVE_H
#include "p4pc/game_elements_native.h"
#include "p4pc/table_result.h"
#include <stdint.h>

/* Malformed expanded carrier state is an unattributable native runtime failure,
 * not a blocked-kind predicate, a host CRT abort or permission to pick a skill. */
_Noreturn void p4_game_elements_ai_native_fail(void);

/* Scalar-only translated-unit boundary. These records contain no host registry,
 * allocator or provenance pointers. Semantic element/channel/profile IDs remain
 * full u32; validity is independent of all ID bits. */
#define P4_GAME_AI_ACTION_BYTES 0x458u
#define P4_GAME_AI_EQUIPPED_SLOTS 8u
/* Three fixed five-entry AICALC groups can name paid commands outside the
 * eight equipped slots. Keep their exact frames without adding them to the
 * retail equipped-skill collector's eligibility set. */
#define P4_GAME_AI_WEIGHT_GROUPS 3u
#define P4_GAME_AI_WEIGHT_ENTRIES 5u
#define P4_GAME_AI_CANDIDATE_COUNT (P4_GAME_AI_EQUIPPED_SLOTS + 1u + \
    P4_GAME_AI_WEIGHT_GROUPS * P4_GAME_AI_WEIGHT_ENTRIES)
#define P4_GAME_AI_CANDIDATE_EQUIPPED 1u
#define P4_GAME_AI_CANDIDATE_TABLE 2u
/* A mod's authored substitution (p4_game_elements_ai_native_substitute): the
 * selection names that command; script permits and argument tags do not
 * apply to it. Never a source candidate's eligibility. */
#define P4_GAME_AI_CANDIDATE_AUTHORED 4u
#define P4_GAME_AI_VM_STACK_SLOTS 48u
#define P4_GAME_AI_VM_RETURN_SLOT 47u
#define P4_GAME_AI_VM_WORK_BYTES 0x158u
#define P4_GAME_AI_VM_WORK_PAYLOAD 0x160u
#define P4_GAME_AI_VM_WORK_SLOTS 3u
#define P4_GAME_AI_VM_LOCAL_INTS 200u
#define P4_GAME_AI_VM_LOCAL_FLOATS 8u
#define P4_GAME_AI_RECORD_BYTES 0xa4u
#define P4_GAME_AI_CONDITION_CALLBACKS 78u
#define P4_GAME_AI_TARGET_CALLBACKS 40u
#define P4_GAME_AI_QUERY_UNQUALIFIED 0u
#define P4_GAME_AI_QUERY_BASIC 1u
#define P4_GAME_AI_QUERY_AVAILABLE 2u
#define P4_GAME_AI_QUERY_SELECTED 3u
#define P4_GAME_AI_CARRIER_SCHEMA 1u
#define P4_GAME_AI_TEST_REPEL 1u
#define P4_GAME_AI_TEST_DRAIN 2u
#define P4_GAME_AI_TEST_NULL 3u
#define P4_GAME_AI_TEST_WEAK 4u
#define P4_GAME_AI_TEST_NORMAL 5u
#define P4_GAME_AI_TEST_NORMAL_NO_GATE 6u
#define P4_GAME_AI_TEST_NORMAL_STATUS 7u
#define P4_GAME_AI_TEST_NOT_NORMAL 8u
#define P4_GAME_AI_TEST_RESIST 9u

typedef struct P4GameAiCandidate {
    P4GameActionFrame frame;
    P4GameAffinityQuery query;
    /* A source candidate has no target query until its actual eligibility gate.
     * Never interpret an unbound zero-initialized query as known affinity. */
    uint32_t query_valid, command, cost_kind, eligibility;
} P4GameAiCandidate;
#define P4_GAME_AI_PLAN_FRAME 1u
#define P4_GAME_AI_PLAN_FAMILY 2u
#define P4_GAME_AI_PLAN_PROFILE 3u
#define P4_GAME_AI_PLAN_BASIC 4u
typedef struct P4GameAiQueryPlan {
    uint32_t source_guest, candidate_epoch, mode, script_work_guest, candidate_mask, selection_phase;
    P4GameActionFrame frame;
} P4GameAiQueryPlan;
typedef struct P4GameAiOwner {
    uint32_t action_guest, action_id, uid_low, uid_high;
} P4GameAiOwner;
typedef struct P4GameAiDecision {
    uint32_t valid, row, condition_mask, conditions[3];
} P4GameAiDecision;
typedef struct P4GameAiSelection {
    uint32_t valid;
    P4GameAiOwner source, target;
    P4GameUnitIdentity target_identity;
    P4GameAiCandidate candidate;
    P4GameAiDecision decision;
} P4GameAiSelection;

/* The first atomic AI prepare proves that no preactivation actions exist.
 * Only commit enables extension allocation/access; no-new-element mode keeps
 * the original allocation, clearing, interpreter and RNG behavior. */
int p4_game_elements_ai_native_enabled(void);
uint32_t p4_game_elements_ai_native_action_size(void);
void p4_game_elements_ai_native_action_init(void *action, uint32_t action_id,
    uint32_t uid_low, uint32_t uid_high);
void p4_game_elements_ai_native_action_release(void *action);
/* Bind at the actual command-selection root, not in a global last-action cell.
 * Query metadata belongs only to this exact defender and selected frame. */
int p4_game_elements_ai_native_select(void *source_action, void *target_action,
    const P4GameAiCandidate *candidate, const P4GameAiDecision *decision);
int p4_game_elements_ai_native_selected(void *source_action, P4GameAiSelection *selection);
int p4_game_elements_ai_native_command(void *source_action, uint32_t command,
    P4GameAiSelection *selection);
/* Translated AI unit adapter; explicit caller work carries VM permissions.
 * C-only callers retain the original legacy collector ABI. */
int32_t p4_game_elements_ai_collect_script(void *script_work, void *source_action,
    void *target_action, int32_t cost, int32_t filter, int32_t selector, int32_t use_selector);
/* Eligibility-only collector: preserves the original HP/cost/cache work but
 * never overwrites the actual decoded selection or this VM's pending return. */
int32_t p4_game_elements_ai_collect_probe(void *script_work, void *source_action,
    void *target_action, int32_t cost, int32_t filter);
int p4_game_elements_ai_eligible_mask(void *source_action, uint32_t *mask);
int p4_game_elements_ai_native_candidate_use(void *source_action, uint32_t command,
    const P4GameActionFrame *frame);
int p4_game_elements_ai_native_source_use(void *source_action);
int32_t p4_game_elements_ai_refresh_source(void *source_action);
int p4_game_elements_ai_native_clear_selection(void *source_action);
int p4_game_elements_ai_native_available_set(void *source_action,
    const P4GameAiCandidate *candidates, uint32_t count);
int p4_game_elements_ai_native_available(void *source_action,
    const P4GameAiCandidate **candidates, uint32_t *count);
/* 001DA230..001DA2F0 use BASE affinity. 001DB360/001DB9F0 use the wrapper
 * with their distinct original unknown/buff/physical policies. */
int p4_game_elements_ai_native_test(void *target_unit, const P4GameActionFrame *frame,
    uint32_t target_genus, uint32_t policy, uint32_t *value, P4GameAffinityQuery *prediction_query);
int p4_game_elements_ai_native_query(void *script_work, void *source_action, void *target_action,
    uint32_t parameter, int32_t selector, uint32_t policy, uint32_t *value);
int32_t p4_game_elements_ai_query_script(void *script_work, void *source_action, void *target_action,
    uint32_t parameter, int32_t selector, uint32_t policy);
int p4_game_elements_ai_native_plan_begin(void *script_work, void *source_action,
    uint32_t parameter, int32_t selector, P4GameAiQueryPlan *plan);
int p4_game_elements_ai_native_plan_mask(const P4GameAiQueryPlan *plan,
    void *target_action, uint32_t require_known, uint32_t *mask);
/* Candidate-source permission only: no result about one target binds which
 * targets the native command finally uses. */
int p4_game_elements_ai_native_selection_mask(const P4GameAiQueryPlan *plan,
    uint32_t mask, uint32_t *allowed_mask);
int p4_game_elements_ai_native_choice_allowed(void *script_work, void *source_action, uint32_t *allowed);
int p4_game_elements_ai_native_plan_test(const P4GameAiQueryPlan *plan,
    void *target_action, uint32_t mask, uint32_t policy, int32_t rejected, uint32_t *value);
int p4_game_elements_ai_native_plan_predict(const P4GameAiQueryPlan *plan,
    void *target_action, uint32_t require_known, uint32_t policy, int32_t rejected,
    uint32_t *words, uint32_t *invalid, uint32_t *value);
/* Side-ANY search keeps its barrier-before-knowledge order and the special
 * option8 unknown success before dead/status filtering. Masks use this plan's
 * own candidate domain; they are not target IDs or shared knowledge bits. */
int p4_game_elements_ai_native_side_prefilter(const P4GameAiQueryPlan *plan,
    void *target_action, uint32_t options, uint32_t *test_mask, uint32_t *unknown_mask);
int p4_game_elements_ai_native_side_project(const P4GameAiQueryPlan *plan,
    void *target_action, uint32_t test_mask, uint32_t unknown_mask, uint32_t policy,
    uint32_t status_eligible, uint32_t *words, uint32_t *value);
int32_t p4_game_elements_ai_find_script(void *script_work, void *source_action,
    uint32_t parameter, int32_t selector, uint32_t side_mask, uint32_t status_mask,
    uint32_t filter_flags, uint32_t policy);
int p4_game_elements_ai_native_side_mask(void *source_action, uint32_t opposite,
    uint32_t *side_mask);
int32_t p4_game_elements_ai_select_script(void *script_work, void *source_action,
    uint32_t parameter, int32_t selector, int32_t excluded_status, int32_t selection_flags,
    int32_t rejected_result, uint32_t policy);
int p4_game_elements_ai_vm_target_contract(void *script_work, void *source_action,
    const P4GameAiQueryPlan *plan, uint32_t require_known, uint32_t policy, int32_t rejected_result);
typedef struct P4GameAiVmToken { uint32_t slot, generation; } P4GameAiVmToken;
/* This callback reuses the actual interpreter's private canonical numeric
 * body. It evaluates only metadata shadows: no opcode/native-command replay,
 * RNG, external hook dispatch, affinity lookup or formula copy. */
typedef void (*P4GameAiVmShadowFn)(void *scratch, uint32_t opcode, uint32_t count,
    uint32_t left_type, const uint32_t *left_words,
    uint32_t right_type, const uint32_t *right_words,
    uint32_t *out_type, uint32_t *out_words, uint32_t *invalid_mask);
int p4_game_elements_ai_native_script_check(uint32_t header_guest, int32_t procedure);
/* Actual native consumption, not sealing/loading/byte verification. */
int p4_game_elements_ai_native_script_use(uint32_t header_guest);
/* The AICALC record's guest address: translated callers hold guest pointers
 * in 32-bit locals on the 64-bit port, so no host pointer is written through
 * their address. */
int p4_game_elements_ai_native_record(uint32_t index, uint32_t *row_guest);
int p4_game_elements_ai_native_source_record(void *source_action, const uint8_t **row);
int p4_game_elements_ai_native_preview_command(void *source_action, uint32_t command,
    P4GameAiCandidate *candidate);
int p4_game_elements_ai_native_root_begin(void *source_action, void *script_work);
int p4_game_elements_ai_native_decision_begin(void *source_action, void *script_work);
int p4_game_elements_ai_native_condition_begin(void *source_action,
    uint32_t row, uint32_t slot, uint32_t packed);
int p4_game_elements_ai_native_condition_capture(void *source_action, const uint32_t *words);
int p4_game_elements_ai_native_condition_end(void *source_action, uint32_t scalar);
int p4_game_elements_ai_native_decision_group(void *source_action,
    uint32_t row, uint32_t condition_mask, uint32_t group, uint32_t *eligible);
int p4_game_elements_ai_native_decision_end(void *source_action);
int p4_game_elements_ai_native_root_end(void *source_action);
int p4_game_elements_ai_native_decision_work(void *source_action, void **script_work);
int p4_game_elements_ai_native_weight(void *source_action, uint32_t group,
    uint32_t entry, uint32_t original_weight, uint32_t *weight);
int p4_game_elements_ai_native_weight_pick(void *source_action, uint32_t group, uint32_t entry);
int p4_game_elements_ai_native_decoded(void *source_action, uint32_t kind, uint32_t command);
/* A mod's authored substitution of the command that 001DE640 is about to
 * decode for this action. Its only caller is the Luau binding, which accepts
 * it solely inside that function's before-hook for the same action and then
 * writes the new code. The skill must be active and name no individual
 * target: self (row +0x0A bit 0) or every collected target (row +0x08
 * nonzero), so the game's own collector supplies the targets. With the
 * expanded carrier it authorizes exactly the next decode of that command for
 * this action, keeping the original draw's decision; the decode, a new
 * decision or the end of the routed call discards an unused authorization.
 * Returns NOT_FOUND for an inactive skill and CONFLICT for another scope. */
int p4_game_elements_ai_native_substitute(void *source_action, uint32_t skill);
/* p4.battle.decide (include/p4pc/battle_decide.h): after the AI released this
 * action's command record, which now names the authored command, rebind the
 * selected frame to it: kind 2 = that skill with any target scope (the decide
 * stage took its targets from the game's own collector), keeping the original
 * draw's decision; kind 7 = a pass, which selects no frame. Without the
 * expanded carrier there is nothing to bind. NOT_FOUND for an inactive skill. */
int p4_game_elements_ai_native_decide(void *source_action, uint32_t kind, uint32_t skill);
/* Script decodes pass through the hookable 001DE640 entry like retail BF
 * setters; their explicit caller work and argument rides on the carrier for
 * that one synchronous call. Outside it the entry has no script work. */
int p4_game_elements_ai_native_decode_begin(void *source_action, void *script_work,
    uint32_t command_parameter);
int p4_game_elements_ai_native_decode_work(void *source_action, void **script_work,
    uint32_t *command_parameter);
int p4_game_elements_ai_native_decode_end(void *source_action);
int p4_game_elements_ai_native_condition_query(void *source_action, int32_t selector,
    uint32_t policy, uint32_t *value);
int32_t p4_game_elements_ai_pick_table_script(void *script_work, void *source_action);
void p4_game_elements_ai_decode_script(void *script_work, void *source_action,
    void *command_work, uint32_t packed_action_code, uint32_t command_parameter);
int p4_game_elements_ai_vm_decoded(void *script_work, void *source_action,
    uint32_t command_parameter);
int p4_game_elements_ai_native_script_bind(void *script_work, void *source_action);
void p4_game_elements_ai_native_script_pool_release(void *pool, void *script_work);
/* enter: 1 actual bound AI work, 0 unbound ordinary/UI work, negative failure.
 * The token names this exact live native pool slot/generation, not an actor's
 * mutable current VM or an ambient last element. */
int p4_game_elements_ai_vm_enter(void *script_work, P4GameAiVmToken *token);
int p4_game_elements_ai_vm_before(void *script_work, const P4GameAiVmToken *token);
int p4_game_elements_ai_vm_after(void *script_work, const P4GameAiVmToken *token,
    uint32_t code_result, P4GameAiVmShadowFn shadow, void *scratch);
void p4_game_elements_ai_vm_leave(const P4GameAiVmToken *token);
void p4_game_elements_ai_vm_return_set(void *script_work);
int p4_game_elements_ai_vm_argument_selection(void *script_work, uint32_t parameter,
    void *source_action, P4GameAiSelection *selection);
int p4_game_elements_ai_vm_argument_element(void *script_work, uint32_t parameter,
    void *source_action, P4GameAiSelection *selection);
int p4_game_elements_ai_vm_pending_selection(void *script_work,
    const P4GameAiSelection *selection, uint32_t scalar);
int p4_game_elements_ai_vm_pending_element(void *script_work,
    const P4GameAiSelection *selection, uint32_t scalar);
int p4_game_elements_ai_vm_pending_numbers(void *script_work,
    const P4GameActionFrame *actual_query_frame, const uint32_t *words, uint32_t invalid_mask,
    uint32_t source_independent);
int p4_game_elements_ai_vm_query_role(void *script_work, void *source_action,
    uint32_t *role);
int p4_game_elements_ai_vm_candidate_allowed(void *script_work,
    void *source_action, const P4GameActionFrame *frame);
int p4_game_elements_ai_vm_commit_choice(void *script_work, void *source_action,
    const P4GameActionFrame *frame);
#endif
