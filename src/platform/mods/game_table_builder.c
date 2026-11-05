#include "p4pc/game_table_builder.h"
#include <string.h>

static int writable(const P4GameTablesDefinitionBuilder *b)
{ return b && b->tables && b->sources && b->registry_revision && !b->sealing && !b->sealed; }
static int registry_use(const P4GameTables *view, uint32_t domain, const uint32_t ids[5], void *context)
{
    const P4GameTablesDefinitionBuilder *b = context;
    if (!b || view != b->tables || !b->native_uses) return P4_TABLE_NOT_READY;
    return p4_game_table_sources_semantic_use(view, domain, ids, b->native_uses);
}
static int skill_use(const P4GameSkills *view, uint32_t id, void *context)
{
    const P4GameTablesDefinitionBuilder *b = context;
    if (!b || view != b->skills || !b->native_uses) return P4_TABLE_NOT_READY;
    const uint32_t ids[5] = {id, 0, 0, 0, 0};
    return p4_game_table_sources_semantic_use(view, P4_TABLE_SOURCE_SKILL, ids, b->native_uses);
}
static int enemy_use(const P4GameEnemies *view, uint32_t id, void *context)
{
    const P4GameTablesDefinitionBuilder *b = context;
    if (!b || view != b->enemies || !b->native_uses) return P4_TABLE_NOT_READY;
    const uint32_t ids[5] = {id, 0, 0, 0, 0};
    return p4_game_table_sources_semantic_use(view, P4_TABLE_SOURCE_ENEMY, ids, b->native_uses);
}
static int persona_use(const P4GamePersonas *view, uint32_t id, void *context)
{
    const P4GameTablesDefinitionBuilder *b = context;
    if (!b || view != b->personas || !b->native_uses) return P4_TABLE_NOT_READY;
    const uint32_t ids[5] = {id, 0, 0, 0, 0};
    return p4_game_table_sources_semantic_use(view, P4_TABLE_SOURCE_PERSONA, ids, b->native_uses);
}
const P4GameTableInput *p4_game_tables_builder_input(const P4GameTablesDefinitionBuilder *b,
    P4GameTableInputKind kind)
{
    if (!b || (unsigned)kind >= P4_TABLE_INPUT_COUNT) return NULL;
    for (size_t i = 0; i < b->input_count; ++i)
        if (p4_game_table_input_kind(b->inputs[i]) == kind) return b->inputs[i];
    return NULL;
}
static int item_use(const P4GameItems *view, uint32_t table, uint32_t id, void *context)
{
    const P4GameTablesDefinitionBuilder *b = context;
    if (!b || view != b->items || !b->native_uses || table > P4_ITEM_USE_CATEGORY) return P4_TABLE_NOT_READY;
    if (table == P4_ITEM_USE_ROW && id < P4_ITEM_RETAIL_COUNT) {
        const P4GameTableInput *input = p4_game_tables_builder_input(b, P4_TABLE_INPUT_ITEM);
        const P4GameTableInputSpan *span = p4_game_table_input_span(input, 0);
        return span ? p4_game_table_input_semantic_use(span, (void *)input) : P4_TABLE_NOT_READY;
    }
    const uint32_t ids[5] = {id, 0, 0, 0, 0};
    return p4_game_table_sources_semantic_use(view,
        table == P4_ITEM_USE_ROW ? P4_TABLE_SOURCE_ITEM : P4_TABLE_SOURCE_CATEGORY, ids, b->native_uses);
}
static int text_use(const P4GameText *view, uint32_t table, uint32_t id, uint32_t role, void *context)
{
    const P4GameTablesDefinitionBuilder *b = context;
    if (!b || !b->native_uses || (view != b->names && view != b->descriptions)) return P4_TABLE_NOT_READY;
    const uint32_t ids[5] = {table, id, role, 0, 0};
    return p4_game_table_sources_semantic_use(view, P4_TABLE_SOURCE_STRING, ids, b->native_uses);
}
int p4_game_tables_builder_init(P4GameTablesDefinitionBuilder *b,
    const P4GameTableAllocator *allocator, uint32_t revision)
{
    if (!b || !revision || b->tables || b->sources || b->skills || b->items || b->enemies || b->personas || b->names ||
        b->descriptions || b->input_count || b->input_mask || b->sealing || b->sealed || b->native_uses ||
        b->action_prepare || b->action_context ||
        (allocator && (!allocator->allocate || !allocator->release))) return P4_TABLE_INVALID;
    b->registry_revision = revision;
    if (allocator) b->allocator = *allocator;
    b->tables = p4_game_tables_create();
    b->sources = p4_game_table_sources_create();
    if (!b->tables || !b->sources) return P4_TABLE_NOMEM;
    int result = p4_game_tables_bind_runtime_use(b->tables, registry_use, b);
    return result ? result : p4_game_table_sources_view(b->sources, P4_TABLE_SOURCE_REGISTRY, b->tables);
}
int p4_game_tables_builder_request_input(P4GameTablesDefinitionBuilder *b, P4GameTableInputKind kind)
{
    if (!writable(b) || (unsigned)kind >= P4_TABLE_INPUT_COUNT) return P4_TABLE_INVALID;
    b->input_mask |= 1u << (unsigned)kind;
    return P4_TABLE_OK;
}
P4GameSkills *p4_game_tables_builder_skills(P4GameTablesDefinitionBuilder *b)
{
    if (!writable(b)) return NULL;
    if (b->skills) return b->skills;
    b->skills = p4_game_skills_create(b->allocator.allocate ? &b->allocator : NULL);
    if (!b->skills || p4_game_skills_bind_runtime_use(b->skills,
        p4_game_table_input_semantic_use, skill_use, b) ||
        p4_game_table_sources_view(b->sources, P4_TABLE_SOURCE_SKILLS, b->skills)) {
        b->sealing = 1;
        return NULL;
    }
    b->input_mask |= (1u << P4_TABLE_INPUT_SKILL) | (1u << P4_TABLE_INPUT_EFFECT) | (1u << P4_TABLE_INPUT_MSG);
    return b->skills;
}
P4GameItems *p4_game_tables_builder_items(P4GameTablesDefinitionBuilder *b)
{
    if (!writable(b)) return NULL;
    if (b->items) return b->items;
    b->items = p4_game_tables_items_create(b->allocator.allocate ? &b->allocator : NULL);
    if (!b->items || p4_game_tables_items_bind_runtime_use(b->items, item_use, b) ||
        p4_game_table_sources_view(b->sources, P4_TABLE_SOURCE_ITEMS, b->items)) {
        b->sealing = 1;
        return NULL;
    }
    b->input_mask |= 1u << P4_TABLE_INPUT_ITEM;
    return b->items;
}
P4GameEnemies *p4_game_tables_builder_enemies(P4GameTablesDefinitionBuilder *b)
{
    if (!writable(b)) return NULL;
    if (b->enemies) return b->enemies;
    b->enemies = p4_game_enemies_create(b->allocator.allocate ? &b->allocator : NULL);
    if (!b->enemies || p4_game_enemies_bind_runtime_use(b->enemies,
        p4_game_table_input_semantic_use, enemy_use, b) ||
        p4_game_table_sources_view(b->sources, P4_TABLE_SOURCE_ENEMIES, b->enemies)) {
        b->sealing = 1;
        return NULL;
    }
    b->input_mask |= (1u << P4_TABLE_INPUT_UNIT) | (1u << P4_TABLE_INPUT_MODEL) |
        (1u << P4_TABLE_INPUT_AICALC) | (1u << P4_TABLE_INPUT_MSG);
    return b->enemies;
}
P4GamePersonas *p4_game_tables_builder_personas(P4GameTablesDefinitionBuilder *b)
{
    if (!writable(b)) return NULL;
    if (b->personas) return b->personas;
    b->personas = p4_game_personas_create(b->allocator.allocate ? &b->allocator : NULL);
    if (!b->personas || p4_game_personas_bind_runtime_use(b->personas,
        p4_game_table_input_semantic_use, persona_use, b) ||
        p4_game_table_sources_view(b->sources, P4_TABLE_SOURCE_PERSONAS, b->personas)) {
        b->sealing = 1;
        return NULL;
    }
    b->input_mask |= (1u << P4_TABLE_INPUT_PERSONA) | (1u << P4_TABLE_INPUT_MODEL) | (1u << P4_TABLE_INPUT_MSG) |
        (1u << P4_TABLE_INPUT_UNIT);
    return b->personas;
}
P4GameText *p4_game_tables_builder_text(P4GameTablesDefinitionBuilder *b, uint32_t role)
{
    if (!writable(b) || role > P4_TEXT_DESCRIPTION) return NULL;
    P4GameText **slot = role == P4_TEXT_NAME ? &b->names : &b->descriptions;
    if (*slot) return *slot;
    const P4TextProfile profile = {
        role == P4_TEXT_NAME ? P4_TEXT_ENCODING_US_RAW_NATIVE : P4_TEXT_ENCODING_US_NATIVE,
        P4_TEXT_FONT_US_NATIVE
    };
    *slot = p4_game_tables_text_create(b->allocator.allocate ? &b->allocator : NULL, &profile);
    if (!*slot || p4_game_tables_text_bind_runtime_use(*slot, text_use, b) ||
        p4_game_table_sources_view(b->sources, P4_TABLE_SOURCE_TEXT, *slot)) {
        b->sealing = 1;
        return NULL;
    }
    return *slot;
}
int p4_game_tables_builder_seal(P4GameTablesDefinitionBuilder *b,
    const struct P4PackageProvider **failed_provider)
{
    if (failed_provider) *failed_provider = NULL;
    if (!writable(b)) return P4_TABLE_FROZEN;
    /* Effective actions require original skill/item definitions and the actual
     * AI rows/BF9/BF10 owner, even without new rows in those families. */
    if (p4_game_elements_requested(b->tables)) {
        if (!p4_game_tables_builder_skills(b) || !p4_game_tables_builder_items(b)) return P4_TABLE_NOMEM;
        b->input_mask |= (1u << P4_TABLE_INPUT_UNIT) | (1u << P4_TABLE_INPUT_AICALC);
    }
    b->sealing = 1;
    for (unsigned kind = 0; kind < P4_TABLE_INPUT_COUNT; ++kind) {
        if (!(b->input_mask & (1u << kind))) continue;
        P4GameTableInput *input = NULL;
        int result = p4_game_table_input_prepare((P4GameTableInputKind)kind, UINT32_MAX, &input, failed_provider);
        if (result) return result > 0 ? P4_TABLE_NOT_FOUND : P4_TABLE_INVALID;
        b->inputs[b->input_count++] = input;
        if (p4_game_table_input_seal(input)) return P4_TABLE_INVALID;
    }
    int result = b->names ? p4_game_tables_text_seal(b->names, b->registry_revision) : P4_TABLE_OK;
    if (!result && b->descriptions) result = p4_game_tables_text_seal(b->descriptions, b->registry_revision);
    if (!result && p4_game_tables_affinity_retail_pending(b->tables)) {
        /* Retail-derived side rows read the actual selected UNIT banks:
         * block 1 enemy and block 2 persona affinity, 32-byte rows. */
        const P4GameTableInput *unit = p4_game_tables_builder_input(b, P4_TABLE_INPUT_UNIT);
        const P4GameTableInputSpan *enemy = p4_game_table_input_span(unit, 1);
        const P4GameTableInputSpan *persona = p4_game_table_input_span(unit, 2);
        result = !enemy || !persona ? P4_TABLE_NOT_FOUND : p4_game_tables_affinity_resolve_retail(b->tables,
            p4_game_table_input_span_bytes(enemy), p4_game_table_input_span_size(enemy),
            p4_game_table_input_span_bytes(persona), p4_game_table_input_span_size(persona));
    }
    if (!result) result = p4_game_tables_freeze(b->tables, b->registry_revision);
    if (!result && b->skills) {
        const P4GameTableInput *skill = p4_game_tables_builder_input(b, P4_TABLE_INPUT_SKILL);
        const P4GameTableInput *effect = p4_game_tables_builder_input(b, P4_TABLE_INPUT_EFFECT);
        const P4GameTableInput *msg = p4_game_tables_builder_input(b, P4_TABLE_INPUT_MSG);
        result = p4_game_skills_seal(b->skills, p4_game_table_input_span(skill, 1),
            p4_game_table_input_span(skill, 0), p4_game_table_input_span(effect, 0),
            p4_game_table_input_span(msg, 1), b->tables, b->names, b->descriptions, b->registry_revision);
    }
    if (!result && b->items)
        result = p4_game_tables_items_prepare_actions(b->items, b->action_prepare, b->action_context);
    if (!result && b->items) {
        const P4GameTableInput *item = p4_game_tables_builder_input(b, P4_TABLE_INPUT_ITEM);
        result = p4_game_tables_items_seal(b->items, p4_game_table_input_bytes(item),
            p4_game_table_input_size(item), b->names, b->descriptions, b->registry_revision);
    }
    if (!result && b->enemies) {
        const P4GameTableInput *unit = p4_game_tables_builder_input(b, P4_TABLE_INPUT_UNIT);
        const P4GameTableInput *model = p4_game_tables_builder_input(b, P4_TABLE_INPUT_MODEL);
        const P4GameTableInput *ai = p4_game_tables_builder_input(b, P4_TABLE_INPUT_AICALC);
        const P4GameTableInput *msg = p4_game_tables_builder_input(b, P4_TABLE_INPUT_MSG);
        result = p4_game_enemies_seal(b->enemies, p4_game_table_input_span(unit, 0),
            p4_game_table_input_span(unit, 1), p4_game_table_input_span(model, 1),
            p4_game_table_input_span(ai, 8), p4_game_table_input_span(msg, 2), b->names, b->registry_revision);
    }
    if (!result && b->personas) {
        const P4GameTableInput *persona = p4_game_tables_builder_input(b, P4_TABLE_INPUT_PERSONA);
        const P4GameTableInput *model = p4_game_tables_builder_input(b, P4_TABLE_INPUT_MODEL);
        const P4GameTableInput *msg = p4_game_tables_builder_input(b, P4_TABLE_INPUT_MSG);
        const P4GameTableInput *unit = p4_game_tables_builder_input(b, P4_TABLE_INPUT_UNIT);
        result = p4_game_personas_seal(b->personas, p4_game_table_input_span(persona, 0),
            p4_game_table_input_span(persona, 1), p4_game_table_input_span(model, 2),
            p4_game_table_input_span(persona, 5), p4_game_table_input_span(msg, 3),
            p4_game_table_input_span(unit, 2), b->names, b->registry_revision);
    }
    if (!result) result = p4_game_table_sources_seal(b->sources);
    if (!result && p4_game_table_native_uses_prepare(b->inputs, b->input_count, b->sources, &b->native_uses))
        result = P4_TABLE_NOT_READY;
    if (!result) b->sealed = 1;
    return result;
}
int p4_game_tables_builder_destroy(P4GameTablesDefinitionBuilder *b)
{
    if (!b) return P4_TABLE_INVALID;
    if (b->native_uses && p4_game_table_native_uses_destroy(b->native_uses)) return P4_TABLE_CONFLICT;
    p4_game_table_sources_destroy(b->sources);
    p4_game_skills_destroy(b->skills);
    p4_game_tables_items_destroy(b->items);
    p4_game_enemies_destroy(b->enemies);
    p4_game_personas_destroy(b->personas);
    p4_game_tables_text_destroy(b->names);
    p4_game_tables_text_destroy(b->descriptions);
    for (size_t i = 0; i < b->input_count; ++i) p4_game_table_input_destroy(b->inputs[i]);
    p4_game_tables_destroy(b->tables);
    memset(b, 0, sizeof *b);
    return P4_TABLE_OK;
}
