/* Compose the independently specified M5 converters without a retail base. */
#include "golden_mode_projection.h"
#include "p4pc/golden_skill_convert.h"
#include "p4pc/golden_enemy_convert.h"
#include "p4pc/golden_item_convert.h"
#include "p4pc/golden_persona_convert.h"
#include "p4pc/golden_encounter_convert.h"
#include "p4pc/golden_names_native.h"
#include "p4pc/golden_names.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(char *reason, size_t capacity, const char *text)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return -1;
}
static int allocate(P4GoldenModeBytes *out, size_t size)
{
    if (!size || !(out->data = malloc(size))) return -1;
    out->size = size;
    return 0;
}
void p4_golden_mode_projection_free(P4GoldenModeProjection *p)
{
    if (!p) return;
    free(p->skill_elements.data); free(p->skill_metadata.data); free(p->skill_names.data);
    free(p->enemy_metadata.data); free(p->enemy_names.data); free(p->persona_names.data);
    for (size_t i = 0; i < 3; ++i) free(p->persona[i].data);
    for (size_t i = 0; i < 5; ++i) free(p->encounter[i].data);
    free(p->item_metadata.data); free(p->item_names.data);
    memset(p, 0, sizeof *p);
}
int p4_golden_mode_project(const P4GoldenDomainMap *map, const P4GoldenSource *source,
                           P4GoldenModeProjection *out, char *reason, size_t capacity)
{
    P4GoldenModeProjection p = {0};
    P4GoldenDomainMap checked;
    const P4GoldenTable *skill, *unit, *msg, *persona, *encount, *item;
    uint32_t max_skill = 0, max_active = 0;
    size_t written;
    int result = -1;
    if (!map || !source || !out || !map->shape.skills)
        return fail(reason, capacity, "missing Golden projection inputs");
    if (p4_golden_domain_map_build(&checked, &map->shape, reason, capacity)) return -1;
    map = &checked;
    for (uint32_t i = 0; i < map->shape.skills; ++i) {
        uint32_t port;
        if (p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, i, &port, NULL))
            return fail(reason, capacity, "invalid Golden skill map");
        if (port > max_skill) max_skill = port;
        if (i < map->shape.active_skills && port > max_active) max_active = port;
    }
    skill = &source->tables[P4_GOLDEN_SKILL]; unit = &source->tables[P4_GOLDEN_UNIT];
    msg = &source->tables[P4_GOLDEN_MSG]; persona = &source->tables[P4_GOLDEN_PERSONA];
    encount = &source->tables[P4_GOLDEN_ENCOUNT]; item = &source->tables[P4_GOLDEN_ITEM];
    if (skill->span_count < 2 || unit->span_count < 1 || msg->span_count < 4 ||
        persona->span_count < 5 || encount->span_count < 5 || !item->bytes)
        return fail(reason, capacity, "missing Golden table spans");
    if (!skill->spans[1].bytes || skill->spans[1].size != (size_t)map->shape.active_skills * 44u)
        return fail(reason, capacity, "invalid SKILL metadata shape");
    p.skill_golden = skill->spans[1];
    /* The largest mapped ID includes holes caused by the active-skill shift. */
    if (allocate(&p.skill_elements, ((size_t)max_skill + 1u) * 2u) ||
        allocate(&p.skill_metadata, ((size_t)max_active + 1u) * P4_SKILL_ROW_SIZE) ||
        allocate(&p.skill_names, ((size_t)max_skill + 1u) * P4_GOLDEN_SKILL_NAME_SIZE) ||
        allocate(&p.enemy_metadata, (size_t)map->shape.enemies * P4_ENEMY_ROW_SIZE) ||
        allocate(&p.enemy_names, (size_t)map->shape.enemies * P4_ENEMY_NAME_SIZE) ||
        allocate(&p.persona_names, (size_t)map->shape.personas * P4_GOLDEN_PERSONA_NAME_SIZE) ||
        allocate(&p.item_metadata, (size_t)map->shape.items * P4_ITEM_ROW_SIZE) ||
        allocate(&p.item_names, (size_t)map->shape.items * 24u)) goto memory;
    if (p4_golden_skill_elements(map, skill->spans[0].bytes, skill->spans[0].size,
            p.skill_elements.data, p.skill_elements.size, &written, reason, capacity)) goto done;
    p.skill_elements.size = written;
    if (p4_golden_skill_metadata(map, skill->spans[1].bytes, skill->spans[1].size,
            p.skill_metadata.data, p.skill_metadata.size, &written, reason, capacity)) goto done;
    p.skill_metadata.size = written;
    if (p4_golden_enemy_convert(map, unit->spans[0].bytes, unit->spans[0].size,
            p.enemy_metadata.data, p.enemy_metadata.size, reason, capacity)) goto done;
    P4GoldenModeBytes *names[] = {&p.skill_names, &p.enemy_names, &p.persona_names};
    const P4GoldenDomain domains[] = {P4_GOLDEN_DOMAIN_SKILL, P4_GOLDEN_DOMAIN_ENEMY, P4_GOLDEN_DOMAIN_PERSONA};
    const unsigned rows[] = {map->shape.skills, map->shape.enemies, map->shape.personas};
    const unsigned src_width[] = {23, 21, 21}, dst_width[] = {
        P4_GOLDEN_SKILL_NAME_SIZE, P4_ENEMY_NAME_SIZE, P4_GOLDEN_PERSONA_NAME_SIZE};
    for (unsigned i = 0; i < 3; ++i) {
        const P4GoldenSpan *span = &msg->spans[i + 1];
        if (p4_golden_names_convert(map, domains[i], span->bytes, span->size, rows[i],
                src_width[i], dst_width[i], names[i]->data, names[i]->size, &written, reason, capacity)) goto done;
        names[i]->size = written;
    }
    const unsigned blocks[] = {1, 2, 4};
    for (unsigned i = 0; i < 3; ++i) {
        const P4GoldenSpan *span = &persona->spans[blocks[i]];
        if (span->size > 256u * 622u || allocate(&p.persona[i], span->size)) goto memory;
        if (p4_golden_persona_convert(map, blocks[i], span->bytes, span->size,
                p.persona[i].data, p.persona[i].size, reason, capacity)) goto done;
    }
    for (unsigned i = 0; i < 5; ++i) {
        const P4GoldenSpan *span = &encount->spans[i];
        if (span->size > 1024u * 1024u || allocate(&p.encounter[i], span->size)) goto memory;
        if (p4_golden_encounter_convert(map, i, span->bytes, span->size,
                p.encounter[i].data, p.encounter[i].size, &written, reason, capacity)) goto done;
        p.encounter[i].size = written;
    }
    if (p4_golden_items_convert(map, item->bytes, item->size, p.item_metadata.data, p.item_metadata.size,
            p.item_names.data, p.item_names.size, reason, capacity)) goto done;
    *out = p; memset(&p, 0, sizeof p); result = 0; goto done;
memory:
    fail(reason, capacity, "Golden projection allocation or size limit failed");
done:
    p4_golden_mode_projection_free(&p);
    return result;
}
