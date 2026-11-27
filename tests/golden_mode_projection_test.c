/* Entire synthetic Golden-shaped banks; no game data. */
#include "../src/platform/golden/golden_mode_projection.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s (%s)\n", __LINE__, #x, reason); return 1; } } while (0)
static uint8_t elements[624 * 2], skills[472 * 44], enemies[368 * 60];
static uint8_t skill_names[624 * 23], enemy_names[368 * 21], persona_names[256 * 21];
static uint8_t persona1[256 * 70], persona2[24 * 622], persona4[108 * 4];
static uint8_t enc0[944 * 24], enc1[180 * 10], enc2[200], enc3[68 * 252], enc4[84 * 348];
static uint8_t item[32 + 2560 * (68 + 24)];
static void put32(uint8_t *p, unsigned v)
{ p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static void span(P4GoldenTable *t, unsigned i, uint8_t *bytes, size_t size)
{ t->spans[i].bytes = bytes; t->spans[i].size = size; }
int main(void)
{
    P4GoldenSource source = {0}; P4GoldenModeProjection out = {0}; P4GoldenDomainMap map;
    const P4GoldenDomainShape shape = {624, 472, 368, 2560, 256};
    char reason[192] = "";
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    P4GoldenTable *s = &source.tables[P4_GOLDEN_SKILL], *u = &source.tables[P4_GOLDEN_UNIT];
    P4GoldenTable *m = &source.tables[P4_GOLDEN_MSG], *p = &source.tables[P4_GOLDEN_PERSONA];
    P4GoldenTable *e = &source.tables[P4_GOLDEN_ENCOUNT], *it = &source.tables[P4_GOLDEN_ITEM];
    s->span_count = 2; span(s, 0, elements, sizeof elements); span(s, 1, skills, sizeof skills);
    u->span_count = 1; span(u, 0, enemies, sizeof enemies);
    m->span_count = 4; span(m, 1, skill_names, sizeof skill_names); span(m, 2, enemy_names, sizeof enemy_names);
    span(m, 3, persona_names, sizeof persona_names);
    p->span_count = 5; span(p, 1, persona1, sizeof persona1); span(p, 2, persona2, sizeof persona2);
    span(p, 4, persona4, sizeof persona4);
    e->span_count = 5; span(e, 0, enc0, sizeof enc0); span(e, 1, enc1, sizeof enc1);
    span(e, 2, enc2, sizeof enc2); span(e, 3, enc3, sizeof enc3); span(e, 4, enc4, sizeof enc4);
    it->bytes = item; it->size = sizeof item;
    put32(item, 2560); put32(item + 4, 1); put32(item + 8, 256); put32(item + 12, 1);
    uint8_t *second = item + 16 + 2560 * 68;
    put32(second, 2560); put32(second + 4, 1); put32(second + 8, 256); put32(second + 12, 1);
    skills[440 * 44] = 3; elements[440 * 2] = 2;
    CHECK(!p4_golden_mode_project(&map, &source, &out, reason, sizeof reason));
    CHECK(out.skill_metadata.size == 608u * 40u && out.skill_metadata.data[576u * 40u] == 3);
    CHECK(out.skill_elements.size == 624u * 2u && out.skill_elements.data[576u * 2u] == 2);
    CHECK(out.encounter[3].size == 68u * 248u && out.item_metadata.size == 2560u * 68u);
    p4_golden_mode_projection_free(&out);
    memset(skill_names, 'x', 22); skill_names[22] = 0;
    memset(persona_names, 'y', 20); persona_names[20] = 0;
    CHECK(!p4_golden_mode_project(&map, &source, &out, reason, sizeof reason));
    CHECK(out.skill_names.size == 624u * 23u && out.persona_names.size == 256u * 21u);
    CHECK(!memcmp(out.skill_names.data, skill_names, 23) && !memcmp(out.persona_names.data, persona_names, 21));
    p4_golden_mode_projection_free(&out);
    memset(skill_names, 'x', 23);
    CHECK(p4_golden_mode_project(&map, &source, &out, reason, sizeof reason) == -1);
    CHECK(strstr(reason, "name does not fit") && !out.skill_metadata.data && !out.item_names.data);
    memset(skill_names, 0, 23);
    skills[440 * 44 + 1] = 0x10; skills[440 * 44 + 7] = 0xA5;
    skills[10 * 44 + 1] = 0x1E; skills[10 * 44 + 2] = 0x81; skills[10 * 44 + 3] = 0x42;
    CHECK(!p4_golden_mode_project(&map, &source, &out, reason, sizeof reason));
    CHECK(out.skill_golden.bytes == skills && out.skill_golden.size == sizeof skills);
    CHECK(out.skill_golden.bytes[440 * 44 + 1] == 0x10 && out.skill_golden.bytes[440 * 44 + 7] == 0xA5);
    CHECK(!memcmp(out.skill_golden.bytes + 10 * 44 + 1, "\x1e\x81\x42", 3));
    CHECK(out.skill_metadata.data[576 * 40] == 3);
    p4_golden_mode_projection_free(&out);
    puts("golden_mode_projection: ok");
    return 0;
}
