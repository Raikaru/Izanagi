#include "game_elements_ai_state.h"
#include "game_elements_ai_commands.h"
#include <stdlib.h>

/* CPU-only proof over the actual prepared BF and its canonical native bindings.
 * No procedure name, guessed PC, source hash or numeric8000 by itself is a role.
 * A basic effect is the actual basic setter, a nonnegative8000/zero command
 * argument to the real encoded-command setter, or its successful local-result
 * protocol. The latter derives BOTH flag and command slots from their consumer.
 */
static uint32_t previous(const P4AiScriptProduct *script, uint32_t pc)
{
    while (pc) {
        --pc;
        if (script->nodes[pc].opcode != P4_AI_UNUSED_OPCODE) return pc;
    }
    return P4_AI_EXIT;
}
static int constant(const P4AiScriptProduct *script, uint32_t pc, uint32_t *value)
{
    if (pc == P4_AI_EXIT) return 0;
    const P4AiCfgNode *node = &script->nodes[pc];
    if (node->opcode == 29u) { *value = (uint32_t)(int32_t)node->operand; return 1; }
    if (node->opcode != 0u) return 0;
    const uint8_t *bytes = script->bytes + script->code_offset + (size_t)(pc + 1u) * 4u;
    *value = bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
    return 1;
}
static int native_command(const P4AiCfgNode *node, uint32_t command)
{
    return node->opcode == 8u && (uint16_t)node->operand == command;
}
static int literal_phys_query(const P4AiScriptProduct *script, uint32_t pc,
    const P4AiQueryBinding *binding)
{
    uint32_t cursor = previous(script, pc);
    for (uint32_t p = 0; p < binding->parameter; ++p) {
        if (cursor == P4_AI_EXIT) return 0;
        uint32_t op = script->nodes[cursor].opcode;
        if (op != 0u && op != 1u && op != 29u && op != 30u && op != 31u && op != 34u) return 0;
        cursor = previous(script, cursor);
    }
    uint32_t value;
    return constant(script, cursor, &value) && value == 0u;
}
static int nonzero_flag_branch(const P4AiScriptProduct *script, uint32_t pc, uint32_t *slot)
{
    uint32_t comparison = previous(script, pc);
    if (comparison == P4_AI_EXIT || script->nodes[comparison].opcode != 23u) return 0;
    uint32_t local = previous(script, comparison);
    if (local == P4_AI_EXIT || script->nodes[local].opcode != 30u) return 0;
    uint32_t value;
    if (!constant(script, previous(script, local), &value) || value) return 0;
    *slot = (uint16_t)script->nodes[local].operand;
    return *slot < P4_GAME_AI_VM_LOCAL_INTS;
}
static int flag_is_nonzero(const P4AiScriptProduct *script, uint32_t pc, uint32_t flag)
{
    while ((pc = previous(script, pc)) != P4_AI_EXIT) {
        const P4AiCfgNode *node = &script->nodes[pc];
        if (node->opcode == 32u && (uint16_t)node->operand == flag) {
            uint32_t value;
            return constant(script, previous(script, pc), &value) && value != 0u;
        }
        /* Never infer a definition across control transfer, an opaque native
         * command, a procedure call or a procedure-entry marker. */
        if (node->opcode == 7u || node->opcode == 8u || node->opcode == 9u ||
            node->opcode == 10u || node->opcode == 11u || node->opcode == 13u || node->opcode == 28u) break;
    }
    return 0;
}
static int mark_basic_expression(P4AiScriptProduct *script, uint32_t branch)
{
    uint32_t need = 1u, pending_return = 0u, pc = branch;
    while ((need || pending_return) && (pc = previous(script, pc)) != P4_AI_EXIT) {
        P4AiCfgNode *node = &script->nodes[pc];
        uint32_t op = node->opcode;
        if (pending_return && op != 8u) break;
        if (op == 8u) {
            if (pending_return) {
                const P4AiQueryBinding *binding = p4_ai_query_binding((uint16_t)node->operand);
                if (binding && literal_phys_query(script, pc, binding)) {
                    if (node->query_role && node->query_role != P4_GAME_AI_QUERY_BASIC) return P4_TABLE_CONFLICT;
                    node->query_role = P4_GAME_AI_QUERY_BASIC;
                }
            }
            pending_return = 0u;
            need += node->command_arity;
        } else if (op == 4u) {
            if (!need) break;
            --need; pending_return = 1u;
        } else if (op == 0u || op == 1u || op == 29u || op == 30u || op == 31u || op == 34u) {
            if (!need) break;
            --need;
        } else if ((op >= 14u && op <= 17u) || (op >= 20u && op <= 27u)) {
            ++need;
        } else if (op != 18u && op != 19u) {
            break;
        }
        if (need > P4_GAME_AI_VM_STACK_SLOTS) return P4_TABLE_INVALID;
    }
    return P4_TABLE_OK;
}
int p4_ai_script_basic_roles(P4AiScriptProduct *script)
{
    if (!script || !script->nodes || !script->code_count) return P4_TABLE_INVALID;
    enum { FLAG_WORDS = (P4_GAME_AI_VM_LOCAL_INTS + 31u) / 32u };
    uint32_t protocols[P4_GAME_AI_VM_LOCAL_INTS][FLAG_WORDS] = {{0}};
    /* A command-local protocol is only recognized behind the actual caller's
     * flag!=0 branch and actual native encoded-command setter. */
    for (uint32_t pc = 0; pc < script->code_count; ++pc) {
        const P4AiCfgNode *branch = &script->nodes[pc];
        uint32_t flag;
        if (branch->opcode != 28u || !nonzero_flag_branch(script, pc, &flag)) continue;
        for (uint32_t r = 0; r < branch->region_count; ++r) {
            const P4AiCfgRegion *region = &script->regions[branch->first_region + r];
            for (uint32_t q = region->first; q <= region->last; ++q) {
                if (!native_command(&script->nodes[q], 0x203eu)) continue;
                uint32_t local = previous(script, q);
                if (local == P4_AI_EXIT || script->nodes[local].opcode != 30u) continue;
                uint32_t command = (uint16_t)script->nodes[local].operand;
                if (command >= P4_GAME_AI_VM_LOCAL_INTS) return P4_TABLE_INVALID;
                protocols[command][flag >> 5] |= 1u << (flag & 31u);
            }
        }
    }
    if ((size_t)script->code_count + 1u > SIZE_MAX / sizeof(uint32_t)) return P4_TABLE_NOMEM;
    uint32_t *prefix = calloc((size_t)script->code_count + 1u, sizeof *prefix);
    if (!prefix) return P4_TABLE_NOMEM;
    for (uint32_t pc = 0; pc < script->code_count; ++pc) {
        const P4AiCfgNode *node = &script->nodes[pc];
        uint32_t basic = native_command(node, 0x203du), value;
        if (native_command(node, 0x203eu) && constant(script, previous(script, pc), &value) &&
            (value == 0u || value == 0x8000u)) basic = 1u;
        if (node->opcode == 32u && constant(script, previous(script, pc), &value) && value == 0u &&
            node->successor[0] != P4_AI_EXIT && script->nodes[node->successor[0]].opcode == 9u) {
            uint32_t command = (uint16_t)node->operand;
            if (command >= P4_GAME_AI_VM_LOCAL_INTS) { free(prefix); return P4_TABLE_INVALID; }
            for (uint32_t flag = 0; flag < script->local_int_count; ++flag)
                if ((protocols[command][flag >> 5] & (1u << (flag & 31u))) && flag_is_nonzero(script, pc, flag)) {
                    basic = 1u; break;
                }
        }
        prefix[pc + 1u] = prefix[pc] + basic;
    }
    int result = P4_TABLE_OK;
    for (uint32_t pc = 0; pc < script->code_count; ++pc) {
        const P4AiCfgNode *branch = &script->nodes[pc];
        if (branch->opcode != 28u) continue;
        for (uint32_t r = 0; r < branch->region_count; ++r) {
            const P4AiCfgRegion *region = &script->regions[branch->first_region + r];
            if (prefix[region->last + 1u] == prefix[region->first]) continue;
            result = mark_basic_expression(script, pc);
            break;
        }
        if (result) break;
    }
    free(prefix);
    if (!result) {
        for (uint32_t pc = 0; pc < script->code_count; ++pc) {
            const P4AiCfgNode *node = &script->nodes[pc];
            if (node->opcode != 8u || node->query_role) continue;
            const P4AiQueryBinding *binding = p4_ai_query_binding((uint16_t)node->operand);
            if (binding && literal_phys_query(script, pc, binding)) return P4_TABLE_INVALID;
        }
    }
    return result;
}
