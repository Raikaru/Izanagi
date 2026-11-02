/* The game heap API (include/p4pc/platform.h) for tests that link a port module using it without the boot
 * heap: blocks come from guest storage (src/platform/gport.c), which is what the real heap's blocks are. A block
 * has a 16-byte header with its size; free is a no-op (a test process is short). */
#include <stdint.h>
#include <string.h>

#include "p4pc/gmem.h"
#include "p4pc/platform.h"

void *p4pc_heap_malloc(size_t n)
{
    uint8_t *p = P4_G2H(p4_gport_alloc((uint32_t)n + 16u, 16));

    if (p == NULL) {
        return NULL;
    }
    memcpy(p, &n, sizeof n < 16 ? sizeof n : 16);
    return p + 16;
}

void *p4pc_heap_calloc(size_t count, size_t size)
{
    return p4pc_heap_malloc(count * size); /* guest storage is zeroed */
}

void *p4pc_heap_realloc(void *old, size_t n)
{
    void *fresh = p4pc_heap_malloc(n);
    size_t have = 0;

    if (old != NULL && fresh != NULL) {
        memcpy(&have, (uint8_t *)old - 16, sizeof have);
        memcpy(fresh, old, have < n ? have : n);
    }
    return fresh;
}

void p4pc_heap_free(void *p)
{
    (void)p;
}
