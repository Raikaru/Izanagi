/* Golden mode ID domains: the Golden -> port ID map and the declaration of Golden-only entities
 * (include/p4pc/golden_domains.h). */
#include "p4pc/golden_domains.h"
#include <stdio.h>
#include <string.h>

static int fail(char *reason, size_t capacity, const char *text)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return -1;
}

int p4_golden_domain_map_build(P4GoldenDomainMap *map, const P4GoldenDomainShape *shape, char *reason, size_t capacity)
{
    uint32_t passives, expanded;

    if (!map) return fail(reason, capacity, "no domain shape");
    memset(map, 0, sizeof *map);
    if (!shape) return fail(reason, capacity, "no domain shape");
    if (shape->active_skills < P4_SKILL_RETAIL_ACTIVE_COUNT || shape->active_skills > shape->skills)
        return fail(reason, capacity, "Golden active skills are fewer than PS2's or more than all skills");
    if (shape->enemies < P4_ENEMY_RETAIL_ROWS || shape->items < P4_ITEM_RETAIL_COUNT ||
        shape->personas < P4_PERSONA_RETAIL_ROWS)
        return fail(reason, capacity, "a Golden domain is smaller than the PS2 game's");
    if (shape->skills > P4_SKILL_MAX_ID + 1u)
        return fail(reason, capacity, "Golden-only skills exceed the skill expansion ceiling");
    map->added_actives = shape->active_skills - P4_SKILL_RETAIL_ACTIVE_COUNT;
    passives = shape->skills - shape->active_skills;
    map->mapped_passives = passives < P4_SKILL_RETAIL_COUNT - P4_SKILL_RETAIL_ACTIVE_COUNT ?
        passives : P4_SKILL_RETAIL_COUNT - P4_SKILL_RETAIL_ACTIVE_COUNT;
    expanded = map->added_actives + passives - map->mapped_passives;
    if (expanded && P4_SKILL_RETAIL_COUNT + expanded - 1 > P4_SKILL_MAX_ID)
        return fail(reason, capacity, "Golden-only skills exceed the skill expansion ceiling");
    if (shape->enemies - 1 > P4_ENEMY_MAX_ID || shape->items - 1 > P4_ITEM_MAX_ID ||
        shape->personas - 1 > P4_PERSONA_MAX_ID)
        return fail(reason, capacity, "a Golden domain exceeds its expansion ceiling");
    map->shape = *shape;
    if (reason && capacity) reason[0] = 0;
    return 0;
}

static int identity_to_port(uint32_t count, uint32_t retail, uint32_t golden, uint32_t *port, int *expanded)
{
    if (golden >= count) return -1;
    *port = golden;
    if (expanded) *expanded = golden >= retail;
    return 0;
}

int p4_golden_domain_to_port(const P4GoldenDomainMap *map, P4GoldenDomain domain, uint32_t golden, uint32_t *port,
                             int *expanded)
{
    const P4GoldenDomainShape *s;
    if (!map || !port || !map->shape.skills) return -1;
    s = &map->shape;
    switch (domain) {
    case P4_GOLDEN_DOMAIN_SKILL:
        if (golden >= s->skills) return -1;
        if (golden < P4_SKILL_RETAIL_ACTIVE_COUNT) {
            *port = golden;
            if (expanded) *expanded = 0;
        } else if (golden < s->active_skills) {
            *port = P4_SKILL_RETAIL_COUNT + (golden - P4_SKILL_RETAIL_ACTIVE_COUNT);
            if (expanded) *expanded = 1;
        } else {
            uint32_t ps2 = golden - map->added_actives;
            if (ps2 < P4_SKILL_RETAIL_COUNT) {
                *port = ps2;
                if (expanded) *expanded = 0;
            } else {
                *port = P4_SKILL_RETAIL_COUNT + map->added_actives + (ps2 - P4_SKILL_RETAIL_COUNT);
                if (expanded) *expanded = 1;
            }
        }
        return 0;
    case P4_GOLDEN_DOMAIN_ENEMY: return identity_to_port(s->enemies, P4_ENEMY_RETAIL_ROWS, golden, port, expanded);
    case P4_GOLDEN_DOMAIN_ITEM: return identity_to_port(s->items, P4_ITEM_RETAIL_COUNT, golden, port, expanded);
    case P4_GOLDEN_DOMAIN_PERSONA: return identity_to_port(s->personas, P4_PERSONA_RETAIL_ROWS, golden, port, expanded);
    default: return -1;
    }
}

int p4_golden_domain_from_port(const P4GoldenDomainMap *map, P4GoldenDomain domain, uint32_t port, uint32_t *golden)
{
    const P4GoldenDomainShape *s;
    uint32_t candidate, mapped;
    if (!map || !golden || !map->shape.skills) return -1;
    s = &map->shape;
    switch (domain) {
    case P4_GOLDEN_DOMAIN_SKILL:
        if (port > P4_SKILL_MAX_ID) return -1;
        if (port < P4_SKILL_RETAIL_ACTIVE_COUNT) candidate = port;                       /* shared active */
        else if (port < P4_SKILL_RETAIL_COUNT) candidate = port + map->added_actives;    /* PS2 passive slot */
        else if (port < P4_SKILL_RETAIL_COUNT + map->added_actives)                      /* Golden-only active */
            candidate = P4_SKILL_RETAIL_ACTIVE_COUNT + (port - P4_SKILL_RETAIL_COUNT);
        else candidate = port;  /* passive past PS2's domain: port = 576 + A + (p - 576) = p + A = Golden ID */
        if (candidate >= s->skills ||
            p4_golden_domain_to_port(map, domain, candidate, &mapped, NULL) || mapped != port) return -1;
        *golden = candidate;
        return 0;
    case P4_GOLDEN_DOMAIN_ENEMY: if (port >= s->enemies) return -1; *golden = port; return 0;
    case P4_GOLDEN_DOMAIN_ITEM: if (port >= s->items) return -1; *golden = port; return 0;
    case P4_GOLDEN_DOMAIN_PERSONA: if (port >= s->personas) return -1; *golden = port; return 0;
    default: return -1;
    }
}

uint32_t p4_golden_domain_expanded_count(const P4GoldenDomainMap *map, P4GoldenDomain domain)
{
    const P4GoldenDomainShape *s;
    if (!map || !map->shape.skills) return 0;
    s = &map->shape;
    switch (domain) {
    case P4_GOLDEN_DOMAIN_SKILL:
        return map->added_actives + (s->skills - s->active_skills - map->mapped_passives);
    case P4_GOLDEN_DOMAIN_ENEMY: return s->enemies - P4_ENEMY_RETAIL_ROWS;
    case P4_GOLDEN_DOMAIN_ITEM: return s->items - P4_ITEM_RETAIL_COUNT;
    case P4_GOLDEN_DOMAIN_PERSONA: return s->personas - P4_PERSONA_RETAIL_ROWS;
    default: return 0;
    }
}

static const char *const domain_names[P4_GOLDEN_DOMAIN_COUNT] = {"skill", "enemy", "item", "persona"};

/* Each declaration names a distinct Golden-only ID of its domain, and every Golden-only ID is declared. */
static int check_domain(const P4GoldenDomainMap *map, P4GoldenDomain domain, const uint32_t *ids, size_t count,
                        size_t stride, char *reason, size_t capacity)
{
    uint32_t expected = p4_golden_domain_expanded_count(map, domain);
    if (count && !ids) return fail(reason, capacity, "no declaration array");
    for (size_t i = 0; i < count; ++i) {
        uint32_t golden = *(const uint32_t *)((const uint8_t *)ids + i * stride), port;
        int expanded;
        if (p4_golden_domain_to_port(map, domain, golden, &port, &expanded)) {
            snprintf(reason, capacity, "Golden %s 0x%x is outside Golden's domain", domain_names[domain], golden);
            return -1;
        }
        if (!expanded) {
            snprintf(reason, capacity, "Golden %s 0x%x is one the PS2 game already has", domain_names[domain], golden);
            return -1;
        }
        for (size_t j = 0; j < i; ++j) {
            if (*(const uint32_t *)((const uint8_t *)ids + j * stride) == golden) {
                snprintf(reason, capacity, "Golden %s 0x%x is declared twice", domain_names[domain], golden);
                return -1;
            }
        }
    }
    if (count != expected) {
        snprintf(reason, capacity, "%u of %u Golden-only %ss declared", (unsigned)count, (unsigned)expected,
                 domain_names[domain]);
        return -1;
    }
    return 0;
}

static void make_key(char *key, size_t capacity, P4GoldenDomain domain, uint32_t golden)
{
    snprintf(key, capacity, P4_GOLDEN_DECLARE_OWNER ":%s-%x", domain_names[domain], golden);
}

static int refused(char *reason, size_t capacity, P4GoldenDomain domain, uint32_t golden, int status)
{
    snprintf(reason, capacity, "the table service refused Golden %s 0x%x (status %d)", domain_names[domain], golden,
             status);
    return -1;
}

int p4_golden_domains_declare(P4GameMode mode, const P4GoldenDomainMap *map, const P4GoldenDeclarations *declarations,
                              const P4GoldenDeclareSink *sink, char *reason, size_t capacity)
{
    char scratch[160];
    char *why = reason && capacity ? reason : scratch;
    size_t room = reason && capacity ? capacity : sizeof scratch;
    const P4GoldenDeclarations *d = declarations;
    uint32_t port;
    int status;

    why[0] = 0;
    if (mode != P4_GAME_MODE_GOLDEN) return fail(why, room, "Golden-only entities are declared only in Golden mode");
    if (!map || !map->shape.skills || !d || !sink || !sink->skill || !sink->enemy || !sink->item || !sink->persona)
        return fail(why, room, "no domain map, declarations or sink");
    if (check_domain(map, P4_GOLDEN_DOMAIN_SKILL, d->skills ? &d->skills->golden_id : NULL, d->skill_count,
                     sizeof *d->skills, why, room) ||
        check_domain(map, P4_GOLDEN_DOMAIN_ENEMY, d->enemies ? &d->enemies->golden_id : NULL, d->enemy_count,
                     sizeof *d->enemies, why, room) ||
        check_domain(map, P4_GOLDEN_DOMAIN_ITEM, d->items ? &d->items->golden_id : NULL, d->item_count,
                     sizeof *d->items, why, room) ||
        check_domain(map, P4_GOLDEN_DOMAIN_PERSONA, d->personas ? &d->personas->golden_id : NULL, d->persona_count,
                     sizeof *d->personas, why, room)) {
        return -1;
    }
    for (size_t i = 0; i < d->skill_count; ++i) {
        P4GameSkillDesc desc = d->skills[i].desc;
        p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_SKILL, d->skills[i].golden_id, &port, NULL);
        desc.id = port;
        make_key(desc.key, sizeof desc.key, P4_GOLDEN_DOMAIN_SKILL, d->skills[i].golden_id);
        if ((status = sink->skill(sink->user, P4_GOLDEN_DECLARE_OWNER, &desc)))
            return refused(why, room, P4_GOLDEN_DOMAIN_SKILL, d->skills[i].golden_id, status);
    }
    for (size_t i = 0; i < d->enemy_count; ++i) {
        P4GameEnemyDesc desc = d->enemies[i].desc;
        p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_ENEMY, d->enemies[i].golden_id, &port, NULL);
        desc.id = port;
        make_key(desc.key, sizeof desc.key, P4_GOLDEN_DOMAIN_ENEMY, d->enemies[i].golden_id);
        if ((status = sink->enemy(sink->user, P4_GOLDEN_DECLARE_OWNER, &desc)))
            return refused(why, room, P4_GOLDEN_DOMAIN_ENEMY, d->enemies[i].golden_id, status);
    }
    for (size_t i = 0; i < d->item_count; ++i) {
        P4GameItemDesc desc = d->items[i].desc;
        char key[P4_TABLE_KEY_SIZE];
        p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_ITEM, d->items[i].golden_id, &port, NULL);
        desc.id = port;
        make_key(key, sizeof key, P4_GOLDEN_DOMAIN_ITEM, d->items[i].golden_id);
        desc.key = key;
        if ((status = sink->item(sink->user, P4_GOLDEN_DECLARE_OWNER, &desc)))
            return refused(why, room, P4_GOLDEN_DOMAIN_ITEM, d->items[i].golden_id, status);
    }
    for (size_t i = 0; i < d->persona_count; ++i) {
        P4GamePersonaDesc desc = d->personas[i].desc;
        p4_golden_domain_to_port(map, P4_GOLDEN_DOMAIN_PERSONA, d->personas[i].golden_id, &port, NULL);
        desc.id = port;
        make_key(desc.key, sizeof desc.key, P4_GOLDEN_DOMAIN_PERSONA, d->personas[i].golden_id);
        if ((status = sink->persona(sink->user, P4_GOLDEN_DECLARE_OWNER, &desc)))
            return refused(why, room, P4_GOLDEN_DOMAIN_PERSONA, d->personas[i].golden_id, status);
    }
    return 0;
}
