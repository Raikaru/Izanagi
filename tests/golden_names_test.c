/* Synthetic fixed-width name banks. */
#include "p4pc/golden_names.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main(void)
{
    const P4GoldenDomainShape shape = {624, 472, 368, 2560, 256};
    P4GoldenDomainMap map;
    static uint8_t in[624 * 23], out[624 * 19];
    size_t written;
    char reason[128];
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    memcpy(in + 440 * 23, "new", 4); memcpy(in + 472 * 23, "passive", 8);
    CHECK(!p4_golden_names_convert(&map, P4_GOLDEN_DOMAIN_SKILL, in, sizeof in, 624, 23, 19,
                                   out, sizeof out, &written, reason, sizeof reason));
    CHECK(written == sizeof out && !strcmp((char *)out + 576 * 19, "new") && !strcmp((char *)out + 440 * 19, "passive"));
    memset(out, 0x55, sizeof out); memset(in, 'x', 19); in[19] = 0;
    CHECK(p4_golden_names_convert(&map, P4_GOLDEN_DOMAIN_SKILL, in, sizeof in, 624, 23, 19,
                                  out, sizeof out, &written, reason, sizeof reason) == -1 && out[0] == 0x55);
    puts("golden_names: ok");
    return 0;
}
