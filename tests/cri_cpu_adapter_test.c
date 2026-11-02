/* The actual generated native entries versus the retail R5900 instructions. Only the
 * external CVFS seek/status/legacy-count delegates, error dispatcher, printf
 * and application/option callbacks are intercepted. The extended 64-bit query,
 * legacy transfer tail and ADXF history run in full on both sides; generated
 * ABI classes and canonical alias bindings are checked too. History compares
 * the actual guest ring, counters, index and guards, not the RE4 model's private
 * host statics. The ELF supplies the code and formats; no retail bytes live here.
 */
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

typedef int32_t s32;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

enum {
    WORK = 0x01000000, CONTEXT = 0x01000400,
    FILE_TEXT = 0x01000800, MESSAGE_TEXT = 0x01000900,
    DETAIL_TEXT = 0x01000a00, ERROR_TEXT = 0x00925ea8,
    ROOT = 0x00743a50, LAST_ERROR = 0x00743a54,
    CVFS_OBJECT = 0x01002000, CVFS_INTERFACE1 = 0x01002100,
    CVFS_INTERFACE2 = 0x01002200, CVFS_HANDLE1 = 0x01002400, CVFS_HANDLE2 = 0x01002500,
    ADXT_OBJECT = 0x01003000, ADXT_ALTERNATE = 0x01003100,
    SFD_OBJECT = 0x01004000, SFD_SEE = 0x01008000,
    SFD_WORK = 0x0100c000, SFD_DATA = 0x0100d000, SFD_OUT = 0x0100e000,
    SFD_SPAN = 0xa100
};

typedef struct {
    u64 result;
    u32 seek_calls, handle, position, type;
    u32 status_calls, status_handle;
    u32 callback_calls, callback_context, callback_text;
    s32 callback_code;
    u32 legacy_calls, legacy_handle;
    u32 cvfs_errors, cvfs_error_message;
    u32 option_calls, option[2][6];
    u32 last_error, context[4];
    u32 object_words[2];
    u32 unlock_calls, unlock_flag;
    u32 pump_calls, reserved; /* explicit padding keeps the byte comparison defined */
    unsigned char text[512];
} Outcome;
_Static_assert(sizeof(Outcome) == 8 + 38 * 4 + 512, "outcome must have no padding");

static Outcome observed;
static s32 seek_result;
static s32 status_result;
static u32 callback_address;
static u32 option_addresses[2];
static u64 option_value;
static s32 legacy_result;
static unsigned option_available, switch_after_high;
static unsigned cases;
static R5900 cpus[2];
static unsigned depth;

enum {
    CPU_SEEK, CPU_STATUS, CPU_ERROR, CPU_QUERY64, CPU_TRANSFER,
    CPU_LEGACY, CPU_UNLOCK, CPU_PUMP, CPU_HISTORY, CPU_READY,
    CPU_SILENCE, CPU_AUDIO_INFO, CPU_AHDR, CPU_MUX, CPU_VOICE, CPU_NATIVE_COUNT
};
static const u32 native_addresses[CPU_NATIVE_COUNT] = {
    0x004f13c0, 0x004f1460, 0x004f53c0, 0x004e3448,
    0x004f14e0, 0x004f14c8, 0x004f9a58, 0x004f9a70, 0x004c7260, 0x00513380,
    0x00512d90, 0x00512ea8, 0x00513dc8, 0x00517840, 0x004cd470
};
static void *natives[CPU_NATIVE_COUNT];
static u32 sfd_header_callback;
extern void func_00512bb0(void *sfd, unsigned char *data, s32 len, s32 *nbyte);

#ifdef P4_GUEST64
static int same_register_abi(const char *actual, const char *expected)
{
    if (actual == NULL) return 0;
    /* As gptr_rewrite.letter_class: void and int share the ordinary return
     * register class. Argument count and every pointer/FP class stay exact. */
    if ((actual[0] == 'v' || actual[0] == 'i') &&
        (expected[0] == 'v' || expected[0] == 'i')) {
        return strcmp(actual + 1, expected + 1) == 0;
    }
    return strcmp(actual, expected) == 0;
}
#endif

static void load_native_targets(void)
{
#ifdef P4_GUEST64
    static const char *const signatures[CPU_NATIVE_COUNT] = {
        "ipii", "ip", "ipippi", "ip", "ip", "ip", "v", "v", "viiiii", "ip",
        "vppip", "ippp", "pp", "ip", "vii"
    };
#endif
    for (size_t i = 0; i < p4pc_native_count; i++) {
        const p4pc_native_t *entry = &p4pc_native_table[i];
        unsigned slot;
        switch (entry->addr) {
        case 0x004f13c0: slot = CPU_SEEK; break;
        case 0x004f1460: slot = CPU_STATUS; break;
        case 0x004f53c0: slot = CPU_ERROR; break;
        case 0x004e3448: slot = CPU_QUERY64; break;
        case 0x004f14e0: slot = CPU_TRANSFER; break;
        case 0x004f14c8: slot = CPU_LEGACY; break;
        case 0x004f9a58: slot = CPU_UNLOCK; break;
        case 0x004f9a70: slot = CPU_PUMP; break;
        case 0x004c7260: slot = CPU_HISTORY; break;
        case 0x00513380: slot = CPU_READY; break;
        case 0x00512d90: slot = CPU_SILENCE; break;
        case 0x00512ea8: slot = CPU_AUDIO_INFO; break;
        case 0x00513dc8: slot = CPU_AHDR; break;
        case 0x00517840: slot = CPU_MUX; break;
        case 0x004cd470: slot = CPU_VOICE; break;
        default: continue;
        }
        if (natives[slot] != NULL) {
            fprintf(stderr, "duplicate native CPU entry %08x\n", entry->addr);
            exit(1);
        }
        natives[slot] = entry->fn;
#ifdef P4_GUEST64
        if (!same_register_abi(entry->sig, signatures[slot])) {
            fprintf(stderr, "native CPU ABI %08x: %s, expected %s\n",
                    entry->addr, entry->sig ? entry->sig : "-", signatures[slot]);
            exit(1);
        }
#endif
    }
    for (unsigned i = 0; i < CPU_NATIVE_COUNT; i++) {
        if (natives[i] == NULL) {
            fprintf(stderr, "missing native CPU entry %08x\n", native_addresses[i]);
            exit(1);
        }
    }
}

static int check_voice_count_forwarder(void)
{
    static const u32 addresses[] = {
        0x0073a740, 0x0073a744, 0x0073a748, 0x0073a768, 0x0073a76c
    };
    static const u32 expected[] = {10, 10 * 0x4100, 1, 3, 4};
    u32 saved[5];
    int failed = 0;

    for (unsigned i = 0; i < 5; i++) {
        saved[i] = *(volatile u32 *)P4_G2H(addresses[i]);
    }
    /* Exercise the actual source-owned entry, not the removed private copy. */
    ((void (*)(s32, s32))natives[CPU_VOICE])(3, 4);
    for (unsigned i = 0; i < 5; i++) {
        volatile u32 *slot = P4_G2H(addresses[i]);
        if (*slot != expected[i]) {
            fprintf(stderr, "voice-count forwarder %08x: %u, expected %u\n",
                    addresses[i], *slot, expected[i]);
            failed = 1;
        }
        *slot = saved[i];
    }
    return failed;
}

static s32 probe_seek(void *handle, s32 position, s32 type)
{
    observed.seek_calls++;
    observed.handle = P4_H2G(handle);
    observed.position = (u32)position;
    observed.type = (u32)type;
    return seek_result;
}

static s32 probe_status(void *handle)
{
    observed.status_calls++;
    observed.status_handle = P4_H2G(handle);
    return status_result;
}

static s32 probe_legacy(void *handle)
{
    observed.legacy_calls++;
    observed.legacy_handle = P4_H2G(handle);
    return legacy_result;
}

/* Both names are wrapped: generation may bind an address-name call directly
 * to its canonical typed CVFS symbol. No tested adapter body is copied here.
 */
s32 __wrap_func_004e2108(void *h, s32 p, s32 t) { return probe_seek(h, p, t); }
s32 __wrap_cvFsSeek(void *h, s32 p, s32 t) { return probe_seek(h, p, t); }
s32 __wrap_func_004e22f8(void *h) { return probe_status(h); }
s32 __wrap_cvFsGetStat(void *h) { return probe_status(h); }
s32 __wrap_func_004e2750(void *h) { return probe_legacy(h); }
s32 __wrap_cvFsGetNumTr(void *h) { return probe_legacy(h); }

static void probe_unlock(s32 flag)
{
    observed.unlock_calls++;
    observed.unlock_flag = (u32)flag;
}

void __wrap_func_004f0578(s32 flag) { probe_unlock(flag); }

static void probe_pump(void) { observed.pump_calls++; }
void __wrap_func_004e4190(void) { probe_pump(); }

static void error_callback(void *context, char *text, s32 code)
{
    observed.callback_calls++;
    observed.callback_context = P4_H2G(context);
    observed.callback_text = P4_H2G(text);
    observed.callback_code = code;
    /* A wrong pointer class is observable, not merely compared as an address. */
    ((u32 *)context)[0]++;
    ((u32 *)context)[1] = (u32)code;
}

static void probe_cvfs_error(void *object, const char *message, void *handle)
{
    /* Its retail wrapper's incoming a0 is not observed: 004e1578 replaces
     * it with the registered context before the application callback. */
    (void)object;
    if (handle != NULL) abort();
    observed.cvfs_errors++;
    observed.cvfs_error_message = P4_H2G(message);
}

void __wrap_cvFsCallUsrErrFn(void *object, const char *message, void *handle)
{
    probe_cvfs_error(object, message, handle);
}

static s32 option_callback(unsigned which, void *handle, s32 command, s32 a, s32 b)
{
    unsigned n = observed.option_calls++;
    if (n < 2) {
        observed.option[n][0] = which;
        observed.option[n][1] = P4_H2G(handle);
        observed.option[n][2] = (u32)command;
        observed.option[n][3] = (u32)a;
        observed.option[n][4] = (u32)b;
        observed.option[n][5] = *(const u32 *)handle;
    }
    if (which == 1 && command == 200 && switch_after_high) {
        P4_G32(CVFS_OBJECT) = CVFS_INTERFACE2;
        P4_G32(CVFS_OBJECT + 4) = CVFS_HANDLE2;
    }
    if (command == 200) return (s32)(u32)(option_value >> 32);
    if (command == 201) return (s32)(u32)option_value;
    return 0;
}

static s32 option_callback1(void *handle, s32 command, s32 a, s32 b)
{
    return option_callback(1, handle, command, a, b);
}

static s32 option_callback2(void *handle, s32 command, s32 a, s32 b)
{
    return option_callback(2, handle, command, a, b);
}

static u64 retail(u32 entry, const u32 args[5]);

static int retail_hook(R5900 *cpu, u32 target, void *user)
{
    u32 a[8];
    u64 result;
    (void)user;
    for (unsigned i = 0; i < 8; i++) a[i] = (u32)cpu->r[4 + i];
    if (target == 0x004e2108) {
        result = probe_seek(P4_G2H(a[0]), (s32)a[1], (s32)a[2]);
    } else if (target == 0x004e22f8) {
        result = probe_status(P4_G2H(a[0]));
    } else if (target == 0x004e3448) {
        result = retail(target, a);
    } else if (target == 0x004f14c8) {
        result = retail(target, a);
    } else if (target == 0x004e2750) {
        result = probe_legacy(P4_G2H(a[0]));
    } else if (target == 0x004f0620) {
        result = retail(target, a);
    } else if (target == 0x004f0578) {
        probe_unlock((s32)a[0]);
        result = 0;
    } else if (target == 0x004e4190) {
        probe_pump();
        result = 0;
    } else if (target == 0x004e15a8) {
        probe_cvfs_error(NULL, P4_G2H(a[0]), NULL);
        result = 0;
    } else if (target == option_addresses[0] || target == option_addresses[1]) {
        result = option_callback(target == option_addresses[0] ? 1 : 2,
                                 P4_G2H(a[0]), (s32)a[1], (s32)a[2], (s32)a[3]);
    } else if (target == 0x004f53c0) {
        result = retail(target, a);
    } else if (target == 0x00442088) {
        if (a[1] == 0x0075d938) {
            result = sprintf(P4_G2H(a[0]), P4_G2H(a[1]), (s32)a[2],
                             P4_G2H(a[3]), P4_G2H(a[4]), P4_G2H(a[5]), (s32)a[6]);
        } else if (a[1] == 0x0075d950) {
            result = sprintf(P4_G2H(a[0]), P4_G2H(a[1]), (s32)a[2],
                             P4_G2H(a[3]), P4_G2H(a[4]), (s32)a[5]);
        } else {
            fprintf(stderr, "unexpected retail error format %08x\n", a[1]);
            return 0;
        }
    } else if (target == callback_address) {
        error_callback(P4_G2H(a[0]), P4_G2H(a[1]), (s32)a[2]);
        /* The error reporter must ignore the callback's scratch v0. */
        result = 0x6badc0de;
    } else {
        fprintf(stderr, "unexpected retail call %08x\n", target);
        return 0;
    }
    cpu->r[2] = result;
    return 1;
}

static u64 retail(u32 entry, const u32 args[5])
{
    R5900 *cpu;
    int rc;
    if (depth == sizeof cpus / sizeof *cpus) abort();
    cpu = &cpus[depth];
    memset(cpu, 0, sizeof *cpu);
    cpu->r[28] = 0x007690f0;
    cpu->r[29] = 0x01fff000 - depth * 0x1000;
    for (unsigned i = 0; i < 5; i++) cpu->r[4 + i] = (uint64_t)(int64_t)(s32)args[i];
    cpu->lo_pc = entry;
    switch (entry) {
    case 0x004c7260: cpu->hi_pc = 0x004c7340; break;
    case 0x004e3448: cpu->hi_pc = 0x004e34e0; break;
    case 0x004f0620: cpu->hi_pc = 0x004f0638; break;
    case 0x004f13c0: cpu->hi_pc = 0x004f1418; break;
    case 0x004f1460: cpu->hi_pc = 0x004f14b0; break;
    case 0x004f14c8: cpu->hi_pc = 0x004f14e0; break;
    case 0x004f14e0: cpu->hi_pc = 0x004f1518; break;
    case 0x004f53c0: cpu->hi_pc = 0x004f549c; break;
    case 0x004f9a58: cpu->hi_pc = 0x004f9a70; break;
    case 0x004f9a70: cpu->hi_pc = 0x004f9a88; break;
    case 0x00513380:
    case 0x00512d90:
    case 0x00512ea8:
    case 0x00513dc8:
    case 0x00517840:
        /* Execute the complete ADXT readiness and raw SFD helper closures,
         * including the actual status getter, header lookups and memcmp. */
        cpu->lo_pc = 0x00100000;
        cpu->hi_pc = 0x02000000;
        break;
    default: abort();
    }
    cpu->hook = retail_hook;
    cpu->max_steps = 10000;
    depth++;
    rc = r5900_call(cpu, entry);
    depth--;
    if (rc != R5900_OK) {
        fprintf(stderr, "retail %08x: rc=%d pc=%08x fault=%s address=%08x\n",
                entry, rc, cpu->pc_now, cpu->fault_why ? cpu->fault_why : "-", cpu->fault_addr);
        exit(2);
    }
    if (entry == 0x004c7260 || entry == 0x004f0620 ||
        entry == 0x004f9a58 || entry == 0x004f9a70) {
        return 0; /* void: scratch v0 is not an output */
    }
    if (entry != 0x004e3448 && entry != 0x004f14e0 &&
        cpu->r[2] != (uint64_t)(int64_t)(s32)cpu->r[2]) {
        fprintf(stderr, "retail %08x returned a non-sign-extended s32: %016llx\n",
                entry, (unsigned long long)cpu->r[2]);
        exit(2);
    }
    return cpu->r[2];
}

static void prepare(unsigned state)
{
    memset(&observed, 0, sizeof observed);
    P4_G32(CVFS_OBJECT) = CVFS_INTERFACE1;
    P4_G32(CVFS_OBJECT + 4) = CVFS_HANDLE1;
    memset(P4_G2H(CVFS_INTERFACE1), 0, 0x68);
    memset(P4_G2H(CVFS_INTERFACE2), 0, 0x68);
    P4_G32(CVFS_INTERFACE1 + 0x60) = option_available ? option_addresses[0] : 0;
    P4_G32(CVFS_INTERFACE2 + 0x60) = option_addresses[1];
    P4_G32(CVFS_HANDLE1) = 0x13579bdf;
    P4_G32(CVFS_HANDLE2) = 0x2468ace0;
    memset(P4_G2H(WORK), 0, 0x80);
    memset(P4_G2H(CONTEXT), 0, 16);
    P4_G32(CONTEXT) = 0x12345678;
    P4_G32(ROOT) = state == 0 ? 0 : WORK;
    P4_G32(LAST_ERROR) = 0xcafebabe;
    if (state == 2) {
        P4_G32(WORK + 0x40) = callback_address;
        P4_G32(WORK + 0x44) = CONTEXT;
    }
    memset(P4_G2H(ERROR_TEXT), 0xa5, sizeof observed.text);
}

static Outcome run(int use_retail, u32 entry, const u32 args[5], unsigned state)
{
    prepare(state);
    if (use_retail) {
        observed.result = retail(entry, args);
    } else if (entry == 0x004f13c0) {
        observed.result = ((s32 (*)(void *, s32, s32))natives[CPU_SEEK])(
            P4_G2H(args[0]), (s32)args[1], (s32)args[2]);
    } else if (entry == 0x004f1460) {
        observed.result = ((s32 (*)(void *))natives[CPU_STATUS])(P4_G2H(args[0]));
    } else if (entry == 0x004e3448) {
        observed.result = ((u64 (*)(void *))natives[CPU_QUERY64])(P4_G2H(args[0]));
    } else if (entry == 0x004f14e0) {
        observed.result = ((u64 (*)(void *))natives[CPU_TRANSFER])(P4_G2H(args[0]));
    } else if (entry == 0x004f14c8) {
        observed.result = ((s32 (*)(void *))natives[CPU_LEGACY])(P4_G2H(args[0]));
    } else if (entry == 0x004f9a58) {
        ((void (*)(void))natives[CPU_UNLOCK])();
    } else if (entry == 0x004f9a70) {
        ((void (*)(void))natives[CPU_PUMP])();
    } else {
        observed.result = ((s32 (*)(void *, s32, void *, void *, s32))natives[CPU_ERROR])(
            P4_G2H(args[0]), (s32)args[1], P4_G2H(args[2]), P4_G2H(args[3]), (s32)args[4]);
    }
    observed.last_error = P4_G32(LAST_ERROR);
    memcpy(observed.context, P4_G2H(CONTEXT), sizeof observed.context);
    memcpy(observed.object_words, P4_G2H(CVFS_OBJECT), sizeof observed.object_words);
    memcpy(observed.text, P4_G2H(ERROR_TEXT), sizeof observed.text);
    return observed;
}

static void compare(u32 entry, const u32 args[5], unsigned state)
{
    Outcome native = run(0, entry, args, state);
    Outcome reference = run(1, entry, args, state);
    const unsigned char *a = (const unsigned char *)&native;
    const unsigned char *b = (const unsigned char *)&reference;
    for (size_t i = 0; i < sizeof native; i++) {
        if (a[i] != b[i]) {
            fprintf(stderr, "case %u entry=%08x state=%u args=%08x,%08x,%08x,%08x,%08x "
                    "byte=%zu native=%02x retail=%02x\n", cases, entry, state,
                    args[0], args[1], args[2], args[3], args[4], i, a[i], b[i]);
            exit(1);
        }
    }
    cases++;
}

static void compare_history(u32 index, u32 command, u32 phase)
{
    enum {
        SNAPSHOT = 0x007165e8, SIZE = 0x144,
        COUNTERS = 0x007166f8, NEXT = 0x00716718
    };
    unsigned char initial[SIZE], native[SIZE];
    u16 count = index & 1 ? UINT16_MAX : 0x7fff;
    u32 args[5] = {
        command, phase, index & 1 ? 0x003a2770 : 0xdeadbeef,
        0x87654321 ^ index, UINT32_MAX
    };

    for (size_t i = 0; i < sizeof initial; i++) initial[i] = (unsigned char)(i * 37 + 0x5b);
    memcpy(initial + NEXT - SNAPSHOT, &index, sizeof index);
    memcpy(initial + COUNTERS - SNAPSHOT + command * 2, &count, sizeof count);
    memcpy(P4_G2H(SNAPSHOT), initial, sizeof initial);
    ((void (*)(s32, s32, s32, s32, s32))natives[CPU_HISTORY])(
        (s32)args[0], (s32)args[1], (s32)args[2], (s32)args[3], (s32)args[4]);
    memcpy(native, P4_G2H(SNAPSHOT), sizeof native);
    memcpy(P4_G2H(SNAPSHOT), initial, sizeof initial);
    retail(0x004c7260, args);
    for (size_t i = 0; i < sizeof native; i++) {
        unsigned char reference = ((unsigned char *)P4_G2H(SNAPSHOT))[i];
        if (native[i] != reference) {
            fprintf(stderr, "history case %u index=%u command=%u phase=%u "
                    "address=%08x native=%02x retail=%02x\n",
                    cases, index, command, phase, SNAPSHOT + (u32)i, native[i], reference);
            exit(1);
        }
    }
    cases++;
}

static void compare_readiness(u32 handle, unsigned state)
{
    u32 initial[8] = {
        0x12340056u | state << 8, 0x11223344, 0x55667788, 0x99aabbcc,
        0x89ab00cdu | (255 - state) << 8, 0xfedcba98, 0x76543210, 0xdeadbeef
    };
    u32 native[8];
    u32 args[5] = {handle, 0, 0, 0, 0};
    s32 result;
    u64 reference;

    memcpy(P4_G2H(ADXT_OBJECT), initial, 16);
    memcpy(P4_G2H(ADXT_ALTERNATE), initial + 4, 16);
    result = ((s32 (*)(void *))natives[CPU_READY])(P4_G2H(handle));
    memcpy(native, P4_G2H(ADXT_OBJECT), 16);
    memcpy(native + 4, P4_G2H(ADXT_ALTERNATE), 16);
    memcpy(P4_G2H(ADXT_OBJECT), initial, 16);
    memcpy(P4_G2H(ADXT_ALTERNATE), initial + 4, 16);
    reference = retail(0x00513380, args);
    if ((u64)(int64_t)result != reference) {
        fprintf(stderr, "readiness case %u handle=%08x state=%u native=%d retail=%llu\n",
                cases, handle, state, result, (unsigned long long)reference);
        exit(1);
    }
    for (unsigned i = 0; i < 8; i++) {
        u32 address = (i < 4 ? ADXT_OBJECT : ADXT_ALTERNATE) + (i & 3) * 4;
        if (native[i] != P4_G32(address) || native[i] != initial[i]) {
            fprintf(stderr, "readiness case %u changed input %08x\n", cases, address);
            exit(1);
        }
    }
    cases++;
}

static void prepare_sfd(s32 valid, s32 major, s32 minor, unsigned have_see,
                        s32 seek, s32 nch, s32 sfreq, int non_silence)
{
    static const u64 counters[] = {
        0, UINT32_MAX, UINT64_MAX - 17, UINT64_MAX
    };
    static const s32 samples[] = {0, INT32_MAX - 15, INT32_MIN};
    u64 bytes = counters[cases % 4];
    memset(P4_G2H(SFD_OBJECT), 0xa5, SFD_SPAN);
    P4_G32(SFD_OBJECT + 0x78) = (u32)valid;
    P4_G32(SFD_OBJECT + 0x7c) = (u32)major;
    P4_G32(SFD_OBJECT + 0x80) = (u32)minor;
    P4_G32(SFD_OBJECT + 0x2004) = SFD_WORK;
    P4_G32(SFD_OBJECT + 0x36b0) = have_see ? SFD_SEE : 0;
    P4_G32(SFD_OBJECT + 0x9c8) = (u32)bytes;
    P4_G32(SFD_OBJECT + 0x9cc) = (u32)(bytes >> 32);
    P4_G32(SFD_WORK + 0x38) = (u32)samples[cases % 3];
    P4_G32(SFD_WORK + 0x40) = (u32)seek;
    P4_G32(SFD_SEE + 0xd0c + 0x0c) = (u32)nch;
    P4_G32(SFD_SEE + 0xd0c + 0x10) = (u32)sfreq;
    for (unsigned i = 0; i + 18 <= 512; i += 18)
        memcpy(P4_G2H(SFD_DATA + i), P4_G2H(0x007601d0), 18);
    if (non_silence >= 0)
        P4_G8(SFD_DATA + (u32)non_silence * 18) ^= 0x80;
}

static void compare_sfd(unsigned slot, s32 len, u32 output)
{
    static unsigned char initial[SFD_SPAN], native[SFD_SPAN];
    u32 args[5] = {SFD_OBJECT, SFD_OUT, SFD_OUT + 4, 0, 0};
    u64 result = 0, reference;
    u32 callback;

    memcpy(initial, P4_G2H(SFD_OBJECT), SFD_SPAN);
    switch (slot) {
    case CPU_MUX:
        result = (u64)(int64_t)((s32 (*)(void *))natives[slot])(P4_G2H(SFD_OBJECT));
        break;
    case CPU_AHDR:
        result = P4_H2G(((u32 *(*)(void *))natives[slot])(P4_G2H(SFD_OBJECT)));
        break;
    case CPU_AUDIO_INFO:
        result = (u64)(int64_t)((s32 (*)(void *, s32 *, s32 *))natives[slot])(
            P4_G2H(SFD_OBJECT), P4_G2H(args[1]), P4_G2H(args[2]));
        break;
    case CPU_SILENCE:
        args[1] = SFD_DATA;
        args[2] = (u32)len;
        args[3] = output;
        ((void (*)(void *, unsigned char *, s32, s32 *))natives[slot])(
            P4_G2H(SFD_OBJECT), P4_G2H(SFD_DATA), len, P4_G2H(output));
        break;
    default: abort();
    }
    memcpy(native, P4_G2H(SFD_OBJECT), SFD_SPAN);
    /* One explicit code-pointer correspondence, not a data-address mask:
     * the next callback is the actual compiled func_00512bb0 symbol. */
    memcpy(&callback, native + SFD_WORK - SFD_OBJECT + 0x3c, sizeof callback);
    if (slot == CPU_SILENCE && callback == sfd_header_callback) {
        callback = 0x00512bb0;
        memcpy(native + SFD_WORK - SFD_OBJECT + 0x3c, &callback, sizeof callback);
    }
    memcpy(P4_G2H(SFD_OBJECT), initial, SFD_SPAN);
    reference = retail(native_addresses[slot], args);
    if (slot != CPU_SILENCE && result != reference) {
        fprintf(stderr, "SFD case %u entry=%08x native=%llx retail=%llx\n", cases,
                native_addresses[slot], (unsigned long long)result,
                (unsigned long long)reference);
        exit(1);
    }
    if (memcmp(native, P4_G2H(SFD_OBJECT), SFD_SPAN) != 0) {
        for (u32 i = 0; i < SFD_SPAN; i++) {
            if (native[i] != P4_G8(SFD_OBJECT + i)) {
                fprintf(stderr, "SFD case %u entry=%08x len=%d address=%08x native=%02x retail=%02x\n",
                        cases, native_addresses[slot], len, SFD_OBJECT + i,
                        native[i], P4_G8(SFD_OBJECT + i));
                exit(1);
            }
        }
    }
    cases++;
}

static int test_main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    static const s32 types[] = {0, 1, 2, -1, 3, INT32_MIN, INT32_MAX};
    static const s32 positions[] = {0, -17, 1234567, INT32_MIN, INT32_MAX};
    static const s32 results[] = {0, -33, 0x12345678, INT32_MIN, INT32_MAX};
    static const s32 lines[] = {0, 132, 32767, 32768, -1, 0x12348001};
    static const s32 statuses[] = {INT32_MIN, -1, 0, 1, 2, 3, 4, INT32_MAX};
    static const u64 counts[] = {
        0, 1, 0x7fffffff, 0x80000000, 0xffffffff, UINT64_C(0x100000000),
        UINT64_C(0x100000001), UINT64_C(0x7fffffffffffffff),
        UINT64_C(0x8000000000000000), UINT64_C(0xffffffff00000000), UINT64_MAX
    };
    static const u32 history_indices[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 31, 32, INT32_MAX
    };
    static const s32 versions[][2] = {
        {0, 0}, {1, 7}, {1, 8}, {1, 9}, {2, 0}, {-1, 90},
        {INT32_MAX, INT32_MIN}, {INT32_MIN, INT32_MAX}
    };
    static const s32 valid_flags[] = {-1, 0, 1};
    static const s32 seek_flags[] = {INT32_MIN, -1, 0, 1, INT32_MAX};
    static const s32 channels[] = {-1, 1, 2, 3};
    static const s32 frequencies[] = {-44100, 0, 48000};
    static const s32 lengths[] = {-1, 0, 1, 17, 18, 19, 35, 36, 37, 53, 54, 55, 288};
    static const int non_silence[] = {-1, 0, 1, 2, 7, 15};
    static const u32 output_aliases[] = {
        SFD_OUT + 8, SFD_OBJECT + 0x9c8, SFD_OBJECT + 0x9cc,
        SFD_WORK + 0x38, SFD_WORK + 0x3c
    };
    const char *elf = getenv("P4PC_ELF");
    if (elf == NULL || *elf == '\0') return 77;
    p4pc_load_image(elf);
    load_native_targets();
    if (check_voice_count_forwarder()) return 1;
    callback_address = p4_fn_h2g_sig(error_callback, "vppi");
    option_addresses[0] = p4_fn_h2g_sig(option_callback1, "ipiii");
    option_addresses[1] = p4_fn_h2g_sig(option_callback2, "ipiii");
    sfd_header_callback = P4_FN_H2G(func_00512bb0);
    strcpy(P4_G2H(FILE_TEXT), "cpu-witness.c");
    strcpy(P4_G2H(MESSAGE_TEXT), "seek rejected");
    strcpy(P4_G2H(DETAIL_TEXT), "test-volume");
    for (unsigned state = 0; state < 3; state++) {
        u32 unused[5] = {0x12345678, 0x76543210, 0xfedcba98, 0x89abcdef, 0x55aa33cc};
        compare(0x004f9a58, unused, state);
        compare(0x004f9a70, unused, state);
        for (size_t r = 0; r < sizeof results / sizeof *results; r++) {
            u32 args[5] = {CVFS_OBJECT, 0, 0, 0, 0};
            legacy_result = results[r];
            compare(0x004f14c8, args, state);
            args[0] = 0;
            compare(0x004f14c8, args, state);
        }
        for (size_t s = 0; s < sizeof statuses / sizeof *statuses; s++) {
            u32 args[5] = {0x01234000, 0, 0, 0, 0};
            status_result = statuses[s];
            compare(0x004f1460, args, state);
            args[0] = 0;
            compare(0x004f1460, args, state);
        }
        for (size_t t = 0; t < sizeof types / sizeof *types; t++) {
            for (size_t p = 0; p < sizeof positions / sizeof *positions; p++) {
                for (size_t r = 0; r < sizeof results / sizeof *results; r++) {
                    u32 args[5] = {0x01234000, (u32)positions[p], (u32)types[t], 0, 0};
                    seek_result = results[r];
                    compare(0x004f13c0, args, state);
                    args[0] = 0;
                    compare(0x004f13c0, args, state);
                }
            }
        }
        for (size_t l = 0; l < sizeof lines / sizeof *lines; l++) {
            for (size_t r = 0; r < sizeof results / sizeof *results; r++) {
                for (unsigned detail = 0; detail < 2; detail++) {
                    u32 args[5] = {FILE_TEXT, (u32)lines[l], MESSAGE_TEXT,
                                   detail ? DETAIL_TEXT : 0, (u32)results[r]};
                    compare(0x004f53c0, args, state);
                }
            }
        }
    }
    for (size_t c = 0; c < sizeof counts / sizeof *counts; c++) {
        option_value = counts[c];
        for (option_available = 0; option_available < 2; option_available++) {
            for (switch_after_high = 0; switch_after_high < 2; switch_after_high++) {
                for (unsigned object = 0; object < 2; object++) {
                    u32 args[5] = {object ? CVFS_OBJECT : 0, 0, 0, 0, 0};
                    compare(0x004e3448, args, 0);
                    for (size_t r = 0; r < sizeof results / sizeof *results; r++) {
                        legacy_result = results[r];
                        compare(0x004f14e0, args, 0);
                    }
                }
            }
        }
    }
    for (size_t i = 0; i < sizeof history_indices / sizeof *history_indices; i++) {
        for (u32 command = 0; command < 16; command++) {
            for (u32 phase = 0; phase < 2; phase++) {
                compare_history(history_indices[i], command, phase);
            }
        }
    }
    for (unsigned state = 0; state < 256; state++) {
        compare_readiness(ADXT_OBJECT, state);
        compare_readiness(ADXT_ALTERNATE, state);
    }
    compare_readiness(0, 0);
    for (size_t f = 0; f < sizeof valid_flags / sizeof *valid_flags; f++) {
        for (size_t v = 0; v < sizeof versions / sizeof *versions; v++) {
            prepare_sfd(valid_flags[f], versions[v][0], versions[v][1],
                        1, 0, 2, 48000, -1);
            compare_sfd(CPU_MUX, 0, SFD_OUT + 8);
        }
    }
    for (unsigned see = 0; see < 2; see++) {
        for (size_t s = 0; s < sizeof seek_flags / sizeof *seek_flags; s++) {
            for (size_t c = 0; c < sizeof channels / sizeof *channels; c++) {
                for (size_t f = 0; f < sizeof frequencies / sizeof *frequencies; f++) {
                    prepare_sfd(1, 1, 7, see, seek_flags[s], channels[c], frequencies[f], -1);
                    compare_sfd(CPU_AHDR, 0, SFD_OUT + 8);
                    compare_sfd(CPU_AUDIO_INFO, 0, SFD_OUT + 8);
                }
            }
        }
    }
    for (unsigned header = 0; header < 7; header++) {
        for (size_t l = 0; l < sizeof lengths / sizeof *lengths; l++) {
            for (size_t n = 0; n < sizeof non_silence / sizeof *non_silence; n++) {
                s32 nch = header == 6 ? -1 : header >= 3 ? (s32)header - 2 : 1;
                prepare_sfd(0, 1, 8, header != 0,
                            header == 1 ? 1 : header == 2 ? -1 : 0,
                            nch, 48000, non_silence[n]);
                compare_sfd(CPU_SILENCE, lengths[l], SFD_OUT + 8);
            }
        }
    }
    for (s32 minor = 8; minor <= 10; minor++) {
        for (size_t l = 0; l < sizeof lengths / sizeof *lengths; l++) {
            prepare_sfd(1, 1, minor, 1, 0, 0, 48000, -1);
            compare_sfd(CPU_SILENCE, lengths[l], SFD_OUT + 8);
        }
    }
    for (s32 minor = 7; minor <= 8; minor++) {
        for (size_t a = 0; a < sizeof output_aliases / sizeof *output_aliases; a++) {
            prepare_sfd(1, 1, minor, 1, 0, 1, 48000, -1);
            compare_sfd(CPU_SILENCE, 55, output_aliases[a]);
        }
    }
    printf("CRI CPU adapters: %u native/retail cases exact\n", cases);
    return 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}
