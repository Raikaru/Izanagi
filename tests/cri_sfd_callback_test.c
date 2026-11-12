/* Raw SFD audio callbacks against the actual retail R5900 instructions.
 * Only caller-owned SJ methods, the application error callback and the ADXT
 * silence-insertion service are delegates. Header/end-code parsing, timer
 * conversions, error/condition helpers, chunk splitting and copies execute
 * their real retail bodies. No retail data is stored in this test. */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/gmem.h"
#include "p4pc/image.h"
#include "p4pc/tables.h"
#include "r5900.h"
#include "guest_test.h"
#include "../src/cri/cri_mem.h"

typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
enum {
    OBJECT = 0x01004000, SEE = 0x01008000, WORK = 0x0100c000,
    OUT = 0x0100e000, JOINT = 0x0100f000, VT = 0x0100f100, ALT_VT = 0x0100f200,
    DATA = 0x01010000, DEST = 0x01030000, SPAN = 0x50000,
    ADXT = 0x01003200, DEBUG = 0x00745b0c, DEBUG_SIZE = 28,
    ERROR_GLOBAL = 0x00745a58, ERROR_SIZE = 20
};
enum { GET, PUT, COPY, ALIGN, INSERT, HEADER, PHASE, ERROR, SILENCE, ERROR_INFO, NATIVE_COUNT };
static const u32 addresses[NATIVE_COUNT] = {
    0x00512730, 0x00512760, 0x005127e8, 0x005128a0, 0x00512b90,
    0x00512bb0, 0x00512c70, 0x00517ad0, 0x00512d90, 0x00517b40
};
static void *native_fn[NATIVE_COUNT];
static const unsigned callback_slots[] = {SILENCE, HEADER, ALIGN, COPY};
static u32 callback_handles[4], sj_handles[5], error_handle;
static unsigned cases, depth, switch_vtable;
static s32 free_len, inserted_result;
static R5900 cpus[3];
typedef struct { u32 count, calls[16][6]; } Trace;
_Static_assert(sizeof(Trace) == 97 * 4, "trace contains only initialized words");
_Static_assert(sizeof(Ck) == 8, "SJ chunks retain their retail layout");
static Trace trace;
static unsigned char initial[SPAN], observed[SPAN];
static unsigned char initial_debug[DEBUG_SIZE], observed_debug[DEBUG_SIZE];
static unsigned char initial_error[ERROR_SIZE], observed_error[ERROR_SIZE];

static void record(u32 op, u32 a, u32 b, u32 c, u32 d, u32 e)
{
    unsigned n = trace.count++;
    if (n >= 16) abort();
    trace.calls[n][0] = op;
    trace.calls[n][1] = a;
    trace.calls[n][2] = b;
    trace.calls[n][3] = c;
    trace.calls[n][4] = d;
    trace.calls[n][5] = e;
}

s32 __wrap_func_004d6910(void *adxt, s32 nch, s32 nsmpl)
{
    record(4, P4_H2G(adxt), (u32)nch, (u32)nsmpl, (u32)inserted_result,
           adxt ? *(u32 *)adxt : 0);
    return inserted_result;
}

static void get_chunk(void *sj, s32 line, s32 len, Ck *chunk)
{
    record(0, P4_H2G(sj), (u32)line, (u32)len, DEST, (u32)free_len);
    chunk->data = DEST;
    chunk->len = free_len;
    if (switch_vtable != 0) P4_G32(JOINT) = ALT_VT;
}

static void put_common(void *sj, s32 line, Ck *chunk, unsigned alternate)
{
    record(1 + alternate * 8, P4_H2G(sj), (u32)line, chunk->data, (u32)chunk->len, 0);
    if (switch_vtable == 2) P4_G32(JOINT) = VT;
}
static void unget_common(void *sj, s32 line, Ck *chunk, unsigned alternate)
{
    record(2 + alternate * 8, P4_H2G(sj), (u32)line, chunk->data, (u32)chunk->len, 0);
}
static void put_chunk(void *sj, s32 line, Ck *chunk) { put_common(sj, line, chunk, 0); }
static void unget_chunk(void *sj, s32 line, Ck *chunk) { unget_common(sj, line, chunk, 0); }
static void alt_put_chunk(void *sj, s32 line, Ck *chunk) { put_common(sj, line, chunk, 1); }
static void alt_unget_chunk(void *sj, s32 line, Ck *chunk) { unget_common(sj, line, chunk, 1); }
static void error_callback(void *context, s32 code)
{
    record(3, P4_H2G(context), (u32)code, P4_G32(OBJECT + 0x48),
           P4_G32(OBJECT + 0xa00), P4_G32(ERROR_GLOBAL + 12));
}

#ifdef P4_GUEST64
static int same_register_abi(const char *actual, const char *expected)
{
    if (actual == NULL) return 0;
    /* As the existing CPU adapter witness and gptr_rewrite.letter_class:
     * void/int share the ordinary result register class. Every parameter
     * count, pointer and FP class remains exact. Void results are not read. */
    if ((actual[0] == 'v' || actual[0] == 'i') &&
        (expected[0] == 'v' || expected[0] == 'i'))
        return strcmp(actual + 1, expected + 1) == 0;
    return strcmp(actual, expected) == 0;
}
#endif

static void load_natives(void)
{
#ifdef P4_GUEST64
    static const char *const signatures[NATIVE_COUNT] = {
        "vpip", "vpip", "vppip", "vppip", "ipii", "vppip", "pppi", "ipi", "vppip", "vpi"
    };
#endif
    for (size_t i = 0; i < p4pc_native_count; i++) {
        const p4pc_native_t *entry = &p4pc_native_table[i];
        for (unsigned n = 0; n < NATIVE_COUNT; n++) {
            if (entry->addr != addresses[n]) continue;
            if (native_fn[n] != NULL) abort();
            native_fn[n] = entry->fn;
#ifdef P4_GUEST64
            if (!same_register_abi(entry->sig, signatures[n])) {
                fprintf(stderr, "SFD ABI %08x: %s, expected %s\n", entry->addr,
                        entry->sig ? entry->sig : "-", signatures[n]);
                exit(1);
            }
#endif
        }
    }
    for (unsigned n = 0; n < NATIVE_COUNT; n++) {
        if (native_fn[n] == NULL) {
            fprintf(stderr, "missing actual SFD native %08x\n", addresses[n]);
            exit(1);
        }
    }
    for (unsigned n = 0; n < 4; n++) {
        callback_handles[n] = p4_fn_h2g_sig(native_fn[callback_slots[n]], "vppip");
        if (callback_handles[n] == 0) abort();
    }
    sj_handles[0] = p4_fn_h2g_sig(get_chunk, "vpiip");
    sj_handles[1] = p4_fn_h2g_sig(put_chunk, "vpip");
    sj_handles[2] = p4_fn_h2g_sig(unget_chunk, "vpip");
    sj_handles[3] = p4_fn_h2g_sig(alt_put_chunk, "vpip");
    sj_handles[4] = p4_fn_h2g_sig(alt_unget_chunk, "vpip");
    error_handle = p4_fn_h2g_sig(error_callback, "vpi");
}

static u64 retail(u32 entry, const u32 args[4]);
static int retail_hook(R5900 *cpu, u32 target, void *unused)
{
    u32 a[4];
    u64 result = 0;
    (void)unused;
    for (unsigned i = 0; i < 4; i++) a[i] = (u32)cpu->r[4 + i];
    if (target == 0x004d6910) {
        result = (u64)(int64_t)__wrap_func_004d6910(P4_G2H(a[0]), (s32)a[1], (s32)a[2]);
    } else if (target == sj_handles[0]) {
        get_chunk(P4_G2H(a[0]), (s32)a[1], (s32)a[2], P4_G2H(a[3]));
    } else if (target == sj_handles[1]) {
        put_chunk(P4_G2H(a[0]), (s32)a[1], P4_G2H(a[2]));
    } else if (target == sj_handles[2]) {
        unget_chunk(P4_G2H(a[0]), (s32)a[1], P4_G2H(a[2]));
    } else if (target == sj_handles[3]) {
        alt_put_chunk(P4_G2H(a[0]), (s32)a[1], P4_G2H(a[2]));
    } else if (target == sj_handles[4]) {
        alt_unget_chunk(P4_G2H(a[0]), (s32)a[1], P4_G2H(a[2]));
    } else if (target == error_handle) {
        error_callback(P4_G2H(a[0]), (s32)a[1]);
    } else {
        switch (target) {
        case 0x0043f778: case 0x004d6838: case 0x004d68d0: case 0x004ef8b8:
        case 0x005010a0: case 0x00512730: case 0x00512760: case 0x00512b08:
        case 0x00512b90: case 0x00512c70: case 0x00512ea8: case 0x00513dc8:
        case 0x00517840: case 0x00517ad0: case 0x00524ed0: case 0x00525098:
        case 0x00525b58: case 0x00525bb8: case 0x00525c30:
            result = retail(target, a);
            break;
        default:
            fprintf(stderr, "unexpected SFD retail call %08x from %08x\n", target, cpu->pc_now);
            exit(2);
        }
    }
    cpu->r[2] = result;
    cpu->r[3] = 0;
    return 1;
}

static u64 retail(u32 entry, const u32 args[4])
{
    R5900 *cpu;
    int rc;
    if (depth == sizeof cpus / sizeof *cpus) abort();
    cpu = &cpus[depth];
    memset(cpu, 0, sizeof *cpu);
    cpu->r[28] = 0x007690f0;
    cpu->r[29] = 0x01fff000 - depth * 0x1000;
    for (unsigned i = 0; i < 4; i++) cpu->r[4 + i] = (u64)(int64_t)(s32)args[i];
    cpu->lo_pc = entry;
    switch (entry) {
    case 0x00512730: cpu->hi_pc = 0x00512760; break;
    case 0x00512760: cpu->hi_pc = 0x005127e8; break;
    case 0x005127e8: cpu->hi_pc = 0x005128a0; break;
    case 0x005128a0: cpu->hi_pc = 0x00512b08; break;
    case 0x00512b90: cpu->hi_pc = 0x00512bb0; break;
    case 0x00512bb0: cpu->hi_pc = 0x00512c70; break;
    case 0x00512c70: cpu->hi_pc = 0x00512d90; break;
    case 0x00512d90: cpu->hi_pc = 0x00512ea8; break;
    case 0x00512ea8: cpu->hi_pc = 0x00512ef8; break;
    case 0x00513dc8: cpu->hi_pc = 0x00513df8; break;
    case 0x00517840: cpu->hi_pc = 0x00517878; break;
    default:
        /* These are real SDK CPU leaves, not substituted return values. */
        cpu->lo_pc = 0x00100000;
        cpu->hi_pc = 0x02000000;
        break;
    }
    cpu->hook = retail_hook;
    cpu->max_steps = 1000000;
    depth++;
    rc = r5900_call(cpu, entry);
    depth--;
    if (rc != R5900_OK) {
        fprintf(stderr, "SFD retail %08x rc=%d pc=%08x fault=%s address=%08x\n",
                entry, rc, cpu->pc_now, cpu->fault_why ? cpu->fault_why : "-", cpu->fault_addr);
        exit(2);
    }
    return cpu->r[2];
}

static u32 callback_code(u32 handle)
{
    for (unsigned n = 0; n < 4; n++)
        if (handle == callback_handles[n]) return addresses[callback_slots[n]];
    return handle;
}
static void capture(void)
{
    u32 code = callback_code(P4_G32(WORK + 0x3c));
    memcpy(observed, P4_G2H(OBJECT), SPAN);
    memcpy(observed + WORK - OBJECT + 0x3c, &code, sizeof code);
    memcpy(observed_debug, P4_G2H(DEBUG), DEBUG_SIZE);
    memcpy(observed_error, P4_G2H(ERROR_GLOBAL), ERROR_SIZE);
}
static void save_initial(void)
{
    memcpy(initial, P4_G2H(OBJECT), SPAN);
    memcpy(initial_debug, P4_G2H(DEBUG), DEBUG_SIZE);
    memcpy(initial_error, P4_G2H(ERROR_GLOBAL), ERROR_SIZE);
    memset(&trace, 0, sizeof trace);
}
static void restore_initial(void)
{
    memcpy(P4_G2H(OBJECT), initial, SPAN);
    memcpy(P4_G2H(DEBUG), initial_debug, DEBUG_SIZE);
    memcpy(P4_G2H(ERROR_GLOBAL), initial_error, ERROR_SIZE);
    memset(&trace, 0, sizeof trace);
}
static void compare_state(unsigned slot, const Trace *native_trace)
{
    if (memcmp(observed, P4_G2H(OBJECT), SPAN) != 0) {
        for (u32 i = 0; i < SPAN; i++) {
            if (observed[i] != P4_G8(OBJECT + i)) {
                fprintf(stderr, "SFD case %u entry=%08x address=%08x native=%02x retail=%02x\n",
                        cases, addresses[slot], OBJECT + i, observed[i], P4_G8(OBJECT + i));
                exit(1);
            }
        }
    }
    for (u32 i = 0; i < DEBUG_SIZE; i++) {
        if (observed_debug[i] != P4_G8(DEBUG + i)) {
            fprintf(stderr, "SFD case %u entry=%08x timer=%08x native=%02x retail=%02x\n",
                    cases, addresses[slot], DEBUG + i, observed_debug[i], P4_G8(DEBUG + i));
            exit(1);
        }
    }
    for (u32 i = 0; i < ERROR_SIZE; i++) {
        if (observed_error[i] != P4_G8(ERROR_GLOBAL + i)) {
            fprintf(stderr, "SFD case %u entry=%08x error-global=%08x native=%02x retail=%02x\n",
                    cases, addresses[slot], ERROR_GLOBAL + i, observed_error[i],
                    P4_G8(ERROR_GLOBAL + i));
            exit(1);
        }
    }
    if (memcmp(native_trace, &trace, sizeof trace) != 0) {
        fprintf(stderr, "SFD case %u entry=%08x external calls native=%u retail=%u\n",
                cases, addresses[slot], native_trace->count, trace.count);
        for (unsigned n = 0; n < 16; n++) {
            for (unsigned w = 0; w < 6; w++) {
                if (native_trace->calls[n][w] != trace.calls[n][w]) {
                    fprintf(stderr, "call=%u word=%u native=%08x retail=%08x\n", n, w,
                            native_trace->calls[n][w], trace.calls[n][w]);
                    exit(1);
                }
            }
        }
        exit(1);
    }
    cases++;
}

static void prepare(unsigned pattern)
{
    memset(P4_G2H(OBJECT), 0xa5, SPAN);
    memset(P4_G2H(DEBUG), 0x59, DEBUG_SIZE);
    memset(P4_G2H(ERROR_GLOBAL), 0x59, ERROR_SIZE);
    P4_G32(ERROR_GLOBAL + 4) = 0;
    P4_G32(ERROR_GLOBAL + 8) = OUT + 0x40;
    P4_G32(ERROR_GLOBAL + 12) = 0;
    P4_G32(ADXT) = 0x13579bdf;
    P4_G32(OBJECT + 0x48) = 3;
    P4_G32(OBJECT + 0x78) = 1;
    P4_G32(OBJECT + 0x7c) = 1;
    P4_G32(OBJECT + 0x80) = 8;
    P4_G32(OBJECT + 0x2004) = WORK;
    P4_G32(OBJECT + 0x36b0) = SEE;
    P4_G32(OBJECT + 0x9c8) = UINT32_MAX - 17;
    P4_G32(OBJECT + 0x9cc) = UINT32_MAX;
    P4_G32(OBJECT + 0x9f8) = error_handle;
    P4_G32(OBJECT + 0x9fc) = OUT + 0x40;
    P4_G32(OBJECT + 0xa0c + 5 * 4) = 0;
    P4_G32(OBJECT + 0xa0c + 6 * 4) = 1;
    P4_G32(WORK) = ADXT;
    P4_G32(WORK + 4) = JOINT;
    P4_G32(WORK + 8) = 0x20000;
    P4_G32(WORK + 0x38) = 0;
    P4_G32(WORK + 0x40) = 0;
    P4_G32(WORK + 0x48) = INT32_MAX - 7;
    P4_G32(SEE + 0xd0c + 0x0c) = 2;
    P4_G32(SEE + 0xd0c + 0x10) = 48000;
    P4_G32(OBJECT + 0xd30 + 0x158) = 0;
    P4_G32(OBJECT + 0xd30 + 0x15c) = 0;
    P4_G32(OBJECT + 0xd30 + 0x118) = 1;
    P4_G32(OBJECT + 0xd30 + 0x13c) = 0;
    P4_G32(OBJECT + 0xd30 + 0x140) = 48000;
    P4_G32(OBJECT + 0xd30 + 0x110) = 0;
    P4_G32(OBJECT + 0xd30 + 0x114) = 48000;
    P4_G32(JOINT) = VT;
    P4_G32(VT + 0x18) = sj_handles[0];
    P4_G32(VT + 0x1c) = sj_handles[2];
    P4_G32(VT + 0x20) = sj_handles[1];
    P4_G32(ALT_VT + 0x18) = sj_handles[0];
    P4_G32(ALT_VT + 0x1c) = sj_handles[4];
    P4_G32(ALT_VT + 0x20) = sj_handles[3];
    memset(P4_G2H(DATA), pattern >= 2 && pattern <= 6 ? 0xff : 0, 512);
    if (pattern == 1) P4_G8(DATA) = 1;
    if (pattern >= 3 && pattern <= 6) {
        u32 offset = pattern == 3 ? 0 : pattern == 4 ? 2 : 34;
        P4_G8(DATA + offset) = 0x80;
        P4_G8(DATA + offset + 1) = 1;
        if (pattern == 6) { P4_G8(DATA) = 0x80; P4_G8(DATA + 1) = 1; }
    }
    if (pattern == 7) {
        /* A self-authored 36-byte ADX header, not bytes copied from retail. */
        unsigned char *p = P4_G2H(DATA);
        p[0] = 0x80; p[1] = 0; p[2] = 0; p[3] = 32;
        p[4] = 3; p[5] = 18; p[6] = 4; p[7] = 2;
        p[8] = 0; p[9] = 0; p[10] = 0xbb; p[11] = 0x80;
        p[15] = 64;
    }
    free_len = 288;
    inserted_result = 0;
    switch_vtable = 0;
}

static void compare(unsigned slot, const u32 args[4])
{
    Trace native_trace;
    u64 result = 0, reference;
    save_initial();
    switch (slot) {
    case GET: case PUT:
        ((void (*)(void *, s32, Ck *))native_fn[slot])(
            P4_G2H(args[0]), (s32)args[1], P4_G2H(args[2]));
        break;
    case INSERT:
        result = (u64)(int64_t)((s32 (*)(void *, s32, s32))native_fn[slot])(
            P4_G2H(args[0]), (s32)args[1], (s32)args[2]);
        break;
    case PHASE:
        result = P4_H2G(((unsigned char *(*)(void *, unsigned char *, s32))native_fn[slot])(
            P4_G2H(args[0]), P4_G2H(args[1]), (s32)args[2]));
        break;
    case ERROR_INFO:
        ((void (*)(void *, s32))native_fn[slot])(P4_G2H(args[0]), (s32)args[1]);
        break;
    case ERROR:
        result = (u64)(int64_t)((s32 (*)(void *, s32))native_fn[slot])(
            P4_G2H(args[0]), (s32)args[1]);
        break;
    default:
        ((void (*)(void *, unsigned char *, s32, s32 *))native_fn[slot])(
            P4_G2H(args[0]), P4_G2H(args[1]), (s32)args[2], P4_G2H(args[3]));
        break;
    }
    capture();
    native_trace = trace;
    restore_initial();
    reference = retail(addresses[slot], args);
    if ((slot == INSERT || slot == PHASE || slot == ERROR) && result != reference) {
        fprintf(stderr, "SFD case %u entry=%08x native=%llx retail=%llx\n", cases,
                addresses[slot], (unsigned long long)result, (unsigned long long)reference);
        exit(1);
    }
    compare_state(slot, &native_trace);
}

static void compare_chain(void)
{
    static const u32 expected_steps[4][3] = {
        {0x00512d90, 18, 0x00512bb0}, {0x00512bb0, 36, 0x005128a0},
        {0x005128a0, 0, 0x005127e8}, {0x005127e8, 458, 0x005127e8}
    };
    Trace native_trace;
    u32 native_steps[4][3], reference_steps[4][3];
    u32 args[4] = {OBJECT, DATA, 512, OUT};
    u32 next = callback_handles[0];
    prepare(7);
    memmove(P4_G2H(DATA + 18), P4_G2H(DATA), 494);
    memset(P4_G2H(DATA), 0, 18);
    P4_G32(OBJECT + 0x80) = 7;
    free_len = 458;
    save_initial();
    for (unsigned i = 0; i < 4; i++) {
        native_steps[i][0] = callback_code(next);
        ((void (*)(void *, unsigned char *, s32, s32 *))p4_fn_g2h_sig(
            next, P4_SIG(4, 0, 1, 1, 0, 1, 0, 0)))(
            P4_G2H(args[0]), P4_G2H(args[1]), (s32)args[2], P4_G2H(args[3]));
        native_steps[i][1] = P4_G32(OUT);
        next = P4_G32(WORK + 0x3c);
        native_steps[i][2] = callback_code(next);
        args[1] += P4_G32(OUT);
        args[2] -= P4_G32(OUT);
    }
    capture();
    native_trace = trace;
    restore_initial();
    args[1] = DATA; args[2] = 512;
    next = addresses[SILENCE];
    for (unsigned i = 0; i < 4; i++) {
        reference_steps[i][0] = next;
        retail(next, args);
        reference_steps[i][1] = P4_G32(OUT);
        next = P4_G32(WORK + 0x3c);
        reference_steps[i][2] = next;
        args[1] += P4_G32(OUT);
        args[2] -= P4_G32(OUT);
    }
    if (memcmp(native_steps, reference_steps, sizeof native_steps) != 0 ||
        memcmp(reference_steps, expected_steps, sizeof expected_steps) != 0) {
        fprintf(stderr, "SFD callback chain transitions or consumption differ\n");
        exit(1);
    }
    compare_state(SILENCE, &native_trace);
}

static int test_main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    static const s32 sizes[] = {0, 1, 17, 18, 36, 0x18fff, 0x19000, 0x19001, 0x19020};
    static const s32 returns[] = {INT32_MIN, -1, 0, 1, 31, 32, 64, INT32_MAX};
    static const s32 sample_offsets[] = {INT32_MIN, -65, -32, -1, 0, 1, 31, 32, 65, INT32_MAX};
    const char *elf = getenv("P4PC_ELF");
    if (elf == NULL || *elf == '\0') return 77;
    p4pc_load_image(elf);
    load_natives();
    for (unsigned change = 0; change < 3; change++) {
        for (size_t s = 0; s < sizeof sizes / sizeof *sizes; s++) {
            u32 args[4] = {JOINT, (u32)sizes[s], OUT + 16, 0};
            prepare(0);
            switch_vtable = change;
            free_len = sizes[s];
            compare(GET, args);
            prepare(0);
            switch_vtable = change;
            P4_G32(OUT + 16) = DEST;
            P4_G32(OUT + 20) = 0x20000;
            compare(PUT, args);
            for (size_t f = 0; f < sizeof sizes / sizeof *sizes; f++) {
                u32 copy_args[4] = {OBJECT, DATA, (u32)sizes[s], OUT};
                prepare(0);
                switch_vtable = change;
                free_len = sizes[f];
                for (u32 i = 0; i < 0x19020; i++)
                    P4_G8(DATA + i) = (unsigned char)(i * 37u ^ i >> 7 ^ cases);
                compare(COPY, copy_args);
            }
        }
    }
    for (size_t r = 0; r < sizeof returns / sizeof *returns; r++) {
        for (s32 nch = 1; nch <= 3; nch++) {
            u32 args[4] = {OBJECT, (u32)nch, 0x12345678, 0};
            prepare(0);
            inserted_result = returns[r];
            compare(INSERT, args);
        }
    }
    for (unsigned pattern = 0; pattern < 8; pattern++) {
        for (s32 len = 0; len <= 36; len++) {
            u32 args[4] = {OBJECT, DATA, (u32)len, OUT};
            prepare(pattern);
            compare(PHASE, args);
        }
        for (s32 minor = 7; minor <= 8; minor++) {
            for (s32 len = 287; len <= 289; len++) {
                u32 args[4] = {OBJECT, DATA, (u32)len, OUT};
                prepare(pattern);
                P4_G32(OBJECT + 0x80) = (u32)minor;
                compare(HEADER, args);
            }
        }
    }
    for (unsigned state = 0; state < 8; state++) {
        for (unsigned video = 0; video < 3; video++) {
            for (size_t s = 0; s < sizeof sample_offsets / sizeof *sample_offsets; s++) {
                for (size_t r = 0; r < sizeof returns / sizeof *returns; r++) {
                    u32 args[4] = {OBJECT, DATA, 288, OUT};
                    prepare((unsigned)(s % 7));
                    inserted_result = returns[r];
                    P4_G32(WORK + 0x38) = (u32)sample_offsets[s];
                    P4_G32(OBJECT + 0xa0c + 5 * 4) = video != 0;
                    P4_G32(OBJECT + 0xd30 + 0x118) = video == 2;
                    P4_G32(OBJECT + 0xd30 + 0x13c) = 64;
                    P4_G32(OBJECT + 0xd30 + 0x110) = state == 4 ? UINT32_MAX : 64;
                    if (state == 0) P4_G32(OBJECT + 0x36b0) = 0;
                    if (state == 1) P4_G32(WORK + 0x40) = 1;
                    if (state == 2) {
                        P4_G32(OBJECT + 0xd30 + 0x158) = UINT32_MAX;
                        P4_G32(OBJECT + 0xd30 + 0x15c) = UINT32_MAX;
                    }
                    if (state == 5) P4_G32(OBJECT + 0xd30 + 0x158) = 60;
                    if (state == 6) P4_G32(OBJECT + 0xd30 + 0x158) = 90000;
                    if (state == 7) P4_G32(OBJECT + 0xd30 + 0x15c) = 1;
                    compare(ALIGN, args);
                }
            }
        }
    }
    for (unsigned global = 0; global < 2; global++) {
        for (unsigned callback = 0; callback < 2; callback++) {
            for (unsigned retained = 0; retained < 2; retained++) {
                for (s32 state = -3; state <= 3; state += 3) {
                    for (size_t r = 0; r < sizeof returns / sizeof *returns; r++) {
                        u32 args[4] = {global ? 0 : OBJECT, (u32)returns[r], 0, 0};
                        prepare(0);
                        P4_G32(OBJECT + 0x48) = (u32)state;
                        P4_G32(OBJECT + 0x9f8) = callback ? error_handle : 0;
                        P4_G32(OBJECT + 0xa00) = retained ? 0x11223344 : 0;
                        P4_G32(ERROR_GLOBAL + 4) = callback ? error_handle : 0;
                        P4_G32(ERROR_GLOBAL + 12) = retained ? 0x55667788 : 0;
                        compare(ERROR, args);
                    }
                }
            }
        }
    }
    for (unsigned global = 0; global < 2; global++) {
        for (unsigned callback = 0; callback < 2; callback++) {
            for (unsigned retained = 0; retained < 2; retained++) {
                for (size_t r = 0; r < sizeof returns / sizeof *returns; r++) {
                    u32 error = global ? ERROR_GLOBAL + 4 : OBJECT + 0x9f8;
                    u32 args[4] = {error, (u32)returns[r], 0, 0};
                    prepare(0);
                    P4_G32(error) = callback ? error_handle : 0;
                    P4_G32(error + 8) = retained ? 0x33445566 : 0;
                    compare(ERROR_INFO, args);
                }
            }
        }
    }
    compare_chain();
    printf("SFD raw callbacks: %u native/retail cases and full callback chain exact\n", cases);
    return 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}
