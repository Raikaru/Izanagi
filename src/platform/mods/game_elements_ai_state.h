#ifndef P4PC_GAME_ELEMENTS_AI_STATE_H
#define P4PC_GAME_ELEMENTS_AI_STATE_H
#include "p4pc/game_elements_ai.h"

#define P4_AI_EXIT UINT32_MAX
#define P4_AI_UNUSED_OPCODE UINT16_MAX
#define P4_AI_PRODUCT_STAGED 1u
#define P4_AI_PRODUCT_ACTIVE 2u
#define P4_AI_TAG_NONE 0u
#define P4_AI_TAG_ELEMENT 1u
#define P4_AI_TAG_NUMBER 2u
#define P4_AI_TAG_COMMAND 3u
#define P4_AI_PROOF_SOURCES (P4_GAME_AI_CANDIDATE_COUNT + 1u)

/* A live battle action and its unit's identity, so a stored selection names
 * the same unit when it is used later. */
typedef struct P4AiTargetFact {
    P4GameAiOwner owner;
    P4GameUnitIdentity identity;
} P4AiTargetFact;

/* Host-only, immutable after CPU preparation except the verified late binding.
 * Calls use their continuation in this CFG: callee control scopes have their
 * own VM call depth and inherit, rather than prematurely end, caller scopes. */
typedef struct P4AiCfgNode {
    uint32_t successor[2], join, first_region, region_count;
    uint16_t opcode;
    int16_t operand;
    uint8_t query_role, command_arity;
} P4AiCfgNode;
typedef struct P4AiCfgRegion { uint32_t first, last; } P4AiCfgRegion;
typedef struct P4AiScriptProduct {
    const uint8_t *bytes;
    const void *source_identity;
    size_t size;
    uint32_t native_header, code_offset, code_count, procedure_offset, procedure_count;
    uint32_t label_offset, message_offset, string_offset, section_mask, message_count;
    uint32_t local_int_count, local_float_count, max_control_overlap;
    uint32_t source_used;
    P4AiCfgNode *nodes;
    P4AiCfgRegion *regions;
    size_t region_count, region_capacity;
} P4AiScriptProduct;

/* Typed VM values own their full selections: two live command-valued locals
 * must never alias one mutable last-selection cell. This metadata belongs to
 * the actual VM work-pool slot, not to a guessed single VM per battle actor. */
typedef struct P4AiValueVector {
    uint32_t words[P4_GAME_AI_CANDIDATE_COUNT];
    uint32_t count;
    P4GameAiOwner source;
    P4GameActionFrame explicit_frame;
} P4AiValueVector;
typedef struct P4AiValueTag {
    uint32_t generation, kind, actual_word, actual_type, invalid_mask, candidate_epoch;
    uint32_t constrained, explicit_valid, source_unrestricted;
    uint32_t proofs[P4_AI_PROOF_SOURCES];
    union {
        P4GameAiSelection selection;
        P4AiValueVector vector;
    } value;
} P4AiValueTag;
typedef struct P4AiControlScope {
    uint32_t branch_pc, call_depth, allowed_mask, candidate_epoch;
    uint32_t explicit_valid, source_unrestricted;
    P4GameActionFrame explicit_frame;
    uint32_t proofs[P4_AI_PROOF_SOURCES];
} P4AiControlScope;
typedef struct P4AiCarrier {
    uint32_t schema, generation, candidate_epoch, candidate_count;
    P4GameAiOwner owner;
    P4GameAiDecision decision;
    P4GameAiCandidate available[P4_GAME_AI_CANDIDATE_COUNT];
    uint32_t candidate_used[P4_GAME_AI_CANDIDATE_COUNT];
    uint32_t table_active, table_work_guest, condition_active, condition_seen, condition_slot, condition_epoch;
    uint32_t condition_words[P4_GAME_AI_CANDIDATE_COUNT], condition_masks[3];
    uint32_t selected_permission;
    P4GameAiSelection selected;
    uint32_t choice_generation;
    uint32_t weighted_valid, weighted_group, weighted_mask;
    uint32_t weighted_candidate[P4_GAME_AI_WEIGHT_ENTRIES];
} P4AiCarrier;
typedef struct P4AiVmState {
    uint32_t active, work_guest, program, generation, call_depth, control_count;
    P4GameAiOwner owner;
    uint32_t command_active, command_role, command_pc;
    uint32_t command_allowed_mask, running;
    uint32_t command_constrained, command_explicit_valid, command_source_unrestricted;
    P4GameActionFrame command_explicit_frame;
    uint32_t command_proofs[P4_AI_PROOF_SOURCES];
    uint32_t choice_valid, choice_generation;
    P4GameActionFrame choice_frame;
    uint32_t target_contract_valid, target_contract_known, target_contract_policy, target_contract_count;
    int32_t target_contract_rejected;
    P4GameAiQueryPlan target_contract_plan;
    uint32_t before_pc, before_sp, before_words[2], before_types[2], step_active;
    P4AiValueTag pending_return;
    P4AiValueTag stack[P4_GAME_AI_VM_STACK_SLOTS];
    /* Prepared local-int tags, local-float tags and control scopes follow. */
} P4AiVmState;

struct P4GameAiPrepared {
    const P4GameTables *tables;
    P4AiScriptProduct *scripts;
    size_t script_count;
    uint32_t local_int_capacity, local_float_capacity, control_capacity;
    uint32_t action_size, usage;
    uint32_t native_work_pool;
    size_t vm_stride;
    uint8_t *vm_arena;
    P4GameAiBankSource bank;
    uint32_t bank_count, bank_native_base, bank_source_used;
    uint32_t bank_player_script, bank_enemy_script;
};
int p4_ai_native_sources_quiescent(const P4GameAiPrepared *prepared);
P4GameAiPrepared *p4_ai_native_product(void);
P4AiCarrier *p4_ai_native_carrier(void *action);
void *p4_ai_native_unit_data(void *action);
void p4_ai_native_vm_running(int delta);
int p4_ai_native_target_fact(void *action, P4AiTargetFact *fact);
int p4_ai_script_basic_roles(P4AiScriptProduct *script);
#endif
