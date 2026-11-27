/* Recipe conversion from docs/golden_mode/specs/fcl_tables.md. */
#include "p4pc/golden_recipe.h"
#include <stdio.h>
#include <string.h>

static unsigned u16(const uint8_t *p) { return (unsigned)p[0] | (unsigned)p[1] << 8; }
static int fail(char *reason, size_t capacity, const char *text, int status)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return status;
}

int p4_golden_recipe_convert(const P4GoldenDomainMap *map, const uint8_t *row, size_t size,
                             unsigned material_count, P4GoldenRecipe *out, char *reason, size_t capacity)
{
    uint32_t result, materials[12];
    P4GoldenRecipe converted = {0};
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
    converted.result = (uint16_t)result;
    converted.material_count = (uint8_t)material_count;
    for (unsigned i = 0; i < material_count; ++i) converted.materials[i] = (uint16_t)materials[i];
    *out = converted;
    if (reason && capacity) reason[0] = 0;
    return 0;
}
