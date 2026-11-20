/* ITEMTBL sections from docs/golden_mode/specs/battle_tables.md. */
#include "p4pc/golden_item_convert.h"
#include <stdio.h>
#include <string.h>

static uint32_t u32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void put32(uint8_t *p, uint32_t n)
{
    p[0] = (uint8_t)n; p[1] = (uint8_t)(n >> 8); p[2] = (uint8_t)(n >> 16); p[3] = (uint8_t)(n >> 24);
}
static int header(const uint8_t *p, uint32_t count)
{
    return u32(p) == count && u32(p + 4) == 1 && u32(p + 8) == 256 && u32(p + 12) == 1;
}
static int fail(char *reason, size_t capacity, const char *text)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return -1;
}
int p4_golden_items_convert(const P4GoldenDomainMap *map, const uint8_t *input, size_t size,
                            uint8_t *rows, size_t rows_capacity, uint8_t *names, size_t names_capacity,
                            char *reason, size_t reason_capacity)
{
    size_t row_bytes, name_bytes, second;
    uint32_t count, port;
    if (!map || !input || !rows || !names || !map->shape.items || map->shape.items > P4_ITEM_MAX_ID + 1u)
        return fail(reason, reason_capacity, "invalid ITEMTBL arguments");
    count = map->shape.items;
    row_bytes = (size_t)count * 68;
    name_bytes = (size_t)count * 24;
    second = 16 + row_bytes;
    if (size != second + 16 + name_bytes || rows_capacity < row_bytes || names_capacity < name_bytes ||
        !header(input, count) || !header(input + second, count))
        return fail(reason, reason_capacity, "invalid ITEMTBL section shape");
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t *name = input + second + 16 + (size_t)i * 24;
        uint32_t skill = u32(input + 16 + (size_t)i * 68 + 0x24);
        size_t j = 0;
        if (skill && p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, skill, &port, NULL))
            return fail(reason, reason_capacity, "item granted skill outside Golden domain");
        while (j < 24 && name[j]) {
            if (name[j] < 0x20 || name[j] > 0x7e)
                return fail(reason, reason_capacity, "item name is not printable ASCII");
            ++j;
        }
        if (j == 24) return fail(reason, reason_capacity, "item name is not terminated");
        for (; j < 24; ++j) if (name[j]) return fail(reason, reason_capacity, "item name padding is not zero");
    }
    memcpy(rows, input + 16, row_bytes);
    memcpy(names, input + second + 16, name_bytes);
    for (uint32_t i = 0; i < count; ++i) {
        uint8_t *field = rows + (size_t)i * 68 + 0x24;
        uint32_t skill = u32(field);
        if (skill) {
            p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, skill, &port, NULL);
            put32(field, port);
        }
    }
    if (reason && reason_capacity) reason[0] = 0;
    return 0;
}
