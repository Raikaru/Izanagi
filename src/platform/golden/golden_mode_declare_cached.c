/* Decode portable declaration rows from verified cache objects. */
#include "golden_mode_declare_cached.h"
#include "golden_mode_declare_builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t u32(const uint8_t *p)
{ return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static const uint8_t *find(const P4GoldenCacheTable *extra, size_t count, const char *name, size_t size,
                           char *reason, size_t capacity)
{
    for (size_t i = 0; i < count; ++i) {
        if (!extra[i].name || strcmp(extra[i].name, name)) continue;
        if (extra[i].data && extra[i].size == size) return extra[i].data;
        break;
    }
    if (reason && capacity) snprintf(reason, capacity, "missing or malformed cached declaration %s", name);
    return NULL;
}
static int text_domain(uint32_t table, P4GoldenDomain *domain)
{
    switch (table) {
    case P4_TEXT_ITEM: *domain = P4_GOLDEN_DOMAIN_ITEM; return 0;
    case P4_TEXT_SKILL: *domain = P4_GOLDEN_DOMAIN_SKILL; return 0;
    case P4_TEXT_PERSONA: *domain = P4_GOLDEN_DOMAIN_PERSONA; return 0;
    case P4_TEXT_ENEMY: *domain = P4_GOLDEN_DOMAIN_ENEMY; return 0;
    default: return -1;
    }
}
/* decl/text: count u32, then table/id/role/size u32 and exact encoded bytes.
 * All framing and domain checks precede any text registration. */
static int register_text(P4GameTablesDefinitionBuilder *builder, const P4GoldenDomainMap *map,
                         const P4GoldenCacheTable *extra, size_t count, char *reason, size_t capacity)
{
    const P4GoldenCacheTable *text = NULL;
    for (size_t i = 0; i < count; ++i)
        if (extra[i].name && !strcmp(extra[i].name, "decl/text")) { text = &extra[i]; break; }
    if (!text || !text->data || text->size < 4 || text->size > (4u << 20)) goto malformed;
    uint32_t rows = u32(text->data); size_t offset = 4;
    if (!rows || rows > 131072u) goto malformed;
    for (uint32_t i = 0; i < rows; ++i) {
        if (offset > text->size || text->size - offset < 16) goto malformed;
        const uint8_t *row = text->data + offset;
        uint32_t table = u32(row), id = u32(row + 4), role = u32(row + 8), size = u32(row + 12);
        P4GoldenDomain domain; uint32_t golden, port; int expanded;
        offset += 16;
        if (text_domain(table, &domain) || role > P4_TEXT_DESCRIPTION || !size ||
            size > text->size - offset || text->data[offset + size - 1] ||
            p4_golden_domain_from_port(map, domain, id, &golden) ||
            p4_golden_domain_to_port(map, domain, golden, &port, &expanded) || !expanded || port != id) goto malformed;
        offset += size;
    }
    if (offset != text->size) goto malformed;
    uint32_t origin;
    if (p4_golden_mode_cache_origin(P4_GAME_MODE_GOLDEN, builder, &origin, reason, capacity)) return -1;
    offset = 4;
    for (uint32_t i = 0; i < rows; ++i) {
        const uint8_t *row = text->data + offset;
        uint32_t table = u32(row), id = u32(row + 4), role = u32(row + 8), size = u32(row + 12);
        offset += 16;
        P4GameText *view = p4_game_tables_builder_text(builder, role);
        if (!view) goto refused;
        int status = p4_game_tables_text_register(view, P4_GOLDEN_DECLARE_OWNER, table, id, role, 1,
                                                   text->data + offset, size);
        P4TableSourceEntry entry = {.domain = P4_TABLE_SOURCE_STRING, .schema = 1, .revision = 1};
        entry.ids[0] = table; entry.ids[1] = id; entry.ids[2] = role;
        strcpy(entry.owner, P4_GOLDEN_DECLARE_OWNER);
        if (!status) status = p4_game_table_sources_bind(builder->sources, view, &entry, origin);
        if (status) goto refused;
        offset += size;
    }
    return 0;
malformed:
    if (reason && capacity) snprintf(reason, capacity, "missing or malformed cached declaration text");
    return -1;
refused:
    if (reason && capacity) snprintf(reason, capacity, "Golden cached declaration text was refused");
    return -1;
}
int p4_golden_mode_declare_cached(void *context, P4GameTablesDefinitionBuilder *builder,
                                   const P4GoldenCacheTable *extra, size_t count,
                                   char *reason, size_t capacity)
{
    const P4GoldenDomainMap *source = context;
    P4GoldenDomainMap checked;
    P4GoldenModeDeclarationInputs in = {0};
    uint32_t *elements = NULL, *models = NULL;
    P4ResourceRef *icons = NULL; P4GamePersonaDesc *personas = NULL;
    const uint8_t *data;
    size_t skill_count = 0, active_count = 0;
    int result = -1;
    if (!source || !builder || (count && !extra) ||
        p4_golden_domain_map_build(&checked, source ? &source->shape : NULL, reason, capacity)) return -1;
    for (uint32_t i = 0; i < checked.shape.skills; ++i) {
        uint32_t port;
        if (p4_golden_domain_to_port(&checked, P4_GOLDEN_DOMAIN_SKILL, i, &port, NULL)) goto done;
        if ((size_t)port + 1 > skill_count) skill_count = (size_t)port + 1;
        if (i < checked.shape.active_skills && (size_t)port + 1 > active_count) active_count = (size_t)port + 1;
    }
    in.skill_count = skill_count; in.active_count = active_count;
    in.enemy_count = checked.shape.enemies; in.item_count = checked.shape.items; in.persona_count = checked.shape.personas;
    if (p4_golden_domain_expanded_count(&checked, P4_GOLDEN_DOMAIN_SKILL)) {
        data = find(extra, count, "decl/skill-elements", skill_count * 4, reason, capacity); if (!data) goto done;
        elements = malloc(skill_count * sizeof *elements); if (!elements) goto memory;
        for (size_t i = 0; i < skill_count; ++i) elements[i] = u32(data + i * 4);
        in.skill_element_ids = elements;
        in.skill_element_aux = find(extra, count, "decl/skill-aux", skill_count, reason, capacity); if (!in.skill_element_aux) goto done;
        in.skill_inheritance_groups = find(extra, count, "decl/skill-groups", skill_count, reason, capacity); if (!in.skill_inheritance_groups) goto done;
        in.skill_rates = (const int8_t *)find(extra, count, "decl/skill-rates", skill_count, reason, capacity); if (!in.skill_rates) goto done;
        in.skill_metadata = find(extra, count, "decl/skill-metadata", active_count * P4_SKILL_ROW_SIZE, reason, capacity); if (!in.skill_metadata) goto done;
        in.skill_presentation = find(extra, count, "decl/skill-presentation", active_count * 4, reason, capacity); if (!in.skill_presentation) goto done;
    }
    if (p4_golden_domain_expanded_count(&checked, P4_GOLDEN_DOMAIN_ENEMY)) {
        size_t n = in.enemy_count;
        in.enemy_metadata = find(extra, count, "decl/enemy-metadata", n * P4_ENEMY_ROW_SIZE, reason, capacity); if (!in.enemy_metadata) goto done;
        in.enemy_affinity = find(extra, count, "decl/enemy-affinity", n * P4_ENEMY_AFFINITY_SIZE, reason, capacity); if (!in.enemy_affinity) goto done;
        in.enemy_models = find(extra, count, "decl/enemy-model", n * P4_ENEMY_MODEL_SIZE, reason, capacity); if (!in.enemy_models) goto done;
        in.enemy_ai = find(extra, count, "decl/enemy-ai", n * P4_ENEMY_AI_SIZE, reason, capacity); if (!in.enemy_ai) goto done;
        data = find(extra, count, "decl/enemy-model-source", n * 4, reason, capacity); if (!data) goto done;
        models = malloc(n * sizeof *models); if (!models) goto memory;
        for (size_t i = 0; i < n; ++i) models[i] = u32(data + i * 4);
        in.enemy_model_sources = models;
    }
    if (p4_golden_domain_expanded_count(&checked, P4_GOLDEN_DOMAIN_ITEM)) {
        size_t n = in.item_count;
        in.item_metadata = find(extra, count, "decl/item-metadata", n * P4_ITEM_ROW_SIZE, reason, capacity); if (!in.item_metadata) goto done;
        data = find(extra, count, "decl/item-icons", n * 164u, reason, capacity); if (!data) goto done;
        icons = calloc(n, sizeof *icons); if (!icons) goto memory;
        for (size_t i = 0; i < n; ++i) {
            const uint8_t *row = data + i * 164u;
            memcpy(icons[i].owner, row, 64); memcpy(icons[i].key, row + 64, 96); icons[i].revision = u32(row + 160);
        }
        in.item_icons = icons;
    }
    if (p4_golden_domain_expanded_count(&checked, P4_GOLDEN_DOMAIN_PERSONA)) {
        size_t n = in.persona_count;
        const size_t stride = 4u + P4_PERSONA_METADATA_SIZE + P4_PERSONA_GROWTH_SIZE + P4_PERSONA_MODEL_SIZE +
                              P4_PERSONA_CANDIDATES_SIZE + P4_PERSONA_AFFINITY_SIZE;
        data = find(extra, count, "decl/persona-rows", n * stride, reason, capacity); if (!data) goto done;
        personas = calloc(n, sizeof *personas); if (!personas) goto memory;
        for (size_t i = 0; i < n; ++i) {
            const uint8_t *row = data + i * stride;
            personas[i].model_source = u32(row); personas[i].schema = personas[i].revision = 1; row += 4;
            memcpy(personas[i].metadata, row, P4_PERSONA_METADATA_SIZE); row += P4_PERSONA_METADATA_SIZE;
            memcpy(personas[i].growth, row, P4_PERSONA_GROWTH_SIZE); row += P4_PERSONA_GROWTH_SIZE;
            memcpy(personas[i].model, row, P4_PERSONA_MODEL_SIZE); row += P4_PERSONA_MODEL_SIZE;
            memcpy(personas[i].candidates, row, P4_PERSONA_CANDIDATES_SIZE); row += P4_PERSONA_CANDIDATES_SIZE;
            memcpy(personas[i].affinity, row, P4_PERSONA_AFFINITY_SIZE);
        }
        in.personas = personas;
    }
    if ((p4_golden_domain_expanded_count(&checked, P4_GOLDEN_DOMAIN_SKILL) ||
         p4_golden_domain_expanded_count(&checked, P4_GOLDEN_DOMAIN_ENEMY) ||
         p4_golden_domain_expanded_count(&checked, P4_GOLDEN_DOMAIN_ITEM) ||
         p4_golden_domain_expanded_count(&checked, P4_GOLDEN_DOMAIN_PERSONA)) &&
        register_text(builder, &checked, extra, count, reason, capacity)) goto done;
    result = p4_golden_mode_declare_to_builder(P4_GAME_MODE_GOLDEN, builder, &checked, &in, reason, capacity);
    goto done;
memory:
    if (reason && capacity) snprintf(reason, capacity, "cannot allocate cached Golden declarations");
done:
    free(elements); free(models); free(icons); free(personas);
    return result;
}
