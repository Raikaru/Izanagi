/* Synthetic declaration inputs, with no game data. */
#include "../src/platform/golden/golden_mode_declarations.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (0)
static unsigned skill_count, enemy_count, item_count, persona_count;
static int skill(void *u, const char *owner, const P4GameSkillDesc *row)
{
    (void)u;
    if (strcmp(owner, "golden-mode") || row->id != 576 || !row->active || row->element_id != 2 ||
        row->metadata[0] != 7 || row->presentation[0] != 9 || row->effect_source) return -1;
    ++skill_count; return 0;
}
static int enemy(void *u, const char *owner, const P4GameEnemyDesc *row)
{
    (void)u;
    if (strcmp(owner, "golden-mode") || row->id != 336 || row->model_source != 3 ||
        row->metadata[0] != 11 || row->ai[0] != 13) return -1;
    ++enemy_count; return 0;
}
static int item(void *u, const char *owner, const P4GameItemDesc *row)
{
    (void)u;
    if (strcmp(owner, "golden-mode") || row->id != 1536 || row->metadata.kind != 5 ||
        row->icon.revision != 1 || strcmp(row->icon.key, "test:icon")) return -1;
    ++item_count; return 0;
}
static int persona(void *u, const char *owner, const P4GamePersonaDesc *row)
{
    (void)u;
    if (strcmp(owner, "golden-mode") || row->id != 256 || row->model_source != 7 ||
        row->metadata[0] != 17) return -1;
    ++persona_count; return 0;
}
int main(void)
{
    static uint32_t elements[577], models[337];
    static uint8_t aux[577], groups[577], metadata[577 * P4_SKILL_ROW_SIZE], presentation[577 * 4];
    static int8_t rates[577];
    static uint8_t em[337 * P4_ENEMY_ROW_SIZE], ea[337 * P4_ENEMY_AFFINITY_SIZE];
    static uint8_t eb[337 * P4_ENEMY_MODEL_SIZE], ai[337 * P4_ENEMY_AI_SIZE];
    static uint8_t im[1537 * P4_ITEM_ROW_SIZE];
    static P4ResourceRef icons[1537];
    static P4GamePersonaDesc personas[257];
    const P4GoldenDomainShape shape = {577, 441, 337, 1537, 257};
    P4GoldenDomainMap map;
    P4GoldenModeDeclarationInputs in = {
        .skill_element_ids = elements, .skill_element_aux = aux, .skill_metadata = metadata,
        .skill_presentation = presentation, .skill_rates = rates, .skill_inheritance_groups = groups,
        .skill_count = 577, .active_count = 577, .enemy_metadata = em, .enemy_affinity = ea,
        .enemy_models = eb, .enemy_ai = ai, .enemy_model_sources = models, .enemy_count = 337,
        .item_metadata = im, .item_icons = icons, .item_count = 1537, .personas = personas, .persona_count = 257};
    P4GoldenDeclareSink sink = {NULL, skill, enemy, item, persona};
    char reason[192];
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    elements[576] = 2; metadata[576 * P4_SKILL_ROW_SIZE] = 7; presentation[576 * 4] = 9;
    models[336] = 3; em[336 * P4_ENEMY_ROW_SIZE] = 11; ai[336 * P4_ENEMY_AI_SIZE] = 13;
    im[1536 * P4_ITEM_ROW_SIZE] = 5;
    strcpy(icons[1536].owner, "test"); strcpy(icons[1536].key, "test:icon"); icons[1536].revision = 1;
    personas[256].model_source = 7; personas[256].schema = personas[256].revision = 1;
    personas[256].metadata[0] = 17;
    CHECK(p4_golden_mode_declare(P4_GAME_MODE_PS2, &map, &in, &sink, reason, sizeof reason) == -1);
    CHECK(!skill_count && !enemy_count && !item_count && !persona_count);
    CHECK(!p4_golden_mode_declare(P4_GAME_MODE_GOLDEN, &map, &in, &sink, reason, sizeof reason));
    CHECK(skill_count == 1 && enemy_count == 1 && item_count == 1 && persona_count == 1);
    skill_count = enemy_count = item_count = persona_count = 0;
    models[336] = 336;
    CHECK(p4_golden_mode_declare(P4_GAME_MODE_GOLDEN, &map, &in, &sink, reason, sizeof reason) == -1);
    CHECK(strstr(reason, "model/icon") && !skill_count && !enemy_count && !item_count && !persona_count);
    models[336] = 3; in.skill_count = 576;
    CHECK(p4_golden_mode_declare(P4_GAME_MODE_GOLDEN, &map, &in, &sink, reason, sizeof reason) == -1);
    CHECK(!skill_count && !enemy_count && !item_count && !persona_count);
    in.skill_count = 577; im[1536 * P4_ITEM_ROW_SIZE] = 32;
    CHECK(p4_golden_mode_declare(P4_GAME_MODE_GOLDEN, &map, &in, &sink, reason, sizeof reason) == -1);
    CHECK(!skill_count && !enemy_count && !item_count && !persona_count);
    im[1536 * P4_ITEM_ROW_SIZE] = 5; groups[576] = P4_SKILL_INHERITANCE_GROUP_COUNT;
    CHECK(p4_golden_mode_declare(P4_GAME_MODE_GOLDEN, &map, &in, &sink, reason, sizeof reason) == -1);
    CHECK(!skill_count && !enemy_count && !item_count && !persona_count);
    groups[576] = 0; icons[1536].revision = 0;
    CHECK(p4_golden_mode_declare(P4_GAME_MODE_GOLDEN, &map, &in, &sink, reason, sizeof reason) == -1);
    CHECK(!skill_count && !enemy_count && !item_count && !persona_count);
    icons[1536].revision = 1; strcpy(icons[1536].key, "wrong:icon");
    CHECK(p4_golden_mode_declare(P4_GAME_MODE_GOLDEN, &map, &in, &sink, reason, sizeof reason) == -1);
    CHECK(!skill_count && !enemy_count && !item_count && !persona_count);
    puts("golden_mode_declarations: ok");
    return 0;
}
