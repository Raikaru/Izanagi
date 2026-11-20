/* MSG.TBL fixed-width names: docs/golden_mode/specs/battle_tables.md. */
#include "p4pc/golden_names.h"
#include <stdio.h>
#include <string.h>
static int fail(char *reason, size_t capacity, const char *text)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return -1;
}
int p4_golden_names_convert(const P4GoldenDomainMap *map, P4GoldenDomain domain,
                            const uint8_t *input, size_t size, unsigned rows,
                            unsigned source_width, unsigned dest_width,
                            uint8_t *out, size_t capacity, size_t *written,
                            char *reason, size_t reason_capacity)
{
    uint32_t port, max = 0, expected;
    if (!map || !input || !out || !written || !rows || rows > P4_ITEM_MAX_ID + 1u ||
        !source_width || source_width > 256 || !dest_width || dest_width > 256 ||
        size != (size_t)rows * source_width)
        return fail(reason, reason_capacity, "invalid name bank shape");
    switch (domain) {
    case P4_GOLDEN_DOMAIN_SKILL: expected = map->shape.skills; break;
    case P4_GOLDEN_DOMAIN_ENEMY: expected = map->shape.enemies; break;
    case P4_GOLDEN_DOMAIN_ITEM: expected = map->shape.items; break;
    case P4_GOLDEN_DOMAIN_PERSONA: expected = map->shape.personas; break;
    default: return fail(reason, reason_capacity, "invalid name domain");
    }
    if (rows != expected) return fail(reason, reason_capacity, "name count does not match domain");
    for (uint32_t i = 0; i < rows; ++i) {
        const uint8_t *name = input + (size_t)i * source_width;
        unsigned length = 0, padding;
        if (p4_golden_domain_to_port(map, domain, i, &port, NULL))
            return fail(reason, reason_capacity, "name ID outside Golden domain");
        if (port > max) max = port;
        while (length < source_width && name[length]) ++length;
        if (length == source_width || length >= dest_width)
            return fail(reason, reason_capacity, "name does not fit destination width");
        for (padding = length; padding < source_width; ++padding)
            if (name[padding]) return fail(reason, reason_capacity, "name padding is not zero");
    }
    if ((size_t)max + 1 > capacity / dest_width)
        return fail(reason, reason_capacity, "name bank destination too small");
    *written = ((size_t)max + 1) * dest_width;
    memset(out, 0, *written);
    for (uint32_t i = 0; i < rows; ++i) {
        const uint8_t *name = input + (size_t)i * source_width;
        size_t length = strlen((const char *)name);
        p4_golden_domain_to_port(map, domain, i, &port, NULL);
        memcpy(out + (size_t)port * dest_width, name, length);
    }
    if (reason && reason_capacity) reason[0] = 0;
    return 0;
}
