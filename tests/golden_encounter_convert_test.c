/* Synthetic ENCOUNT.TBL blocks. */
#include "p4pc/golden_encounter_convert.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s (%s)\n", __LINE__, #x, reason); return 1; } } while (0)
static void put16(uint8_t *p, unsigned n) { p[0] = (uint8_t)n; p[1] = (uint8_t)(n >> 8); }
int main(void)
{
    const P4GoldenDomainShape shape = {624, 472, 368, 2560, 256};
    const unsigned count[] = {944, 180, 1, 68, 84}, src[] = {24, 10, 200, 252, 348}, dst[] = {24, 10, 200, 248, 348};
    P4GoldenDomainMap map;
    static uint8_t in[84 * 348], out[84 * 348];
    char reason[128] = "";
    size_t written;
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    for (unsigned block = 0; block < 5; ++block) {
        memset(in, 0, sizeof in);
        if (block == 0) put16(in + 8, 336);
        if (block == 3) { in[0] = 1; put16(in + 12, 3); put16(in + 14, 9); }
        if (block == 4) put16(in + 2, 1536);
        CHECK(!p4_golden_encounter_convert(&map, block, in, count[block] * src[block], out, sizeof out, &written, reason, sizeof reason));
        CHECK(written == count[block] * dst[block]);
        if (block == 0) CHECK(out[8] == 0x50 && out[9] == 1);
        if (block == 3) CHECK(out[0] == 1 && out[8] == 3 && out[10] == 9);
        if (block == 4) CHECK(out[2] == 0 && out[3] == 6);
    }
    memset(in, 0, sizeof in); put16(in + 8, 368);
    CHECK(p4_golden_encounter_convert(&map, 0, in, 944 * 24, out, sizeof out, &written, reason, sizeof reason) == -1);
    memset(in, 0, sizeof in); in[0] = 1;
    CHECK(!p4_golden_encounter_convert(&map, 3, in, 68 * 252, out, sizeof out, &written, reason, sizeof reason) && out[0] == 1);
    puts("golden_encounter_convert: ok");
    return 0;
}
