/* SKILL.TBL projections from docs/golden_mode/specs/battle_tables.md. */
#include "p4pc/golden_skill_convert.h"
#include <stdio.h>
#include <string.h>

static int fail(char *reason, size_t capacity, const char *text)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return -1;
}
static int prepare(const P4GoldenDomainMap *map, const uint8_t *input, size_t size, uint8_t *out,
                   size_t capacity, size_t *written, unsigned rows, unsigned source_stride, unsigned dest_stride,
                   char *reason, size_t reason_capacity)
{
    uint32_t port, max = 0;
    if (!map || !input || !out || !written || !rows || rows > P4_SKILL_MAX_ID + 1u ||
        size != (size_t)rows * source_stride)
        return fail(reason, reason_capacity, "invalid SKILL table shape");
    for (uint32_t i = 0; i < rows; ++i) {
        if (p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, i, &port, NULL))
            return fail(reason, reason_capacity, "skill ID outside Golden domain");
        if (port > max) max = port;
    }
    if ((size_t)max + 1 > capacity / dest_stride)
        return fail(reason, reason_capacity, "SKILL destination too small");
    *written = ((size_t)max + 1) * dest_stride;
    memset(out, 0, *written);
    return 0;
}
int p4_golden_skill_elements(const P4GoldenDomainMap *map, const uint8_t *input, size_t size,
                             uint8_t *out, size_t capacity, size_t *written, char *reason, size_t reason_capacity)
{
    unsigned rows = map ? map->shape.skills : 0;
    uint32_t port;
    if (prepare(map, input, size, out, capacity, written, rows, 2, 2, reason, reason_capacity)) return -1;
    for (uint32_t i = 0; i < rows; ++i) {
        p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, i, &port, NULL);
        memcpy(out + (size_t)port * 2, input + (size_t)i * 2, 2);
    }
    if (reason && reason_capacity) reason[0] = 0;
    return 0;
}
int p4_golden_skill_semantics(const uint8_t *elements, size_t rows, uint8_t *element_ids,
    size_t element_capacity, uint8_t *groups, size_t group_capacity)
{
    if (!elements || !rows || rows > P4_SKILL_MAX_ID + 1u || !element_ids || !groups ||
        rows > element_capacity / 4u || rows > group_capacity) return -1;
    for (size_t i = 0; i < rows; ++i)
        if (elements[2u * i] > 18u && elements[2u * i] != 0xFF) return -1;
    for (size_t i = 0; i < rows; ++i) {
        uint8_t element = elements[2u * i];
        uint32_t semantic = element == 0xFF ? UINT32_MAX : element;
        uint8_t *out = element_ids + 4u * i;
        out[0] = (uint8_t)semantic; out[1] = (uint8_t)(semantic >> 8);
        out[2] = (uint8_t)(semantic >> 16); out[3] = (uint8_t)(semantic >> 24);
        groups[i] = element == 0xFF ? 18u : element;
    }
    return 0;
}
int p4_golden_skill_metadata(const P4GoldenDomainMap *map, const uint8_t *input, size_t size,
                             uint8_t *out, size_t capacity, size_t *written, char *reason, size_t reason_capacity)
{
    unsigned rows = map ? map->shape.active_skills : 0;
    uint32_t port;
    if (prepare(map, input, size, out, capacity, written, rows, 44, 40, reason, reason_capacity)) return -1;
    for (uint32_t i = 0; i < rows; ++i) {
        const uint8_t *src = input + (size_t)i * 44;
        uint8_t *dst;
        p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, i, &port, NULL);
        dst = out + (size_t)port * 40;
        dst[0] = src[0];
        memcpy(dst + 1, src + 4, 3);
        memcpy(dst + 4, src + 8, 36);
    }
    if (reason && reason_capacity) reason[0] = 0;
    return 0;
}
