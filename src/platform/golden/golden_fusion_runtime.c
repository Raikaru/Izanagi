/* Runtime storage for the verified, relocated Golden fusion image tables. */
#include "p4pc/golden_fusion_runtime.h"
#include <stdlib.h>
#include <string.h>

enum { CHART_BYTES = 28 * 28, CHART_COUNT = 3, GATE_BYTES = 26 * 8 };
static uint8_t runtime_charts[CHART_COUNT * CHART_BYTES];
static uint8_t runtime_gates[GATE_BYTES];
static int installed;

int p4_golden_fusion_install(P4GameMode mode, const uint8_t *charts, size_t chart_size,
                             const uint8_t *gates, size_t gate_size)
{
    if (mode != P4_GAME_MODE_GOLDEN || !charts || !gates ||
        chart_size != sizeof runtime_charts || gate_size != sizeof runtime_gates) return -1;
    for (size_t i = 0; i < chart_size; ++i)
        if (charts[i] >= P4_GOLDEN_ARCANA_COUNT) return -1;
    memcpy(runtime_charts, charts, sizeof runtime_charts);
    memcpy(runtime_gates, gates, sizeof runtime_gates);
    installed = 1;
    return 0;
}
void p4_golden_fusion_reset(void) { installed = 0; }
static void require_install(void) { if (!installed) abort(); }
unsigned p4_golden_fusion_chart(unsigned chart, unsigned first, unsigned second)
{
    require_install();
    if (chart >= CHART_COUNT || first >= P4_GOLDEN_ARCANA_COUNT || second >= P4_GOLDEN_ARCANA_COUNT) abort();
    if (first > second) { unsigned t = first; first = second; second = t; }
    return runtime_charts[chart * CHART_BYTES + first * P4_GOLDEN_ARCANA_COUNT + second];
}
unsigned p4_golden_fusion_gate_count(void) { require_install(); return P4_GOLDEN_FUSION_GATE_COUNT; }
static const uint8_t *gate(unsigned i) { require_install(); if (i >= P4_GOLDEN_FUSION_GATE_COUNT) abort(); return runtime_gates + i * 8; }
static int32_t signed16(const uint8_t *p) { unsigned n = p[0] | (unsigned)p[1] << 8; return n < 0x8000u ? (int32_t)n : (int32_t)n - 0x10000; }
int32_t p4_golden_fusion_gate_flag(unsigned i)
{
    const uint8_t *p = gate(i);
    uint32_t n = p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
    return n <= INT32_MAX ? (int32_t)n : -1 - (int32_t)(UINT32_MAX - n);
}
int32_t p4_golden_fusion_gate_persona(unsigned i) { return signed16(gate(i) + 4); }
int32_t p4_golden_fusion_gate_bits(unsigned i) { return signed16(gate(i) + 6); }
