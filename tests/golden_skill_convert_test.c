/* Synthetic SKILL.TBL row projections. */
#include "p4pc/golden_skill_convert.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main(void)
{
    const P4GoldenDomainShape shape = {624, 472, 368, 2560, 256};
    P4GoldenDomainMap map;
    static uint8_t elements[624 * 2], element_out[624 * 2], rows[472 * 44], row_out[608 * 40];
    size_t written;
    char reason[128];
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    elements[440 * 2] = 11; elements[472 * 2] = 22;
    CHECK(!p4_golden_skill_elements(&map, elements, sizeof elements, element_out, sizeof element_out, &written, reason, sizeof reason));
    CHECK(written == sizeof element_out && element_out[576 * 2] == 11 && element_out[440 * 2] == 22);
    for (unsigned i = 0; i < 44; ++i) rows[440 * 44 + i] = (uint8_t)i;
    CHECK(!p4_golden_skill_metadata(&map, rows, sizeof rows, row_out, sizeof row_out, &written, reason, sizeof reason));
    CHECK(written == sizeof row_out && row_out[576 * 40] == 0 && row_out[576 * 40 + 1] == 4);
    CHECK(row_out[576 * 40 + 4] == 8 && row_out[576 * 40 + 39] == 43);
    memset(row_out, 0x55, sizeof row_out);
    CHECK(p4_golden_skill_metadata(&map, rows, sizeof rows, row_out, sizeof row_out - 1, &written, reason, sizeof reason) == -1);
    CHECK(row_out[0] == 0x55);
    CHECK(p4_golden_skill_elements(&map, elements, sizeof elements - 1, element_out, sizeof element_out, &written, reason, sizeof reason) == -1);
    puts("golden_skill_convert: ok");
    return 0;
}
