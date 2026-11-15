/* PERSONA.TBL event and replacement skill IDs; see battle_tables.md. */
#include "p4pc/golden_persona_convert.h"
#include <stdio.h>
#include <string.h>
static uint16_t u16(const uint8_t *p) { return (uint16_t)((unsigned)p[0] | (unsigned)p[1] << 8); }
static void put16(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static int fail(char *reason, size_t capacity, const char *text)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return -1;
}
int p4_golden_persona_convert(const P4GoldenDomainMap *map, unsigned block, const uint8_t *input,
                               size_t size, uint8_t *out, size_t capacity,
                               char *reason, size_t reason_capacity)
{
    unsigned rows, stride, offset, entries;
    uint32_t port;
    int terminated = 0;
    if (!map || !input || !out || (block != 1 && block != 2 && block != 4))
        return fail(reason, reason_capacity, "invalid persona block arguments");
    rows = block == 1 ? 256 : block == 2 ? 24 : 108;
    stride = block == 1 ? 70 : block == 2 ? 622 : 4;
    offset = block == 1 ? 6 : block == 2 ? 4 : 0;
    entries = block == 1 ? 16 : block == 2 ? 32 : 1;
    if (map->shape.personas != 256 || size != (size_t)rows * stride || capacity < size)
        return fail(reason, reason_capacity, "invalid persona block shape");
    for (unsigned row = 0; row < rows; ++row) {
        const uint8_t *src = input + (size_t)row * stride + offset;
        if (block == 4) {
            unsigned a = u16(src), b = u16(src + 2);
            if (!a || !b) { terminated = 1; break; }
            if (p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, a, &port, NULL) ||
                p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, b, &port, NULL))
                return fail(reason, reason_capacity, "replacement skill outside Golden domain");
        } else for (unsigned j = 0; j < entries; ++j) {
            const uint8_t *event = src + j * 4;
            unsigned kind = event[1], skill = u16(event + 2);
            if (!kind) break;
            if (kind > 4) return fail(reason, reason_capacity, "unknown persona event kind");
            if (kind <= 2 && skill && p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, skill, &port, NULL))
                return fail(reason, reason_capacity, "learned skill outside Golden domain");
        }
    }
    if (block == 4 && !terminated) return fail(reason, reason_capacity, "replacement list has no terminator");
    memcpy(out, input, size);
    for (unsigned row = 0; row < rows; ++row) {
        uint8_t *dst = out + (size_t)row * stride + offset;
        if (block == 4) {
            unsigned a = u16(dst), b = u16(dst + 2);
            if (!a || !b) break;
            p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, a, &port, NULL); put16(dst, port);
            p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, b, &port, NULL); put16(dst + 2, port);
        } else for (unsigned j = 0; j < entries; ++j) {
            uint8_t *event = dst + j * 4;
            unsigned kind = event[1], skill = u16(event + 2);
            if (!kind) break;
            if (kind <= 2 && skill) {
                p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, skill, &port, NULL);
                put16(event + 2, port);
            }
        }
    }
    if (reason && reason_capacity) reason[0] = 0;
    return 0;
}
