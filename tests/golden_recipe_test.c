/* Synthetic special-fusion rows only. */
#include "p4pc/golden_recipe.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main(void)
{
    const P4GoldenDomainShape shape = {624, 472, 368, 2560, 256};
    P4GoldenDomainMap map;
    uint8_t row[44] = {0}, out[32];
    char reason[128];
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    row[0] = 40; row[12] = 10; row[14] = 20;
    CHECK(!p4_golden_recipe_convert(&map, row, sizeof row, 2, out, reason, sizeof reason));
    CHECK(out[0] == 40 && out[8] == 10 && out[10] == 20 && !out[12]);
    for (unsigned i = 2; i < 12; ++i) row[12 + i * 2] = (uint8_t)(i + 1);
    memset(out, 0x55, sizeof out);
    CHECK(p4_golden_recipe_convert(&map, row, sizeof row, 12, out, reason, sizeof reason) == 1);
    CHECK(strstr(reason, "eight") && out[0] == 0x55);
    row[2] = 1;
    CHECK(p4_golden_recipe_convert(&map, row, sizeof row, 12, out, reason, sizeof reason) == -1);
    row[2] = 0; row[12] = 0;
    CHECK(p4_golden_recipe_convert(&map, row, sizeof row, 12, out, reason, sizeof reason) == -1);
    puts("golden_recipe: ok");
    return 0;
}
