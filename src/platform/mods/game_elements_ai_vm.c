#include "game_elements_ai_state.h"
#include "game_elements_ai_commands.h"
#include "p4pc/gmem.h"
#include <stdlib.h>
#include <string.h>
extern uint8_t *btlActionFindById(int32_t id);
extern uint32_t datCalcGetBadStatusNoDown(void *unit);

/* Parallel metadata for the ACTUAL three native work-pool slots. No guest
 * padding, actor->last-element global, pointer-keyed map or runtime allocation.
 * proofs[i] is whether a value holds for candidate source i (the last slot:
 * the explicit or unrestricted source). It is never about a defender: the
 * original script may ask about unit B and act on C, so no result about a
 * target binds the targets the native command finally uses. */
typedef struct Permit {
    uint32_t mask, constrained, explicit_valid, source_unrestricted;
    P4GameActionFrame explicit_frame;
    uint32_t proofs[P4_AI_PROOF_SOURCES];
} Permit;
static int finalize_choice(P4AiVmState *vm);
static int same_owner(const P4GameAiOwner *a, const P4GameAiOwner *b)
{
    return a->action_guest == b->action_guest && a->action_id == b->action_id &&
        a->uid_low == b->uid_low && a->uid_high == b->uid_high;
}
static int same_frame(const P4GameActionFrame *a, const P4GameActionFrame *b)
{
    return !memcmp(a, b, sizeof *a);
}
static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static uint32_t word(uint32_t work, uint32_t index) { return P4_G32(work + 0x50u + index * 4u); }
static uint32_t type(uint32_t work, uint32_t index) { return P4_G8(work + 0x20u + index); }
static uint32_t all_mask(const P4AiCarrier *carrier) { return (1u << carrier->candidate_count) - 1u; }
static P4AiValueTag *int_tags(P4AiVmState *vm) { return (P4AiValueTag *)(vm + 1); }
static P4AiValueTag *float_tags(P4GameAiPrepared *product, P4AiVmState *vm)
{
    return int_tags(vm) + product->local_int_capacity;
}
static P4AiControlScope *controls(P4GameAiPrepared *product, P4AiVmState *vm)
{
    return (P4AiControlScope *)(float_tags(product, vm) + product->local_float_capacity);
}
static P4AiVmState *slot_for_work(P4GameAiPrepared *product, void *pointer, uint32_t *index)
{
    if (!product || !pointer || !product->native_work_pool) return NULL;
    uint32_t guest = P4_H2G(pointer), first = product->native_work_pool + 0x18u;
    uint32_t stride = P4_GAME_AI_VM_WORK_PAYLOAD + 8u;
    if (guest < first || (guest - first) % stride) return NULL;
    uint32_t slot = (guest - first) / stride;
    if (slot >= P4_GAME_AI_VM_WORK_SLOTS) return NULL;
    if (index) *index = slot;
    return (P4AiVmState *)(product->vm_arena + slot * product->vm_stride);
}
static P4AiVmState *bound_work(void *work)
{
    P4GameAiPrepared *product = p4_ai_native_product();
    P4AiVmState *vm = slot_for_work(product, work, NULL);
    return vm && vm->active && vm->work_guest == P4_H2G(work) ? vm : NULL;
}
static P4AiVmState *from_token(P4GameAiPrepared *product, const P4GameAiVmToken *token)
{
    if (!product || !token || token->slot >= P4_GAME_AI_VM_WORK_SLOTS) return NULL;
    P4AiVmState *vm = (P4AiVmState *)(product->vm_arena + token->slot * product->vm_stride);
    return vm->active && vm->running && vm->generation == token->generation ? vm : NULL;
}
static P4AiCarrier *vm_carrier(const P4AiVmState *vm)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(P4_G2H(vm->owner.action_guest));
    return carrier && same_owner(&carrier->owner, &vm->owner) && carrier->candidate_count ? carrier : NULL;
}
static int validate_work(P4GameAiPrepared *product, const P4AiVmState *vm)
{
    if (vm->program >= product->script_count || !vm_carrier(vm)) return P4_TABLE_NOT_READY;
    const P4AiScriptProduct *script = &product->scripts[vm->program];
    uint32_t work = vm->work_guest, header = script->native_header;
    if (!header || P4_G32(work + 0x110u) != header || P4_G32(work + 0x114u) != header + 32u ||
        P4_G32(work + 0x118u) != header + script->procedure_offset ||
        P4_G32(work + 0x120u) != header + script->code_offset ||
        P4_G32(work + 0x154u) != vm->owner.action_guest || P4_G32(work + 0x1cu) > 47u)
        return P4_TABLE_CONFLICT;
    uint32_t labels = script->section_mask & 2u ? header + script->label_offset : 0u;
    uint32_t messages = script->message_count ? header + script->message_offset : 0u;
    uint32_t strings = script->section_mask & 16u ? header + script->string_offset : 0u;
    if (P4_G32(work + 0x11cu) != labels || P4_G32(work + 0x124u) != messages ||
        P4_G32(work + 0x128u) != strings) return P4_TABLE_CONFLICT;
    uint32_t pool = P4_G32(0x00764614u), local = P4_G32(work + 0x140u);
    if (!pool || local < pool + 0x18u || (local - pool - 0x18u) % 0x348u ||
        (local - pool - 0x18u) / 0x348u >= P4_GAME_AI_VM_WORK_SLOTS ||
        P4_G32(work + 0x144u) != local + 0x320u) return P4_TABLE_CONFLICT;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_script_check(uint32_t header, int32_t procedure)
{
    P4GameAiPrepared *product = p4_ai_native_product();
    if (!product) return P4_TABLE_NOT_READY;
    for (size_t i = 0; i < product->script_count; ++i) {
        const P4AiScriptProduct *script = &product->scripts[i];
        if (script->native_header == header)
            return procedure >= 0 && (uint32_t)procedure < script->procedure_count ? P4_TABLE_OK : P4_TABLE_INVALID;
    }
    return P4_TABLE_NOT_FOUND;
}
int p4_game_elements_ai_native_script_bind(void *work, void *source)
{
    P4GameAiPrepared *product = p4_ai_native_product();
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    P4AiVmState *vm = slot_for_work(product, work, NULL);
    if (!vm || !carrier || !carrier->candidate_count) return P4_TABLE_NOT_READY;
    if (vm->active || vm->running) return P4_TABLE_CONFLICT;
    uint32_t guest = P4_H2G(work), header = P4_G32(guest + 0x110u);
    size_t program;
    for (program = 0; program < product->script_count; ++program)
        if (product->scripts[program].native_header == header) break;
    if (program == product->script_count) return P4_TABLE_NOT_FOUND;
    if (++vm->generation == 0u) abort();
    vm->choice_valid = 0u;
    vm->work_guest = guest; vm->program = (uint32_t)program; vm->owner = carrier->owner;
    vm->call_depth = vm->control_count = vm->command_active = vm->running = vm->step_active = 0;
    vm->pending_return.kind = P4_AI_TAG_NONE;
    int result = validate_work(product, vm);
    if (result) return result;
    vm->active = 1;
    return P4_TABLE_OK;
}
void p4_game_elements_ai_native_script_pool_release(void *pool, void *work)
{
    P4GameAiPrepared *product = p4_ai_native_product();
    if (!product || P4_H2G(pool) != product->native_work_pool) return;
    P4AiVmState *vm = slot_for_work(product, work, NULL);
    if (!vm) abort();
    if (vm->running) abort();
    if (vm->active && finalize_choice(vm)) abort();
    vm->active = vm->step_active = vm->command_active = vm->control_count = 0;
    vm->pending_return.kind = P4_AI_TAG_NONE;
}
int p4_game_elements_ai_vm_enter(void *work, P4GameAiVmToken *token)
{
    if (!token) return P4_TABLE_INVALID;
    P4GameAiPrepared *product = p4_ai_native_product();
    uint32_t slot;
    P4AiVmState *vm = slot_for_work(product, work, &slot);
    if (!vm || !vm->active) return 0;
    if (vm->running) return P4_TABLE_CONFLICT;
    int result = validate_work(product, vm);
    if (result) return result;
    result = p4_game_elements_ai_native_script_use(product->scripts[vm->program].native_header);
    if (result) return result;
    vm->running = 1;
    p4_ai_native_vm_running(1);
    *token = (P4GameAiVmToken){slot, vm->generation};
    return 1;
}
void p4_game_elements_ai_vm_leave(const P4GameAiVmToken *token)
{
    P4AiVmState *vm = from_token(p4_ai_native_product(), token);
    if (!vm || vm->step_active || vm->command_active) abort();
    vm->running = 0;
    p4_ai_native_vm_running(-1);
}
static const P4AiValueTag *live_tag(const P4AiVmState *vm, const P4AiValueTag *tag)
{
    return tag->generation == vm->generation && tag->kind != P4_AI_TAG_NONE ? tag : NULL;
}
static int checked_tag(const P4AiVmState *vm, const P4AiValueTag *tag, uint32_t actual_type, uint32_t actual_word)
{
    const P4AiValueTag *value = live_tag(vm, tag);
    if (!value) return P4_TABLE_OK;
    P4AiCarrier *carrier = vm_carrier(vm);
    if (!carrier || value->actual_type != actual_type || value->actual_word != actual_word ||
        (value->constrained && value->candidate_epoch != carrier->candidate_epoch)) return P4_TABLE_CONFLICT;
    const P4GameAiOwner *owner = value->kind == P4_AI_TAG_NUMBER ?
        &value->value.vector.source : &value->value.selection.source;
    return same_owner(owner, &vm->owner) ? P4_TABLE_OK : P4_TABLE_CONFLICT;
}
static int permit_frame(const Permit *permit, const P4AiCarrier *carrier, const P4GameActionFrame *frame)
{
    if (!permit->constrained || permit->source_unrestricted) return 1;
    for (uint32_t i = 0; i < carrier->candidate_count; ++i)
        if ((permit->mask & (1u << i)) && same_frame(frame, &carrier->available[i].frame)) return 1;
    return permit->explicit_valid && same_frame(frame, &permit->explicit_frame);
}
static uint32_t frame_proof(const Permit *permit, const P4AiCarrier *carrier, const P4GameActionFrame *frame)
{
    if (!permit->constrained) return 1u;
    if (permit->source_unrestricted) return permit->proofs[P4_GAME_AI_CANDIDATE_COUNT];
    for (uint32_t i = 0; i < carrier->candidate_count; ++i)
        if ((permit->mask & (1u << i)) && same_frame(frame, &carrier->available[i].frame))
            return permit->proofs[i];
    return permit->explicit_valid && same_frame(frame, &permit->explicit_frame) &&
        permit->proofs[P4_GAME_AI_CANDIDATE_COUNT];
}
static void merge_permit(Permit *permit, uint32_t mask, uint32_t explicit_valid, uint32_t source_unrestricted,
    const P4GameActionFrame *explicit_frame, const uint32_t *proofs)
{
    permit->mask &= mask;
    for (uint32_t i = 0; i < P4_GAME_AI_CANDIDATE_COUNT; ++i) {
        permit->proofs[i] = (permit->mask & (1u << i)) && permit->proofs[i] && proofs[i];
        if (!permit->proofs[i]) permit->mask &= ~(1u << i);
    }
    uint32_t previous_any = !permit->constrained || permit->source_unrestricted;
    if (previous_any) {
        permit->explicit_valid = explicit_valid;
        if (explicit_valid) permit->explicit_frame = *explicit_frame;
    } else if (!source_unrestricted &&
        (!explicit_valid || !permit->explicit_valid || !same_frame(&permit->explicit_frame, explicit_frame))) {
        permit->explicit_valid = 0u;
    }
    permit->source_unrestricted = previous_any && source_unrestricted;
    uint32_t *generic = &permit->proofs[P4_GAME_AI_CANDIDATE_COUNT];
    *generic = (permit->explicit_valid || permit->source_unrestricted) && *generic &&
        proofs[P4_GAME_AI_CANDIDATE_COUNT];
    if (!*generic) permit->explicit_valid = permit->source_unrestricted = 0u;
    permit->constrained = 1;
}
static int current_permit(P4GameAiPrepared *product, P4AiVmState *vm, int command, Permit *permit)
{
    P4AiCarrier *carrier = vm_carrier(vm);
    if (!carrier) return P4_TABLE_NOT_READY;
    *permit = (Permit){.mask = all_mask(carrier), .source_unrestricted = 1u};
    for (uint32_t i = 0; i < P4_AI_PROOF_SOURCES; ++i) permit->proofs[i] = 1u;
    if (command && vm->command_active) {
        permit->mask = vm->command_allowed_mask; permit->constrained = vm->command_constrained;
        permit->explicit_valid = vm->command_explicit_valid;
        permit->source_unrestricted = vm->command_source_unrestricted;
        if (permit->explicit_valid) permit->explicit_frame = vm->command_explicit_frame;
        memcpy(permit->proofs, vm->command_proofs, sizeof permit->proofs);
        return P4_TABLE_OK;
    }
    const P4AiControlScope *scope = controls(product, vm);
    for (uint32_t i = 0; i < vm->control_count; ++i) {
        if (scope[i].candidate_epoch != carrier->candidate_epoch) return P4_TABLE_CONFLICT;
        merge_permit(permit, scope[i].allowed_mask, scope[i].explicit_valid,
            scope[i].source_unrestricted, &scope[i].explicit_frame, scope[i].proofs);
    }
    return P4_TABLE_OK;
}
static int tag_permit(const P4AiValueTag *tag, P4AiCarrier *carrier, Permit *permit)
{
    if (!tag || !tag->constrained) return P4_TABLE_OK;
    if (tag->candidate_epoch != carrier->candidate_epoch) return P4_TABLE_CONFLICT;
    const P4GameActionFrame *extra = tag->kind == P4_AI_TAG_NUMBER ?
        &tag->value.vector.explicit_frame : &tag->value.selection.candidate.frame;
    merge_permit(permit, all_mask(carrier) & ~tag->invalid_mask, tag->explicit_valid,
        tag->source_unrestricted, extra, tag->proofs);
    return P4_TABLE_OK;
}
static void clear_tag(P4AiValueTag *tag)
{
    tag->kind = P4_AI_TAG_NONE;
}
static void store_tag(P4AiValueTag *out, const P4AiValueTag *in)
{
    out->generation = in->generation; out->kind = in->kind;
    out->actual_word = in->actual_word; out->actual_type = in->actual_type;
    out->invalid_mask = in->invalid_mask; out->candidate_epoch = in->candidate_epoch;
    out->constrained = in->constrained; out->explicit_valid = in->explicit_valid;
    out->source_unrestricted = in->source_unrestricted;
    if (in->kind != P4_AI_TAG_NONE) memcpy(out->proofs, in->proofs, sizeof out->proofs);
    if (in->kind == P4_AI_TAG_NUMBER) {
        out->value.vector.count = in->value.vector.count;
        out->value.vector.source = in->value.vector.source;
        memcpy(out->value.vector.words, in->value.vector.words, in->value.vector.count * sizeof(uint32_t));
        if (in->explicit_valid) out->value.vector.explicit_frame = in->value.vector.explicit_frame;
    }
    else if (in->kind != P4_AI_TAG_NONE) out->value.selection = in->value.selection;
}
static void number_constant(P4AiVmState *vm, P4AiCarrier *carrier, P4AiValueTag *out,
    uint32_t actual_type, uint32_t actual_word, const Permit *permit)
{
    out->generation = vm->generation; out->actual_type = actual_type; out->actual_word = actual_word;
    out->kind = permit->constrained ? P4_AI_TAG_NUMBER : P4_AI_TAG_NONE;
    out->constrained = permit->constrained; out->candidate_epoch = carrier->candidate_epoch;
    out->invalid_mask = all_mask(carrier) & ~permit->mask; out->explicit_valid = permit->explicit_valid;
    out->source_unrestricted = permit->source_unrestricted;
    if (out->kind == P4_AI_TAG_NONE) return;
    memcpy(out->proofs, permit->proofs, sizeof out->proofs);
    out->value.vector.source = vm->owner;
    out->value.vector.count = carrier->candidate_count;
    for (uint32_t i = 0; i < carrier->candidate_count; ++i) out->value.vector.words[i] = actual_word;
    if (permit->explicit_valid) out->value.vector.explicit_frame = permit->explicit_frame;
}
static int copy_value(P4GameAiPrepared *product, P4AiVmState *vm, P4AiValueTag *out,
    const P4AiValueTag *in, uint32_t actual_type, uint32_t actual_word)
{
    P4AiCarrier *carrier = vm_carrier(vm);
    Permit permit;
    int result = current_permit(product, vm, 0, &permit);
    if (result) return result;
    const P4AiValueTag *tag = in ? live_tag(vm, in) : NULL;
    if (!tag) { number_constant(vm, carrier, out, actual_type, actual_word, &permit); return P4_TABLE_OK; }
    result = tag_permit(tag, carrier, &permit);
    if (result) return result;
    store_tag(out, tag);
    out->actual_type = actual_type; out->actual_word = actual_word;
    out->constrained = permit.constrained; out->invalid_mask = all_mask(carrier) & ~permit.mask;
    out->source_unrestricted = permit.source_unrestricted;
    memcpy(out->proofs, permit.proofs, sizeof out->proofs);
    const P4GameActionFrame *extra = tag->kind == P4_AI_TAG_NUMBER ?
        &tag->value.vector.explicit_frame : &tag->value.selection.candidate.frame;
    out->explicit_valid = !out->source_unrestricted && tag->explicit_valid && permit_frame(&permit, carrier, extra);
    return P4_TABLE_OK;
}
static int contains_pc(const P4AiScriptProduct *script, const P4AiCfgNode *branch, uint32_t pc)
{
    size_t lo = branch->first_region, hi = lo + branch->region_count;
    while (lo < hi) {
        size_t middle = lo + (hi - lo) / 2u;
        const P4AiCfgRegion *region = &script->regions[middle];
        if (pc < region->first) hi = middle;
        else if (pc > region->last) lo = middle + 1u;
        else return 1;
    }
    return 0;
}
static int prune_controls(P4GameAiPrepared *product, P4AiVmState *vm, uint32_t pc)
{
    P4AiControlScope *scope = controls(product, vm);
    const P4AiScriptProduct *script = &product->scripts[vm->program];
    uint32_t keep = 0;
    for (uint32_t i = 0; i < vm->control_count; ++i) {
        if (scope[i].call_depth > vm->call_depth || scope[i].branch_pc >= script->code_count)
            return P4_TABLE_CONFLICT;
        const P4AiCfgNode *branch = &script->nodes[scope[i].branch_pc];
        if (scope[i].call_depth == vm->call_depth &&
            (pc == scope[i].branch_pc || pc == branch->join || !contains_pc(script, branch, pc)))
            continue;
        if (keep != i) scope[keep] = scope[i];
        ++keep;
    }
    vm->control_count = keep;
    return P4_TABLE_OK;
}
static int legacy_constant(uint32_t value_type, uint32_t value, int32_t *constant)
{
    if (value_type == 0u) {
        int32_t number = (int32_t)value;
        if (number < -1 || number >= (int32_t)P4_ELEMENT_RETAIL_COUNT) return P4_TABLE_INVALID;
        *constant = number; return P4_TABLE_OK;
    }
    if (value_type == 1u) {
        float number; memcpy(&number, &value, sizeof number);
        for (int32_t i = -1; i < (int32_t)P4_ELEMENT_RETAIL_COUNT; ++i)
            if (number == (float)i) { *constant = i; return P4_TABLE_OK; }
    }
    return P4_TABLE_INVALID;
}
static uint32_t small_word(int32_t number, uint32_t value_type)
{
    if (value_type == 0u) return (uint32_t)number;
    float value = (float)number; uint32_t bits; memcpy(&bits, &value, sizeof bits); return bits;
}
/* A tagged legacy element result is an opaque frame, not a numeric new ID.
 * In the real COMMON_Skill_STD code, ==0/!=0 classify Physical. -1 tests the
 * explicit no-element state; a valid UINT32_MAX leaf must NOT become -1. */
static int reference_witness(const P4GameActionFrame *frame, uint32_t own_type,
    uint32_t other_type, uint32_t other_word, uint32_t *normalized)
{
    if (own_type > 1u) return P4_TABLE_INVALID;
    int32_t value;
    int result = legacy_constant(other_type, other_word, &value);
    if (result) return result;
    const P4GameActionElement *element = &frame->element;
    int equal = value == -1 ? !element->valid : value == 0 ?
        p4_game_action_element_is_physical(element) && element->component == P4_ACTION_COMPONENT_DAMAGE :
        element->valid && element->element_id == (uint32_t)value;
    *normalized = small_word(equal ? value : value == 0 ? 1 : 0, own_type);
    return P4_TABLE_OK;
}
static int truth(uint32_t value_type, uint32_t value, uint32_t *out)
{
    if (value_type == 0u) { *out = value != 0u; return P4_TABLE_OK; }
    if (value_type == 1u) { float number; memcpy(&number, &value, sizeof number); *out = number != 0.0f; return P4_TABLE_OK; }
    return P4_TABLE_INVALID;
}
int p4_game_elements_ai_vm_before(void *work_pointer, const P4GameAiVmToken *token)
{
    P4GameAiPrepared *product = p4_ai_native_product();
    P4AiVmState *vm = from_token(product, token);
    if (!vm || vm->work_guest != P4_H2G(work_pointer) || vm->step_active) return P4_TABLE_CONFLICT;
    P4AiCarrier *carrier = vm_carrier(vm);
    if (!carrier) return P4_TABLE_NOT_READY;
    const P4AiScriptProduct *script = &product->scripts[vm->program];
    uint32_t work = vm->work_guest, pc = P4_G32(work + 0x18u), sp = P4_G32(work + 0x1cu);
    if (pc >= script->code_count || sp > 47u || P4_G32(work + 0x110u) != script->native_header ||
        P4_G32(work + 0x120u) != script->native_header + script->code_offset) return P4_TABLE_CONFLICT;
    const P4AiCfgNode *node = &script->nodes[pc];
    uint32_t instruction = P4_G32(script->native_header + script->code_offset + pc * 4u);
    if (node->opcode == P4_AI_UNUSED_OPCODE || (uint16_t)instruction != node->opcode ||
        (int16_t)(instruction >> 16) != node->operand) return P4_TABLE_CONFLICT;
    uint32_t op = node->opcode;
    if (((op <= 1u || op == 4u || op == 11u || op == 29u || op == 30u || op == 31u || op == 34u) && sp == 47u) ||
        (((op >= 14u && op <= 17u) || (op >= 20u && op <= 27u)) && sp < 2u) ||
        ((op == 18u || op == 19u || op == 28u || op == 32u || op == 33u) && !sp))
        return P4_TABLE_INVALID;
    int result = prune_controls(product, vm, pc);
    if (result) return result;
    if (op == 8u && p4_ai_command_uses_frames((uint16_t)node->operand)) {
        result = p4_game_elements_ai_refresh_source(P4_G2H(vm->owner.action_guest));
        if (result) return result;
    }
    vm->before_pc = pc; vm->before_sp = sp;
    for (uint32_t i = 0; i < 2u && i < sp; ++i) {
        uint32_t index = sp - 1u - i;
        vm->before_words[i] = word(work, index); vm->before_types[i] = type(work, index);
        result = checked_tag(vm, &vm->stack[index], vm->before_types[i], vm->before_words[i]);
        if (result) return result;
    }
    if (op >= 14u && op <= 27u && op != 19u && op != 20u && op != 21u && op != 22u && op != 23u) {
        for (uint32_t i = 0; i < (op == 18u ? 1u : 2u); ++i) {
            const P4AiValueTag *tag = live_tag(vm, &vm->stack[sp - 1u - i]);
            if (tag && tag->kind == P4_AI_TAG_ELEMENT) return P4_TABLE_INVALID;
        }
    }
    if (node->opcode == 8u) {
        if (sp < node->command_arity) return P4_TABLE_INVALID;
        Permit permit;
        result = current_permit(product, vm, 0, &permit);
        if (result) return result;
        for (uint32_t i = 0; i < node->command_arity; ++i) {
            uint32_t index = sp - 1u - i;
            result = checked_tag(vm, &vm->stack[index], type(work, index), word(work, index));
            if (result) return result;
            const P4AiValueTag *tag = live_tag(vm, &vm->stack[index]);
            result = tag_permit(tag, carrier, &permit);
            if (result) return result;
            if (tag && tag->kind == P4_AI_TAG_NUMBER) {
                uint32_t mask = all_mask(carrier);
                for (uint32_t j = 0; j < carrier->candidate_count; ++j)
                    if (tag->value.vector.words[j] != tag->actual_word) mask &= ~(1u << j);
                permit.mask &= mask;
            }
        }
        vm->command_active = 1; vm->command_pc = pc; vm->command_role = node->query_role;
        vm->command_allowed_mask = permit.mask; vm->command_constrained = permit.constrained;
        vm->command_explicit_valid = permit.explicit_valid;
        vm->command_source_unrestricted = permit.source_unrestricted;
        if (permit.explicit_valid) vm->command_explicit_frame = permit.explicit_frame;
        memcpy(vm->command_proofs, permit.proofs, sizeof vm->command_proofs);
        clear_tag(&vm->pending_return);
        vm->target_contract_valid = 0u;
    }
    if (op == 20u || op == 21u) {
        for (uint32_t i = 0; i < 2u; ++i) {
            const P4AiValueTag *tag = live_tag(vm, &vm->stack[sp - 1u - i]);
            if (tag && tag->kind == P4_AI_TAG_ELEMENT) {
                uint32_t normalized;
                result = reference_witness(&tag->value.selection.candidate.frame, vm->before_types[i], 0u, 0u, &normalized);
                if (result) return result;
                P4_G32(work + 0x50u + (sp - 1u - i) * 4u) = normalized;
            }
        }
    }
    if (node->opcode == 22u || node->opcode == 23u) {
        if (sp < 2u) return P4_TABLE_INVALID;
        for (uint32_t i = 0; i < 2u; ++i) {
            const P4AiValueTag *tag = live_tag(vm, &vm->stack[sp - 1u - i]);
            if (tag && tag->kind == P4_AI_TAG_ELEMENT) {
                const P4AiValueTag *other = live_tag(vm, &vm->stack[sp - 2u + i]);
                if (other && other->kind == P4_AI_TAG_ELEMENT) return P4_TABLE_INVALID;
                uint32_t normalized;
                result = reference_witness(&tag->value.selection.candidate.frame, vm->before_types[i],
                    vm->before_types[1u - i], vm->before_words[1u - i], &normalized);
                if (result) return result;
                P4_G32(work + 0x50u + (sp - 1u - i) * 4u) = normalized;
            }
        }
    } else if (node->opcode == 19u || node->opcode == 28u) {
        if (!sp) return P4_TABLE_INVALID;
        const P4AiValueTag *tag = live_tag(vm, &vm->stack[sp - 1u]);
        if (tag && tag->kind == P4_AI_TAG_ELEMENT) {
            uint32_t normalized;
            result = reference_witness(&tag->value.selection.candidate.frame, vm->before_types[0], 0u, 0u, &normalized);
            if (result) return result;
            P4_G32(work + 0x50u + (sp - 1u) * 4u) = normalized;
            vm->before_words[0] = normalized;
        }
    }
    vm->step_active = 1;
    return P4_TABLE_OK;
}

static const P4GameActionFrame *tag_extra(const P4AiValueTag *tag)
{
    return tag->kind == P4_AI_TAG_NUMBER ? &tag->value.vector.explicit_frame :
        &tag->value.selection.candidate.frame;
}
static int shadow_value(const P4AiValueTag *tag, const P4GameActionFrame *frame,
    uint32_t index, uint32_t actual_word, uint32_t own_type,
    uint32_t other_type, uint32_t other_word, uint32_t opcode, uint32_t *out)
{
    if (!tag) { *out = actual_word; return P4_TABLE_OK; }
    if (tag->kind == P4_AI_TAG_NUMBER) { *out = tag->value.vector.words[index]; return P4_TABLE_OK; }
    if (tag->kind == P4_AI_TAG_COMMAND) { *out = actual_word; return P4_TABLE_OK; }
    if (tag->kind != P4_AI_TAG_ELEMENT) return P4_TABLE_INVALID;
    if (tag->value.selection.candidate.command == 0u)
        frame = &tag->value.selection.candidate.frame; /* Actual basic, never a paid virtual member. */
    if (opcode == 32u || opcode == 33u) { *out = actual_word; return P4_TABLE_OK; }
    if (opcode == 19u || opcode == 20u || opcode == 21u)
        return reference_witness(frame, own_type, 0u, 0u, out);
    if (opcode == 22u || opcode == 23u)
        return reference_witness(frame, own_type, other_type, other_word, out);
    return P4_TABLE_INVALID;
}
static uint32_t value_proof(const P4AiValueTag *tag, uint32_t index)
{
    if (!tag) return 1u;
    if (index < P4_GAME_AI_CANDIDATE_COUNT && (tag->invalid_mask & (1u << index))) return 0u;
    if (index == P4_GAME_AI_CANDIDATE_COUNT && !tag->explicit_valid && !tag->source_unrestricted && tag->constrained)
        return 0u;
    return tag->proofs[index];
}
static int numeric_proof(P4AiVmState *vm, uint32_t opcode, uint32_t left, uint32_t right,
    uint32_t lword, uint32_t rword, uint32_t *out)
{
    if (opcode == 20u || opcode == 21u) {
        uint32_t lt, rt;
        int result = truth(vm->before_types[0], lword, &lt);
        if (!result) result = truth(vm->before_types[1], rword, &rt);
        if (result) return result;
        uint32_t decisive = opcode == 20u ? 1u : 0u;
        if (lt == decisive && rt == decisive) { *out = left || right; return P4_TABLE_OK; }
        if (lt == decisive) { *out = left; return P4_TABLE_OK; }
        if (rt == decisive) { *out = right; return P4_TABLE_OK; }
    }
    *out = left && right;
    return P4_TABLE_OK;
}
static int numeric_after(P4GameAiPrepared *product, P4AiVmState *vm, uint32_t opcode,
    uint32_t destination, const P4AiValueTag *left, const P4AiValueTag *right,
    P4GameAiVmShadowFn shadow, void *scratch)
{
    if (!shadow || !scratch) return P4_TABLE_INVALID;
    P4AiCarrier *carrier = vm_carrier(vm);
    Permit permit;
    int result = current_permit(product, vm, 0, &permit);
    if (result) return result;
    /* Operand validity is a proof of its value, not an unconditional AND gate:
     * true OR unknown and false AND unknown are still provable. */
    uint32_t output_type = type(vm->work_guest, destination), output_word = word(vm->work_guest, destination);
    int reference = (left && left->kind == P4_AI_TAG_ELEMENT) || (right && right->kind == P4_AI_TAG_ELEMENT);
    if (!reference && (!left || left->kind != P4_AI_TAG_NUMBER) && (!right || right->kind != P4_AI_TAG_NUMBER)) {
        result = tag_permit(left, carrier, &permit);
        if (!result) result = tag_permit(right, carrier, &permit);
        if (result) return result;
        number_constant(vm, carrier, &vm->stack[destination], output_type, output_word, &permit);
        return P4_TABLE_OK;
    }
    P4AiValueTag output = {.generation = vm->generation, .kind = P4_AI_TAG_NUMBER,
        .actual_type = output_type, .actual_word = output_word, .candidate_epoch = carrier->candidate_epoch,
        .constrained = 1u, .invalid_mask = all_mask(carrier) & ~permit.mask};
    output.value.vector.source = vm->owner;
    output.value.vector.count = carrier->candidate_count;
    uint32_t lwords[P4_GAME_AI_CANDIDATE_COUNT], rwords[P4_GAME_AI_CANDIDATE_COUNT];
    uint32_t invalid = output.invalid_mask, shadow_type = output_type;
    for (uint32_t i = 0; i < carrier->candidate_count; ++i) {
        uint32_t lraw = left && left->kind == P4_AI_TAG_NUMBER ? left->value.vector.words[i] : vm->before_words[0];
        uint32_t rraw = right && right->kind == P4_AI_TAG_NUMBER ? right->value.vector.words[i] : vm->before_words[1];
        result = shadow_value(left, &carrier->available[i].frame, i, lraw, vm->before_types[0],
            vm->before_types[1], rraw, opcode, &lwords[i]);
        if (!result && right)
            result = shadow_value(right, &carrier->available[i].frame, i, rraw, vm->before_types[1],
                vm->before_types[0], lraw, opcode, &rwords[i]);
        else if (!right) rwords[i] = vm->before_words[1];
        if (result) return result;
    }
    shadow(scratch, opcode, carrier->candidate_count, vm->before_types[0], lwords,
        vm->before_types[1], rwords, &shadow_type, output.value.vector.words, &invalid);
    if (shadow_type != output_type || (invalid & ~all_mask(carrier))) return P4_TABLE_CONFLICT;
    output.invalid_mask = invalid;
    for (uint32_t i = 0; i < carrier->candidate_count; ++i) {
        if (invalid & (1u << i)) {
            output.value.vector.words[i] = 0u; /* Undefined metadata only; never a game fallback. */
            output.proofs[i] = 0u;
            continue;
        }
        uint32_t proof;
        result = numeric_proof(vm, opcode, value_proof(left, i), value_proof(right, i),
            lwords[i], rwords[i], &proof);
        if (result) return result;
        output.proofs[i] = permit.proofs[i] && proof;
        if (!output.proofs[i]) output.invalid_mask |= 1u << i;
    }
    uint32_t generic_left = !left || left->source_unrestricted ? value_proof(left, P4_GAME_AI_CANDIDATE_COUNT) : 0u;
    uint32_t generic_right = !right || right->source_unrestricted ? value_proof(right, P4_GAME_AI_CANDIDATE_COUNT) : 0u;
    uint32_t generic_lword = !left || left->source_unrestricted ? vm->before_words[0] : 0u;
    uint32_t generic_rword = !right || right->source_unrestricted ? vm->before_words[1] : 0u;
    if (left && left->kind == P4_AI_TAG_ELEMENT && left->source_unrestricted) {
        result = shadow_value(left, &left->value.selection.candidate.frame, 0u, vm->before_words[0],
            vm->before_types[0], vm->before_types[1], vm->before_words[1], opcode, &generic_lword);
        if (result) return result;
    }
    if (right && right->kind == P4_AI_TAG_ELEMENT && right->source_unrestricted) {
        result = shadow_value(right, &right->value.selection.candidate.frame, 0u, vm->before_words[1],
            vm->before_types[1], vm->before_types[0], vm->before_words[0], opcode, &generic_rword);
        if (result) return result;
    }
    uint32_t generic_proof;
    result = numeric_proof(vm, opcode, generic_left, generic_right, generic_lword, generic_rword, &generic_proof);
    if (result) return result;
    output.proofs[P4_GAME_AI_CANDIDATE_COUNT] = (!permit.constrained || permit.source_unrestricted) &&
        permit.proofs[P4_GAME_AI_CANDIDATE_COUNT] && generic_proof;
    output.source_unrestricted = output.proofs[P4_GAME_AI_CANDIDATE_COUNT];
    const P4AiValueTag *extra = left && left->explicit_valid ? left : right && right->explicit_valid ? right : NULL;
    if (!output.source_unrestricted && extra && (!left || !left->constrained || left->source_unrestricted || left->explicit_valid) &&
        (!right || !right->constrained || right->source_unrestricted || right->explicit_valid) &&
        (!(left && right && left->explicit_valid && right->explicit_valid) ||
            same_frame(tag_extra(left), tag_extra(right)))) {
        output.value.vector.explicit_frame = *tag_extra(extra);
        output.explicit_valid = permit_frame(&permit, carrier, tag_extra(extra));
        uint32_t lword = vm->before_words[0], rword = vm->before_words[1];
        if (left && left->kind == P4_AI_TAG_ELEMENT && (opcode == 20u || opcode == 21u)) {
            result = reference_witness(tag_extra(left), vm->before_types[0], 0u, 0u, &lword);
            if (result) return result;
        }
        if (right && right->kind == P4_AI_TAG_ELEMENT && (opcode == 20u || opcode == 21u)) {
            result = reference_witness(tag_extra(right), vm->before_types[1], 0u, 0u, &rword);
            if (result) return result;
        }
        uint32_t proof;
        result = numeric_proof(vm, opcode, value_proof(left, P4_GAME_AI_CANDIDATE_COUNT),
            value_proof(right, P4_GAME_AI_CANDIDATE_COUNT), lword, rword, &proof);
        if (result) return result;
        output.proofs[P4_GAME_AI_CANDIDATE_COUNT] = permit.proofs[P4_GAME_AI_CANDIDATE_COUNT] && proof;
        if (!output.proofs[P4_GAME_AI_CANDIDATE_COUNT]) output.explicit_valid = 0u;
    }
    store_tag(&vm->stack[destination], &output);
    return P4_TABLE_OK;
}
static int finish_branch(P4GameAiPrepared *product, P4AiVmState *vm,
    const P4AiCfgNode *node, const P4AiValueTag *tag)
{
    if (!tag) return P4_TABLE_OK;
    P4AiCarrier *carrier = vm_carrier(vm);
    uint32_t actual_taken;
    int result = truth(vm->before_types[0], vm->before_words[0], &actual_taken);
    if (result) return result;
    P4AiControlScope scope = {.branch_pc = vm->before_pc, .call_depth = vm->call_depth,
        .candidate_epoch = carrier->candidate_epoch};
    for (uint32_t i = 0; i < carrier->candidate_count; ++i) {
        uint32_t value = vm->before_words[0], taken;
        if (tag->kind == P4_AI_TAG_NUMBER) value = tag->value.vector.words[i];
        else if (tag->kind == P4_AI_TAG_ELEMENT) {
            const P4GameActionFrame *frame = tag->value.selection.candidate.command == 0u ?
                &tag->value.selection.candidate.frame : &carrier->available[i].frame;
            result = reference_witness(frame, vm->before_types[0], 0u, 0u, &value);
            if (result) return result;
        }
        result = truth(vm->before_types[0], value, &taken);
        if (result) return result;
        if (!(tag->invalid_mask & (1u << i)) && taken == actual_taken) {
            scope.allowed_mask |= 1u << i;
            scope.proofs[i] = tag->proofs[i];
        }
    }
    scope.explicit_valid = tag->explicit_valid;
    scope.source_unrestricted = tag->source_unrestricted;
    if (scope.explicit_valid) scope.explicit_frame = *tag_extra(tag);
    if (scope.explicit_valid || scope.source_unrestricted)
        scope.proofs[P4_GAME_AI_CANDIDATE_COUNT] = tag->proofs[P4_GAME_AI_CANDIDATE_COUNT];
    if (!node->region_count) return P4_TABLE_OK;
    if (vm->control_count >= product->control_capacity) return P4_TABLE_CONFLICT;
    controls(product, vm)[vm->control_count++] = scope;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_vm_after(void *work_pointer, const P4GameAiVmToken *token,
    uint32_t code_result, P4GameAiVmShadowFn shadow, void *scratch)
{
    P4GameAiPrepared *product = p4_ai_native_product();
    P4AiVmState *vm = from_token(product, token);
    if (!vm || !vm->step_active || vm->work_guest != P4_H2G(work_pointer)) return P4_TABLE_CONFLICT;
    const P4AiScriptProduct *script = &product->scripts[vm->program];
    const P4AiCfgNode *node = &script->nodes[vm->before_pc];
    uint32_t op = node->opcode, sp = P4_G32(vm->work_guest + 0x1cu), pc = P4_G32(vm->work_guest + 0x18u);
    uint32_t before_sp = vm->before_sp;
    int result = P4_TABLE_OK;
    if (code_result == 0u) {
        result = finalize_choice(vm);
        vm->control_count = 0; goto finish;
    }
    if (code_result == 2u) {
        if (op != 8u || pc != vm->before_pc || sp != before_sp) result = P4_TABLE_CONFLICT;
        goto finish;
    }
    if (code_result != 1u) { result = P4_TABLE_INVALID; goto finish; }
    uint32_t expected_pc = node->successor[0], expected_sp = before_sp;
    if (op == 11u) {
        expected_pc = le32(script->bytes + script->procedure_offset + (size_t)(uint16_t)node->operand * 32u + 24u);
        expected_sp = before_sp + 1u;
        ++vm->call_depth;
    } else if (op == 9u) {
        if (!before_sp || vm->before_types[0] != 4u || !vm->call_depth) { result = P4_TABLE_CONFLICT; goto finish; }
        expected_pc = vm->before_words[0] + 1u; expected_sp = before_sp - 1u;
        --vm->call_depth;
        P4AiControlScope *scope = controls(product, vm);
        uint32_t keep = 0;
        for (uint32_t i = 0; i < vm->control_count; ++i)
            if (scope[i].call_depth <= vm->call_depth) scope[keep++] = scope[i];
        vm->control_count = keep;
    } else if (op == 28u) {
        uint32_t taken;
        result = truth(vm->before_types[0], vm->before_words[0], &taken);
        if (result) goto finish;
        expected_pc = node->successor[taken ? 0u : 1u]; expected_sp = before_sp - 1u;
    } else if (op == 8u) expected_sp = before_sp - node->command_arity;
    else if (op <= 1u || op == 4u || op == 29u || op == 30u || op == 31u || op == 34u) ++expected_sp;
    else if (op == 32u || op == 33u || (op >= 14u && op <= 17u) || (op >= 20u && op <= 27u)) --expected_sp;
    if (sp != expected_sp || pc != expected_pc || sp > 47u) { result = P4_TABLE_CONFLICT; goto finish; }
    if (op == 4u) {
        result = checked_tag(vm, &vm->stack[47], type(vm->work_guest, 47), word(vm->work_guest, 47));
        if (!result) result = copy_value(product, vm, &vm->stack[before_sp], &vm->stack[47],
            type(vm->work_guest, before_sp), word(vm->work_guest, before_sp));
    } else if (op == 30u || op == 31u) {
        P4AiValueTag *local = op == 30u ? int_tags(vm) : float_tags(product, vm);
        uint32_t local_type = op == 30u ? 0u : 1u;
        result = checked_tag(vm, &local[(uint16_t)node->operand], local_type, word(vm->work_guest, before_sp));
        if (!result) result = copy_value(product, vm, &vm->stack[before_sp], &local[(uint16_t)node->operand],
            local_type, word(vm->work_guest, before_sp));
    } else if (op == 32u || op == 33u) {
        P4AiValueTag *local = op == 32u ? int_tags(vm) : float_tags(product, vm);
        const P4AiValueTag *input = live_tag(vm, &vm->stack[before_sp - 1u]);
        uint32_t local_type = op == 32u ? 0u : 1u;
        uint32_t address = P4_G32(vm->work_guest + (op == 32u ? 0x140u : 0x144u)) + (uint16_t)node->operand * 4u;
        if (input && input->kind == P4_AI_TAG_NUMBER) {
            uint32_t converted[P4_GAME_AI_CANDIDATE_COUNT], unused[P4_GAME_AI_CANDIDATE_COUNT] = {0};
            uint32_t output_type = local_type, invalid = input->invalid_mask;
            shadow(scratch, op, vm_carrier(vm)->candidate_count, vm->before_types[0], input->value.vector.words,
                0u, unused, &output_type, converted, &invalid);
            if (output_type != local_type) result = P4_TABLE_CONFLICT;
            else {
                result = copy_value(product, vm, &local[(uint16_t)node->operand], input, local_type, P4_G32(address));
                if (!result) {
                    P4AiValueTag *output = &local[(uint16_t)node->operand];
                    memcpy(output->value.vector.words, converted, vm_carrier(vm)->candidate_count * sizeof *converted);
                    output->invalid_mask |= invalid;
                    for (uint32_t i = 0; i < vm_carrier(vm)->candidate_count; ++i) {
                        if (invalid & (1u << i)) {
                            output->proofs[i] = 0u;
                            output->value.vector.words[i] = 0u;
                        }
                    }
                }
            }
        } else result = copy_value(product, vm, &local[(uint16_t)node->operand], input, local_type, P4_G32(address));
    } else if ((op >= 14u && op <= 27u)) {
        const P4AiValueTag *left = live_tag(vm, &vm->stack[before_sp - 1u]);
        const P4AiValueTag *right = op == 18u || op == 19u ? NULL : live_tag(vm, &vm->stack[before_sp - 2u]);
        result = numeric_after(product, vm, op, op == 18u || op == 19u ? before_sp - 1u : before_sp - 2u,
            left, right, shadow, scratch);
    } else if (op == 28u) result = finish_branch(product, vm, node, live_tag(vm, &vm->stack[before_sp - 1u]));
    else if (sp > before_sp) {
        if (op == 11u) clear_tag(&vm->stack[before_sp]);
        else result = copy_value(product, vm, &vm->stack[before_sp], NULL,
            type(vm->work_guest, before_sp), word(vm->work_guest, before_sp));
    }
    if (!result && sp < before_sp)
        for (uint32_t i = sp; i < before_sp; ++i) clear_tag(&vm->stack[i]);
finish:
    vm->step_active = vm->command_active = 0;
    clear_tag(&vm->pending_return);
    vm->target_contract_valid = 0u;
    return result;
}

static int count_result(P4AiVmState *vm, void *work, uint32_t scalar)
{
    P4AiCarrier *carrier = vm_carrier(vm);
    if (!carrier || scalar > 0xffffu) return P4_TABLE_CONFLICT;
    uint32_t words[P4_GAME_AI_CANDIDATE_COUNT] = {0}, invalid = 0u;
    P4GameAiQueryPlan *plan = &vm->target_contract_plan;
    if (plan->mode == P4_GAME_AI_PLAN_BASIC || plan->mode == P4_GAME_AI_PLAN_PROFILE) {
        /* A fixed basic/profile/status-only count is independent of later
         * source choice. Retain the exact original scalar, not a new domain. */
        for (uint32_t i = 0; i < carrier->candidate_count; ++i) words[i] = scalar;
    } else {
        uint32_t action = vm->owner.action_guest, count = P4_G16(action + 0xd0u), actual = 0u;
        if (count > 12u) return P4_TABLE_CONFLICT; /* Actual candidate pool at0x98. */
        for (uint32_t t = 0; t < count; ++t) {
            uint32_t target_guest = P4_G32(action + 0x98u + t * 4u);
            if (!target_guest) return P4_TABLE_CONFLICT;
            if (!(P4_G16(target_guest + 0x1au) & 1u)) continue;
            void *target = P4_G2H(target_guest), *unit = p4_ai_native_unit_data(target);
            if (!unit || !p4_ai_native_carrier(target)) return P4_TABLE_CONFLICT;
            if (datCalcGetBadStatusNoDown(unit) & 0xffefffffu) continue;
            uint32_t values[P4_GAME_AI_CANDIDATE_COUNT], bad, value;
            int result = p4_game_elements_ai_native_plan_predict(plan, target,
                vm->target_contract_known, vm->target_contract_policy, vm->target_contract_rejected,
                values, &bad, &value);
            if (result) return result;
            actual = (actual + value) & 0xffffu;
            invalid |= bad;
            for (uint32_t i = 0; i < carrier->candidate_count; ++i)
                words[i] = (words[i] + values[i]) & 0xffffu;
        }
        if (actual != scalar) return P4_TABLE_CONFLICT;
    }
    const P4GameActionFrame *frame = plan->mode == P4_GAME_AI_PLAN_FRAME ? &plan->frame : NULL;
    uint32_t independent = plan->mode == P4_GAME_AI_PLAN_BASIC || plan->mode == P4_GAME_AI_PLAN_PROFILE ||
        (plan->mode == P4_GAME_AI_PLAN_FAMILY && !scalar);
    return p4_game_elements_ai_vm_pending_numbers(work, frame, words, invalid, independent);
}
static int target_result(P4AiVmState *vm, void *work, uint32_t actual_type, uint32_t scalar)
{
    if (!vm->target_contract_valid) return P4_TABLE_OK;
    vm->target_contract_valid = 0u;
    if (actual_type != 0u) return P4_TABLE_CONFLICT;
    clear_tag(&vm->pending_return);
    if (vm->target_contract_count) return count_result(vm, work, scalar);
    if (scalar == UINT32_MAX) return P4_TABLE_OK;
    void *target = btlActionFindById((int32_t)(scalar & 0x0fffffffu));
    if (!target) return P4_TABLE_CONFLICT;
    uint32_t words[P4_GAME_AI_CANDIDATE_COUNT], invalid, value;
    int result = p4_game_elements_ai_native_plan_predict(&vm->target_contract_plan, target,
        vm->target_contract_known, vm->target_contract_policy, vm->target_contract_rejected,
        words, &invalid, &value);
    if (result) return result;
    if (!value) return P4_TABLE_CONFLICT;
    P4AiCarrier *carrier = vm_carrier(vm);
    if (!carrier) return P4_TABLE_NOT_READY;
    for (uint32_t i = 0; i < carrier->candidate_count; ++i) {
        if (!words[i]) invalid |= 1u << i;
        words[i] = scalar;
    }
    const P4GameActionFrame *frame = vm->target_contract_plan.mode == P4_GAME_AI_PLAN_FRAME ?
        &vm->target_contract_plan.frame : NULL;
    uint32_t independent = vm->target_contract_plan.mode == P4_GAME_AI_PLAN_BASIC ||
        vm->target_contract_plan.mode == P4_GAME_AI_PLAN_PROFILE;
    return p4_game_elements_ai_vm_pending_numbers(work, frame, words, invalid, independent);
}
void p4_game_elements_ai_vm_return_set(void *work)
{
    P4AiVmState *vm = bound_work(work);
    if (!vm) return;
    P4GameAiPrepared *product = p4_ai_native_product();
    P4AiCarrier *carrier = vm_carrier(vm);
    if (!carrier) abort();
    uint32_t actual_type = type(vm->work_guest, 47u), actual_word = word(vm->work_guest, 47u);
    if (vm->running && vm->command_active && target_result(vm, work, actual_type, actual_word)) abort();
    const P4AiValueTag *pending = live_tag(vm, &vm->pending_return);
    if (vm->running && vm->command_active && pending && actual_type == 0u &&
        (pending->kind == P4_AI_TAG_NUMBER || pending->actual_word == actual_word)) {
        store_tag(&vm->stack[47], pending);
        vm->stack[47].actual_type = actual_type;
        vm->stack[47].actual_word = actual_word;
    } else {
        Permit permit;
        if (current_permit(product, vm, 1, &permit)) abort();
        number_constant(vm, carrier, &vm->stack[47], actual_type, actual_word, &permit);
    }
    clear_tag(&vm->pending_return);
}
static int argument(void *work, uint32_t parameter, void *source, uint32_t kind, P4GameAiSelection *out)
{
    if (!out) return P4_TABLE_INVALID;
    P4AiVmState *vm = bound_work(work);
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!vm || !vm->running || !vm->command_active || !carrier || !same_owner(&carrier->owner, &vm->owner))
        return P4_TABLE_NOT_READY;
    uint32_t sp = P4_G32(vm->work_guest + 0x1cu);
    if (parameter >= sp) return P4_TABLE_INVALID;
    uint32_t index = sp - 1u - parameter;
    int result = checked_tag(vm, &vm->stack[index], type(vm->work_guest, index), word(vm->work_guest, index));
    if (result) return result;
    const P4AiValueTag *tag = live_tag(vm, &vm->stack[index]);
    if (!tag || tag->kind == P4_AI_TAG_NUMBER) return 0;
    if (tag->kind != kind) return P4_TABLE_INVALID;
    P4GameActionFrame current;
    p4_game_elements_native_primary_frame(p4_ai_native_unit_data(source), tag->value.selection.candidate.command,
        tag->value.selection.candidate.frame.native_options, &current);
    if (!same_frame(&current, &tag->value.selection.candidate.frame)) return P4_TABLE_CONFLICT;
    *out = tag->value.selection;
    return 1;
}
int p4_game_elements_ai_vm_argument_selection(void *work, uint32_t parameter,
    void *source, P4GameAiSelection *selection)
{
    return argument(work, parameter, source, P4_AI_TAG_COMMAND, selection);
}
int p4_game_elements_ai_vm_argument_element(void *work, uint32_t parameter,
    void *source, P4GameAiSelection *selection)
{
    return argument(work, parameter, source, P4_AI_TAG_ELEMENT, selection);
}
static int pending_reference(void *work, const P4GameAiSelection *selection, uint32_t scalar, uint32_t kind)
{
    P4AiVmState *vm = bound_work(work);
    if (!vm || !vm->running || !vm->command_active) return P4_TABLE_NOT_READY;
    P4AiCarrier *carrier = vm_carrier(vm);
    if (!carrier || !selection || selection->valid != 1u || !same_owner(&selection->source, &vm->owner))
        return P4_TABLE_INVALID;
    Permit permit;
    int result = current_permit(p4_ai_native_product(), vm, 1, &permit);
    if (result) return result;
    P4AiValueTag *pending = &vm->pending_return;
    clear_tag(pending);
    pending->generation = vm->generation; pending->kind = kind;
    pending->actual_word = scalar; pending->actual_type = 0u;
    pending->candidate_epoch = carrier->candidate_epoch;
    pending->source_unrestricted = kind == P4_AI_TAG_ELEMENT && selection->candidate.command == 0u &&
        (!permit.constrained || permit.source_unrestricted);
    pending->constrained = permit.constrained; pending->invalid_mask = all_mask(carrier) & ~permit.mask;
    pending->explicit_valid = !pending->source_unrestricted && permit_frame(&permit, carrier, &selection->candidate.frame);
    memcpy(pending->proofs, permit.proofs, sizeof pending->proofs);
    pending->value.selection = *selection;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_vm_pending_selection(void *work, const P4GameAiSelection *selection, uint32_t scalar)
{
    return pending_reference(work, selection, scalar, P4_AI_TAG_COMMAND);
}
int p4_game_elements_ai_vm_pending_element(void *work, const P4GameAiSelection *selection, uint32_t scalar)
{
    return pending_reference(work, selection, scalar, P4_AI_TAG_ELEMENT);
}
int p4_game_elements_ai_vm_pending_numbers(void *work, const P4GameActionFrame *actual_frame,
    const uint32_t *words, uint32_t invalid, uint32_t independent)
{
    P4AiVmState *vm = bound_work(work);
    P4AiCarrier *carrier = vm ? vm_carrier(vm) : NULL;
    if (!vm || !carrier || !vm->running || !vm->command_active || !words || (invalid & ~all_mask(carrier)))
        return P4_TABLE_INVALID;
    if (independent > 1u || (independent && invalid)) return P4_TABLE_INVALID;
    if (independent)
        for (uint32_t i = 1u; i < carrier->candidate_count; ++i)
            if (words[i] != words[0]) return P4_TABLE_INVALID;
    Permit permit;
    int result = current_permit(p4_ai_native_product(), vm, 1, &permit);
    if (result) return result;
    P4AiValueTag *pending = &vm->pending_return;
    clear_tag(pending);
    pending->generation = vm->generation; pending->kind = P4_AI_TAG_NUMBER;
    /* The actual scalar is installed by the real return setter below. */
    pending->actual_word = word(vm->work_guest, 47u); pending->actual_type = 0u;
    pending->candidate_epoch = carrier->candidate_epoch; pending->constrained = 1u;
    pending->invalid_mask = invalid | (all_mask(carrier) & ~permit.mask);
    pending->value.vector.source = vm->owner;
    pending->value.vector.count = carrier->candidate_count;
    memcpy(pending->value.vector.words, words, carrier->candidate_count * sizeof *words);
    pending->explicit_valid = actual_frame && permit_frame(&permit, carrier, actual_frame);
    pending->source_unrestricted = independent && (!permit.constrained || permit.source_unrestricted);
    if (pending->explicit_valid) pending->value.vector.explicit_frame = *actual_frame;
    for (uint32_t i = 0; i < P4_AI_PROOF_SOURCES; ++i)
        pending->proofs[i] = permit.proofs[i] &&
            ((i < carrier->candidate_count && !(pending->invalid_mask & (1u << i))) ||
             (i == P4_GAME_AI_CANDIDATE_COUNT && (pending->explicit_valid || pending->source_unrestricted)));
    return P4_TABLE_OK;
}
int p4_game_elements_ai_vm_query_role(void *work, void *source, uint32_t *role)
{
    P4AiVmState *vm = bound_work(work);
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!role || !vm || !carrier || !vm->running || !vm->command_active ||
        !same_owner(&carrier->owner, &vm->owner)) return P4_TABLE_NOT_READY;
    *role = vm->command_role;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_vm_candidate_allowed(void *work, void *source, const P4GameActionFrame *frame)
{
    if (!work) return 1;
    P4AiVmState *vm = bound_work(work);
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!frame || !vm || !carrier || !vm->running || !vm->command_active ||
        !same_owner(&carrier->owner, &vm->owner)) return P4_TABLE_NOT_READY;
    Permit permit;
    int result = current_permit(p4_ai_native_product(), vm, 1, &permit);
    return result ? result : (int)frame_proof(&permit, carrier, frame);
}

int p4_game_elements_ai_vm_commit_choice(void *work, void *source, const P4GameActionFrame *frame)
{
    P4AiVmState *vm = bound_work(work);
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!vm || !carrier || !frame || !vm->running || !vm->command_active ||
        !same_owner(&carrier->owner, &vm->owner)) return P4_TABLE_NOT_READY;
    Permit permit;
    int result = current_permit(p4_ai_native_product(), vm, 1, &permit);
    if (result) return result;
    if (!frame_proof(&permit, carrier, frame)) return P4_TABLE_CONFLICT;
    if (++carrier->choice_generation == 0u) return P4_TABLE_CONFLICT;
    vm->choice_valid = 1u; vm->choice_generation = carrier->choice_generation;
    vm->choice_frame = *frame;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_vm_decoded(void *work, void *source, uint32_t command_parameter)
{
    P4AiVmState *vm = bound_work(work);
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!vm || !carrier || !vm->running || !vm->command_active ||
        !same_owner(&carrier->owner, &vm->owner)) return P4_TABLE_NOT_READY;
    uint32_t kind = P4_G16(vm->owner.action_guest + 0x6cu);
    if (kind == 1u || kind == 2u || kind == 3u) {
        if (!carrier->selected.valid ||
            carrier->selected.candidate.command != P4_G16(vm->owner.action_guest + 0x6eu))
            return P4_TABLE_CONFLICT;
        if (command_parameter != UINT32_MAX) {
            const P4AiScriptProduct *script = &p4_ai_native_product()->scripts[vm->program];
            if (command_parameter >= script->nodes[vm->command_pc].command_arity) return P4_TABLE_INVALID;
            P4GameAiSelection argument_selection;
            int tagged = p4_game_elements_ai_vm_argument_selection(work, command_parameter, source, &argument_selection);
            if (tagged < 0) return tagged;
            if (tagged) {
                if (argument_selection.candidate.command != carrier->selected.candidate.command ||
                    !same_frame(&argument_selection.candidate.frame, &carrier->selected.candidate.frame))
                    return P4_TABLE_CONFLICT;
                void *target = NULL;
                if (argument_selection.target.action_guest) {
                    target = P4_G2H(argument_selection.target.action_guest);
                    P4AiTargetFact fact;
                    int result = p4_ai_native_target_fact(target, &fact);
                    if (result) return result;
                    if (!same_owner(&fact.owner, &argument_selection.target) ||
                        memcmp(&fact.identity, &argument_selection.target_identity, sizeof fact.identity))
                        return P4_TABLE_CONFLICT;
                }
                int result = p4_game_elements_ai_native_select(source, target,
                    &argument_selection.candidate, &argument_selection.decision);
                if (result) return result;
            }
        }
        return p4_game_elements_ai_vm_commit_choice(work, source, &carrier->selected.candidate.frame);
    }
    vm->choice_valid = 0u;
    return P4_TABLE_OK;
}
static int finalize_choice(P4AiVmState *vm)
{
    if (!vm->choice_valid) return P4_TABLE_OK;
    P4AiCarrier *carrier = p4_ai_native_carrier(P4_G2H(vm->owner.action_guest));
    /* Teardown destroys actions before task work. A destroyed/superseded
     * decision cannot execute, and must not validate some new owner's plan. */
    if (!carrier || !same_owner(&carrier->owner, &vm->owner) ||
        carrier->choice_generation != vm->choice_generation) return P4_TABLE_OK;
    uint32_t action = vm->owner.action_guest, kind = P4_G16(action + 0x6cu);
    if (kind != 1u && kind != 2u && kind != 3u) return P4_TABLE_CONFLICT;
    P4GameActionFrame current;
    p4_game_elements_native_primary_frame(p4_ai_native_unit_data(P4_G2H(action)),
        P4_G16(action + 0x6eu), vm->choice_frame.native_options, &current);
    return same_frame(&current, &vm->choice_frame) ? P4_TABLE_OK : P4_TABLE_CONFLICT;
}
int p4_game_elements_ai_vm_target_contract(void *work, void *source, const P4GameAiQueryPlan *plan,
    uint32_t known, uint32_t policy, int32_t rejected)
{
    P4AiVmState *vm = bound_work(work);
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!vm || !carrier || !plan || !vm->running || !vm->command_active ||
        !same_owner(&carrier->owner, &vm->owner) || known > 1u) return P4_TABLE_NOT_READY;
    if (plan->source_guest != vm->owner.action_guest || plan->candidate_epoch != carrier->candidate_epoch ||
        policy < P4_GAME_AI_TEST_REPEL || policy > P4_GAME_AI_TEST_RESIST) return P4_TABLE_INVALID;
    vm->target_contract_plan = *plan; vm->target_contract_known = known;
    const P4AiScriptProduct *script = &p4_ai_native_product()->scripts[vm->program];
    /* Executable63D5E0+DA*8 is001E7C20/arity1: this is a COUNT,
     * never a tagged action ID. Its post-selector filter is pure status. */
    vm->target_contract_count = (uint16_t)script->nodes[vm->command_pc].operand == 0x20dau;
    vm->target_contract_policy = policy; vm->target_contract_rejected = rejected;
    vm->target_contract_valid = 1u;
    return P4_TABLE_OK;
}
