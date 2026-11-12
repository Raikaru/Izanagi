#include "p4pc/battle_live.h"
#include <string.h>

static uint32_t word(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

int p4_battle_live_decode(const uint8_t *bytes, size_t length, P4BattleLiveEntry *out)
{
    if (!bytes || !out || length != 64 || memcmp(bytes, "P4BLIVE1", 8) || word(bytes + 8) != 64)
        return 0;
    P4BattleLiveEntry entry = {
        word(bytes + 12), word(bytes + 16), word(bytes + 20), word(bytes + 24),
        word(bytes + 28), word(bytes + 32), word(bytes + 36), word(bytes + 40),
        word(bytes + 44), word(bytes + 48), word(bytes + 52), word(bytes + 56), word(bytes + 60)
    };
    if ((entry.flags & ~3u) || (entry.mode != 1 && entry.mode != 2 && entry.mode != 3 && entry.mode != 4) ||
        entry.encounter > 65535 || !entry.route_limit || !entry.fight_limit || !entry.action_limit ||
        !entry.occurrence || (entry.mode == 3 && (entry.encounter || entry.completion != 2)) ||
        (entry.completion != 1 && entry.completion != 2) || (entry.endpoint_mask & ~15u) ||
        (entry.completion == 1 && entry.endpoint_mask) || (entry.completion == 2 && !entry.endpoint_mask) ||
        (!(entry.endpoint_mask & 1u) && entry.field) || (!(entry.endpoint_mask & 2u) && entry.room) ||
        (!(entry.endpoint_mask & 4u) && entry.date) || (!(entry.endpoint_mask & 8u) && entry.step)) return 0;
    *out = entry;
    return 1;
}
