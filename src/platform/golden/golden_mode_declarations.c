/* Assemble validated Golden-only rows for the M4 declaration API. */
#include "golden_mode_declarations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(char *reason, size_t capacity, const char *message)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", message);
    return -1;
}

static int identifier(const char *text, size_t capacity)
{
    if (!text || !text[0]) return 0;
    for (size_t i = 0; i < capacity; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (!c) return 1;
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
              c == '_' || c == '-' || c == '.')) return 0;
    }
    return 0;
}
static int item_icon_valid(const P4ResourceRef *icon)
{
    if (!icon->revision || !identifier(icon->owner, sizeof icon->owner)) return 0;
    size_t n = strlen(icon->owner);
    return !strncmp(icon->key, icon->owner, n) && icon->key[n] == ':' &&
        identifier(icon->key + n + 1, sizeof icon->key - n - 1);
}

int p4_golden_mode_declare(P4GameMode mode, const P4GoldenDomainMap *map, const P4GoldenModeDeclarationInputs *in,
                           const P4GoldenDeclareSink *sink, char *reason, size_t capacity)
{
    P4GoldenDeclaredSkill *skills = NULL;
    P4GoldenDeclaredEnemy *enemies = NULL;
    P4GoldenDeclaredPersona *personas = NULL;
    P4GoldenDeclaredItem *items = NULL;
    P4GoldenDeclarations declarations = {0};
    int result = -1;
    if (mode != P4_GAME_MODE_GOLDEN) return fail(reason, capacity, "Golden declarations require Golden mode");
    if (!map || !in || !sink) return fail(reason, capacity, "missing Golden declaration inputs");
    size_t ns = p4_golden_domain_expanded_count(map, P4_GOLDEN_DOMAIN_SKILL);
    size_t ne = p4_golden_domain_expanded_count(map, P4_GOLDEN_DOMAIN_ENEMY);
    size_t ni = p4_golden_domain_expanded_count(map, P4_GOLDEN_DOMAIN_ITEM);
    size_t np = p4_golden_domain_expanded_count(map, P4_GOLDEN_DOMAIN_PERSONA);
    if ((ns && (!in->skill_element_ids || !in->skill_element_aux || !in->skill_rates ||
                !in->skill_inheritance_groups || !in->skill_metadata || !in->skill_presentation)) ||
        (ne && (!in->enemy_metadata || !in->enemy_affinity || !in->enemy_models ||
                !in->enemy_ai || !in->enemy_model_sources)) || (ni && (!in->item_metadata || !in->item_icons)) ||
        (np && !in->personas))
        return fail(reason, capacity, "missing converted Golden declaration rows");
    skills = ns ? calloc(ns, sizeof *skills) : NULL;
    enemies = ne ? calloc(ne, sizeof *enemies) : NULL;
    items = ni ? calloc(ni, sizeof *items) : NULL;
    personas = np ? calloc(np, sizeof *personas) : NULL;
    if ((ns && !skills) || (ne && !enemies) || (ni && !items) || (np && !personas)) {
        fail(reason, capacity, "cannot allocate Golden declarations"); goto done;
    }
    size_t s = 0, e = 0, t = 0, p = 0;
    for (uint32_t g = 0; g < map->shape.skills; ++g) {
        uint32_t port; int expanded;
        if (p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, g, &port, &expanded)) goto malformed;
        if (!expanded) continue;
        if (port >= in->skill_count || (g < map->shape.active_skills && port >= in->active_count) ||
            in->skill_inheritance_groups[port] >= P4_SKILL_INHERITANCE_GROUP_COUNT) goto malformed;
        P4GoldenDeclaredSkill *row = &skills[s++];
        row->golden_id = g;
        row->desc.schema = row->desc.revision = 1;
        row->desc.active = g < map->shape.active_skills;
        row->desc.element_id = in->skill_element_ids[port];
        row->desc.element_aux = in->skill_element_aux[port];
        row->desc.inheritance_group = in->skill_inheritance_groups[port];
        row->desc.inheritance_rate = in->skill_rates[port];
        if (row->desc.active) {
            memcpy(row->desc.metadata, in->skill_metadata + (size_t)port * P4_SKILL_ROW_SIZE, P4_SKILL_ROW_SIZE);
            memcpy(row->desc.presentation, in->skill_presentation + (size_t)port * 4, 4);
        }
        /* effect_source zero means casts refuse, not an invented effect. */
    }
    for (uint32_t g = P4_ENEMY_RETAIL_ROWS; g < map->shape.enemies; ++g) {
        if (g >= in->enemy_count || in->enemy_model_sources[g] >= P4_ENEMY_RETAIL_ROWS) goto malformed;
        P4GoldenDeclaredEnemy *row = &enemies[e++];
        row->golden_id = g;
        row->desc.schema = row->desc.revision = 1;
        row->desc.model_source = in->enemy_model_sources[g];
        memcpy(row->desc.metadata, in->enemy_metadata + (size_t)g * P4_ENEMY_ROW_SIZE, P4_ENEMY_ROW_SIZE);
        memcpy(row->desc.affinity, in->enemy_affinity + (size_t)g * P4_ENEMY_AFFINITY_SIZE, P4_ENEMY_AFFINITY_SIZE);
        memcpy(row->desc.model, in->enemy_models + (size_t)g * P4_ENEMY_MODEL_SIZE, P4_ENEMY_MODEL_SIZE);
        memcpy(row->desc.ai, in->enemy_ai + (size_t)g * P4_ENEMY_AI_SIZE, P4_ENEMY_AI_SIZE);
    }
    for (uint32_t g = P4_ITEM_RETAIL_COUNT; g < map->shape.items; ++g) {
        if (g >= in->item_count) goto malformed;
        P4GoldenDeclaredItem *row = &items[t++];
        row->golden_id = g;
        row->desc.revision = 1;
        if (!item_icon_valid(&in->item_icons[g])) goto malformed;
        row->desc.icon = in->item_icons[g];
        memcpy(&row->desc.metadata, in->item_metadata + (size_t)g * P4_ITEM_ROW_SIZE, P4_ITEM_ROW_SIZE);
        if (row->desc.metadata.kind > 31) goto malformed;
    }
    for (uint32_t g = P4_PERSONA_RETAIL_ROWS; g < map->shape.personas; ++g) {
        if (g >= in->persona_count || in->personas[g].model_source >= P4_PERSONA_RETAIL_ROWS ||
            !in->personas[g].schema || !in->personas[g].revision) goto malformed;
        personas[p].golden_id = g;
        personas[p++].desc = in->personas[g];
    }
    declarations.skills = skills; declarations.skill_count = s;
    declarations.enemies = enemies; declarations.enemy_count = e;
    declarations.items = items; declarations.item_count = t;
    declarations.personas = personas; declarations.persona_count = p;
    result = p4_golden_domains_declare(mode, map, &declarations, sink, reason, capacity);
    goto done;
malformed:
    fail(reason, capacity, "invalid converted declaration rows or unapproved model/icon source");
done:
    free(skills); free(enemies); free(items); free(personas);
    return result;
}
