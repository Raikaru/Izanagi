/* UNIT.TBL enemy skill and item references, per battle_tables.md. */
#include "p4pc/golden_enemy_convert.h"
#include <stdio.h>
#include <string.h>
static uint16_t u16(const uint8_t *p) { return (uint16_t)((unsigned)p[0] | (unsigned)p[1] << 8); }
static void put16(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static int fail(char *reason, size_t capacity, const char *text)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return -1;
}
static int skill(const P4GoldenDomainMap *map, unsigned id, uint32_t *port)
{
    if (!id) { *port = 0; return 0; }
    return p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, id, port, NULL);
}
int p4_golden_enemy_convert(const P4GoldenDomainMap *map, const uint8_t *input, size_t size,
                            uint8_t *out, size_t capacity, char *reason, size_t reason_capacity)
{
    uint32_t port;
    if (!map || !input || !out || map->shape.enemies != 368 || size != 368 * 60 || capacity < size)
        return fail(reason, reason_capacity, "invalid UNIT enemy block shape");
    for (unsigned row = 0; row < 368; ++row) {
        const uint8_t *src = input + row * 60;
        for (unsigned i = 0; i < 8; ++i)
            if (skill(map, u16(src + 0x0e + 2 * i), &port))
                return fail(reason, reason_capacity, "enemy skill outside Golden domain");
        if (u16(src + 0x34) && p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_ITEM, u16(src + 0x34), &port, NULL))
            return fail(reason, reason_capacity, "enemy special drop outside Golden domain");
        for (unsigned i = 0; i < 4; ++i) {
            unsigned id = u16(src + 0x22 + 4 * i);
            if (id && p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_ITEM, id, &port, NULL))
                return fail(reason, reason_capacity, "enemy drop outside Golden domain");
        }
    }
    memcpy(out, input, size);
    for (unsigned row = 0; row < 368; ++row) {
        uint8_t *dst = out + row * 60;
        for (unsigned i = 0; i < 8; ++i) {
            uint8_t *field = dst + 0x0e + 2 * i;
            skill(map, u16(field), &port); put16(field, port);
        }
        if (u16(dst + 0x34)) {
            p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_ITEM, u16(dst + 0x34), &port, NULL);
            put16(dst + 0x34, port);
        }
        for (unsigned i = 0; i < 4; ++i) {
            uint8_t *field = dst + 0x22 + 4 * i;
            unsigned id = u16(field);
            if (id) { p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_ITEM, id, &port, NULL); put16(field, port); }
        }
    }
    if (reason && reason_capacity) reason[0] = 0;
    return 0;
}
