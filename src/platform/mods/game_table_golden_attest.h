#ifndef P4_GAME_TABLE_GOLDEN_ATTEST_H
#define P4_GAME_TABLE_GOLDEN_ATTEST_H
#include "game_table_inputs_internal.h"
#include <string.h>
/* The input owners were created only by cache admission. Validate every actual
 * sealed kind, its canonical origin, and one common content-cache key. */
static inline int p4_game_table_golden_inputs_attest(P4GameTableInput *const *inputs, size_t count, int exposed)
{
    static const char prefix[] = "golden-cache/";
    const size_t key_start = sizeof prefix - 1u, key_end = key_start + 64u;
    const char *first = NULL;
    unsigned seen = 0;
    if (!exposed || !inputs || count != P4_TABLE_INPUT_COUNT) return 0;
    for (size_t i = 0; i < count; ++i) {
        const P4GameTableInput *input = inputs[i];
        if (!input || !input->sealed || (unsigned)input->kind >= P4_TABLE_INPUT_COUNT ||
            (seen & (1u << input->kind))) return 0;
        const char *origin = input->golden_origin;
        if (!memchr(origin, 0, sizeof input->golden_origin) ||
            strncmp(origin, prefix, key_start) || strlen(origin) <= key_end || origin[key_end] != '/' ||
            strcmp(origin + key_end + 1u, p4_game_table_input_label(input->kind))) return 0;
        for (size_t j = key_start; j < key_end; ++j)
            if (!((origin[j] >= '0' && origin[j] <= '9') || (origin[j] >= 'a' && origin[j] <= 'f'))) return 0;
        if (!first) first = origin;
        else if (strncmp(first + key_start, origin + key_start, 64u)) return 0;
        seen |= 1u << input->kind;
    }
    return seen == (1u << P4_TABLE_INPUT_COUNT) - 1u;
}
#endif
