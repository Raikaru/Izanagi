/* Synthetic ITEMTBL sections; no game data. */
#include "p4pc/golden_item_convert.h"
#include <stdio.h>
#include <string.h>
#define COUNT 1536u
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (0)
static void put32(uint8_t *p, uint32_t n) { p[0] = (uint8_t)n; p[1] = (uint8_t)(n >> 8); p[2] = (uint8_t)(n >> 16); p[3] = (uint8_t)(n >> 24); }
static void header(uint8_t *p) { put32(p, COUNT); put32(p + 4, 1); put32(p + 8, 256); put32(p + 12, 1); }
int main(void)
{
    const P4GoldenDomainShape shape = {624, 472, 336, COUNT, 256};
    P4GoldenDomainMap map;
    static uint8_t input[32 + COUNT * 92], rows[COUNT * 68], names[COUNT * 24];
    char reason[128];
    size_t second = 16 + COUNT * 68;
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    header(input); header(input + second);
    put32(input + 16 + 0x24, 440);
    memcpy(input + second + 16, "Synthetic", 10);
    CHECK(!p4_golden_items_convert(&map, input, sizeof input, rows, sizeof rows, names, sizeof names, reason, sizeof reason));
    CHECK(rows[0x24] == 0x40 && rows[0x25] == 2 && !strcmp((char *)names, "Synthetic"));
    memset(rows, 0x55, sizeof rows);
    put32(input + 16 + 0x24, 9000);
    CHECK(p4_golden_items_convert(&map, input, sizeof input, rows, sizeof rows, names, sizeof names, reason, sizeof reason) == -1 && rows[0] == 0x55);
    put32(input + 16 + 0x24, 0);
    input[second + 16] = 1;
    CHECK(p4_golden_items_convert(&map, input, sizeof input, rows, sizeof rows, names, sizeof names, reason, sizeof reason) == -1);
    puts("golden_item_convert: ok");
    return 0;
}
