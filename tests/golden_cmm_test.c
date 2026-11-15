/* Synthetic DAT records based on the cmm specification vector. */
#include "p4pc/golden_cmm.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main(void)
{
    const P4GoldenDomainShape shape = {624, 472, 368, 2560, 256};
    P4GoldenDomainMap map;
    uint8_t dat[512] = {0}, out[480];
    char reason[128];
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    dat[0] = 0xe0; dat[1] = 1; dat[4] = 10;
    dat[8] = 1; dat[10] = 2;
    dat[12] = 0xb8; dat[13] = 1; /* Golden-only active 440 -> 576 */
    dat[14] = 0xd8; dat[15] = 1; /* Golden passive 472 -> 440 */
    CHECK(!p4_golden_cmm_rankup_convert(&map, dat, sizeof dat, out, reason, sizeof reason));
    CHECK(out[0] == 1 && out[2] == 2 && out[4] == 0x40 && out[5] == 2);
    CHECK(out[6] == 0xb8 && out[7] == 1);
    memset(out, 0x55, sizeof out);
    dat[12] = 0xff; dat[13] = 0xff;
    CHECK(p4_golden_cmm_rankup_convert(&map, dat, sizeof dat, out, reason, sizeof reason) == -1 && out[0] == 0x55);
    dat[4] = 9;
    CHECK(p4_golden_cmm_rankup_convert(&map, dat, sizeof dat, out, reason, sizeof reason) == -1);
    puts("golden_cmm: ok");
    return 0;
}
