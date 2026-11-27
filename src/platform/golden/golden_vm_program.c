/* Validated borrowed Golden FLW0 programs; framing follows the existing
 * game_elements_ai_sources reader, without its PS2-only opcode restrictions. */
#include "golden_vm.h"
#include <limits.h>
#include <string.h>
static uint32_t read32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
int p4_golden_vm_program(P4GoldenVmProgram *out, const uint8_t *bytes, size_t size)
{
    if (!out || !bytes || size < 32 || size > UINT32_MAX ||
        memcmp(bytes + 8, "FLW0", 4)) return -1;
    uint32_t sections = read32(bytes + 16), declared = read32(bytes + 4);
    if (sections < 2 || sections > 5 || declared > size ||
        declared < 32 + sections * 16) return -1;
    uint32_t offset[5] = {0}, count[5] = {0}, stride[5] = {0}, mask = 0;
    for (uint32_t i = 0; i < sections; ++i) {
        const uint8_t *record = bytes + 32 + i * 16;
        uint32_t kind = read32(record), cell = read32(record + 4);
        uint32_t cells = read32(record + 8), start = read32(record + 12);
        if (kind >= 5 || (mask & (1u << kind)) ||
            cell != (kind < 2 ? 32u : kind == 2 ? 4u : 1u) ||
            /* FLW0's declared size can exclude the trailing string section;
             * the enclosing immutable span is the backing extent. */
            start > size || cells > (size - start) / cell ||
            (cells && start < 32 + sections * 16)) return -1;
        for (unsigned j = 0; j < 5; ++j)
            if (cells && count[j] && start < offset[j] + count[j] * stride[j] &&
                offset[j] < start + cells * cell) return -1;
        offset[kind] = start; count[kind] = cells; stride[kind] = cell;
        mask |= 1u << kind;
    }
    if ((mask & 5) != 5 || !count[0] || !count[2] || count[2] > INT32_MAX) return -1;
    /* Targets must be opcode boundaries, not the second word of PUSHI/F.
     * Scan targets in sorted-PC-independent order without a per-program bitmap. */
    for (uint32_t pc = 0; pc < count[2];) {
        uint32_t instruction = read32(bytes + offset[2] + (size_t)pc * 4);
        int op = (int16_t)instruction, k = (int16_t)(instruction >> 16);
        if (op < 0 || op > 34 || (op < 2 && pc + 1 >= count[2])) return -1;
        if ((op == 10 || op == 11) && (k < 0 || (uint32_t)k >= count[0])) return -1;
        if ((op == 13 || op == 28) && (k < 0 || (uint32_t)k >= count[1])) return -1;
        if (op == 34 && (k < 0 || (uint32_t)k >= count[4] ||
            !memchr(bytes + offset[4] + k, 0, count[4] - (uint32_t)k))) return -1;
        uint32_t next = pc + (op < 2 ? 2u : 1u);
        if (next == pc + 2)
            for (unsigned kind = 0; kind < 2; ++kind)
                for (uint32_t i = 0; i < count[kind]; ++i)
                    if (read32(bytes + offset[kind] + (size_t)i * 32 + 24) == pc + 1) return -1;
        pc = next;
    }
    for (unsigned kind = 0; kind < 2; ++kind)
        for (uint32_t i = 0; i < count[kind]; ++i) {
            const uint8_t *record = bytes + offset[kind] + (size_t)i * 32;
            if (read32(record + 24) >= count[2] || !memchr(record, 0, 24)) return -1;
        }
    P4GoldenVmProgram program = {0};
    program.bytes = bytes; program.size = size;
    program.code = bytes + offset[2]; program.code_words = count[2];
    program.procedures = bytes + offset[0]; program.procedure_count = count[0];
    program.labels = bytes + offset[1]; program.label_count = count[1];
    program.strings = bytes + offset[4]; program.string_size = count[4];
    program.local_integers = (uint16_t)(bytes[20] | (uint16_t)bytes[21] << 8);
    program.local_floats = (uint16_t)(bytes[22] | (uint16_t)bytes[23] << 8);
    *out = program; return 0;
}
int p4_golden_vm_start(P4GoldenVm *vm, const P4GoldenVmProgram *program,
    size_t procedure, P4GoldenVmBank local, P4GoldenVmBank global,
    const P4GoldenBfTables *tables, P4GoldenAi *ai)
{
    if (!vm || !program || procedure >= program->procedure_count ||
        local.integer_count < program->local_integers ||
        local.float_count < program->local_floats ||
        (local.integer_count && !local.integers) || (local.float_count && !local.floats) ||
        (global.integer_count && !global.integers) || (global.float_count && !global.floats)) return -1;
    P4GoldenVm started = {0};
    started.local = local; started.global = global;
    started.code = program->code; started.code_words = program->code_words;
    started.procedures = program->procedures; started.procedure_count = program->procedure_count;
    started.labels = program->labels; started.label_count = program->label_count;
    started.strings = program->strings; started.string_size = program->string_size;
    started.tables = tables; started.ai = ai;
    uint32_t pc = read32(program->procedures + procedure * 32 + 24);
    if (pc >= program->code_words || pc > INT32_MAX) return -1;
    started.pc = (int32_t)pc;
    *vm = started; return 0;
}
