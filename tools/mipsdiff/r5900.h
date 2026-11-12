/* R5900 (PS2 Emotion Engine core) interpreter over the process's identity-mapped guest memory.
 *
 * It decodes the retail code words straight from guest memory, so it runs whatever p4pc_load_image put at
 * a retail address. Integer core with 64-bit registers and the upper halves of the 128-bit ones, the EE FPU
 * (no NaN/Inf/denormals, results truncated, overflow clamped), the MMI instructions the game uses, and
 * VU0 in macro mode. An instruction it does not model stops the run with R5900_UNSUPPORTED and the word.
 * Used by tools/mipsdiff/ovdiff.c. */
#ifndef P4PC_R5900_H
#define P4PC_R5900_H

#include <setjmp.h>
#include <stdint.h>

enum {
    R5900_OK = 0,
    R5900_FAULT,        /* address error / unmapped / hardware register access: the case is invalid */
    R5900_UNSUPPORTED,  /* instruction not modelled (word in c->bad_word) */
    R5900_STEPS,        /* step limit */
    R5900_CALL,         /* the call hook refused the target */
    R5900_TRAP          /* syscall / break / trap instruction */
};

typedef struct R5900 R5900;

/* Called for jal/jalr/j/b to a target outside [lo, hi). Returns 1 when it handled the call (results in
 * the registers, execution continues after the call), 0 to refuse (the run ends with R5900_CALL). */
typedef int (*r5900_call_hook)(R5900 *c, uint32_t target, void *user);

/* Optional, explicitly supplied offline EE kernel environment. The interpreter
 * still executes all original library/allocator instructions. Without this
 * record, COP0 and syscalls retain their strict unsupported/trap behavior. */
typedef struct {
    uint32_t status;
    uint32_t heap_end;
} R5900KernelEnv;

struct R5900 {
    uint64_t r[32], rh[32];          /* low and high 64 bits of the 128-bit GPRs */
    uint64_t hi, lo, hi1, lo1;       /* lower and upper 64-bit banks of the one 128-bit HI/LO pair */
    uint32_t sa;                    /* hardware-visible byte selector, not a bit count */
    uint32_t f[32];                  /* FPR bit patterns */
    uint32_t acc;                    /* FPU accumulator */
    int fcond;
    uint32_t fcr31;
    uint32_t vf[32][4];              /* VU0 macro registers */
    uint32_t vacc[4];
    uint32_t vi[16];
    uint32_t vq, vp, vcli, vstat, vmac, vi_;
    uint32_t lo_pc, hi_pc;           /* the function under test: jal inside stays interpreted */
    r5900_call_hook hook;
    void *user;
    R5900KernelEnv *kernel;
    unsigned long steps, max_steps;
    uint32_t bad_word, bad_pc, pc_now;
    uint32_t fault_addr;
    uint32_t src[32];                /* address of the word a register was last loaded from (0: computed) */
    uint32_t fault_src;              /* for a fault or refused call: where the base/target register came from */
    int special_in;                  /* a NaN/Inf/denormal bit pattern was loaded as a float */
    int trace;                       /* print every instruction (P4_TRACE=1) */
    int indirect;                    /* the call being hooked is jalr/jr through a register */
    const char *fault_why;
    jmp_buf jb;
};

/* Runs the code at `entry` until it returns to the sentinel in $ra (set by r5900_call) or fails. */
int r5900_call(R5900 *c, uint32_t entry);

/* Mnemonic of a word, for reports ("lw", "vmaddx", ...); "?" when unknown. */
const char *r5900_name(uint32_t word);

#define R5900_SENTINEL 0xDEAD0000u

/* 1: execute float instructions with host (IEEE, round to nearest, NaN/Inf/denormals, x86 conversions) semantics
 * instead of the EE's. A difference between the native and the interpreter that disappears in this mode is
 * proven to be float semantics alone: the logic is identical. */
extern int r5900_ieee;

/* EE float conversions shared with the harness. */
float r5900_ee_in(uint32_t bits);
uint32_t r5900_ee_out(long double v);
extern unsigned long *r5900_stat;

#endif
