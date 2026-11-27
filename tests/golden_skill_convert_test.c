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
    rows[440 * 44 + 1] = rows[440 * 44 + 2] = rows[440 * 44 + 3] = rows[440 * 44 + 7] = 0;
    CHECK(!p4_golden_skill_metadata(&map, rows, sizeof rows, row_out, sizeof row_out, &written, reason, sizeof reason));
    CHECK(written == sizeof row_out && row_out[576 * 40] == 0 && row_out[576 * 40 + 1] == 4);
    CHECK(row_out[576 * 40 + 4] == 8 && row_out[576 * 40 + 39] == 43);
    memset(row_out, 0x55, sizeof row_out);
    CHECK(p4_golden_skill_metadata(&map, rows, sizeof rows, row_out, sizeof row_out - 1, &written, reason, sizeof reason) == -1);
    CHECK(row_out[0] == 0x55);
    rows[10 * 44 + 1] = 0x10; rows[10 * 44 + 2] = 0x80; rows[10 * 44 + 3] = 0xA5;
    rows[10 * 44 + 7] = 0xC3;
    CHECK(!p4_golden_skill_metadata(&map, rows, sizeof rows, row_out, sizeof row_out, &written, reason, sizeof reason));
    CHECK(written == sizeof row_out && row_out[10 * 40] == rows[10 * 44]);
    CHECK(rows[10 * 44 + 1] == 0x10 && rows[10 * 44 + 2] == 0x80 &&
          rows[10 * 44 + 3] == 0xA5 && rows[10 * 44 + 7] == 0xC3);
    CHECK(p4_golden_skill_elements(&map, elements, sizeof elements - 1, element_out, sizeof element_out, &written, reason, sizeof reason) == -1);
    const uint8_t semantic_rows[] = {0xFF, 0x01, 0xFF, 0xA5, 0, 1, 18, 0};
    uint8_t ids[16], groups[4];
    CHECK(!p4_golden_skill_semantics(semantic_rows, 4, ids, sizeof ids, groups, sizeof groups));
    CHECK(!memcmp(ids, "\xff\xff\xff\xff\xff\xff\xff\xff", 8));
    CHECK(ids[8] == 0 && ids[12] == 18 && groups[0] == 18 && groups[1] == 18 && groups[2] == 0);
    const uint8_t unknown[] = {0xFE, 1};
    memset(ids, 0x55, sizeof ids); memset(groups, 0x55, sizeof groups);
    CHECK(p4_golden_skill_semantics(unknown, 1, ids, sizeof ids, groups, sizeof groups) == -1);
    CHECK(ids[0] == 0x55 && groups[0] == 0x55);
    CHECK(p4_golden_skill_semantics(semantic_rows, 4, ids, 15, groups, sizeof groups) == -1);
    CHECK(ids[0] == 0x55 && groups[0] == 0x55);
    puts("golden_skill_convert: ok");
    return 0;
}
