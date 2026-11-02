#include "game_knowledge_runtime_internal.h"
#include "p4pc/game_knowledge_native.h"
#include "p4pc/game_elements.h"
#include <stdlib.h>

/* Retail Analyze bitmap (0010F3D0/0010F420): bit entry*8+bit of 0x55 words. */
extern void datSetBitmapBit(uint32_t entry, uint32_t bit);
extern uint32_t datTestBitmapBit(uint32_t entry, uint32_t bit);
extern uint32_t datGetFlag(int32_t bit);
#define RETAIL_KNOWLEDGE_ENEMIES 336u
#define RETAIL_KNOWLEDGE_BITS 8u

static const P4GameTables *active_tables, *prepared_tables;
static P4GameKnowledge *active, *prepared;

static int complete_prepare(void *user)
{
    const P4GameTables *tables = user;
    int result = p4_game_knowledge_save_ready(tables);
    if (result) return result;
    if (!p4_game_tables_revision(tables)) return P4_TABLE_INVALID;
    result = p4_game_tables_validate_runtime_sources(tables);
    if (result) return result;
    if (prepared_tables) return prepared_tables == tables ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    /* A real reload revalidates the same committed snapshot; it must neither
     * erase observations nor allocate a second observation store. */
    if (active_tables) {
        if (active_tables != tables) return P4_TABLE_CONFLICT;
        prepared_tables = tables;
        prepared = active;
        return P4_TABLE_OK;
    }
    P4GameKnowledge *state;
    result = p4_game_knowledge_create(tables, &state);
    if (result) return result;
    prepared = state;
    prepared_tables = tables;
    return P4_TABLE_OK;
}
static void complete_commit(void *user)
{
    (void)user;
    if (!prepared_tables) abort();
    active_tables = prepared_tables;
    active = prepared;
    prepared_tables = NULL;
    prepared = NULL;
}
int p4_game_knowledge_native_boot_stage(const P4GameTables *tables)
{
    if (!tables) return P4_TABLE_INVALID;
    /* A store alone cannot advertise persistent expanded knowledge. */
    int result = p4_game_knowledge_save_ready(tables);
    if (result) return result;
    result = p4_game_tables_battle_complete_register("element-knowledge",
        complete_prepare, complete_commit, (void *)tables);
    if (result) return result;
    result = p4_game_tables_battle_complete_require("element-knowledge", "element");
    if (result) {
        int rollback = p4_game_tables_battle_complete_unregister("element-knowledge",
            complete_prepare, complete_commit, (void *)tables);
        return rollback ? rollback : result;
    }
    return P4_TABLE_OK;
}
void p4_game_knowledge_native_discard(void)
{
    /* Core/session teardown only, with all users and sidecar stages quiesced. */
    if (prepared != active) p4_game_knowledge_destroy(prepared);
    p4_game_knowledge_destroy(active);
    prepared = active = NULL;
    prepared_tables = active_tables = NULL;
}
P4GameKnowledge *p4_game_knowledge_native_active(void) { return active; }
const P4GameTables *p4_game_knowledge_native_snapshot(void) { return active_tables; }

/* Where the native affinity resolver answers this frame for this defender, and
 * therefore where its knowledge lives. */
typedef enum KnowledgeRoute { ROUTE_NONE, ROUTE_PERSONA, ROUTE_SIDE, ROUTE_RETAIL } KnowledgeRoute;
static int resolve_route(void *defender, const P4GameActionFrame *frame,
    KnowledgeRoute *route, P4GameUnitIdentity *identity, P4KnowledgeKey *key)
{
    if (!defender || !frame || frame->element.valid > 1u) return P4_TABLE_INVALID;
    if (!active_tables) return P4_TABLE_NOT_READY;
    /* No element or no affinity reveals nothing; otherwise the authored
     * alias/side pair owns the key. */
    if (!p4_game_action_element_has_affinity(&frame->element)) { *route = ROUTE_NONE; return P4_TABLE_OK; }
    int result = p4_game_elements_unit_identity(defender, identity);
    if (result) return result;
    if (identity->entity_kind == P4_ENTITY_PERSONA) { *route = ROUTE_PERSONA; return P4_TABLE_OK; }
    if (identity->entity_kind != P4_ENTITY_ENEMY) return P4_TABLE_INVALID;
    result = p4_game_knowledge_runtime_resolve(active, identity->entity_id, identity->native_profile,
        &frame->element, key);
    if (!result) { *route = ROUTE_SIDE; return P4_TABLE_OK; }
    /* The original resolver answers an unmodified retail component without an
     * authored pair, so its original retail bitmap applies. */
    if (result != P4_TABLE_NOT_FOUND || !p4_game_elements_original_component(active_tables, frame) ||
        identity->entity_id >= RETAIL_KNOWLEDGE_ENEMIES) return result;
    *route = ROUTE_RETAIL;
    return P4_TABLE_OK;
}
int p4_game_knowledge_native_observe(void *defender, const P4GameActionFrame *frame)
{
    KnowledgeRoute route;
    P4GameUnitIdentity identity;
    P4KnowledgeKey key;
    int result = resolve_route(defender, frame, &route, &identity, &key);
    if (result || route == ROUTE_NONE) return result;
    if (route == ROUTE_PERSONA) return P4_TABLE_INVALID;
    if (route == ROUTE_SIDE) return p4_game_knowledge_observe(active, &key);
    /* Original learning: retail leaves only, never a new or wider id. */
    if (frame->element.element_id < RETAIL_KNOWLEDGE_BITS)
        datSetBitmapBit(identity.entity_id, frame->element.element_id);
    return P4_TABLE_OK;
}
int p4_game_knowledge_native_ai_query(void *defender, const P4GameActionFrame *frame,
    P4GameAffinityQuery *query)
{
    if (!query) return P4_TABLE_INVALID;
    KnowledgeRoute route;
    P4GameUnitIdentity identity;
    P4KnowledgeKey key;
    int result = resolve_route(defender, frame, &route, &identity, &key);
    if (result) return result;
    int known = 1;
    if (route == ROUTE_SIDE) {
        result = p4_game_knowledge_test(active, &key, &known);
        if (result) return result;
    } else if (route == ROUTE_RETAIL) {
        /* Exactly the retail AI read datTestBitmapBit(charId, (u16)element). */
        known = datTestBitmapBit(identity.entity_id, frame->element.element_id) != 0;
    }
    *query = (P4GameAffinityQuery){known ? P4_AFFINITY_QUERY_ACTUAL : P4_AFFINITY_QUERY_PROVIDED,
        known ? 0u : 100u};
    return P4_TABLE_OK;
}
int p4_game_knowledge_native_analyze(void *defender, const P4GameActionElement *element, uint32_t *known)
{
    if (!element || !known) return P4_TABLE_INVALID;
    /* A displayed pair resolves no action: no owned binding. The pair is
     * resolved first, so a story reveal never makes an undeclared pair pass. */
    const P4GameActionFrame frame = {.element = *element};
    KnowledgeRoute route;
    P4GameUnitIdentity identity;
    P4KnowledgeKey key;
    int result = resolve_route(defender, &frame, &route, &identity, &key);
    if (result) return result;
    if (route == ROUTE_NONE) { *known = 1; return P4_TABLE_OK; }
    if (route == ROUTE_PERSONA || (route == ROUTE_RETAIL && element->element_id >= RETAIL_KNOWLEDGE_BITS))
        return route == ROUTE_PERSONA ? P4_TABLE_INVALID : P4_TABLE_NOT_FOUND;
    int value;
    if (route == ROUTE_SIDE) {
        result = p4_game_knowledge_test(active, &key, &value);
        if (result) return result;
    } else {
        value = datTestBitmapBit(identity.entity_id, element->element_id) != 0;
    }
    /* 001F0950's encounter override, before its bitmap read. */
    uint32_t metadata;
    result = p4_game_elements_enemy_metadata_flags(identity.entity_id, &metadata);
    if (result) return result;
    if (!(metadata & 0x4004u) && datGetFlag(0x1403)) {
        if (datGetFlag(0x1408)) value = 1;
        else if (datGetFlag(0x1409)) value = 0;
    }
    *known = (uint32_t)value;
    return P4_TABLE_OK;
}
int p4_game_knowledge_native_any(void *defender, uint32_t *known)
{
    if (!defender || !known) return P4_TABLE_INVALID;
    if (!active_tables) return P4_TABLE_NOT_READY;
    P4GameUnitIdentity identity;
    int result = p4_game_elements_unit_identity(defender, &identity);
    if (result) return result;
    if (identity.entity_kind != P4_ENTITY_ENEMY) return P4_TABLE_INVALID;
    const P4AffinityProfileAlias *alias = p4_game_tables_find_affinity_alias(active_tables,
        P4_ENTITY_ENEMY, identity.entity_id, identity.native_profile);
    int value = 0;
    if (alias) {
        result = p4_game_tables_runtime_use_row(active_tables, P4_TABLE_SOURCE_ALIAS, alias);
        if (!result) result = p4_game_knowledge_any(active, alias->knowledge_id, alias->profile_id, &value);
        if (result) return result;
    } else if (identity.entity_id >= RETAIL_KNOWLEDGE_ENEMIES) {
        return P4_TABLE_NOT_FOUND;
    }
    /* Natural retail pairs the original resolver still answers keep their
     * original bits: leaves 0..5 damage, Light/Dark instant KO. */
    for (uint32_t leaf = 0; !value && identity.entity_id < RETAIL_KNOWLEDGE_ENEMIES &&
         leaf < RETAIL_KNOWLEDGE_BITS; ++leaf) {
        const uint32_t channel = leaf < 6u ? P4_CHANNEL_DAMAGE : P4_CHANNEL_INSTANT_KO;
        if (alias && p4_game_tables_find_affinity(active_tables, P4_ENTITY_ENEMY, identity.entity_id,
                alias->profile_id, leaf, channel)) continue;
        if (p4_game_tables_has_affinity_effect_pair(active_tables, leaf, channel)) continue;
        value = datTestBitmapBit(identity.entity_id, leaf) != 0;
    }
    *known = (uint32_t)value;
    return P4_TABLE_OK;
}
