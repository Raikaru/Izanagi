#include "game_elements_ai_state.h"
#include "game_elements_ai_commands.h"
#include "p4pc/guest.h"
#include "p4pc/game_skills.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | (uint16_t)p[1] << 8);
}
static uint32_t read32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
        (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static int valid_role(uint32_t role)
{
    return role == P4_GAME_AI_QUERY_BASIC || role == P4_GAME_AI_QUERY_AVAILABLE ||
        role == P4_GAME_AI_QUERY_SELECTED;
}
static int span(size_t size, uint32_t offset, uint32_t count, uint32_t stride)
{
    return offset <= size && stride && count <= (size - offset) / stride;
}
static unsigned edge_count(const P4AiCfgNode *node)
{
    if (node->opcode == P4_AI_UNUSED_OPCODE) return 0;
    return node->opcode == 28 ? 2u : 1u;
}
static uint32_t edge(const P4AiCfgNode *node, unsigned index, uint32_t exit)
{
    return node->successor[index] == P4_AI_EXIT ? exit : node->successor[index];
}
static int region_append(P4AiScriptProduct *script, uint32_t first, uint32_t last)
{
    if (script->region_count == UINT32_MAX) return P4_TABLE_NOMEM;
    if (script->region_count == script->region_capacity) {
        size_t capacity = script->region_capacity ? script->region_capacity * 2u : 64u;
        if (capacity < script->region_capacity || capacity > SIZE_MAX / sizeof *script->regions)
            return P4_TABLE_NOMEM;
        P4AiCfgRegion *regions = realloc(script->regions, capacity * sizeof *regions);
        if (!regions) return P4_TABLE_NOMEM;
        script->regions = regions;
        script->region_capacity = capacity;
    }
    script->regions[script->region_count++] = (P4AiCfgRegion){ first, last };
    return P4_TABLE_OK;
}
static uint32_t intersect(uint32_t a, uint32_t b, const uint32_t *order,
    const uint32_t *idom)
{
    while (a != b) {
        while (order[a] > order[b]) a = idom[a];
        while (order[b] > order[a]) b = idom[b];
    }
    return a;
}

/* Immediate postdominators are ordinary dominators of the reversed graph.
 * Cooper/Harvey/Kennedy avoids an instruction-count-squared bit matrix. The
 * single exit is a VM return; tail procedure jumps remain real graph edges. */
static int compile_cfg(P4AiScriptProduct *script)
{
    uint32_t n = script->code_count;
    if ((size_t)n > (SIZE_MAX / sizeof(uint32_t) - 9u) / 10u) return P4_TABLE_NOMEM;
    uint32_t *work = calloc((size_t)n * 10u + 9u, sizeof *work);
    if (!work) return P4_TABLE_NOMEM;
    uint32_t *offsets = work;
    uint32_t *cursor = offsets + (size_t)n + 2u;
    uint32_t *predecessors = cursor + (size_t)n + 1u;
    uint32_t *order = predecessors + (size_t)n * 2u;
    uint32_t *rpo = order + (size_t)n + 1u;
    uint32_t *idom = rpo + (size_t)n + 1u;
    uint32_t *stack = idom + (size_t)n + 1u;
    uint32_t *seen = stack + (size_t)n + 1u;
    uint32_t *coverage = seen + (size_t)n + 1u;
    int result = P4_TABLE_OK;
    for (uint32_t pc = 0; pc < n; ++pc) {
        P4AiCfgNode *node = &script->nodes[pc];
        for (unsigned k = 0; k < edge_count(node); ++k)
            ++offsets[edge(node, k, n) + 1u];
    }
    for (uint32_t i = 1; i <= n + 1u; ++i) offsets[i] += offsets[i - 1u];
    memcpy(cursor, offsets, ((size_t)n + 1u) * sizeof *cursor);
    for (uint32_t pc = 0; pc < n; ++pc) {
        P4AiCfgNode *node = &script->nodes[pc];
        for (unsigned k = 0; k < edge_count(node); ++k)
            predecessors[cursor[edge(node, k, n)]++] = pc;
    }
    memcpy(cursor, offsets, ((size_t)n + 1u) * sizeof *cursor);
    uint32_t depth = 1, count = 0;
    stack[0] = n;
    seen[n] = 1;
    while (depth) {
        uint32_t v = stack[depth - 1u];
        if (cursor[v] < offsets[v + 1u]) {
            uint32_t child = predecessors[cursor[v]++];
            if (!seen[child]) {
                seen[child] = 1;
                stack[depth++] = child;
            }
        } else {
            rpo[count++] = v;
            --depth;
        }
    }
    for (uint32_t i = 0; i <= n; ++i) order[i] = idom[i] = P4_AI_EXIT;
    for (uint32_t i = 0; i < count / 2u; ++i) {
        uint32_t value = rpo[i]; rpo[i] = rpo[count - 1u - i]; rpo[count - 1u - i] = value;
    }
    for (uint32_t i = 0; i < count; ++i) order[rpo[i]] = i;
    idom[n] = n;
    int changed;
    do {
        changed = 0;
        for (uint32_t i = 1; i < count; ++i) {
            uint32_t pc = rpo[i], parent = P4_AI_EXIT;
            P4AiCfgNode *node = &script->nodes[pc];
            for (unsigned k = 0; k < edge_count(node); ++k) {
                uint32_t successor = edge(node, k, n);
                if (idom[successor] != P4_AI_EXIT)
                    parent = parent == P4_AI_EXIT ? successor : intersect(parent, successor, order, idom);
            }
            if (parent != idom[pc]) { idom[pc] = parent; changed = 1; }
        }
    } while (changed);

    /* All declared entry points and their reachable instructions must have a
     * possible VM return. Do not invent a join for an unqualified closed loop. */
    memset(seen, 0, ((size_t)n + 1u) * sizeof *seen);
    depth = 0;
    for (uint32_t i = 0; i < script->procedure_count; ++i) {
        uint32_t pc = read32(script->bytes + script->procedure_offset + (size_t)i * 32u + 24u);
        if (!seen[pc]) { seen[pc] = 1; stack[depth++] = pc; }
    }
    while (depth) {
        uint32_t pc = stack[--depth];
        if (idom[pc] == P4_AI_EXIT) { result = P4_TABLE_INVALID; goto finish; }
        P4AiCfgNode *node = &script->nodes[pc];
        for (unsigned k = 0; k < edge_count(node); ++k) {
            uint32_t successor = edge(node, k, n);
            if (successor != n && !seen[successor]) {
                seen[successor] = 1; stack[depth++] = successor;
            }
        }
    }
    memset(seen, 0, ((size_t)n + 1u) * sizeof *seen);
    uint32_t stamp = 0;
    for (uint32_t pc = 0; pc < n; ++pc) {
        P4AiCfgNode *branch = &script->nodes[pc];
        branch->first_region = branch->region_count = 0;
        branch->join = P4_AI_EXIT;
        if (branch->opcode != 28 || idom[pc] == P4_AI_EXIT) continue;
        uint32_t join = idom[pc];
        branch->join = join == n ? P4_AI_EXIT : join;
        ++stamp;
        depth = 0;
        for (unsigned k = 0; k < 2; ++k) {
            uint32_t successor = edge(branch, k, n);
            if (successor != n && successor != join && seen[successor] != stamp) {
                seen[successor] = stamp; stack[depth++] = successor;
            }
        }
        while (depth) {
            uint32_t current = stack[--depth];
            ++coverage[current];
            P4AiCfgNode *node = &script->nodes[current];
            for (unsigned k = 0; k < edge_count(node); ++k) {
                uint32_t successor = edge(node, k, n);
                if (successor != n && successor != join && seen[successor] != stamp) {
                    seen[successor] = stamp; stack[depth++] = successor;
                }
            }
        }
        branch->first_region = (uint32_t)script->region_count;
        for (uint32_t i = 0; i < n;) {
            if (seen[i] != stamp) { ++i; continue; }
            uint32_t first = i;
            do { ++i; } while (i < n && seen[i] == stamp);
            result = region_append(script, first, i - 1u);
            if (result) goto finish;
        }
        branch->region_count = (uint32_t)(script->region_count - branch->first_region);
    }
    for (uint32_t i = 0; i < n; ++i)
        if (coverage[i] > script->max_control_overlap) script->max_control_overlap = coverage[i];
finish:
    free(work);
    return result;
}

static int prepare_script(const P4GameAiScriptSource *source, P4AiScriptProduct *script)
{
    if (!source->bytes || !source->source_identity || source->size < 32u ||
        source->size > UINT32_MAX ||
        (source->query_scope_count && !source->query_scopes)) return P4_TABLE_INVALID;
    const uint8_t *bytes = source->bytes;
    uint32_t total = read32(bytes + 16u), declared_size = read32(bytes + 4u);
    if (memcmp(bytes + 8u, "FLW0", 4u) || total < 2u || total > 5u ||
        !span(source->size, 32u, total, 16u) || declared_size < 32u + total * 16u ||
        declared_size > source->size || read32(bytes + 32u) != 0u ||
        read16(bytes + 20u) > P4_GAME_AI_VM_LOCAL_INTS ||
        read16(bytes + 22u) > P4_GAME_AI_VM_LOCAL_FLOATS) return P4_TABLE_INVALID;
    uint32_t offsets[5] = {0}, counts[5] = {0}, strides[5] = {0};
    uint32_t present = 0;
    for (uint32_t i = 0; i < total; ++i) {
        const uint8_t *entry = bytes + 32u + (size_t)i * 16u;
        uint32_t kind = read32(entry), stride = read32(entry + 4u);
        uint32_t count = read32(entry + 8u), offset = read32(entry + 12u);
        if (kind >= 5u || (present & (1u << kind)) || count > INT32_MAX ||
            stride != (kind <= 1u ? 32u : kind == 2u ? 4u : 1u) ||
            !span(source->size, offset, count, stride) ||
            (count && offset < 32u + total * 16u)) return P4_TABLE_INVALID;
        for (uint32_t j = 0; j < 5u; ++j) {
            if (!(present & (1u << j)) || !count || !counts[j]) continue;
            size_t end = (size_t)offset + (size_t)count * stride;
            size_t other_end = (size_t)offsets[j] + (size_t)counts[j] * strides[j];
            if (offset < other_end && offsets[j] < end) return P4_TABLE_INVALID;
        }
        present |= 1u << kind;
        offsets[kind] = offset; counts[kind] = count; strides[kind] = stride;
    }
    if (!(present & 1u) || !(present & 4u) || !counts[0] || !counts[2] ||
        source->query_scope_count > counts[2]) return P4_TABLE_INVALID;
    if (counts[2] > SIZE_MAX / sizeof *script->nodes) return P4_TABLE_NOMEM;
    script->bytes = bytes; script->size = source->size;
    script->source_identity = source->source_identity;
    script->code_offset = offsets[2]; script->code_count = counts[2];
    script->procedure_offset = offsets[0]; script->procedure_count = counts[0];
    script->label_offset = offsets[1]; script->message_offset = offsets[3];
    script->string_offset = offsets[4]; script->section_mask = present;
    script->message_count = counts[3];
    script->local_int_count = read16(bytes + 20u);
    script->local_float_count = read16(bytes + 22u);
    script->nodes = calloc(counts[2], sizeof *script->nodes);
    if (!script->nodes) return P4_TABLE_NOMEM;
    for (uint32_t i = 0; i < counts[2]; ++i) script->nodes[i].opcode = P4_AI_UNUSED_OPCODE;
    for (uint32_t pc = 0; pc < counts[2];) {
        const uint8_t *instruction = bytes + offsets[2] + (size_t)pc * 4u;
        uint16_t opcode = read16(instruction);
        int16_t operand = (int16_t)read16(instruction + 2u);
        if (opcode >= 35u || opcode == 12u) return P4_TABLE_INVALID;
        /* AI metadata is scoped to the actual action/script. The shipping AI
         * corpus has no global VM opcodes. Unscoped globals are not a place to
         * discard or forge an element/permission tag; reject before exposure. */
        if (opcode == 2u || opcode == 3u || opcode == 5u || opcode == 6u)
            return P4_TABLE_INVALID;
        uint32_t next = pc + (opcode <= 1u ? 2u : 1u);
        if (next > counts[2]) return P4_TABLE_INVALID;
        P4AiCfgNode *node = &script->nodes[pc];
        node->opcode = opcode; node->operand = operand;
        node->first_region = P4_AI_EXIT;
        node->successor[0] = next; node->successor[1] = P4_AI_EXIT;
        if ((opcode == 30u || opcode == 32u) &&
            (operand < 0 || (uint32_t)operand >= script->local_int_count)) return P4_TABLE_INVALID;
        if ((opcode == 31u || opcode == 33u) &&
            (operand < 0 || (uint32_t)operand >= script->local_float_count)) return P4_TABLE_INVALID;
        if (opcode == 34u && (operand < 0 || (uint32_t)operand >= counts[4] ||
            !memchr(bytes + offsets[4] + operand, 0, counts[4] - (uint32_t)operand)))
            return P4_TABLE_INVALID;
        if (opcode == 8u) {
            node->command_arity = p4_ai_command_arity((uint16_t)operand);
            if (node->command_arity == 255u) return P4_TABLE_INVALID;
        }
        pc = next;
    }
    for (uint32_t kind = 0; kind < 2u; ++kind) {
        for (uint32_t i = 0; i < counts[kind]; ++i) {
            const uint8_t *record = bytes + offsets[kind] + (size_t)i * 32u;
            uint32_t address = read32(record + 24u);
            if (address >= counts[2] || script->nodes[address].opcode == P4_AI_UNUSED_OPCODE ||
                !memchr(record, 0, 24u)) return P4_TABLE_INVALID;
        }
    }
    for (uint32_t pc = 0; pc < counts[2]; ++pc) {
        P4AiCfgNode *node = &script->nodes[pc];
        uint32_t opcode = node->opcode;
        int32_t operand = node->operand;
        if (opcode == P4_AI_UNUSED_OPCODE) continue;
        if (opcode == 9u) node->successor[0] = P4_AI_EXIT;
        else if (opcode == 10u || opcode == 11u) {
            if (operand < 0 || (uint32_t)operand >= counts[0]) return P4_TABLE_INVALID;
            if (opcode == 10u)
                node->successor[0] = read32(bytes + offsets[0] + (size_t)operand * 32u + 24u);
        } else if (opcode == 13u || opcode == 28u) {
            if (operand < 0 || (uint32_t)operand >= counts[1]) return P4_TABLE_INVALID;
            uint32_t target = read32(bytes + offsets[1] + (size_t)operand * 32u + 24u);
            node->successor[opcode == 28u ? 1u : 0u] = target;
        }
        for (unsigned k = 0; k < edge_count(node); ++k) {
            uint32_t target = node->successor[k];
            if (target != P4_AI_EXIT && (target >= counts[2] ||
                script->nodes[target].opcode == P4_AI_UNUSED_OPCODE)) return P4_TABLE_INVALID;
        }
    }
    for (size_t i = 0; i < source->query_scope_count; ++i) {
        const P4GameAiScriptQueryScope *scope = &source->query_scopes[i];
        if (scope->instruction_pc >= counts[2] || !valid_role(scope->role)) return P4_TABLE_INVALID;
        P4AiCfgNode *node = &script->nodes[scope->instruction_pc];
        if (node->opcode != 8u || (uint16_t)node->operand != scope->command_id ||
            node->first_region != P4_AI_EXIT) return P4_TABLE_INVALID;
        node->query_role = scope->role;
        node->first_region = 0;
    }
    int result = compile_cfg(script);
    return result ? result : p4_ai_script_basic_roles(script);
}

int p4_game_elements_ai_sources_destroy(P4GameAiPrepared *prepared)
{
    if (!prepared) return P4_TABLE_OK;
    if (prepared->usage) return P4_TABLE_FROZEN;
    for (size_t i = 0; i < prepared->script_count; ++i) {
        free(prepared->scripts[i].nodes);
        free(prepared->scripts[i].regions);
    }
    free(prepared->scripts);
    free(prepared->vm_arena);
    free(prepared);
    return P4_TABLE_OK;
}
int p4_game_elements_ai_sources_prepare(const P4GameTables *tables, int request,
    const P4GameAiScriptSource *sources, size_t count, P4GameAiPrepared **out)
{
    if (!out || (request != 0 && request != 1)) return P4_TABLE_INVALID;
    if (!request) { *out = NULL; return P4_TABLE_OK; }
    if (!tables || !p4_game_tables_revision(tables) || !p4_game_elements_requested(tables) ||
        !sources || !count || count > UINT32_MAX ||
        count > SIZE_MAX / sizeof(P4AiScriptProduct)) return P4_TABLE_INVALID;
    P4GameAiPrepared *prepared = calloc(1u, sizeof *prepared);
    if (!prepared) return P4_TABLE_NOMEM;
    prepared->tables = tables;
    prepared->scripts = calloc(count, sizeof *prepared->scripts);
    if (!prepared->scripts) { free(prepared); return P4_TABLE_NOMEM; }
    prepared->script_count = count;
    int result = P4_TABLE_OK;
    uint32_t overlap = 0;
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = 0; j < i; ++j) {
            if (sources[i].source_identity == sources[j].source_identity) {
                result = P4_TABLE_CONFLICT; goto failure;
            }
        }
        result = prepare_script(&sources[i], &prepared->scripts[i]);
        if (result) goto failure;
        const P4AiScriptProduct *script = &prepared->scripts[i];
        if (script->local_int_count > prepared->local_int_capacity)
            prepared->local_int_capacity = script->local_int_count;
        if (script->local_float_count > prepared->local_float_capacity)
            prepared->local_float_capacity = script->local_float_count;
        if (script->max_control_overlap > overlap) overlap = script->max_control_overlap;
    }
    if (overlap > UINT32_MAX / P4_GAME_AI_VM_STACK_SLOTS) { result = P4_TABLE_NOMEM; goto failure; }
    prepared->control_capacity = overlap * P4_GAME_AI_VM_STACK_SLOTS;
    uint64_t vm_bytes = sizeof(P4AiVmState) +
        ((uint64_t)prepared->local_int_capacity + prepared->local_float_capacity) * sizeof(P4AiValueTag) +
        (uint64_t)prepared->control_capacity * sizeof(P4AiControlScope);
    /* Explicit bounded metadata storage, prepared once for the real three-slot
     * work pool: no per-step malloc or map. */
    vm_bytes = (vm_bytes + 7u) & ~(uint64_t)7u;
    if (vm_bytes > SIZE_MAX / P4_GAME_AI_VM_WORK_SLOTS) { result = P4_TABLE_NOMEM; goto failure; }
    prepared->vm_stride = (size_t)vm_bytes;
    prepared->vm_arena = calloc(P4_GAME_AI_VM_WORK_SLOTS, prepared->vm_stride);
    if (!prepared->vm_arena) { result = P4_TABLE_NOMEM; goto failure; }
    prepared->action_size = P4_GAME_AI_ACTION_BYTES + (uint32_t)sizeof(P4AiCarrier);
    *out = prepared;
    return P4_TABLE_OK;
failure:
    p4_game_elements_ai_sources_destroy(prepared);
    return result;
}
int p4_game_elements_ai_source_bind(P4GameAiPrepared *prepared,
    const void *identity, const uint8_t *loaded, size_t size, uint32_t guest)
{
    if (!prepared || !identity || !loaded || !guest || guest >= P4_GUEST_SIZE ||
        size > P4_GUEST_SIZE - guest || P4_G2H(guest) != loaded) return P4_TABLE_INVALID;
    P4AiScriptProduct *found = NULL;
    for (size_t i = 0; i < prepared->script_count; ++i) {
        P4AiScriptProduct *script = &prepared->scripts[i];
        if (script->source_identity == identity) found = script;
        else if (script->native_header == guest) return P4_TABLE_CONFLICT;
    }
    if (!found) return P4_TABLE_NOT_FOUND;
    if (found->size != size || memcmp(found->bytes, loaded, size)) return P4_TABLE_CONFLICT;
    if (found->native_header && found->native_header != guest &&
        (prepared->usage & P4_AI_PRODUCT_ACTIVE) && !p4_ai_native_sources_quiescent(prepared))
        return P4_TABLE_FROZEN;
    found->native_header = guest;
    return P4_TABLE_OK;
}

static int opaque_action(uint16_t action)
{
    if ((action & 0xf000u) == 0x1000u || (action & 0xf000u) == 0x2000u) return 1;
    switch (action) {
    case 0x8000u: case 0x8001u: case 0x8002u: case 0x8004u: case 0x8005u:
    case 0x8006u: case 0x8007u: case 0x8008u: case 0x8009u: return 1;
    default: return 0;
    }
}
int p4_game_elements_ai_bank_stage(P4GameAiPrepared *prepared, const P4GameAiBankSource *source)
{
    if (!prepared) return source ? P4_TABLE_INVALID : P4_TABLE_OK;
    if (prepared->usage) return P4_TABLE_FROZEN;
    if (!source || !source->rows || !source->rows_size || source->rows_size >= P4_GUEST_SIZE ||
        source->rows_size % P4_GAME_AI_RECORD_BYTES || !source->rows_identity ||
        !source->player_script_identity || !source->enemy_script_identity || !source->skills ||
        !p4_game_skills_semantic_revision(source->skills) || !source->use || !source->use_context)
        return P4_TABLE_INVALID;
    if (source->player_script_identity == source->enemy_script_identity ||
        source->rows_identity == source->player_script_identity ||
        source->rows_identity == source->enemy_script_identity) return P4_TABLE_CONFLICT;
    if (prepared->bank.rows_identity) {
        const P4GameAiBankSource *bank = &prepared->bank;
        return bank->rows == source->rows && bank->rows_size == source->rows_size &&
            bank->rows_identity == source->rows_identity && bank->skills == source->skills &&
            bank->player_script_identity == source->player_script_identity &&
            bank->enemy_script_identity == source->enemy_script_identity &&
            bank->use == source->use && bank->use_context == source->use_context ?
            P4_TABLE_OK : P4_TABLE_CONFLICT;
    }
    size_t player = SIZE_MAX, enemy = SIZE_MAX;
    for (size_t i = 0; i < prepared->script_count; ++i) {
        if (prepared->scripts[i].source_identity == source->player_script_identity) player = i;
        if (prepared->scripts[i].source_identity == source->enemy_script_identity) enemy = i;
        if (prepared->scripts[i].source_identity == source->rows_identity) return P4_TABLE_CONFLICT;
    }
    if (player == SIZE_MAX || enemy == SIZE_MAX) return P4_TABLE_NOT_FOUND;
    uint32_t count = (uint32_t)(source->rows_size / P4_GAME_AI_RECORD_BYTES);
    uint32_t seen_skills[2048] = {0}; /* Exact original u16 action-code domain. */
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t *row = source->rows + (size_t)i * P4_GAME_AI_RECORD_BYTES;
        uint32_t procedure = read16(row + 2u);
        if (procedure >= prepared->scripts[enemy].procedure_count) return P4_TABLE_INVALID;
        for (uint32_t r = 0; r < 2u; ++r) {
            const uint8_t *conditions = row + 4u + r * 20u;
            for (uint32_t k = 0; k < 3u; ++k)
                if ((read32(conditions + k * 4u) >> 24) >= P4_GAME_AI_CONDITION_CALLBACKS)
                    return P4_TABLE_INVALID;
            /*001DEA2C also reads the eighth/default action after all positive cases. */
            for (uint32_t k = 0; k < 8u; ++k) {
                uint8_t group = conditions[12u + k];
                if (group != 8u && group >= 3u) return P4_TABLE_INVALID;
            }
        }
        for (uint32_t k = 0; k < 15u; ++k) {
            const uint8_t *entry = row + 0x2cu + k * 8u;
            if (!entry[0]) continue; /* No read/validation of never-selected padding commands. */
            if ((read32(entry + 4u) >> 24) >= P4_GAME_AI_TARGET_CALLBACKS) return P4_TABLE_INVALID;
            uint16_t action = read16(entry + 2u);
            if (opaque_action(action)) continue;
            uint32_t mask = 1u << (action & 31u);
            if (seen_skills[action >> 5] & mask) continue;
            P4GameSkillDefinition definition;
            int result = p4_game_skills_definition_find(source->skills, action, &definition);
            if (result) return result;
            if (!definition.active || !definition.metadata) return P4_TABLE_INVALID;
            seen_skills[action >> 5] |= mask;
        }
    }
    prepared->bank = *source;
    prepared->bank_count = count;
    prepared->bank_player_script = (uint32_t)player;
    prepared->bank_enemy_script = (uint32_t)enemy;
    return P4_TABLE_OK;
}
int p4_game_elements_ai_bank_bind(P4GameAiPrepared *prepared, const void *identity,
    const uint8_t *loaded, size_t size, uint32_t guest)
{
    if (!prepared || !prepared->bank.rows_identity || identity != prepared->bank.rows_identity ||
        !loaded || !guest || guest >= P4_GUEST_SIZE || size > P4_GUEST_SIZE - guest ||
        size != prepared->bank.rows_size || P4_G2H(guest) != loaded) return P4_TABLE_INVALID;
    if (memcmp(prepared->bank.rows, loaded, size)) return P4_TABLE_CONFLICT;
    if (prepared->bank_native_base && prepared->bank_native_base != guest &&
        (prepared->usage & P4_AI_PRODUCT_ACTIVE) && !p4_ai_native_sources_quiescent(prepared))
        return P4_TABLE_FROZEN;
    prepared->bank_native_base = guest;
    return P4_TABLE_OK;
}
