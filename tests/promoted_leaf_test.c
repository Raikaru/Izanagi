/* Host test for the small promoted leaf overrides (tests/CMakeLists.txt compiles each override
 * as its own translation unit). Retail globals the overrides read are mapped at their retail
 * addresses (see p4_os_map_fixed). Expected values are worked from the retail asm
 * descriptions in the override headers, not from the override code.
 *
 *   001f5680 reads the object pointer at 0x0076449C, then +0xC68, then the u16 at +2
 *   00396e80 reads the table base at 0x007646C8
 *   003cc250 calls the function pointer at 0x00887300
 *   003d5710 calls the function pointer at +0x28 of *arg0
 *   003b5fb0 003b6390 003b6420 003b64c0 are strided record copies
 *   003b6540 003b65d0 003d0790 003cbf30 ... later batches add their cases here */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "p4pc/os.h"
#include "guest_test.h"

#ifdef P4_GUEST64
/* decomp type.h clashes with the system headers on a 64-bit host (uintptr_t); the names the test uses are these */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;
typedef float f32;
typedef uint64_t u64;
typedef int64_t s64;
#else
#include "type.h"
#endif

/* Address words refer to objects on the guest stack. Native callbacks carry
 * their real ABI; retail globals are guest memory at their retail addresses. */
#define GADDR(x) ((u32)P4_H2G(x))
#define GFN(f, sig) ((u32)p4_fn_h2g_sig((f), (sig)))

s32 func_001f5680(void);
s32 func_00396e80(s32 arg0);
s32 func_003cc250(s32 arg0, u8 **arg1);
s32 func_003d5710(u8 **arg0);
void func_003b5fb0(u8 *arg0, s32 arg1, u8 *arg2);
void func_003b6390(u8 *arg0, s32 arg1, u8 *arg2);
void func_003b6420(u8 *arg0, s32 arg1, u8 *arg2);
void func_003b64c0(u8 *arg0, s32 arg1, u8 *arg2);
void func_003b4230(u8 *arg0, s32 arg1, u8 *arg2);
void func_003b65d0(u8 *arg0, s32 arg1, u8 *arg2);
void func_003b6540(u8 *arg0, s32 arg1, u8 *arg2);
s32 func_003d0790(u8 *arg0);
u8 *func_003cbf30(u8 *arg0, u8 *arg1);
u32 func_003ce840(u8 *arg0);
void func_0039a910(u8 *arg0);
s32 func_003a2770(u8 *arg0, u32 *arg1, u32 arg2, u32 arg3);
s32 func_003a5180(u8 *arg0);
void func_003b4ab0(u8 *arg0, f32 *arg1, f32 *arg2, s32 arg3, u8 *arg4);
void func_003b4c10(u8 *arg0, f32 *arg1, f32 *arg2, s32 arg3, u8 *arg4);
void func_003b4ff0(u8 *arg0, f32 *arg1, f32 *arg2, s32 arg3, u8 *arg4);
void func_003b5650(u8 *arg0, f32 *arg1, f32 *arg2, s32 arg3, u8 *arg4);
void func_003b5ac0(u8 *arg0, f32 *arg1, f32 *arg2, s32 arg3, u8 *arg4);
void func_003b5bf0(u8 *arg0, f32 *arg1, f32 *arg2, s32 arg3, u8 *arg4);
void func_003b6020(u8 *arg0, s32 arg1, u8 *arg2);
void func_003b61e0(u8 *arg0, s32 arg1, u8 *arg2);

static int checks;
static int failures;

#define CHECK(cond, ...)                                          \
    do {                                                          \
        checks++;                                                 \
        if (!(cond)) {                                            \
            failures++;                                           \
            fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__);  \
            fprintf(stderr, __VA_ARGS__);                         \
            fputc('\n', stderr);                                  \
        }                                                         \
    } while (0)

#define G32(addr) (*(volatile u32 *)P4_G2H((uint32_t)(addr)))

static void map_fixed(uint32_t addr)
{
    if (p4t_guest_map(addr, 0x1000) != 0) {
        fprintf(stderr, "promoted_leaf_test: cannot map 0x%08x\n", (unsigned)addr);
        exit(1);
    }
}

static s32 call_arg_seen;
static s32 call_a;
static s32 call_b;
static s32 call_ret;

static s32 fn_record(s32 a, s32 b)
{
    call_a = a;
    call_b = b;
    call_arg_seen++;
    return call_ret;
}

static u8 **indirect_arg_seen;

static s32 fn_indirect(u8 **arg)
{
    indirect_arg_seen = arg;
    return 1000;
}

static void test_001f5680(void)
{
    _Alignas(16) u8 objA[0x1000] = {0};
    _Alignas(16) u8 objB[0x40] = {0};

    G32(0x0076449C) = GADDR(objA);
    *(u32 *)(objA + 0xC68) = 0;
    CHECK(func_001f5680() == -1, "null second pointer");

    *(u32 *)(objA + 0xC68) = GADDR(objB);
    *(u16 *)(objB + 2) = 0;
    CHECK(func_001f5680() == -1, "value 0");
    *(u16 *)(objB + 2) = 6;
    CHECK(func_001f5680() == -1, "value 6");
    *(u16 *)(objB + 2) = 0xFFFF;
    CHECK(func_001f5680() == -1, "value 0xFFFF");
    *(u16 *)(objB + 2) = 1;
    CHECK(func_001f5680() == 0x12, "value 1 -> %d", func_001f5680());
    *(u16 *)(objB + 2) = 5;
    CHECK(func_001f5680() == 0x16, "value 5 -> %d", func_001f5680());
}

static void test_00396e80(void)
{
    _Alignas(16) u32 rec[2] = {0};
    _Alignas(16) u32 target[4] = {0};

    G32(0x007646C8) = GADDR(rec);
    rec[0] = 0xFFFFFFFFu;
    rec[1] = 0;
    CHECK(func_00396e80(0) == 0, "terminator record");

    rec[0] = 0;
    rec[1] = 0;
    CHECK(func_00396e80(0) == 12, "null target");

    target[0] = 1;
    target[1] = 7;
    rec[1] = GADDR(target);
    CHECK(func_00396e80(0) == 12, "bit 0 set");

    target[0] = 2;
    CHECK(func_00396e80(0) == 20 + 12 * 7, "count 7 -> %d", func_00396e80(0));
    target[1] = 0;
    CHECK(func_00396e80(0) == 20, "count 0");

    /* 0xFFFFFFFF with a non-null second word is not a terminator. */
    rec[0] = 0xFFFFFFFFu;
    target[0] = 0;
    target[1] = 2;
    CHECK(func_00396e80(0) == 44, "0xFFFFFFFF with target");

    /* The argument is a byte offset added to the table base. */
    {
        u32 two[4] = {0, 0, 0xFFFFFFFFu, 0};
        G32(0x007646C8) = GADDR(two);
        CHECK(func_00396e80(8) == 0, "offset 8 terminator");
        CHECK(func_00396e80(0) == 12, "offset 0 null target");
    }
}

static void test_003cc250(void)
{
    _Alignas(16) u8 obj[64] = {0};
    _Alignas(16) u32 objw = 0;        /* the callee loads a four-byte guest pointer */
    u8 **objp = (u8 **)&objw;

    objw = GADDR(obj);

    G32(0x00887300) = GFN(fn_record, "iii");
    memset(obj, 0, sizeof obj);

    /* u16 at +0 is zero: return 0, no call, no copy. */
    *(u32 *)(obj + 4) = 0xAAAA5555u;
    call_arg_seen = 0;
    CHECK(func_003cc250(99, objp) == 0, "empty object");
    CHECK(*(u32 *)(obj + 24) == 0, "no copy when empty");
    CHECK(call_arg_seen == 0, "no call when empty");

    /* Bit 0 of +12 set: copy +4 to +24, return 1, no call. */
    *(u16 *)obj = 3;
    *(u32 *)(obj + 12) = 1;
    CHECK(func_003cc250(99, objp) == 1, "bit0 set");
    CHECK(*(u32 *)(obj + 24) == 0xAAAA5555u, "copy bit0 set");
    CHECK(call_arg_seen == 0, "no call when bit0 set");

    /* Bit 0 clear: copy, call fn(1, 0), return 1 regardless of the callee's result. */
    *(u32 *)(obj + 12) = 2;
    *(u32 *)(obj + 4) = 0x12345678u;
    call_ret = 77;
    CHECK(func_003cc250(99, objp) == 1, "bit0 clear returns 1");
    CHECK(*(u32 *)(obj + 24) == 0x12345678u, "copy bit0 clear");
    CHECK(call_arg_seen == 1 && call_a == 1 && call_b == 0, "callee args (%d, %d) x%d",
          call_a, call_b, call_arg_seen);
}

static void test_003d5710(void)
{
    _Alignas(16) u8 obj[0x40] = {0};
    _Alignas(16) u32 objw = 0;
    u8 **objp = (u8 **)&objw;

    objw = GADDR(obj);

    *(u32 *)(obj + 0x28) = GFN(fn_indirect, "ip");
    indirect_arg_seen = 0;
    CHECK(func_003d5710(objp) == 1020, "result + 20");
    CHECK(indirect_arg_seen == objp, "callee receives arg0 unchanged");
}

/* Source records: word j of record i is 0x1000 * (i + 1) + j so every word is unique. */
static void fill_src(u8 *base, u32 stride, int n, int words)
{
    int i;
    int j;

    for (i = 0; i < n; i++) {
        for (j = 0; j < words; j++) {
            *(u32 *)(base + (size_t)i * stride + j * 4) = 0x1000u * (u32)(i + 1) + (u32)j;
        }
    }
}

static u32 W(int i, int j)
{
    return 0x1000u * (u32)(i + 1) + (u32)j;
}

static void test_strided_copies(void)
{
    enum { N = 3, S = 24, SRC_STRIDE = 48 };
    _Alignas(16) u8 dstbuf[4 * N * S + 64] = {0};
    _Alignas(16) u8 srcbuf[N * SRC_STRIDE + 64] = {0};
    _Alignas(16) u8 hdr_dst[8] = {0};   /* arg0: +0 dst base, +4 dst stride */
    _Alignas(16) u8 ctx[256] = {0};     /* arg2 */
    int i;
    int k;

    *(u32 *)(hdr_dst + 0) = GADDR(dstbuf);
    *(u32 *)(hdr_dst + 4) = S;

    /* 003b6390: ctx+60 src base, ctx+64 src stride. dst k*S: {w0,w3},{w2,w3},{w2,w1},{w0,w1}. */
    memset(dstbuf, 0xEE, sizeof dstbuf);
    fill_src(srcbuf, SRC_STRIDE, N, 4);
    *(u32 *)(ctx + 60) = GADDR(srcbuf);
    *(u32 *)(ctx + 64) = SRC_STRIDE;
    func_003b6390(hdr_dst, N, ctx);
    for (i = 0; i < N; i++) {
        u8 *d = dstbuf + (size_t)i * 4 * S;
        CHECK(*(u32 *)(d + 0 * S) == W(i, 0) && *(u32 *)(d + 0 * S + 4) == W(i, 3), "6390 rec %d slot 0", i);
        CHECK(*(u32 *)(d + 1 * S) == W(i, 2) && *(u32 *)(d + 1 * S + 4) == W(i, 3), "6390 rec %d slot 1", i);
        CHECK(*(u32 *)(d + 2 * S) == W(i, 2) && *(u32 *)(d + 2 * S + 4) == W(i, 1), "6390 rec %d slot 2", i);
        CHECK(*(u32 *)(d + 3 * S) == W(i, 0) && *(u32 *)(d + 3 * S + 4) == W(i, 1), "6390 rec %d slot 3", i);
    }
    /* Bytes outside the written slots stay untouched. */
    CHECK(dstbuf[8] == 0xEE && dstbuf[S - 1] == 0xEE, "6390 gap bytes untouched");
    memset(dstbuf, 0xEE, sizeof dstbuf);
    func_003b6390(hdr_dst, 0, ctx);
    func_003b6390(hdr_dst, -2, ctx);
    CHECK(dstbuf[0] == 0xEE, "6390 count <= 0 writes nothing");

    /* 003b6420: ctx+68 src base, ctx+72 stride; slot k gets words 2k and 2k+1. */
    memset(dstbuf, 0xEE, sizeof dstbuf);
    fill_src(srcbuf, SRC_STRIDE, N, 8);
    memset(ctx, 0, sizeof ctx);
    *(u32 *)(ctx + 68) = GADDR(srcbuf);
    *(u32 *)(ctx + 72) = SRC_STRIDE;
    func_003b6420(hdr_dst, N, ctx);
    for (i = 0; i < N; i++) {
        u8 *d = dstbuf + (size_t)i * 4 * S;
        for (k = 0; k < 4; k++) {
            CHECK(*(u32 *)(d + k * S) == W(i, 2 * k) && *(u32 *)(d + k * S + 4) == W(i, 2 * k + 1),
                  "6420 rec %d slot %d", i, k);
        }
    }
    memset(dstbuf, 0xEE, sizeof dstbuf);
    func_003b6420(hdr_dst, 0, ctx);
    CHECK(dstbuf[0] == 0xEE, "6420 count 0 writes nothing");

    /* 003b64c0: pair A at ctx+144/+148, pair B at ctx+152/+156, source not advanced.
     * slots: {A0,B1},{B0,B1},{B0,A1},{A0,A1}. */
    memset(dstbuf, 0xEE, sizeof dstbuf);
    memset(ctx, 0, sizeof ctx);
    *(u32 *)(ctx + 144) = 0xA0A0A0A0u;
    *(u32 *)(ctx + 148) = 0xA1A1A1A1u;
    *(u32 *)(ctx + 152) = 0xB0B0B0B0u;
    *(u32 *)(ctx + 156) = 0xB1B1B1B1u;
    func_003b64c0(hdr_dst, N, ctx);
    for (i = 0; i < N; i++) {
        u8 *d = dstbuf + (size_t)i * 4 * S;
        CHECK(*(u32 *)(d + 0 * S) == 0xA0A0A0A0u && *(u32 *)(d + 0 * S + 4) == 0xB1B1B1B1u, "64c0 rec %d slot 0", i);
        CHECK(*(u32 *)(d + 1 * S) == 0xB0B0B0B0u && *(u32 *)(d + 1 * S + 4) == 0xB1B1B1B1u, "64c0 rec %d slot 1", i);
        CHECK(*(u32 *)(d + 2 * S) == 0xB0B0B0B0u && *(u32 *)(d + 2 * S + 4) == 0xA1A1A1A1u, "64c0 rec %d slot 2", i);
        CHECK(*(u32 *)(d + 3 * S) == 0xA0A0A0A0u && *(u32 *)(d + 3 * S + 4) == 0xA1A1A1A1u, "64c0 rec %d slot 3", i);
    }
    memset(dstbuf, 0xEE, sizeof dstbuf);
    func_003b64c0(hdr_dst, 0, ctx);
    CHECK(dstbuf[0] == 0xEE, "64c0 count 0 writes nothing");
}

/* 003b5fb0: the destination words are spaced by the destination stride, the source record's
 * four words are read from consecutive words at srcStride apart (arg2+44 base, arg2+48 stride). */
static void test_003b5fb0(void)
{
    enum { N = 3, S = 16, SRC_STRIDE = 32 };
    _Alignas(16) u8 dstbuf[4 * N * S + 64] = {0};
    _Alignas(16) u8 srcbuf[N * SRC_STRIDE + 64] = {0};
    _Alignas(16) u8 hdr[8] = {0};
    _Alignas(16) u8 ctx[128] = {0};
    int i;
    int k;

    memset(dstbuf, 0xEE, sizeof dstbuf);
    fill_src(srcbuf, SRC_STRIDE, N, 4);
    *(u32 *)(hdr + 0) = GADDR(dstbuf);
    *(u32 *)(hdr + 4) = S;
    *(u32 *)(ctx + 44) = GADDR(srcbuf);
    *(u32 *)(ctx + 48) = SRC_STRIDE;
    func_003b5fb0(hdr, N, ctx);
    for (i = 0; i < N; i++) {
        u8 *d = dstbuf + (size_t)i * 4 * S;
        for (k = 0; k < 4; k++) {
            CHECK(*(u32 *)(d + k * S) == W(i, k), "5fb0 rec %d word %d", i, k);
        }
    }
    memset(dstbuf, 0xEE, sizeof dstbuf);
    func_003b5fb0(hdr, 0, ctx);
    CHECK(dstbuf[0] == 0xEE, "5fb0 count 0 writes nothing");
}

/* ---- batch 1 (three-word copies, fills, pair fills) ---- */

static void test_rep3(void (*fn)(u8 *, s32, u8 *), const char *name)
{
    enum { N = 3, S = 32, SRC_STRIDE = 20 };
    _Alignas(16) u8 dstbuf[4 * N * S + 64] = {0};
    _Alignas(16) u8 srcbuf[N * SRC_STRIDE + 64] = {0};
    _Alignas(16) u8 hdr[8] = {0};
    _Alignas(16) u8 ctx[64] = {0};
    int i;
    int k;
    int j;

    memset(dstbuf, 0xEE, sizeof dstbuf);
    fill_src(srcbuf, SRC_STRIDE, N, 3);
    *(u32 *)(hdr + 0) = GADDR(dstbuf);
    *(u32 *)(hdr + 4) = S;
    *(u32 *)(ctx + 0x14) = GADDR(srcbuf);
    *(u32 *)(ctx + 0x18) = SRC_STRIDE;
    fn(hdr, N, ctx);
    for (i = 0; i < N; i++) {
        for (k = 0; k < 4; k++) {
            for (j = 0; j < 3; j++) {
                u32 got = *(u32 *)(dstbuf + (size_t)i * 4 * S + k * S + j * 4);
                CHECK(got == W(i, j), "%s rec %d slot %d word %d = %08x", name, i, k, j, got);
            }
            CHECK(dstbuf[(size_t)i * 4 * S + k * S + 12] == 0xEE, "%s rec %d slot %d tail untouched", name, i, k);
        }
    }
    memset(dstbuf, 0xEE, sizeof dstbuf);
    fn(hdr, 0, ctx);
    CHECK(dstbuf[0] == 0xEE, "%s count 0", name);
}

static void test_6540(void)
{
    enum { N = 2, S = 16 };
    _Alignas(16) u8 dstbuf[4 * N * S + 32] = {0};
    _Alignas(16) u8 hdr[8] = {0};
    _Alignas(16) u8 ctx[0xB0] = {0};
    int i;
    int k;

    memset(dstbuf, 0xEE, sizeof dstbuf);
    for (k = 0; k < 8; k++) {
        *(u32 *)(ctx + 0x90 + 4 * k) = 0x7000u + (u32)k;
    }
    *(u32 *)(hdr + 0) = GADDR(dstbuf);
    *(u32 *)(hdr + 4) = S;
    func_003b6540(hdr, N, ctx);
    for (i = 0; i < N; i++) {
        for (k = 0; k < 4; k++) {
            u8 *d = dstbuf + (size_t)i * 4 * S + k * S;
            CHECK(*(u32 *)d == 0x7000u + 2 * k && *(u32 *)(d + 4) == 0x7000u + 2 * k + 1, "6540 rec %d slot %d", i, k);
        }
    }
    memset(dstbuf, 0xEE, sizeof dstbuf);
    func_003b6540(hdr, 0, ctx);
    CHECK(dstbuf[0] == 0xEE, "6540 count 0");
}

static void test_fills(void)
{
    enum { N = 11, S = 16 };
    _Alignas(16) u8 dstbuf[4 * N * S + 32] = {0};
    _Alignas(16) u8 hdr[8] = {0};
    _Alignas(16) u8 ctx[0x90] = {0};
    int i;
    int k;

    *(u32 *)(hdr + 0) = GADDR(dstbuf);
    *(u32 *)(hdr + 4) = S;
    for (k = 0; k < 4; k++) {
        *(u32 *)(ctx + 0x80 + 4 * k) = 0x5100u + (u32)k;
    }
    *(u32 *)(ctx + 0x7C) = 0x6600u;

    /* Counts below, at and above the retail unroll factor (8). */
    {
        static const int counts[] = {1, 7, 8, 9, 11};
        int c;

        for (c = 0; c < 5; c++) {
            memset(dstbuf, 0xEE, sizeof dstbuf);
            func_003b6020(hdr, counts[c], ctx);
            for (i = 0; i < counts[c]; i++) {
                for (k = 0; k < 4; k++) {
                    CHECK(*(u32 *)(dstbuf + (size_t)(i * 4 + k) * S) == 0x5100u + (u32)k, "6020 n=%d rec %d slot %d", counts[c], i, k);
                }
            }
            CHECK(dstbuf[(size_t)counts[c] * 4 * S] == 0xEE, "6020 n=%d stops", counts[c]);

            memset(dstbuf, 0xEE, sizeof dstbuf);
            func_003b61e0(hdr, counts[c], ctx);
            for (i = 0; i < counts[c]; i++) {
                for (k = 0; k < 4; k++) {
                    CHECK(*(u32 *)(dstbuf + (size_t)(i * 4 + k) * S) == 0x6600u, "61e0 n=%d rec %d slot %d", counts[c], i, k);
                }
            }
            CHECK(dstbuf[(size_t)counts[c] * 4 * S] == 0xEE, "61e0 n=%d stops", counts[c]);
        }
    }
    memset(dstbuf, 0xEE, sizeof dstbuf);
    func_003b6020(hdr, 0, ctx);
    func_003b61e0(hdr, -1, ctx);
    CHECK(dstbuf[0] == 0xEE, "fills count <= 0");
}

static void test_003d0790(void)
{
    u32 flagWord = 0;
    _Alignas(16) u32 cacheBody[0x20] = {0};   /* the cache: +0x24 count, +0x2C flags (word indices 9 and 11) */
    u32 cachePtr = 0;
    _Alignas(16) u32 record[2] = {0};
    _Alignas(16) u8 obj[0x40] = {0};

    *(u32 *)(obj + 0xC) = GADDR(&flagWord);
    *(u32 *)(obj + 0x2C) = GADDR(record);
    *(u32 *)(obj + 0x30) = GADDR(&cachePtr);
    cachePtr = GADDR(cacheBody);

    /* flags clear: count = record[1]; matches; flags copy matches. */
    flagWord = 0x10;
    record[1] = 12;
    cacheBody[9] = 12;
    cacheBody[11] = 0x10;
    *(u32 *)(obj + 0x34) = 0;
    CHECK(func_003d0790(obj) == 1, "3d0790 returns 1");
    CHECK(*(u32 *)(obj + 0x34) == 0, "3d0790 consistent -> status clear");
    CHECK(*(u32 *)(obj + 0x38) == 0x10, "3d0790 flag word copied");

    /* count mismatch sets bit 3 and skips the flag copy. */
    cacheBody[9] = 13;
    *(u32 *)(obj + 0x34) = 0x100;
    *(u32 *)(obj + 0x38) = 0x77;
    CHECK(func_003d0790(obj) == 1, "3d0790 returns 1 (mismatch)");
    CHECK(*(u32 *)(obj + 0x34) == 0x108, "3d0790 count mismatch -> bit 3");
    CHECK(*(u32 *)(obj + 0x38) == 0x77, "3d0790 no copy after mismatch");

    /* flags bit 1 set: count = (record[1] - 2) * 3. */
    flagWord = 0x12;
    record[1] = 6;
    cacheBody[9] = 12;
    cacheBody[11] = 0x12;
    *(u32 *)(obj + 0x34) = 0;
    func_003d0790(obj);
    CHECK(*(u32 *)(obj + 0x34) == 0, "3d0790 (6-2)*3 == 12 matches");
    cacheBody[9] = 6;
    func_003d0790(obj);
    CHECK(*(u32 *)(obj + 0x34) == 8, "3d0790 unscaled count rejected when bit 1 set");

    /* flags differ from the cache's expected flags: bit 3. */
    cacheBody[9] = 12;
    cacheBody[11] = 0x13;
    *(u32 *)(obj + 0x34) = 0;
    func_003d0790(obj);
    CHECK(*(u32 *)(obj + 0x34) == 8, "3d0790 flag mismatch");
    CHECK(*(u32 *)(obj + 0x38) == 0x12, "3d0790 flag copy happened before the compare");

    /* Already-set bit 3 skips the flag copy and leaves the status alone. */
    cacheBody[11] = 0x12;
    *(u32 *)(obj + 0x34) = 8;
    *(u32 *)(obj + 0x38) = 0x55;
    func_003d0790(obj);
    CHECK(*(u32 *)(obj + 0x34) == 8 && *(u32 *)(obj + 0x38) == 0x55, "3d0790 bit 3 already set");
}

static u8 *free_list_seen[4];
static u8 *free_elem_seen[4];
static int free_calls;

static void fn_free(u8 *list, u8 *elem)
{
    if (free_calls < 4) {
        free_list_seen[free_calls] = list;
        free_elem_seen[free_calls] = elem;
    }
    free_calls++;
}

/* Doubly linked { next, prev } nodes, as u32 pairs. */
static void test_003cbf30(void)
{
    _Alignas(16) u32 light[32] = {0};              /* head list at +0x2C (word 11), world link at +0x34 (word 13) */
    _Alignas(16) u32 world[2] = {0};               /* sentinel of the world list */
    _Alignas(16) u32 e1[8] = {0};                  /* element: own link at words 0..1, node link at words 3..4 */
    _Alignas(16) u32 e2[8] = {0};
    _Alignas(16) u32 other[2] = {0};               /* sentinel of the list that the element links sit on */
    u32 fakeList = 0;
    u32 *head = &light[11];
    u32 *n1 = &e1[3];
    u32 *n2 = &e2[3];

    free_calls = 0;
    G32(0x008873FC) = GFN(fn_free, "vpp");
    G32(0x00764804) = 0x20;
    G32(0x00764808) = 0x10;
    G32(0x008872E4 + 0x10) = GADDR(&fakeList);
    light[8] = 0xDEAD;

    head[0] = GADDR(n1);
    head[1] = GADDR(n2);
    n1[0] = GADDR(n2);
    n1[1] = GADDR(head);
    n2[0] = GADDR(head);
    n2[1] = GADDR(n1);
    /* element links: other <-> e1 <-> e2 <-> other */
    other[0] = GADDR(e1);
    other[1] = GADDR(e2);
    e1[0] = GADDR(e2);
    e1[1] = GADDR(other);
    e2[0] = GADDR(other);
    e2[1] = GADDR(e1);
    /* world list: world <-> light link <-> world */
    light[13] = GADDR(world);
    light[14] = GADDR(world);
    world[0] = GADDR(&light[13]);
    world[1] = GADDR(&light[13]);

    CHECK(func_003cbf30((u8 *)0x1234, (u8 *)light) == (u8 *)0x1234, "cbf30 returns arg0");
    CHECK(light[8] == 0, "cbf30 clears the world pointer at the plugin offset");
    CHECK(free_calls == 2, "cbf30 frees both elements (%d)", free_calls);
    CHECK(free_elem_seen[0] == (u8 *)e1 && free_elem_seen[1] == (u8 *)e2, "cbf30 element order");
    CHECK(free_list_seen[0] == (u8 *)&fakeList && free_list_seen[1] == (u8 *)&fakeList, "cbf30 free-list pointer");
    CHECK(head[0] == GADDR(head) && head[1] == GADDR(head), "cbf30 head list emptied");
    CHECK(other[0] == GADDR(other) && other[1] == GADDR(other), "cbf30 element links unlinked");
    CHECK(world[0] == GADDR(world) && world[1] == GADDR(world), "cbf30 light unlinked from the world");

    /* An empty head list only unlinks the light. */
    free_calls = 0;
    head[0] = GADDR(head);
    head[1] = GADDR(head);
    light[13] = GADDR(world);
    light[14] = GADDR(world);
    world[0] = GADDR(&light[13]);
    world[1] = GADDR(&light[13]);
    func_003cbf30((u8 *)0, (u8 *)light);
    CHECK(free_calls == 0, "cbf30 empty list frees nothing");
    CHECK(world[0] == GADDR(world) && world[1] == GADDR(world), "cbf30 empty list still unlinks the light");
}

static u32 ref_size(u32 *table)
{
    u32 size = 0x10;
    u32 i;

    for (i = 1; i <= table[0]; i++) {
        u32 obj = table[i];
        u32 a = *(u32 *)P4_G2H(obj + 8);
        u32 b = *(u32 *)P4_G2H(obj + 0x20);

        size += a - (b - obj) + 8;
    }
    return size;
}

static void test_003ce840(void)
{
    _Alignas(16) u32 objs[12][12] = {0};
    _Alignas(16) u32 table[16] = {0};
    _Alignas(16) u8 holder[16] = {0};
    u32 tablePtrSlot = 0;
    u8 *arg;
    u32 n;
    u32 i;

    G32(0x00764820) = 4;
    /* arg0 is a block: +8 flags, +4 holds the table pointer (arg0 + 0x00764820). */
    arg = holder;
    *(u32 *)(arg + 8) = 0;
    *(u32 *)(arg + 4) = GADDR(table);
    CHECK(func_003ce840(arg) == 0, "ce840 flag clear returns 0");

    *(u32 *)(arg + 8) = 0x01000000;
    for (n = 0; n <= 11; n++) {
        table[0] = n;
        for (i = 1; i <= n; i++) {
            u32 *o = objs[i - 1];

            o[2] = 0x100u * i + 3;                 /* +8 */
            o[8] = GADDR(o) + 0x30u * i;  /* +0x20 */
            table[i] = GADDR(o);
        }
        CHECK(func_003ce840(arg) == ref_size(table), "ce840 n=%u -> %u (ref %u)", n, func_003ce840(arg), ref_size(table));
    }
    (void)tablePtrSlot;
}

static void test_0039a910(void)
{
    _Alignas(16) u8 rec[0x100] = {0};
    u32 slot = 0;
    static const u32 codes[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x20};
    static const u32 mapped[] = {5, 0, 1, 5, 5, 2, 3, 4, 5, 5, 5};
    u8 *tab = (u8 *)P4_G2H(0x0064F280u);
    int ai;
    int bi;
    int a;
    int b;
    int base;

    for (a = 0; a < 36; a++) {
        tab[a] = (u8)(0x40 + a);
    }
    G32(0x007646D0) = 0;
    slot = GADDR(rec);
    for (base = 0; base < 2; base++) {
        for (ai = 0; ai < 11; ai++) {
            for (bi = 0; bi < 11; bi++) {
                u8 *r = base ? rec + 0x40 : rec;

                memset(rec, 0, sizeof rec);
                /* *(rec+0x80) == 4 selects the first record, anything else the second. */
                *(u32 *)(rec + 0x80) = base ? 7 : 4;
                *(u32 *)(r + 4) = codes[ai];
                *(u32 *)(r + 8) = codes[bi];
                *(u32 *)(r + 0x28) = 0xEEEEEEEEu;
                *(u32 *)(r + 0x2C) = 0xEEEEEEEEu;
                func_0039a910((u8 *)&slot);
                CHECK(*(u32 *)(r + 0x28) == 0x40u + mapped[ai] + 6u * mapped[bi], "a910 base %d codes %u,%u low = %02x",
                      base, codes[ai], codes[bi], *(u32 *)(r + 0x28));
                CHECK(*(u32 *)(r + 0x2C) == 0x80, "a910 high word");
            }
        }
    }
    (void)b;
}

static void test_003a2770(void)
{
    _Alignas(16) u8 obj[0x100] = {0};
    u32 slot = 0;
    u32 out[2];
    static const struct { u32 mask; int off; } cases[] = {
        {0x001, 0x54}, {0x002, 0x74}, {0x004, 0x6C}, {0x008, 0x5C}, {0x010, 0x64},
        {0x020, 0x84}, {0x040, 0x7C}, {0x080, 0x8C}, {0x100, 0x94},
    };
    int i;

    G32(0x00764700) = 0;
    slot = GADDR(obj);
    for (i = 0; i < 9; i++) {
        memset(obj, 0, sizeof obj);
        *(u32 *)(obj + 0xA8) = 0x1FF;
        *(u32 *)(obj + cases[i].off) = 0x1111u + (u32)i;
        *(u32 *)(obj + cases[i].off + 4) = 0x2222u + (u32)i;
        out[0] = out[1] = 0xCCCCCCCCu;
        CHECK(func_003a2770((u8 *)&slot, out, cases[i].mask, 0) == 1, "2770 mask %x returns 1", cases[i].mask);
        CHECK(out[0] == 0x1111u + (u32)i && out[1] == 0x2222u + (u32)i, "2770 mask %x words %x %x", cases[i].mask, out[0], out[1]);
        CHECK(*(u32 *)(obj + 0x3C) == 0, "2770 no flags without bit 30");
    }

    /* A mask outside the object's capability mask clears the output. */
    memset(obj, 0, sizeof obj);
    *(u32 *)(obj + 0xA8) = 0x3;
    out[0] = out[1] = 0xCCCCCCCCu;
    CHECK(func_003a2770((u8 *)&slot, out, 0x4, 0) == 0 && out[0] == 0 && out[1] == 0, "2770 unsupported mask");

    /* obj[0xA4] overrides the second word; bit 30 of arg3 records the request. */
    *(u32 *)(obj + 0xA8) = 0x3;
    *(u32 *)(obj + 0x54) = 0xAB;
    *(u32 *)(obj + 0x58) = 0xCD;
    *(u32 *)(obj + 0xA4) = 0xEF;
    CHECK(func_003a2770((u8 *)&slot, out, 0x1, 0x40000000u) == 1, "2770 override returns 1");
    CHECK(out[0] == 0xAB && out[1] == 0xEF, "2770 +0xA4 replaces the second word");
    CHECK(*(u32 *)(obj + 0x3C) == 0x40000001u, "2770 obj+0x3C |= arg3 | mask (%08x)", *(u32 *)(obj + 0x3C));

    /* A multi-bit mask matches no case: the output is left alone, +0xA4 still applies. */
    *(u32 *)(obj + 0xA8) = 0x3;
    out[0] = 0x77;
    out[1] = 0x88;
    func_003a2770((u8 *)&slot, out, 0x3, 0);
    CHECK(out[0] == 0x77 && out[1] == 0xEF, "2770 multi-bit mask");
    /* A zero first word suppresses the flag update. */
    *(u32 *)(obj + 0x3C) = 0;
    out[0] = 0;
    func_003a2770((u8 *)&slot, out, 0x3, 0x40000000u);
    CHECK(*(u32 *)(obj + 0x3C) == 0, "2770 zero first word: no flag update");
}

static void test_003a5180(void)
{
    _Alignas(16) u8 obj[0x100] = {0};
    _Alignas(16) u8 rec[0x200] = {0};
    u32 slot = 0;
    int j;
    int f;

    G32(0x00764700) = 0;
    slot = GADDR(obj);
    *(u32 *)(obj + 0x9C) = GADDR(rec);

    memset(rec, 0, sizeof rec);
    CHECK(func_003a5180((u8 *)&slot) == 1, "5180 unused record returns 1");

    *(u32 *)(rec + 0xD8) = 1;
    *(u32 *)(rec + 0xE8) = 3;    /* frame count */
    *(u32 *)(rec + 0xE4) = 0;    /* index */
    for (f = 0; f < 4; f++) {
        for (j = 0; j < 9; j++) {
            *(u32 *)(rec + 0x140 + 0x24 * f + 4 * j) = 0x1000u * (u32)(f + 1) + (u32)j;
        }
        *(u32 *)(rec + 0x10 + 0x20 * f) = (f == 2) ? 0 : 5;
    }
    memset(obj, 0, 0x90);
    CHECK(func_003a5180((u8 *)&slot) == 0, "5180 advances to frame 1: flag word non-zero");
    CHECK(*(u32 *)(rec + 0xE4) == 1, "5180 index 1");
    for (j = 0; j < 3; j++) {
        CHECK(*(u32 *)(obj + 0x50 + 0x18 * j + 4) == 0x2000u + 3u * (u32)j, "5180 slot %d word 0", j);
        CHECK(*(u32 *)(obj + 0x50 + 0x18 * j + 0xC) == 0x2000u + 3u * (u32)j + 1, "5180 slot %d word 1", j);
        CHECK(*(u32 *)(obj + 0x50 + 0x18 * j + 0x14) == 0x2000u + 3u * (u32)j + 2, "5180 slot %d word 2", j);
    }
    CHECK(func_003a5180((u8 *)&slot) == 1, "5180 frame 2: flag word zero");
    CHECK(*(u32 *)(rec + 0xE4) == 2, "5180 index 2");
    CHECK(func_003a5180((u8 *)&slot) == 0, "5180 index 3: flag non-zero");
    CHECK(*(u32 *)(rec + 0xE4) == 3, "5180 index 3");
    func_003a5180((u8 *)&slot);
    CHECK(*(u32 *)(rec + 0xE4) == 0, "5180 index wraps to 0 once it reaches the count");
    /* A zero frame count leaves the index at 0 and copies nothing. */
    *(u32 *)(rec + 0xE8) = 0;
    memset(obj, 0xEE, 0x90);
    func_003a5180((u8 *)&slot);
    CHECK(obj[0x54] == 0xEE, "5180 zero frame count copies nothing");
}

/* Corner-position records. Reference deltas are formed from the definitions in each override's
 * header with the same single-precision operation order. */
typedef struct { f32 x, y, z; } V3;

static int near_bits(f32 a, f32 b)
{
    return memcmp(&a, &b, 4) == 0;
}

static void check_corner(const char *name, int rec, int corner, const f32 *got, V3 want)
{
    CHECK(near_bits(got[0], want.x) && near_bits(got[1], want.y) && near_bits(got[2], want.z),
          "%s rec %d corner %d got (%g %g %g) want (%g %g %g)", name, rec, corner,
          got[0], got[1], got[2], want.x, want.y, want.z);
}

static void test_corners(void)
{
    enum { N = 3, S = 48, PSTRIDE = 32 };
    _Alignas(16) u8 dst[4 * N * S + 64] = {0};
    _Alignas(16) u8 hdr[8] = {0};
    _Alignas(16) u8 ctx[0x100] = {0};
    _Alignas(16) u8 pos[N * PSTRIDE + 64] = {0};
    _Alignas(16) u8 ext[N * 24 + 64] = {0};
    _Alignas(16) f32 a[3] = {0.75f, -1.5f, 2.25f};
    _Alignas(16) f32 b[3] = {0.5f, 3.0f, -0.125f};
    int i;
    int k;

    *(u32 *)(hdr + 0) = GADDR(dst);
    *(u32 *)(hdr + 4) = S;
    for (i = 0; i < N; i++) {
        f32 *p = (f32 *)(pos + i * PSTRIDE);

        p[0] = 10.0f * (f32)(i + 1) + 0.25f;
        p[1] = -3.0f * (f32)(i + 1);
        p[2] = 0.5f * (f32)(i + 2);
        ((f32 *)(ext + i * 24))[0] = 2.0f + (f32)i;
        ((f32 *)(ext + i * 24))[1] = 4.0f - 0.5f * (f32)i;
    }
    memset(ctx, 0, sizeof ctx);
    *(u32 *)(ctx + 4) = GADDR(pos);
    *(u32 *)(ctx + 8) = PSTRIDE;
    *(f32 *)(ctx + 0x68) = 1.5f;
    *(f32 *)(ctx + 0x6C) = -0.25f;
    *(f32 *)(ctx + 0x70) = 3.0f;
    *(f32 *)(ctx + 0x74) = 5.0f;
    *(u32 *)(ctx + 0x1C) = GADDR(ext);
    *(u32 *)(ctx + 0x20) = 24;

    /* 4ab0: A = a*(0.5*s), B = b*(0.5*t); P + (-A-B), (A-B), (A+B), (-A+B). */
    memset(dst, 0xEE, sizeof dst);
    func_003b4ab0(hdr, a, b, N, ctx);
    for (i = 0; i < N; i++) {
        f32 *p = (f32 *)(pos + i * PSTRIDE);
        f32 sa = 0.5f * 3.0f;
        f32 sb = 0.5f * 5.0f;
        V3 A = {a[0] * sa, a[1] * sa, a[2] * sa};
        V3 B = {b[0] * sb, b[1] * sb, b[2] * sb};
        u8 *d = dst + (size_t)i * 4 * S;

        check_corner("4ab0", i, 0, (f32 *)(d + 0 * S), (V3){p[0] + (-A.x - B.x), p[1] + (-A.y - B.y), p[2] + (-A.z - B.z)});
        check_corner("4ab0", i, 1, (f32 *)(d + 1 * S), (V3){p[0] + (A.x - B.x), p[1] + (A.y - B.y), p[2] + (A.z - B.z)});
        check_corner("4ab0", i, 2, (f32 *)(d + 2 * S), (V3){p[0] + (A.x + B.x), p[1] + (A.y + B.y), p[2] + (A.z + B.z)});
        check_corner("4ab0", i, 3, (f32 *)(d + 3 * S), (V3){p[0] + (-A.x + B.x), p[1] + (-A.y + B.y), p[2] + (-A.z + B.z)});
    }
    CHECK(dst[(size_t)N * 4 * S] == 0xEE, "4ab0 stops after N records");
    memset(dst, 0xEE, sizeof dst);
    func_003b4ab0(hdr, a, b, 0, ctx);
    CHECK(dst[0] == 0xEE, "4ab0 count 0");

    /* 4c10: B scaled by the same h, offset o = c6C*b + c68*a added first. */
    memset(dst, 0xEE, sizeof dst);
    func_003b4c10(hdr, a, b, N, ctx);
    for (i = 0; i < N; i++) {
        f32 *p = (f32 *)(pos + i * PSTRIDE);
        f32 h = 0.5f * 3.0f;
        V3 o = {-0.25f * b[0] + 1.5f * a[0], -0.25f * b[1] + 1.5f * a[1], -0.25f * b[2] + 1.5f * a[2]};
        V3 A = {a[0] * h, a[1] * h, a[2] * h};
        V3 B = {b[0] * h, b[1] * h, b[2] * h};
        u8 *d = dst + (size_t)i * 4 * S;

        check_corner("4c10", i, 0, (f32 *)(d + 0 * S), (V3){p[0] + (o.x + (-A.x - B.x)), p[1] + (o.y + (-A.y - B.y)), p[2] + (o.z + (-A.z - B.z))});
        check_corner("4c10", i, 1, (f32 *)(d + 1 * S), (V3){p[0] + (o.x + (A.x - B.x)), p[1] + (o.y + (A.y - B.y)), p[2] + (o.z + (A.z - B.z))});
        check_corner("4c10", i, 2, (f32 *)(d + 2 * S), (V3){p[0] + (o.x + (A.x + B.x)), p[1] + (o.y + (A.y + B.y)), p[2] + (o.z + (A.z + B.z))});
        check_corner("4c10", i, 3, (f32 *)(d + 3 * S), (V3){p[0] + (o.x + (-A.x + B.x)), p[1] + (o.y + (-A.y + B.y)), p[2] + (o.z + (-A.z + B.z))});
    }

    /* 4ff0 and 5650: the half-extents come from the per-record stream. */
    memset(dst, 0xEE, sizeof dst);
    func_003b4ff0(hdr, a, b, N, ctx);
    for (i = 0; i < N; i++) {
        f32 *p = (f32 *)(pos + i * PSTRIDE);
        f32 h1 = 0.5f * ((f32 *)(ext + i * 24))[0];
        f32 h2 = 0.5f * ((f32 *)(ext + i * 24))[1];
        V3 A = {a[0] * h1, a[1] * h1, a[2] * h1};
        V3 B = {b[0] * h2, b[1] * h2, b[2] * h2};
        u8 *d = dst + (size_t)i * 4 * S;

        check_corner("4ff0", i, 0, (f32 *)(d + 0 * S), (V3){p[0] + (-A.x - B.x), p[1] + (-A.y - B.y), p[2] + (-A.z - B.z)});
        check_corner("4ff0", i, 1, (f32 *)(d + 1 * S), (V3){p[0] + (A.x - B.x), p[1] + (A.y - B.y), p[2] + (A.z - B.z)});
        check_corner("4ff0", i, 2, (f32 *)(d + 2 * S), (V3){p[0] + (A.x + B.x), p[1] + (A.y + B.y), p[2] + (A.z + B.z)});
        check_corner("4ff0", i, 3, (f32 *)(d + 3 * S), (V3){p[0] + (-A.x + B.x), p[1] + (-A.y + B.y), p[2] + (-A.z + B.z)});
    }
    memset(dst, 0xEE, sizeof dst);
    func_003b5650(hdr, a, b, N, ctx);
    for (i = 0; i < N; i++) {
        f32 *p = (f32 *)(pos + i * PSTRIDE);
        f32 h1 = 0.5f * ((f32 *)(ext + i * 24))[0];
        f32 h2 = 0.5f * ((f32 *)(ext + i * 24))[1];
        V3 o = {-0.25f * b[0] + 1.5f * a[0], -0.25f * b[1] + 1.5f * a[1], -0.25f * b[2] + 1.5f * a[2]};
        V3 A = {a[0] * h1, a[1] * h1, a[2] * h1};
        V3 B = {b[0] * h2, b[1] * h2, b[2] * h2};
        u8 *d = dst + (size_t)i * 4 * S;

        check_corner("5650", i, 0, (f32 *)(d + 0 * S), (V3){(p[0] + o.x) + (-A.x - B.x), (p[1] + o.y) + (-A.y - B.y), (p[2] + o.z) + (-A.z - B.z)});
        check_corner("5650", i, 1, (f32 *)(d + 1 * S), (V3){(p[0] + o.x) + (A.x - B.x), (p[1] + o.y) + (A.y - B.y), (p[2] + o.z) + (A.z - B.z)});
        check_corner("5650", i, 2, (f32 *)(d + 2 * S), (V3){(p[0] + o.x) + (A.x + B.x), (p[1] + o.y) + (A.y + B.y), (p[2] + o.z) + (A.z + B.z)});
        check_corner("5650", i, 3, (f32 *)(d + 3 * S), (V3){(p[0] + o.x) + (-A.x + B.x), (p[1] + o.y) + (-A.y + B.y), (p[2] + o.z) + (-A.z + B.z)});
    }

    /* 5ac0 and 5bf0 read 0x3C-byte-or-larger records through ctx+0x0C / +0x10. */
    {
        enum { RSTRIDE = 96 };
        _Alignas(16) u8 rec[N * RSTRIDE + 64] = {0};

        for (i = 0; i < N; i++) {
            f32 *r = (f32 *)(rec + i * RSTRIDE);

            for (k = 0; k < 16; k++) {
                r[k] = 0.25f * (f32)(k + 1) * (f32)(i + 1) - 1.0f;
            }
        }
        *(u32 *)(ctx + 0xC) = GADDR(rec);
        *(u32 *)(ctx + 0x10) = RSTRIDE;

        memset(dst, 0xEE, sizeof dst);
        func_003b5ac0(hdr, a, b, N, ctx);
        for (i = 0; i < N; i++) {
            f32 *r = (f32 *)(rec + i * RSTRIDE);
            f32 *p = r + 12;
            f32 ra = 0.5f * r[0];
            f32 rb = 0.5f * r[4];
            f32 rc = 0.5f * r[2];
            f32 rd = 0.5f * r[1];
            f32 re = 0.5f * r[5];
            f32 rf = 0.5f * r[6];
            V3 D = {ra - rb, rd - re, rc - rf};
            V3 Q = {ra + rb, rd + re, rc + rf};
            u8 *d = dst + (size_t)i * 4 * S;

            check_corner("5ac0", i, 0, (f32 *)(d + 0 * S), (V3){p[0] + D.x, p[1] + D.y, p[2] + D.z});
            check_corner("5ac0", i, 1, (f32 *)(d + 1 * S), (V3){p[0] - Q.x, p[1] - Q.y, p[2] - Q.z});
            check_corner("5ac0", i, 2, (f32 *)(d + 2 * S), (V3){p[0] - D.x, p[1] - D.y, p[2] - D.z});
            check_corner("5ac0", i, 3, (f32 *)(d + 3 * S), (V3){p[0] + Q.x, p[1] + Q.y, p[2] + Q.z});
        }

        memset(dst, 0xEE, sizeof dst);
        func_003b5bf0(hdr, a, b, N, ctx);
        for (i = 0; i < N; i++) {
            f32 *r = (f32 *)(rec + i * RSTRIDE);
            f32 a0 = 0.5f * (r[0] - r[4]);
            f32 b0 = 0.5f * (r[0] + r[4]);
            f32 a1 = 0.5f * (r[1] - r[5]);
            f32 b1 = 0.5f * (r[1] + r[5]);
            f32 a2 = 0.5f * (r[2] - r[6]);
            f32 c2 = 0.5f * (r[2] + r[6]);
            f32 o0 = ((0.0f + r[12]) + r[0] * 1.5f) + r[4] * -0.25f;
            f32 o1 = ((0.0f + r[13]) + r[1] * 1.5f) + r[5] * -0.25f;
            f32 o2 = ((0.0f + r[14]) + r[2] * 1.5f) + r[6] * -0.25f;
            u8 *d = dst + (size_t)i * 4 * S;

            check_corner("5bf0", i, 0, (f32 *)(d + 0 * S), (V3){o0 + a0, o1 + a1, o2 + a2});
            check_corner("5bf0", i, 1, (f32 *)(d + 1 * S), (V3){o0 - b0, o1 - b1, o2 - c2});
            check_corner("5bf0", i, 2, (f32 *)(d + 2 * S), (V3){o0 - a0, o1 - a1, o2 - a2});
            check_corner("5bf0", i, 3, (f32 *)(d + 3 * S), (V3){o0 + b0, o1 + b1, o2 + c2});
        }
    }
}

static int run_test(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    map_fixed(0x00764000);
    map_fixed(0x00887000);
    map_fixed(0x0064F000);

    test_001f5680();
    test_00396e80();
    test_003cc250();
    test_003d5710();
    test_003b5fb0();
    test_strided_copies();
    test_rep3(func_003b4230, "4230");
    test_rep3(func_003b65d0, "65d0");
    test_6540();
    test_fills();
    test_003d0790();
    test_003cbf30();
    test_003ce840();
    test_0039a910();
    test_003a2770();
    test_003a5180();
    test_corners();

    printf("promoted_leaf: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(run_test, argc, argv);
}
