/* Clean-room Switch profile: 8014bab0b bf-vm-values.md. */
#include "golden_vm.h"
#include <limits.h>
#include <math.h>
#include <string.h>
#pragma STDC FENV_ACCESS ON
#pragma STDC FP_CONTRACT OFF

static uint32_t read32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static int64_t signed64(uint64_t bits) { int64_t n; memcpy(&n, &bits, 8); return n; }
static int32_t signed32(uint32_t bits) { int32_t n; memcpy(&n, &bits, 4); return n; }
static float floating(P4GoldenVmValue v)
{
    uint32_t bits = (uint32_t)v.bits; float f; memcpy(&f, &bits, 4); return f;
}
static void set_float(P4GoldenVmValue *v, float f)
{
    uint32_t bits; memcpy(&bits, &f, 4);
    v->bits = (v->bits & UINT64_C(0xffffffff00000000)) | bits; v->tag = 1;
}
static int64_t convert(P4GoldenVm *vm, float f)
{
    if (isnan(f)) { ++vm->anomalies; return 0; }
    if (f >= 0x1p63f) { ++vm->anomalies; return INT64_MAX; }
    if (f < -0x1p63f) { ++vm->anomalies; return INT64_MIN; }
    return (int64_t)f;
}
static int integer(P4GoldenVm *vm, P4GoldenVmValue v, int64_t *out)
{
    int64_t index = signed64(v.bits);
    switch (v.tag) {
    case 0: case 4: *out = index; return 1;
    case 1: *out = convert(vm, floating(v)); return 1;
    case 2:
        if (index < 0 || (uint64_t)index >= vm->global.integer_count || !vm->global.integers) return 0;
        *out = vm->global.integers[index]; return 1;
    case 3:
        if (index < 0 || (uint64_t)index >= vm->global.float_count || !vm->global.floats) return 0;
        *out = convert(vm, vm->global.floats[index]); return 1;
    default: *out = 0; return 1;
    }
}
static int real(P4GoldenVm *vm, P4GoldenVmValue v, int argument, float *out)
{
    int64_t n;
    if (v.tag == 1) { *out = floating(v); return 1; }
    if (v.tag == 3) {
        n = signed64(v.bits);
        if (n < 0 || (uint64_t)n >= vm->global.float_count || !vm->global.floats) return 0;
        *out = vm->global.floats[n]; return 1;
    }
    if (v.tag == 0 || v.tag == 2 || (v.tag == 4 && argument)) {
        if (!integer(vm, v, &n)) return 0;
        if (argument && v.tag == 2) n = signed32((uint32_t)n);
        volatile float f = (float)n; *out = f; return 1;
    }
    *out = 0; return 1;
}
int p4_golden_vm_integer(P4GoldenVm *vm, int argument, int64_t *out)
{
    return vm && out && argument >= 0 && argument < vm->depth && vm->depth <= 47 &&
        integer(vm, vm->stack[vm->depth - 1 - argument], out);
}
int p4_golden_vm_float(P4GoldenVm *vm, int argument, float *out)
{
    return vm && out && argument >= 0 && argument < vm->depth && vm->depth <= 47 &&
        real(vm, vm->stack[vm->depth - 1 - argument], 1, out);
}
const char *p4_golden_vm_string(P4GoldenVm *vm, int argument)
{
    if (!vm || argument < 0 || argument >= vm->depth || vm->depth > 47) return NULL;
    P4GoldenVmValue v = vm->stack[vm->depth - 1 - argument];
    return v.tag == 5 ? (const char *)(uintptr_t)v.bits : NULL;
}
void p4_golden_vm_return_integer(P4GoldenVm *vm, int64_t value)
{
    vm->stack[47] = (P4GoldenVmValue){(uint64_t)value, 0};
}
void p4_golden_vm_return_float(P4GoldenVm *vm, float value) { set_float(&vm->stack[47], value); }
static int push(P4GoldenVm *vm, uint8_t tag, uint64_t bits)
{
    if (vm->depth < 0 || vm->depth >= 47) return 0;
    vm->stack[vm->depth++] = (P4GoldenVmValue){bits, tag}; return 1;
}
static int push_float(P4GoldenVm *vm, float f)
{
    if (vm->depth < 0 || vm->depth >= 47) return 0;
    set_float(&vm->stack[vm->depth++], f); return 1;
}
static int destination(P4GoldenVm *vm, const uint8_t *table, size_t count, int k)
{
    if (!table || k < 0 || (size_t)k >= count) return 0;
    int32_t pc = signed32(read32(table + (size_t)k * 32 + 24));
    if (pc < 0 || (size_t)pc >= vm->code_words) return 0;
    vm->pc = pc; return 1;
}
static int binary(P4GoldenVm *vm, int op)
{
    if (vm->depth < 2) return 0;
    P4GoldenVmValue a = vm->stack[vm->depth - 1], b = vm->stack[vm->depth - 2];
    int truth = 0;
    if ((a.tag == 0 || a.tag == 2) && (b.tag == 0 || b.tag == 2)) {
        int64_t av, bv;
        if (!integer(vm, a, &av) || !integer(vm, b, &bv)) return 0;
        int32_t x = signed32((uint32_t)av), y = signed32((uint32_t)bv), result = 0;
        switch (op) {
        case 14: result = signed32((uint32_t)x + (uint32_t)y); break;
        case 15: result = signed32((uint32_t)x - (uint32_t)y); break;
        case 16: result = signed32((uint32_t)x * (uint32_t)y); break;
        case 17:
            if (!y) { ++vm->anomalies; result = 0; }
            else if (x == INT32_MIN && y == -1) { ++vm->anomalies; result = INT32_MIN; }
            else result = x / y;
            break;
        case 20: result = x != 0 || y != 0; break;
        case 21: result = x != 0 && y != 0; break;
        case 22: result = x == y; break;
        case 23: result = x != y; break;
        case 24: result = x < y; break;
        case 25: result = x > y; break;
        case 26: result = x <= y; break;
        case 27: result = x >= y; break;
        default: return 0;
        }
        vm->depth -= 2; return push(vm, 0, (uint64_t)(int64_t)result);
    }
    float x, y; volatile float result = 0;
    if (!real(vm, a, 0, &x) || !real(vm, b, 0, &y)) return 0;
    switch (op) {
    case 14: result = x + y; break;
    case 15: result = x - y; break;
    case 16: result = x * y; break;
    case 17: result = x / y; break;
    case 20: truth = x != 0 || y != 0; break;
    case 21: truth = x != 0 && y != 0; break;
    case 22: truth = x == y; break;
    case 23: truth = x != y; break;
    case 24: truth = x < y; break;
    case 25: truth = x > y; break;
    case 26: truth = x <= y; break;
    case 27: truth = x >= y; break;
    default: return 0;
    }
    vm->depth -= 2;
    return op <= 17 ? push_float(vm, result) : push(vm, 0, (uint64_t)truth);
}
int p4_golden_vm_run(P4GoldenVm *vm, size_t steps)
{
    if (!vm || !vm->code || vm->depth < 0 || vm->depth > 47) return -1;
    while (steps--) {
        if (vm->pc < 0 || (size_t)vm->pc >= vm->code_words) return -1;
        uint32_t instruction = read32(vm->code + (size_t)vm->pc * 4);
        int op = (int16_t)(instruction & 0xffff), k = (int16_t)(instruction >> 16);
        int64_t n; float f;
        P4GoldenVmBank *bank = op >= 30 && op <= 33 ? &vm->local : &vm->global;
        if (op >= 14 && op <= 27 && op != 18 && op != 19) {
            if (!binary(vm, op)) return -1;
        } else switch (op) {
        case 0: case 1:
            if ((size_t)vm->pc + 1 >= vm->code_words) return -1;
            instruction = read32(vm->code + ((size_t)vm->pc + 1) * 4);
            if (op == 0) { if (!push(vm, 0, (uint64_t)(int64_t)signed32(instruction))) return -1; }
            else { memcpy(&f, &instruction, 4); if (!push_float(vm, f)) return -1; }
            ++vm->pc; break;
        case 2: case 30:
            if (k < 0 || (size_t)k >= bank->integer_count || !bank->integers ||
                !push(vm, 0, (uint64_t)bank->integers[k])) return -1;
            break;
        case 3: case 31:
            if (k < 0 || (size_t)k >= bank->float_count || !bank->floats ||
                !push_float(vm, bank->floats[k])) return -1;
            break;
        case 4:
            if (!push(vm, vm->stack[47].tag, vm->stack[47].bits)) return -1;
            break;
        case 5: case 32:
            if (k < 0 || (size_t)k >= bank->integer_count || !bank->integers || vm->depth < 1 ||
                !integer(vm, vm->stack[vm->depth - 1], &n)) return -1;
            --vm->depth; bank->integers[k] = n; break;
        case 6: case 33:
            if (k < 0 || (size_t)k >= bank->float_count || !bank->floats || vm->depth < 1 ||
                !real(vm, vm->stack[vm->depth - 1], 0, &f)) return -1;
            --vm->depth; bank->floats[k] = f; break;
        case 7: case 12: break;
        case 8: {
            uint32_t command = (uint32_t)(int32_t)k;
            const P4GoldenBfCommand *entry = p4_golden_bf_resolve(vm->tables, command);
            if (!entry || entry->arity > (uint32_t)vm->depth) return -1;
            int32_t pc = vm->pc;
            int success;
            if (entry->golden) {
                int64_t args[47];
                for (uint32_t i = 0; i < entry->arity; ++i)
                    if (!p4_golden_vm_integer(vm, (int)i, &args[i])) return -1;
                P4GoldenAiResult result = {signed64(vm->stack[47].bits), vm->stack[47].tag};
                success = entry->golden(P4_GAME_MODE_GOLDEN, vm->ai, command, args, &result);
                vm->stack[47] = (P4GoldenVmValue){(uint64_t)result.value, (uint8_t)result.type};
            } else {
                if (!vm->native || !entry->native_function) return -1;
                success = vm->native(vm, entry->native_function, command);
            }
            if (!success) return 1;
            if (vm->depth < 0 || entry->arity > (uint32_t)vm->depth) return -1;
            vm->depth -= (int32_t)entry->arity;
            if (vm->pc != pc) continue;
            break;
        }
        case 9:
            if (!vm->depth) return 2;
            if (!integer(vm, vm->stack[vm->depth - 1], &n)) return -1;
            --vm->depth; vm->pc = signed32((uint32_t)n + 1u); continue;
        case 10: case 11:
            if (op == 11 && !push(vm, 4, (uint64_t)(int64_t)vm->pc)) return -1;
            if (!destination(vm, vm->procedures, vm->procedure_count, k)) return -1;
#ifdef P4_GOLDEN_AI_QUALIFICATION
            if(vm->call) vm->call(vm,(size_t)k);
#endif
            continue;
        case 13:
            if (!destination(vm, vm->labels, vm->label_count, k)) return -1;
            continue;
        case 18: {
            if (!vm->depth) return -1;
            P4GoldenVmValue *v = &vm->stack[vm->depth - 1];
            if (v->tag == 0) v->bits = 0 - v->bits;
            else if (v->tag == 1) set_float(v, -floating(*v));
            else if (v->tag == 2) {
                if (!integer(vm, *v, &n)) return -1;
                vm->global.integers[signed64(v->bits)] = signed64(0 - (uint64_t)n);
            } else if (v->tag == 3) {
                if (!real(vm, *v, 0, &f)) return -1;
                vm->global.floats[signed64(v->bits)] = -f;
            }
            break;
        }
        case 19: case 28: {
            if (!vm->depth) return -1;
            P4GoldenVmValue v = vm->stack[vm->depth - 1];
            int zero = 1;
            if (v.tag == 0 || v.tag == 2) {
                if (!integer(vm, v, &n)) return -1;
                zero = op == 28 ? (uint32_t)n == 0 : n == 0;
                --vm->depth;
            } else if (v.tag == 1 || v.tag == 3) {
                if (!real(vm, v, 0, &f)) return -1;
                zero = f == 0; --vm->depth;
            } else if (op == 19) break;
            if (op == 19) { if (!push(vm, 0, (uint64_t)zero)) return -1; }
            else if (zero) {
                if (!destination(vm, vm->labels, vm->label_count, k)) return -1;
                continue;
            }
            break;
        }
        case 29: if (!push(vm, 0, (uint64_t)(int64_t)k)) return -1; break;
        case 34:
            if (!vm->strings || k < 0 || (size_t)k >= vm->string_size ||
                !push(vm, 5, (uint64_t)(uintptr_t)(vm->strings + k))) return -1;
            break;
        default: return -1;
        }
        vm->pc = signed32((uint32_t)vm->pc + 1u);
    }
    return 0;
}


