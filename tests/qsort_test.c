#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "guest_test.h"
#include "p4pc/ps2_libc.h"


/* compat.h selected host math for this host fixture. Rebind those names
 * explicitly before including the game's libc aliases. */
#undef sinf
#undef cosf
#undef tanf
#undef atanf
#undef sin
#undef cos
#undef atan2
#undef pow
#undef log

/* Keep the real override and the game's binding together: deleting the alias
 * must exercise the host CRT and fail, rather than testing an unused native. */
#include "p4pc/libc_rename.h"
#ifdef P4_GUEST64
void func_00440bb8(char *, uint32_t, uint32_t, int (*)(const void *, const void *));
#else
#include "../overrides/00440bb8.c"
#endif

struct Record {
    uint32_t key;
    uint32_t id;
};

static int compare_key(const void *left, const void *right)
{
    uint32_t a, b;
    memcpy(&a, left, sizeof a);
    memcpy(&b, right, sizeof b);
    return a < b ? -1 : a > b;
}

int main(void)
{
    /* Retail's n=8 median-of-three chooses its last candidate when every key
     * compares equal. Its three-way partition leaves this exact order. */
    static const uint32_t expected[] = {7, 1, 2, 3, 4, 5, 6, 0};
    unsigned char *storage;
    unsigned offset, i;

    if (p4t_guest_init() != 0)
        return 77;
    storage = p4t_galloc(8 * sizeof(struct Record) + 1, 8);
    if (storage == NULL)
        return 1;

    for (offset = 0; offset < 2; ++offset) {
        unsigned char *base = storage + offset;
        for (i = 0; i < 8; ++i) {
            struct Record record = {17, i};
            memcpy(base + i * sizeof record, &record, sizeof record);
        }
        qsort((char *)base, 8, sizeof(struct Record), compare_key);
        for (i = 0; i < 8; ++i) {
            struct Record record;
            memcpy(&record, base + i * sizeof record, sizeof record);
            if (record.key != 17 || record.id != expected[i]) {
                fprintf(stderr, "retail qsort offset %u row %u: key %u id %u, expected 17/%u\n",
                        offset, i, record.key, record.id, expected[i]);
                return 1;
            }
        }
    }
    puts("retail qsort equal-key order: aligned and unaligned passed");
    return 0;
}
