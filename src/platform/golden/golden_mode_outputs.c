/* Pack only specified projections; unresolved inputs remain absent and cannot publish. */
#include "golden_mode_outputs.h"
#include "p4pc/game_table_inputs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static const struct { const char *name; P4GoldenDomain domain; } extra_names[] = {
    {"decl/enemy-affinity",P4_GOLDEN_DOMAIN_ENEMY}, {"decl/enemy-ai",P4_GOLDEN_DOMAIN_ENEMY},
    {"decl/enemy-metadata",P4_GOLDEN_DOMAIN_ENEMY}, {"decl/enemy-model",P4_GOLDEN_DOMAIN_ENEMY},
    {"decl/enemy-model-source",P4_GOLDEN_DOMAIN_ENEMY}, {"decl/item-icons",P4_GOLDEN_DOMAIN_ITEM},
    {"decl/item-metadata",P4_GOLDEN_DOMAIN_ITEM}, {"decl/persona-rows",P4_GOLDEN_DOMAIN_PERSONA},
    {"decl/skill-aux",P4_GOLDEN_DOMAIN_SKILL}, {"decl/skill-elements",P4_GOLDEN_DOMAIN_SKILL},
    {"decl/skill-groups",P4_GOLDEN_DOMAIN_SKILL}, {"decl/skill-metadata",P4_GOLDEN_DOMAIN_SKILL},
    {"decl/skill-presentation",P4_GOLDEN_DOMAIN_SKILL}, {"decl/skill-rates",P4_GOLDEN_DOMAIN_SKILL},
    {"decl/text",P4_GOLDEN_DOMAIN_COUNT}
};
static int compare(const void *a, const void *b)
{ return strcmp(((const P4GoldenCacheTable *)a)->name, ((const P4GoldenCacheTable *)b)->name); }
static void word(uint8_t *p, uint32_t n)
{ p[0]=(uint8_t)n; p[1]=(uint8_t)(n>>8); p[2]=(uint8_t)(n>>16); p[3]=(uint8_t)(n>>24); }
static int packed(const P4GoldenSpan *spans, size_t count, uint8_t **out, size_t *size)
{
    size_t total = 0;
    for (size_t i = 0; i < count; ++i) {
        if (!spans[i].bytes || !spans[i].size || spans[i].size > UINT32_MAX || spans[i].size > SIZE_MAX - 19u)
            return -1;
        size_t n = (spans[i].size + 19u) & ~(size_t)15u;
        if (n > SIZE_MAX - total) return -1;
        total += n;
    }
    uint8_t *data = calloc(1, total);
    if (!data) return -1;
    size_t offset = 0;
    for (size_t i = 0; i < count; ++i) {
        word(data + offset, (uint32_t)spans[i].size);
        memcpy(data + offset + 4, spans[i].bytes, spans[i].size);
        offset += (spans[i].size + 19u) & ~(size_t)15u;
    }
    *out = data; *size = total; return 0;
}
int p4_golden_mode_outputs_set(P4GoldenModeOutputs *out, const char *name, uint8_t *data, size_t size)
{
    if (!out || !name || !data || !size) return -1;
    for (size_t i = 0; i < out->table_count; ++i)
        if (out->tables[i].name && !strcmp(name, out->tables[i].name)) {
            if (out->tables[i].data) return -1;
            out->tables[i].data = data; out->tables[i].size = size; return 0;
        }
    return -1;
}
void p4_golden_mode_outputs_free(P4GoldenModeOutputs *out)
{
    if (!out) return;
    for (size_t i = 0; i < sizeof out->owned / sizeof out->owned[0]; ++i) free(out->owned[i]);
    memset(out, 0, sizeof *out);
}
static int set_packed(P4GoldenModeOutputs *out, unsigned owner, P4GameTableInputKind kind,
                      const P4GoldenSpan *spans, size_t count)
{
    size_t size;
    if (packed(spans, count, &out->owned[owner], &size)) return -1;
    return p4_golden_mode_outputs_set(out, p4_game_table_input_label(kind), out->owned[owner], size);
}
int p4_golden_mode_outputs_prepare(P4GoldenModeOutputs *out, const P4GoldenDomainMap *map,
    const P4GoldenSource *source, const P4GoldenExtraSource *extra, const P4GoldenModeProjection *p,
    const P4GoldenModeIdentityArtifacts *artifacts, char *reason, size_t capacity)
{
    P4GoldenModeOutputs built = {0};
    if (!out || !map || !source || !extra || !p || !artifacts) goto invalid;
    built.table_count = P4_GOLDEN_OUTPUT_NATIVE_COUNT;
    for (unsigned i = 0; i < P4_GOLDEN_OUTPUT_NATIVE_COUNT; ++i)
        built.tables[i].name = p4_game_table_input_label((P4GameTableInputKind)i);
    for (unsigned i = 0; i < P4_GOLDEN_OUTPUT_EXTRA_COUNT; ++i) {
        if (extra_names[i].domain != P4_GOLDEN_DOMAIN_COUNT &&
            !p4_golden_domain_expanded_count(map, extra_names[i].domain)) continue;
        const char *name = extra_names[i].name;
        built.extra[built.extra_count++].name = name;
        built.tables[built.table_count++].name = name;
    }
    qsort(built.tables, built.table_count, sizeof built.tables[0], compare);

    const P4GoldenTable *unit = &source->tables[P4_GOLDEN_UNIT];
    const P4GoldenTable *persona = &source->tables[P4_GOLDEN_PERSONA];
    const P4GoldenTable *item = &source->tables[P4_GOLDEN_ITEM];
    const P4GoldenExtraTable *ai = &extra->tables[P4_GOLDEN_EXTRA_AICALC];
    size_t skill_count = p->skill_elements.size / 2u;
    if (!p->skill_elements.data || p->skill_elements.size % 2u || skill_count < P4_SKILL_RETAIL_COUNT ||
        !p->skill_metadata.data || p->skill_metadata.size < P4_SKILL_RETAIL_ACTIVE_COUNT * P4_SKILL_ROW_SIZE ||
        !p->enemy_metadata.data || p->enemy_metadata.size != (size_t)map->shape.enemies * P4_ENEMY_ROW_SIZE ||
        !p->item_metadata.data || p->item_metadata.size != (size_t)map->shape.items * P4_ITEM_ROW_SIZE ||
        unit->span_count < 3 || !unit->spans[1].bytes ||
        unit->spans[1].size != (size_t)map->shape.enemies * P4_ENEMY_AFFINITY_SIZE ||
        !unit->spans[2].bytes || unit->spans[2].size != (size_t)map->shape.personas * P4_ENEMY_AFFINITY_SIZE ||
        persona->span_count < 6 || !item->bytes || item->size < P4_ITEM_FILE_HEADER_SIZE ||
        ai->span_count < 9 || !ai->spans[8].bytes || ai->spans[8].size != (size_t)map->shape.enemies * P4_ENEMY_AI_SIZE ||
        artifacts->inheritance_size != skill_count) goto invalid_built;
    P4GoldenSpan skill[] = {{p->skill_elements.data, P4_SKILL_RETAIL_COUNT * 2u},
                            {p->skill_metadata.data, P4_SKILL_RETAIL_ACTIVE_COUNT * P4_SKILL_ROW_SIZE}};
    P4GoldenSpan enemy[] = {{p->enemy_metadata.data, P4_ENEMY_RETAIL_ROWS * P4_ENEMY_ROW_SIZE},
                            {unit->spans[1].bytes, P4_ENEMY_RETAIL_ROWS * P4_ENEMY_AFFINITY_SIZE}, unit->spans[2]};
    P4GoldenSpan personas[] = {persona->spans[0], {p->persona[0].data,p->persona[0].size},
        {p->persona[1].data,p->persona[1].size}, persona->spans[3],
        {p->persona[2].data,p->persona[2].size}, persona->spans[5]};
    P4GoldenSpan encounters[5];
    for (unsigned i = 0; i < 5; ++i) encounters[i] = (P4GoldenSpan){p->encounter[i].data,p->encounter[i].size};
    if (set_packed(&built,0,P4_TABLE_INPUT_SKILL,skill,2) || set_packed(&built,1,P4_TABLE_INPUT_UNIT,enemy,3) ||
        set_packed(&built,2,P4_TABLE_INPUT_PERSONA,personas,6) || set_packed(&built,3,P4_TABLE_INPUT_ENCOUNT,encounters,5))
        goto invalid_built;
    size_t item_size = P4_ITEM_FILE_HEADER_SIZE + P4_ITEM_RETAIL_COUNT * P4_ITEM_ROW_SIZE;
    built.owned[4] = malloc(item_size);
    built.owned[5] = malloc(skill_count);
    if (!built.owned[4] || !built.owned[5]) goto invalid_built;
    memcpy(built.owned[4], item->bytes, P4_ITEM_FILE_HEADER_SIZE);
    word(built.owned[4], P4_ITEM_RETAIL_COUNT);
    memcpy(built.owned[4] + P4_ITEM_FILE_HEADER_SIZE, p->item_metadata.data, item_size - P4_ITEM_FILE_HEADER_SIZE);
    for (size_t i = 0; i < skill_count; ++i) built.owned[5][i] = p->skill_elements.data[i * 2u + 1u];
    if (p4_golden_mode_outputs_set(&built,p4_game_table_input_label(P4_TABLE_INPUT_ITEM),built.owned[4],item_size))
        goto invalid_built;
    if (p4_golden_domain_expanded_count(map,P4_GOLDEN_DOMAIN_SKILL) &&
        (p4_golden_mode_outputs_set(&built,"decl/skill-aux",built.owned[5],skill_count) ||
         p4_golden_mode_outputs_set(&built,"decl/skill-metadata",p->skill_metadata.data,p->skill_metadata.size) ||
         p4_golden_mode_outputs_set(&built,"decl/skill-rates",(uint8_t *)artifacts->inheritance,skill_count)))
        goto invalid_built;
    if (p4_golden_domain_expanded_count(map,P4_GOLDEN_DOMAIN_ENEMY) &&
        (p4_golden_mode_outputs_set(&built,"decl/enemy-metadata",p->enemy_metadata.data,p->enemy_metadata.size) ||
         p4_golden_mode_outputs_set(&built,"decl/enemy-affinity",(uint8_t *)unit->spans[1].bytes,unit->spans[1].size) ||
         p4_golden_mode_outputs_set(&built,"decl/enemy-ai",(uint8_t *)ai->spans[8].bytes,ai->spans[8].size)))
        goto invalid_built;
    if (p4_golden_domain_expanded_count(map,P4_GOLDEN_DOMAIN_ITEM) &&
        p4_golden_mode_outputs_set(&built,"decl/item-metadata",p->item_metadata.data,p->item_metadata.size))
        goto invalid_built;
    *out = built;
    if (reason && capacity) reason[0] = 0;
    return 0;
invalid_built:
    p4_golden_mode_outputs_free(&built);
invalid:
    if (reason && capacity) snprintf(reason, capacity, "invalid or unavailable approved Golden output rows");
    return -1;
}
