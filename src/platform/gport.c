/* Port-owned guest storage (include/p4pc/gmem.h): zeroed, never freed.
 *
 * Objects the game addresses (stream joints, handle tables) are guest memory, not host statics, so
 * that their address is a valid guest address on a 64-bit host. 64-bit: a bump allocator over the top
 * 16 MB of the guest block (0x03000000..0x03FFFFFF; the port half below it belongs to the port-owned
 * data and the string pool). 32-bit: host memory, whose address is the guest address (identity). */
#include <stdlib.h>
#include <string.h>

#include "p4pc/gmem.h"

#ifdef P4_GUEST64

#define GPORT_BASE 0x03000000u

uint32_t p4_gport_alloc(uint32_t size, uint32_t align)
{
    static uint32_t next = GPORT_BASE;
    uint32_t at;

    if (align < 16u) {
        align = 16u;
    }
    at = (next + align - 1u) & ~(align - 1u);
    if (p4_guest_init() != 0 || at < next || at >= P4_GUEST_SIZE || size > P4_GUEST_SIZE - at) {
        return 0;
    }
    next = at + size;
    memset(P4_G2H(at), 0, size);
    return at;
}

#else

uint32_t p4_gport_alloc(uint32_t size, uint32_t align)
{
    uint8_t *raw;
    uintptr_t at;

    if (align < 16u) {
        align = 16u;
    }
    raw = (uint8_t *)calloc(1, (size_t)size + align);
    if (raw == NULL) {
        return 0;
    }
    at = ((uintptr_t)raw + align - 1u) & ~(uintptr_t)(align - 1u);
    return P4_H2G((void *)at);
}

#endif

/* String pool: one guest copy per distinct source address (a literal is interned once). */
#define GPORT_STR_MAX 256

uint32_t p4_gport_str(const char *s)
{
    static struct {
        const char *key;
        uint32_t addr;
    } pool[GPORT_STR_MAX];
    static unsigned n;
    unsigned i;
    size_t len;
    uint32_t at;

    if (s == NULL) {
        return 0;
    }
    for (i = 0; i < n; i++) {
        if (pool[i].key == s) {
            return pool[i].addr;
        }
    }
    if (n == GPORT_STR_MAX) {
        return 0;
    }
    len = strlen(s) + 1;
    at = p4_gport_alloc((uint32_t)len, 1);
    if (at == 0) {
        return 0;
    }
    memcpy(P4_G2H(at), s, len);
    pool[n].key = s;
    pool[n].addr = at;
    n++;
    return at;
}
