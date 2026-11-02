#include "game_elements_ai_state.h"
#include "game_elements_ai_commands.h"
#include "game_tables_runtime_internal.h"
#include "p4pc/game_elements.h"
#include "p4pc/game_knowledge_native.h"
#include "p4pc/game_skills.h"
#include "p4pc/gmem.h"
#include "p4pc/package_boot.h"
#include <stdlib.h>
#include <string.h>

_Noreturn void p4_game_elements_ai_native_fail(void)
{
    p4_package_boot_runtime_failed("Expanded enemy AI carrier or query state failed validation.");
}

extern int8_t datCalcGetBuffStatus(void *unit, int32_t buff);
extern uint32_t datCalcGetTableLength(void *unit);
extern uint8_t *datCalcSkillList(void *unit);
#include "game_elements_ai_predicate.inc"

/* Actual native ownership: 001B0930 allocates one action; 001B0A60 and
 * 001B0BF0 free that same base. The VM has a separate existing THREE-slot
 * pool (0029D950), so its parallel metadata is prepared once, not per actor. */
static P4GameAiPrepared *staged, *prepared, *active;
static uint32_t live_actions, running_vms;
typedef char P4AiFrameScalarSize[sizeof(P4GameActionFrame) == 44u ? 1 : -1];
typedef char P4AiCandidateScalarSize[sizeof(P4GameAiCandidate) == 68u ? 1 : -1];

static int guest_span(uint32_t address, uint32_t size)
{
    return address && address < P4_GUEST_SIZE && size <= P4_GUEST_SIZE - address;
}
static int owner_equal(const P4GameAiOwner *a, const P4GameAiOwner *b)
{
    return a->action_guest == b->action_guest && a->action_id == b->action_id &&
        a->uid_low == b->uid_low && a->uid_high == b->uid_high;
}
static P4GameAiOwner owner_at(uint32_t action)
{
    return (P4GameAiOwner){action, P4_G32(action + 8u), P4_G32(action), P4_G32(action + 4u)};
}
static int action_head(uint32_t *head)
{
    const uint32_t battle = P4_GPDATA(uint32_t, 0xb3ac);
    /* Ordinary raw loading is the fifth init-boot callback (005E46D0+0x10),
     * before initBootChainDestroy00111AE0 requests a playable sequence. A
     * NULL actual battle singleton proves absence, not an unexamined pool:
     * 001B0930 links every created action through this very singleton. */
    if (!battle) { *head = 0; return P4_TABLE_OK; }
    if (!guest_span(battle, 0x178u)) return P4_TABLE_NOT_READY;
    *head = P4_G32(battle + 0x174u);
    return P4_TABLE_OK;
}
static int pool_shape(uint32_t address, uint32_t payload)
{
    const uint32_t bytes = 0x10u + P4_GAME_AI_VM_WORK_SLOTS * (payload + 8u);
    return guest_span(address, bytes) && P4_G32(address) == payload &&
        P4_G32(address + 4u) == P4_GAME_AI_VM_WORK_SLOTS;
}
static int no_vm_metadata(const P4GameAiPrepared *product)
{
    for (uint32_t i = 0; i < P4_GAME_AI_VM_WORK_SLOTS; ++i) {
        const P4AiVmState *vm = (const P4AiVmState *)(product->vm_arena + i * product->vm_stride);
        if (vm->active || vm->running) return 0;
    }
    return 1;
}
int p4_ai_native_sources_quiescent(const P4GameAiPrepared *product)
{
    if (!product || product != active || live_actions || running_vms || !no_vm_metadata(product)) return 0;
    uint32_t head;
    const uint32_t battle = P4_GPDATA(uint32_t, 0xb3ac);
    if (!battle) return 1;
    return action_head(&head) == P4_TABLE_OK && !head;
}
static P4AiScriptProduct *script_at_header(P4GameAiPrepared *product, uint32_t header)
{
    if (!header) return NULL;
    for (size_t i = 0; i < product->script_count; ++i)
        if (product->scripts[i].native_header == header) return &product->scripts[i];
    return NULL;
}
static int validate_dispatch(const P4GameAiPrepared *product)
{
    static const uint32_t counts[4] = {121u, 51u, 276u, 61u};
    static const uint32_t first[4] = {0u, 121u, 172u, 448u};
    uint32_t tables[4], seen[16] = {0};
    for (uint32_t group = 0; group < 4u; ++group) {
        tables[group] = P4_G32(0x00748530u + group * 8u);
        if (P4_G32(0x00748534u + group * 8u) != counts[group] ||
            !guest_span(tables[group], counts[group] * 8u)) return P4_TABLE_CONFLICT;
    }
    for (size_t i = 0; i < product->script_count; ++i) {
        const P4AiScriptProduct *script = &product->scripts[i];
        for (uint32_t pc = 0; pc < script->code_count; ++pc) {
            const P4AiCfgNode *node = &script->nodes[pc];
            if (node->opcode != 8u) continue;
            uint32_t command = (uint16_t)node->operand, group = command >> 12;
            uint32_t index = command & 0xfffu, key = first[group] + index;
            uint32_t bit = 1u << (key & 31u);
            if (seen[key >> 5] & bit) continue;
            seen[key >> 5] |= bit;
            uint32_t entry = tables[group] + index * 8u;
            if (!P4_G32(entry) || P4_G32(entry + 4u) != node->command_arity ||
                p4_ai_command_arity(command) != node->command_arity) return P4_TABLE_CONFLICT;
            uint32_t expected = p4_ai_semantic_binding(command);
            if (expected && P4_G32(entry) != expected) return P4_TABLE_CONFLICT;
        }
    }
    return P4_TABLE_OK;
}
static int complete_prepare(void *unused)
{
    (void)unused;
    if (!staged || !staged->vm_arena || !p4_game_tables_revision(staged->tables) ||
        !staged->bank.rows_identity || !staged->bank_count || !staged->bank_native_base)
        return P4_TABLE_NOT_READY;
    const P4GameTables *elements = p4_game_elements_prepared_snapshot();
    if (!elements) return P4_TABLE_NOT_READY;
    if (elements != staged->tables || (active && active != staged)) return P4_TABLE_CONFLICT;
    if (staged->bank.skills != p4_game_elements_prepared_skills()) return P4_TABLE_CONFLICT;
    if (staged->scripts[staged->bank_player_script].native_header != P4_GPDATA(uint32_t, 0xb428) ||
        staged->scripts[staged->bank_enemy_script].native_header != P4_GPDATA(uint32_t, 0xb42c) ||
        staged->bank_native_base != P4_GPDATA(uint32_t, 0xb3d0)) return P4_TABLE_CONFLICT;
    if (!staged->scripts[staged->bank_player_script].native_header ||
        !staged->scripts[staged->bank_enemy_script].native_header) return P4_TABLE_NOT_READY;
    uint32_t work_pool = P4_G32(0x00764610u), locals_pool = P4_G32(0x00764614u);
    if (!pool_shape(work_pool, P4_GAME_AI_VM_WORK_PAYLOAD) || !pool_shape(locals_pool, 0x340u))
        return P4_TABLE_NOT_READY;
    int result = validate_dispatch(staged);
    if (result) return result;
    if (!active) {
        uint32_t head;
        result = action_head(&head);
        if (result) return result;
        /* Recheck after EVERY failed-cohort retry, not merely the first prepare.
         * Never inspect +0x458 on any preactivation action to guess ownership. */
        if (head || live_actions || running_vms) return P4_TABLE_NOT_READY;
    } else if (staged->native_work_pool != work_pool && !p4_ai_native_sources_quiescent(staged)) {
        return P4_TABLE_FROZEN;
    }
    staged->native_work_pool = work_pool;
    prepared = staged;
    return P4_TABLE_OK;
}
static void complete_commit(void *unused)
{
    (void)unused;
    /* Presence/order and exact prepared identity were proved before ANY commit.
     * No allocation, lookup, validation or possible failure belongs here. */
    active = prepared;
    active->usage |= P4_AI_PRODUCT_ACTIVE;
    prepared = NULL;
}
int p4_game_elements_ai_boot_stage(const P4GameTables *tables, int request, P4GameAiPrepared *product)
{
    if (request != 0 && request != 1) return P4_TABLE_INVALID;
    if (!request) return product ? P4_TABLE_INVALID : P4_TABLE_OK;
    if (!tables || !p4_game_tables_revision(tables) || !p4_game_elements_requested(tables) ||
        !product || product->tables != tables || !product->script_count || !product->vm_arena ||
        !product->bank.rows_identity || !product->bank_count)
        return P4_TABLE_INVALID;
    if (staged) return staged == product ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    if (active) return P4_TABLE_FROZEN;
    int result = p4_game_tables_battle_complete_register("element-ai", complete_prepare, complete_commit, NULL);
    if (result) return result;
    result = p4_game_tables_battle_complete_require("element-ai", "element");
    if (result) return result;
    result = p4_game_tables_battle_complete_require("element-ai", "table-source");
    if (result) return result;
    staged = product;
    product->usage |= P4_AI_PRODUCT_STAGED;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_session_end(void)
{
    if (active && !p4_ai_native_sources_quiescent(active)) return P4_TABLE_FROZEN;
    if (running_vms || live_actions) return P4_TABLE_FROZEN;
    if (staged) staged->usage = 0;
    if (active) active->usage = 0;
    active = prepared = staged = NULL;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_enabled(void) { return active != NULL; }
int p4_game_elements_ai_native_script_use(uint32_t header)
{
    if (!active) return P4_TABLE_NOT_READY;
    P4AiScriptProduct *script = script_at_header(active, header);
    if (!script) return P4_TABLE_NOT_FOUND;
    if (!script->source_used) {
        int result = active->bank.use(script->source_identity, active->bank.use_context);
        if (result) return result < 0 ? result : P4_TABLE_INVALID;
        script->source_used = 1u;
    }
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_record(uint32_t index, uint32_t *out)
{
    if (!out) return P4_TABLE_INVALID;
    if (!active || !active->bank_native_base) return P4_TABLE_NOT_READY;
    if (index >= active->bank_count) return P4_TABLE_NOT_FOUND;
    if (active->bank_native_base != P4_GPDATA(uint32_t, 0xb3d0)) return P4_TABLE_CONFLICT;
    if (!active->bank_source_used) {
        int result = active->bank.use(active->bank.rows_identity, active->bank.use_context);
        if (result) return result < 0 ? result : P4_TABLE_INVALID;
        active->bank_source_used = 1u;
    }
    *out = active->bank_native_base + index * P4_GAME_AI_RECORD_BYTES;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_source_record(void *source, const uint8_t **out)
{
    if (!out || !p4_ai_native_carrier(source)) return P4_TABLE_INVALID;
    uint32_t action = P4_H2G(source), unit = P4_G32(action + 0x30u);
    if (!guest_span(unit, 0xa68u)) return P4_TABLE_INVALID;
    uint32_t index = P4_G16(unit + 0xa4u);
    if (index >= active->bank_count) return P4_TABLE_NOT_FOUND;
    /* Lookahead borrows the retained CPU definition. Binding already proved
     * its native copy; this is not actual AICALC consumption or a use touch. */
    *out = active->bank.rows + index * P4_GAME_AI_RECORD_BYTES;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_source_use(void *source)
{
    if (!p4_ai_native_carrier(source)) return P4_TABLE_INVALID;
    uint32_t unit = P4_G32(P4_H2G(source) + 0x30u);
    if (!guest_span(unit, 0xa68u)) return P4_TABLE_INVALID;
    uint32_t row;
    return p4_game_elements_ai_native_record(P4_G16(unit + 0xa4u), &row);
}
uint32_t p4_game_elements_ai_native_action_size(void)
{
    return active ? active->action_size : P4_GAME_AI_ACTION_BYTES;
}
P4GameAiPrepared *p4_ai_native_product(void) { return active; }
void p4_ai_native_vm_running(int delta)
{
    if (delta == 1) ++running_vms;
    else if (delta == -1 && running_vms) --running_vms;
    else abort();
}
void p4_game_elements_ai_native_action_init(void *action, uint32_t id, uint32_t lo, uint32_t hi)
{
    if (!active) return;
    uint32_t guest = P4_H2G(action);
    if (!guest_span(guest, active->action_size)) abort();
    P4AiCarrier *carrier = (P4AiCarrier *)((uint8_t *)action + P4_GAME_AI_ACTION_BYTES);
    /* The original constructor already cleared the ONE enlarged allocation. */
    carrier->schema = P4_GAME_AI_CARRIER_SCHEMA;
    carrier->generation = carrier->candidate_epoch = 1u;
    carrier->owner = (P4GameAiOwner){guest, id, lo, hi};
    ++live_actions;
}
void p4_game_elements_ai_native_action_release(void *action)
{
    if (!active) return;
    uint32_t guest = P4_H2G(action);
    if (!guest_span(guest, active->action_size) || !live_actions) abort();
    P4AiCarrier *carrier = (P4AiCarrier *)((uint8_t *)action + P4_GAME_AI_ACTION_BYTES);
    P4GameAiOwner owner = owner_at(guest);
    if (carrier->schema != P4_GAME_AI_CARRIER_SCHEMA || !owner_equal(&carrier->owner, &owner)) abort();
    carrier->schema = 0;
    --live_actions;
}
P4AiCarrier *p4_ai_native_carrier(void *action)
{
    if (!active || !action) return NULL;
    uint32_t guest = P4_H2G(action), current;
    if (!guest_span(guest, active->action_size) || action_head(&current)) return NULL;
    /* Bound by actual constructor/free accounting, not a guessed actor count.
     * Membership is checked BEFORE accessing the appended storage. */
    for (uint32_t i = 0; current && i < live_actions; ++i) {
        if (!guest_span(current, active->action_size)) return NULL;
        if (current == guest) {
            P4AiCarrier *carrier = (P4AiCarrier *)((uint8_t *)action + P4_GAME_AI_ACTION_BYTES);
            P4GameAiOwner owner = owner_at(guest);
            return carrier->schema == P4_GAME_AI_CARRIER_SCHEMA &&
                owner_equal(&carrier->owner, &owner) ? carrier : NULL;
        }
        current = P4_G32(current + 0x450u);
    }
    return NULL;
}
void *p4_ai_native_unit_data(void *action)
{
    uint32_t guest = P4_H2G(action);
    if (!guest_span(guest, P4_GAME_AI_ACTION_BYTES)) return NULL;
    uint32_t unit = P4_G32(guest + 0x30u);
    if (!guest_span(unit, 0xa68u)) return NULL;
    uint32_t data = P4_G32(unit + 0xa64u);
    return guest_span(data, 0x30u) ? P4_G2H(data) : NULL;
}
int p4_game_elements_ai_native_select(void *source, void *target,
    const P4GameAiCandidate *candidate, const P4GameAiDecision *decision)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier) return P4_TABLE_NOT_READY;
    if (!candidate || candidate->query_valid > 1u || !candidate->eligibility ||
        (candidate->eligibility & ~(P4_GAME_AI_CANDIDATE_EQUIPPED | P4_GAME_AI_CANDIDATE_TABLE)) ||
        (decision && (decision->valid > 1u || decision->condition_mask > 7u))) return P4_TABLE_INVALID;
    if (candidate->query_valid && (!target || candidate->query.mode > P4_AFFINITY_QUERY_PROVIDED ||
        candidate->query.value != (candidate->query.mode == P4_AFFINITY_QUERY_PROVIDED ? 100u : 0u)))
        return P4_TABLE_INVALID;
    void *data = p4_ai_native_unit_data(source);
    if (!data) return P4_TABLE_INVALID;
    P4GameActionFrame actual_frame;
    p4_game_elements_native_primary_frame(data, candidate->command, candidate->frame.native_options, &actual_frame);
    if (memcmp(&actual_frame, &candidate->frame, sizeof actual_frame)) return P4_TABLE_CONFLICT;
    P4GameAiSelection selection = {.valid = 1u, .source = carrier->owner, .candidate = *candidate};
    if (decision) selection.decision = *decision;
    else if (carrier->weighted_valid) selection.decision = carrier->decision;
    if (target) {
        P4AiCarrier *target_carrier = p4_ai_native_carrier(target);
        void *target_data = p4_ai_native_unit_data(target);
        if (!target_carrier || !target_data) return P4_TABLE_INVALID;
        selection.target = target_carrier->owner;
        int result = p4_game_elements_unit_identity(target_data, &selection.target_identity);
        if (result) return result;
        if (candidate->query_valid) {
            P4GameAffinityQuery current;
            result = p4_game_knowledge_native_ai_query(target_data, &actual_frame, &current);
            if (result) return result;
            /* Never promote an earlier neutral prediction to a hidden-row read.
             * A newly unknown/current profile may only downgrade ACTUAL. */
            if (candidate->query.mode == P4_AFFINITY_QUERY_ACTUAL) selection.candidate.query = current;
        }
    }
    carrier->selected = selection;
    carrier->decision = selection.decision;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_selected(void *source, P4GameAiSelection *selection)
{
    if (!selection) return P4_TABLE_INVALID;
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier) return P4_TABLE_NOT_READY;
    if (!carrier->selected.valid) return P4_TABLE_NOT_FOUND;
    *selection = carrier->selected;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_clear_selection(void *source)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier) return P4_TABLE_NOT_READY;
    carrier->selected.valid = carrier->decision.valid = 0;
    return P4_TABLE_OK;
}

int p4_game_elements_ai_native_available_set(void *source,
    const P4GameAiCandidate *candidates, uint32_t count)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier) return P4_TABLE_NOT_READY;
    if (!candidates || !count || count > P4_GAME_AI_CANDIDATE_COUNT || candidates[0].command != 0u ||
        !(candidates[0].eligibility & P4_GAME_AI_CANDIDATE_EQUIPPED))
        return P4_TABLE_INVALID;
    uint32_t equipped = 0u;
    for (uint32_t i = 0; i < count; ++i) {
        if ((candidates[i].eligibility & P4_GAME_AI_CANDIDATE_EQUIPPED) &&
            ++equipped > P4_GAME_AI_EQUIPPED_SLOTS + 1u) return P4_TABLE_INVALID;
        if (!candidates[i].eligibility ||
            (candidates[i].eligibility & ~(P4_GAME_AI_CANDIDATE_EQUIPPED | P4_GAME_AI_CANDIDATE_TABLE)) ||
            candidates[i].query_valid || candidates[i].query.mode || candidates[i].query.value ||
            candidates[i].frame.skill_id != candidates[i].command ||
            candidates[i].frame.element.valid > 1u || candidates[i].frame.owned_binding > 1u)
            return P4_TABLE_INVALID;
    }
    if (carrier->candidate_count == count &&
        !memcmp(carrier->available, candidates, count * sizeof *candidates)) return P4_TABLE_OK;
    if (++carrier->candidate_epoch == 0u) abort();
    memcpy(carrier->available, candidates, count * sizeof *candidates);
    memset(carrier->candidate_used, 0, count * sizeof *carrier->candidate_used);
    carrier->candidate_count = count;
    carrier->selected.valid = carrier->decision.valid = carrier->weighted_valid = 0u;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_available(void *source,
    const P4GameAiCandidate **candidates, uint32_t *count)
{
    if (!candidates || !count) return P4_TABLE_INVALID;
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier || !carrier->candidate_count) return P4_TABLE_NOT_READY;
    *candidates = carrier->available;
    *count = carrier->candidate_count;
    return P4_TABLE_OK;
}

int p4_game_elements_ai_native_test(void *unit, const P4GameActionFrame *frame,
    uint32_t genus, uint32_t policy, uint32_t *value, P4GameAffinityQuery *prediction)
{
    if (!active) return P4_TABLE_NOT_READY;
    return p4_ai_test_frame(unit, frame, genus, policy, 0u, value, prediction);
}

int p4_game_elements_ai_native_command(void *source, uint32_t command, P4GameAiSelection *out)
{
    if (!out) return P4_TABLE_INVALID;
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier) return P4_TABLE_NOT_READY;
    P4GameSkillDefinition definition;
    int result = p4_game_skills_definition_find(active->bank.skills, command, &definition);
    if (result) return result;
    if (!definition.active || !definition.metadata) return P4_TABLE_INVALID;
    void *unit = p4_ai_native_unit_data(source);
    if (!unit) return P4_TABLE_INVALID;
    P4GameAiSelection selection = {.valid = 1u, .source = carrier->owner};
    selection.candidate.command = command;
    selection.candidate.eligibility = P4_GAME_AI_CANDIDATE_TABLE;
    uint32_t index;
    for (index = 0; index < carrier->candidate_count; ++index)
        if (carrier->available[index].command == command) {
            selection.candidate.eligibility = carrier->available[index].eligibility;
            break;
        }
    /* The canonical runtime frame boundary records actual skill/item use before
     * this command consumes even the cost-class metadata. CPU definition
     * validation above remains pure and must not manufacture semantic use. */
    p4_game_elements_native_primary_frame(unit, command, 0u, &selection.candidate.frame);
    selection.candidate.cost_kind = command ? definition.metadata[8u] : 0u;
    if (index < carrier->candidate_count &&
        !memcmp(&selection.candidate.frame, &carrier->available[index].frame, sizeof selection.candidate.frame))
        carrier->candidate_used[index] = 1u;
    *out = selection;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_preview_command(void *source, uint32_t command, P4GameAiCandidate *out)
{
    if (!out || !p4_ai_native_carrier(source)) return P4_TABLE_INVALID;
    P4GameSkillDefinition definition;
    int result = p4_game_skills_definition_find(active->bank.skills, command, &definition);
    if (result) return result;
    if (!definition.active || !definition.metadata) return P4_TABLE_INVALID;
    void *unit = p4_ai_native_unit_data(source);
    if (!unit) return P4_TABLE_INVALID;
    P4GameAiCandidate candidate = {.command = command, .eligibility = P4_GAME_AI_CANDIDATE_TABLE};
    result = p4_game_elements_native_preview_primary_frame(unit, command, 0u, &candidate.frame);
    if (result) return result;
    candidate.cost_kind = command ? definition.metadata[8u] : 0u;
    *out = candidate;
    return P4_TABLE_OK;
}

int p4_ai_native_target_fact(void *action, P4AiTargetFact *out)
{
    if (!out) return P4_TABLE_INVALID;
    P4AiCarrier *carrier = p4_ai_native_carrier(action);
    if (!carrier) return P4_TABLE_NOT_READY;
    void *unit = p4_ai_native_unit_data(action);
    if (!unit) return P4_TABLE_INVALID;
    P4AiTargetFact fact = {.owner = carrier->owner};
    int result = p4_game_elements_unit_identity(unit, &fact.identity);
    if (result) return result;
    *out = fact;
    return P4_TABLE_OK;
}

static int plan_source(const P4GameAiQueryPlan *plan, P4AiCarrier **out)
{
    if (!plan || !out) return P4_TABLE_INVALID;
    P4AiCarrier *carrier = p4_ai_native_carrier(P4_G2H(plan->source_guest));
    if (!carrier || !carrier->candidate_count) return P4_TABLE_NOT_READY;
    if (carrier->candidate_epoch != plan->candidate_epoch) return P4_TABLE_CONFLICT;
    *out = carrier;
    return P4_TABLE_OK;
}
static int plan_frame_use(const P4GameAiQueryPlan *plan, P4AiCarrier *carrier,
    const P4GameActionFrame *frame)
{
    for (uint32_t i = 0; i < carrier->candidate_count; ++i) {
        if (memcmp(frame, &carrier->available[i].frame, sizeof *frame)) continue;
        if (carrier->candidate_used[i]) return P4_TABLE_OK;
        void *source = P4_G2H(plan->source_guest), *unit = p4_ai_native_unit_data(source);
        if (!unit) return P4_TABLE_INVALID;
        P4GameActionFrame actual;
        p4_game_elements_native_primary_frame(unit, carrier->available[i].command, frame->native_options, &actual);
        if (memcmp(&actual, frame, sizeof actual)) return P4_TABLE_CONFLICT;
        carrier->candidate_used[i] = 1u;
        return P4_TABLE_OK;
    }
    /* Explicit script argument frames already crossed the actual command
     * getter; literal nonphysical profile queries have no action source. */
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_candidate_use(void *source, uint32_t command, const P4GameActionFrame *frame)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!carrier || !frame) return P4_TABLE_INVALID;
    for (uint32_t i = 0; i < carrier->candidate_count; ++i)
        if (carrier->available[i].command == command &&
            !memcmp(&carrier->available[i].frame, frame, sizeof *frame)) {
            P4GameAiQueryPlan plan = {.source_guest = P4_H2G(source), .candidate_epoch = carrier->candidate_epoch};
            return plan_frame_use(&plan, carrier, frame);
        }
    return P4_TABLE_NOT_FOUND;
}
static int plan_target(void *target, void **unit, uint32_t *genus)
{
    if (!p4_ai_native_carrier(target)) return P4_TABLE_NOT_READY;
    *unit = p4_ai_native_unit_data(target);
    if (!*unit) return P4_TABLE_INVALID;
    *genus = P4_G8(P4_G32(P4_H2G(target) + 0x30u) + 0xa2u);
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_side_mask(void *source, uint32_t opposite, uint32_t *out)
{
    if (!out || opposite > 1u || !p4_ai_native_carrier(source)) return P4_TABLE_INVALID;
    uint32_t action = P4_H2G(source), unit = P4_G32(action + 0x30u);
    if (!guest_span(unit, 0xa68u)) return P4_TABLE_INVALID;
    uint32_t genus = P4_G8(unit + 0xa2u);
    uint32_t side = opposite ? (genus == 0u ? 1u : 0u) : genus;
    if (side >= 32u) return P4_TABLE_INVALID;
    *out = (1u << side) & 0xffffu;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_plan_begin(void *work, void *source, uint32_t parameter,
    int32_t selector, P4GameAiQueryPlan *out)
{
    if (!active) return P4_TABLE_NOT_READY;
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!out || !carrier || !carrier->candidate_count) return P4_TABLE_INVALID;
    P4GameAiQueryPlan plan = {.source_guest = P4_H2G(source), .candidate_epoch = carrier->candidate_epoch,
        .script_work_guest = work ? P4_H2G(work) : 0u, .selection_phase = parameter == UINT32_MAX};
    uint32_t role = P4_GAME_AI_QUERY_AVAILABLE;
    if (work) {
        int result = p4_game_elements_ai_vm_query_role(work, source, &role);
        if (result) return result;
        if (parameter != UINT32_MAX) {
            P4GameAiSelection argument;
            int typed = p4_game_elements_ai_vm_argument_element(work, parameter, source, &argument);
            if (typed < 0) return typed;
            if (typed) {
                plan.mode = argument.candidate.command == 0u ? P4_GAME_AI_PLAN_BASIC : P4_GAME_AI_PLAN_FRAME;
                plan.frame = argument.candidate.frame;
                *out = plan; return P4_TABLE_OK;
            }
        } else {
            /* A native table target callback has no BF selector argument.
             * Its explicit caller work supplies permissions, not a fake ARG0. */
            role = P4_GAME_AI_QUERY_AVAILABLE;
        }
    }
    if (selector == 0) {
        if (role == P4_GAME_AI_QUERY_BASIC) {
            plan.mode = P4_GAME_AI_PLAN_BASIC; plan.frame = carrier->available[0].frame;
        } else if (role == P4_GAME_AI_QUERY_AVAILABLE || role == P4_GAME_AI_QUERY_SELECTED) {
            plan.mode = P4_GAME_AI_PLAN_FAMILY;
            if (role == P4_GAME_AI_QUERY_SELECTED || plan.selection_phase) {
                /* Only an explicit selection phase may use a decoded action.
                 * Carry its receipt, never reconstruct an actor's old command. */
                uint32_t action = P4_H2G(source), kind = P4_G16(action + 0x6cu);
                if (kind == 1u || kind == 2u || kind == 3u) {
                    const P4GameAiSelection *actual = &carrier->selected;
                    if (!actual->valid || actual->candidate.command != P4_G16(action + 0x6eu) ||
                        memcmp(&actual->source, &carrier->owner, sizeof actual->source)) return P4_TABLE_CONFLICT;
                    if (role == P4_GAME_AI_QUERY_SELECTED || kind == 1u ||
                        p4_game_action_element_is_physical(&actual->candidate.frame.element)) {
                        plan.mode = kind == 1u ? P4_GAME_AI_PLAN_BASIC : P4_GAME_AI_PLAN_FRAME;
                        plan.frame = actual->candidate.frame;
                    }
                } else if (role == P4_GAME_AI_QUERY_SELECTED) return P4_TABLE_INVALID;
            }
        } else return P4_TABLE_INVALID;
    } else {
        if (selector < -1 || selector >= (int32_t)P4_ELEMENT_RETAIL_COUNT) return P4_TABLE_INVALID;
        plan.mode = P4_GAME_AI_PLAN_PROFILE;
        if (selector >= 1) {
            P4GameActionElement declared = {.valid = 1u, .element_id = (uint32_t)selector,
                .channel_id = selector < 6 ? P4_CHANNEL_DAMAGE : selector < 8 ?
                    P4_CHANNEL_INSTANT_KO : P4_CHANNEL_STATUS,
                .component = selector < 6 ? P4_ACTION_COMPONENT_DAMAGE : selector < 8 ?
                    P4_ACTION_COMPONENT_INSTANT_KO : P4_ACTION_COMPONENT_STATUS};
            int result = p4_game_tables_classify_runtime_action_element(active->tables, &declared, &plan.frame.element);
            if (result) return result;
        }
    }
    if (plan.mode == P4_GAME_AI_PLAN_FAMILY) {
        int result = p4_game_elements_ai_eligible_mask(source, &plan.candidate_mask);
        if (result) return result;
    }
    *out = plan;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_plan_mask(const P4GameAiQueryPlan *plan, void *target,
    uint32_t require_known, uint32_t *out)
{
    if (!out || require_known > 1u) return P4_TABLE_INVALID;
    P4AiCarrier *carrier;
    int result = plan_source(plan, &carrier);
    if (result) return result;
    void *unit; uint32_t genus;
    result = plan_target(target, &unit, &genus);
    if (result) return result;
    uint32_t count = plan->mode == P4_GAME_AI_PLAN_FAMILY ? carrier->candidate_count : 1u, mask = 0u;
    for (uint32_t i = 0; i < count; ++i) {
        const P4GameActionFrame *frame = plan->mode == P4_GAME_AI_PLAN_FAMILY ?
            &carrier->available[i].frame : &plan->frame;
        if (plan->mode == P4_GAME_AI_PLAN_FAMILY && !(plan->candidate_mask & (1u << i))) continue;
        result = plan_frame_use(plan, carrier, frame);
        if (result) return result;
        if (plan->mode == P4_GAME_AI_PLAN_FAMILY &&
            (!p4_game_action_element_is_physical(&frame->element) ||
             frame->element.component != P4_ACTION_COMPONENT_DAMAGE)) continue;
        if (require_known && genus == 1u) {
            P4GameAffinityQuery query;
            result = p4_game_knowledge_native_ai_query(unit, frame, &query);
            if (result) return result;
            if (query.mode == P4_AFFINITY_QUERY_PROVIDED) continue;
        }
        mask |= 1u << i;
    }
    *out = mask;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_target_allowed(void *work, void *source, void *target, uint32_t *out)
{
    P4AiCarrier *carrier = p4_ai_native_carrier(source);
    if (!out || !carrier) return P4_TABLE_INVALID;
    uint32_t guest = P4_H2G(source), kind = P4_G16(guest + 0x6cu);
    if (!work || (kind != 1u && kind != 2u && kind != 3u)) { *out = 1u; return P4_TABLE_OK; }
    const P4GameAiSelection *actual = &carrier->selected;
    if (!actual->valid || actual->candidate.command != P4_G16(guest + 0x6eu) ||
        memcmp(&actual->source, &carrier->owner, sizeof actual->source)) return P4_TABLE_CONFLICT;
    int allowed = p4_game_elements_ai_vm_candidate_allowed(work, source, target, &actual->candidate.frame);
    if (allowed < 0) return allowed;
    *out = !!allowed;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_selection_mask(const P4GameAiQueryPlan *plan,
    void *target, uint32_t mask, uint32_t *out)
{
    if (!out) return P4_TABLE_INVALID;
    P4AiCarrier *carrier;
    int result = plan_source(plan, &carrier);
    if (result) return result;
    uint32_t count = plan->mode == P4_GAME_AI_PLAN_FAMILY ? carrier->candidate_count : 1u;
    if (mask & ~((1u << count) - 1u)) return P4_TABLE_INVALID;
    if (!plan->script_work_guest) { *out = mask; return P4_TABLE_OK; }
    const P4GameActionFrame *profile_choice = NULL;
    if (plan->mode == P4_GAME_AI_PLAN_PROFILE && !plan->selection_phase) {
        /* A profile query before command selection has no decoded action.
         * Do not infer one from this actor's old/default command work. */
        for (uint32_t i = 0; mask && i < carrier->candidate_count; ++i) {
            int allowed = p4_game_elements_ai_vm_candidate_allowed(P4_G2H(plan->script_work_guest),
                P4_G2H(plan->source_guest), target, &carrier->available[i].frame);
            if (allowed < 0) return allowed;
            if (allowed) { *out = mask; return P4_TABLE_OK; }
        }
        *out = 0u;
        return P4_TABLE_OK;
    }
    if (plan->mode == P4_GAME_AI_PLAN_PROFILE) {
        uint32_t kind = P4_G16(plan->source_guest + 0x6cu);
        if (kind != 1u && kind != 2u && kind != 3u) { *out = mask; return P4_TABLE_OK; }
        const P4GameAiSelection *selected = &carrier->selected;
        if (!selected->valid || selected->candidate.command != P4_G16(plan->source_guest + 0x6eu) ||
            memcmp(&selected->source, &carrier->owner, sizeof selected->source)) return P4_TABLE_CONFLICT;
        profile_choice = &selected->candidate.frame;
    }
    for (uint32_t i = 0; i < count; ++i) {
        if (!(mask & (1u << i))) continue;
        const P4GameActionFrame *frame = profile_choice ? profile_choice :
            plan->mode == P4_GAME_AI_PLAN_FAMILY ? &carrier->available[i].frame : &plan->frame;
        int allowed = p4_game_elements_ai_vm_candidate_allowed(P4_G2H(plan->script_work_guest),
            P4_G2H(plan->source_guest), target, frame);
        if (allowed < 0) return allowed;
        if (!allowed) mask &= ~(1u << i);
    }
    *out = mask;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_plan_test(const P4GameAiQueryPlan *plan, void *target,
    uint32_t mask, uint32_t policy, int32_t rejected, uint32_t *out)
{
    if (!out) return P4_TABLE_INVALID;
    P4AiCarrier *carrier;
    int result = plan_source(plan, &carrier);
    if (result) return result;
    void *unit; uint32_t genus;
    result = plan_target(target, &unit, &genus);
    if (result) return result;
    uint32_t count = plan->mode == P4_GAME_AI_PLAN_FAMILY ? carrier->candidate_count : 1u;
    if (mask & ~((1u << count) - 1u)) return P4_TABLE_INVALID;
    uint32_t value = 0u;
    for (uint32_t i = 0; i < count; ++i) {
        if (!(mask & (1u << i))) continue;
        const P4GameActionFrame *frame = plan->mode == P4_GAME_AI_PLAN_FAMILY ?
            &carrier->available[i].frame : &plan->frame;
        result = plan_frame_use(plan, carrier, frame);
        if (result) return result;
        uint32_t tested;
        result = p4_ai_test_frame(unit, frame, genus, policy,
            plan->mode == P4_GAME_AI_PLAN_PROFILE, &tested, NULL);
        if (result) return result;
        if (tested != (uint32_t)rejected) { value = 1u; break; }
    }
    *out = value;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_side_prefilter(const P4GameAiQueryPlan *plan,
    void *target, uint32_t options, uint32_t *test_out, uint32_t *unknown_out)
{
    if (!test_out || !unknown_out) return P4_TABLE_INVALID;
    P4AiCarrier *carrier;
    int result = plan_source(plan, &carrier);
    if (result) return result;
    void *unit; uint32_t genus;
    result = plan_target(target, &unit, &genus);
    if (result) return result;
    uint32_t count = plan->mode == P4_GAME_AI_PLAN_FAMILY ? carrier->candidate_count : 1u;
    uint32_t tests = 0u, unknown = 0u;
    for (uint32_t i = 0; i < count; ++i) {
        const P4GameActionFrame *frame = plan->mode == P4_GAME_AI_PLAN_FAMILY ?
            &carrier->available[i].frame : &plan->frame;
        if (plan->mode == P4_GAME_AI_PLAN_FAMILY && !(plan->candidate_mask & (1u << i))) continue;
        result = plan_frame_use(plan, carrier, frame);
        if (result) return result;
        if (plan->mode == P4_GAME_AI_PLAN_FAMILY &&
            (!p4_game_action_element_is_physical(&frame->element) ||
             frame->element.component != P4_ACTION_COMPONENT_DAMAGE)) continue;
        uint32_t test, shortcut;
        result = p4_ai_side_frame_gate(unit, frame, genus, options, &test, &shortcut);
        if (result) return result;
        tests |= test << i;
        unknown |= shortcut << i;
    }
    *test_out = tests; *unknown_out = unknown;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_side_project(const P4GameAiQueryPlan *plan,
    void *target, uint32_t tests, uint32_t unknown, uint32_t policy,
    uint32_t status_eligible, uint32_t *words, uint32_t *value)
{
    if (!words || !value || status_eligible > 1u) return P4_TABLE_INVALID;
    P4AiCarrier *carrier;
    int result = plan_source(plan, &carrier);
    if (result) return result;
    void *unit; uint32_t genus;
    result = plan_target(target, &unit, &genus);
    if (result) return result;
    uint32_t count = plan->mode == P4_GAME_AI_PLAN_FAMILY ? carrier->candidate_count : 1u;
    if (((tests | unknown) & ~((1u << count) - 1u)) || (tests & unknown)) return P4_TABLE_INVALID;
    uint32_t any = 0u;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t tested = !!(unknown & (1u << i));
        if (status_eligible && (tests & (1u << i))) {
            const P4GameActionFrame *frame = plan->mode == P4_GAME_AI_PLAN_FAMILY ?
                &carrier->available[i].frame : &plan->frame;
            result = plan_frame_use(plan, carrier, frame);
            if (result) return result;
            result = p4_ai_test_frame(unit, frame, genus, policy,
                plan->mode == P4_GAME_AI_PLAN_PROFILE, &tested, NULL);
            if (result) return result;
        }
        any |= !!tested;
        if (plan->mode == P4_GAME_AI_PLAN_FAMILY) words[i] |= !!tested;
    }
    if (plan->mode != P4_GAME_AI_PLAN_FAMILY)
        for (uint32_t i = 0; i < carrier->candidate_count; ++i) words[i] |= any;
    *value |= any;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_plan_predict(const P4GameAiQueryPlan *plan, void *target,
    uint32_t require_known, uint32_t policy, int32_t rejected,
    uint32_t *words, uint32_t *invalid_out, uint32_t *value_out)
{
    if (!words || !invalid_out || !value_out) return P4_TABLE_INVALID;
    P4AiCarrier *carrier;
    int result = plan_source(plan, &carrier);
    if (result) return result;
    uint32_t mask, value, invalid = 0u;
    result = p4_game_elements_ai_native_plan_mask(plan, target, require_known, &mask);
    if (!result) result = p4_game_elements_ai_native_plan_test(plan, target, mask, policy, rejected, &value);
    if (result) return result;
    void *unit; uint32_t genus;
    result = plan_target(target, &unit, &genus);
    if (result) return result;
    if (plan->mode == P4_GAME_AI_PLAN_PROFILE || plan->mode == P4_GAME_AI_PLAN_BASIC) {
        for (uint32_t i = 0; i < carrier->candidate_count; ++i) words[i] = value;
    } else if (plan->mode == P4_GAME_AI_PLAN_FRAME) {
        for (uint32_t i = 0; i < carrier->candidate_count; ++i) {
            int same = !memcmp(&carrier->available[i].frame, &plan->frame, sizeof plan->frame);
            words[i] = same ? value : 0u;
            if (!same && value) invalid |= 1u << i;
        }
    } else {
        for (uint32_t i = 0; i < carrier->candidate_count; ++i) {
            words[i] = 0u;
            const P4GameActionFrame *frame = &carrier->available[i].frame;
            if (plan->mode == P4_GAME_AI_PLAN_FAMILY && !(plan->candidate_mask & (1u << i))) continue;
            if (plan->mode == P4_GAME_AI_PLAN_FAMILY &&
                (!p4_game_action_element_is_physical(&frame->element) ||
                 frame->element.component != P4_ACTION_COMPONENT_DAMAGE)) {
                if (value) invalid |= 1u << i;
                continue;
            }
            if (require_known && genus == 1u) {
                P4GameAffinityQuery query;
                result = p4_game_knowledge_native_ai_query(unit, frame, &query);
                if (result) return result;
                if (query.mode == P4_AFFINITY_QUERY_PROVIDED) { invalid |= 1u << i; continue; }
            }
            uint32_t tested;
            result = p4_ai_test_frame(unit, frame, genus, policy, 0u, &tested, NULL);
            if (result) return result;
            words[i] = tested != (uint32_t)rejected;
        }
    }
    *invalid_out = invalid; *value_out = value;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_native_query(void *work, void *source, void *target,
    uint32_t parameter, int32_t selector, uint32_t policy, uint32_t id_domain, uint32_t *out)
{
    if (!out || id_domain > 1u) return P4_TABLE_INVALID;
    P4GameAiQueryPlan plan;
    int result = p4_game_elements_ai_native_plan_begin(work, source, parameter, selector, &plan);
    if (result) return result;
    uint32_t words[P4_GAME_AI_CANDIDATE_COUNT], invalid, value;
    result = p4_game_elements_ai_native_plan_predict(&plan, target, 0u, policy, 0, words, &invalid, &value);
    if (result) return result;
    const P4GameActionFrame *actual = plan.mode == P4_GAME_AI_PLAN_FRAME ? &plan.frame : NULL;
    uint32_t independent = plan.mode == P4_GAME_AI_PLAN_BASIC || plan.mode == P4_GAME_AI_PLAN_PROFILE ||
        (plan.mode == P4_GAME_AI_PLAN_FAMILY && !value);
    result = p4_game_elements_ai_vm_pending_numbers(work, actual, words, invalid,
        id_domain ? target : NULL, independent);
    if (result) return result;
    *out = value;
    return P4_TABLE_OK;
}
