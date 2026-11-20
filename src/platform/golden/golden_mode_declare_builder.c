/* Bridge the M4 declaration sink to real builder families and source ledger. */
#include "golden_mode_declare_builder.h"
#include "../mods/game_table_golden_attest.h"
#include <stdio.h>
#include <string.h>

typedef struct Bridge { P4GameTablesDefinitionBuilder *builder; uint32_t source; } Bridge;
static int bind(Bridge *bridge, const void *view, uint32_t domain, uint32_t id,
                const char *owner, const char *key, uint32_t schema, uint32_t revision)
{
    P4TableSourceEntry entry = {.domain = domain, .schema = schema, .revision = revision};
    if (!owner || !key || strlen(owner) >= sizeof entry.owner || strlen(key) >= sizeof entry.key) return P4_TABLE_INVALID;
    entry.ids[0] = id; strcpy(entry.owner, owner); strcpy(entry.key, key);
    return p4_game_table_sources_bind(bridge->builder->sources, view, &entry, bridge->source);
}
static int skill(void *user, const char *owner, const P4GameSkillDesc *desc)
{
    Bridge *bridge = user; P4GameSkills *view = p4_game_tables_builder_skills(bridge->builder);
    if (!view) return P4_TABLE_NOMEM;
    int result = p4_game_skills_register(view, owner, desc);
    return result ? result : bind(bridge, view, P4_TABLE_SOURCE_SKILL, desc->id, owner, desc->key, desc->schema, desc->revision);
}
static int enemy(void *user, const char *owner, const P4GameEnemyDesc *desc)
{
    Bridge *bridge = user; P4GameEnemies *view = p4_game_tables_builder_enemies(bridge->builder);
    if (!view) return P4_TABLE_NOMEM;
    int result = p4_game_enemies_register(view, owner, desc);
    return result ? result : bind(bridge, view, P4_TABLE_SOURCE_ENEMY, desc->id, owner, desc->key, desc->schema, desc->revision);
}
static int item(void *user, const char *owner, const P4GameItemDesc *desc)
{
    Bridge *bridge = user; P4GameItems *view = p4_game_tables_builder_items(bridge->builder);
    if (!view) return P4_TABLE_NOMEM;
    int result = p4_game_tables_items_register(view, owner, desc);
    return result ? result : bind(bridge, view, P4_TABLE_SOURCE_ITEM, desc->id, owner, desc->key, 1, desc->revision);
}
static int persona(void *user, const char *owner, const P4GamePersonaDesc *desc)
{
    Bridge *bridge = user; P4GamePersonas *view = p4_game_tables_builder_personas(bridge->builder);
    if (!view) return P4_TABLE_NOMEM;
    int result = p4_game_personas_register(view, owner, desc);
    return result ? result : bind(bridge, view, P4_TABLE_SOURCE_PERSONA, desc->id, owner, desc->key, desc->schema, desc->revision);
}
int p4_golden_mode_cache_origin(P4GameMode mode, P4GameTablesDefinitionBuilder *builder,
                                 uint32_t *source, char *reason, size_t capacity)
{
    if (mode != P4_GAME_MODE_GOLDEN || !builder || !source || !builder->sources || builder->sealing || builder->sealed ||
        !p4_game_table_golden_inputs_attest(builder->inputs, builder->input_count, 1)) {
        if (reason && capacity) snprintf(reason, capacity, "Golden declarations require a verified cache-backed builder");
        return -1;
    }
    char origin[sizeof "golden-cache:" + 64];
    snprintf(origin, sizeof origin, "golden-cache:%.64s", builder->inputs[0]->golden_origin + sizeof "golden-cache/" - 1);
    int result = p4_game_table_sources_native(builder->sources, origin, 1, 1, source);
    if (result) {
        if (reason && capacity) snprintf(reason, capacity, "cannot register Golden cache declaration origin (%d)", result);
        return -1;
    }
    return 0;
}
int p4_golden_mode_declare_to_builder(P4GameMode mode, P4GameTablesDefinitionBuilder *builder,
                                      const P4GoldenDomainMap *map, const P4GoldenModeDeclarationInputs *inputs,
                                      char *reason, size_t capacity)
{
    Bridge bridge = {builder, 0};
    if (p4_golden_mode_cache_origin(mode, builder, &bridge.source, reason, capacity)) return -1;
    const P4GoldenDeclareSink sink = {&bridge, skill, enemy, item, persona};
    return p4_golden_mode_declare(mode, map, inputs, &sink, reason, capacity);
}
