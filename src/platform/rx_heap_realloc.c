/* RxHeapRealloc (RenderWare P2 heap). The retail image has no copy of it (the linker dropped the
 * unused function), but the compiled p2core.c source calls it from RxClusterResizeData, so the
 * port defines it from the RenderWare 3.7 source (p2heap.c: HeapRealloc, HeapReallocLarger,
 * HeapReallocSmaller). It works on the retail heap layout that overrides/00414820.c, 00414b50.c,
 * 00414c10.c and 00414e10.c implement:
 *   heap +0x0c free-entry array of {size, header}, +0x14 entries used.
 *   block header (0x20 bytes before the data): +0 prev, +4 next, +8 size, +0xc free entry (0 when
 *   allocated).
 * Allocation and release go through the retail functions (func_00414e10, func_00414c10, and
 * func_00414820 for a new free entry). The `allowCopy` argument is unused, as in the source. */
#include <stdint.h>
#include <string.h>

#include "p4pc/gmem.h"

/* The heap records are guest memory: every link is a 4-byte guest address. HD(p, off) is the word at a host
 * pointer plus offset, HG(a, off) the word at a guest address plus offset, HP(p, off) a link read as a
 * host pointer. */
#define HD(p, off) (*(uint32_t *)((uint8_t *)(p) + (off)))
#define HG(a, off) HD(P4_G2H(a), off)
#define HP(p, off) ((uint8_t *)P4_G2H(HD(p, off)))

extern uint8_t *func_00414e10(uint8_t *heap, int32_t size);
extern void func_00414c10(uint8_t *heap, uint8_t *block);
extern uint8_t *func_00414820(uint32_t *heap);

/* HeapFreeBlocksDeleteEntry: removes a free entry, moving the last entry into its slot. */
static void delete_free_entry(uint8_t *heap, uint32_t *entry)
{
    uint32_t *entries = (uint32_t *)HP(heap, 0x0c);
    uint32_t used = HD(heap, 0x14);

    if (entries + (used - 1) * 2 != entry) {
        uint32_t *last = entries + (used - 1) * 2;

        entry[0] = last[0];
        entry[1] = last[1];
        HG(entry[1], 0x0c) = P4_H2G(entry);
    }
    HD(heap, 0x14) = used - 1;
}

static void *realloc_larger(uint8_t *heap, uint8_t *block, uint32_t new_size, uint8_t *hdr, int32_t delta)
{
    uint8_t *next = HP(hdr, 4);

    if (next != NULL && HD(next, 0x0c) != 0 && HD(next, 8) + 0x20 >= (uint32_t)delta) {
        uint32_t merged = HD(hdr, 8) + HD(next, 8) + 0x20;
        uint32_t excess = merged - new_size;

        if (excess < 256) {
            delete_free_entry(heap, (uint32_t *)HP(next, 0x0c));
            HD(hdr, 8) = merged;
            HD(hdr, 4) = HD(next, 4);
            if (HD(hdr, 4) != 0) {
                HG(HD(hdr, 4), 0) = P4_H2G(hdr);
            }
        } else {
            uint8_t *shifted = hdr + 0x20 + new_size;

            HD(shifted, 0x0c) = HD(next, 0x0c);
            HD(shifted, 8) = HD(next, 8) - delta;
            HD(shifted, 0) = P4_H2G(hdr);
            HD(shifted, 4) = HD(next, 4);
            if (HD(shifted, 4) != 0) {
                HG(HD(shifted, 4), 0) = P4_H2G(shifted);
            }
            HD(hdr, 8) = new_size;
            HD(hdr, 4) = P4_H2G(shifted);
            HG(HD(shifted, 0x0c), 4) = P4_H2G(shifted);
            HG(HD(shifted, 0x0c), 0) = HD(shifted, 8);
        }
        return block;
    } else {
        /* Free, allocate and copy. A failed allocation leaves the old block. */
        uint8_t *fresh = func_00414e10(heap, (int32_t)new_size);

        if (fresh != NULL) {
            memcpy(fresh, block, HD(hdr, 8));
            func_00414c10(heap, block);
            return fresh;
        }
    }
    return NULL;
}

static void *realloc_smaller(uint8_t *heap, uint8_t *block, uint32_t new_size, uint8_t *hdr, int32_t delta)
{
    uint32_t *entry;
    uint8_t *fresh;

    if (delta < 256) {
        return block;
    }
    entry = (uint32_t *)func_00414820((uint32_t *)heap);
    if (entry == NULL) {
        return block;
    }
    fresh = hdr + 0x20 + new_size;
    HD(fresh, 8) = (uint32_t)delta - 0x20;
    HD(fresh, 0) = P4_H2G(hdr);
    HD(fresh, 4) = HD(hdr, 4);
    HD(fresh, 0x0c) = P4_H2G(entry);
    HD(hdr, 8) = new_size;
    HD(hdr, 4) = P4_H2G(fresh);
    if (HD(fresh, 4) != 0) {
        HG(HD(fresh, 4), 0) = P4_H2G(fresh);
    }
    entry[1] = P4_H2G(fresh);
    entry[0] = HD(fresh, 8);
    return block;
}

void *RxHeapRealloc(uint8_t *heap, void *block, uint32_t new_size, int allow_copy)
{
    uint8_t *hdr = (uint8_t *)block - 0x20;
    int32_t delta;

    (void)allow_copy;
    new_size = (new_size + 31) & ~31u;
    delta = (int32_t)(new_size - HD(hdr, 8));
    if (delta > 0) {
        return realloc_larger(heap, (uint8_t *)block, new_size, hdr, delta);
    }
    return realloc_smaller(heap, (uint8_t *)block, new_size, hdr, -delta);
}
