#ifndef P4_GOLDEN_VM_H
#define P4_GOLDEN_VM_H
#include "golden_bf.h"

/* Golden storage is independent of the s32 PS2 work pool. Malformed accesses
 * are rejected rather than reproducing the native diagnostic's memory damage. */
typedef struct P4GoldenVmProgram {
    const uint8_t *bytes, *code, *procedures, *labels, *strings;
    size_t size, code_words, procedure_count, label_count, string_size;
    uint16_t local_integers, local_floats;
} P4GoldenVmProgram;
/* Borrow a validated FLW0 image. No script bytes are rewritten or copied. */
int p4_golden_vm_program(P4GoldenVmProgram *out, const uint8_t *bytes, size_t size);
typedef struct P4GoldenVmValue { uint64_t bits; uint8_t tag; } P4GoldenVmValue;
typedef struct P4GoldenVmBank {
    int64_t *integers;
    float *floats;
    size_t integer_count, float_count;
} P4GoldenVmBank;
typedef struct P4GoldenVm {
    P4GoldenVmValue stack[48];
    P4GoldenVmBank global, local;
    const uint8_t *code, *procedures, *labels, *strings;
    size_t code_words, procedure_count, label_count, string_size;
    int32_t pc, depth;
    uint64_t anomalies;
    const P4GoldenBfTables *tables;
    P4GoldenAi *ai;
    /* Native bridge must use the wide argument/result accessors below. */
    int (*native)(struct P4GoldenVm *, uint32_t function, uint32_t command);
#ifdef P4_GOLDEN_AI_QUALIFICATION
    /* Observes actual CALL/JUMP destinations, never substitutes execution. */
    void (*call)(struct P4GoldenVm *,size_t procedure);
#endif
} P4GoldenVm;
/* Return 2=stopped, 1=yielded, -1=unsupported/malformed, 0=step limit.
 * A step limit does not change the next instruction or the result register. */
int p4_golden_vm_run(P4GoldenVm *vm, size_t steps);
int p4_golden_vm_integer(P4GoldenVm *vm, int argument, int64_t *out);
int p4_golden_vm_float(P4GoldenVm *vm, int argument, float *out);
const char *p4_golden_vm_string(P4GoldenVm *vm, int argument);
void p4_golden_vm_return_integer(P4GoldenVm *vm, int64_t value);
void p4_golden_vm_return_float(P4GoldenVm *vm, float value);
/* Initialize only the execution context; caller-owned local banks retain their
 * pooled lifetime, including across CALL/PROC. Returns -1 for invalid backing. */
int p4_golden_vm_start(P4GoldenVm *vm, const P4GoldenVmProgram *program,
    size_t procedure, P4GoldenVmBank local, P4GoldenVmBank global,
    const P4GoldenBfTables *tables, P4GoldenAi *ai);
#endif
