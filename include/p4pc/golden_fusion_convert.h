/* Golden fusion and inheritance FTD projections. */
#ifndef P4PC_GOLDEN_FUSION_CONVERT_H
#define P4PC_GOLDEN_FUSION_CONVERT_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/golden_domains.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum P4GoldenFusionKind {
    P4_GOLDEN_FUSION_LIMIT, P4_GOLDEN_FUSION_AFFINITY,
    P4_GOLDEN_FUSION_GATE, P4_GOLDEN_FUSION_CHART
} P4GoldenFusionKind;
/* Validates a complete FTD and copies its unpadded payload. Gate persona IDs
 * are mapped; chart arcana indices are bounded. Input/output must not overlap. */
int p4_golden_fusion_convert(const P4GoldenDomainMap *map, P4GoldenFusionKind kind,
                              const uint8_t *ftd, size_t size, uint8_t *out, size_t capacity,
                              size_t *written, char *reason, size_t reason_capacity);
#ifdef __cplusplus
}
#endif
#endif
