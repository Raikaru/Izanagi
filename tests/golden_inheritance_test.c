/* Synthetic FTD and skill IDs; no game data. */
#include "p4pc/golden_inheritance.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (0)

int main(void)
{
    const P4GoldenDomainShape shape = {624, 472, 368, 2560, 256};
    P4GoldenDomainMap map;
    uint8_t ftd[640] = {0}, out[624];
    size_t written = 0;
    char reason[128];
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    ftd[0] = 0x70; ftd[1] = 2; /* 624 payload bytes */
    ftd[4] = 0x70; ftd[5] = 2;
    for (unsigned i = 0; i < 624; ++i) ftd[16 + i] = (uint8_t)i;
    CHECK(!p4_golden_inheritance_convert(&map, ftd, sizeof ftd, out, sizeof out, &written, reason, sizeof reason));
    CHECK(written == 624 && out[439] == (uint8_t)439 && out[576] == (uint8_t)440);
    CHECK(out[607] == (uint8_t)471 && out[440] == (uint8_t)472 && out[608] == (uint8_t)608);
    memset(out, 0x55, sizeof out);
    CHECK(p4_golden_inheritance_convert(&map, ftd, sizeof ftd, out, 623, &written, reason, sizeof reason) == -1);
    CHECK(out[0] == 0x55);
    ftd[8] = 1;
    CHECK(p4_golden_inheritance_convert(&map, ftd, sizeof ftd, out, sizeof out, &written, reason, sizeof reason) == -1);
    puts("golden_inheritance: ok");
    return 0;
}
