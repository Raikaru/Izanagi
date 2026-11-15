/* Fusion and inheritance FTDs per docs/golden_mode/specs/fcl_tables.md. */
#include "p4pc/golden_fusion_convert.h"
#include "p4pc/golden_source.h"
#include <stdio.h>
#include <string.h>
static uint16_t u16(const uint8_t *p) { return (uint16_t)((unsigned)p[0] | (unsigned)p[1] << 8); }
static void put16(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static int fail(char *reason, size_t capacity, const char *text)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return -1;
}
int p4_golden_fusion_convert(const P4GoldenDomainMap *map, P4GoldenFusionKind kind,
                              const uint8_t *ftd, size_t size, uint8_t *out, size_t capacity,
                              size_t *written, char *reason, size_t reason_capacity)
{
    static const unsigned counts[] = {9, 14, 26, 28}, strides[] = {2, 38, 8, 28};
    P4GoldenSpan payload;
    uint32_t rows, port;
    if (!map || !out || !written || (unsigned)kind >= 4 ||
        p4_golden_record_table(ftd, size, 0, &payload, &rows) || rows != counts[kind] ||
        payload.size != (size_t)counts[kind] * strides[kind] || capacity < payload.size)
        return fail(reason, reason_capacity, "invalid fusion table shape or capacity");
    if (kind == P4_GOLDEN_FUSION_GATE) {
        for (unsigned i = 0; i < rows; ++i) {
            unsigned id = u16(payload.bytes + i * 8 + 4);
            if (id && p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_PERSONA, id, &port, NULL))
                return fail(reason, reason_capacity, "fusion gate persona outside Golden domain");
        }
    } else if (kind == P4_GOLDEN_FUSION_CHART) {
        for (size_t i = 0; i < payload.size; ++i)
            if (payload.bytes[i] >= 28) return fail(reason, reason_capacity, "fusion chart arcana outside Golden domain");
    }
    memcpy(out, payload.bytes, payload.size);
    if (kind == P4_GOLDEN_FUSION_GATE) for (unsigned i = 0; i < rows; ++i) {
        uint8_t *field = out + i * 8 + 4;
        unsigned id = u16(field);
        if (id) { p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_PERSONA, id, &port, NULL); put16(field, port); }
    }
    *written = payload.size;
    if (reason && reason_capacity) reason[0] = 0;
    return 0;
}
