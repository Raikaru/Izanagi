/* Golden-only fusion image-table runtime. The image-table binder owns installation. */
#ifndef P4PC_GOLDEN_FUSION_RUNTIME_H
#define P4PC_GOLDEN_FUSION_RUNTIME_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_mode.h"

#define P4_GOLDEN_ARCANA_COUNT 28u
#define P4_GOLDEN_FUSION_GATE_COUNT 26u
/* Chart order: normal, triangle first, triangle final. Copies all input bytes.
 * Refuses invalid mode, lengths, or chart values without changing installed data. */
int p4_golden_fusion_install(P4GameMode mode, const uint8_t *charts, size_t chart_size,
                             const uint8_t *gates, size_t gate_size);
void p4_golden_fusion_reset(void);
/* chart: 0 normal/alternate, 1 triangle first, 2 triangle final. */
unsigned p4_golden_fusion_chart(unsigned chart, unsigned first, unsigned second);
unsigned p4_golden_fusion_gate_count(void);
int32_t p4_golden_fusion_gate_flag(unsigned index);
int32_t p4_golden_fusion_gate_persona(unsigned index);
int32_t p4_golden_fusion_gate_bits(unsigned index);
#endif
