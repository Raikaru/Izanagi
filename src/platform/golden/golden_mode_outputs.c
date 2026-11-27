/* Pack only specified projections; unresolved inputs remain absent and cannot publish. */
#include "golden_mode_outputs.h"
#include "p4pc/game_table_inputs.h"
#include "p4pc/golden_names_native.h"
#include "p4pc/golden_skill_convert.h"
#include "golden_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static const struct { const char *name; P4GoldenDomain domain; } extra_names[] = {
    {"decl/enemy-affinity",P4_GOLDEN_DOMAIN_ENEMY}, {"decl/enemy-ai",P4_GOLDEN_DOMAIN_ENEMY},
    {"decl/enemy-metadata",P4_GOLDEN_DOMAIN_ENEMY}, {"decl/enemy-model",P4_GOLDEN_DOMAIN_ENEMY},
    {"decl/enemy-model-source",P4_GOLDEN_DOMAIN_ENEMY}, {"decl/item-icons",P4_GOLDEN_DOMAIN_ITEM},
    {"decl/item-metadata",P4_GOLDEN_DOMAIN_ITEM}, {"decl/item-names",P4_GOLDEN_DOMAIN_COUNT},
    {"decl/persona-rows",P4_GOLDEN_DOMAIN_PERSONA},
    {"decl/skill-aux",P4_GOLDEN_DOMAIN_SKILL}, {"decl/skill-elements",P4_GOLDEN_DOMAIN_SKILL},
    {"decl/skill-groups",P4_GOLDEN_DOMAIN_SKILL}, {"decl/skill-metadata",P4_GOLDEN_DOMAIN_SKILL},
    {"decl/skill-presentation",P4_GOLDEN_DOMAIN_SKILL}, {"decl/skill-rates",P4_GOLDEN_DOMAIN_SKILL},
    {"decl/text",P4_GOLDEN_DOMAIN_COUNT},
    {"golden-ai/item",P4_GOLDEN_DOMAIN_COUNT}, {"golden-ai/levels",P4_GOLDEN_DOMAIN_COUNT},
    {"golden-ai/persona",P4_GOLDEN_DOMAIN_COUNT},
    {"golden-ai/programs",P4_GOLDEN_DOMAIN_COUNT}, {"golden-ai/skill",P4_GOLDEN_DOMAIN_COUNT},
    {"golden-ai/unit",P4_GOLDEN_DOMAIN_COUNT},
    {"items/cmp-tables",P4_GOLDEN_DOMAIN_COUNT},
    {"rows/effect0",P4_GOLDEN_DOMAIN_COUNT}, {"rows/skill1",P4_GOLDEN_DOMAIN_COUNT}
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
    for (unsigned i = 0; i < sizeof extra_names / sizeof extra_names[0]; ++i) {
        if (extra_names[i].domain != P4_GOLDEN_DOMAIN_COUNT &&
            !p4_golden_domain_expanded_count(map, extra_names[i].domain)) continue;
        const char *name = extra_names[i].name;
        built.extra[built.extra_count++].name = name;
        built.tables[built.table_count++].name = name;
    }
    P4GoldenCacheTable images[P4_GOLDEN_IMAGE_EXTRA_COUNT];
    if (p4_golden_image_encode(artifacts, images, &built.owned[9], &built.owned[10])) goto invalid_built;
    for (unsigned i = 0; i < P4_GOLDEN_IMAGE_EXTRA_COUNT; ++i) {
        built.extra[built.extra_count++].name = images[i].name;
        built.tables[built.table_count++] = images[i];
    }
    qsort(built.extra, built.extra_count, sizeof built.extra[0], compare);
    qsort(built.tables, built.table_count, sizeof built.tables[0], compare);

    const P4GoldenTable *unit = &source->tables[P4_GOLDEN_UNIT];
    const P4GoldenTable *persona = &source->tables[P4_GOLDEN_PERSONA];
    const P4GoldenTable *item = &source->tables[P4_GOLDEN_ITEM];
    const P4GoldenTable *msg = &source->tables[P4_GOLDEN_MSG];
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
        msg->span_count < 5 || !msg->spans[0].bytes || !msg->spans[4].bytes ||
        !p->skill_names.data || p->skill_names.size != skill_count * P4_GOLDEN_SKILL_NAME_SIZE ||
        !p->enemy_names.data || p->enemy_names.size != (size_t)map->shape.enemies * P4_ENEMY_NAME_SIZE ||
        !p->persona_names.data || p->persona_names.size != (size_t)map->shape.personas * P4_GOLDEN_PERSONA_NAME_SIZE ||
        !p->item_names.data || p->item_names.size != (size_t)map->shape.items * 24u ||
        ai->span_count < 9 || !ai->spans[8].bytes || ai->spans[8].size != (size_t)map->shape.enemies * P4_ENEMY_AI_SIZE ||
        artifacts->inheritance_size != skill_count) goto invalid_built;
    P4GoldenSpan skill[] = {{p->skill_elements.data, P4_SKILL_RETAIL_COUNT * 2u},
                            {p->skill_metadata.data, P4_SKILL_RETAIL_ACTIVE_COUNT * P4_SKILL_ROW_SIZE}};
    P4GoldenSpan enemy[] = {{p->enemy_metadata.data, P4_ENEMY_RETAIL_ROWS * P4_ENEMY_ROW_SIZE},
                            {unit->spans[1].bytes, P4_ENEMY_RETAIL_ROWS * P4_ENEMY_AFFINITY_SIZE}, unit->spans[2]};
    P4GoldenSpan personas[] = {persona->spans[0], {p->persona[0].data,p->persona[0].size},
        {p->persona[1].data,p->persona[1].size}, persona->spans[3],
        {p->persona[2].data,p->persona[2].size}, persona->spans[5]};
    P4GoldenSpan names[] = {msg->spans[0], {p->skill_names.data,p->skill_names.size},
        {p->enemy_names.data,p->enemy_names.size}, {p->persona_names.data,p->persona_names.size}, msg->spans[4]};
    P4GoldenSpan encounters[5];
    for (unsigned i = 0; i < 5; ++i) encounters[i] = (P4GoldenSpan){p->encounter[i].data,p->encounter[i].size};
    if (set_packed(&built,0,P4_TABLE_INPUT_SKILL,skill,2) || set_packed(&built,1,P4_TABLE_INPUT_UNIT,enemy,3) ||
        set_packed(&built,2,P4_TABLE_INPUT_PERSONA,personas,6) || set_packed(&built,3,P4_TABLE_INPUT_ENCOUNT,encounters,5) ||
        set_packed(&built,6,P4_TABLE_INPUT_MSG,names,5))
        goto invalid_built;
    /* Constants and opaque BF are retained exactly. Only the legacy enemy-row
     * prefix is installed here; expanded rows remain in decl/enemy-ai. */
    if (ai->span_count == 11) {
        P4GoldenVmProgram program;
        for (unsigned i = 9; i < 11; ++i)
            if (p4_golden_vm_program(&program, ai->spans[i].bytes, ai->spans[i].size))
                goto invalid_built;
        P4GoldenSpan spans[11];
        memcpy(spans, ai->spans, sizeof spans);
        spans[8].size = P4_ENEMY_RETAIL_ROWS * P4_ENEMY_AI_SIZE;
        if (set_packed(&built,14,P4_TABLE_INPUT_AICALC,spans,11)) goto invalid_built;
    }
    const P4GoldenTable *raw_skill = &source->tables[P4_GOLDEN_SKILL];
    /* Private cache objects retain original ID domains, widths and flags.
     * Deliberately absent synthetic inputs remain absent, never fabricated. */
    if (item->bytes && persona->bytes && ai->bytes && raw_skill->bytes && unit->bytes &&
        (p4_golden_mode_outputs_set(&built,"golden-ai/item",item->bytes,item->size) ||
         p4_golden_mode_outputs_set(&built,"golden-ai/persona",persona->bytes,persona->size) ||
         p4_golden_mode_outputs_set(&built,"golden-ai/programs",ai->bytes,ai->size) ||
         p4_golden_mode_outputs_set(&built,"golden-ai/skill",raw_skill->bytes,raw_skill->size) ||
         p4_golden_mode_outputs_set(&built,"golden-ai/unit",unit->bytes,unit->size))) goto invalid_built;
    if (p4_golden_mode_outputs_set(&built,"golden-ai/levels",(uint8_t *)artifacts->levels,
        sizeof artifacts->levels)) goto invalid_built;
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
    if (extra->tables[P4_GOLDEN_EXTRA_CMP].bytes &&
        p4_golden_mode_outputs_set(&built,"items/cmp-tables",
            extra->tables[P4_GOLDEN_EXTRA_CMP].bytes,extra->tables[P4_GOLDEN_EXTRA_CMP].size))
        goto invalid_built;
    if (p4_golden_mode_outputs_set(&built,"decl/item-names",p->item_names.data,p->item_names.size))
        goto invalid_built;
    if (p4_golden_domain_expanded_count(map,P4_GOLDEN_DOMAIN_SKILL) &&
        (p4_golden_mode_outputs_set(&built,"decl/skill-aux",built.owned[5],skill_count) ||
         p4_golden_mode_outputs_set(&built,"decl/skill-metadata",p->skill_metadata.data,p->skill_metadata.size) ||
         p4_golden_mode_outputs_set(&built,"decl/skill-rates",(uint8_t *)artifacts->inheritance,skill_count)))
        goto invalid_built;
    if (p4_golden_domain_expanded_count(map,P4_GOLDEN_DOMAIN_SKILL)) {
        built.owned[7] = malloc(skill_count * 4u);
        built.owned[8] = malloc(skill_count);
        if (!built.owned[7] || !built.owned[8] ||
            p4_golden_skill_semantics(p->skill_elements.data, skill_count,
                built.owned[7], skill_count * 4u, built.owned[8], skill_count) ||
            p4_golden_mode_outputs_set(&built,"decl/skill-elements",built.owned[7],skill_count * 4u) ||
            p4_golden_mode_outputs_set(&built,"decl/skill-groups",built.owned[8],skill_count))
            goto invalid_built;
    }
    if (p4_golden_domain_expanded_count(map,P4_GOLDEN_DOMAIN_ENEMY) &&
        (p4_golden_mode_outputs_set(&built,"decl/enemy-metadata",p->enemy_metadata.data,p->enemy_metadata.size) ||
         p4_golden_mode_outputs_set(&built,"decl/enemy-affinity",(uint8_t *)unit->spans[1].bytes,unit->spans[1].size) ||
         p4_golden_mode_outputs_set(&built,"decl/enemy-ai",(uint8_t *)ai->spans[8].bytes,ai->spans[8].size)))
        goto invalid_built;
    if (p4_golden_domain_expanded_count(map,P4_GOLDEN_DOMAIN_ITEM) &&
        p4_golden_mode_outputs_set(&built,"decl/item-metadata",p->item_metadata.data,p->item_metadata.size))
        goto invalid_built;
    const P4GoldenExtraTable *model = &extra->tables[P4_GOLDEN_EXTRA_MODEL];
    if (model->span_count) {
        /* Keep MODEL0 one contiguous raw block, not eleven projected rows.
         * The native packed loader supplies its actual guest backing. */
        if (model->span_count != 3 || !model->spans[0].bytes ||
            model->spans[0].size != 11u * 382u || !model->spans[1].bytes ||
            model->spans[1].size != (size_t)map->shape.enemies * 232u ||
            !model->spans[2].bytes || model->spans[2].size != 256u * 88u ||
            set_packed(&built,11,P4_TABLE_INPUT_MODEL,model->spans,3))
            goto invalid_built;
    }
    if (p->skill_golden.bytes) {
        if (p->skill_golden.size != (size_t)map->shape.active_skills * 44u ||
            p4_golden_mode_outputs_set(&built,"rows/skill1",(uint8_t *)p->skill_golden.bytes,p->skill_golden.size))
            goto invalid_built;
    }
    const P4GoldenExtraTable *effect = &extra->tables[P4_GOLDEN_EXTRA_EFFECT];
    if (effect->span_count) {
        size_t active_count = p->skill_metadata.size / P4_SKILL_ROW_SIZE;
        if (effect->span_count != 1 || !effect->spans[0].bytes ||
            effect->spans[0].size != (size_t)map->shape.active_skills * 12u ||
            !(built.owned[12] = calloc(active_count, 4u))) goto invalid_built;
        for (uint32_t id = 0; id < map->shape.active_skills; ++id) {
            uint32_t port;
            if (p4_golden_domain_to_port(map,P4_GOLDEN_DOMAIN_SKILL,id,&port,NULL) || port >= active_count)
                goto invalid_built;
            const uint8_t *raw = effect->spans[0].bytes + id * 12u;
            uint8_t *native = built.owned[12] + port * 4u;
            memcpy(native,raw,2); memcpy(native + 2,raw + 4,2);
        }
        P4GoldenSpan prefix = {built.owned[12],P4_SKILL_RETAIL_ACTIVE_COUNT * 4u};
        if (set_packed(&built,13,P4_TABLE_INPUT_EFFECT,&prefix,1) ||
            p4_golden_mode_outputs_set(&built,"rows/effect0",(uint8_t *)effect->spans[0].bytes,effect->spans[0].size) ||
            (p4_golden_domain_expanded_count(map,P4_GOLDEN_DOMAIN_SKILL) &&
             p4_golden_mode_outputs_set(&built,"decl/skill-presentation",built.owned[12],active_count * 4u)))
            goto invalid_built;
    }
    *out = built;
    if (reason && capacity) reason[0] = 0;
    return 0;
invalid_built:
    p4_golden_mode_outputs_free(&built);
invalid:
    if (reason && capacity) snprintf(reason, capacity, "invalid or unavailable approved Golden output rows");
    return -1;
}
