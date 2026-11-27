#ifndef P4_GOLDEN_MODE_PROJECTION_H
#define P4_GOLDEN_MODE_PROJECTION_H
#include "p4pc/golden_source.h"
#include "p4pc/golden_domains.h"

typedef struct P4GoldenModeBytes { uint8_t *data; size_t size; } P4GoldenModeBytes;
/* Converted banks, including expanded rows. P4GoldenModeBytes buffers are owned;
 * skill_golden is borrowed. Split retail/declaration rows before native inputs. */
typedef struct P4GoldenModeProjection {
    P4GoldenModeBytes skill_elements, skill_metadata, skill_names;
    P4GoldenSpan skill_golden; /* Full raw SKILL1; borrowed from source through cache publication. */
    P4GoldenModeBytes enemy_metadata, enemy_names, persona_names;
    P4GoldenModeBytes persona[3]; /* blocks 1, 2, 4 */
    P4GoldenModeBytes encounter[5];
    P4GoldenModeBytes item_metadata, item_names;
} P4GoldenModeProjection;
/* out must be zero-initialized or previously freed; on refusal it is unchanged. */
int p4_golden_mode_project(const P4GoldenDomainMap *map, const P4GoldenSource *source,
                           P4GoldenModeProjection *out, char *reason, size_t capacity);
void p4_golden_mode_projection_free(P4GoldenModeProjection *projection);
#endif
