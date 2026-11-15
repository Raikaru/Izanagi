/* fclCombineSkillTargetLv.ftd -> rates indexed by port skill ID.
 * Layout and mapping: docs/golden_mode/specs/fcl_tables.md. */
#include "p4pc/golden_inheritance.h"
#include "p4pc/golden_source.h"
#include <stdio.h>
#include <string.h>

static int fail(char *reason, size_t capacity, const char *text)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return -1;
}

int p4_golden_inheritance_convert(const P4GoldenDomainMap *map, const uint8_t *ftd, size_t size,
                                  uint8_t *out, size_t capacity, size_t *written,
                                  char *reason, size_t reason_capacity)
{
    P4GoldenSpan payload;
    uint32_t rows, port, highest = 0;
    if (!map || !out || !written || p4_golden_record_table(ftd, size, 0, &payload, &rows))
        return fail(reason, reason_capacity, "invalid inheritance rate table");
    if (rows != map->shape.skills || payload.size != rows)
        return fail(reason, reason_capacity, "inheritance rate count does not match skill domain");
    for (uint32_t i = 0; i < rows; ++i) {
        if (p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, i, &port, NULL) || port >= capacity)
            return fail(reason, reason_capacity, "inheritance rate destination is too small or invalid");
        if (port > highest) highest = port;
    }
    memset(out, 0, (size_t)highest + 1);
    for (uint32_t i = 0; i < rows; ++i) {
        p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, i, &port, NULL);
        out[port] = payload.bytes[i];
    }
    *written = (size_t)highest + 1;
    if (reason && reason_capacity) reason[0] = 0;
    return 0;
}
