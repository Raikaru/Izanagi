/* Synthetic builder/source objects exercise the real declaration bridge. */
#include "../src/platform/golden/golden_mode_declare_builder.h"
#include "../src/platform/golden/golden_mode_declare_cached.h"
#include "../src/platform/mods/game_table_inputs_internal.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (0)
static unsigned registrations, bindings, origins, text_registrations;
#ifndef P4_GOLDEN_REAL_INPUT_FIXTURE
const char *p4_game_table_input_label(P4GameTableInputKind kind)
{
    static const char *labels[] = {"init/itemtbl.bin", "battle/SKILL.TBL", "battle/UNIT.TBL", "battle/MSG.TBL",
        "battle/PERSONA.TBL", "battle/ENCOUNT.TBL", "battle/EFFECT.TBL", "battle/MODEL.TBL", "battle/AICALC.TBL"};
    return labels[kind];
}
#endif
P4GameSkills *p4_game_tables_builder_skills(P4GameTablesDefinitionBuilder *b) { (void)b; return (P4GameSkills *)(uintptr_t)1; }
P4GameEnemies *p4_game_tables_builder_enemies(P4GameTablesDefinitionBuilder *b) { (void)b; return NULL; }
P4GameItems *p4_game_tables_builder_items(P4GameTablesDefinitionBuilder *b) { (void)b; return NULL; }
P4GamePersonas *p4_game_tables_builder_personas(P4GameTablesDefinitionBuilder *b) { (void)b; return NULL; }
P4GameText *p4_game_tables_builder_text(P4GameTablesDefinitionBuilder *b, uint32_t role)
{ (void)b; return role <= 1 ? (P4GameText *)(uintptr_t)(role + 2) : NULL; }
int p4_game_tables_text_register(P4GameText *view, const char *owner, uint32_t table,
    uint32_t id, uint32_t role, uint32_t revision, const uint8_t *bytes, size_t size)
{
    if (view != (P4GameText *)(uintptr_t)(role + 2) || strcmp(owner, "golden-mode") ||
        table != P4_TEXT_SKILL || id != 576 || role > 1 || revision != 1 || size != 2 || bytes[1] ||
        bytes[0] != (role ? 'y' : 'x')) return P4_TABLE_INVALID;
    ++text_registrations; return P4_TABLE_OK;
}
int p4_game_skills_register(P4GameSkills *view, const char *owner, const P4GameSkillDesc *desc)
{
    if (view != (P4GameSkills *)(uintptr_t)1 || strcmp(owner, "golden-mode") || desc->id != 576 ||
        strcmp(desc->key, "golden-mode:skill-1b8")) return P4_TABLE_INVALID;
#ifdef P4_GOLDEN_REAL_INPUT_FIXTURE
    if (desc->metadata[0] != 3) return P4_TABLE_INVALID;
#endif
    ++registrations; return P4_TABLE_OK;
}
int p4_game_enemies_register(P4GameEnemies *v, const char *o, const P4GameEnemyDesc *d)
{ (void)v; (void)o; (void)d; return P4_TABLE_INVALID; }
int p4_game_tables_items_register(P4GameItems *v, const char *o, const P4GameItemDesc *d)
{ (void)v; (void)o; (void)d; return P4_TABLE_INVALID; }
int p4_game_personas_register(P4GamePersonas *v, const char *o, const P4GamePersonaDesc *d)
{ (void)v; (void)o; (void)d; return P4_TABLE_INVALID; }
int p4_game_table_sources_native(P4GameTableSources *s, const char *origin, uint32_t schema, uint32_t revision, uint32_t *id)
{
    if (!s || strncmp(origin, "golden-cache:", 13) || strlen(origin) != 77 || schema != 1 || revision != 1)
        return P4_TABLE_INVALID;
    ++origins; *id = 7; return P4_TABLE_OK;
}
int p4_game_table_sources_bind(P4GameTableSources *s, const void *view, const P4TableSourceEntry *entry, uint32_t origin)
{
    if (!s || origin != 7 || entry->schema != 1 || entry->revision != 1 ||
        strcmp(entry->owner, "golden-mode")) return P4_TABLE_INVALID;
    if (entry->domain == P4_TABLE_SOURCE_SKILL) {
        if (view != (void *)(uintptr_t)1 || entry->ids[0] != 576 ||
            strcmp(entry->key, "golden-mode:skill-1b8")) return P4_TABLE_INVALID;
    } else if (entry->domain == P4_TABLE_SOURCE_STRING) {
        if (entry->ids[0] != P4_TEXT_SKILL || entry->ids[1] != 576 || entry->ids[2] > 1 ||
            view != (void *)(uintptr_t)(entry->ids[2] + 2) || entry->key[0]) return P4_TABLE_INVALID;
    } else return P4_TABLE_INVALID;
    ++bindings; return P4_TABLE_OK;
}
int main(void)
{
    static uint32_t elements[577]; static uint8_t aux[577], groups[577], metadata[577 * P4_SKILL_ROW_SIZE], presentation[577 * 4];
    static uint8_t element_bytes[577 * 4];
    static int8_t rates[577]; char reason[160], key[65]; memset(key, 'a', 64); key[64] = 0;
    P4GoldenDomainMap map; const P4GoldenDomainShape shape = {577, 441, 336, 1536, 256};
    P4GoldenModeDeclarationInputs inputs = {.skill_element_ids=elements,.skill_element_aux=aux,.skill_metadata=metadata,
        .skill_presentation=presentation,.skill_rates=rates,.skill_inheritance_groups=groups,.skill_count=577,.active_count=577};
    P4GameTablesDefinitionBuilder builder = {0}; builder.sources = (P4GameTableSources *)(uintptr_t)2;
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    for (unsigned i = 0; i < P4_TABLE_INPUT_COUNT; ++i) {
        P4GameTableInput *input = calloc(1, sizeof *input); CHECK(input);
        input->kind = (P4GameTableInputKind)i; input->sealed = 1;
        snprintf(input->golden_origin, sizeof input->golden_origin, "golden-cache/%s/%s", key, p4_game_table_input_label(input->kind));
        builder.inputs[builder.input_count++] = input;
    }
    CHECK(p4_golden_mode_declare_to_builder(P4_GAME_MODE_PS2, &builder, &map, &inputs, reason, sizeof reason) == -1);
    CHECK(!origins && !registrations && !bindings);
    CHECK(!p4_golden_mode_declare_to_builder(P4_GAME_MODE_GOLDEN, &builder, &map, &inputs, reason, sizeof reason));
    CHECK(origins == 1 && registrations == 1 && bindings == 1);
    uint8_t text[40] = {2,0,0,0, 1,0,0,0, 0x40,2,0,0, 0,0,0,0, 2,0,0,0, 'x',0,
                       1,0,0,0, 0x40,2,0,0, 1,0,0,0, 2,0,0,0, 'y',0};
    P4GoldenCacheTable extra[] = {
        {"decl/skill-aux", aux, sizeof aux}, {"decl/skill-elements", element_bytes, sizeof element_bytes},
        {"decl/skill-groups", groups, sizeof groups}, {"decl/skill-metadata", metadata, sizeof metadata},
        {"decl/skill-presentation", presentation, sizeof presentation}, {"decl/skill-rates", (uint8_t *)rates, sizeof rates},
        {"decl/text", text, sizeof text}
    };
    CHECK(!p4_golden_mode_declare_cached(&map, &builder, extra, sizeof extra / sizeof extra[0], reason, sizeof reason));
    CHECK(origins == 3 && registrations == 2 && text_registrations == 2 && bindings == 4);
    CHECK(p4_golden_mode_declare_cached(&map, &builder, extra, 1, reason, sizeof reason) == -1);
    CHECK(origins == 3 && registrations == 2 && text_registrations == 2 && bindings == 4);
    builder.inputs[1]->golden_origin[13] = 'b';
    CHECK(p4_golden_mode_declare_to_builder(P4_GAME_MODE_GOLDEN, &builder, &map, &inputs, reason, sizeof reason) == -1);
    CHECK(origins == 3 && registrations == 2 && text_registrations == 2 && bindings == 4);
    for (size_t i = 0; i < builder.input_count; ++i) free(builder.inputs[i]);
    puts("golden_mode_declare_builder: ok"); return 0;
}
