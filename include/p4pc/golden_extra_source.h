/* Additional pinned Golden battle and camp table sources. */
#ifndef P4PC_GOLDEN_EXTRA_SOURCE_H
#define P4PC_GOLDEN_EXTRA_SOURCE_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/golden_source.h"
#ifdef __cplusplus
extern "C" {
#endif
enum { P4_GOLDEN_EXTRA_EFFECT, P4_GOLDEN_EXTRA_MODEL, P4_GOLDEN_EXTRA_AICALC,
       P4_GOLDEN_EXTRA_CMP, P4_GOLDEN_EXTRA_COUNT };
typedef struct P4GoldenExtraTable {
    uint8_t *bytes;
    size_t size;
    P4GoldenSpan spans[12];
    int span_count;
} P4GoldenExtraTable;
typedef struct P4GoldenExtraSource { P4GoldenExtraTable tables[P4_GOLDEN_EXTRA_COUNT]; } P4GoldenExtraSource;
typedef struct P4GoldenExtraIdentity { const char *member, *sha256; } P4GoldenExtraIdentity;
const P4GoldenExtraIdentity *p4_golden_extra_identities(size_t *count);
/* Reads, hashes and retains the four measured English members. Supply a
 * zero-initialized output and free it before reuse. AICALC's opaque program
 * blocks are exposed as bytes, not asserted compatible with PS2 AI. */
int p4_golden_extra_load(P4GoldenExtraSource *out, const char *directory, char *reason, size_t capacity);
void p4_golden_extra_free(P4GoldenExtraSource *source);
#ifdef __cplusplus
}
#endif
#endif
