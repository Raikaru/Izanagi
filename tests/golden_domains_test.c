/* Golden mode ID domains (src/platform/golden/golden_domains.c): the Golden -> port map over the measured
 * Golden shape, its inverse, shape refusals, and the declaration of Golden-only entities through a recording
 * sink, including every refusal and the PS2-mode guard. Synthetic descriptors only; no game data. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/golden_domains.h"

static int failures;
#define CHECK(c)                                                         \
    do {                                                                 \
        if (!(c)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                  \
        }                                                                \
    } while (0)

static const P4GoldenDomainShape kGolden = {624, 472, 368, 2560, 256};

static uint32_t port_of(const P4GoldenDomainMap *map, P4GoldenDomain domain, uint32_t golden, int *expanded)
{
    uint32_t port = 0xFFFFFFFFu;
    CHECK(!p4_golden_domain_to_port(map, domain, golden, &port, expanded));
    return port;
}

static void test_map(void)
{
    P4GoldenDomainMap map;
    char reason[160];
    int expanded;
    uint32_t port, golden;

    CHECK(!p4_golden_domain_map_build(&map, &kGolden, reason, sizeof reason));
    CHECK(map.added_actives == 32 && map.mapped_passives == 136);
    /* Skills: shared actives, Golden-only actives after PS2's domain, shifted passives, passives past PS2's. */
    CHECK(port_of(&map, P4_GOLDEN_DOMAIN_SKILL, 0x001, &expanded) == 0x001 && !expanded);
    CHECK(port_of(&map, P4_GOLDEN_DOMAIN_SKILL, 0x1B7, &expanded) == 0x1B7 && !expanded);
    CHECK(port_of(&map, P4_GOLDEN_DOMAIN_SKILL, 0x1B8, &expanded) == 576 && expanded);
    CHECK(port_of(&map, P4_GOLDEN_DOMAIN_SKILL, 0x1D7, &expanded) == 607 && expanded);
    CHECK(port_of(&map, P4_GOLDEN_DOMAIN_SKILL, 0x1D8, &expanded) == 0x1B8 && !expanded);
    CHECK(port_of(&map, P4_GOLDEN_DOMAIN_SKILL, 0x25F, &expanded) == 0x23F && !expanded);
    CHECK(port_of(&map, P4_GOLDEN_DOMAIN_SKILL, 0x260, &expanded) == 608 && expanded);
    CHECK(port_of(&map, P4_GOLDEN_DOMAIN_SKILL, 0x26F, &expanded) == 623 && expanded);
    CHECK(p4_golden_domain_to_port(&map, P4_GOLDEN_DOMAIN_SKILL, 624, &port, NULL) == -1);
    CHECK(p4_golden_domain_expanded_count(&map, P4_GOLDEN_DOMAIN_SKILL) == 48);
    /* The skill map is a bijection of 0..623: every port ID has exactly one Golden skill and back. */
    {
        unsigned char *seen = calloc(624, 1);
        unsigned expanded_seen = 0;
        for (uint32_t g = 0; g < 624; ++g) {
            port = port_of(&map, P4_GOLDEN_DOMAIN_SKILL, g, &expanded);
            CHECK(port < 624 && !seen[port]);
            if (port < 624) seen[port] = 1;
            expanded_seen += (unsigned)expanded;
            CHECK(!p4_golden_domain_from_port(&map, P4_GOLDEN_DOMAIN_SKILL, port, &golden) && golden == g);
        }
        CHECK(expanded_seen == 48);
        free(seen);
    }
    CHECK(p4_golden_domain_from_port(&map, P4_GOLDEN_DOMAIN_SKILL, 624, &golden) == -1);
    /* Enemies, items, personas: identity, expanded from the PS2 domain. */
    CHECK(port_of(&map, P4_GOLDEN_DOMAIN_ENEMY, 335, &expanded) == 335 && !expanded);
    CHECK(port_of(&map, P4_GOLDEN_DOMAIN_ENEMY, 336, &expanded) == 336 && expanded);
    CHECK(p4_golden_domain_to_port(&map, P4_GOLDEN_DOMAIN_ENEMY, 368, &port, NULL) == -1);
    CHECK(port_of(&map, P4_GOLDEN_DOMAIN_ITEM, 2559, &expanded) == 2559 && expanded);
    CHECK(port_of(&map, P4_GOLDEN_DOMAIN_ITEM, 1535, &expanded) == 1535 && !expanded);
    CHECK(port_of(&map, P4_GOLDEN_DOMAIN_PERSONA, 255, &expanded) == 255 && !expanded);
    CHECK(p4_golden_domain_expanded_count(&map, P4_GOLDEN_DOMAIN_ENEMY) == 32);
    CHECK(p4_golden_domain_expanded_count(&map, P4_GOLDEN_DOMAIN_ITEM) == 1024);
    CHECK(p4_golden_domain_expanded_count(&map, P4_GOLDEN_DOMAIN_PERSONA) == 0);
    CHECK(p4_golden_domain_from_port(&map, P4_GOLDEN_DOMAIN_ENEMY, 400, &golden) == -1);
    CHECK(p4_golden_domain_to_port(&map, P4_GOLDEN_DOMAIN_COUNT, 0, &port, NULL) == -1);

    /* A shape like PS2's own expands nothing. */
    {
        const P4GoldenDomainShape ps2 = {576, 440, 336, 1536, 256};
        CHECK(!p4_golden_domain_map_build(&map, &ps2, reason, sizeof reason));
        for (int d = 0; d < P4_GOLDEN_DOMAIN_COUNT; ++d) CHECK(p4_golden_domain_expanded_count(&map, (P4GoldenDomain)d) == 0);
        CHECK(port_of(&map, P4_GOLDEN_DOMAIN_SKILL, 500, &expanded) == 500 && !expanded);
    }
    {
        const P4GoldenDomainShape sparse = {442, 442, 336, 1536, 256};
        CHECK(!p4_golden_domain_map_build(&map, &sparse, reason, sizeof reason));
        CHECK(p4_golden_domain_from_port(&map, P4_GOLDEN_DOMAIN_SKILL, 440, &golden) == -1);
        CHECK(p4_golden_domain_from_port(&map, P4_GOLDEN_DOMAIN_SKILL, UINT32_MAX, &golden) == -1);
        CHECK(!p4_golden_domain_from_port(&map, P4_GOLDEN_DOMAIN_SKILL, 576, &golden) && golden == 440);
    }
    /* Refused shapes. */
    {
        P4GoldenDomainShape bad = kGolden;
        bad.active_skills = 400;
        CHECK(p4_golden_domain_map_build(&map, &bad, reason, sizeof reason) == -1 && reason[0]);
        CHECK(p4_golden_domain_to_port(&map, P4_GOLDEN_DOMAIN_SKILL, 0, &port, NULL) == -1);   /* unbuilt map */
        bad = kGolden; bad.active_skills = 700;
        CHECK(p4_golden_domain_map_build(&map, &bad, reason, sizeof reason) == -1);
        bad = kGolden; bad.enemies = 300;
        CHECK(p4_golden_domain_map_build(&map, &bad, reason, sizeof reason) == -1);
        bad = kGolden; bad.skills = 5000;
        CHECK(p4_golden_domain_map_build(&map, &bad, reason, sizeof reason) == -1 && strstr(reason, "ceiling"));
        bad = kGolden; bad.personas = 600;
        CHECK(p4_golden_domain_map_build(&map, &bad, reason, sizeof reason) == -1);
        CHECK(p4_golden_domain_map_build(&map, NULL, reason, sizeof reason) == -1);
    }
}

typedef struct Recorder {
    unsigned skills, enemies, items, personas;
    uint32_t ids[64];
    char keys[64][P4_TABLE_KEY_SIZE];
    unsigned count;
    int refuse_at; /* 1-based registration to refuse, 0 none */
} Recorder;

static int record(Recorder *r, uint32_t id, const char *owner, const char *key)
{
    if (r->refuse_at && (int)r->count + 1 == r->refuse_at) return -3;
    CHECK(!strcmp(owner, P4_GOLDEN_DECLARE_OWNER));
    if (r->count < 64) {
        r->ids[r->count] = id;
        snprintf(r->keys[r->count], sizeof r->keys[0], "%s", key);
    }
    ++r->count;
    return 0;
}
static int skill_sink(void *u, const char *owner, const P4GameSkillDesc *d)
{ ((Recorder *)u)->skills++; return record(u, d->id, owner, d->key); }
static int enemy_sink(void *u, const char *owner, const P4GameEnemyDesc *d)
{ ((Recorder *)u)->enemies++; return record(u, d->id, owner, d->key); }
static int item_sink(void *u, const char *owner, const P4GameItemDesc *d)
{ ((Recorder *)u)->items++; return record(u, d->id, owner, d->key); }
static int persona_sink(void *u, const char *owner, const P4GamePersonaDesc *d)
{ ((Recorder *)u)->personas++; return record(u, d->id, owner, d->key); }

/* The table service's owned-key rule: "<owner>:<identifier>", identifiers in [a-z0-9_.-]. */
static int key_ok(const char *key)
{
    size_t n = strlen(P4_GOLDEN_DECLARE_OWNER);
    if (strncmp(key, P4_GOLDEN_DECLARE_OWNER, n) || key[n] != ':' || !key[n + 1]) return 0;
    for (const char *s = key + n + 1; *s; ++s)
        if (!((*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9') || *s == '_' || *s == '-' || *s == '.')) return 0;
    return 1;
}

static void test_declare(void)
{
    /* A small Golden shape: 2 Golden-only actives, 1 passive past PS2's domain, 2 enemies, 1 item, 1 persona. */
    const P4GoldenDomainShape shape = {442 + 136 + 1, 442, 338, 1537, 257};
    P4GoldenDomainMap map;
    char reason[160];
    P4GoldenDeclaredSkill skills[3];
    P4GoldenDeclaredEnemy enemies[2];
    P4GoldenDeclaredItem items[1];
    P4GoldenDeclaredPersona personas[1];
    P4GoldenDeclarations d;
    Recorder rec;
    P4GoldenDeclareSink sink = {&rec, skill_sink, enemy_sink, item_sink, persona_sink};

    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    CHECK(p4_golden_domain_expanded_count(&map, P4_GOLDEN_DOMAIN_SKILL) == 3);
    memset(skills, 0, sizeof skills); memset(enemies, 0, sizeof enemies);
    memset(items, 0, sizeof items); memset(personas, 0, sizeof personas);
    skills[0].golden_id = 440; skills[1].golden_id = 441; skills[2].golden_id = 442 + 136;
    enemies[0].golden_id = 336; enemies[1].golden_id = 337;
    items[0].golden_id = 1536; personas[0].golden_id = 256;
    memset(&d, 0, sizeof d);
    d.skills = skills; d.skill_count = 3; d.enemies = enemies; d.enemy_count = 2;
    d.items = items; d.item_count = 1; d.personas = personas; d.persona_count = 1;

    memset(&rec, 0, sizeof rec);
    CHECK(p4_golden_domains_declare(P4_GAME_MODE_PS2, &map, &d, &sink, reason, sizeof reason) == -1 &&
          strstr(reason, "Golden mode") && rec.count == 0);
    CHECK(p4_golden_domains_declare(P4_GAME_MODE_INVALID, &map, &d, &sink, reason, sizeof reason) == -1);

    CHECK(!p4_golden_domains_declare(P4_GAME_MODE_GOLDEN, &map, &d, &sink, reason, sizeof reason));
    CHECK(rec.skills == 3 && rec.enemies == 2 && rec.items == 1 && rec.personas == 1 && rec.count == 7);
    CHECK(rec.ids[0] == 576 && rec.ids[1] == 577 && rec.ids[2] == 578);   /* actives, then the passive past PS2's */
    CHECK(rec.ids[3] == 336 && rec.ids[4] == 337 && rec.ids[5] == 1536 && rec.ids[6] == 256);
    CHECK(!strcmp(rec.keys[0], "golden-mode:skill-1b8") && !strcmp(rec.keys[5], "golden-mode:item-600"));
    for (unsigned i = 0; i < 7; ++i) CHECK(key_ok(rec.keys[i]));

    /* Refusals register nothing. */
    memset(&rec, 0, sizeof rec);
    d.skill_count = 2;                                         /* a Golden-only skill undeclared */
    CHECK(p4_golden_domains_declare(P4_GAME_MODE_GOLDEN, &map, &d, &sink, reason, sizeof reason) == -1 &&
          strstr(reason, "2 of 3") && rec.count == 0);
    d.skill_count = 3;
    skills[1].golden_id = 440;                                 /* duplicate */
    CHECK(p4_golden_domains_declare(P4_GAME_MODE_GOLDEN, &map, &d, &sink, reason, sizeof reason) == -1 &&
          strstr(reason, "twice") && rec.count == 0);
    skills[1].golden_id = 0x10;                                /* a skill PS2 already has */
    CHECK(p4_golden_domains_declare(P4_GAME_MODE_GOLDEN, &map, &d, &sink, reason, sizeof reason) == -1 &&
          strstr(reason, "already has") && rec.count == 0);
    skills[1].golden_id = 5000;                                /* outside Golden's domain */
    CHECK(p4_golden_domains_declare(P4_GAME_MODE_GOLDEN, &map, &d, &sink, reason, sizeof reason) == -1 &&
          strstr(reason, "outside") && rec.count == 0);
    skills[1].golden_id = 441;
    enemies[1].golden_id = 300;                                /* an enemy PS2 already has */
    CHECK(p4_golden_domains_declare(P4_GAME_MODE_GOLDEN, &map, &d, &sink, reason, sizeof reason) == -1 && rec.count == 0);
    enemies[1].golden_id = 337;
    /* A sink refusal stops at that entity with its status. */
    memset(&rec, 0, sizeof rec);
    rec.refuse_at = 4;
    CHECK(p4_golden_domains_declare(P4_GAME_MODE_GOLDEN, &map, &d, &sink, reason, sizeof reason) == -1 &&
          strstr(reason, "enemy 0x150") && strstr(reason, "-3") && rec.count == 3);
    /* Missing pieces. */
    CHECK(p4_golden_domains_declare(P4_GAME_MODE_GOLDEN, NULL, &d, &sink, reason, sizeof reason) == -1);
    CHECK(p4_golden_domains_declare(P4_GAME_MODE_GOLDEN, &map, &d, NULL, reason, sizeof reason) == -1);
    d.skills = NULL;
    CHECK(p4_golden_domains_declare(P4_GAME_MODE_GOLDEN, &map, &d, &sink, NULL, 0) == -1);
}

int main(void)
{
    test_map();
    test_declare();
    if (failures) fprintf(stderr, "%d failure(s)\n", failures);
    else printf("golden_domains: ok\n");
    return failures != 0;
}
