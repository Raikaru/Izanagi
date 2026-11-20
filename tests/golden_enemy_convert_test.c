/* Synthetic UNIT enemy rows. */
#include "p4pc/golden_enemy_convert.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s (%s)\n", __LINE__, #x, reason); return 1; } } while (0)
static void put16(uint8_t *p, unsigned n) { p[0] = (uint8_t)n; p[1] = (uint8_t)(n >> 8); }
int main(void)
{
    const P4GoldenDomainShape shape = {624, 472, 368, 2560, 256};
    P4GoldenDomainMap map;
    static uint8_t in[368 * 60], out[368 * 60];
    char reason[128] = "";
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    put16(in + 0x0e, 440); put16(in + 0x34, 1537); put16(in + 0x22, 1536);
    CHECK(!p4_golden_enemy_convert(&map, in, sizeof in, out, sizeof out, reason, sizeof reason));
    CHECK(out[0x0e] == 0x40 && out[0x0f] == 2 && out[0x34] == 1 && out[0x35] == 6);
    CHECK(out[0x22] == 0 && out[0x23] == 6);
    memset(out, 0x55, sizeof out); put16(in + 0x10, 624);
    CHECK(p4_golden_enemy_convert(&map, in, sizeof in, out, sizeof out, reason, sizeof reason) == -1 && out[0] == 0x55);
    puts("golden_enemy_convert: ok");
    return 0;
}
