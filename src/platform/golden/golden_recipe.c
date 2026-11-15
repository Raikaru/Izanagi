/* Recipe conversion from docs/golden_mode/specs/fcl_tables.md. */
#include "p4pc/golden_recipe.h"
#include <stdio.h>
#include <string.h>

static unsigned u16(const uint8_t *p) { return (unsigned)p[0] | (unsigned)p[1] << 8; }
static void put16(uint8_t *p, uint32_t value) { p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8); }
static int fail(char *reason, size_t capacity, const char *text, int status)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return status;
}

int p4_golden_recipe_convert(const P4GoldenDomainMap *map, const uint8_t *row, size_t size,
                             unsigned material_count, uint8_t out[32], char *reason, size_t capacity)
{
    uint32_t result, materials[12];
    uint8_t converted[32] = {0};
    if (!map || !row || !out || size != 44 || material_count < 2 || material_count > 12)
        return fail(reason, capacity, "invalid recipe shape", -1);
    for (unsigned i = 2; i < 12; ++i)
        if (row[i]) return fail(reason, capacity, "invalid recipe reserved bytes", -1);
    for (unsigned i = 36; i < 44; ++i)
        if (row[i]) return fail(reason, capacity, "invalid recipe reserved bytes", -1);
    if (!u16(row) || p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_PERSONA, u16(row), &result, NULL))
        return fail(reason, capacity, "invalid recipe result persona", -1);
    for (unsigned i = 0; i < 12; ++i) {
        unsigned id = u16(row + 12 + i * 2);
        if (i >= material_count) {
            if (id) return fail(reason, capacity, "unexpected recipe material", -1);
        } else if (!id || p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_PERSONA, id, &materials[i], NULL)) {
            return fail(reason, capacity, "invalid recipe material persona", -1);
        }
    }
    if (material_count > 8)
        return fail(reason, capacity, "recipe requires more than eight materials (Golden fusion widening required)", 1);
    put16(converted, result);
    for (unsigned i = 0; i < material_count; ++i) put16(converted + 8 + i * 2, materials[i]);
    memcpy(out, converted, sizeof converted);
    if (reason && capacity) reason[0] = 0;
    return 0;
}
