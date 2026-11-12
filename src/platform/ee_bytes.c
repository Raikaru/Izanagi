/* Canonical EE-width byte/string bindings. Kept independent of formatter,
 * parser and guest-heap state so isolated game-unit fixtures use the same
 * implementation as the complete port. */
#include <stdint.h>
#include <string.h>

#include "p4pc/ps2_libc.h"

static P4ByteObserver byte_observer;
int p4pc_bytes_observer(P4ByteObserver observer)
{
    if (observer && byte_observer && observer!=byte_observer) return -1;
    byte_observer=observer;
    return 0;
}

void p4pc_bytes_invalidate(void *dst, uint32_t count)
{
    if (byte_observer) byte_observer(dst,NULL,count,0);
}

/* func_0043f9c8 is memset (decomp symbol; retail is the R5900 quadword version, which
 * returns its destination). Host memset has the same contract. */
void *func_0043f9c8(void *dst, int c, uint32_t n)
{
    if (byte_observer) byte_observer(dst,NULL,n,0);
    return memset(dst, c, n);
}

/* func_0043f778 is memcmp (retail compares 16 bytes at a time when both pointers are aligned, then
 * bytewise): it returns the difference of the first differing bytes, 0 for equal (a count of 0 is
 * equal). Host memcmp only promises the sign, so the loop is spelled out. */
int func_0043f778(const void *a, const void *b, uint32_t n)
{
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;

    for (; n != 0; n--, pa++, pb++) {
        if (*pa != *pb) {
            return (int)*pa - (int)*pb;
        }
    }
    return 0;
}

int func_004426e8(const char *left, const char *right)
{
    const uint8_t *a = (const uint8_t *)left;
    const uint8_t *b = (const uint8_t *)right;

    while (*a != 0 && *a == *b) {
        a++;
        b++;
    }
    return (int)*a - (int)*b;
}

int func_00442c30(const char *left, const char *right, uint32_t count)
{
    const uint8_t *a = (const uint8_t *)left;
    const uint8_t *b = (const uint8_t *)right;

    while (count-- != 0) {
        unsigned x = *a++, y = *b++;
        if (x != y || x == 0)
            return (int)x - (int)y;
    }
    return 0;
}

/* func_0043f810 is memcpy (the RW sources alias `memcpy` to it; retail is a forward copy
 * that returns its destination). Host memcpy has the same contract for non-overlapping
 * buffers. */
void *func_0043f810(void *dst, const void *src, uint32_t n)
{
    if (byte_observer) {
        byte_observer(dst,src,n,0);
        memcpy(dst,src,n);
        byte_observer(dst,src,n,1);
        return dst;
    }
    return memcpy(dst, src, n);
}

/* EE size_t is a guest word even when the host's size_t is eight bytes.
 * The host operation itself is reviewed: its overlapping byte result is
 * fully specified, and the native call performs the explicit width widening. */
void *func_0043f8c0(void *dst, const void *src, uint32_t n)
{
    if (byte_observer) {
        byte_observer(dst,src,n,0);
        memmove(dst,src,n);
        byte_observer(dst,src,n,1);
        return dst;
    }
    return memmove(dst, src, n);
}

/* func_00443010 is strstr (decomp rename in include/p4pc/libc_rename.h). Retail returns
 * a pointer into the haystack or NULL; host strstr has the same contract. */
char *func_00443010(const char *haystack, const char *needle)
{
    return strstr(haystack, needle);
}

/* func_00442a80 is strncat (decomp rename in include/p4pc/libc_rename.h). Retail finds the end
 * of dst, appends at most n bytes of src and always terminates the result when it copied
 * anything; with n == 0 it appends nothing. It returns dst, the same contract as host strncat. */
char *func_00442a80(char *dst, const char *src, uint32_t n)
{
    return strncat(dst, src, n);
}

/* strncpy (newlib, the aligned quadword version; same contract as the host's) */
char *func_00442de8(char *dst, const char *src, uint32_t n)
{
    return strncpy(dst, src, n);
}

/* strlen (0x00442948): the R5900 MMI version scans 16 bytes at a time for a zero byte (psubb / pnor /
 * pand against 0x01 and 0x80 splats), with a doubleword and a byte loop for unaligned starts; it
 * returns the byte count before the terminator, exactly as the host strlen. */
uint32_t func_00442948(const char *s)
{
    return (uint32_t)strlen(s);
}
