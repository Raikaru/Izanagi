#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "p4pc/battle_live.h"

static void put(uint8_t *p, uint32_t n)
{
    p[0] = (uint8_t)n; p[1] = (uint8_t)(n >> 8); p[2] = (uint8_t)(n >> 16); p[3] = (uint8_t)(n >> 24);
}
int main(void)
{
    uint8_t bytes[64] = "P4BLIVE1";
    P4BattleLiveEntry e;
    put(bytes + 8, 64); put(bytes + 12, 2); put(bytes + 16, 1); put(bytes + 20, 0xffff);
    put(bytes + 24, 1); put(bytes + 28, 50000); put(bytes + 32, 10000);
    put(bytes + 36, 1000); put(bytes + 40, 1);
    assert(p4_battle_live_decode(bytes, sizeof bytes, &e));
    assert(e.mode == 1 && e.encounter == 0xffff && e.occurrence == 1 && e.flags == 2);
    assert(!p4_battle_live_decode(bytes, sizeof bytes - 1, &e));
    assert(!p4_battle_live_decode(NULL, sizeof bytes, &e));
    assert(!p4_battle_live_decode(bytes, sizeof bytes, NULL));
    put(bytes + 20, 0x10000); assert(!p4_battle_live_decode(bytes, sizeof bytes, &e));
    put(bytes + 20, 123); put(bytes + 24, 0); assert(!p4_battle_live_decode(bytes, sizeof bytes, &e));
    put(bytes + 24, 1); put(bytes + 12, 4); assert(!p4_battle_live_decode(bytes, sizeof bytes, &e));
    put(bytes + 12, 3); put(bytes + 16, 2); put(bytes + 40, 2);
    assert(!p4_battle_live_decode(bytes, sizeof bytes, &e));
    put(bytes + 44, 15); put(bytes + 48, 28); put(bytes + 52, 2); put(bytes + 56, 353); put(bytes + 60, 110);
    assert(p4_battle_live_decode(bytes, sizeof bytes, &e));
    assert(e.mode == 2 && e.completion == 2 && e.field == 28 && e.date == 353 && e.step == 110);
    put(bytes + 44, 7); assert(!p4_battle_live_decode(bytes, sizeof bytes, &e));
    put(bytes + 60, 0); assert(p4_battle_live_decode(bytes, sizeof bytes, &e));
    put(bytes + 16, 4); assert(p4_battle_live_decode(bytes, sizeof bytes, &e));
    assert(e.mode == 4 && e.encounter == 123 && e.occurrence == 1);
    put(bytes + 16, 3); assert(!p4_battle_live_decode(bytes, sizeof bytes, &e));
    put(bytes + 20, 0); put(bytes + 24, 0); assert(!p4_battle_live_decode(bytes, sizeof bytes, &e));
    put(bytes + 24, 42000); assert(p4_battle_live_decode(bytes, sizeof bytes, &e));
    assert(e.mode == 3 && !e.encounter && e.occurrence == 42000 && e.completion == 2);
    put(bytes + 40, 1); assert(!p4_battle_live_decode(bytes, sizeof bytes, &e));
    put(bytes + 40, 2);
    bytes[0] = 'X'; assert(!p4_battle_live_decode(bytes, sizeof bytes, &e));
    return 0;
}
