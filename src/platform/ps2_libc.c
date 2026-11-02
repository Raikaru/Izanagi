/*
 * Native bindings for PS2 library functions that the decomp calls by address.
 *
 * Both the caller and this definition are compiled natively: a pointer argument is a host pointer into
 * guest memory (the register ABI of docs/port64.md), so byte transports use it directly. A pointer
 * the function stores into guest memory (an integer conversion's end pointer) is stored as a guest address.
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/ps2_libc.h"
#include "p4pc/guest.h"
#include "ee_stdio.h"

/* func_00440b68 is printf (375 direct retail call sites). Output goes to stdout and is flushed
 * on every call, so the boot messages appear before any later trap or abort. */
int func_00440b68(const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = p4pc_ee_vfprintf(stdout, fmt, 0, ap);
    va_end(ap);
    fflush(stdout);
    return n;
}

/* func_004244c8 is scePrintf (libkernl): formatted output to the EE console, routed to the
 * host stdout like func_00440b68. */
int func_004244c8(const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = p4pc_ee_vfprintf(stdout, fmt, 0, ap);
    va_end(ap);
    fflush(stdout);
    return n;
}


/* 00443f38: bytewise tokenizer. The save slot is a four-byte guest pointer.
 * 00443f18 selects the active newlib reent's +0x5c slot, not host TLS/static
 * state; delimiter changes and exhausted continuations follow the retail loop. */
char *func_00443f38(char *text, const char *delimiters, uint32_t *save)
{
    char *token;
    const unsigned char *d;
    if (text == NULL) {
        if (*save == 0)
            return NULL;
        text = P4_G2H(*save);
    }
    for (;;) {
        unsigned char c = (unsigned char)*text;
        for (d = (const unsigned char *)delimiters; *d && *d != c; ++d) {}
        if (c == 0) {
            *save = 0;
            return NULL;
        }
        if (*d == 0)
            break;
        ++text;
    }
    token = text++;
    for (;;) {
        unsigned char c = (unsigned char)*text;
        for (d = (const unsigned char *)delimiters; *d && *d != c; ++d) {}
        if (*d == c) {
            if (c != 0) {
                *text++ = 0;
                *save = P4_H2G(text);
            } else {
                *save = 0;
            }
            return token;
        }
        ++text;
    }
}

char *func_00443f18(char *text, const char *delimiters)
{
    uint32_t reent;
    memcpy(&reent, P4_G2H(0x00710070u), sizeof reent);
    return func_00443f38(text, delimiters, P4_G2H(reent + 0x5cu));
}

/* func_00446ed8 is vsprintf (RW and CRI debug paths pass a va_list). It writes
 * into a caller buffer and does not flush, matching the retail contract. */
int func_00446ed8(char *buf, const char *fmt, va_list ap)
{
    return p4pc_ee_vsnprintf(buf, (size_t)0x7fffffff, fmt, 0, ap);
}

/* func_00446e88 is vprintf: retail passes the reent's stdout (*(0x00710070) + 8) and its two
 * arguments to vfprintf. Output flushes as in func_00440b68. */
int func_00446e88(const char *fmt, va_list ap)
{
    int n = p4pc_ee_vfprintf(stdout, fmt, 0, ap);
    fflush(stdout);
    return n;
}

int func_00442088(char *buf, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = p4pc_ee_vsnprintf(buf, (size_t)0x7fffffff, fmt, 0, ap);
    va_end(ap);
    return n;
}

int p4pc_ee_snprintf(char *buffer, size_t capacity, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int count = p4pc_ee_vsnprintf(buffer, capacity, format, 0, args);
    va_end(args);
    return count;
}

/* The double sin, cos and atan2 the RenderWare C names (RwMatrixRotate, RwMatrixQueryRotate, RpLightSetConeAngle) are the retail float cores: retail has no
 * double sin/cos/atan2, its RwMatrixRotate calls sinf (0x0044b7b0) and cosf (0x0044b610). include/p4pc/libc_rename.h renames the decomp's calls to these. */
extern float func_0044b7b0(float x);
extern float func_0044b610(float x);
extern float func_004494b8(float y, float x);

double p4pc_rw_sin(double x)
{
    return (double)func_0044b7b0((float)x);
}

double p4pc_rw_cos(double x)
{
    return (double)func_0044b610((float)x);
}

double p4pc_rw_atan2(double y, double x)
{
    return (double)func_004494b8((float)y, (float)x);
}


/* 00444210 -> 00443fd0 is signed-64; 00444458 -> 00444240 is unsigned-64.
 * Both consume the same ASCII grammar and keep scanning after overflow. The
 * retail ctype table marks only TAB..CR/space, 0..9 and A..Z/a..z. A host long
 * has the wrong width on Win64, and host strtol is also the wrong signedness
 * for 00444458. Keep the cutoff arithmetic and the guest errno/end slots here. */
static uint64_t parse_integer(uint32_t *reent, const char *text, uint32_t *end, int base, int is_signed)
{
    const char *p = text;
    unsigned c = (uint8_t)*p++;
    int negative = 0, any = 0, overflow = 0;
    uint64_t value = 0, limit, radix, cutoff, cutlim;

    while (c == ' ' || (c >= '\t' && c <= '\r'))
        c = (uint8_t)*p++;
    if (c == '-' || c == '+') {
        negative = c == '-';
        c = (uint8_t)*p++;
    }
    if ((base == 0 || base == 16) && c == '0' && (*p == 'x' || *p == 'X')) {
        c = (uint8_t)p[1];
        p += 2;
        base = 16;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;
    limit = is_signed ? (negative ? UINT64_C(1) << 63 : UINT64_MAX >> 1) : UINT64_MAX;
    radix = (uint64_t)(int64_t)base;
    cutoff = limit / radix;
    cutlim = limit % radix;
    for (;;) {
        int digit;
        if (c >= '0' && c <= '9')
            digit = (int)c - '0';
        else if (c >= 'A' && c <= 'Z')
            digit = (int)c - 'A' + 10;
        else if (c >= 'a' && c <= 'z')
            digit = (int)c - 'a' + 10;
        else
            break;
        if (digit >= base)
            break;
        any = 1;
        if (value > cutoff || (value == cutoff && (uint64_t)digit > cutlim))
            overflow = 1;
        if (!overflow)
            value = value * radix + (unsigned)digit;
        c = (uint8_t)*p++;
    }
    if (end != NULL)
        *end = P4_H2G(any ? p - 1 : text);
    if (overflow) {
        reent[0] = 34; /* retail ERANGE */
        value = limit;
        if (!is_signed)
            return value;
    }
    return negative ? UINT64_C(0) - value : value;
}

int64_t func_00443fd0(uint32_t *reent, const char *text, uint32_t *end, int base)
{
    uint64_t bits = parse_integer(reent, text, end, base, 1);
    int64_t value;

    memcpy(&value, &bits, sizeof value);
    return value;
}

uint64_t func_00444240(uint32_t *reent, const char *text, uint32_t *end, int base)
{
    return parse_integer(reent, text, end, base, 0);
}

int64_t func_00444210(const char *text, uint32_t *end, int base)
{
    uint32_t reent;
    memcpy(&reent, P4_G2H(0x00710070u), 4);
    return func_00443fd0(P4_G2H(reent), text, end, base);
}

uint64_t func_00444458(const char *text, uint32_t *end, int base)
{
    uint32_t reent;
    memcpy(&reent, P4_G2H(0x00710070u), 4);
    return func_00444240(P4_G2H(reent), text, end, base);
}

int32_t func_0043c6b0(const char *text)
{
    uint32_t bits = (uint32_t)func_00444210(text, NULL, 10);
    int32_t value;

    memcpy(&value, &bits, sizeof value);
    return value;
}

/* func_004156e0 is the RW registry's keyed sort: it orders `n` records of `size` bytes by the
 * unsigned 32-bit key at byte offset `key_offset`, ascending. Retail runs only when base is set,
 * the key fits in the record (key_offset + 4 <= size) and lo < hi as 64-bit values (the range
 * only sizes its radix partition); fewer than two records are left alone. Its sole retail
 * caller, 004142b0 (jal at 00414348), supplies unique registry identifiers: 00413350 finds and
 * reuses an existing cell instead of appending a duplicate. The total unsigned-key order is
 * therefore uniquely defined here. Use the retail qsort binding rather than the host CRT. */
/* The comparator reads the key offset from this variable: the sort runs on the single host
 * thread the guest code shares (kernel_thread.c switches contexts cooperatively). */
static uint32_t keyed_sort_offset;

static int keyed_sort_cmp(const void *a, const void *b)
{
    uint32_t x, y;

    memcpy(&x, (const uint8_t *)a + keyed_sort_offset, sizeof x);
    memcpy(&y, (const uint8_t *)b + keyed_sort_offset, sizeof y);
    return x < y ? -1 : x > y;
}

extern void func_00440bb8(void *, uint32_t, uint32_t,
                          int (*)(const void *, const void *));

void func_004156e0(void *base, uint32_t n, uint32_t size, int32_t key_offset, int32_t lo, int32_t hi)
{
    if (base == NULL || (int64_t)key_offset + 4 > (int64_t)size ||
        !((uint64_t)(int64_t)lo < (uint64_t)(int64_t)hi) || n < 2)
        return;
    keyed_sort_offset = (uint32_t)key_offset;
    func_00440bb8(base, n, size, keyed_sort_cmp);
}

