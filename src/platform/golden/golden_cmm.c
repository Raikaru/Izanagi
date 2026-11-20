/* cmmRankUpSkillTable.dat: docs/golden_mode/specs/cmm_tables.md. */
#include "p4pc/golden_cmm.h"
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
int p4_golden_cmm_rankup_convert(const P4GoldenDomainMap *map, const uint8_t *dat, size_t size,
                                  uint8_t out[480], char *reason, size_t capacity)
{
    P4GoldenSpan payload;
    uint8_t result[480];
    uint32_t rows, port;
    if (!map || !out || p4_golden_record_table(dat, size, 1, &payload, &rows) || rows != 10 || payload.size != sizeof result)
        return fail(reason, capacity, "invalid rank-up DAT shape");
    memcpy(result, payload.bytes, sizeof result);
    for (size_t i = 0; i < sizeof result; i += 8) {
        uint16_t skill = u16(payload.bytes + i + 4), replaced = u16(payload.bytes + i + 6);
        if (p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, skill, &port, NULL))
            return fail(reason, capacity, "rank-up skill outside Golden domain");
        put16(result + i + 4, port);
        if (replaced) {
            if (p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, replaced, &port, NULL))
                return fail(reason, capacity, "rank-up replacement outside Golden domain");
            put16(result + i + 6, port);
        }
    }
    memcpy(out, result, sizeof result);
    if (reason && capacity) reason[0] = 0;
    return 0;
}
