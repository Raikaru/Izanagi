/* ovdiff: differential check of the port's native functions against the retail code.
 *
 * For each function it builds cases (random and edge arguments, a scratch guest heap filled with
 * integers, floats and pointers into itself), runs the retail instructions in the R5900 interpreter
 * (r5900.c) and the compiled native on the same guest memory, and compares the return value and every
 * guest-memory write. Where the retail code calls a function, the interpreter calls the native one
 * (so the comparison composes leaf by leaf); a callee with no native is a logged stub on both sides.
 *
 *   ovdiff [-n cases] [-s seed] [-v] [-c case] [-o report.tsv] <SLUS_217.82> <addr|all|@file>...
 *
 * See docs/override_diff.md. Exit status: 0 when no function mismatched. */
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <errno.h>
#include <execinfo.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/personality.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* The thread's float control as the port's EE mode sets it (include/p4pc/hostmath.h): MXCSR on x86, FPCR on AArch64. */
#if defined(__x86_64__) || defined(__i386__)
#define OVD_FPU_GET() __builtin_ia32_stmxcsr()
#define OVD_FPU_SET(v) __builtin_ia32_ldmxcsr(v)
#define OVD_FPU_EE(v) ((v) | 0x6000u | 0x8000u | 0x0040u)     /* round toward zero, flush-to-zero, denormals-are-zero */
#define OVD_FPU_NEAR(v) ((v) & ~0xE040u)
#define OVD_FPU_RESET() __asm__ volatile("fninit")
#else
static inline unsigned ovd_fpcr_get(void) { unsigned long v; __asm__ volatile("mrs %0, fpcr" : "=r"(v)); return (unsigned)v; }
static inline void ovd_fpcr_set(unsigned v) { __asm__ volatile("msr fpcr, %0" : : "r"((unsigned long)v)); }
#define OVD_FPU_GET() ovd_fpcr_get()
#define OVD_FPU_SET(v) ovd_fpcr_set(v)
#define OVD_FPU_EE(v) ((v) | (3u << 22) | (1u << 24))         /* RMode = round toward zero, FZ */
#define OVD_FPU_NEAR(v) ((v) & ~((3u << 22) | (1u << 24)))
#define OVD_FPU_RESET() ((void)0)
#endif

#include "p4pc/guest.h"
#include "p4pc/image.h"
#include "p4pc/platform.h"
#include "p4pc/tables.h"
#include "p4pc/vu0.h"
#include "r5900.h"
#include "../../src/render/p4gl.h"

static int g_verbose;
static int g_replay_paired;     /* --paired: recorded-entry EE/native caller differential */
static int g_paired_device;     /* opt-in production RW raster boundary */
/* -F: the retail code runs on its own the whole way: a callee that has a native is interpreted too (the soft-float double helpers and libm cores run as retail's
 * code beside the native, which keeps its host doubles), so the comparison is the port's double arithmetic against the retail's, not against itself. */
static int g_full;

/* Guest memory: GH turns a guest address into a host pointer, GA a host pointer into its guest address, P4_FN_H2G a host function
 * pointer into the function word the guest holds. Identity casts on i386; on x86_64 the guest RAM is a block at no fixed address. */
#define GH(a) ((void *)P4_G2H(a))
#define GA(p) ((uint32_t)P4_H2G(p))

#define RAM_LO 0x00100000u
#define RAM_HI 0x02000000u
#define NPG ((RAM_HI - RAM_LO) >> 12)
#define ARENA 0x01C00000u
#define DATA_SZ 0x10000u
#define STACK_LO (ARENA + 0x30000u)
#define STACK_TOP (ARENA + 0x38000u - 16u)
#define ARENA_END (ARENA + 0x40000u)
#define POOL_BASE (ARENA + 0x40000u)     /* the engine allocator's blocks */
#define POOL_END (POOL_BASE + 0x40000u)
#define SLOT_ALLOC 0x008873E8u            /* RwEngineInstance memory function slots: malloc, free, realloc, calloc */

/* ---------------------------------------------------------------- tables */

typedef struct {
    uint32_t addr;
    char name[64];
    char ret;
    char args[16];
    int callee_only;
} Sig;

static Sig *g_sigs;
static size_t g_nsigs;

static int sig_cmp(const void *a, const void *b) {
    uint32_t x = ((const Sig *)a)->addr, y = ((const Sig *)b)->addr;
    return x < y ? -1 : x > y;
}

static void load_sigs(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); exit(2); }
    char line[256];
    size_t cap = 0;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#') continue;
        char a[16], name[64], r[4], args[32], origin[4] = "o";
        if (sscanf(line, "%15s %63s %3s %31s %3s", a, name, r, args, origin) < 4) continue;
        if (g_nsigs == cap) { cap = cap ? cap * 2 : 1024; g_sigs = realloc(g_sigs, cap * sizeof *g_sigs); }
        Sig *s = &g_sigs[g_nsigs++];
        s->addr = (uint32_t)strtoul(a, NULL, 16);
        snprintf(s->name, sizeof s->name, "%s", name);
        s->ret = r[0];
        s->callee_only = origin[0] == 'd';
        snprintf(s->args, sizeof s->args, "%s", strcmp(args, "-") ? args : "");
    }
    fclose(f);
    qsort(g_sigs, g_nsigs, sizeof *g_sigs, sig_cmp);
}

static const Sig *find_sig(uint32_t addr) {
    size_t lo = 0, hi = g_nsigs;
    while (lo < hi) {
        size_t m = (lo + hi) / 2;
        if (g_sigs[m].addr < addr) lo = m + 1; else hi = m;
    }
    return lo < g_nsigs && g_sigs[lo].addr == addr ? &g_sigs[lo] : NULL;
}

static const p4pc_native_t *find_native(uint32_t addr) {
    size_t lo = 0, hi = p4pc_native_count;
    while (lo < hi) {
        size_t m = (lo + hi) / 2;
        if (p4pc_native_table[m].addr < addr) lo = m + 1; else hi = m;
    }
    return lo < p4pc_native_count && p4pc_native_table[lo].addr == addr ? &p4pc_native_table[lo] : NULL;
}

/* Callees the harness will not run natively: host I/O, GL, threads. A function that reaches one is skipped.
 * Lines of tools/mipsdiff/impure.txt: `<addr8> reason` or `~name-prefix reason`. */
typedef struct { uint32_t addr; char prefix[48]; char why[80]; } Impure;
static Impure g_impure[256];
static size_t g_nimpure;

static void load_impure(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof line, f) && g_nimpure < 256) {
        if (line[0] == '#' || line[0] == '\n') continue;
        Impure *im = &g_impure[g_nimpure];
        memset(im, 0, sizeof *im);
        char tok[64];
        char why[128] = "";
        if (sscanf(line, "%63s %127[^\n]", tok, why) < 1) continue;
        if (tok[0] == '~') snprintf(im->prefix, sizeof im->prefix, "%s", tok + 1);
        else im->addr = (uint32_t)strtoul(tok, NULL, 16);
        snprintf(im->why, sizeof im->why, "%s", why);
        g_nimpure++;
    }
    fclose(f);
}

/* Intended differences (tools/mipsdiff/expected.txt: `<addr8> reason`): the override does not do what retail does. */
static Impure g_expected[128];
static size_t g_nexpected;

static void load_expected(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof line, f) && g_nexpected < 128) {
        if (line[0] == '#' || line[0] == '\n') continue;
        Impure *im = &g_expected[g_nexpected];
        memset(im, 0, sizeof *im);
        char tok[64], why[128] = "";
        if (sscanf(line, "%63s %127[^\n]", tok, why) < 1) continue;
        im->addr = (uint32_t)strtoul(tok, NULL, 16);
        snprintf(im->why, sizeof im->why, "%s", why);
        g_nexpected++;
    }
    fclose(f);
}

/* Argument domains (tools/mipsdiff/domains.txt: `<addr8> <arg index> <lo> <hi> [reason]`): the values the game can pass, where the
 * type allows more (an enum index, a count the caller bounds). The generator draws such an argument from [lo, hi] only. */
typedef struct { uint32_t addr, arg, lo, hi; } Domain;
static Domain g_domains[128];
static size_t g_ndomains;

static void load_domains(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof line, f) && g_ndomains < 128) {
        if (line[0] == '#' || line[0] == '\n') continue;
        char a[32], i[16], lo[32], hi[32];
        if (sscanf(line, "%31s %15s %31s %31s", a, i, lo, hi) < 4) continue;
        Domain *d = &g_domains[g_ndomains++];
        d->addr = (uint32_t)strtoul(a, NULL, 16);
        d->arg = (uint32_t)strtoul(i, NULL, 0);
        d->lo = (uint32_t)strtoul(lo, NULL, 0);
        d->hi = (uint32_t)strtoul(hi, NULL, 0);
    }
    fclose(f);
}

static const Domain *domain_of(uint32_t addr, size_t arg) {
    for (size_t i = 0; i < g_ndomains; i++) if (g_domains[i].addr == addr && g_domains[i].arg == arg) return &g_domains[i];
    return NULL;
}

static const char *expected_reason(uint32_t addr) {
    for (size_t i = 0; i < g_nexpected; i++) if (g_expected[i].addr == addr) return g_expected[i].why;
    return NULL;
}

static const char *impure_reason(uint32_t addr, const char *name) {
    for (size_t i = 0; i < g_nimpure; i++) {
        if (g_impure[i].prefix[0] ? (name && strncmp(name, g_impure[i].prefix, strlen(g_impure[i].prefix)) == 0)
                                  : g_impure[i].addr == addr)
            return g_impure[i].why;
    }
    return NULL;
}

/* ---------------------------------------------------------------- static analysis of retail code */

static inline uint32_t gw(uint32_t a) { return *(const uint32_t *)GH(a); }

static int in_text(uint32_t a) { return a >= RAM_LO && a < 0x00700000u && !(a & 3); }

typedef struct {
    uint32_t addr, end;
    uint32_t garg;     /* bit i: a_i (GPR 4+i) read before written */
    uint32_t farg;     /* bit i: f(12+i) read before written */
    int hw;            /* touches hardware / scratchpad / syscalls / cop0 */
    int ok;
    uint32_t calls[32];
    int ncalls;
    int unresolved_calls;   /* jalr / jr to a register other than ra */
    int quad;               /* uses lq/sq/lqc2/sqc2: retail masks the low 4 address bits, the natives' VU0 helpers do not */
} Info;

static Info **g_info;     /* individually allocated: callers keep the pointers across later analyze() calls */
static size_t g_ninfo;

/* Source and destination GPRs / FPRs of one instruction (the registers that matter for argument discovery). */
static void regs_of(uint32_t w, uint32_t *gs, uint32_t *gd, uint32_t *fs, uint32_t *fd) {
    uint32_t op = w >> 26, rs = (w >> 21) & 31, rt = (w >> 16) & 31, rd = (w >> 11) & 31, sa = (w >> 6) & 31, fn = w & 63;
    *gs = *gd = *fs = *fd = 0;
#define GS(r) (*gs |= 1u << (r))
#define GD(r) (*gd |= 1u << (r))
    switch (op) {
    case 0:
        switch (fn) {
        case 0x00: case 0x02: case 0x03: case 0x38: case 0x3A: case 0x3B: case 0x3C: case 0x3E: case 0x3F: GS(rt); GD(rd); break;
        case 0x08: GS(rs); break;
        case 0x09: GS(rs); GD(rd); break;
        case 0x0A: case 0x0B: GS(rs); GS(rt); break;                   /* conditional write: not a kill */
        case 0x10: case 0x12: case 0x28: GD(rd); break;
        case 0x11: case 0x13: case 0x29: GS(rs); break;
        case 0x18: case 0x19: GS(rs); GS(rt); GD(rd); break;
        case 0x1A: case 0x1B: GS(rs); GS(rt); break;
        case 0x0C: case 0x0D: break;
        default: GS(rs); GS(rt); GD(rd); break;
        }
        break;
    case 1: GS(rs); break;
    case 2: break;
    case 3: GD(31); break;
    case 4: case 5: case 20: case 21: GS(rs); GS(rt); break;
    case 6: case 7: case 22: case 23: GS(rs); break;
    case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 24: case 25: GS(rs); GD(rt); break;
    case 15: GD(rt); break;
    case 26: case 27: case 34: case 38: GS(rs); GS(rt); GD(rt); break;
    case 32: case 33: case 35: case 36: case 37: case 39: case 55: case 30: GS(rs); GD(rt); break;
    case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 63: case 31: GS(rs); GS(rt); break;
    case 47: case 51: case 49: case 57: case 54: case 62: GS(rs); break;
    case 17:
        switch (rs) {
        case 0: case 2: GD(rt); *fs |= 1u << rd; break;
        case 4: GS(rt); *fd |= 1u << rd; break;
        case 6: GS(rt); break;
        case 8: break;
        default:
            switch (fn) {
            case 0: case 1: case 2: case 3: case 22: case 40: case 41: *fs |= (1u << rd) | (1u << rt); *fd |= 1u << sa; break;
            case 4: *fs |= 1u << rt; *fd |= 1u << sa; break;
            case 5: case 6: case 7: case 36: case 32: *fs |= 1u << rd; *fd |= 1u << sa; break;
            case 24: case 25: case 26: case 30: case 31: *fs |= (1u << rd) | (1u << rt); break;
            case 28: case 29: *fs |= (1u << rd) | (1u << rt); *fd |= 1u << sa; break;
            default: if (fn >= 48) *fs |= (1u << rd) | (1u << rt); break;
            }
        }
        break;
    case 18: if (rs == 1 || rs == 2) GD(rt); else if (rs == 5 || rs == 6) GS(rt); break;
    case 28:
        switch (fn) {
        case 0x10: case 0x12: GD(rd); break;
        case 0x11: case 0x13: GS(rs); break;
        case 0x34: case 0x36: case 0x37: case 0x3C: case 0x3E: case 0x3F: GS(rt); GD(rd); break;
        default: GS(rs); GS(rt); GD(rd); break;
        }
        break;
    default: break;
    }
#undef GS
#undef GD
}

static Info *analyze(uint32_t addr) {
    for (size_t i = 0; i < g_ninfo; i++) if (g_info[i]->addr == addr) return g_info[i];
    g_info = realloc(g_info, (g_ninfo + 1) * sizeof *g_info);
    Info *in = calloc(1, sizeof *in);
    g_info[g_ninfo++] = in;
    in->addr = addr;
    if (!in_text(addr)) return in;
    uint32_t pend = addr, p = addr, limit = addr + 0x10000;
    uint32_t written = 0, fwritten = 0;
    int found = 0;
    for (; p < limit && in_text(p); p += 4) {
        uint32_t w = gw(p), op = w >> 26, rs = (w >> 21) & 31, rt = (w >> 16) & 31;
        int is_branch = op == 4 || op == 5 || op == 6 || op == 7 || (op >= 20 && op <= 23) ||
                        (op == 1 && (rt < 4 || (rt >= 16 && rt < 20))) || ((op == 17 || op == 18) && rs == 8);
        if (is_branch) {
            uint32_t t = p + 4 + (uint32_t)((int32_t)(int16_t)(w & 0xFFFF) << 2);
            if (t > pend) pend = t;
        }
        uint32_t gs, gd, fs, fd;
        regs_of(w, &gs, &gd, &fs, &fd);
        in->garg |= ((gs & ~written) >> 4) & 0xFF;
        in->farg |= ((fs & ~fwritten) >> 12) & 0xFF;
        if (!is_branch) { written |= gd; fwritten |= fd; }
        /* `break 7` after a divide is the compiler's divide-by-zero check: the interpreter traps on it (invalid case) */
        if (op == 0 && ((w & 63) == 12 || ((w & 63) == 13 && ((w >> 6) & 0xFFFFF) != 7))) in->hw = 1;
        if (op == 16) in->hw = 1;
        if (op == 30 || op == 31 || op == 54 || op == 62) in->quad = 1;
        if (op == 15) {
            uint32_t hi = w & 0xFFFF;
            if ((hi >= 0x1000 && hi <= 0x1200) || hi == 0x7000 || hi == 0xB000 || (hi >= 0x1100 && hi < 0x1200)) in->hw = 1;
        }
        if (op == 3) {
            uint32_t t = ((p + 4) & 0xF0000000u) | ((w & 0x3FFFFFFu) << 2);
            int dup = 0;
            for (int i = 0; i < in->ncalls; i++) dup |= in->calls[i] == t;
            if (!dup && in->ncalls < 32) in->calls[in->ncalls++] = t;
        }
        if (op == 0 && (w & 63) == 9) in->unresolved_calls++;
        if (op == 0 && (w & 63) == 8 && rs != 31) in->unresolved_calls++;
        if (op == 0 && (w & 63) == 8 && rs == 31 && p + 8 > pend) { in->end = p + 8; found = 1; break; }
        if (op == 2) {
            uint32_t t = ((p + 4) & 0xF0000000u) | ((w & 0x3FFFFFFu) << 2);
            if ((t < addr || t >= p + 8) && p + 8 > pend) { in->end = p + 8; found = 1; break; }
        }
    }
    in->ok = found;
    return in;
}

/* ---------------------------------------------------------------- native calls */

typedef struct { uint32_t eax, edx; float st0; int has_st0; } NRet;

/* A signature has up to 16 parameters; q/d consume two native i386 words.
 * Selected random/recorded callers still have at most eight source parameters,
 * but their SDK callees (notably sprites/fonts) may use the full signature. */
#define OVD_ARG_WORDS 32

#ifndef P4_GUEST64

void ovd_call8_raw(void *fn, const uint32_t slots[OVD_ARG_WORDS], NRet *out);
uint32_t ovd_stack_top;
__asm__(
    ".text\n.globl ovd_call8_raw\n.type ovd_call8_raw,@function\novd_call8_raw:\n"
    "  pushl %ebp\n  movl %esp,%ebp\n  pushl %esi\n  pushl %ebx\n  pushl %edi\n"
    "  movl 12(%ebp),%esi\n  movl 8(%ebp),%ebx\n"
    "  movl %esp,%edi\n"                       /* the harness stack, restored after the call */
    "  movl ovd_stack_top,%esp\n  andl $-16,%esp\n"   /* the native runs on its own zeroed stack */
    "  movl $32,%ecx\n"
    "4:\n  decl %ecx\n  pushl (%esi,%ecx,4)\n  testl %ecx,%ecx\n  jnz 4b\n"
    "  call *%ebx\n"
    "  movl %edi,%esp\n"
    "  movl 16(%ebp),%ecx\n"
    "  movl %eax,0(%ecx)\n  movl %edx,4(%ecx)\n  movl $0,12(%ecx)\n"
    "  fxam\n  fnstsw %ax\n  andl $0x4500,%eax\n  cmpl $0x4100,%eax\n  je 1f\n"
    "  fstps 8(%ecx)\n  movl $1,12(%ecx)\n"
    "1:\n  popl %edi\n  popl %ebx\n  popl %esi\n  popl %ebp\n  ret\n"
    ".size ovd_call8_raw,.-ovd_call8_raw\n");

#define OVD_STACK_SIZE 0x40000u
#define OVD_STACK_BASE 0x30000000u      /* fixed, so the stack's addresses do not depend on ASLR */
static int on_native_stack(uint32_t v) { return v >= OVD_STACK_BASE && v < OVD_STACK_BASE + OVD_STACK_SIZE; }
#define OVD_STACK_ZERO 0x8000u

#else /* x86_64 */

/* System V: the integer-class arguments in rdi rsi rdx rcx r8 r9, the float-class in xmm0..7, the rest on the stack; the result in
 * rax (rdx) or xmm0. ovd_sysv_raw switches to the native stack, loads the registers, pushes `nstk` stack words and stores rax, rdx
 * and xmm0 into out[0..2]. */
void ovd_sysv_raw(void *fn, const uint64_t *ireg, const uint64_t *xreg, const uint64_t *stk, size_t nstk, uint64_t *out);
uintptr_t ovd_stack_top;
#if defined(__aarch64__)
/* AAPCS64: integer-class arguments in x0..x7, float-class in v0..v7 (doubles here), the rest on the stack; the result in x0 (x1) or v0. ovd_sysv_raw switches to the
 * native stack, loads the registers, copies `nstk` stack words, and stores x0, x1 and d0 into out[0..2]. */
#define OVD_NINT 8
__asm__(
    ".text\n.globl ovd_sysv_raw\n.type ovd_sysv_raw,%function\novd_sysv_raw:\n"
    "  stp x29, x30, [sp, #-48]!\n  mov x29, sp\n  stp x19, x20, [sp, #16]\n  str x5, [sp, #32]\n"
    "  mov x16, x0\n  mov x9, x1\n  mov x10, x2\n  mov x11, x3\n  mov x12, x4\n"
    "  adrp x13, ovd_stack_top\n  ldr x13, [x13, :lo12:ovd_stack_top]\n  and x13, x13, #-16\n"
    "  mov x19, sp\n"
    "  lsl x14, x12, #3\n  add x14, x14, #15\n  and x14, x14, #-16\n  sub x13, x13, x14\n  mov sp, x13\n"
    "  mov x15, #0\n"
    "1:\n  cmp x15, x12\n  b.ge 2f\n  ldr x17, [x11, x15, lsl #3]\n  str x17, [sp, x15, lsl #3]\n  add x15, x15, #1\n  b 1b\n"
    "2:\n  ldp d0, d1, [x10]\n  ldp d2, d3, [x10, #16]\n  ldp d4, d5, [x10, #32]\n  ldp d6, d7, [x10, #48]\n"
    "  ldp x0, x1, [x9]\n  ldp x2, x3, [x9, #16]\n  ldp x4, x5, [x9, #32]\n  ldp x6, x7, [x9, #48]\n"
    "  blr x16\n"
    "  mov sp, x19\n  ldr x15, [sp, #32]\n  str x0, [x15]\n  str x1, [x15, #8]\n  str d0, [x15, #16]\n"
    "  ldp x19, x20, [sp, #16]\n  ldp x29, x30, [sp], #48\n  ret\n"
    ".size ovd_sysv_raw,.-ovd_sysv_raw\n");
#else
#define OVD_NINT 6
__asm__(
    ".text\n.globl ovd_sysv_raw\n.type ovd_sysv_raw,@function\novd_sysv_raw:\n"
    "  pushq %rbp\n  movq %rsp,%rbp\n  pushq %rbx\n  pushq %r12\n  pushq %r13\n  pushq %r14\n"
    "  movq %rsp,%r12\n  movq %r9,%r13\n  movq %rdi,%r11\n  movq %rsi,%rbx\n  movq %rdx,%r14\n  movq %rcx,%r10\n"
    "  movq ovd_stack_top(%rip),%rsp\n  andq $-16,%rsp\n"
    "  testq $1,%r8\n  jz 1f\n  subq $8,%rsp\n"
    "1:\n  movq %r8,%rax\n"
    "2:\n  testq %rax,%rax\n  jz 3f\n  decq %rax\n  pushq (%r10,%rax,8)\n  jmp 2b\n"
    "3:\n  movq 0(%r14),%xmm0\n  movq 8(%r14),%xmm1\n  movq 16(%r14),%xmm2\n  movq 24(%r14),%xmm3\n"
    "  movq 32(%r14),%xmm4\n  movq 40(%r14),%xmm5\n  movq 48(%r14),%xmm6\n  movq 56(%r14),%xmm7\n"
    "  movq 0(%rbx),%rdi\n  movq 8(%rbx),%rsi\n  movq 16(%rbx),%rdx\n  movq 24(%rbx),%rcx\n  movq 32(%rbx),%r8\n  movq 40(%rbx),%r9\n"
    "  movl $8,%eax\n  call *%r11\n"
    "  movq %r12,%rsp\n"
    "  movq %rax,0(%r13)\n  movq %rdx,8(%r13)\n  movq %xmm0,16(%r13)\n"
    "  popq %r14\n  popq %r13\n  popq %r12\n  popq %rbx\n  popq %rbp\n  ret\n"
    ".size ovd_sysv_raw,.-ovd_sysv_raw\n");
#endif

#define OVD_STACK_SIZE 0x40000u
#define OVD_STACK_ZERO 0x8000u
/* The native's private stack is guest memory (the stack arena), as the game's stacks are: a native that passes the address of a local to
 * memcpy or stores it hands out a valid guest address. Such an address differs between the interpreter's run and the native's, so the
 * call log maps both to the same marker. */
static const char *golden_path;   /* -R: replay mode (defined below) */
static uint32_t ovd_stack_glo, ovd_stack_ghi;
static int on_native_stack(uint32_t v) { return v >= ovd_stack_glo && v < ovd_stack_ghi; }

#endif

/* A retail code address the port has a native for: the host function that stands for it (an argument the native calls as code). */
static int ovd_code_native(uint32_t v, void **fn) {
    if (v >= 0x00100000u && v < 0x00560000u && !(v & 3)) {
        const p4pc_native_t *n = find_native(v);
        if (n) { *fn = n->fn; return 1; }
    }
    return 0;
}

/* Calls the native `fn` with the argument slots of a retail call: `cls` is the arguments' class string of the function's signature
 * (p pointer, f float, q/d two slots, the rest integers; NULL: eight integers) and `ret` its result class. On i386 the slots are the
 * stack words. On x86_64 a pointer slot is a guest address and becomes a host pointer (a retail code address a native stands for
 * becomes the native's host function), and a pointer result becomes a guest address again, so the harness sees guest values only. */
static void ovd_call_raw(void *fn, const char *cls, char ret, const uint32_t slots[OVD_ARG_WORDS], NRet *out) {   /* see ovd_classes */
    static int chop = -1;
    if (chop < 0) chop = getenv("P4_OVDIFF_NEAR") == NULL;
#ifndef P4_GUEST64
    (void)cls; (void)ret;
    if (!ovd_stack_top) {
        uint8_t *base = mmap((void *)(uintptr_t)0x30000000u, OVD_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
        if (base == MAP_FAILED) { perror("native stack"); exit(2); }
        mprotect(base, 4096, PROT_NONE);
        ovd_stack_top = (uint32_t)(uintptr_t)(base + OVD_STACK_SIZE - 64);
    }
    memset((void *)(uintptr_t)(ovd_stack_top + 64 - OVD_STACK_ZERO), 0, OVD_STACK_ZERO);
    /* The native runs in the EE float mode the port runs game code in (MXCSR round toward zero, flush-to-zero, denormals-are-zero, include/p4pc/hostmath.h);
     * P4_OVDIFF_NEAR=1 runs it in round-to-nearest. The interpreter's host-float proof pass runs in the same mode. */
    if (chop) {
        unsigned csr = OVD_FPU_GET();
        OVD_FPU_SET(OVD_FPU_EE(csr));
        ovd_call8_raw(fn, slots, out);
        OVD_FPU_SET(csr);
        return;
    }
    ovd_call8_raw(fn, slots, out);
#else
    uint64_t ir[8] = {0}, xr[8] = {0}, stk[16] = {0}, o[3] = {0};
    size_t ni = 0, nx = 0, ns = 0, k = 0;
    char dflt[9] = "iiiiiiii";
    if (!cls) cls = dflt;
    if (!ovd_stack_top) {
        uint8_t *base = p4_guest_stack_alloc(OVD_STACK_SIZE);
        if (!base) { fprintf(stderr, "ovdiff: no guest stack for the natives\n"); exit(2); }
        /* As on i386, nothing is mapped above the game's RAM: a native that runs off the end of it (a wild length) faults at once, as it
         * does on the interpreter's side, instead of walking the rest of the block (stack arena, port storage). A replay keeps its libc heap
         * above RAM and so keeps the block. */
        if (!golden_path) mprotect(GH(RAM_HI), P4_GUEST_SIZE - RAM_HI, PROT_NONE);
        mprotect(base, OVD_STACK_SIZE, PROT_READ | PROT_WRITE);
        mprotect(base, 4096, PROT_NONE);
        ovd_stack_glo = GA(base);
        ovd_stack_ghi = ovd_stack_glo + OVD_STACK_SIZE;
        ovd_stack_top = (uintptr_t)(base + OVD_STACK_SIZE - 64);
    }
    memset((void *)(ovd_stack_top + 64 - OVD_STACK_ZERO), 0, OVD_STACK_ZERO);
    for (size_t i = 0; cls[i] && k < OVD_ARG_WORDS; i++) {
        uint64_t v = slots[k++];
        int isf = 0, memory = 0;
        switch (cls[i]) {
        case 'p': {
            void *code = NULL;
            /* An address below the game's RAM is not memory (i386: unmapped; the interpreter faults on it). A native computes NULL + offset
             * for it, so the same number as a host address is what it dereferences: pass it raw, not as base + offset. */
            if ((uint32_t)v < RAM_LO) { v = (uint32_t)v; break; }
            v = ovd_code_native((uint32_t)v, &code) ? (uint64_t)(uintptr_t)code : (uint64_t)(uintptr_t)GH((uint32_t)v);
            break;
        }
        case 'f': isf = 1; break;
        case 'g': isf = 1; break; /* unlowered four-byte float record */
        case 'e': /* unlowered two-float record */
            v |= (uint64_t)slots[k < OVD_ARG_WORDS ? k++ : OVD_ARG_WORDS - 1] << 32;
#if defined(__aarch64__)
            if (nx <= 6) {
                xr[nx++] = (uint32_t)v;
                xr[nx++] = v >> 32;
                continue;
            }
            nx = 8;
            memory = 1;
#else
            isf = 1;
#endif
            break;
        case 'm': /* packed/unaligned five-to-eight-byte record */
            v |= (uint64_t)slots[k < OVD_ARG_WORDS ? k++ : OVD_ARG_WORDS - 1] << 32;
            /* fall through */
        case 'n': /* packed/unaligned one-to-four-byte record */
#if !defined(__aarch64__)
            memory = 1; /* SysV passes unaligned records in memory. */
#endif
            break;
        case 'q': v |= (uint64_t)slots[k < OVD_ARG_WORDS ? k++ : OVD_ARG_WORDS - 1] << 32; break;
        case 'd': v |= (uint64_t)slots[k < OVD_ARG_WORDS ? k++ : OVD_ARG_WORDS - 1] << 32; isf = 1; break;
        /* An integer slot is a 32-bit guest value. Zero-extended: a callee that takes it as size_t (the port's memset, n bytes) then sees what a
         * native's own caller passes it (a uint32_t widened), not 2^64 - 2 for 0xFFFFFFFE. A 32-bit parameter ignores the upper half. */
        default: v = (uint64_t)(uint32_t)v; break;
        }
        if (!memory && (isf ? nx < 8 : ni < OVD_NINT)) { if (isf) xr[nx++] = v; else ir[ni++] = v; }
        else if (ns < 16) stk[ns++] = v;
    }
    {
        unsigned csr = OVD_FPU_GET();
        if (chop) OVD_FPU_SET(OVD_FPU_EE(csr));
        ovd_sysv_raw(fn, ir, xr, stk, ns, o);
        OVD_FPU_SET(csr);
    }
    memset(out, 0, sizeof *out);
    switch (ret) {
    case 'f': memcpy(&out->st0, &o[2], 4); out->has_st0 = 1; break;
    case 'd': { double d; memcpy(&d, &o[2], 8); out->st0 = (float)d; out->has_st0 = 1; break; }
    case 'p': out->eax = GA((void *)(uintptr_t)o[0]); break;
    case 'q': out->eax = (uint32_t)o[0]; out->edx = (uint32_t)(o[0] >> 32); break;
    default: out->eax = (uint32_t)o[0]; out->edx = (uint32_t)o[1]; memcpy(&out->st0, &o[2], 4); out->has_st0 = ret != 'v' && ret != 'i' ? 1 : 0; break;
    }
#endif
}

/* The register classes to call a native with. On x86_64 they are the ABI table's (the tables' `sig` of the native: the compiled function's real
 * parameter classes, which for a decomp function are its typed prototype), not sigs.txt's, whose decomp rows default to all integers; a signature
 * with a float, double or 64-bit class comes from sigs.txt (the table has none for those). */
static void ovd_classes(const Sig *sg, const p4pc_native_t *n, char cls[17], char *ret) {
    size_t i = 0;
    cls[0] = 0;
    *ret = sg ? sg->ret : 'i';
    if (sg && (strpbrk(sg->args, "fdqgemn") || strchr("fdq", sg->ret))) { snprintf(cls, 17, "%s", sg->args); return; }
    if (n && n->sig && n->sig[0] && !strchr(n->sig, 'x')) {     /* a float or double ('x') says nothing of which: sigs.txt does */
        const char *g = n->sig;
        *ret = g[0] == 'v' ? 'v' : g[0] == 'p' ? 'p' : 'i';
        for (i = 1; g[i] && i < 17; i++) cls[i - 1] = (g[i] == 'p' || g[i] == 'f') ? 'p' : 'i';
        cls[i - 1] = 0;
        return;
    }
    if (sg) snprintf(cls, 17, "%s", sg->args);
}

static void ovd_call8(void *fn, const Sig *sg, const p4pc_native_t *n, const uint32_t slots[OVD_ARG_WORDS], NRet *out) {
#ifdef P4_GUEST64
    char cls[17], ret;
    ovd_classes(sg, n, cls, &ret);
    ovd_call_raw(fn, sg || (n && n->sig) ? cls : NULL, ret, slots, out);
#else
    (void)n;
    ovd_call_raw(fn, sg ? sg->args : NULL, sg ? sg->ret : 'i', slots, out);
#endif
}

/* ---------------------------------------------------------------- dirty-page tracking and recovery */

static uint8_t *g_saved;            /* original content of a dirtied page, indexed by page */
static uint8_t *g_dirty;
static uint32_t g_dlist[NPG];
static uint32_t g_nd;
static volatile int g_tracking;
static sigjmp_buf g_recover;
static volatile int g_recover_armed;
static volatile int g_sig;
static volatile uintptr_t g_sig_addr;

static int g_sigdebug;
#include <ucontext.h>
#if defined(__aarch64__)
#define OVD_FAULT_PC(uc) ((void *)((ucontext_t *)(uc))->uc_mcontext.pc)
#else
#ifndef REG_RIP
#define REG_RIP 16   /* x86_64 gregs index (the feature macros may not have been set when <sys/ucontext.h> first came in) */
#endif
#define OVD_FAULT_PC(uc) ((void *)((ucontext_t *)(uc))->uc_mcontext.gregs[REG_RIP])
#endif
static void on_signal(int sig, siginfo_t *si, void *uc) {
    uintptr_t a = (uintptr_t)si->si_addr;
    if (sig == SIGSEGV && g_tracking && a >= (uintptr_t)GH(RAM_LO) && a < (uintptr_t)GH(RAM_LO) + (RAM_HI - RAM_LO)) {
        uint32_t pg = (uint32_t)((a - (uintptr_t)GH(RAM_LO)) >> 12);
        uint8_t *page = (uint8_t *)GH(RAM_LO + (pg << 12));
        if (!g_dirty[pg]) {
            memcpy(g_saved + (size_t)pg * 4096, page, 4096);
            g_dirty[pg] = 1;
            g_dlist[g_nd++] = pg;
        }
        mprotect(page, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
        return;
    }
    if (g_sigdebug) {   /* only the faults the page tracking does not handle */
        char m[120];
#ifdef P4_GUEST64
        int n = snprintf(m, sizeof m, "sig %d addr %p trk %d armed %d rip %p\n", sig, (void *)a, g_tracking, g_recover_armed,
                         OVD_FAULT_PC(uc));
#else
        int n = snprintf(m, sizeof m, "sig %d addr %p trk %d armed %d\n", sig, (void *)a, g_tracking, g_recover_armed);
#endif
        if (write(2, m, (size_t)n) < 0) { }
#if defined(__aarch64__)
        {
            const mcontext_t *mc = &((ucontext_t *)uc)->uc_mcontext;
            char r[600];
            int k = snprintf(r, sizeof r, "  x0-x8 %llx %llx %llx %llx %llx %llx %llx %llx %llx x19-x22 %llx %llx %llx %llx lr %llx\n",
                             (unsigned long long)mc->regs[0], (unsigned long long)mc->regs[1], (unsigned long long)mc->regs[2], (unsigned long long)mc->regs[3],
                             (unsigned long long)mc->regs[4], (unsigned long long)mc->regs[5], (unsigned long long)mc->regs[6], (unsigned long long)mc->regs[7],
                             (unsigned long long)mc->regs[8], (unsigned long long)mc->regs[19], (unsigned long long)mc->regs[20], (unsigned long long)mc->regs[21],
                             (unsigned long long)mc->regs[22], (unsigned long long)mc->regs[30]);
            if (write(2, r, (size_t)k) < 0) { }
        }
#endif
    }
    if (g_recover_armed) {
        if (g_sigdebug) { void *bt[24]; int nb = backtrace(bt, 24); backtrace_symbols_fd(bt, nb, 2); }   /* P4_SIGDEBUG: the native's call chain at the fault */
        g_sig = sig;
        g_sig_addr = a;
        siglongjmp(g_recover, 1);
    }
    {
        char m[120];
        int n = snprintf(m, sizeof m, "ovdiff: unhandled signal %d at %p (tracking %d, armed %d)\n", sig, (void *)a, g_tracking, g_recover_armed);
        if (write(2, m, (size_t)n) < 0) { }
        void *bt[24];
        backtrace_symbols_fd(bt, backtrace(bt, 24), 2);      /* a harness crash (a glibc abort, say) says where */
    }
    signal(sig, SIG_DFL);
    raise(sig);
}

static void track_begin(void) {
    g_nd = 0;
    g_tracking = 1;
    mprotect(GH(RAM_LO), RAM_HI - RAM_LO, PROT_READ | PROT_EXEC);
}

typedef struct {
    uint32_t n, total;
    uint32_t pg[512];
    uint8_t *orig, *now;      /* n pages each */
} Writes;

static Writes *writes_new(void) {
    Writes *w = calloc(1, sizeof *w);
    w->orig = malloc(512 * 4096);
    w->now = malloc(512 * 4096);
    return w;
}

static void paired_device_end(void);

/* Observe the caller before device teardown, then roll back both its writes
 * and any cleanup writes. The real EE allocator keeps bins in guest memory:
 * restoring those pages before RASTERDESTROY invalidates its live allocations. */
static void track_end(Writes *w) {
    w->total = g_nd;
    w->n = g_nd > 512 ? 512 : g_nd;
    for (uint32_t i = 0; i < w->n; i++) {
        uint32_t pg = g_dlist[i];
        uint8_t *page = (uint8_t *)GH(RAM_LO + (pg << 12));
        w->pg[i] = pg;
        memcpy(w->now + (size_t)i * 4096, page, 4096);
        memcpy(w->orig + (size_t)i * 4096, g_saved + (size_t)pg * 4096, 4096);
    }
    /* Keep tracking enabled: destruction can dirty previously untouched pages,
     * which must be restored without becoming part of the caller observation. */
    if (g_paired_device) paired_device_end();
    g_tracking = 0;
    mprotect(GH(RAM_LO), RAM_HI - RAM_LO, PROT_READ | PROT_WRITE | PROT_EXEC);
    for (uint32_t i = 0; i < g_nd; i++) {
        uint32_t pg = g_dlist[i];
        memcpy(GH(RAM_LO + (pg << 12)), g_saved + (size_t)pg * 4096, 4096);
        g_dirty[pg] = 0;
    }
    g_nd = 0;
}

/* ---------------------------------------------------------------- PRNG */

static uint64_t g_rng;
static uint32_t rnd(void) {
    g_rng ^= g_rng << 13; g_rng ^= g_rng >> 7; g_rng ^= g_rng << 17;
    return (uint32_t)(g_rng >> 16);
}
static void seed_rng(uint64_t s) { g_rng = s * 0x9E3779B97F4A7C15ull + 0x1234567ull; for (int i = 0; i < 4; i++) rnd(); }
static float f_of(uint32_t b) { float f; memcpy(&f, &b, 4); return f; }
static uint32_t b_of(float f) { uint32_t b; memcpy(&b, &f, 4); return b; }

static const uint32_t kEdgeInt[] = { 0, 1, 2, 3, 4, 7, 8, 15, 16, 0x7F, 0x80, 0xFF, 0x100, 0x7FFF, 0x8000, 0xFFFF, 0x10000,
                                     0x7FFFFFFF, 0x80000000u, 0xFFFFFFFFu, 0xFFFFFFFEu, 0xFFFFFF80u, 0xFFFF8000u, 0x55555555, 0xAAAAAAAAu };
static const uint32_t kEdgeFloat[] = { 0x00000000, 0x80000000, 0x3F800000, 0xBF800000, 0x3F000000, 0xBF000000, 0x40000000, 0x40490FDB,
                                       0x3A83126F, 0x358637BD, 0x447A0000, 0xC47A0000, 0x42C80000, 0x3DCCCCCD, 0x4B000000, 0x7F7FFFFF,
                                       0xFF7FFFFF, 0x00800000, 0x3FC90FDB, 0xBFC90FDB, 0x437F0000, 0x43800000 };
static const uint32_t kSpecialFloat[] = { 0x7F800000, 0xFF800000, 0x7FC00000, 0xFFC00000, 0x00000001, 0x80000001, 0x007FFFFF, 0x7F800001 };

/* Words of the arena the retail code used as a pointer (or a function pointer) and found invalid: later cases of the
 * same function fill them with a pointer into the arena (kind 0) or a harness callback (kind 1), so structures the
 * function walks become valid without knowing their layout. Learned from the cases run so far, deterministically. */
#define ARGTAG 0xF0000000u
#define MAXHINT 96
static int g_argptr[8];         /* argument i was dereferenced by the retail code: generate a pointer for it */
static int g_smallint;          /* a case hit the step limit: draw smaller integers */
static int g_quad;              /* the function uses quadword accesses: pointer arguments are 16-byte aligned */
static uint32_t g_hint[MAXHINT];
static int g_hint_kind[MAXHINT];
static int g_nhint;

#ifdef P4_GUEST64
static uint32_t ov_cb(uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d);
#else
static uint32_t ov_cb(uint32_t a, uint32_t b, uint32_t c, uint32_t d);
#endif

static int add_hint(uint32_t y, int kind) {
    if (y < ARENA || y >= ARENA + DATA_SZ || (y & 3)) return 0;
    for (int i = 0; i < g_nhint; i++) if (g_hint[i] == y) { if (kind && !g_hint_kind[i]) { g_hint_kind[i] = 1; return 1; } return 0; }
    if (g_nhint >= MAXHINT) return 0;
    g_hint[g_nhint] = y;
    g_hint_kind[g_nhint++] = kind;
    return 1;
}

/* tools/mipsdiff/engine_slots.txt */
static uint32_t g_slot_addr[128], g_slot_fn[128];
static int g_nslots;

static void load_slots(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[128];
    while (fgets(line, sizeof line, f) && g_nslots < 128) {
        unsigned a, v;
        if (line[0] == '#' || sscanf(line, "%x %x", &a, &v) != 2) continue;
        g_slot_addr[g_nslots] = a;
        g_slot_fn[g_nslots++] = v;
    }
    fclose(f);
}

/* A random 32-bit value that is not an address of the harness's own memory (its data and heap near 0x08000000, the libraries and
 * stack from 0xF0000000 up to the top page): the natives are handed such values as integers and a wild write must not land in
 * the harness. */
static uint32_t rnd_safe(void) {
    for (;;) {
        uint32_t v = rnd();
        if ((v >= 0x07000000u && v < 0x10000000u) || (v >= 0xF0000000u && v < 0xFFFF0000u) || (v >= 0x30000000u && v < 0x30040000u) ||
            (v >= 0x40000000u && v < 0x40400000u)) continue;
        return v;
    }
}

static uint32_t arena_ptr(int align) {
    uint32_t off = rnd() % (DATA_SZ - 512);
    return ARENA + (off & ~(uint32_t)(align - 1));
}
/* An untyped scratch word can later be interpreted as a guest pointer. Never
 * let random scratch contents alias native executable statics through the
 * port's image window. Explicit known callbacks are installed separately. */
static uint32_t synthetic_word(uint32_t value) {
    return value >= 0x04000000u && value < 0x10000000u ? 0 : value;
}

static int synthetic_word_selftest(void) {
    static const struct { uint32_t in, out; } cases[] = {
        {0, 0}, {ARENA, ARENA}, {0x03ffffffu, 0x03ffffffu},
        {0x04000000u, 0}, {0x04fcdfc6u, 0}, {0x0fffffffu, 0},
        {0x10000000u, 0x10000000u}, {0xffffffffu, 0xffffffffu}
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i)
        if (synthetic_word(cases[i].in) != cases[i].out) return 1;
    return 0;
}

static uint32_t gen_float(int special) {
    uint32_t k = rnd() % 100;
    if (special && k < 25) return kSpecialFloat[rnd() % (sizeof kSpecialFloat / 4)];
    if (k < 35) return kEdgeFloat[rnd() % (sizeof kEdgeFloat / 4)];
    if (k < 60) return b_of((float)((int32_t)(rnd() % 20001) - 10000) / 100.0f);
    if (k < 80) return b_of((float)((int32_t)(rnd() % 2001) - 1000) / 1000.0f);
    if (k < 90) return b_of((float)((int32_t)rnd() / 4096.0f));
    return b_of(ldexpf((float)(rnd() % 2000 + 1) / 1000.0f, (int)(rnd() % 40) - 20) * ((rnd() & 1) ? 1.f : -1.f));
}

static uint32_t ov_word(void *fn, const char *sig);
static void ov_install_slots(uint32_t *slot);
static void *ov_malloc(uint32_t size, uint32_t flags);
static void ov_free(void *p);
static void *ov_realloc(void *p, uint32_t size);
static void *ov_calloc(uint32_t n, uint32_t size);
static void fill_arena(int mode, int special) {
    uint32_t *d = (uint32_t *)GH(ARENA);
    for (uint32_t i = 0; i < DATA_SZ / 4; i++) {
        uint32_t k = rnd() % 100, v;
        switch (mode) {
        case 0:  /* integers and zeros */
            v = k < 40 ? 0 : k < 70 ? rnd() % 64 : k < 85 ? kEdgeInt[rnd() % (sizeof kEdgeInt / 4)] : k < 95 ? arena_ptr(4) : rnd_safe();
            break;
        case 1:  /* pointer-rich structures */
            v = k < 20 ? 0 : k < 40 ? rnd() % 32 : k < 85 ? arena_ptr(16) : k < 95 ? gen_float(0) : rnd_safe();
            break;
        case 2:  /* floats */
            v = k < 10 ? 0 : k < 75 ? gen_float(special) : k < 85 ? rnd() % 16 : k < 95 ? arena_ptr(16) : rnd_safe();
            break;
        default: /* noise */
            v = rnd_safe();
            break;
        }
        d[i] = synthetic_word(v);
    }
    for (int i = 0; i < g_nhint; i++) {
        uint32_t r = rnd();
        if (r % 10 == 0) continue;                                   /* sometimes leave the random word: null/garbage paths */
        *(uint32_t *)GH(g_hint[i]) = g_hint_kind[i] ? ov_word(ov_cb, "iiiii") : ARENA + 0x1000u * (1 + (r >> 4) % 8) + (((r >> 8) % 4 == 0) ? 4 * ((r >> 12) % 16) : 0);
    }
    memset(GH(ARENA + DATA_SZ), 0, 0x38000 - DATA_SZ);
    memset(GH(POOL_BASE), 0xCD, 0x4000);
    /* The scene camera (kwlnGetCamera = *0x00764B70): a camera object whose frame (+4) is a clean frame with identity
     * matrices (local at +0x10, LTM at +0x50), a view matrix at +0x20 and near/far planes at +0x80/+0x84. */
    {
        static const float id[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        uint8_t *cam = (uint8_t *)GH(ARENA + 0xC000u), *fr = (uint8_t *)GH(ARENA + 0xC400u);
        memset(cam, 0, 0x200);
        memset(fr, 0, 0x200);
        *(uint32_t *)GH(0x00764B70u) = ARENA + 0xC000u;
        *(uint32_t *)(cam + 4) = ARENA + 0xC400u;
        memcpy(cam + 0x20, id, 64);
        *(float *)(cam + 0x80) = 1.0f;
        *(float *)(cam + 0x84) = 1000.0f;
        memcpy(fr + 0x10, id, 64);
        memcpy(fr + 0x50, id, 64);
        *(uint32_t *)(fr + 0xA0) = ARENA + 0xC400u;     /* the frame's root is itself, clean: RwFrameGetLTM returns +0x50 */
    }
    for (int i = 0; i < g_nslots; i++) {
        const p4pc_native_t *n = find_native(g_slot_fn[i]);
        /* a slot whose native is impure (the Im2D draw functions drive OpenGL, absent in the harness) logs its call like a slot the port lacks */
        *(uint32_t *)GH(g_slot_addr[i]) = n && !impure_reason(n->addr, n->name) ? P4_FN_H2G(n->fn) : ov_word(ov_cb, "iiiii");
    }
    ov_install_slots((uint32_t *)GH(SLOT_ALLOC));
}

/* ---------------------------------------------------------------- the harness state of one run */

typedef struct { uint32_t addr, n; uint32_t a[4]; } CallRec;
enum { OVD_LOG_CAP = 4096 };
static CallRec g_log[OVD_LOG_CAP];
static unsigned g_nlog;
static int g_log_overflow;
static uint32_t g_callback_entry_sp;   /* paired replay's EE-local stack addresses */
static unsigned g_stubcalls[1];
static unsigned g_stub_seq;
static int g_sandbox_native;     /* 1 while the native side runs (trap stubs log) */

static uint32_t stub_ret(uint32_t addr, unsigned n) {
    uint32_t h = (addr + 1) * 2654435761u + n * 40503u;
    h ^= h >> 15;
    return h % 3;   /* 0..2: small, so it is usable as a count, an index or a flag */
}

static void log_call(uint32_t addr, const uint32_t slots[4], uint32_t garg) {
    if (g_nlog >= (g_replay_paired ? OVD_LOG_CAP : 256u)) { g_log_overflow = 1; return; }
    CallRec *r = &g_log[g_nlog++];
    r->addr = addr;
    r->n = 0;
    for (int i = 0; i < 4; i++) {
        uint32_t v = (garg >> i) & 1 ? slots[i] : 0;
        if ((v >= 0xFF000000u && v < 0xFFFFE000u) || (v >= STACK_LO && v < ARENA_END) || on_native_stack(v) ||
            (g_callback_entry_sp && v < g_callback_entry_sp && g_callback_entry_sp - v <= 0x4000u)) v = 0xFF000000u;
        r->a[i] = v;
    }
}

static int trap_hook(const char *name, uint32_t addr, const uint32_t args[4]) {
    (void)name;
    Info *in = analyze(addr);
    uint32_t zero[4] = { 0, 0, 0, 0 };
    log_call(addr, args ? args : zero, in->garg);
    return (int)stub_ret(addr, g_stub_seq++);
}

static uint32_t g_pool;
static uint32_t pool_alloc(uint32_t size) {
    uint32_t p = (g_pool + 15) & ~15u;
    if (size > 0x20000 || p + size > POOL_END) return 0;
    g_pool = p + (size ? size : 1);
    return p;
}
static void *ov_malloc(uint32_t size, uint32_t flags) {
    uint32_t a[4] = { size, flags, 0, 0 };
    log_call(0xFFFF0001u, a, 3);
    return GH(pool_alloc(size));
}
/* The guest address a callback's pointer argument stands for; a pointer below the game's RAM was passed raw (ovd_call_raw). */
static uint32_t ov_ga(const void *p) { return (uintptr_t)p < RAM_LO ? (uint32_t)(uintptr_t)p : GA(p); }
static void ov_free(void *p) {
    uint32_t a[4] = { ov_ga(p), 0, 0, 0 };
    log_call(0xFFFF0002u, a, 1);
}
static void *ov_realloc(void *p, uint32_t size) {
    uint32_t a[4] = { ov_ga(p), size, 0, 0 };
    log_call(0xFFFF0003u, a, 3);
    uint32_t n = pool_alloc(size);
    if (n && ov_ga(p) >= RAM_LO && ov_ga(p) + 64 <= RAM_HI) memcpy(GH(n), p, size < 64 ? size : 64);
    return GH(n);
}
static void *ov_calloc(uint32_t n, uint32_t size) {
    uint32_t a[4] = { n, size, 0, 0 };
    log_call(0xFFFF0004u, a, 3);
    uint32_t p = pool_alloc(n * size);
    if (p) memset(GH(p), 0, n * size);
    return GH(p);
}

#ifdef P4_GUEST64
/* A call site with a float parameter is not adapted (P4_SIG has no float class): its pointer arguments arrive as host pointers in 64-bit
 * registers. Such a value is its guest address; an integer is itself. */
static uint32_t ov_arg(uintptr_t v) { return v > 0xFFFFFFFFul ? GA((void *)v) : (uint32_t)v; }
static uint32_t ov_cb(uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d) {
    uint32_t args[4] = { ov_arg(a), ov_arg(b), ov_arg(c), ov_arg(d) };
#else
static uint32_t ov_cb(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    uint32_t args[4] = { a, b, c, d };
#endif
    log_call(0xFFFF0005u, args, 1);      /* only the first argument is known to exist */
    return 1;
}

/* A paired rendering caller needs the real RW lock contract: the generic
 * logging callback cannot provide CPU pixels. Hydrate each recorded raster
 * through the production CREATE handler, then use the production handlers
 * for lock/upload. Private port shadows live above retail's 32 MiB; they and
 * GL textures are torn down between callers, not copied from one to the other. */
extern int32_t func_003f98a0(void *, void *, int32_t);
extern int32_t func_003fb830(void *, void *, int32_t);
extern int32_t func_003fc5f0(void *, void *, int32_t);
extern int32_t func_003fd230(void *, void *, int32_t);
extern int32_t func_003fd460(void *, void *, int32_t);
extern int32_t func_003fd520(void *, void *, int32_t);
static struct { P4RwRaster *r; P4RwRaster original; uint32_t owned_palette; } g_device_rasters[4096];
static size_t g_device_nrasters;
static int g_device_ready;

static int paired_device_raster(P4RwRaster *r) {
    if (!r) return 0;
    for (size_t i = 0; i < g_device_nrasters; ++i)
        if (g_device_rasters[i].r == r) return 1;
    if (g_device_nrasters == sizeof g_device_rasters / sizeof g_device_rasters[0]) return 0;
    P4RwRaster original = *r;
    if (!func_003f98a0(NULL, r, original.cType | (int32_t)original.cFormat << 8)) return 0;
    g_device_rasters[g_device_nrasters].r = r;
    g_device_rasters[g_device_nrasters].original = original;
    g_device_rasters[g_device_nrasters++].owned_palette =
        r->palette != original.palette ? r->palette : 0;
    return 1;
}

static int32_t paired_device_lock(void *out, void *raster, int32_t mode) {
    uint32_t args[4] = {0, GA(raster), (uint32_t)mode, 0};
    log_call(0x003fc5f0u, args, 6);
    return paired_device_raster(raster) ? func_003fc5f0(out, raster, mode) : 0;
}
static int32_t paired_device_unlock(void *out, void *raster, int32_t mode) {
    uint32_t args[4] = {0, GA(raster), (uint32_t)mode, 0};
    log_call(0x003fd230u, args, 6);
    return func_003fd230(out, raster, mode);
}
static int32_t paired_device_palette_lock(void *out, void *raster, int32_t mode) {
    uint32_t args[4] = {0, GA(raster), (uint32_t)mode, 0};
    log_call(0x003fd460u, args, 6);
    return paired_device_raster(raster) ? func_003fd460(out, raster, mode) : 0;
}
static int32_t paired_device_palette_unlock(void *out, void *raster, int32_t mode) {
    uint32_t args[4] = {0, GA(raster), (uint32_t)mode, 0};
    log_call(0x003fd520u, args, 6);
    return func_003fd520(out, raster, mode);
}
static int paired_device_init(void) {
    if (!g_paired_device) return 1;
    if (!g_device_ready) {
        /* main already creates the production heap above retail RAM. */
        if (p4gl_context_open() != 0) return 0;
        g_device_ready = 1;
    }
    return 1;
}
static int paired_device_begin(void) {
    if (!g_paired_device) return 1;
    *(uint32_t *)GH(0x00887364u) = ov_word(paired_device_lock, "ippi");
    *(uint32_t *)GH(0x00887368u) = ov_word(paired_device_unlock, "ippi");
    *(uint32_t *)GH(0x00887384u) = ov_word(paired_device_palette_lock, "ippi");
    *(uint32_t *)GH(0x00887388u) = ov_word(paired_device_palette_unlock, "ippi");
    return 1;
}
static void paired_device_end(void) {
    while (g_device_nrasters) {
        --g_device_nrasters;
        P4RwRaster *r = g_device_rasters[g_device_nrasters].r;
        /* Free only the palette our production CREATE allocated. */
        r->palette = g_device_rasters[g_device_nrasters].owned_palette;
        func_003fb830(NULL, r, 0);
        *r = g_device_rasters[g_device_nrasters].original;
    }
}

/* The function words the harness puts into guest memory for its own callbacks, with their register signatures (the table
 * adapts a native that declares integers where the callback takes pointers). */
static uint32_t ov_word(void *fn, const char *sig) { return p4_fn_h2g_sig(fn, sig); }
static void ov_install_slots(uint32_t *slot) {
    slot[0] = ov_word((void *)ov_malloc, "pii");
    slot[1] = ov_word((void *)ov_free, "vp");
    slot[2] = ov_word((void *)ov_realloc, "ppi");
    slot[3] = ov_word((void *)ov_calloc, "pii");
}
static int paired_device_callback(void *fn) {
    return fn == (void *)paired_device_lock || fn == (void *)paired_device_unlock ||
        fn == (void *)paired_device_palette_lock || fn == (void *)paired_device_palette_unlock;
}
#ifdef P4_GUEST64
static int ov_callback(uint32_t t) {
    void *f = P4_FN_G2H(t);
    return f == (void *)ov_malloc || f == (void *)ov_free || f == (void *)ov_realloc || f == (void *)ov_calloc ||
        f == (void *)ov_cb || paired_device_callback(f);
}
static const Sig *ov_sig(void *f) {
    static const Sig sm = { 0, "ov_malloc", 'p', "ii", 0 }, sf = { 0, "ov_free", 'v', "p", 0 }, sr = { 0, "ov_realloc", 'p', "pi", 0 },
                     sc = { 0, "ov_calloc", 'p', "ii", 0 }, sb = { 0, "ov_cb", 'i', "iiii", 0 };
    static const Sig sd = {0, "production_rw_raster", 'i', "ppi", 0};
    if (paired_device_callback(f)) return &sd;
    return f == (void *)ov_malloc ? &sm : f == (void *)ov_free ? &sf : f == (void *)ov_realloc ? &sr : f == (void *)ov_calloc ? &sc : &sb;
}
#else
static int ov_callback(uint32_t t) {
    return t == (uint32_t)(uintptr_t)ov_malloc || t == (uint32_t)(uintptr_t)ov_free || t == (uint32_t)(uintptr_t)ov_realloc ||
           t == (uint32_t)(uintptr_t)ov_calloc || t == (uint32_t)(uintptr_t)ov_cb || paired_device_callback((void *)(uintptr_t)t);
}
#endif

/* The port's memcpy and memset (func_0043f810, func_0043f9c8) are linked with --wrap, so both sides log their arguments: a native
 * that copies the wrong amount or to the wrong place shows up as a differing call, not as a crash somewhere in libc. */
void *__real_func_0043f810(void *dst, const void *src, size_t n);
void *__real_func_0043f9c8(void *dst, int c, size_t n);
void *__wrap_func_0043f810(void *dst, const void *src, size_t n) {
    uint32_t a[4] = { GA(dst), GA(src), (uint32_t)n, 0 };
    log_call(0x0043f810u, a, 7);
    return __real_func_0043f810(dst, src, n);
}
void *__wrap_func_0043f9c8(void *dst, int c, size_t n) {
    uint32_t a[4] = { GA(dst), (uint32_t)c, (uint32_t)n, 0 };
    log_call(0x0043f9c8u, a, 7);
    return __real_func_0043f9c8(dst, c, n);
}

static uint32_t g_refused_target;   /* the target of the last indirect call the interpreter refused */
static const char *g_callee;   /* the native callee being run (for crash reports) */
static const char *g_why;       /* why the interpreter side refused a call */
static const char *g_top_name;

/* The decomp's VU0 helpers share one host register file (p4pc_vu0); a retail callee that passes data in VU0 registers
 * (vf10 as a result, say) is a native that uses that file, so the interpreter's registers are copied across the call. */
static void vu_reset_host(void) {
    memset(&p4pc_vu0, 0, sizeof p4pc_vu0);
    p4pc_vu0.vf[0].u[3] = 0x3F800000u;
}
static void vu_seed_host(const uint32_t (*entry_vf)[4]) {
    vu_reset_host();
    if (!entry_vf) return;
    for (int i = 1; i < 32; ++i) memcpy(p4pc_vu0.vf[i].u, entry_vf[i], 16);
    memcpy(p4pc_vu0.acc.u, entry_vf[32], 16);
}
static void vu_to_host(const R5900 *c) {
    for (int i = 0; i < 32; i++) memcpy(p4pc_vu0.vf[i].u, c->vf[i], 16);
    memcpy(p4pc_vu0.acc.u, c->vacc, 16);
    memcpy(&p4pc_vu0.q, &c->vq, 4);
}
static void vu_from_host(R5900 *c) {
    for (int i = 1; i < 32; i++) memcpy(c->vf[i], p4pc_vu0.vf[i].u, 16);
    memcpy(c->vacc, p4pc_vu0.acc.u, 16);
    memcpy(&c->vq, &p4pc_vu0.q, 4);
}

/* The x86 stack slots for a call the retail code makes with registers: positional, so the integer and float
 * registers are interleaved as the callee's C signature does. A callee with a known signature (every override and
 * port native) uses it; others get their used integer registers then their used float registers. */
static void callee_slots_raw(const R5900 *c, uint32_t addr, const Info *in, uint32_t slots[OVD_ARG_WORDS]);

/* Arguments that are the retail address of a function the port has a native for (a callback handed to qsort, a stream
 * function) become the native's address: the native callee calls the argument as host code. */
static void callee_slots(const R5900 *c, uint32_t addr, const Info *in, uint32_t slots[OVD_ARG_WORDS]) {
    callee_slots_raw(c, addr, in, slots);
#ifndef P4_GUEST64
    for (int i = 0; i < OVD_ARG_WORDS; i++) {
        const uint32_t v = slots[i];
        if (v >= 0x00100000u && v < 0x00560000u && !(v & 3)) {
            const p4pc_native_t *n = find_native(v);
            if (n) slots[i] = (uint32_t)(uintptr_t)n->fn;
        }
    }
#endif   /* on x86_64 the slot stays the guest code address: ovd_call_raw turns it into the native for a pointer-class parameter */
}

static void callee_slots_raw(const R5900 *c, uint32_t addr, const Info *in, uint32_t slots[OVD_ARG_WORDS]) {
    const Sig *sg = addr ? find_sig(addr) : NULL;
    memset(slots, 0, OVD_ARG_WORDS * sizeof slots[0]);
    if (sg) {
        size_t ni = 0, nf = 0, words = 0;
        uint32_t spill = 0;
        for (size_t i = 0; sg->args[i] && words < OVD_ARG_WORDS; ++i) {
            const char cls = sg->args[i];
            uint64_t value;
            if (cls == 'f' && nf < 8) value = c->f[12 + nf++];
            else if (cls != 'f' && ni < 8) value = c->r[4 + ni++];
            else {
                /* EE GP and FP sequences each have eight registers. Further
                 * values use the caller's outgoing stack, in parameter order. */
                if (cls == 'q' || cls == 'd' || cls == 'e' || cls == 'm') {
                    spill = (spill + 7u) & ~7u;
                    memcpy(&value, GH((uint32_t)c->r[29] + spill), 8);
                    spill += 8;
                } else {
                    value = *(uint32_t *)GH((uint32_t)c->r[29] + spill);
                    spill += cls == 'f' ? 4u : 8u; /* EE spills GP values with sd, even narrow scalars */
                }
            }
            slots[words++] = (uint32_t)value;
            if ((cls == 'q' || cls == 'd' || cls == 'e' || cls == 'm') && words < OVD_ARG_WORDS)
                slots[words++] = (uint32_t)(value >> 32);
        }
        return;
    }
    int k = 0, hi_i = -1;
    if (!addr) { for (int i = 0; i < 8; i++) slots[i] = (uint32_t)c->r[4 + i]; return; }   /* a host pointer of unknown signature: all argument registers */
    for (int i = 0; i < 8; i++) if ((in->garg >> i) & 1) hi_i = i;
    if (hi_i < 3 && !in->farg) hi_i = 3;   /* a thin wrapper passes a1..a3 through to a tail call without reading them: always pass four */
    for (int i = 0; i <= hi_i && k < 8; i++) slots[k++] = (uint32_t)c->r[4 + i];
    for (int i = 0; i < 8 && k < 8; i++) if ((in->farg >> i) & 1) slots[k++] = c->f[12 + i];
}

/* What a native callee leaves in the result registers. A void callee leaves them alone: retail callers rely on that
 * (func_003f1600 keeps its index in $v0 across the void 003f0510), while the native's eax is garbage. */
static void set_results(R5900 *c, const Sig *sg, const NRet *r) {
    char rt = sg ? sg->ret : 'i';
    if (rt == 'v') return;
    if (rt == 'f') { if (r->has_st0) memcpy(&c->f[0], &r->st0, 4); return; }
    if (rt == 'q') c->r[2] = (uint64_t)r->eax | ((uint64_t)r->edx << 32);
    else c->r[2] = (uint64_t)(int64_t)(int32_t)r->eax;
    if (!sg && r->has_st0) memcpy(&c->f[0], &r->st0, 4);
}

/* EE integer results occupy $v0, including all 64 bits for a q result.
 * $v1 is an independent scratch register, not its upper half. */
static uint32_t ee_result_hi(const R5900 *c, char ret) {
    return ret == 'q' ? (uint32_t)(c->r[2] >> 32) : (uint32_t)c->r[3];
}

static int callee_signature_supported(uint32_t address) {
    const Sig *signature = find_sig(address);
    if (signature && strchr(signature->args, '?')) {
        g_why = "unsupported aggregate callee boundary";
        return 0;
    }
    return 1;
}

static int interp_hook(R5900 *c, uint32_t t, void *user) {
    (void)user;
    if (t == 0x0046D730u) { g_why = "assertion failure (kAssertFail: retail halts there, the input is one the game never produces)"; return 0; }
    if (t < RAM_LO || t >= RAM_HI) {
        /* a host function pointer the game's data holds (patched table words, the engine memory slots) */
        uint32_t slots[OVD_ARG_WORDS];
        const p4pc_native_t *hn = NULL;
#ifdef P4_GUEST64
        void *fnp = P4_FN_G2H(t);   /* a function word of the game's data: a code address in the function table */
        for (size_t i = 0; i < p4pc_native_count; i++)
            if (p4pc_native_table[i].fn == fnp) { hn = &p4pc_native_table[i]; break; }
#else
        for (size_t i = 0; i < p4pc_native_count; i++)
            if ((uint32_t)(uintptr_t)p4pc_native_table[i].fn == t) { hn = &p4pc_native_table[i]; break; }
#endif
        Info *hi = hn ? analyze(hn->addr) : NULL;
        callee_slots(c, hn ? hn->addr : 0, hi ? hi : analyze(0), slots);
        if (hn) { const char *why = impure_reason(hn->addr, hn->name); if (why) { g_why = why; return 0; } }
        if (hn && !callee_signature_supported(hn->addr)) return 0;
        /* Only a known function may be called: a native from the table or one of the harness's own callbacks. Any other word that
         * happens to look like a host address is data (a random value from the arena), and calling it would run arbitrary memory. */
        if (!hn && !ov_callback(t)) { g_refused_target = t; g_why = "indirect call to data (a random value used as a function pointer)"; return 0; }
        NRet r;
        g_callee = hn ? hn->name : "a host function pointer";
        vu_to_host(c);
#ifdef P4_GUEST64
        ovd_call8(fnp, hn ? find_sig(hn->addr) : ov_sig(fnp), hn, slots, &r);
#else
        ovd_call8((void *)(uintptr_t)t, NULL, hn, slots, &r);
#endif
        vu_from_host(c);
        set_results(c, hn ? find_sig(hn->addr) : NULL, &r);
        return 1;
    }
    const p4pc_native_t *n = find_native(t);
    Info *in = analyze(t);
    uint32_t slots[OVD_ARG_WORDS];
    callee_slots(c, t, in, slots);
    if (n) {
        if (!callee_signature_supported(t)) return 0;
        const char *why = impure_reason(t, n->name);
        if (why) { g_why = why; return 0; }
        NRet r;
        g_callee = n->name;
        vu_to_host(c);
        ovd_call8(n->fn, find_sig(t), n, slots, &r);
        vu_from_host(c);
        set_results(c, find_sig(t), &r);
        return 1;
    }
    if (c->indirect && (t >= 0x00600000u || !in->ok)) { g_refused_target = t; g_why = "indirect call to data (a random value used as a function pointer)"; return 0; }
    /* No native: both sides see the same logged stub. */
    log_call(t, slots, in->garg);
    c->r[2] = stub_ret(t, g_stub_seq++);
    c->r[3] = 0;
    return 1;
}

/* ---------------------------------------------------------------- one case */

typedef struct {
    uint32_t slot[OVD_ARG_WORDS];  /* positional native words; callees may exceed eight parameters */
    uint32_t entry_sp;       /* random cases use STACK_TOP; replays use the recorded EE stack */
    int mode, special;
    char text[256];
} Case;

static void gen_case(const Sig *s, uint32_t idx, Case *cs) {
    seed_rng(((uint64_t)s->addr << 20) ^ (idx * 7919u + 13u));
    cs->mode = (int)(idx % 4);
    cs->special = (idx % 10) == 9;
    cs->entry_sp = STACK_TOP;
    fill_arena(cs->mode, cs->special);
    memset(cs->slot, 0, sizeof cs->slot);
    char *t = cs->text;
    t += sprintf(t, "mode=%d%s", cs->mode, cs->special ? " special" : "");
    for (size_t i = 0; i < strlen(s->args) && i < 8; i++) {
        uint32_t v = 0;
        uint32_t k = rnd() % 100;
        switch (s->args[i]) {
        case 'p': {
            uint32_t base = ARENA + 0x1000u * (uint32_t)(i + 1);
            v = k < 3 ? 0 : (k < 6 && !g_quad) ? base + 1 + rnd() % 3 : (k < 12 && !g_quad) ? base + 4 * (1 + rnd() % 3) : k < 30 ? base + 16 * (rnd() % 8) : base;
            break;
        }
        case 'f': v = gen_float(cs->special); break;
        case 'b': v = (uint32_t)(int32_t)(int8_t)(k < 50 ? kEdgeInt[rnd() % 14] : rnd()); break;
        case 'B': v = (uint8_t)(k < 50 ? kEdgeInt[rnd() % 14] : rnd()); break;
        case 'h': v = (uint32_t)(int32_t)(int16_t)(k < 50 ? kEdgeInt[rnd() % 18] : rnd()); break;
        case 'H': v = (uint16_t)(k < 50 ? kEdgeInt[rnd() % 18] : rnd()); break;
        default:
            if (g_argptr[i]) { uint32_t base = ARENA + 0x1000u * (uint32_t)(i + 1); v = k < 3 ? 0 : (k < 12 && !g_quad) ? base + 4 * (1 + rnd() % 3) : k < 30 ? base + 16 * (rnd() % 8) : base; break; }
            if (g_smallint) { v = k < 40 ? rnd() % 6 : k < 70 ? kEdgeInt[rnd() % 8] : rnd() % 64; break; }
            v = k < 35 ? kEdgeInt[rnd() % (sizeof kEdgeInt / 4)] : k < 65 ? rnd() % 40 : k < 74 ? arena_ptr(4) : k < 80 ? ARENA + 0x1000u * (uint32_t)(i + 1) : rnd_safe();
            break;
        }
        { const Domain *dm = domain_of(s->addr, i); if (dm) v = dm->lo + rnd() % (dm->hi - dm->lo + 1); }
        cs->slot[i] = v;
        if (s->args[i] == 'f') t += sprintf(t, " a%zu=%g", i, f_of(v));
        else t += sprintf(t, " a%zu=0x%x", i, v);
    }
}

static void load_interp_regs(R5900 *c, const Sig *s, const Case *cs) {
    memset(c, 0, sizeof *c);
    c->vf[0][3] = 0x3F800000u;
    /* EE calling convention: integer-class arguments take a0, a1, ... in order and floats take f12, f13, ...
     * in order, each sequence counted on its own (func_003d65e0(u32 *, u32 *, u32, u32 *, f32 t) gets t in f12). */
    for (size_t i = 0, ni = 0, nf = 0; i < strlen(s->args) && i < 8; i++) {
        if (s->args[i] == 'f') c->f[12 + nf++] = cs->slot[i];
        else { c->src[4 + ni] = ARGTAG | (uint32_t)i; c->r[4 + ni++] = (uint64_t)(int64_t)(int32_t)cs->slot[i]; }
    }
    c->r[29] = cs->entry_sp;
    c->r[28] = 0x007690F0u;
    c->max_steps = 400000;
}

typedef struct {
    int rc;                  /* R5900_* or -1 native signal */
    uint32_t v0, v1, f0;
    CallRec log[OVD_LOG_CAP];
    unsigned nlog;
    Writes *w;
    int special_in;
    uint32_t bad_word, bad_pc, fault_addr, fault_src;
    const char *fault_why;
    int has_f0;
    int log_overflow;
} Result;

static void run_interp(const Sig *s, const Case *cs, Result *r, Info *in, const uint32_t (*entry_vf)[4]) {
    R5900 *c = calloc(1, sizeof *c);
    load_interp_regs(c, s, cs);
    c->lo_pc = g_full ? RAM_LO : s->addr;
    c->hi_pc = g_full ? RAM_HI : in->end;
    c->trace = g_verbose && getenv("P4_TRACE") != NULL;
    c->hook = interp_hook;
    g_nlog = 0;
    g_log_overflow = 0;
    g_stub_seq = 0;
    g_pool = POOL_BASE;
    g_why = NULL;
    g_sig = 0;
    vu_seed_host(entry_vf);
    vu_from_host(c);
    struct itimerval it = { { 0, 0 }, { 2, 0 } };
    track_begin();
    if (sigsetjmp(g_recover, 1) == 0) {
        g_recover_armed = 1;
        setitimer(ITIMER_REAL, &it, NULL);
        r->rc = r5900_call(c, s->addr);
    } else {
        r->rc = -2;       /* a native callee crashed or hung */
    }
    struct itimerval off = { { 0, 0 }, { 0, 0 } };
    setitimer(ITIMER_REAL, &off, NULL);
    g_recover_armed = 0;
    track_end(r->w);
    if (r->rc == -2) OVD_FPU_RESET();
    r->v0 = (uint32_t)c->r[2];
    r->v1 = ee_result_hi(c, s->ret);
    r->f0 = c->f[0];
    r->bad_word = c->bad_word;
    r->bad_pc = c->bad_pc;
    r->fault_addr = c->fault_addr;
    r->fault_src = c->fault_src;
    r->special_in = c->special_in;
    r->fault_why = c->fault_why;
    r->nlog = g_nlog;
    r->log_overflow = g_log_overflow;
    memcpy(r->log, g_log, g_nlog * sizeof(CallRec));
    free(c);
}

static void on_alarm(int sig) { (void)sig; g_sig = SIGALRM; siglongjmp(g_recover, 1); }

static void run_native(const Sig *s, const Case *cs, Result *r, const p4pc_native_t *n, const uint32_t (*entry_vf)[4]) {
    g_nlog = 0;
    g_log_overflow = 0;
    g_stub_seq = 0;
    g_pool = POOL_BASE;
    NRet nr;
    memset(&nr, 0, sizeof nr);
    vu_seed_host(entry_vf);
    g_sig = 0;
    struct itimerval it = { { 0, 0 }, { 1, 0 } };
    track_begin();
    if (sigsetjmp(g_recover, 1) == 0) {
        g_recover_armed = 1;
        setitimer(ITIMER_REAL, &it, NULL);
        ovd_call8(n->fn, s, n, cs->slot, &nr);
        r->rc = 0;
    } else {
        r->rc = -1;
    }
    struct itimerval off = { { 0, 0 }, { 0, 0 } };
    setitimer(ITIMER_REAL, &off, NULL);
    g_recover_armed = 0;
    track_end(r->w);
    if (r->rc == -1) {
        /* the x87 stack may hold junk after a fault */
        OVD_FPU_RESET();
    }
    (void)s;
    r->v0 = nr.eax;
    r->v1 = nr.edx;
    r->has_f0 = nr.has_st0;
    memcpy(&r->f0, &nr.st0, 4);
    r->nlog = g_nlog;
    r->log_overflow = g_log_overflow;
    memcpy(r->log, g_log, g_nlog * sizeof(CallRec));
}

/* ---------------------------------------------------------------- comparison */

static int is_special(uint32_t b) {
    uint32_t e = (b >> 23) & 255;
    return e == 255 || (e == 0 && (b & 0x7FFFFF)) || (b & 0x7FFFFFFF) == 0x7F7FFFFF;
}

/* Known-float compare (a float return value). 0 equal, 1 within the host tolerance, 2 an EE clamp or denormal
 * where IEEE has Inf/NaN/denormals, 3 different. */
static int fcmp(uint32_t a, uint32_t b) {
    if (a == b) return 0;
    float x = f_of(a), y = f_of(b);
    if (is_special(a) || is_special(b)) {
        if (x != x || y != y || isinf(x) || isinf(y)) return 2;
        if ((a & 0x7FFFFFFF) == 0x7F7FFFFF || (b & 0x7FFFFFFF) == 0x7F7FFFFF) return 2;
        if (fabsf(x) < 1.2e-38f && fabsf(y) < 1.2e-38f) return 1;
        return 2;
    }
    double d = fabs((double)x - (double)y), m = fmax(1.0, fmax(fabs((double)x), fabs((double)y)));
    return d <= 2e-5 * m ? 1 : 3;
}

static int floaty(uint32_t b) {
    float x = fabsf(f_of(b));
    return x == 0.0f || (x >= 1e-20f && x <= 1e20f);
}

/* A word in memory of unknown type. Words that look like integers or guest pointers (below 0x02000000 as a bit
 * pattern: pointers, small values and the tiniest floats) must match bit for bit. Other words may be floats: a few
 * ulps apart is the EE truncating where the host rounds to nearest. EE Fmax against an IEEE Inf/NaN is a documented
 * host difference. */
static int wcmp(uint32_t a, uint32_t b) {
    if (a == b) return 0;
    uint32_t ea = (a >> 23) & 255, eb = (b >> 23) & 255;
    if ((ea == 255 || (a & 0x7FFFFFFF) == 0x7F7FFFFF) && (eb == 255 || (b & 0x7FFFFFFF) == 0x7F7FFFFF)) return 2;
    if (((a & 0x7FFFFFFFu) == 0 && ((b >> 23) & 255) == 0) || ((b & 0x7FFFFFFFu) == 0 && ((a >> 23) & 255) == 0)) return 1;   /* the EE flushes a denormal result to zero, the host keeps it */
    if ((a | b) == 0x80000000u) return 1;     /* -0 against +0: the EE and the host differ in the sign of a zero result */
    if ((a & 0x7FFFFFFFu) < 0x02000000u || (b & 0x7FFFFFFFu) < 0x02000000u) return 3;
    if (ea == 255 || eb == 255) return 3;
    if ((a >> 31) == (b >> 31)) {
        uint32_t d = (a & 0x7FFFFFFFu) > (b & 0x7FFFFFFFu) ? (a & 0x7FFFFFFFu) - (b & 0x7FFFFFFFu) : (b & 0x7FFFFFFFu) - (a & 0x7FFFFFFFu);
        if (d <= 16) return 1;
    }
    float x = f_of(a), y = f_of(b);
    double d = fabs((double)x - (double)y), m = fmax(1e-3, fmax(fabs((double)x), fabs((double)y)));
    return d <= 2e-5 * m ? 1 : 3;
}

typedef struct {
    int worst;               /* 0 exact, 1 fp, 2 special, 3 bad */
    char msg[512];
} Verdict;

static int words_match_native_ptr(uint32_t guest, uint32_t host) {
    if (on_native_stack(host) && guest >= STACK_LO && guest < ARENA_END) return 1;   /* a pointer to a local */
    const p4pc_native_t *n = find_native(guest);
#ifdef P4_GUEST64
    if (n) return host == guest || host == P4_FN_H2G(n->fn);   /* the native stores the function word the table gives its function */
#else
    if (n) return (uint32_t)(uintptr_t)n->fn == host || ((uint32_t)(uintptr_t)n->fn >= 0x08000000u && host >= 0x08000000u && host < 0x0C000000u && n->addr == guest && 0);
#endif
    /* a retail code address the port has no native for: the native stores its trap or alias address */
    return guest >= 0x00100000u && guest < 0x00600000u && !(guest & 3) && host >= 0x08000000u && host < 0x0C000000u;
}

static const Writes *g_wa, *g_wb;

/* The final content of a page as `w` left it, or `orig` (the content before the run) when it did not touch it. */
static const uint8_t *page_of(const Writes *w, uint32_t pg, int now) {
    for (uint32_t i = 0; i < w->n; i++)
        if (w->pg[i] == pg) return (now ? w->now : w->orig) + (size_t)i * 4096;
    return NULL;
}

static void judge(const Sig *s, const Result *a, const Result *b, Verdict *v, int exact, uint32_t entry_sp) {
    v->worst = 0;
    v->msg[0] = 0;
    char *m = v->msg;
    char *end = v->msg + sizeof v->msg - 1;
#define NOTE(...) do { if (m < end - 100) m += snprintf(m, (size_t)(end - m), __VA_ARGS__); } while (0)
    if (b->rc == -1) {
        v->worst = 3;
        for (unsigned i = 0; i < b->nlog; i++) {
            if (i >= a->nlog || a->log[i].addr != b->log[i].addr || memcmp(a->log[i].a, b->log[i].a, sizeof a->log[i].a)) {
                NOTE("native crashed (signal %d at %p) after call %u: native 0x%08x(%x,%x,%x,%x) asm %s", g_sig, (void *)g_sig_addr, i, b->log[i].addr, b->log[i].a[0],
                     b->log[i].a[1], b->log[i].a[2], b->log[i].a[3], i < a->nlog ? "differs" : "made no such call");
                return;
            }
        }
        NOTE("native crashed (signal %d at %p)", g_sig, (void *)g_sig_addr);
        return;
    }
    switch (s->ret) {
    case 'v': break;
    case 'f': {
        if (!b->has_f0) { v->worst = 3; NOTE("native returned no float; "); break; }
        int k = exact ? (a->f0 == b->f0 ? 0 : 3) : fcmp(a->f0, b->f0);
        if (k) { if (k > v->worst) v->worst = k; NOTE("ret f0 asm=%g (%08x) native=%g (%08x); ", f_of(a->f0), a->f0, f_of(b->f0), b->f0); }
        break;
    }
    case 'q':
        if (a->v0 != b->v0 || a->v1 != b->v1) { v->worst = 3; NOTE("ret asm=%08x:%08x native=%08x:%08x; ", a->v1, a->v0, b->v1, b->v0); }
        break;
    default: {
        uint32_t x = a->v0, y = b->v0, msk = 0xFFFFFFFFu;
        if (s->ret == 'b' || s->ret == 'B') msk = 0xFF;
        else if (s->ret == 'h' || s->ret == 'H') msk = 0xFFFF;
        if ((x & msk) != (y & msk)) { v->worst = 3; NOTE("ret asm=0x%08x native=0x%08x; ", x & msk, y & msk); }
    }
    }
    if (!exact && a->nlog > b->nlog) {
        /* the retail path calls a function that has no native and the native never calls it (it does the work
         * inline, or not at all): the stub's made-up result makes the rest of the comparison meaningless */
        v->worst = 4;
        NOTE("retail calls 0x%08x (no native), the native does not", a->log[b->nlog].addr);
        return;
    }
    if (a->nlog != b->nlog) {
        v->worst = 3;
        NOTE("stub calls asm=%u native=%u: asm [", a->nlog, b->nlog);
        for (unsigned k = 0; k < a->nlog && k < 6; k++) NOTE("%x(%x,%x) ", a->log[k].addr, a->log[k].a[0], a->log[k].a[1]);
        NOTE("] native [");
        for (unsigned k = 0; k < b->nlog && k < 6; k++) NOTE("%x(%x,%x) ", b->log[k].addr, b->log[k].a[0], b->log[k].a[1]);
        NOTE("]; ");
    } else {
        for (unsigned i = 0; i < a->nlog; i++) {
            if (a->log[i].addr != b->log[i].addr || memcmp(a->log[i].a, b->log[i].a, sizeof a->log[i].a)) {
                v->worst = 3;
                NOTE("stub call %u asm 0x%08x(%x,%x,%x,%x) native 0x%08x(%x,%x,%x,%x); ", i, a->log[i].addr, a->log[i].a[0], a->log[i].a[1],
                     a->log[i].a[2], a->log[i].a[3], b->log[i].addr, b->log[i].a[0], b->log[i].a[1], b->log[i].a[2], b->log[i].a[3]);
                break;
            }
        }
    }
    int shown = 0;
    for (int side = 0; side < 2; side++) {
        const Writes *w = side ? b->w : a->w;
        for (uint32_t i = 0; i < w->n; i++) {
            uint32_t pg = w->pg[i];
            if (side == 1 && page_of(a->w, pg, 1)) continue;       /* already compared from side a */
            const uint8_t *orig = page_of(w, pg, 0);
            const uint8_t *na = page_of(a->w, pg, 1), *nb = page_of(b->w, pg, 1);
            if (!na) na = orig;
            if (!nb) nb = orig;
            uint32_t base = RAM_LO + (pg << 12);
            for (uint32_t off = 0; off < 4096; off += 4) {
                uint32_t addr = base + off;
                if (addr >= STACK_LO && addr < ARENA_END) continue;
                if (entry_sp && addr < entry_sp && entry_sp - addr <= 0x4000u) continue;
                uint32_t wa = *(const uint32_t *)(na + off), wb = *(const uint32_t *)(nb + off);
                if (wa == wb || words_match_native_ptr(wa, wb)) continue;
                int k = exact ? 3 : wcmp(wa, wb);
                if (k > v->worst) v->worst = k;
                if (k >= 2 && shown < 4) { NOTE("mem[0x%08x] asm=%08x native=%08x%s; ", addr, wa, wb, k == 2 ? " (special float)" : ""); shown++; }
                else if (k == 3 && shown < 4) { NOTE("mem[0x%08x] asm=%08x native=%08x; ", addr, wa, wb); shown++; }
                else if (k == 1 && shown < 2) { NOTE("mem[0x%08x] asm=%g native=%g (host float); ", addr, f_of(wa), f_of(wb)); shown++; }
            }
        }
    }
#undef NOTE
}

/* ---------------------------------------------------------------- driver */

typedef struct {
    char status[96];
    unsigned attempts, valid, exact, proven, fp, special, bad, nfault, inconclusive;
    char repro[1200];
    unsigned repro_case;
    char invalid_why[96];
} Report;

static const char *rc_name(int rc) {
    switch (rc) {
    case R5900_FAULT: return "fault";
    case R5900_UNSUPPORTED: return "unsupported";
    case R5900_STEPS: return "step limit";
    case R5900_CALL: return "call refused";
    case R5900_TRAP: return "syscall/trap";
    default: return "?";
    }
}


static void test_function(const Sig *s, unsigned ncases, unsigned only_case, Report *rep) {
    memset(rep, 0, sizeof *rep);
    const p4pc_native_t *nat = find_native(s->addr);
    if (!nat) { snprintf(rep->status, sizeof rep->status, "skipped: no native at this address"); return; }
    Info *in = analyze(s->addr);
    if (!in->ok) { snprintf(rep->status, sizeof rep->status, "skipped: function end not found"); return; }
    if (in->hw) { snprintf(rep->status, sizeof rep->status, "skipped: touches hardware registers/syscalls"); return; }
    const char *why = impure_reason(s->addr, nat->name);
    if (why) { snprintf(rep->status, sizeof rep->status, "skipped: impure (%s)", why); return; }
    if (strpbrk(s->args, "qd?gemn") || s->ret == 'd' || strlen(s->args) > 8) { snprintf(rep->status, sizeof rep->status, "skipped: signature (%c %s) not supported", s->ret, s->args); return; }
    /* a reachable callee that is impure makes the whole function impure */
    for (int i = 0; i < in->ncalls; i++) {
        const p4pc_native_t *cn = find_native(in->calls[i]);
        const char *cw = cn ? impure_reason(in->calls[i], cn->name) : NULL;
        if (cw) { snprintf(rep->status, sizeof rep->status, "skipped: calls impure 0x%08x (%s)", in->calls[i], cw); return; }
    }
    if (g_verbose) fprintf(stderr, "native %s at %p, retail end 0x%08x\n", nat->name, nat->fn, in->end);
    Result A, B, C;
    A.w = writes_new();
    B.w = writes_new();
    C.w = writes_new();
    Verdict v;
    g_nhint = 0;
    memset(g_argptr, 0, sizeof g_argptr);
    g_smallint = 0;
    g_quad = in->quad;
    unsigned idx_lo = 0;     /* with -c the earlier cases still run: they teach the hints the case depends on */
    unsigned idx_hi = only_case != (unsigned)-1 ? only_case + 1 : ncases * 8;
    int hint_budget = 40;
    unsigned unsup = 0, ntimeout = 0;
    uint32_t unsup_word = 0, unsup_pc = 0;
    for (unsigned idx = idx_lo; idx < idx_hi && (only_case != (unsigned)-1 || rep->valid < ncases); idx++) {
        Case cs;
        gen_case(s, idx, &cs);
        rep->attempts++;
        run_interp(s, &cs, &A, in, NULL);
        if (g_verbose) fprintf(stderr, "case %u: %s -> interp rc=%d v0=%08x\n", idx, cs.text, A.rc, A.v0);
        if (A.rc != R5900_OK) {
            if ((A.rc == R5900_FAULT || (A.rc == R5900_CALL && g_why && strncmp(g_why, "indirect call to data", 21) == 0)) && hint_budget > 0 &&
                add_hint(A.fault_src, A.rc == R5900_CALL)) hint_budget--;
            if (A.rc == R5900_FAULT && (A.fault_src & 0xFFFFFFF8u) == ARGTAG && !g_argptr[A.fault_src & 7]) g_argptr[A.fault_src & 7] = 1;
            if (A.rc == R5900_STEPS) g_smallint = 1;
            if (A.rc == R5900_UNSUPPORTED) { unsup++; unsup_word = A.bad_word; unsup_pc = A.bad_pc; if (unsup > 20 && rep->valid == 0) break; }
            else if (A.rc == R5900_CALL) snprintf(rep->invalid_why, sizeof rep->invalid_why, "callee refused: %s", g_why ? g_why : "no handler");
            else if (A.rc == -2) { snprintf(rep->invalid_why, sizeof rep->invalid_why, "native callee %s crashed (signal %d at %p)", g_callee ? g_callee : "?", g_sig, (void *)g_sig_addr); if (g_sig == SIGALRM) ntimeout++; }
            else if (A.rc == R5900_FAULT) snprintf(rep->invalid_why, sizeof rep->invalid_why, "%s at 0x%08x (pc 0x%08x)", A.fault_why, A.fault_addr, in->addr);
            else snprintf(rep->invalid_why, sizeof rep->invalid_why, "%s", rc_name(A.rc));
            rep->nfault++;
            if (ntimeout >= 3) { snprintf(rep->invalid_why, sizeof rep->invalid_why, "hangs (3 timeouts)"); break; }
            continue;
        }
        run_native(s, &cs, &B, nat, NULL);
#ifdef P4_GUEST64
        /* A native that dereferenced a number below 4 GB outside the game's RAM built a host pointer on NULL (NULL + a guest address): the model
         * has no such address, where on i386 the same sum is a guest address. A harness input (a0 = 0 with a pointer as the offset), not a result. */
        if (B.rc == -1 && g_sig == SIGSEGV && g_sig_addr >= RAM_LO && g_sig_addr < 0x100000000ull) {
            snprintf(rep->invalid_why, sizeof rep->invalid_why, "NULL + a guest address (native dereferenced host 0x%lx)", (unsigned long)g_sig_addr);
            rep->nfault++;
            continue;
        }
#endif
        if (B.rc == -1 && g_sig == SIGALRM) ntimeout++;
        if (ntimeout >= 3) { snprintf(rep->invalid_why, sizeof rep->invalid_why, "hangs (3 timeouts)"); break; }
        judge(s, &A, &B, &v, 0, 0);
        /* a NaN/Inf/denormal pattern read as a float: the EE and IEEE disagree by design, whatever came out of it */
        if (v.worst == 3 && (A.special_in || cs.special) && !strstr(v.msg, "stub call") && !strstr(v.msg, "native crashed")) v.worst = 2;
        if (v.worst >= 1 && v.worst <= 3) {
            /* Proof of a host-float difference: the same case with the interpreter doing host float arithmetic matches the
             * native exactly, or (when the native crashed) the retail code with host floats faults at the same address. */
            Verdict v2;
            const uint32_t crash_addr = (uint32_t)g_sig_addr;
            /* host float = IEEE specials and the rounding the natives run in: MXCSR round toward zero, flush-to-zero, denormals-are-zero */
            const unsigned csr_saved = OVD_FPU_GET();
            if (getenv("P4_OVDIFF_NEAR") == NULL) OVD_FPU_SET(OVD_FPU_EE(csr_saved));
            r5900_ieee = 1;
            run_interp(s, &cs, &C, in, NULL);
            r5900_ieee = 0;
            OVD_FPU_SET(csr_saved);
            if (g_verbose) fprintf(stderr, "  host-float rerun: rc=%d fault=0x%08x why=%s v0=%08x (native crash at 0x%08x)\n", C.rc, C.fault_addr, C.fault_why ? C.fault_why : "-", C.v0, crash_addr);
            if (B.rc == -1) {
                if (C.rc == R5900_FAULT && C.fault_addr == crash_addr) v.worst = 5;
                else if (C.rc == -2 && (uint32_t)g_sig_addr == crash_addr) v.worst = 5;   /* the same native callee crashed at the same address */
                else if (C.rc == R5900_CALL && (g_refused_target == crash_addr || g_refused_target >= ARENA)) v.worst = 5;   /* both call a data word: the native runs it as x86 code, the interpreter refuses it */
            } else if (C.rc == R5900_OK) {
                judge(s, &C, &B, &v2, 0, 0);
                if (v2.worst == 0) v.worst = 5;
            }
        }
        if (v.worst == 4) { rep->inconclusive++; if (!rep->invalid_why[0]) snprintf(rep->invalid_why, sizeof rep->invalid_why, "%s", v.msg); continue; }
        rep->valid++;
        switch (v.worst) {
        case 0: rep->exact++; break;
        case 5: rep->proven++; break;
        case 1: rep->fp++; break;
        case 2: rep->special++; break;
        default:
            rep->bad++;
            if (!rep->repro[0]) {
                rep->repro_case = idx;
                snprintf(rep->repro, sizeof rep->repro, "case %u: %s: %s", idx, cs.text, v.msg);
            }
        }
        if (g_verbose) fprintf(stderr, "  verdict %d %s\n", v.worst, v.msg);
    }
    free(A.w->orig); free(A.w->now); free(A.w);
    free(B.w->orig); free(B.w->now); free(B.w);
    free(C.w->orig); free(C.w->now); free(C.w);
    if (rep->valid == 0) {
        if (unsup)
            snprintf(rep->status, sizeof rep->status, "unsupported: %s (word %08x at 0x%08x)", r5900_name(unsup_word), unsup_word, unsup_pc);
        else
            snprintf(rep->status, sizeof rep->status, "%s: %s", rep->inconclusive ? "inconclusive" : "no valid cases", rep->invalid_why[0] ? rep->invalid_why : "?");
    } else if (rep->bad && expected_reason(s->addr)) snprintf(rep->status, sizeof rep->status, "documented difference: %s", expected_reason(s->addr));
    else if (rep->bad) snprintf(rep->status, sizeof rep->status, "MISMATCH");
    else if (rep->special) snprintf(rep->status, sizeof rep->status, "ok (special-float host differences)");
    else if (rep->fp) snprintf(rep->status, sizeof rep->status, "ok (float tolerance)");
    else if (rep->proven) snprintf(rep->status, sizeof rep->status, "ok (host float semantics only, proven)");
    else snprintf(rep->status, sizeof rep->status, "ok");
}

/* A native that writes through a bad pointer must not be able to hit the harness. Integer arguments are drawn from the whole
 * 32-bit range, and 0xFFFF8000 or a random word can be a valid address of the host stack, so after the fork the child runs on
 * a private stack and the main stack is made read-only: such a write faults (a deterministic "native crashed") instead of
 * corrupting the harness's frames, which showed up as "stack smashing detected" or a hang depending on where the stack was. */
#include <ucontext.h>
static void run_one(const Sig *s, unsigned ncases, unsigned only_case, Report *rep);
static ucontext_t g_child_uc;
static struct { const Sig *s; unsigned ncases, only_case; FILE *rf; } g_child;

static void protect_main_stack(void) {
    FILE *f = fopen("/proc/self/maps", "r");
    char line[256];
    if (!f) return;
    while (fgets(line, sizeof line, f)) {
        unsigned long lo, hi;
        if (strstr(line, "[stack]") && sscanf(line, "%lx-%lx", &lo, &hi) == 2) mprotect((void *)lo, hi - lo, PROT_READ);
    }
    fclose(f);
}

static void child_entry(void) {
    protect_main_stack();
    Report rep;
    const Sig *s = g_child.s;
    g_top_name = s->name;
    run_one(s, g_child.ncases, g_child.only_case, &rep);
    fprintf(g_child.rf, "%08x\t%s\t%s\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%s\n", s->addr, s->name, rep.status, rep.attempts, rep.valid, rep.exact, rep.proven,
            rep.fp, rep.special, rep.bad, rep.nfault, rep.inconclusive, rep.repro);
    fflush(g_child.rf);
    _exit(g_replay_paired ? (!rep.valid || rep.bad || rep.nfault || rep.inconclusive || rep.fp || rep.special)
        : (rep.bad != 0 && !expected_reason(s->addr) ? 1 : 0));
}

static void child_run(const Sig *s, unsigned ncases, unsigned only_case, FILE *rf) {
    g_child.s = s; g_child.ncases = ncases; g_child.only_case = only_case; g_child.rf = rf;
    const size_t size = 0x400000;
    void *stk = mmap((void *)(uintptr_t)(sizeof(void *) == 4 ? 0x40000000u : 0u), size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | (sizeof(void *) == 4 ? MAP_FIXED_NOREPLACE : 0), -1, 0);
    if (stk == MAP_FAILED) _exit(3);
    getcontext(&g_child_uc);
    g_child_uc.uc_stack.ss_sp = stk;
    g_child_uc.uc_stack.ss_size = size;
    g_child_uc.uc_link = NULL;
    makecontext(&g_child_uc, child_entry, 0);
    setcontext(&g_child_uc);
    _exit(3);
}

static const char *dir_of(const char *path, char *buf, size_t n) { snprintf(buf, n, "%s", path); char *sl = strrchr(buf, '/'); if (sl) *sl = 0; else strcpy(buf, "."); return buf; }

#include "replay.c"

static const char *golden_path, *windows_path;      /* -R / -W: replay mode (replay.c) */

static void run_one(const Sig *s, unsigned ncases, unsigned only_case, Report *rep) {
    if (golden_path) replay_function(s, rep);
    else test_function(s, ncases, only_case, rep);
}

/* ovdiff -Q: the retail double libm cores (newlib's fdlibm, compiled by EE-GCC onto its soft-float helpers: those round toward zero, but sqrt is correctly rounded) run on the
 * interpreter against the retail soft-double bindings (src/platform/ee_dlibm.c; native-register adapters in ee_dfloat.c). The cores take the double's 64 bits in $a0
 * (and $a1) and return them in $v0, none calls out of the image. Inputs are drawn from what the game passes (the acs.c volume curves, _rwImageOpen's gamma):
 * for each core the share of calls whose results are bit-identical, within one ulp of the double, and (the double is always cast to float by its callers)
 * identical as floats (converted toward zero, as retail does). Then gcc's soft-float add, subtract, multiply, divide and conversion to float against the host's operations
 * in the EE float mode, and ADX_GetCoefficient against the retail asm. Exit status 1 when a core differs as a float, or multiply, divide, conversion, sqrt or the
 * coefficients differ anywhere. */
extern uint64_t func_004470f0(uint64_t), func_00447b88(uint64_t), func_00447d00(uint64_t, uint64_t);
extern double p4pc_ee_dsqrt(double);
extern void ADX_GetCoefficient(int32_t cutoff, int32_t sfreq, int16_t *c1, int16_t *c2);
static int g_dblcores;

/* A native of the port runs in the EE float mode (MXCSR round toward zero, flush to zero), as in the game; the harness itself runs in round-to-nearest. */
static uint64_t ee_call1(uint64_t (*fn)(uint64_t), uint64_t x) {
    const unsigned csr = OVD_FPU_GET();
    uint64_t r;

    OVD_FPU_SET(OVD_FPU_EE(csr));
    r = fn(x);
    OVD_FPU_SET(csr);
    return r;
}

static uint64_t dbl_bits(double d) { uint64_t u; memcpy(&u, &d, 8); return u; }
static double dbl_of(uint64_t u) { double d; memcpy(&d, &u, 8); return d; }

/* The conversion to float as retail does it (gcc's soft-float __truncdfsf2 rounds toward zero). */
static float trunc_f(double d) {
    float f = (float)d;

    if (fabs((double)f) > fabs(d)) f = nextafterf(f, 0.0f);
    return f;
}

static double dbl_rand(uint64_t *st, double lo, double hi) {
    *st = *st * 6364136223846793005ull + 1442695040888963407ull;
    return lo + (hi - lo) * (double)(*st >> 11) / 9007199254740992.0;
}

static int dblcores(unsigned n) {
    static const struct { uint32_t addr; int nargs; const char *name; double lo, hi, lo2, hi2; } T[] = {
        { 0x004470F0u, 1, "__ieee754_acos", -1.0, 1.0, 0, 0 },
        { 0x00447B88u, 1, "__ieee754_log10", 1e-6, 4e6, 0, 0 },
        { 0x00447B88u, 1, "__ieee754_log10 (0..1)", 1e-9, 1.0, 0, 0 },
        { 0x00447D00u, 2, "__ieee754_pow (10^u)", 10.0, 10.0, -6.0, 6.0 },
        { 0x00447D00u, 2, "__ieee754_pow (x^y)", 0.01, 8.0, -4.0, 4.0 },
        { 0x00448A00u, 1, "__ieee754_sqrt", 0.0, 1e9, 0, 0 },
    };
    int bad = 0;
    R5900 *c = calloc(1, sizeof *c);
    uint64_t rng = 0x9E3779B97F4A7C15ull;

    if (n < 100000u) n = 100000u;
    for (size_t t = 0; t < sizeof T / sizeof T[0]; t++) {
        unsigned exact = 0, one = 0, fl = 0, bad_run = 0;
        double worst = 0;

        for (unsigned i = 0; i < n; i++) {
            double x = T[t].lo == T[t].hi ? T[t].lo : dbl_rand(&rng, T[t].lo, T[t].hi);
            double y = T[t].nargs == 2 ? dbl_rand(&rng, T[t].lo2, T[t].hi2) : 0.0;
            uint64_t ib, hb;
            double ref, got;

            if (i % 7 == 0 && T[t].nargs == 1 ) x = (double)(float)x;       /* the callers pass a widened float */
            if (i % 7 == 0 && T[t].nargs == 2) y = (double)(float)y;
            memset(c, 0, sizeof *c);
            c->r[29] = STACK_TOP;
            c->r[4] = dbl_bits(x);
            c->r[5] = dbl_bits(y);
            c->lo_pc = 0x00100000u;
            c->hi_pc = 0x00800000u;      /* every call stays interpreted: the soft-float helpers run as retail's code */
            c->max_steps = 2000000;
            { int rc = r5900_call(c, T[t].addr);

              if (rc != R5900_OK) {
                  if (bad_run++ == 0) fprintf(stderr, "ovdiff -Q: %s stopped (rc %d, bad word %08x at %08x, fault %08x %s) for %a\n", T[t].name, rc, c->bad_word, c->bad_pc, c->fault_addr,
                                              c->fault_why ? c->fault_why : "", x);
                  continue;
              } }
            ib = c->r[2];
            switch (T[t].addr) {
            case 0x004470F0u: hb = ee_call1(func_004470f0, dbl_bits(x)); break;
            case 0x00447B88u: hb = ee_call1(func_00447b88, dbl_bits(x)); break;
            case 0x00447D00u: { const unsigned csr = OVD_FPU_GET(); OVD_FPU_SET(OVD_FPU_EE(csr)); hb = func_00447d00(dbl_bits(x), dbl_bits(y)); OVD_FPU_SET(csr); break; }
            default: hb = dbl_bits(sqrt(x)); break;
            }
            ref = dbl_of(ib);
            got = dbl_of(hb);
            if (ib == hb) { exact++; one++; fl++; continue; }
            { double d = fabs(ref - got), u = fabs(ref) * 2.220446049250313e-16;
              if (u > 0 && d / u > worst) worst = d / u;
              if (d <= u) one++; }
            if (trunc_f(ref) == trunc_f(got)) fl++;
        }
        printf("%-24s %7u cases: %6.2f%% bit-identical, %6.2f%% within 1 ulp, %6.2f%% identical as float, truncated as retail converts (worst %.2f ulp)%s\n", T[t].name, n - bad_run,
               100.0 * exact / (n - bad_run), 100.0 * one / (n - bad_run), 100.0 * fl / (n - bad_run), worst, bad_run ? " [interpreter stopped on some]" : "");
        if (fl != n - bad_run) bad = 1;
    }
    /* gcc's soft-float double helpers (fp-bit.c at 0x0044e0a8..) on the interpreter against what the port does with the same operation: the host's, in the EE float mode
     * (MXCSR on x86, FPCR on AArch64, round toward zero) for multiply, divide, add, subtract and the conversion to float, the port's p4pc_ee_dsqrt for sqrt.
     * The retail multiply, divide and conversion are round-toward-zero as well: nothing in the units that use doubles needs rewriting. */
    {
        static const struct { uint32_t addr; const char *name; } H[] = {
            { 0x0044E0A8u, "dpadd (addsd)" }, { 0x0044E100u, "dpsub (subsd)" }, { 0x0044E168u, "dpmul (mulsd)" }, { 0x0044E3B8u, "dpdiv (divsd)" },
            { 0x0044E7D8u, "dptofp (cvtsd2ss)" }, { 0x00448A00u, "__ieee754_sqrt (p4pc_ee_dsqrt)" },
        };
        static const double probe[4] = { 7.0, -1.0, 12.0, 0.75 };

        for (size_t t = 0; t < sizeof H / sizeof H[0]; t++) {
            unsigned same = 0, ok = 0;
            int wrong = 0;

            for (unsigned i = 0; i < n; i++) {
                double x, y, rz = 0;
                uint64_t ib;
                int match;
                unsigned csr = OVD_FPU_GET();

                switch (i % 4) {                                  /* wide exponents, widened floats, small integers and 0..1 */
                case 0: x = dbl_rand(&rng, -1.0, 1.0) * ldexp(1.0, (int)dbl_rand(&rng, -40.0, 40.0)); y = dbl_rand(&rng, -1.0, 1.0) * ldexp(1.0, (int)dbl_rand(&rng, -40.0, 40.0)); break;
                case 1: x = (double)(float)dbl_rand(&rng, -1e6, 1e6); y = (double)(float)dbl_rand(&rng, -1e6, 1e6); break;
                case 2: x = (double)(int)dbl_rand(&rng, -1000.0, 1000.0); y = (double)(int)dbl_rand(&rng, -1000.0, 1000.0); break;
                default: x = dbl_rand(&rng, 0.0, 1.0); y = dbl_rand(&rng, 0.0, 1.0); break;
                }
                if (t == 5 && x < 0) x = -x;
                memset(c, 0, sizeof *c);
                c->r[29] = STACK_TOP;
                c->lo_pc = 0x00100000u;
                c->hi_pc = 0x00800000u;
                c->max_steps = 2000000;
                if (i == 0 && t < 4) {                            /* which helper this is: 3 and 4 */
                    c->r[4] = dbl_bits(3.0);
                    c->r[5] = dbl_bits(4.0);
                    if (r5900_call(c, H[t].addr) != R5900_OK || dbl_of(c->r[2]) != probe[t]) { printf("%-30s not the helper expected at %08x\n", H[t].name, H[t].addr); wrong = 1; break; }
                    memset(c, 0, sizeof *c);
                    c->r[29] = STACK_TOP; c->lo_pc = 0x00100000u; c->hi_pc = 0x00800000u; c->max_steps = 2000000;
                }
                c->r[4] = dbl_bits(x);
                c->r[5] = dbl_bits(y);
                if (r5900_call(c, H[t].addr) != R5900_OK) continue;
                ib = c->r[2];
                OVD_FPU_SET(OVD_FPU_EE(csr));
                rz = x;
                switch (t) {
#if defined(__aarch64__)
                case 0: __asm__ volatile("fadd %d0, %d1, %d2" : "=w"(rz) : "w"(x), "w"(y)); match = dbl_bits(rz) == ib; break;
                case 1: __asm__ volatile("fsub %d0, %d1, %d2" : "=w"(rz) : "w"(x), "w"(y)); match = dbl_bits(rz) == ib; break;
                case 2: __asm__ volatile("fmul %d0, %d1, %d2" : "=w"(rz) : "w"(x), "w"(y)); match = dbl_bits(rz) == ib; break;
                case 3: __asm__ volatile("fdiv %d0, %d1, %d2" : "=w"(rz) : "w"(x), "w"(y)); match = dbl_bits(rz) == ib; break;
                case 4: { float fv; uint32_t b32; __asm__ volatile("fcvt %s0, %d1" : "=w"(fv) : "w"(x)); memcpy(&b32, &fv, 4); match = b32 == (uint32_t)ib || b32 == c->f[0]; break; }
#else
                case 0: __asm__ volatile("addsd %1, %0" : "+x"(rz) : "x"(y)); match = dbl_bits(rz) == ib; break;
                case 1: __asm__ volatile("subsd %1, %0" : "+x"(rz) : "x"(y)); match = dbl_bits(rz) == ib; break;
                case 2: __asm__ volatile("mulsd %1, %0" : "+x"(rz) : "x"(y)); match = dbl_bits(rz) == ib; break;
                case 3: __asm__ volatile("divsd %1, %0" : "+x"(rz) : "x"(y)); match = dbl_bits(rz) == ib; break;
                case 4: { float fv; uint32_t b32; __asm__ volatile("cvtsd2ss %1, %0" : "=x"(fv) : "x"(x)); memcpy(&b32, &fv, 4); match = b32 == (uint32_t)ib || b32 == c->f[0]; break; }   /* a float comes back in $v0 or $f0 */
#endif
                default: match = dbl_bits(p4pc_ee_dsqrt(x)) == ib; break;
                }
                OVD_FPU_SET(csr);
                ok++;
                same += match;
            }
            if (!wrong) printf("%-30s %7u cases: %u differ from the retail helper\n", H[t].name, ok, ok - same);
            if (!wrong && t != 0 && t != 1 && ok != same) bad = 1;     /* add and subtract differ in 0.5%: reported, not a failure */
        }
    }
    /* ADX_GetCoefficient (0x004c54e8, src/cri/cri_glue.c): retail's float code, with its cosf and sqrtf run as retail's code too */
    {
        unsigned diff = 0, ok = 0;

        for (unsigned i = 0; i < n / 20; i++) {
            const int32_t sf = (int32_t[]){ 44100, 48000, 32000, 22050, 24000, 11025, 16000, 8000 }[i & 7];
            const int32_t cutoff = 20 + (int32_t)(dbl_rand(&rng, 0.0, 1.0) * (double)(sf / 2 - 20));
            int16_t nc[2] = { 0, 0 };
            int16_t *buf = (int16_t *)GH(ARENA + 0x20000u);

            memset(c, 0, sizeof *c);
            c->r[29] = STACK_TOP;
            c->r[4] = (uint64_t)(int64_t)cutoff;
            c->r[5] = (uint64_t)(int64_t)sf;
            c->r[6] = GA(buf);
            c->r[7] = GA(buf + 2);
            c->lo_pc = 0x00100000u;
            c->hi_pc = 0x00800000u;
            c->max_steps = 2000000;
            if (r5900_call(c, 0x004C54E8u) != R5900_OK) continue;
            { const unsigned csr = OVD_FPU_GET();

              OVD_FPU_SET(OVD_FPU_EE(csr));                   /* the port's natives run in the EE float mode */
              ADX_GetCoefficient(cutoff, sf, &nc[0], &nc[1]);
              OVD_FPU_SET(csr); }
            ok++;
            if (buf[0] != nc[0] || buf[2] != nc[1]) { if (diff++ < 4) fprintf(stderr, "ADX_GetCoefficient(%d, %d): retail %d %d, port %d %d\n", cutoff, sf, buf[0], buf[2], nc[0], nc[1]); }
        }
        printf("%-24s %7u cases: %u differ (retail's cosf and sqrtf against the port's)\n", "ADX_GetCoefficient", ok, diff);
        if (diff) bad = 1;
    }
    free(c);
    return bad;
}

/* Same mixed classes as sdkSpriteDrawSized, but consumes every value without
 * renderer state. This catches truncation of late GP/FP/stack parameters. */
static uint32_t abi_wide_sprite_probe(void *pointer, float x, float y, float z,
    uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha,
    uint16_t scale_x, uint16_t scale_y, int16_t pivot_x, float angle, int16_t pivot_y) {
    uint32_t words[] = {(uint32_t)(uintptr_t)pointer, b_of(x), b_of(y), b_of(z),
        red, green, blue, alpha, scale_x, scale_y, (uint32_t)(int32_t)pivot_x,
        b_of(angle), (uint32_t)(int32_t)pivot_y};
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < sizeof words / sizeof words[0]; ++i) hash = (hash ^ words[i]) * 16777619u;
    return hash;
}

typedef struct { uint32_t low, high; } AbiRecord;
static uint32_t abi_record_probe(AbiRecord record, float depth, uint8_t alpha,
                                 uint32_t style, uint32_t resource) {
    return record.low ^ record.high * 3u ^ b_of(depth) ^ (uint32_t)alpha * 7u ^
        style * 11u ^ resource * 13u;
}

typedef struct { float value; } AbiFloatRecord;
typedef struct { float low, high; } AbiFloatPair;
typedef struct __attribute__((packed)) { uint8_t tag; uint16_t value; } AbiPackedSmall;
typedef struct __attribute__((packed)) { uint8_t tag; uint32_t value; } AbiPackedWide;
static uint32_t abi_float_record_probe(AbiFloatRecord record, float depth, uint32_t tail) {
    return b_of(record.value) ^ b_of(depth) * 3u ^ tail;
}
static uint32_t abi_float_pair_probe(AbiFloatPair record, float depth, uint32_t tail) {
    return b_of(record.low) ^ b_of(record.high) * 7u ^ b_of(depth) * 3u ^ tail;
}
static uint32_t abi_packed_small_probe(AbiPackedSmall record, float depth, uint32_t tail) {
    return record.tag ^ (uint32_t)record.value * 7u ^ b_of(depth) * 3u ^ tail;
}
static uint32_t abi_packed_wide_probe(AbiPackedWide record, float depth, uint32_t tail) {
    return record.tag ^ record.value * 7u ^ b_of(depth) * 3u ^ tail;
}

static int ee_abi_selftest(void) {
    static const uint64_t values[] = {
        0, UINT64_C(0x0000000180000001), UINT64_C(0x4123000000000000),
        UINT64_C(0x80000000fedcba98), UINT64_MAX
    };
    const uint64_t scratch = UINT64_C(0x76543210abcdef01);
    R5900 c = {0};
    Sig s = {0};
    NRet r = {0};
    unsigned failed = 0, checks = 0;
#define ABI_CHECK(cond) do { checks++; if (!(cond)) { \
    fprintf(stderr, "ovdiff EE ABI: %s failed\n", #cond); failed++; \
} } while (0)
    s.ret = 'q';
    for (unsigned i = 0; i < sizeof values / sizeof values[0]; i++) {
        r.eax = (uint32_t)values[i];
        r.edx = (uint32_t)(values[i] >> 32);
        c.r[3] = scratch;
        set_results(&c, &s, &r);
        ABI_CHECK(c.r[2] == values[i]);
        ABI_CHECK(c.r[3] == scratch);
        ABI_CHECK(ee_result_hi(&c, s.ret) == r.edx);
    }
    s.ret = 'i';
    r.eax = UINT32_C(0x80000001);
    set_results(&c, &s, &r);
    ABI_CHECK(c.r[2] == UINT64_C(0xffffffff80000001));
    ABI_CHECK(c.r[3] == scratch);
    ABI_CHECK(ee_result_hi(&c, s.ret) == (uint32_t)scratch);
    s.ret = 'v';
    r.eax = 0;
    set_results(&c, &s, &r);
    ABI_CHECK(c.r[2] == UINT64_C(0xffffffff80000001));
    ABI_CHECK(c.r[3] == scratch);
    s.ret = 'f';
    r.st0 = 1.25f;
    r.has_st0 = 1;
    set_results(&c, &s, &r);
    ABI_CHECK(c.f[0] == UINT32_C(0x3fa00000));
    ABI_CHECK(c.r[2] == UINT64_C(0xffffffff80000001));
    ABI_CHECK(c.r[3] == scratch);
    /* Exercise the real callee registry, not a mirrored signature in the test.
     * RwFrameRotate's callee interleaves an FP argument before the last GP one. */
    char sig_path[600];
    snprintf(sig_path, sizeof sig_path, "%s/sigs.txt",
             getenv("P4_MIPSDIFF_DIR") ? getenv("P4_MIPSDIFF_DIR") : "tools/mipsdiff");
    load_sigs(sig_path);
    const Sig *mixed = find_sig(0x003e0870u);
    ABI_CHECK(mixed != NULL);
    if (mixed) {
        Case cs = {.slot = {0x01500000u, 0x01500100u, 0x41c80000u, 2u}, .entry_sp = 0x01fff940u};
        uint32_t slots[OVD_ARG_WORDS];
        load_interp_regs(&c, mixed, &cs);
        ABI_CHECK(c.r[29] == cs.entry_sp);
        ABI_CHECK(c.r[4] == cs.slot[0]);
        ABI_CHECK(c.r[5] == cs.slot[1]);
        ABI_CHECK(c.f[12] == cs.slot[2]);
        ABI_CHECK(c.r[6] == cs.slot[3]);
        callee_slots_raw(&c, mixed->addr, NULL, slots);
        ABI_CHECK(memcmp(slots, cs.slot, sizeof slots) == 0);
    }
    const char *elf = getenv("P4PC_ELF");
    const Sig *sized = find_sig(0x0034f320u);
    ABI_CHECK(sized != NULL && strlen(sized->args) == 13);
    if (elf && *elf && sized) {
        p4pc_load_image(elf);
        uint32_t expected[OVD_ARG_WORDS] = {0x1234u, 0x3f800000u, 0xc0000000u, 0x3f000000u,
            0x81u, 0x92u, 0xa3u, 0xb4u, 0x1234u, 0x2345u, 0xffffffefu, 0xc2b40000u, 0xffffffedu};
        uint32_t slots[OVD_ARG_WORDS];
        memset(&c, 0, sizeof c);
        c.r[29] = 0x01fff940u;
        c.r[4] = expected[0];
        c.r[5] = expected[4]; c.r[6] = expected[5]; c.r[7] = expected[6]; c.r[8] = expected[7];
        c.r[9] = expected[8]; c.r[10] = expected[9]; c.r[11] = expected[10];
        c.f[12] = expected[1]; c.f[13] = expected[2]; c.f[14] = expected[3]; c.f[15] = expected[11];
        *(uint32_t *)GH((uint32_t)c.r[29]) = expected[12]; /* ninth GP argument: caller outgoing stack */
        callee_slots_raw(&c, sized->addr, NULL, slots);
        ABI_CHECK(!memcmp(slots, expected, sizeof slots));
        Sig probe = *sized;
        probe.ret = 'i';
        ovd_call8((void *)abi_wide_sprite_probe, &probe, NULL, slots, &r);
        ABI_CHECK(r.eax == abi_wide_sprite_probe((void *)(uintptr_t)0x1234u,
            1.0f, -2.0f, 0.5f, 0x81, 0x92, 0xa3, 0xb4, 0x1234, 0x2345, -17, -90.0f, -19));
        const Sig *scaled = find_sig(0x0034f4a0u);
        ABI_CHECK(scaled != NULL && strlen(scaled->args) == 14);
        if (scaled) {
            uint32_t expected_scaled[OVD_ARG_WORDS] = {0x23u, 0x34u, 0x3f800000u, 0x40000000u,
                0x40400000u, 0x81u, 0x92u, 0xa3u, 0xb4u, 0x1234u, 0x2345u, 0xc2b40000u,
                0xffffffefu, 0xffffffedu};
            memset(&c, 0, sizeof c);
            c.r[29] = 0x01fff940u;
            c.r[4] = expected_scaled[0]; c.r[5] = expected_scaled[1];
            c.r[6] = expected_scaled[5]; c.r[7] = expected_scaled[6];
            c.r[8] = expected_scaled[7]; c.r[9] = expected_scaled[8];
            c.r[10] = expected_scaled[9]; c.r[11] = expected_scaled[10];
            c.f[12] = expected_scaled[2]; c.f[13] = expected_scaled[3];
            c.f[14] = expected_scaled[4]; c.f[15] = expected_scaled[11];
            uint32_t *outgoing = GH((uint32_t)c.r[29]);
            outgoing[0] = expected_scaled[12]; outgoing[1] = 0xbad0feedu;
            outgoing[2] = expected_scaled[13];
            callee_slots_raw(&c, scaled->addr, NULL, slots);
            ABI_CHECK(!memcmp(slots, expected_scaled, sizeof slots));
        }
        const Sig *footer = find_sig(0x0034f9d0u);
        ABI_CHECK(footer != NULL && !strcmp(footer->args, "qfBii"));
        if (footer) {
            AbiRecord record = {0x12345678u, 0x89abcdefu};
            uint32_t expected_record[OVD_ARG_WORDS] = {
                record.low, record.high, 0x3fc00000u, 0xb4u, 0x91u, 0x1234abcdu};
            memset(&c, 0, sizeof c);
            c.r[4] = record.low | (uint64_t)record.high << 32;
            c.f[12] = expected_record[2];
            c.r[5] = expected_record[3]; c.r[6] = expected_record[4]; c.r[7] = expected_record[5];
            callee_slots_raw(&c, footer->addr, NULL, slots);
            ABI_CHECK(!memcmp(slots, expected_record, sizeof slots));
            Sig probe = *footer;
            probe.ret = 'i';
            ovd_call8((void *)abi_record_probe, &probe, NULL, slots, &r);
            ABI_CHECK(r.eax == abi_record_probe(record, 1.5f, 0xb4, 0x91, 0x1234abcdu));
        }
    } else if (!elf || !*elf) {
        fprintf(stderr, "ovdiff EE ABI: wide stack/native probe requires P4PC_ELF\n");
#ifdef P4_GUEST64
        /* The 64-bit natives run on guest stacks, which live in the guest block the ELF maps: report the checks so far and skip the rest. */
        free(g_sigs);
        g_sigs = NULL;
        g_nsigs = 0;
        printf("ovdiff EE ABI: %u checks, %u failed; native probes skipped without P4PC_ELF\n", checks, failed);
        return failed ? 1 : 77;
#endif
    }
    {
        uint32_t slots[OVD_ARG_WORDS] = {0};
        Sig probe = {0, "small_record_probe", 'i', "gfi", 0};
        AbiFloatRecord single = {1.25f};
        slots[0] = b_of(single.value); slots[1] = b_of(2.5f); slots[2] = 0x91234567u;
        ovd_call8((void *)abi_float_record_probe, &probe, NULL, slots, &r);
        ABI_CHECK(r.eax == abi_float_record_probe(single, 2.5f, 0x91234567u));
        AbiFloatPair pair = {1.25f, -3.5f};
        strcpy(probe.args, "efi");
        slots[0] = b_of(pair.low); slots[1] = b_of(pair.high);
        slots[2] = b_of(2.5f); slots[3] = 0x91234567u;
        ovd_call8((void *)abi_float_pair_probe, &probe, NULL, slots, &r);
        ABI_CHECK(r.eax == abi_float_pair_probe(pair, 2.5f, 0x91234567u));
        AbiPackedSmall small = {0x91, 0xabcd};
        strcpy(probe.args, "nfi");
        memset(slots, 0, sizeof slots);
        memcpy(slots, &small, sizeof small);
        slots[1] = b_of(2.5f); slots[2] = 0x91234567u;
        ovd_call8((void *)abi_packed_small_probe, &probe, NULL, slots, &r);
        ABI_CHECK(r.eax == abi_packed_small_probe(small, 2.5f, 0x91234567u));
        AbiPackedWide wide = {0x91, 0xabcdef12u};
        strcpy(probe.args, "mfi");
        memset(slots, 0, sizeof slots);
        memcpy(slots, &wide, sizeof wide);
        slots[2] = b_of(2.5f); slots[3] = 0x91234567u;
        ovd_call8((void *)abi_packed_wide_probe, &probe, NULL, slots, &r);
        ABI_CHECK(r.eax == abi_packed_wide_probe(wide, 2.5f, 0x91234567u));
    }
    free(g_sigs);
    g_sigs = NULL;
    g_nsigs = 0;
#undef ABI_CHECK
    printf("ovdiff EE ABI: %u checks, %u failed\n", checks, failed);
    return failed != 0;
}

/* Lane maps are independent scalar expectations transcribed from the EE
 * instruction definitions, not calls back into the interpreter implementation.
 * See ps2homebrew/ps2-unpacker MMI.c and ps2autotests ee_simd/funnel.expected. */
typedef struct {
    const char *name;
    uint8_t fn, sa, bytes, map[16];
} LaneMap;

static const LaneMap lane_maps[] = {
    {"PEXTLW", 8, 18, 4, {4,0,5,1}}, {"PPACW", 8, 19, 4, {4,6,0,2}},
    {"PEXTLH", 8, 22, 2, {8,0,9,1,10,2,11,3}},
    {"PPACH", 8, 23, 2, {8,10,12,14,0,2,4,6}},
    {"PEXTLB", 8, 26, 1, {16,0,17,1,18,2,19,3,20,4,21,5,22,6,23,7}},
    {"PPACB", 8, 27, 1, {16,18,20,22,24,26,28,30,0,2,4,6,8,10,12,14}},
    {"PEXTUW", 40, 18, 4, {6,2,7,3}},
    {"PEXTUH", 40, 22, 2, {12,4,13,5,14,6,15,7}},
    {"PEXTUB", 40, 26, 1, {24,8,25,9,26,10,27,11,28,12,29,13,30,14,31,15}},
    {"PINTH", 9, 10, 2, {8,4,9,5,10,6,11,7}},
    {"PCPYLD", 9, 14, 8, {2,0}},
    {"PEXEH", 9, 26, 2, {10,9,8,11,14,13,12,15}},
    {"PREVH", 9, 27, 2, {11,10,9,8,15,14,13,12}},
    {"PEXEW", 9, 30, 4, {6,5,4,7}},
    {"PROT3W", 9, 31, 4, {5,6,4,7}},
    {"PINTEH", 41, 10, 2, {8,0,10,2,12,4,14,6}},
    {"PCPYUD", 41, 14, 8, {1,3}},
    {"PEXCH", 41, 26, 2, {8,10,9,11,12,14,13,15}},
    {"PCPYH", 41, 27, 2, {8,8,8,8,12,12,12,12}},
    {"PEXCW", 41, 30, 4, {4,6,5,7}}
};

static uint32_t lane_word(unsigned fn, unsigned sa, unsigned rs, unsigned rt, unsigned rd) {
    return 28u << 26 | rs << 21 | rt << 16 | rd << 11 | sa << 6 | fn;
}

static void lane_seed(R5900 *c, unsigned pattern) {
    memset(c, 0, sizeof *c);
    for (unsigned r = 1; r < 32; r++) {
        c->r[r] = UINT64_C(0x0706050403020100) ^ (UINT64_C(0x1111111111111111) * r);
        c->rh[r] = UINT64_C(0x8f8e8d8c8b8a8988) ^ (UINT64_C(0x0303030303030303) * r);
        if (pattern & 1) { c->r[r] = ~c->r[r]; c->rh[r] = ~c->rh[r]; }
        if (pattern & 2) { c->r[r] ^= UINT64_C(0x8000ffff00017fff); c->rh[r] ^= UINT64_C(0x12348000fedc7fff); }
    }
    c->r[31] = R5900_SENTINEL;
    c->rh[31] = 0;
    c->r[28] = 0x007690f0;
    c->r[29] = 0x01fff000;
    c->hi = UINT64_C(0x1122334455667788);
    c->lo = UINT64_C(0x99aabbccddeeff00);
    c->hi1 = UINT64_C(0x0123456789abcdef);
    c->lo1 = UINT64_C(0xfedcba9876543210);
}

static void lane_bytes(const R5900 *c, unsigned reg, unsigned char out[16]) {
    memcpy(out, &c->r[reg], 8);
    memcpy(out + 8, &c->rh[reg], 8);
}

static int lane_program(R5900 *c, const uint32_t *words, unsigned n) {
    const uint32_t entry = 0x01500000;
    memcpy(P4_G2H(entry), words, n * sizeof *words);
    c->lo_pc = entry;
    c->hi_pc = entry + n * 4;
    c->max_steps = 1000;
    return r5900_call(c, entry);
}

static int lane_check(const char *name, R5900 *c, uint32_t word, unsigned rd,
                      const unsigned char expected[16], unsigned *checks) {
    uint64_t before[32][2];
    uint32_t program[] = {word, 31u << 21 | 8u, 0};
    for (unsigned r = 0; r < 32; r++) { before[r][0] = c->r[r]; before[r][1] = c->rh[r]; }
    if (rd) {
        memcpy(&before[rd][0], expected, 8);
        memcpy(&before[rd][1], expected + 8, 8);
    }
    int rc = lane_program(c, program, 3);
    (*checks)++;
    if (rc != R5900_OK) {
        fprintf(stderr, "EE lanes %s: rc=%d word=%08x pc=%08x\n", name, rc, word, c->pc_now);
        return 1;
    }
    for (unsigned r = 0; r < 32; r++) {
        if (c->r[r] != before[r][0] || c->rh[r] != before[r][1]) {
            fprintf(stderr, "EE lanes %s word=%08x r%u: %016llx:%016llx expected %016llx:%016llx\n",
                    name, word, r, (unsigned long long)c->rh[r], (unsigned long long)c->r[r],
                    (unsigned long long)before[r][1], (unsigned long long)before[r][0]);
            return 1;
        }
    }
    return 0;
}

static int ee_lane_selftest(void) {
    static const unsigned regs[][3] = {
        {4,5,6}, {4,5,4}, {4,5,5}, {4,4,6}, {0,5,6},
        {4,0,6}, {0,0,6}, {4,5,0}, {4,4,4}
    };
    static const unsigned shift_fn[] = {0x34,0x36,0x37,0x3c,0x3e,0x3f};
    static const char *const shift_name[] = {"PSLLH","PSRLH","PSRAH","PSLLW","PSRLW","PSRAW"};
    static const uint64_t products[] = {
        0, 1, 60, 48000, UINT64_C(0xffffffff), UINT64_C(0x100000000),
        UINT64_C(0x100000001), UINT64_C(0x7fffffffffffffff),
        UINT64_C(0x8000000000000000), UINT64_MAX
    };
    const char *elf = getenv("P4PC_ELF");
    R5900 c;
    unsigned checks = 0;
    if (!elf || !*elf) {
        fprintf(stderr, "EE lanes: P4PC_ELF is required for the real __muldi3 witness\n");
        return 77;
    }
    p4pc_load_image(elf);
    for (unsigned p = 0; p < 4; p++) {
        for (unsigned a = 0; a < sizeof regs / sizeof regs[0]; a++) {
            unsigned rs = regs[a][0], rt = regs[a][1], rd = regs[a][2];
            for (unsigned op = 0; op < sizeof lane_maps / sizeof lane_maps[0]; op++) {
                const LaneMap *m = &lane_maps[op];
                unsigned char inputs[32], expected[16];
                lane_seed(&c, p);
                lane_bytes(&c, rs, inputs);
                lane_bytes(&c, rt, inputs + 16);
                for (unsigned n = 0; n < 16 / m->bytes; n++)
                    memcpy(expected + n * m->bytes, inputs + m->map[n] * m->bytes, m->bytes);
                /* The one-source shuffles reserve rs as zero. */
                unsigned encoded_rs = (m->fn == 9 || m->fn == 41) && m->sa >= 26 ? 0 : rs;
                if (lane_check(m->name, &c, lane_word(m->fn,m->sa,encoded_rs,rt,rd), rd, expected, &checks)) return 1;
            }
            for (unsigned sa = 0; sa < 16; sa++) {
                unsigned char inputs[32], expected[16];
                lane_seed(&c, p);
                lane_bytes(&c, rt, inputs);
                lane_bytes(&c, rs, inputs + 16);
                memcpy(expected, inputs + sa, 16);
                c.sa = sa;
                if (lane_check("QFSRV", &c, lane_word(40,27,rs,rt,rd), rd, expected, &checks)) return 1;
            }
            for (unsigned op = 0; op < 6; op++) {
                for (unsigned sa = 0; sa < 32; sa++) {
                    unsigned char source[16], expected[16];
                    unsigned bytes = op < 3 ? 2 : 4, bits = bytes * 8, shift = sa & (bits - 1);
                    uint32_t mask = bytes == 2 ? 0xffffu : UINT32_MAX;
                    lane_seed(&c, p);
                    lane_bytes(&c, rt, source);
                    for (unsigned n = 0; n < 16 / bytes; n++) {
                        uint32_t v = 0, result;
                        memcpy(&v, source + n * bytes, bytes);
                        if (op % 3 == 0) result = (uint32_t)((uint64_t)v << shift) & mask;
                        else {
                            result = v >> shift;
                            if (op % 3 == 2 && shift && (v & (1u << (bits - 1))))
                                result |= mask ^ (mask >> shift);
                        }
                        memcpy(expected + n * bytes, &result, bytes);
                    }
                    if (lane_check(shift_name[op], &c, lane_word(shift_fn[op],sa,0,rt,rd), rd, expected, &checks)) return 1;
                }
            }
        }
    }
    for (unsigned rd = 0; rd <= 6; rd += 3) {
        for (unsigned op = 0; op < 4; op++) {
            uint64_t expected[2];
            lane_seed(&c, 0);
            if (op < 2) {
                expected[0] = op ? c.lo : c.hi;
                expected[1] = op ? c.lo1 : c.hi1;
            } else {
                unsigned shift = op == 3 ? 32 : 0;
                expected[0] = (uint32_t)(c.lo >> shift) | ((uint64_t)(uint32_t)(c.hi >> shift) << 32);
                expected[1] = (uint32_t)(c.lo1 >> shift) | ((uint64_t)(uint32_t)(c.hi1 >> shift) << 32);
            }
            uint32_t word = op < 2 ? lane_word(9,8 + op,0,0,rd) : lane_word(0x30,op - 2,0,0,rd);
            if (lane_check(op == 0 ? "PMFHI" : op == 1 ? "PMFLO" : op == 2 ? "PMFHL.LW" : "PMFHL.UW",
                           &c, word, rd, (const unsigned char *)expected, &checks)) return 1;
        }
    }
    /* Producer-to-transfer linkage: MULTU1 is the upper bank read by PMFHI/LO. */
    {
        uint32_t program[] = {
            lane_word(0x19,0,4,5,7), lane_word(9,8,0,0,6),
            lane_word(9,9,0,0,8), 31u << 21 | 8u, 0
        };
        lane_seed(&c, 0);
        c.r[4] = UINT32_MAX; c.r[5] = 2;
        uint64_t low_hi = c.hi, low_lo = c.lo;
        int rc = lane_program(&c, program, 5);
        checks++;
        if (rc || c.r[6] != low_hi || c.rh[6] != 1 || c.r[8] != low_lo ||
            c.rh[8] != UINT64_C(0xfffffffffffffffe)) {
            fprintf(stderr, "EE lanes MULTU1 -> PMFHI/PMFLO linkage failed rc=%d\n", rc);
            return 1;
        }
    }
    /* Hardware-backed MFSA encoding, plus opaque save/restore and QFSRV.
     * The Sony manual permits saving SA, not interpreting arbitrary encodings;
     * the concrete values here are also checked by ps2autotests funnel.expected. */
    for (unsigned half = 0; half < 2; half++) {
        for (unsigned value = 0; value < 32; value++) {
            for (unsigned imm = 0; imm < 32; imm++) {
                unsigned sa = half ? ((value & 7) ^ (imm & 7)) * 2 : (value & 15) ^ (imm & 15);
                unsigned char inputs[32], expected[16], actual[16];
                uint32_t program[] = {
                    1u << 26 | 4u << 21 | (half ? 0x19u : 0x18u) << 16 | imm, 0, 0, 0,
                    8u << 11 | 0x28u, 0, 0, 0, 8u << 21 | 0x29u, 0, 0, 0,
                    lane_word(40,27,4,5,6), 31u << 21 | 8u, 0
                };
                lane_seed(&c, 0);
                c.r[4] = value;
                uint64_t upper = c.rh[8];
                lane_bytes(&c, 5, inputs);
                lane_bytes(&c, 4, inputs + 16);
                memcpy(expected, inputs + sa, 16);
                int rc = lane_program(&c, program, sizeof program / sizeof program[0]);
                lane_bytes(&c, 6, actual);
                checks++;
                if (rc || c.sa != sa || c.r[8] != sa || c.rh[8] != upper || memcmp(actual, expected, 16)) {
                    fprintf(stderr, "EE lanes %s value=%u imm=%u SA save/restore/QFSRV failed rc=%d\n",
                            half ? "MTSAH" : "MTSAB", value, imm, rc);
                    return 1;
                }
            }
        }
    }
    {
        static const uint32_t values[] = {0,1,8,15,16,17,0xffff,UINT32_MAX};
        for (unsigned i = 0; i < sizeof values / sizeof values[0]; i++) {
            uint32_t program[] = {4u << 21 | 0x29u, 0, 0, 0, 8u << 11 | 0x28u, 31u << 21 | 8u, 0};
            lane_seed(&c, 0);
            c.r[4] = values[i];
            uint64_t upper = c.rh[8];
            int rc = lane_program(&c, program, sizeof program / sizeof program[0]);
            checks++;
            if (rc || c.sa != (values[i] & 15) || c.r[8] != (values[i] & 15) || c.rh[8] != upper) {
                fprintf(stderr, "EE lanes hardware MTSA/MFSA(%08x) failed rc=%d\n", values[i], rc);
                return 1;
            }
        }
    }
    {
        static const uint32_t values[] = {0,1,0x80000000u,UINT32_MAX,0x7fffffffu,0x00010000u};
        for (unsigned i = 0; i < sizeof values / sizeof values[0]; i++) {
            for (unsigned j = 0; j < sizeof values / sizeof values[0]; j++) {
                uint32_t expected_words[2];
                uint64_t expected[2];
                lane_seed(&c, 0);
                c.r[4] = values[i] | (uint64_t)values[j] << 32;
                for (unsigned w = 0; w < 2; w++) {
                    uint32_t v = w ? values[j] : values[i];
                    if (v >> 31) v = ~v;
                    unsigned leading = 0;
                    for (unsigned bit = 32; bit && !(v & (1u << (bit - 1))); bit--) leading++;
                    expected_words[w] = leading - 1;
                }
                expected[0] = expected_words[0] | (uint64_t)expected_words[1] << 32;
                expected[1] = c.rh[6];
                if (lane_check("PLZCW", &c, lane_word(4,0,4,0,6), 6,
                               (const unsigned char *)expected, &checks)) return 1;
            }
        }
    }
    /* Absent instructions must stop cleanly, never supply an approximate
     * value that could turn an inconclusive replay into a false exact one. */
    {
        static const LaneMap unsupported[] = {
            {"PEXT5",8,30,0,{0}}, {"PPAC5",8,31,0,{0}},
            {"PSLLVW",9,2,0,{0}}, {"PSRLVW",9,3,0,{0}}, {"PSRAVW",41,3,0,{0}},
            {"PMTHI",41,8,0,{0}}, {"PMTLO",41,9,0,{0}}, {"PMTHL.LW",0x31,0,0,{0}},
            {"PMFHL.SLW",0x30,2,0,{0}}, {"PMFHL.LH",0x30,3,0,{0}}, {"PMFHL.SH",0x30,4,0,{0}}
        };
        for (unsigned i = 0; i < sizeof unsupported / sizeof unsupported[0]; i++) {
            const LaneMap *op = &unsupported[i];
            unsigned rs = 4, rt = 5, rd = 6;
            if (op->fn == 0x30) { rs = 0; rt = 0; }
            else if (op->fn == 0x31 || (op->fn == 41 && (op->sa == 8 || op->sa == 9))) { rt = 0; rd = 0; }
            else if (op->fn == 8) rs = 0;
            uint32_t program[] = {lane_word(op->fn,op->sa,rs,rt,rd), 31u << 21 | 8u, 0};
            lane_seed(&c, 0);
            int rc = lane_program(&c, program, 3);
            checks++;
            if (rc != R5900_UNSUPPORTED || c.bad_word != program[0]) {
                fprintf(stderr, "EE lanes %s did not stop as unsupported: rc=%d\n", op->name, rc);
                return 1;
            }
        }
    }
    for (unsigned i = 0; i < sizeof products / sizeof products[0]; i++) {
        for (unsigned j = 0; j < sizeof products / sizeof products[0]; j++) {
            lane_seed(&c, 0);
            c.r[4] = products[i]; c.r[5] = products[j];
            c.lo_pc = 0x0044b9b0; c.hi_pc = 0x0044b9fc; c.max_steps = 1000;
            uint64_t expected = products[i] * products[j];
            int rc = r5900_call(&c, 0x0044b9b0);
            checks++;
            if (rc || c.r[2] != expected) {
                fprintf(stderr, "EE lanes real __muldi3(%016llx,%016llx): rc=%d got=%016llx expected=%016llx\n",
                        (unsigned long long)products[i], (unsigned long long)products[j], rc,
                        (unsigned long long)c.r[2], (unsigned long long)expected);
                return 1;
            }
        }
    }
    printf("ovdiff EE lanes: %u nonzero/alias/zero/accumulator/SA/real-multiply checks exact\n", checks);
    return 0;
}

int main(int argc, char **argv) {
    if (getenv("P4PC_MOD_WITNESS") != NULL) {
        fputs("ovdiff: fidelity execution refuses P4PC_MOD_WITNESS\n", stderr);
        return 1;
    }
    setenv("P4PC_MODS", "0", 1);
    setenv("P4PC_PACKS", "0", 1);
    if (argc == 2 && !strcmp(argv[1], "--selftest-abi")) return ee_abi_selftest();
    if (argc == 2 && !strcmp(argv[1], "--selftest-lanes")) return ee_lane_selftest();
    if (argc == 2 && !strcmp(argv[1], "--selftest-synthetic-words")) return synthetic_word_selftest();
    /* Host addresses (heap, stack, libraries) leak into what the natives compute (a callee's pointer result, a hash, an
     * alignment), which makes the set of valid cases vary from run to run. Without ASLR a run is reproducible: re-exec once
     * with randomisation off (a container whose seccomp profile forbids it just keeps the randomised layout). */
    if (!getenv("P4_OVDIFF_EXEC")) {
        int old = personality(0xffffffff);
        if (old != -1 && !(old & ADDR_NO_RANDOMIZE) && personality((unsigned long)old | ADDR_NO_RANDOMIZE) != -1) {
            setenv("P4_OVDIFF_EXEC", "1", 1);
            execv("/proc/self/exe", argv);
        }
        if (old != -1 && !(old & ADDR_NO_RANDOMIZE))
            fprintf(stderr, "ovdiff: address randomisation could not be switched off (container seccomp profile? use --security-opt seccomp=unconfined): "
                            "a native's stray write may hit the harness's heap on some runs only\n");
    }
#ifdef P4_GUEST64
    p4_guest_check_off = 1;   /* the random words the cases write are not addresses; they round-trip as they do on i386 */
#endif
    unsigned ncases = 200;
    unsigned only_case = (unsigned)-1;
    const char *report_path = NULL, *sigs_path = NULL, *impure_path = NULL;
    int ai = 1;
    for (; ai < argc && argv[ai][0] == '-'; ai++) {
        if (!strcmp(argv[ai], "-n") && ai + 1 < argc) ncases = (unsigned)atoi(argv[++ai]);
        else if (!strcmp(argv[ai], "-c") && ai + 1 < argc) only_case = (unsigned)atoi(argv[++ai]);
        else if (!strcmp(argv[ai], "-o") && ai + 1 < argc) report_path = argv[++ai];
        else if (!strcmp(argv[ai], "-S") && ai + 1 < argc) sigs_path = argv[++ai];
        else if (!strcmp(argv[ai], "-I") && ai + 1 < argc) impure_path = argv[++ai];
        else if (!strcmp(argv[ai], "-R") && ai + 1 < argc) golden_path = argv[++ai];
        else if (!strcmp(argv[ai], "-W") && ai + 1 < argc) windows_path = argv[++ai];
        else if (!strcmp(argv[ai], "--paired")) g_replay_paired = 1;
        else if (!strcmp(argv[ai], "--paired-device")) g_paired_device = 1;
        else if (!strcmp(argv[ai], "-G")) g_use_globals = 1;
        else if (!strcmp(argv[ai], "-E")) g_replay_interp = 1; else if (!strcmp(argv[ai], "-Q")) g_dblcores = 1; else if (!strcmp(argv[ai], "-F")) g_full = 1;
        else if (!strcmp(argv[ai], "-V")) setenv("P4_REPLAY_DEBUG", "1", 1);      /* with -R -v: print each bad call's arguments and regions */
        else if (!strcmp(argv[ai], "-D") && ai + 1 < argc) g_wanted_path = argv[++ai];
        else if (!strcmp(argv[ai], "-P") && ai + 1 < argc) g_peek = argv[++ai];
        else if (!strcmp(argv[ai], "-v")) g_verbose = 1;
        else { fprintf(stderr, "ovdiff: unknown option %s\n", argv[ai]); return 2; }
    }
    if (g_paired_device && (!g_replay_paired || !g_use_globals)) {
        fprintf(stderr, "ovdiff: --paired-device requires --paired and -G\n");
        return 2;
    }
    if (g_replay_paired && (!golden_path || g_replay_interp || g_full || g_wanted_path)) {
        fprintf(stderr, "ovdiff: --paired requires -R and identical native callees; do not combine with -E, -F or -D\n");
        return 2;
    }
    if (argc - ai < 2) {
        fprintf(stderr, "usage: ovdiff [-n cases] [-c case] [-v] [-o report.tsv] [-S sigs.txt] [-I impure.txt] [-R golden.jsonl [-W windows.jsonl] [-G] [-E | --paired] [-V] [-D wanted.txt] [-P addr,..]] <SLUS_217.82> <addr|all|@file>...\n");
        fprintf(stderr, "       ovdiff --selftest-abi | --selftest-lanes (P4PC_ELF required for lanes)\n");
        return 2;
    }
    const char *elf = argv[ai++];
    {
        void *shared = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (shared != MAP_FAILED) r5900_stat = shared;
    }
    if (!strcmp(elf, "env")) {
        elf = getenv("P4PC_ELF");
        if (!elf || !*elf) { fprintf(stderr, "ovdiff: P4PC_ELF is not set, skipping\n"); return 77; }
    }
    char dir[512];
    char sp[600], ip[600];
    dir_of(argv[0], dir, sizeof dir);
    if (!sigs_path) { snprintf(sp, sizeof sp, "%s/sigs.txt", getenv("P4_MIPSDIFF_DIR") ? getenv("P4_MIPSDIFF_DIR") : "tools/mipsdiff"); sigs_path = sp; }
    if (!impure_path) { snprintf(ip, sizeof ip, "%s/impure.txt", getenv("P4_MIPSDIFF_DIR") ? getenv("P4_MIPSDIFF_DIR") : "tools/mipsdiff"); impure_path = ip; }
    load_sigs(sigs_path);
    load_impure(impure_path);
    if (golden_path) load_golden(golden_path);          /* replay mode: see replay.c */
    if (windows_path) load_windows(windows_path);
    { char xp[600]; snprintf(xp, sizeof xp, "%s/engine_slots.txt", getenv("P4_MIPSDIFF_DIR") ? getenv("P4_MIPSDIFF_DIR") : "tools/mipsdiff"); load_slots(xp); }
    { char xp[600]; snprintf(xp, sizeof xp, "%s/expected.txt", getenv("P4_MIPSDIFF_DIR") ? getenv("P4_MIPSDIFF_DIR") : "tools/mipsdiff"); load_expected(xp); }
    { char dp[600]; snprintf(dp, sizeof dp, "%s/domains.txt", getenv("P4_MIPSDIFF_DIR") ? getenv("P4_MIPSDIFF_DIR") : "tools/mipsdiff"); load_domains(dp); }

    g_sigdebug = getenv("P4_SIGDEBUG") != NULL;
    setenv("P4PC_TRAP_CONTINUE", "1", 1);
    setenv("P4PC_INPUT", "script", 1);  /* natives under verification must never sample physical input */
    p4pc_trap_configure();
    p4pc_load_image(elf);
    p4pc_apply_patches();
    p4pc_kernel_pre_init();
#ifdef P4_GUEST64
    /* As on i386, nothing is mapped below the game's image: a native that follows a NULL pointer plus a small offset (guest address 4, say)
     * faults, where the block's first pages would read as zeros and let it carry on. */
    mprotect((void *)p4_guest_base, RAM_LO, PROT_NONE);
#endif
    /* pre_init puts the process in the EE float mode; the harness runs in round-to-nearest, ovd_call8 and the host-float proof pass set the mode they need. */
    OVD_FPU_SET(OVD_FPU_NEAR(OVD_FPU_GET()));
    /* The port's libc heap lives in guest memory (its metadata is restored with every case), so malloc is deterministic. */
    if (golden_path) {
        /* a replay keeps retail's heap contents in guest RAM: the libc heap sits above it */
#ifdef P4_GUEST64
        /* the block already backs the guest RAM; the port's own storage (p4_gport_alloc) sits at 0x03000000, so the heap takes the top 4 MB */
        p4pc_heap_init(GH(0x03C00000u), 0x400000u);
#else
        void *h = mmap((void *)(uintptr_t)0x03000000u, 0x400000u, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
        if (h == MAP_FAILED) { perror("heap"); return 2; }
        p4pc_heap_init(h, 0x400000u);
#endif
    } else {
        p4pc_heap_init(GH(0x01800000u), 0x100000u);
    }
    p4pc_trap_hook = trap_hook;
    if (g_dblcores) return dblcores(ncases);
    g_saved = mmap(NULL, (size_t)NPG * 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    g_dirty = calloc(NPG, 1);
    static char altstack[65536];
    stack_t ss = { .ss_sp = altstack, .ss_size = sizeof altstack };
    sigaltstack(&ss, NULL);
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = on_signal;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGTRAP, &sa, NULL);
    struct sigaction al;
    memset(&al, 0, sizeof al);
    al.sa_handler = on_alarm;
    sigaction(SIGALRM, &al, NULL);

    FILE *rf = report_path ? fopen(report_path, "w") : stdout;
    if (!rf) { perror(report_path); return 2; }
    fprintf(rf, "# addr\tname\tstatus\tattempts\tvalid\texact\tproven\tfloat-tol\tspecial\tmismatch\tinvalid\tinconclusive\trepro\n");
    int any_bad = 0;
    unsigned nfn = 0;
    char **targets = calloc((size_t)argc + 4096, sizeof *targets);
    size_t ntargets = 0;
    for (; ai < argc; ai++) {
        if (argv[ai][0] == '@') {
            FILE *lf = fopen(argv[ai] + 1, "r");
            if (!lf) { perror(argv[ai] + 1); return 2; }
            char line[128];
            while (fgets(line, sizeof line, lf) && ntargets < (size_t)argc + 4000) {
                char *t = strtok(line, " \t\r\n");
                if (!t || t[0] == '#') continue;
                targets[ntargets++] = strdup(t);
            }
            fclose(lf);
        } else {
            targets[ntargets++] = argv[ai];
        }
    }
    for (size_t ti = 0; ti < ntargets; ti++) {
        const char *a = targets[ti];
        for (size_t i = 0; i < g_nsigs; i++) {
            const Sig *s = &g_sigs[i];
            if (strcmp(a, "all") == 0 ? s->callee_only : s->addr != (uint32_t)strtoul(a, NULL, 16)) continue;
            /* One child per function: a crash in a native (or a harness bug) costs one row, and every function
             * starts from the same pristine guest RAM whatever ran before it. */
            nfn++;
            fflush(rf);
            fflush(stderr);
            signal(SIGCHLD, SIG_DFL);      /* platform init may have ignored it, which would make waitpid fail */
            /* P4_OVDIFF_NOFORK=1: run the function in this process (it exits afterwards), for a debugger that does not follow a fork (qemu-user's gdbstub) */
            pid_t pid = getenv("P4_OVDIFF_NOFORK") ? 0 : fork();
            if (pid == 0) child_run(s, ncases, only_case, rf);
            int st = 0;
            int waited = 0;
            for (int ms = 0; ms < 120000; ms += 20) {
                pid_t r = waitpid(pid, &st, WNOHANG);
                if (r == pid) { waited = 1; break; }
                if (r < 0) { st = SIGKILL; waited = 1; break; }
                struct timespec ts = { 0, 20 * 1000 * 1000 };
                nanosleep(&ts, NULL);
            }
            if (!waited) { kill(pid, SIGKILL); waitpid(pid, &st, 0); fprintf(rf, "%08x\t%s\tharness timeout (120 s)\t0\t0\t0\t0\t0\t0\t0\t0\t0\t\n", s->addr, s->name); any_bad |= 1; continue; }
            if (WIFSIGNALED(st)) {
                fprintf(rf, "%08x\t%s\tharness crashed (signal %d)\t0\t0\t0\t0\t0\t0\t0\t0\t0\t\n", s->addr, s->name, WTERMSIG(st));
                any_bad |= 1;
            } else if (WEXITSTATUS(st) == 1 || (g_replay_paired && WEXITSTATUS(st) != 0)) {
                any_bad |= 1;
            }
            fflush(rf);
        }
    }
    fflush(rf);
    if (g_replay_interp) fprintf(stderr, "ovdiff: EE results clamped to Fmax %lu, float to int conversions saturated %lu\n", r5900_stat[0], r5900_stat[1]);
    fprintf(stderr, "ovdiff: %u functions\n", nfn);
    fflush(stderr);
    _exit(any_bad || (g_replay_paired && !nfn) ? 1 : 0);
}
