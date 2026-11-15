/* ENCOUNT.TBL blocks from docs/golden_mode/specs/battle_tables.md. */
#include "p4pc/golden_encounter_convert.h"
#include <stdio.h>
#include <string.h>
static uint16_t u16(const uint8_t *p) { return (uint16_t)((unsigned)p[0] | (unsigned)p[1] << 8); }
static void put16(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static int fail(char *reason, size_t capacity, const char *text)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return -1;
}
int p4_golden_encounter_convert(const P4GoldenDomainMap *map, unsigned block,
                                 const uint8_t *input, size_t size, uint8_t *out, size_t capacity,
                                 size_t *written, char *reason, size_t reason_capacity)
{
    static const unsigned counts[] = {944, 180, 1, 68, 84};
    static const unsigned source_strides[] = {24, 10, 200, 252, 348};
    static const unsigned dest_strides[] = {24, 10, 200, 248, 348};
    uint32_t port;
    if (!map || !input || !out || !written || block >= 5 ||
        size != (size_t)counts[block] * source_strides[block] || capacity < (size_t)counts[block] * dest_strides[block])
        return fail(reason, reason_capacity, "invalid encounter block shape or capacity");
    for (unsigned i = 0; i < counts[block]; ++i) {
        const uint8_t *row = input + (size_t)i * source_strides[block];
        if (block == 0) {
            for (unsigned j = 0; j < 5; ++j) {
                unsigned enemy = u16(row + 8 + j * 2);
                if (enemy && p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_ENEMY, enemy, &port, NULL))
                    return fail(reason, reason_capacity, "formation enemy outside Golden domain");
            }
        } else if (block == 1) {
            if (u16(row) >= 68 || u16(row + 6) >= 84)
                return fail(reason, reason_capacity, "selector group outside Golden domain");
        } else if (block == 3) {
            for (unsigned j = 0; j < 60; ++j) {
                const uint8_t *entry = row + 12 + j * 4;
                if (u16(entry) >= 944) return fail(reason, reason_capacity, "formation outside Golden domain");
            }
        } else if (block == 4) {
            for (unsigned j = 0; j < 29; ++j) {
                unsigned item = u16(row + j * 12 + 2);
                if (item && p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_ITEM, item, &port, NULL))
                    return fail(reason, reason_capacity, "treasure item outside Golden domain");
            }
        }
    }
    for (unsigned i = 0; i < counts[block]; ++i) {
        const uint8_t *src = input + (size_t)i * source_strides[block];
        uint8_t *dst = out + (size_t)i * dest_strides[block];
        if (block == 3) {
            for (unsigned c = 0; c < 3; ++c) { dst[2*c] = src[3*c]; dst[2*c+1] = src[3*c+1]; }
            dst[6] = dst[7] = 0;
            memcpy(dst + 8, src + 12, 240);
        } else {
            memcpy(dst, src, dest_strides[block]);
            if (block == 0) for (unsigned j = 0; j < 5; ++j) {
                unsigned id = u16(src + 8 + j * 2);
                if (id) { p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_ENEMY, id, &port, NULL); put16(dst + 8 + j * 2, port); }
            }
            if (block == 4) for (unsigned j = 0; j < 29; ++j) {
                unsigned id = u16(src + j * 12 + 2);
                if (id) { p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_ITEM, id, &port, NULL); put16(dst + j * 12 + 2, port); }
            }
        }
    }
    *written = (size_t)counts[block] * dest_strides[block];
    if (reason && reason_capacity) reason[0] = 0;
    return 0;
}
