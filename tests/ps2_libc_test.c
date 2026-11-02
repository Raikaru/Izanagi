#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "guest_test.h"
#include "p4pc/ps2_libc.h"
#include "p4pc/hostmath.h"
#include "ps2_fpbit.h"

/* Exercise the same named bindings as the game, not a separate host fallback. */
#undef sinf
#undef cosf
#undef tanf
#undef atanf
#undef sin
#undef cos
#undef atan2
#undef pow
#undef log
#include "p4pc/libc_rename.h"

static int failures;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)

typedef void *(*ByteCopyEntry)(void *, const void *, uint32_t);
typedef char *(*StringCopyEntry)(char *, const char *, uint32_t);
_Static_assert(_Generic(&memcpy, ByteCopyEntry: 1, default: 0), "EE memcpy count is uint32");
_Static_assert(_Generic(&memmove, ByteCopyEntry: 1, default: 0), "EE memmove count is uint32");
_Static_assert(_Generic(&strncpy, StringCopyEntry: 1, default: 0), "EE strncpy count is uint32");
_Static_assert(_Generic(&strncat, StringCopyEntry: 1, default: 0), "EE strncat count is uint32");
_Static_assert(_Generic(&strlen, uint32_t (*)(const char *): 1, default: 0), "EE strlen result is uint32");
_Static_assert(_Generic(&strncmp, int (*)(const char *, const char *, uint32_t): 1, default: 0), "EE strncmp count is uint32");
_Static_assert(_Generic(&memcmp, int (*)(const void *, const void *, uint32_t): 1, default: 0), "EE memcmp count is uint32");

static void byte_boundaries(void)
{
    ByteCopyEntry volatile copy = memcpy, move = memmove;
    uint8_t input[64], output[64];
    char text[16];
    for (uint32_t count = 0; count <= sizeof input; ++count) {
        for (uint32_t index = 0; index < sizeof input; ++index) {
            input[index] = (uint8_t)(index * 37u + 11u);
            output[index] = 0xcc;
        }
        CHECK(copy(output, input, count) == output);
        for (uint32_t index = 0; index < sizeof output; ++index)
            CHECK(output[index] == (index < count ? input[index] : 0xcc));
    }
    for (uint32_t index = 0; index < sizeof input; ++index)
        input[index] = (uint8_t)index;
    CHECK(move(input + 1, input, 63) == input + 1);
    for (uint32_t index = 1; index < sizeof input; ++index)
        CHECK(input[index] == index - 1);
    for (uint32_t index = 0; index < sizeof input; ++index)
        input[index] = (uint8_t)index;
    CHECK(move(input, input + 1, 63) == input);
    for (uint32_t index = 0; index < 63; ++index)
        CHECK(input[index] == index + 1);
    CHECK(memset(output, -1, sizeof output) == output);
    for (uint32_t index = 0; index < sizeof output; ++index)
        CHECK(output[index] == 0xff);
    memset(text, 0x55, sizeof text);
    CHECK(strncpy(text, "abc", 0) == text && text[0] == 0x55);
    CHECK(strncpy(text, "abc", 2) == text && text[0] == 'a' && text[1] == 'b' && text[2] == 0x55);
    CHECK(strncpy(text, "abc", 6) == text);
    CHECK(text[0] == 'a' && text[1] == 'b' && text[2] == 'c');
    CHECK(text[3] == 0 && text[4] == 0 && text[5] == 0 && text[6] == 0x55);
    text[0] = 'X';
    text[1] = 0;
    CHECK(strncat(text, "abc", 2) == text && strcmp(text, "Xab") == 0);
    CHECK(strncat(text, "ignored", 0) == text && strcmp(text, "Xab") == 0);
    CHECK(strlen("\xff\x80") == 2u);
    CHECK(strncmp("\xff", "", UINT32_MAX) == 255);
}

struct NumberCase {
    const char *text;
    int base;
    uint64_t unsigned_value, signed_bits;
    unsigned consumed;
    int unsigned_overflow, signed_overflow;
};

static void conversions(char *text, uint32_t *error)
{
    static const struct NumberCase cases[] = {
        {"", 10, 0, 0, 0, 0, 0},
        {" \t\n+42tail", 10, 42, 42, 6, 0, 0},
        {"0", 0, 0, 0, 1, 0, 0},
        {"-0", 10, 0, 0, 2, 0, 0},
        {"-1!", 10, UINT64_MAX, UINT64_MAX, 2, 0, 0},
        {"+2147483648", 10, UINT64_C(2147483648), UINT64_C(2147483648), 11, 0, 0},
        {"4294967296", 10, UINT64_C(4294967296), UINT64_C(4294967296), 10, 0, 0},
        {"ffffffffffffffff", 16, UINT64_MAX, INT64_MAX, 16, 0, 1},
        {"80000000", 16, UINT64_C(2147483648), UINT64_C(2147483648), 8, 0, 0},
        {"100000000", 16, UINT64_C(4294967296), UINT64_C(4294967296), 9, 0, 0},
        {"9223372036854775807", 10, INT64_MAX, INT64_MAX, 19, 0, 0},
        {"9223372036854775808", 10, UINT64_C(1) << 63, INT64_MAX, 19, 0, 1},
        {"-9223372036854775808", 10, UINT64_C(1) << 63, UINT64_C(1) << 63, 20, 0, 0},
        {"-9223372036854775809", 10, INT64_MAX, UINT64_C(1) << 63, 20, 0, 1},
        {"18446744073709551615", 10, UINT64_MAX, INT64_MAX, 20, 0, 1},
        {"18446744073709551616tail", 10, UINT64_MAX, INT64_MAX, 20, 1, 1},
        {"-18446744073709551615", 10, 1, UINT64_C(1) << 63, 21, 0, 1},
        {"-18446744073709551616", 10, UINT64_MAX, UINT64_C(1) << 63, 21, 1, 1},
        {"1844674407370955161600009x", 10, UINT64_MAX, INT64_MAX, 25, 1, 1},
        {"0x123tail", 0, 291, 291, 5, 0, 0},
        {"0Xff!", 16, 255, 255, 4, 0, 0},
        {"0779", 0, 63, 63, 3, 0, 0},
        {"0b11", 0, 0, 0, 1, 0, 0},
        {"0x", 0, 0, 0, 0, 0, 0},
        {"+", 10, 0, 0, 0, 0, 0},
        {" \r-xyz", 10, 0, 0, 0, 0, 0},
        {"z", 36, 35, 35, 1, 0, 0},
        {"10", 2, 2, 2, 2, 0, 0},
        {"00x", 1, 0, 0, 2, 0, 0},
        {"z", 37, 35, 35, 1, 0, 0},
        {"123", -1, 0, 0, 0, 0, 0},
        {"\x80" "1", 10, 0, 0, 0, 0, 0},
        {"1_2", 10, 1, 1, 1, 0, 0}
    };
    struct EndSlot { uint32_t before, end, after; } *slot = p4t_galloc(sizeof *slot, 4);
    CHECK(slot != NULL);
    if (slot == NULL)
        return;
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        const struct NumberCase *c = &cases[i];
        strcpy(text, c->text);
        for (int signed_mode = 0; signed_mode < 2; signed_mode++) {
            uint64_t value, expected = signed_mode ? c->signed_bits : c->unsigned_value;
            int overflow = signed_mode ? c->signed_overflow : c->unsigned_overflow;
            slot->before = UINT32_C(0x13579bdf);
            slot->end = 0;
            slot->after = UINT32_C(0x2468ace0);
            *error = 123;
            value = signed_mode ? (uint64_t)func_00444210(text, &slot->end, c->base)
                                : func_00444458(text, &slot->end, c->base);
            if (value != expected) {
                fprintf(stderr, "conversion case %u signed=%d: %016" PRIx64 " != %016" PRIx64 "\n", i, signed_mode, value, expected);
                failures++;
            }
            CHECK(slot->end == P4_H2G(text + c->consumed));
            CHECK(slot->before == UINT32_C(0x13579bdf));
            CHECK(slot->after == UINT32_C(0x2468ace0));
            CHECK(*error == (overflow ? 34u : 123u));
        }
    }
    strcpy(text, "0xffffffffffffffff");
    CHECK(func_00444458(text, NULL, 0) == UINT64_MAX);
}

static void atoi_width(char *text)
{
    static const struct { const char *text; int32_t value; } cases[] = {
        {" \t+12tail", 12}, {"2147483647", INT32_MAX}, {"2147483648", INT32_MIN},
        {"4294967295", -1}, {"4294967296", 0}, {"-2147483649", INT32_MAX},
        {"-4294967295", 1}, {"9223372036854775807", -1}, {"-9223372036854775808", 0},
        {"9223372036854775808", -1}, {"-9223372036854775809", 0}, {"0x10", 0}, {"none", 0}
    };
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        strcpy(text, cases[i].text);
        CHECK(atoi(text) == cases[i].value);
    }
}

static void comparisons(unsigned char *storage)
{
    CHECK(strncmp(NULL, NULL, 0) == 0);
    CHECK(memcmp(NULL, NULL, 0) == 0);
    for (unsigned offset = 0; offset < 16; offset++) {
        char *a = (char *)storage + offset;
        char *b = (char *)storage + 128 + offset;
        memset(a, 'q', 80);
        memset(b, 'q', 80);
        a[64] = (char)0xfe;
        b[64] = 1;
        a[65] = b[65] = 0;
        CHECK(strcmp(a, b) == 253);
        CHECK(strcmp(b, a) == -253);
        CHECK(strncmp(a, b, 64) == 0);
        CHECK(strncmp(a, b, 65) == 253);
        CHECK(strncmp(b, a, 65) == -253);
        CHECK(memcmp(a, b, 64) == 0);
        CHECK(memcmp(a, b, 65) == 253);
        CHECK(memcmp(b, a, 65) == -253);
        a[1] = b[1] = 0;
        a[2] = 'B';
        b[2] = 'A';
        CHECK(strcmp(a, b) == 0);
        CHECK(strncmp(a, b, 80) == 0);
        CHECK(memcmp(a, b, 3) == 1);
    }
}

static void hex_scan_width(void)
{
    static const struct { const char *text; unsigned long long value; } cases[] = {
        {"0", 0}, {"ffffffff", UINT64_C(0xffffffff)},
        {"100000000", UINT64_C(0x100000000)},
        {"ffffffffffffffff", UINT64_MAX}, {"-1", UINT64_MAX},
        {"10000000000000000", UINT64_MAX}
    };
    struct ScanSlot {
        uint64_t before;
        unsigned long long value;
        uint64_t after;
    } slot[3];
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        slot[0].before = UINT64_C(0x13579bdf2468ace0);
        slot[0].value = UINT64_C(0xaaaaaaaaaaaaaaaa);
        slot[0].after = UINT64_C(0x2468ace013579bdf);
        CHECK(sscanf(cases[i].text, "%llx", &slot[0].value) == 1);
        CHECK(slot[0].value == cases[i].value);
        CHECK(slot[0].before == UINT64_C(0x13579bdf2468ace0));
        CHECK(slot[0].after == UINT64_C(0x2468ace013579bdf));
    }
    for (unsigned i = 0; i < 3; i++) {
        slot[i].before = UINT64_C(0x13579bdf2468ace0);
        slot[i].value = UINT64_C(0xaaaaaaaaaaaaaaaa);
        slot[i].after = UINT64_C(0x2468ace013579bdf);
    }
    CHECK(sscanf("100000001 12345678abcdef01 ffffffff", "%llx %llx %llx",
                 &slot[0].value, &slot[1].value, &slot[2].value) == 3);
    CHECK((uint32_t)slot[0].value == 1);
    CHECK((uint32_t)slot[1].value == UINT32_C(0xabcdef01));
    CHECK((uint32_t)slot[2].value == UINT32_MAX);
    for (unsigned i = 0; i < 3; i++) {
        CHECK(slot[i].before == UINT64_C(0x13579bdf2468ace0));
        CHECK(slot[i].after == UINT64_C(0x2468ace013579bdf));
    }
}

static int binding_vsprintf(char *buffer, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n=func_00446ed8(buffer, fmt, ap);
    va_end(ap);
    return n;
}

static int binding_vprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n=func_00446e88(fmt, ap);
    va_end(ap);
    return n;
}

static void format_bindings(void)
{
    char buffer[128];
    const char *expected="retail -7 1.38";
    CHECK(func_00442088(buffer, "%s %d %.2f", "retail", -7, 1.375)==(int)strlen(expected));
    CHECK(strcmp(buffer, expected)==0);
    expected="-0000012 gue 1.25e-01";
    CHECK(binding_vsprintf(buffer, "%+08d %.3s %.2e", -12, "guest", 0.125)==(int)strlen(expected));
    CHECK(strcmp(buffer, expected)==0);
    CHECK(func_00440b68("retail printf: %.2f\n", 1.375)==(int)strlen("retail printf: 1.38\n"));
    CHECK(func_004244c8("retail scePrintf: %.2f\n", 1.375)==(int)strlen("retail scePrintf: 1.38\n"));
    CHECK(binding_vprintf("retail vprintf: %.2f\n", 1.375)==(int)strlen("retail vprintf: 1.38\n"));
    memset(buffer, 0xa5, sizeof buffer);
    CHECK(p4pc_ee_snprintf(buffer, 0, "%dx%d", 640, 448) == 7);
    CHECK((unsigned char)buffer[0] == 0xa5);
    CHECK(p4pc_ee_snprintf(buffer, 1, "%dx%d", 640, 448) == 7);
    CHECK(buffer[0] == 0 && (unsigned char)buffer[1] == 0xa5);
    CHECK(p4pc_ee_snprintf(buffer, 6, "%dx%d", 640, 448) == 7);
    CHECK(strcmp(buffer, "640x4") == 0);
    CHECK(p4pc_ee_snprintf(buffer, sizeof buffer, "%dx%d", INT32_MIN, INT32_MAX) == 22);
    CHECK(strcmp(buffer, "-2147483648x2147483647") == 0);
    CHECK(p4pc_ee_snprintf(buffer, sizeof buffer, "scale:%d", 8) == 7);
    CHECK(strcmp(buffer, "scale:8") == 0);
}


#ifdef P4_GUEST64
/* These are the same private declarations emitted by the 64-bit call rewriter. */
extern int p4pf_sprintf(char *buffer, const char *format, ...);

static void soft_double_format(char *text)
{
    static const char *formats[] = {"%f", "%4.2f", "_%4.2f"};
    static const struct { float value; const char *result[3]; } cases[] = {
        {0.0f, {"0.000000", "0.00", "_0.00"}},
        {1.0f, {"1.000000", "1.00", "_1.00"}},
        {-1.0f, {"-1.000000", "-1.00", "_-1.00"}},
        {1.375f, {"1.375000", "1.38", "_1.38"}},
        {123.456f, {"123.456001", "123.46", "_123.46"}},
        {10000.125f, {"10000.125000", "10000.12", "_10000.12"}}
    };
    char buffer[128];
    uint64_t bits;
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        double value;
        bits = fptodp(cases[i].value);
        __builtin_memcpy(&value, &bits, sizeof value);
        for (unsigned f = 0; f < sizeof formats / sizeof formats[0]; f++) {
            p4pf_sprintf(buffer, formats[f], 0u, value);
            CHECK(strcmp(buffer, cases[i].result[f]) == 0);
            /* The next call supplies no FPR argument: stale save slots must not
             * replace the actual integer-class soft-double payload. */
            p4pf_sprintf(buffer, "%f", 0u, -999.0);
            p4pf_sprintf(buffer, formats[f], 1u, bits);
            CHECK(strcmp(buffer, cases[i].result[f]) == 0);
        }
    }
    /* Colored debug forwards an unmasked native va_list: preserve retail
     * fptodp bits but move them through the native double argument class. */
    static const uint32_t promotion_edges[] = {
        0x00000000u, 0x80000000u, 0x00000001u, 0x80000001u,
        0x007fffffu, 0x807fffffu, 0x00800000u, 0x80800000u,
        0x7f7fffffu, 0xff7fffffu, 0x7f800000u, 0xff800000u,
        0x7fa12345u, 0xffa12345u, 0x7fc12345u, 0xffc12345u
    };
    static const char *edge_formats[] = {"%f", "%e", "%g"};
    for (unsigned i = 0; i < sizeof promotion_edges / sizeof promotion_edges[0]; i++) {
        float input;
        double promoted;
        char wanted[128];
        __builtin_memcpy(&input, &promotion_edges[i], sizeof input);
        bits = fptodp(input);
        __builtin_memcpy(&promoted, &bits, sizeof promoted);
        for (unsigned f = 0; f < sizeof edge_formats / sizeof edge_formats[0]; f++) {
            p4pf_sprintf(buffer, "%f", 0u, -999.0);
            int count = p4pf_sprintf(wanted, edge_formats[f], 1u, bits);
            CHECK(binding_vsprintf(buffer, edge_formats[f], promoted) == count);
            CHECK(strcmp(buffer, wanted) == 0);
        }
    }
    strcpy(text, "guest");
    {
        double value = 1.375;
        memcpy(&bits, &value, sizeof bits);
    }
    p4pf_sprintf(buffer, "%s %4.2f %d %4.2f", 7u,
                  (uint64_t)P4_H2G(text), bits, (uint64_t)(int64_t)-7, 2.25);
    CHECK(strcmp(buffer, "guest 1.38 -7 2.25") == 0);
}
#endif

static void allocation_bindings(void)
{
    static const uint32_t products[][2] = {
        {0, UINT32_MAX}, {UINT32_MAX, 0}, {UINT32_C(0x80000001), 2},
        {UINT32_MAX, UINT32_MAX}, {UINT32_C(0x40000000), 4}
    };
    uint8_t *p = calloc(8, 4), *q;
    CHECK(p != NULL);
    if (p == NULL)
        return;
    CHECK(P4_H2G(p) != 0);
    for (unsigned i = 0; i < 32; i++) {
        CHECK(p[i] == 0);
        p[i] = (uint8_t)(i + 1);
    }
    q = realloc(p, UINT32_C(0x01000000));
    CHECK(q == NULL);
    for (unsigned i = 0; i < 32; i++)
        CHECK(p[i] == (uint8_t)(i + 1));
    q = realloc(p, 64);
    CHECK(q != NULL);
    if (q == NULL) {
        free(p);
        return;
    }
    p = q;
    for (unsigned i = 0; i < 32; i++)
        CHECK(p[i] == (uint8_t)(i + 1));
    q = realloc(p, 8);
    CHECK(q != NULL);
    if (q == NULL) {
        free(p);
        return;
    }
    p = q;
    for (unsigned i = 0; i < 8; i++)
        CHECK(p[i] == (uint8_t)(i + 1));
    q = realloc(p, 0);
    CHECK(q != NULL); /* Retail normalizes to a minimum chunk, not free. */
    free(q != NULL ? q : p);
    p = realloc(NULL, 4);
    CHECK(p != NULL);
    free(p);
    for (unsigned i = 0; i < sizeof products / sizeof products[0]; i++) {
        uint32_t count = products[i][0], size = products[i][1], bytes = count * size;
        p = calloc(count, size);
        CHECK(p != NULL);
        if (p == NULL)
            continue;
        for (uint32_t j = 0; j < bytes; j++)
            CHECK(p[j] == 0);
        free(p);
    }
}

int main(void)
{
    uint32_t *error;
    char *text;
    unsigned char *storage;
    if (p4t_guest_map(0x0070c000u, 0x6000) != 0 ||
        p4t_guest_map(0x00754000u, 8192) != 0 ||
        p4t_guest_map(0x00938000u, 4096) != 0)
        return 77;
    *(uint32_t *)P4_G2H(0x00710070u) = 0x00710500u;
    if (p4t_heap_init(1u << 20) != 0)
        return 1;
    strcpy(P4_G2H(0x00754e88u), "Infinity");
    strcpy(P4_G2H(0x00754e98u), "NaN");
    strcpy(P4_G2H(0x00754ea0u), "0");
    /* This no-ELF unit fixture supplies mathematical constants at their retail
     * addresses; the original-ELF differentials load the real tables instead. */
    static const double tens[]={
        1e0,1e1,1e2,1e3,1e4,1e5,1e6,1e7,1e8,1e9,
        1e10,1e11,1e12,1e13,1e14,1e15,1e16,1e17,1e18,1e19,
        1e20,1e21,1e22,1e23,1e24
    };
    static const double bigtens[]={1e16,1e32,1e64,1e128,1e256};
    static const double tinytens[]={1e-16,1e-32,1e-64,1e-128,1e-256};
    memcpy(P4_G2H(0x00754f38u),tens,sizeof tens);
    memcpy(P4_G2H(0x00755000u),bigtens,sizeof bigtens);
    memcpy(P4_G2H(0x00755028u),tinytens,sizeof tinytens);
    error = p4t_galloc(0x100, 8);
    text = p4t_galloc(128, 8);
    storage = p4t_galloc(256, 16);
    if (error == NULL || text == NULL || storage == NULL)
        return 1;
    *(uint32_t *)P4_G2H(0x00710070u) = P4_H2G(error);
    conversions(text, error);
    atoi_width(text);
    byte_boundaries();
    comparisons(storage);
    allocation_bindings();
    hex_scan_width();
    format_bindings();
#ifdef P4_GUEST64
    soft_double_format(text);
#endif
    p4pc_fpu_ee_mode();
    format_bindings();
#ifdef P4_GUEST64
    soft_double_format(text);
#endif
    if (failures != 0)
        return 1;
    puts("retail CRT: 64-bit conversions, EE-width byte callbacks, 32-bit atoi/end slots, errno, comparisons and allocator boundaries passed");
#ifdef P4_GUEST64
    puts("retail CRT: integer-class soft-double formatting passed");
#endif
    return 0;
}
