/* Synthetic persona learning events and replacement pairs. */
#include "p4pc/golden_persona_convert.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s (%s)\n", __LINE__, #x, reason); return 1; } } while (0)
static void put16(uint8_t *p, unsigned n) { p[0] = (uint8_t)n; p[1] = (uint8_t)(n >> 8); }
int main(void)
{
    const P4GoldenDomainShape shape = {624, 472, 368, 2560, 256};
    P4GoldenDomainMap map;
    static uint8_t in[256 * 70], out[256 * 70];
    char reason[128] = "";
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    in[6] = 1; in[7] = 1; put16(in + 8, 440);
    in[10] = 2; in[11] = 2; put16(in + 12, 472);
    in[14] = 3; in[15] = 3; put16(in + 16, 65000);
    CHECK(!p4_golden_persona_convert(&map, 1, in, 256 * 70, out, sizeof out, reason, sizeof reason));
    CHECK(out[8] == 0x40 && out[9] == 2 && out[12] == 0xb8 && out[13] == 1);
    CHECK(out[16] == in[16] && out[17] == in[17]);
    in[7] = 5;
    CHECK(p4_golden_persona_convert(&map, 1, in, 256 * 70, out, sizeof out, reason, sizeof reason) == -1);
    memset(in, 0, sizeof in); in[4] = 1; in[5] = 1; put16(in + 6, 440);
    CHECK(!p4_golden_persona_convert(&map, 2, in, 24 * 622, out, sizeof out, reason, sizeof reason));
    CHECK(out[6] == 0x40 && out[7] == 2);
    memset(in, 0, sizeof in); put16(in, 440); put16(in + 2, 472);
    CHECK(!p4_golden_persona_convert(&map, 4, in, 108 * 4, out, sizeof out, reason, sizeof reason));
    CHECK(out[0] == 0x40 && out[1] == 2 && out[2] == 0xb8 && out[3] == 1);
    puts("golden_persona_convert: ok");
    return 0;
}
