/* Compare complete allocator state against unchanged retail instructions.
 * Uses the real mirrored chkmem bodies and shipping native EE allocator, not
 * a replacement checked-block implementation or an interpreted call hook. */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p4pc/gmem.h"
#include "p4pc/image.h"
#include "p4pc/platform.h"
#include "p4pc/ps2_libc.h"
#include "p4pc/hostmath.h"
#include "ee_heap.h"
#include "r5900.h"

extern int32_t chkmemAllocate(uint32_t);
extern void chkmemFree(uint8_t *);
extern int32_t chkmemReallocate(void *, uint32_t);
extern int32_t chkmemAllocZeroed(int32_t, int32_t);
extern int32_t chkmemAllocAligned(int32_t, int32_t, int32_t);
extern int32_t chkmemGetBytes(void);
extern uint16_t chkmemGetClasses(void);
extern int32_t func_0042ba20(void);
extern int32_t func_0042ba70(void);
extern int32_t (**chkmemInitRenderWareMemoryFunctions(void))(uint32_t);
extern void mdlSetColor(void *, const void *);
extern void mdlSetLayerWeight(uint8_t *, int32_t, float);
extern int32_t func_00421f98(int32_t, void *);
extern int32_t func_00421fb8(int32_t, int32_t);
extern void AfPackRunList(void);

#define HEAP_BASE 0x00938a00u
#define HEAP_SIZE 0x00100000u
#define STATE_SIZE (HEAP_SIZE + 0x460u + 4u + 4u + 0x20u + 0x78u + 12u)
static unsigned char before[STATE_SIZE], expected[STATE_SIZE], initial[STATE_SIZE];
static uint32_t active_reent;
static unsigned cases;
static uint64_t digest = UINT64_C(14695981039346656037);
static uint32_t native_callbacks[4];

static void transfer(unsigned char *buffer, int restore)
{
    static const uint32_t addresses[] = {HEAP_BASE, 0x00710080u, 0x0070c5d4u, 0, 0x009389e0u, 0x008be2a0u, 0x00764c98u};
    static const uint32_t sizes[] = {HEAP_SIZE, 0x460u, 4, 4, 0x20u, 0x78u, 12};
    for (unsigned i = 0; i < sizeof sizes / sizeof sizes[0]; ++i) {
        void *guest = P4_G2H(i == 3 ? active_reent : addresses[i]);
        if (restore) memcpy(guest, buffer, sizes[i]);
        else memcpy(buffer, guest, sizes[i]);
        buffer += sizes[i];
    }
}

static uint32_t native_call(uint32_t entry, uint32_t a, uint32_t b, uint32_t d)
{
    switch (entry) {
    case 0x0043ece8u: return P4_H2G(func_0043ece8((int32_t)a));
    case 0x0043ed08u: func_0043ed08(P4_G2H(a)); return 0;
    case 0x0043c710u: return P4_H2G(func_0043c710(a, b));
    case 0x00441558u: return P4_H2G(func_00441558(P4_G2H(a), b));
    case 0x0043ef80u: return P4_H2G(func_0043ef80(P4_G2H(a), (int32_t)b));
    case 0x0043e170u: func_0043e170(P4_G2H(a), P4_G2H(b)); return 0;
    case 0x0043c738u: return p4pc_ee_heap_calloc(a, b, d);
    case 0x00441580u: return p4pc_ee_heap_realloc(a, b, d);
    case 0x0044ec60u: return (uint32_t)chkmemAllocate(a);
    case 0x0044ee70u: chkmemFree(P4_G2H(a)); return 0;
    case 0x0044f140u: return (uint32_t)chkmemReallocate(P4_G2H(a), b);
    case 0x0044f3a0u: return (uint32_t)chkmemAllocZeroed((int32_t)a, (int32_t)b);
    case 0x0044ef70u: return (uint32_t)chkmemAllocAligned((int32_t)a, (int32_t)b, (int32_t)d);
    case 0x00421f98u: return (uint32_t)func_00421f98((int32_t)a, P4_G2H(b));
    case 0x00421fb8u: return (uint32_t)func_00421fb8((int32_t)a, (int32_t)b);
    case 0x004b6e80u: AfPackRunList(); return 0;
    case 0x0047a220u: mdlSetColor(P4_G2H(a), P4_G2H(b)); return 0;
    case 0x0047a0e0u: {
        float weight;
        memcpy(&weight, &d, sizeof weight);
        mdlSetLayerWeight(P4_G2H(a), (int32_t)b, weight);
        return 0;
    }
    default: abort();
    }
}

static uint32_t compare(uint32_t entry, uint32_t a, uint32_t b, uint32_t d)
{
    R5900 machine = {0};
    R5900KernelEnv kernel = {0x00010001u, HEAP_BASE + HEAP_SIZE};
    transfer(before, 0);
    machine.lo_pc = 0x00100000u;
    machine.hi_pc = 0x005e4000u;
    machine.max_steps = 10000000;
    machine.kernel = &kernel;
    machine.r[4] = (uint64_t)(int64_t)(int32_t)a;
    machine.r[5] = (uint64_t)(int64_t)(int32_t)b;
    machine.r[6] = (uint64_t)(int64_t)(int32_t)d;
    if (entry == 0x0047a0e0u) machine.f[12] = d;
    machine.r[28] = 0x007690f0u;
    machine.r[29] = 0x01ff0000u;
#ifndef P4_GUEST64
    /* The i386 C ABI stores native callbacks in RW slots. The interpreter
     * instead receives the original code words; no algorithm is hooked. */
    static const uint32_t retail_callbacks[] = {0x44ec60, 0x44ee70, 0x44f140, 0x44f3a0};
    memcpy(P4_G2H(0x008873e8u), retail_callbacks, sizeof retail_callbacks);
#endif
    int rc = r5900_call(&machine, entry);
#ifndef P4_GUEST64
    memcpy(P4_G2H(0x008873e8u), native_callbacks, sizeof native_callbacks);
#endif
    if (rc != R5900_OK) {
        fprintf(stderr, "heap oracle entry=%08x args=%08x/%08x/%08x rc=%d pc=%08x word=%08x address=%08x reason=%s\n",
                entry, a, b, d, rc, machine.bad_pc, machine.bad_word, machine.fault_addr,
                machine.fault_why ? machine.fault_why : "none");
        exit(1);
    }
    transfer(expected, 0);
    transfer(before, 1);
    uint32_t result = native_call(entry, a, b, d);
    transfer(before, 0);
    int void_result = entry == 0x0043ed08u || entry == 0x0043e170u ||
        entry == 0x0044ee70u || entry == 0x0047a220u || entry == 0x0047a0e0u || entry == 0x004b6e80u;
    if ((!void_result && result != (uint32_t)machine.r[2]) || memcmp(before, expected, sizeof before)) {
        size_t offset = 0;
        while (offset < sizeof before && before[offset] == expected[offset]) ++offset;
        fprintf(stderr, "heap mismatch case=%u entry=%08x args=%08x/%08x/%08x result=%08x/%08x state_offset=%zx native=%02x retail=%02x\n",
                cases, entry, a, b, d, result, (uint32_t)machine.r[2], offset,
                offset < sizeof before ? before[offset] : 0, offset < sizeof before ? expected[offset] : 0);
        exit(1);
    }
    int enabled = func_0042ba20();
    if (enabled) func_0042ba70();
    if ((enabled != 0) != ((kernel.status & 0x10000u) != 0)) {
        fputs("allocator IRQ restoration differs from retail\n", stderr);
        exit(1);
    }
    for (unsigned shift = 0; shift < 32; shift += 8) {
        digest = (digest ^ ((result >> shift) & 255u)) * UINT64_C(1099511628211);
    }
    ++cases;
    return result;
}

static void reset(void)
{
    transfer(initial, 1);
}

static void exercise(void)
{
    static const uint32_t sizes[] = {0, 1, 7, 8, 15, 16, 17, 23, 24, 31, 32, 33,
        63, 64, 127, 128, 255, 256, 479, 480, 495, 496, 511, 512, 513,
        1023, 1024, 4095, 4096, 65536, 0x20000, 0x70000, 0xfffffff0u, 0xffffffffu};
    uint32_t pointers[sizeof sizes / sizeof sizes[0]];
    reset();
    for (unsigned i = 0; i < sizeof sizes / sizeof sizes[0]; ++i) {
        pointers[i] = compare(0x43ece8, sizes[i], 0, 0);
        if (pointers[i] && sizes[i] < HEAP_SIZE) memset(P4_G2H(pointers[i]), (int)i, sizes[i]);
    }
    /* Alternating free order exercises small/large bins and both coalescing directions. */
    for (unsigned parity = 0; parity < 2; ++parity)
        for (unsigned i = parity; i < sizeof sizes / sizeof sizes[0]; i += 2)
            compare(0x43ed08, pointers[i], 0, 0);
    for (unsigned i = 0; i < sizeof sizes / sizeof sizes[0]; ++i) {
        reset();
        uint32_t p = compare(0x43ece8, 256, 0, 0);
        memset(P4_G2H(p), 0x5a, 256);
        uint32_t q = compare(0x441558, p, sizes[i], 0);
        compare(0x43ed08, q ? q : p, 0, 0);
    }
    /* A live successor prevents top/forward growth. The predecessor is
     * smaller than the copied payload, forcing a true overlapping move. */
    reset();
    uint32_t predecessor = compare(0x43ece8, 16, 0, 0);
    uint32_t moving = compare(0x43ece8, 0x300, 0, 0);
    uint32_t successor = compare(0x43ece8, 16, 0, 0);
    memset(P4_G2H(moving), 0x7b, 0x300);
    compare(0x43ed08, predecessor, 0, 0);
    moving = compare(0x441558, moving, 0x310, 0);
    if (moving != predecessor) abort();
    compare(0x43ed08, moving, 0, 0);
    compare(0x43ed08, successor, 0, 0);
    static const uint32_t products[][2] = {{0,0}, {0,0xffffffffu}, {1,1}, {17,31},
        {0x80000001u,2}, {0xffffffffu,0xffffffffu}, {0x40000000u,4}};
    for (unsigned i = 0; i < sizeof products / sizeof products[0]; ++i) {
        reset();
        uint32_t dirty = compare(0x43ece8, 257, 0, 0);
        memset(P4_G2H(dirty), 0xa5, 257);
        compare(0x43ed08, dirty, 0, 0);
        uint32_t p = compare(0x43c710, products[i][0], products[i][1], 0);
        compare(0x43ed08, p, 0, 0);
    }
    reset();
    uint32_t q = compare(0x441558, 0, 0, 0);
    compare(0x43ed08, q, 0, 0);
    q = compare(0x43ef80, active_reent, 128, 0);
    q = compare(0x441580, active_reent, q, 512);
    compare(0x43e170, active_reent, q, 0);
    q = compare(0x43c738, active_reent, 19, 23);
    compare(0x43e170, active_reent, q, 0);
    for (unsigned i = 1; i < 30; ++i) {
        reset();
        uint32_t p = compare(0x44ec60, sizes[i], 0, 0);
        uint32_t raw = P4_G32(p - 4u);
        if ((p & 15u) || p - raw != 32u || P4_G32(raw) != ((sizes[i] + 0x37u) & ~15u)) {
            fputs("checked allocation header/backpointer differs from original layout\n", stderr); exit(1);
        }
        memset(P4_G2H(p), 0x63, sizes[i]);
        q = compare(0x44f140, p, sizes[i] + 17u, 0);
        compare(0x44ee70, q, 0, 0);
        if (chkmemGetBytes() != 0 || chkmemGetClasses() != 0) abort();
    }
    reset();
    q = compare(0x44f3a0, 19, 23, 0);
    compare(0x44ee70, q, 0, 0);
    for (unsigned alignment = 16; alignment <= 256; alignment *= 2) {
        reset();
        q = compare(0x44ef70, 279, alignment, 0);
        compare(0x44ee70, q, 0, 0);
    }
    static const uint16_t classes[] = {1, 7, 8, 0x0107};
    for (unsigned i = 0; i < sizeof classes / sizeof classes[0]; ++i) {
        reset();
        memcpy(P4_G2H(0x008be2a0u + 0x34u), &classes[i], 2);
        q = compare(0x44ec60, 279, 0, 0);
        compare(0x44ee70, q, 0, 0);
        if (chkmemGetBytes() != 0 || chkmemGetClasses() != classes[i]) abort();
    }
    reset();
    uint32_t live[64] = {0}, random = 0x19c3635fu;
    for (unsigned i = 0; i < 1024; ++i) {
        random = random * 1664525u + 1013904223u;
        unsigned slot = (random >> 16) & 63u;
        uint32_t size = (random >> 2) & 16383u;
        if (!live[slot]) {
            live[slot] = compare(0x43ece8, size, 0, 0);
            if (live[slot]) memset(P4_G2H(live[slot]), (int)i, size);
        } else if (random & 1u) {
            q = compare(0x441558, live[slot], size, 0);
            if (q) live[slot] = q;
        } else {
            compare(0x43ed08, live[slot], 0, 0);
            live[slot] = 0;
        }
    }
    for (unsigned i = 0; i < 64; ++i) compare(0x43ed08, live[i], 0, 0);
    /* Original btlUnitReleaseModel frees, then calls these real model setters:
     * colour at +208 decimal (0xd0), layer0 weight at +0xf4/+0x244. */
    reset();
    uint32_t model = compare(0x44ec60, 0x2a0, 0, 0);
    compare(0x44ee70, model, 0, 0);
    P4_G32(0x01d00000u) = 0x6363635fu;
    compare(0x47a220, model, 0x01d00000u, 0);
    compare(0x47a0e0, model, 0, 0x3f800000u);
    q = compare(0x44ec60, 0x3400, 0, 0);
    compare(0x44ee70, q, 0, 0);
    reset();
    uint32_t statbuf = compare(0x43ece8, 0x80, 0, 0);
    static const uint32_t descriptors[] = {0, 1, UINT32_MAX, INT32_MAX, 0x80000000u};
    for (unsigned i = 0; i < sizeof descriptors / sizeof descriptors[0]; ++i) {
        memset(P4_G2H(statbuf), 0xa5, 0x80);
        compare(0x421f98, descriptors[i], statbuf, 0);
    }
    compare(0x43ed08, statbuf, 0, 0);
    static const uint32_t commands[] = {0, 2, UINT32_MAX, INT32_MAX, 0x80000000u};
    for (unsigned i = 0; i < sizeof commands / sizeof commands[0]; ++i)
        compare(0x421fb8, commands[i], 0xdeadbeefu, 0);
    /* Real AfPackRunList reaps an empty afterimage through the RW free slot
     * twice (jalr at004b708c/004b709c), rather than a direct chkmem call.
     * The source casts a void ** slot from a void *** table to code. */
    reset();
    uint32_t node = compare(0x44ec60, 0x20, 0, 0);
    uint32_t work = compare(0x44ec60, 0x40, 0, 0);
    memset(P4_G2H(node), 0, 0x20);
    memset(P4_G2H(work), 0, 0x40);
    P4_G32(node + 4u) = work;
    P4_G32(0x00764c9cu) = node;
    P4_G32(0x00764ca0u) = node;
    compare(0x4b6e80, 0, 0, 0);
    if (P4_G32(0x00764c9cu) || P4_G32(0x00764ca0u) || chkmemGetBytes()) abort();
}

int main(void)
{
    const char *elf = getenv("P4PC_ELF");
    if (!elf || !*elf) return 77;
    p4pc_load_image(elf); /* Do not patch any interpreted algorithm entry. */
    p4pc_apply_patches(); /* Registers native callbacks; allocator instructions are untouched. */
    if (p4pc_heap_init(P4_G2H(HEAP_BASE), HEAP_SIZE)) return 2;
    active_reent = P4_G32(0x00710070u);
    memcpy(native_callbacks, chkmemInitRenderWareMemoryFunctions(), sizeof native_callbacks);
    memcpy(P4_G2H(0x008873e8u), native_callbacks, sizeof native_callbacks);
    transfer(initial, 0);
    exercise();
    p4pc_fpu_ee_mode();
    exercise();
    printf("retail heap: %u exact pointer/heap/bin/header/errno/checked-state comparisons, digest=%016" PRIx64 "\n", cases, digest);
    return 0;
}
