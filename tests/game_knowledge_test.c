/* Real frozen tables, source ledger, completion coordinator, package core,
 * boot handler and sidecar codec. The DatUnit identity and the retail Analyze
 * bitmap are labeled boundaries. Synthetic data only; no gameplay claim. */
#include "p4pc/game_knowledge.h"
#include "p4pc/game_knowledge_native.h"
#include "p4pc/game_elements.h"
#include "p4pc/game_table_boot.h"
#include "p4pc/game_tables_native.h"
#include "p4pc/packages.h"
#include "game_knowledge_runtime_internal.h"
#include "package_boot_internal.h"
#include "mod_save_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const P4PackageHandler p4_game_knowledge_boot_provider;
static unsigned runtime_uses, identity_calls, bitmap_writes;
static int runtime_failure, identity_result;
static P4GameUnitIdentity current_identity = {P4_ENTITY_ENEMY, 300, UINT32_MAX};
static uint32_t bitmap_entry, bitmap_bit;
static int defender; /* Opaque DatUnit token; identity comes from the boundary below. */

/* Labeled registry-dispatch boundary, not a package/source authority witness. */
static int runtime_use(const P4GameTables *view, uint32_t domain, const uint32_t ids[5], void *context)
{
    assert(view == context && ids && domain >= P4_TABLE_SOURCE_ELEMENT &&
        (domain <= P4_TABLE_SOURCE_EFFECT || domain == P4_TABLE_SOURCE_SIDE_AILMENT));
    ++runtime_uses;
    return runtime_failure;
}
int p4_game_elements_unit_identity(void *unit, P4GameUnitIdentity *identity)
{
    assert(unit == &defender && identity);
    ++identity_calls;
    if (identity_result) return identity_result;
    *identity = current_identity;
    return P4_TABLE_OK;
}
/* Retail 0010F3D0/0010F420 bit layout over the 0x55-word store. */
static uint32_t retail_bitmap[0x55];
void datSetBitmapBit(uint32_t entry, uint32_t bit)
{
    assert(entry < 336 && bit < 8);
    bitmap_entry = entry; bitmap_bit = bit; ++bitmap_writes;
    const uint32_t index = (bit & 0xffffu) + (entry & 0xffffu) * 8u;
    retail_bitmap[index >> 5] |= 1u << (index & 31u);
}
uint32_t datTestBitmapBit(uint32_t entry, uint32_t bit)
{
    const uint32_t index = (bit & 0xffffu) + (entry & 0xffffu) * 8u;
    assert(index >> 5 < 0x55u);
    return retail_bitmap[index >> 5] & (1u << (index & 31u));
}
_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{
    fprintf(stderr, "Unexpected knowledge boot recovery: %s\n", reason);
    abort();
}
/* 001F0950 inputs: enemy status-metadata flags and story flags 1403/1408/1409. */
static uint32_t enemy_metadata = 0, story_flags[3];
static int metadata_result;
int p4_game_elements_enemy_metadata_flags(uint32_t enemy_id, uint32_t *flags)
{
    assert(flags);
    if (metadata_result || enemy_id >= 336u) return metadata_result ? metadata_result : P4_TABLE_NOT_READY;
    *flags = enemy_metadata;
    return P4_TABLE_OK;
}
uint32_t datGetFlag(int32_t bit)
{
    assert(bit >= 0x1403 && bit <= 0x1409);
    return bit == 0x1403 ? story_flags[0] : bit == 0x1408 ? story_flags[1] : bit == 0x1409 ? story_flags[2] : 0;
}
/* The effective (wrapper) profile affinity the ailment row classifies. */
static uint32_t ailment_affinity = 100u;
static int affinity_result;
int p4_game_elements_native_profile_affinity(void *unit, uint32_t leaf,
    uint32_t channel, uint32_t component, uint32_t *native_value)
{
    assert(unit == &defender && leaf >= 8u && channel == P4_CHANNEL_STATUS &&
        component == P4_ACTION_COMPONENT_STATUS && native_value);
    if (affinity_result) return affinity_result;
    *native_value = ailment_affinity;
    return P4_TABLE_OK;
}

static void leaf(P4GameTables *tables, uint32_t id, const char *key)
{
    P4ElementDesc d = {0};
    d.id = id; d.flags = P4_ELEMENT_PHYSICAL | P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY;
    d.revision = 7;
    strcpy(d.key, key); strcpy(d.display_name, "Leaf"); strcpy(d.short_name, "Leaf");
    assert(p4_game_tables_element(tables, "test", &d) == P4_TABLE_OK);
}
static void alias(P4GameTables *tables, uint32_t enemy, uint32_t native_profile,
    uint32_t profile, uint32_t knowledge, const char *key)
{
    P4AffinityProfileAlias a = {0};
    a.entity_kind = P4_ENTITY_ENEMY; a.entity_id = enemy; a.native_profile = native_profile;
    a.profile_id = profile; a.knowledge_id = knowledge; a.schema = 1; a.revision = 4;
    strcpy(a.key, key);
    assert(p4_game_tables_affinity_alias(tables, "test", &a) == P4_TABLE_OK);
}
static void row(P4GameTables *tables, uint32_t enemy, uint32_t profile, uint32_t element, uint32_t channel)
{
    P4AffinityRow r = {P4_ENTITY_ENEMY, enemy, profile, element, channel,
        channel == P4_CHANNEL_INSTANT_KO ? P4_KO_NORMAL : 100, 100};
    assert(p4_game_tables_affinity(tables, "test", &r) == P4_TABLE_OK);
}
/* Keys: (MAX,5,slash,dmg) shared by 300/301, (MAX,5,slash,KO), (MAX,5,strike),
 * (MAX,5,Fire), (MAX,6,slash) and (902,5,slash). */
static P4GameTables *fixture(void)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    leaf(tables, 65535, "test:slash"); leaf(tables, 21, "test:strike");
    alias(tables, 300, UINT32_MAX, 5, UINT32_MAX, "test:enemy300");
    alias(tables, 300, 6, 6, UINT32_MAX, "test:enemy300phase6");
    alias(tables, 301, UINT32_MAX, 5, UINT32_MAX, "test:enemy301shared");
    alias(tables, 302, UINT32_MAX, 5, 902, "test:enemy302");
    row(tables, 300, 5, 65535, 0); row(tables, 300, 5, 65535, 1);
    row(tables, 300, 5, 21, 0); row(tables, 300, 5, 1, 0); row(tables, 300, 6, 65535, 0);
    row(tables, 301, 5, 65535, 0); row(tables, 302, 5, 65535, 0);
    assert(p4_game_tables_bind_runtime_use(tables, runtime_use, tables) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(tables, 42) == P4_TABLE_OK);
    return tables;
}
static void status_leaf(P4GameTables *tables, uint32_t id, const char *key)
{
    P4ElementDesc d = {0};
    d.id = id; d.flags = P4_ELEMENT_AFFINITY | P4_ELEMENT_VISIBLE; d.revision = 3;
    strcpy(d.key, key); strcpy(d.display_name, "Ailment"); strcpy(d.short_name, "Ail");
    assert(p4_game_tables_element(tables, "test", &d) == P4_TABLE_OK);
}
/* Side ailments chill (22, default from Ice) and numb (23, no default): enemy
 * 300 has no chill row, so its chill pair is defaulted; enemy 302 (knowledge
 * 902) has rows for both; retail enemies 200/201 have no alias. */
static P4GameTables *side_fixture(void)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    leaf(tables, 65535, "test:slash");
    status_leaf(tables, 22, "test:chill"); status_leaf(tables, 23, "test:numb");
    const P4SideAilmentDef numb = {.leaf = 23, .cure_mask = 0x16, .default_from = P4_SIDE_DEFAULT_NONE,
        .schema = 1, .revision = 1, .key = "test:numb-ailment"};
    const P4SideAilmentDef chill = {.leaf = 22, .cure_mask = 0x16, .default_from = 2,
        .schema = 1, .revision = 1, .key = "test:chill-ailment"};
    assert(p4_game_tables_side_ailment(tables, "test", &numb) == P4_TABLE_OK);
    assert(p4_game_tables_side_ailment(tables, "test", &chill) == P4_TABLE_OK);
    alias(tables, 300, UINT32_MAX, 5, UINT32_MAX, "test:enemy300");
    alias(tables, 302, UINT32_MAX, 5, 902, "test:enemy302");
    row(tables, 300, 5, 65535, P4_CHANNEL_DAMAGE); row(tables, 300, 5, 23, P4_CHANNEL_STATUS);
    row(tables, 302, 5, 65535, P4_CHANNEL_DAMAGE); row(tables, 302, 5, 22, P4_CHANNEL_STATUS);
    row(tables, 302, 5, 23, P4_CHANNEL_STATUS);
    assert(p4_game_tables_bind_runtime_use(tables, runtime_use, tables) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(tables, 43) == P4_TABLE_OK);
    return tables;
}
static P4GameActionElement ailment_leaf(uint32_t id)
{
    return (P4GameActionElement){1, id, P4_CHANNEL_STATUS, P4_ELEMENT_AFFINITY, 0u, P4_ACTION_COMPONENT_STATUS};
}
static P4GameTableSources *sources_for(P4GameTables *tables, const P4PackageProvider *package, uint32_t revision)
{
    P4GameTableSources *sources = p4_game_table_sources_create(); assert(sources);
    uint32_t source;
    if (package) assert(p4_game_table_sources_package(sources, package,
        "test:knowledge-source", 1, revision, &source) == P4_TABLE_OK);
    else assert(p4_game_table_sources_native(sources, "test:knowledge-native", 1, revision, &source) == P4_TABLE_OK);
    assert(p4_game_table_sources_view(sources, P4_TABLE_SOURCE_REGISTRY, tables) == P4_TABLE_OK);
    P4TableSourceCursor cursor = {0};
    P4TableSourceEntry entry;
    int result;
    while ((result = p4_game_table_sources_next(P4_TABLE_SOURCE_REGISTRY, tables, &cursor, &entry)) == P4_TABLE_OK)
        assert(p4_game_table_sources_bind(sources, tables, &entry, source) == P4_TABLE_OK);
    assert(result == P4_TABLE_NOT_FOUND);
    assert(p4_game_table_sources_seal(sources) == P4_TABLE_OK);
    assert(p4_game_table_sources_freeze(sources) == P4_TABLE_OK);
    return sources;
}
static P4GameActionElement element(uint32_t id, uint32_t channel)
{
    return (P4GameActionElement){1, id, channel, P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY,
        id < 8u ? id : 0u, channel == P4_CHANNEL_INSTANT_KO ? P4_ACTION_COMPONENT_INSTANT_KO : P4_ACTION_COMPONENT_DAMAGE};
}
static int known_key(const P4GameKnowledge *state, uint32_t enemy, uint32_t profile, P4GameActionElement e)
{
    P4KnowledgeKey key;
    int known = -1;
    assert(p4_game_knowledge_resolve(state, enemy, profile, &e, &key) == P4_TABLE_OK);
    assert(p4_game_knowledge_test(state, &key, &known) == P4_TABLE_OK);
    return known;
}

static void retail_empty(void)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    P4GameKnowledge *state = NULL;
    assert(p4_game_knowledge_create(tables, &state) == P4_TABLE_INVALID);
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_OK);
    assert(p4_game_knowledge_create(tables, &state) == P4_TABLE_OK && state);
    assert(p4_game_knowledge_count(state) == 0);
    /* Retail status slots need no side row. */
    int known = 9;
    assert(p4_game_knowledge_status_observe(state, 335, 15) == P4_TABLE_OK);
    assert(p4_game_knowledge_status_test(state, 335, 15, &known) == P4_TABLE_OK && known == 1);
    assert(p4_game_knowledge_status_test(state, 335, 14, &known) == P4_TABLE_OK && known == 0);
    const P4KnowledgeRetail *retail = p4_game_knowledge_retail(state);
    assert(retail && retail->status[335] == 0x80u && !p4_game_knowledge_retail(NULL));
    assert(p4_game_knowledge_status_observe(state, 336, 8) == P4_TABLE_INVALID);
    assert(p4_game_knowledge_status_observe(state, 0, 7) == P4_TABLE_INVALID);
    assert(p4_game_knowledge_status_observe(state, 0, 16) == P4_TABLE_INVALID);
    assert(p4_game_knowledge_status_test(state, 0, 8, NULL) == P4_TABLE_INVALID);
    p4_game_knowledge_clear(state);
    assert(!retail->status[335]);
    /* Without a defaulted side ailment there is no side bit. */
    assert(p4_game_knowledge_side_observe(state, 0, 22) == P4_TABLE_NOT_FOUND);
    assert(p4_game_knowledge_side_test(state, 336, 22, &known) == P4_TABLE_INVALID);
    P4KnowledgeRetail bits;
    memset(&bits, 0, sizeof bits);
    bits.status[4] = 0x21u;
    assert(p4_game_knowledge_replace(state, NULL, 0, &bits) == P4_TABLE_OK);
    assert(retail->status[4] == 0x21u);
    assert(p4_game_knowledge_replace(state, NULL, 0, NULL) == P4_TABLE_OK && !retail->status[4]);
    bits.side[4] = 1u;
    assert(p4_game_knowledge_replace(state, NULL, 0, &bits) == P4_TABLE_INVALID && !retail->status[4]);
    p4_game_knowledge_destroy(state);
    assert(p4_game_knowledge_replace(NULL, NULL, 0, NULL) == P4_TABLE_OK);
    assert(p4_game_knowledge_replace(NULL, NULL, 0, &bits) == P4_TABLE_INVALID);
    p4_game_tables_destroy(tables);
}
static void identity_and_channels(void)
{
    P4GameTables *tables = fixture();
    P4GameKnowledge *state;
    assert(p4_game_knowledge_create(tables, &state) == P4_TABLE_OK);
    assert(p4_game_knowledge_count(state) == 6 && p4_game_knowledge_snapshot(state) == tables);
    P4GameActionElement slash = element(65535, P4_CHANNEL_DAMAGE);
    P4KnowledgeKey key;
    assert(p4_game_knowledge_resolve(state, 300, UINT32_MAX, &slash, &key) == P4_TABLE_OK);
    assert(key.knowledge_id == UINT32_MAX && key.profile_id == 5 && key.element_id == 65535 && key.channel_id == 0);
    assert(known_key(state, 300, UINT32_MAX, slash) == 0);
    assert(p4_game_knowledge_observe(state, &key) == P4_TABLE_OK);
    assert(known_key(state, 300, UINT32_MAX, slash) == 1);
    assert(known_key(state, 301, UINT32_MAX, slash) == 1); /* Deliberately shared identity. */
    assert(known_key(state, 302, UINT32_MAX, slash) == 0);
    assert(known_key(state, 300, 6, slash) == 0);
    assert(known_key(state, 300, UINT32_MAX, element(65535, P4_CHANNEL_INSTANT_KO)) == 0);
    assert(known_key(state, 300, UINT32_MAX, element(21, P4_CHANNEL_DAMAGE)) == 0);
    int any = 9;
    assert(p4_game_knowledge_any(state, UINT32_MAX, 5, &any) == P4_TABLE_OK && any == 1);
    assert(p4_game_knowledge_any(state, UINT32_MAX, 6, &any) == P4_TABLE_OK && any == 0);
    assert(p4_game_knowledge_any(state, 7, 7, &any) == P4_TABLE_OK && any == 0);
    assert(p4_game_knowledge_any(state, 902, 5, NULL) == P4_TABLE_INVALID);
    P4GameActionElement strike = element(21, P4_CHANNEL_DAMAGE), invalid = slash;
    P4KnowledgeKey unchanged = key;
    assert(p4_game_knowledge_resolve(state, 301, UINT32_MAX, &strike, &key) == P4_TABLE_NOT_FOUND);
    assert(p4_game_knowledge_resolve(state, 303, UINT32_MAX, &slash, &key) == P4_TABLE_NOT_FOUND);
    invalid.valid = 0;
    assert(p4_game_knowledge_resolve(state, 300, UINT32_MAX, &invalid, &key) == P4_TABLE_INVALID);
    assert(!memcmp(&key, &unchanged, sizeof key));
    P4KnowledgeKey missing = {7, 7, 7, 7};
    int known = 9;
    assert(p4_game_knowledge_test(state, &missing, &known) == P4_TABLE_NOT_FOUND && known == 9);
    assert(p4_game_knowledge_observe(state, &missing) == P4_TABLE_NOT_FOUND);
    p4_game_knowledge_clear(state);
    assert(known_key(state, 301, UINT32_MAX, slash) == 0);
    p4_game_knowledge_destroy(state); p4_game_tables_destroy(tables);
}
static void persistence_identity(void)
{
    P4GameTables *tables = fixture();
    P4GameKnowledge *state;
    assert(p4_game_knowledge_create(tables, &state) == P4_TABLE_OK);
    P4KnowledgeIdentity identity; int known;
    for (size_t i = 0; i < p4_game_knowledge_count(state); ++i) {
        assert(p4_game_knowledge_identity_at(state, i, &identity, &known) == P4_TABLE_OK && !known);
        if (identity.key.element_id == 65535 && identity.key.profile_id == 5 && identity.key.channel_id == 0 &&
            identity.key.knowledge_id == UINT32_MAX) {
            /* The lowest alias index (enemy 300) is the canonical shared identity. */
            assert(!strcmp(identity.alias_key, "test:enemy300") && !strcmp(identity.owner, "test"));
            assert(p4_game_knowledge_identity_alias_at(state, i)->entity_id == 300);
        }
        if (identity.key.element_id == 1) assert(!identity.element_owner); /* Unowned retail Fire. */
        else assert(identity.element_owner && !strcmp(identity.element_owner, "test"));
    }
    assert(p4_game_knowledge_identity_at(state, 0, &identity, &known) == P4_TABLE_OK);
    size_t index = 99;
    assert(p4_game_knowledge_identity_find(state, &identity, &index) == P4_TABLE_OK && index == 0);
    assert(p4_game_knowledge_replace(state, &index, 1, NULL) == P4_TABLE_OK);
    assert(p4_game_knowledge_identity_at(state, 0, &identity, &known) == P4_TABLE_OK && known);
    identity.alias_revision++;
    assert(p4_game_knowledge_identity_find(state, &identity, &index) == P4_TABLE_CONFLICT);
    identity.alias_revision--; identity.element_key = "missing:leaf";
    assert(p4_game_knowledge_identity_find(state, &identity, &index) == P4_TABLE_CONFLICT);
    size_t bad[] = {0, 99}, duplicate[] = {0, 0};
    assert(p4_game_knowledge_replace(state, bad, 2, NULL) == P4_TABLE_INVALID);
    assert(p4_game_knowledge_replace(state, duplicate, 2, NULL) == P4_TABLE_INVALID);
    assert(p4_game_knowledge_identity_at(state, 0, &identity, &known) == P4_TABLE_OK && known);
    assert(p4_game_knowledge_replace(state, NULL, 0, NULL) == P4_TABLE_OK);
    assert(p4_game_knowledge_identity_at(state, 0, &identity, &known) == P4_TABLE_OK && !known);
    p4_game_knowledge_destroy(state); p4_game_tables_destroy(tables);
}

static unsigned element_prepares, element_commits;
static int element_prepare(void *user) { (void)user; ++element_prepares; return P4_TABLE_OK; }
static void element_commit(void *user) { (void)user; ++element_commits; }
static int observe(uint32_t enemy, uint32_t native_profile, P4GameActionFrame frame)
{
    current_identity = (P4GameUnitIdentity){P4_ENTITY_ENEMY, enemy, native_profile};
    return p4_game_knowledge_native_observe(&defender, &frame);
}
static P4GameAffinityQuery ai_query(uint32_t enemy, uint32_t native_profile, P4GameActionFrame frame)
{
    current_identity = (P4GameUnitIdentity){P4_ENTITY_ENEMY, enemy, native_profile};
    P4GameAffinityQuery query = {7, 7};
    assert(p4_game_knowledge_native_ai_query(&defender, &frame, &query) == P4_TABLE_OK);
    assert(query.mode == P4_AFFINITY_QUERY_ACTUAL ? query.value == 0 : query.value == 100);
    return query;
}
static uint32_t any_known(uint32_t enemy, uint32_t native_profile)
{
    current_identity = (P4GameUnitIdentity){P4_ENTITY_ENEMY, enemy, native_profile};
    uint32_t known = 9;
    assert(p4_game_knowledge_native_any(&defender, &known) == P4_TABLE_OK && known <= 1);
    return known;
}
static uint32_t analyze(uint32_t enemy, uint32_t native_profile, P4GameActionElement element)
{
    current_identity = (P4GameUnitIdentity){P4_ENTITY_ENEMY, enemy, native_profile};
    uint32_t known = 9;
    assert(p4_game_knowledge_native_analyze(&defender, &element, &known) == P4_TABLE_OK && known <= 1);
    return known;
}
static void observe_route(void)
{
    P4GameTables *tables = fixture();
    P4GameTableSources *sources = sources_for(tables, NULL, 1);
    const P4GameActionFrame slash = {.element = element(65535, P4_CHANNEL_DAMAGE), .skill_id = 77};
    const unsigned before = identity_calls, commits = element_commits;
    assert(observe(300, UINT32_MAX, slash) == P4_TABLE_NOT_READY && identity_calls == before);
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    assert(p4_game_knowledge_native_boot_stage(tables) == P4_TABLE_NOT_READY); /* No save block yet. */
    assert(p4_game_knowledge_save_register(tables, sources) == P4_TABLE_OK);
    assert(p4_game_knowledge_native_boot_stage(tables) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete() == P4_TABLE_NOT_READY); /* Requires the element owner. */
    assert(p4_game_tables_battle_abort() == P4_TABLE_OK && p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_register("element", element_prepare, element_commit, NULL) == P4_TABLE_OK);
    assert(p4_game_knowledge_native_boot_stage(tables) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK && element_commits == commits + 1);
    assert(p4_game_knowledge_native_snapshot() == tables);
    P4GameKnowledge *state = p4_game_knowledge_native_active();
    assert(state && p4_game_knowledge_count(state) == 6);

    /* Authored alias/side pair: the key is learned, the retail bitmap untouched. */
    unsigned uses = runtime_uses;
    assert(observe(301, UINT32_MAX, slash) == P4_TABLE_OK && runtime_uses == uses + 2);
    assert(known_key(state, 300, UINT32_MAX, slash.element) == 1 && !bitmap_writes);
    P4GameActionFrame fire = {.element = element(1, P4_CHANNEL_DAMAGE)};
    assert(observe(300, UINT32_MAX, fire) == P4_TABLE_OK && !bitmap_writes);
    assert(known_key(state, 300, UINT32_MAX, fire.element) == 1);
    P4GameActionFrame ko = {.element = element(65535, P4_CHANNEL_INSTANT_KO)};
    assert(known_key(state, 300, UINT32_MAX, ko.element) == 0); /* Damage never reveals KO. */
    runtime_failure = P4_TABLE_INVALID;
    assert(observe(300, UINT32_MAX, ko) == P4_TABLE_INVALID);
    runtime_failure = 0;
    assert(known_key(state, 300, UINT32_MAX, ko.element) == 0);

    /* Nothing to learn without an element or an affinity-bearing leaf. */
    unsigned calls = identity_calls;
    P4GameActionFrame none = slash;
    none.element.valid = 0;
    assert(observe(300, UINT32_MAX, none) == P4_TABLE_OK);
    none = slash; none.element.flags = P4_ELEMENT_OFFENSIVE;
    assert(observe(300, UINT32_MAX, none) == P4_TABLE_OK && identity_calls == calls && !bitmap_writes);
    none.element.valid = 2;
    assert(observe(300, UINT32_MAX, none) == P4_TABLE_INVALID);
    assert(p4_game_knowledge_native_observe(NULL, &slash) == P4_TABLE_INVALID);
    assert(p4_game_knowledge_native_observe(&defender, NULL) == P4_TABLE_INVALID);

    /* Only an enemy identity learns; identity failures propagate. */
    current_identity = (P4GameUnitIdentity){P4_ENTITY_PERSONA, 1, 0};
    assert(p4_game_knowledge_native_observe(&defender, &slash) == P4_TABLE_INVALID);
    identity_result = P4_TABLE_NOT_FOUND;
    assert(observe(300, UINT32_MAX, slash) == P4_TABLE_NOT_FOUND);
    identity_result = 0;

    /* The original resolver's route keeps the original retail learning. */
    P4GameActionFrame ice = {.element = element(2, P4_CHANNEL_DAMAGE)};
    assert(observe(200, 0, ice) == P4_TABLE_OK && bitmap_writes == 1 && bitmap_entry == 200 && bitmap_bit == 2);
    assert(observe(300, UINT32_MAX, ice) == P4_TABLE_OK && bitmap_writes == 2 && bitmap_entry == 300);
    P4GameActionFrame mudo = {.element = element(7, P4_CHANNEL_INSTANT_KO)};
    assert(observe(335, 0, mudo) == P4_TABLE_OK && bitmap_writes == 3 && bitmap_entry == 335 && bitmap_bit == 7);
    P4GameActionFrame status = {.element = element(9, P4_CHANNEL_STATUS)};
    status.element.component = P4_ACTION_COMPONENT_STATUS;
    /* No retail bit beyond 7: a landed ailment has the store's own status bit. */
    assert(observe(200, 0, status) == P4_TABLE_OK && bitmap_writes == 3);
    int learned = 0;
    assert(p4_game_knowledge_status_test(state, 200, 9, &learned) == P4_TABLE_OK && learned == 1);
    assert(p4_game_knowledge_status_test(state, 200, 10, &learned) == P4_TABLE_OK && learned == 0);
    assert(observe(336, 0, ice) == P4_TABLE_NOT_FOUND && bitmap_writes == 3);
    P4GameActionFrame owned = ice;
    owned.owned_binding = 1;
    assert(observe(200, 0, owned) == P4_TABLE_NOT_FOUND);
    P4GameActionFrame wide = {.element = element(19, P4_CHANNEL_DAMAGE)};
    assert(observe(200, 0, wide) == P4_TABLE_NOT_FOUND);
    wide = (P4GameActionFrame){.element = element(2, 3)};
    assert(observe(200, 0, wide) == P4_TABLE_NOT_FOUND);
    assert(observe(200, 0, slash) == P4_TABLE_NOT_FOUND && bitmap_writes == 3);
    assert(observe(301, UINT32_MAX, (P4GameActionFrame){.element = element(21, P4_CHANNEL_DAMAGE)}) == P4_TABLE_NOT_FOUND);

    /* AI reads follow the same route: known is ACTUAL, unknown PROVIDED 100. */
    assert(ai_query(301, UINT32_MAX, slash).mode == P4_AFFINITY_QUERY_ACTUAL);
    P4GameAffinityQuery query = ai_query(300, UINT32_MAX, ko);
    assert(query.mode == P4_AFFINITY_QUERY_PROVIDED && query.value == 100);
    assert(ai_query(200, 0, ice).mode == P4_AFFINITY_QUERY_ACTUAL);
    assert(ai_query(201, 0, ice).mode == P4_AFFINITY_QUERY_PROVIDED);
    assert(ai_query(335, 0, mudo).mode == P4_AFFINITY_QUERY_ACTUAL);
    none = slash; none.element.flags = P4_ELEMENT_OFFENSIVE;
    assert(ai_query(201, 0, none).mode == P4_AFFINITY_QUERY_ACTUAL);
    current_identity = (P4GameUnitIdentity){P4_ENTITY_PERSONA, 1, 0};
    query = (P4GameAffinityQuery){7, 7};
    assert(p4_game_knowledge_native_ai_query(&defender, &ko, &query) == P4_TABLE_OK &&
        query.mode == P4_AFFINITY_QUERY_ACTUAL); /* Party affinities stay retail-unrestricted. */
    current_identity.entity_id = 336; current_identity.entity_kind = P4_ENTITY_ENEMY;
    query = (P4GameAffinityQuery){7, 7};
    assert(p4_game_knowledge_native_ai_query(&defender, &ice, &query) == P4_TABLE_NOT_FOUND && query.mode == 7);
    assert(p4_game_knowledge_native_ai_query(&defender, &ice, NULL) == P4_TABLE_INVALID);

    /* 001DB740 any-known: alias side keys plus original bits of natural
     * retail pairs without a side row; a retail-only enemy keeps its bits. */
    assert(any_known(300, UINT32_MAX) == 1 && any_known(200, 0) == 1 && any_known(201, 0) == 0);
    assert(any_known(302, UINT32_MAX) == 0);
    assert(observe(302, UINT32_MAX, ice) == P4_TABLE_OK && any_known(302, UINT32_MAX) == 1);
    p4_game_knowledge_clear(state);
    memset(retail_bitmap, 0, sizeof retail_bitmap);
    assert(any_known(300, UINT32_MAX) == 0);
    datSetBitmapBit(300, 1); /* Fire has an authored side row: the retail bit is not its knowledge. */
    assert(any_known(300, UINT32_MAX) == 0);
    datSetBitmapBit(300, 2);
    assert(any_known(300, UINT32_MAX) == 1);
    current_identity = (P4GameUnitIdentity){P4_ENTITY_ENEMY, 336, 0};
    uint32_t any = 9;
    assert(p4_game_knowledge_native_any(&defender, &any) == P4_TABLE_NOT_FOUND && any == 9);
    current_identity.entity_kind = P4_ENTITY_PERSONA;
    assert(p4_game_knowledge_native_any(&defender, &any) == P4_TABLE_INVALID && any == 9);
    memset(retail_bitmap, 0, sizeof retail_bitmap);
    assert(observe(301, UINT32_MAX, slash) == P4_TABLE_OK);

    /* Analyze: the same route for a displayed pair, then 001F0950's override. */
    assert(analyze(300, UINT32_MAX, slash.element) == 1 && analyze(300, UINT32_MAX, ko.element) == 0);
    assert(analyze(201, 0, ice.element) == 0);
    datSetBitmapBit(201, 2);
    assert(analyze(201, 0, ice.element) == 1);
    P4GameActionElement plain = slash.element;
    plain.flags = P4_ELEMENT_OFFENSIVE;
    assert(analyze(201, 0, plain) == 1); /* Nothing hidden without an affinity. */
    uint32_t shown = 9;
    current_identity = (P4GameUnitIdentity){P4_ENTITY_ENEMY, 201, 0};
    assert(p4_game_knowledge_native_analyze(&defender, &status.element, &shown) == P4_TABLE_OK && shown == 0);
    const unsigned writes = bitmap_writes;
    assert(p4_game_knowledge_native_observe(&defender, &status) == P4_TABLE_OK && bitmap_writes == writes);
    assert(analyze(201, 0, status.element) == 1); /* A landed ailment, outside the retail bitmap. */
    shown = 9;
    assert(p4_game_knowledge_native_analyze(&defender, &wide.element, &shown) == P4_TABLE_NOT_FOUND && shown == 9);
    /* The Analyze ailment row: analyze's answer plus the effective affinity class. */
    uint32_t ailment = 9, seen = 9;
    ailment_affinity = P4_AFF_WEAK | 125u;
    assert(p4_game_knowledge_native_ailment(&defender, 9, &ailment, &seen) == P4_TABLE_OK &&
        ailment == P4_KNOWLEDGE_AILMENT_WEAK && seen == 1);
    ailment_affinity = P4_AFF_NULL;
    assert(p4_game_knowledge_native_ailment(&defender, 10, &ailment, &seen) == P4_TABLE_OK &&
        ailment == P4_KNOWLEDGE_AILMENT_IMMUNE && seen == 0);
    ailment_affinity = P4_AFF_RESIST | 50u;
    assert(p4_game_knowledge_native_ailment(&defender, 10, &ailment, &seen) == P4_TABLE_OK &&
        ailment == P4_KNOWLEDGE_AILMENT_RESIST);
    ailment_affinity = 100u;
    assert(p4_game_knowledge_native_ailment(&defender, 10, &ailment, &seen) == P4_TABLE_OK &&
        ailment == P4_KNOWLEDGE_AILMENT_NORMAL);
    affinity_result = P4_TABLE_NOT_FOUND; ailment = seen = 9;
    assert(p4_game_knowledge_native_ailment(&defender, 9, &ailment, &seen) == P4_TABLE_NOT_FOUND &&
        ailment == 9 && seen == 9);
    affinity_result = 0;
    story_flags[0] = story_flags[2] = 1; /* 001F0950's override applies to the row too. */
    assert(p4_game_knowledge_native_ailment(&defender, 9, &ailment, &seen) == P4_TABLE_OK && seen == 0);
    story_flags[0] = story_flags[2] = 0;
    current_identity = (P4GameUnitIdentity){P4_ENTITY_ENEMY, 201, 0};
    current_identity.entity_kind = P4_ENTITY_PERSONA;
    assert(p4_game_knowledge_native_analyze(&defender, &slash.element, &shown) == P4_TABLE_INVALID && shown == 9);
    assert(p4_game_knowledge_native_analyze(&defender, NULL, &shown) == P4_TABLE_INVALID);
    story_flags[0] = story_flags[1] = 1;
    assert(analyze(300, UINT32_MAX, ko.element) == 1 && analyze(201, 0, element(3, P4_CHANNEL_DAMAGE)) == 1);
    story_flags[1] = 0; story_flags[2] = 1;
    assert(analyze(300, UINT32_MAX, slash.element) == 0 && analyze(201, 0, ice.element) == 0);
    enemy_metadata = 0x4000u; /* Enemies flagged 0x4004 keep their real knowledge. */
    assert(analyze(300, UINT32_MAX, slash.element) == 1 && analyze(300, UINT32_MAX, ko.element) == 0);
    enemy_metadata = 0; story_flags[0] = 0;
    assert(analyze(300, UINT32_MAX, slash.element) == 1);
    current_identity = (P4GameUnitIdentity){P4_ENTITY_ENEMY, 300, UINT32_MAX};
    metadata_result = P4_TABLE_NOT_READY;
    assert(p4_game_knowledge_native_analyze(&defender, &slash.element, &shown) == P4_TABLE_NOT_READY && shown == 9);
    metadata_result = 0; story_flags[2] = 0;
    /* AI reads never take the UI override. */
    story_flags[0] = story_flags[1] = 1;
    assert(ai_query(300, UINT32_MAX, ko).mode == P4_AFFINITY_QUERY_PROVIDED);
    story_flags[0] = story_flags[1] = 0;
    memset(retail_bitmap, 0, sizeof retail_bitmap);

    /* A real reload revalidates the same snapshot without erasing observations. */
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK && element_commits == commits + 2);
    assert(p4_game_knowledge_native_active() == state && known_key(state, 300, UINT32_MAX, slash.element) == 1);
    P4GameTables *other = fixture();
    assert(p4_game_knowledge_native_boot_stage(other) == P4_TABLE_NOT_READY); /* Save bound elsewhere. */
    p4_game_tables_destroy(other);

    assert(p4_game_knowledge_boot_session_end() == P4_TABLE_OK);
    assert(!p4_game_knowledge_native_active() && !p4_game_knowledge_native_snapshot());
    assert(observe(300, UINT32_MAX, slash) == P4_TABLE_NOT_READY);
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    p4_game_table_sources_destroy(sources); p4_game_tables_destroy(tables);
    bitmap_writes = 0;
}

static uint8_t *capture_copy(const P4ModSaveCapture *capture)
{
    uint8_t *copy = malloc(capture->sidecar_size); assert(copy);
    memcpy(copy, capture->sidecar, capture->sidecar_size);
    return copy;
}
static void put_word(uint8_t *bytes, uint32_t word)
{
    bytes[0] = (uint8_t)word; bytes[1] = (uint8_t)(word >> 8);
    bytes[2] = (uint8_t)(word >> 16); bytes[3] = (uint8_t)(word >> 24);
}
static int reject_validation(const P4ModSaveContext *context, void *staged, void *user)
{
    (void)context; (void)staged; (void)user;
    return P4_TABLE_NOT_READY;
}
static int omitted_block(const P4ModSaveContext *context, P4ModSaveWriter *writer, void *user)
{
    (void)context; (void)writer; (void)user; return 1;
}
static int empty_decode(const P4ModSaveContext *context, uint32_t schema,
    P4ModSaveReader *reader, void **staged, void *user)
{
    (void)context; (void)schema; (void)reader; (void)staged; (void)user; return 0;
}
static void empty_finish(void *staged, void *user) { (void)staged; (void)user; }
static void publish(P4GameTables *tables)
{
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_register("element", element_prepare, element_commit, NULL) == P4_TABLE_OK);
    assert(p4_game_knowledge_native_boot_stage(tables) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK);
}
static void save_transactions(const P4PackageProvider *package)
{
    char error[512];
    uint8_t image_bytes[4100] = {0}, image_copy[sizeof image_bytes];
    /* Synthetic, independently valid retail record stream; no game assets. */
    put_word(image_bytes + 56, 0x2000); put_word(image_bytes + 60, 1);
    put_word(image_bytes + 65, UINT32_MAX);
    memcpy(image_copy, image_bytes, sizeof image_bytes);
    const P4ModSaveImage image = {image_bytes, sizeof image_bytes, "synthetic"};
    P4GameTables *tables = fixture();
    P4GameTableSources *sources = sources_for(tables, NULL, 1);
    assert(p4_game_knowledge_save_register(tables, sources) == P4_TABLE_OK);
    P4GameTableSources *unsealed = p4_game_table_sources_create(); assert(unsealed);
    assert(p4_game_knowledge_save_register(tables, unsealed) == P4_TABLE_INVALID);
    p4_game_table_sources_destroy(unsealed);
    publish(tables);
    P4GameKnowledge *state = p4_game_knowledge_native_active();
    const P4GameActionFrame slash = {.element = element(65535, P4_CHANNEL_DAMAGE)};
    const P4GameActionElement ko = element(65535, P4_CHANNEL_INSTANT_KO);

    /* No known side key emits nothing; a retail-route bit is not a side key. */
    assert(observe(200, 0, (P4GameActionFrame){.element = element(2, P4_CHANNEL_DAMAGE)}) == P4_TABLE_OK);
    P4ModSaveCapture *capture = NULL;
    assert(p4_mod_save_capture_image(&image, &capture, error, sizeof error) == 0 && !capture);
    assert(observe(300, UINT32_MAX, slash) == P4_TABLE_OK);
    assert(p4_game_knowledge_status_observe(state, 201, 9) == P4_TABLE_OK);
    unsigned uses = runtime_uses;
    assert(p4_mod_save_capture_image(&image, &capture, error, sizeof error) == 0 && capture);
    assert(!memcmp(image_bytes, image_copy, sizeof image_bytes));
    assert(runtime_uses == uses); /* Encoding references is not gameplay use. */
    p4_game_knowledge_clear(state);
    P4ModSaveLoad *load = NULL;
    assert(p4_mod_save_stage_image(&image, capture_copy(capture), capture->sidecar_size,
        &load, error, sizeof error) == 0 && load);
    assert(runtime_uses == uses && known_key(state, 300, UINT32_MAX, slash.element) == 0);
    p4_mod_save_load_abort(load); load = NULL;
    assert(known_key(state, 300, UINT32_MAX, slash.element) == 0);

    /* A changed source ledger, a rejecting sibling or a missing REQUIRED
     * provider refuses the whole transaction. */
    P4GameTableSources *changed = sources_for(tables, NULL, 2);
    assert(p4_game_knowledge_save_unregister() == P4_TABLE_OK);
    assert(p4_game_knowledge_save_register(tables, changed) == P4_TABLE_OK);
    assert(p4_mod_save_stage_image(&image, capture_copy(capture), capture->sidecar_size,
        &load, error, sizeof error) != 0 && !load);
    assert(p4_game_knowledge_save_unregister() == P4_TABLE_OK);
    p4_game_table_sources_destroy(changed);
    assert(p4_game_knowledge_save_register(tables, sources) == P4_TABLE_OK);
    P4ModSaveRegistration *rejector = NULL;
    const P4ModSaveProvider rejecting = {sizeof rejecting, P4_MOD_SAVE_ABI_VERSION,
        "test.native", "reject", 1, 0, 16, NULL, omitted_block, empty_decode,
        reject_validation, empty_finish, empty_finish};
    assert(p4_mod_save_register(&rejecting, &rejector) == 0);
    assert(p4_mod_save_stage_image(&image, capture_copy(capture), capture->sidecar_size,
        &load, error, sizeof error) != 0 && !load);
    assert(p4_mod_save_unregister(rejector) == 0);
    assert(p4_game_knowledge_save_unregister() == P4_TABLE_OK);
    assert(p4_mod_save_stage_image(&image, capture_copy(capture), capture->sidecar_size,
        &load, error, sizeof error) != 0 && !load);
    assert(known_key(state, 300, UINT32_MAX, slash.element) == 0);

    /* Exact restore, then the absent block clears to the default. */
    assert(p4_game_knowledge_save_register(tables, sources) == P4_TABLE_OK);
    assert(p4_mod_save_stage_image(&image, capture_copy(capture), capture->sidecar_size,
        &load, error, sizeof error) == 0 && load);
    p4_mod_save_load_commit(load); load = NULL;
    assert(known_key(state, 301, UINT32_MAX, slash.element) == 1 && known_key(state, 300, UINT32_MAX, ko) == 0);
    assert(p4_game_knowledge_retail(state)->status[201] == 0x02u);
    assert(p4_mod_save_stage_image(&image, NULL, 0, &load, error, sizeof error) == 0 && load);
    p4_mod_save_load_commit(load); load = NULL;
    assert(known_key(state, 300, UINT32_MAX, slash.element) == 0 && !p4_game_knowledge_retail(state)->status[201]);
    p4_mod_save_capture_free(capture); capture = NULL;
    /* A status-only block round-trips: retail enemies and slots carry no reference. */
    assert(p4_game_knowledge_status_observe(state, 200, 12) == P4_TABLE_OK);
    assert(p4_mod_save_capture_image(&image, &capture, error, sizeof error) == 0 && capture);
    p4_game_knowledge_clear(state);
    assert(p4_mod_save_stage_image(&image, capture_copy(capture), capture->sidecar_size,
        &load, error, sizeof error) == 0 && load);
    p4_mod_save_load_commit(load); load = NULL;
    assert(p4_game_knowledge_retail(state)->status[200] == 0x10u && known_key(state, 300, UINT32_MAX, slash.element) == 0);
    p4_mod_save_capture_free(capture); capture = NULL;
    assert(p4_mod_save_reset_session(error, sizeof error) == 0);
    assert(p4_game_knowledge_boot_session_end() == P4_TABLE_OK);
    p4_game_table_sources_destroy(sources);

    /* A selected package origin round-trips through the same ledger codec. */
    sources = sources_for(tables, package, 1);
    assert(p4_game_knowledge_save_register(tables, sources) == P4_TABLE_OK);
    publish(tables);
    state = p4_game_knowledge_native_active();
    assert(observe(302, UINT32_MAX, slash) == P4_TABLE_OK);
    assert(p4_mod_save_capture_image(&image, &capture, error, sizeof error) == 0 && capture);
    p4_game_knowledge_clear(state);
    assert(p4_mod_save_stage_image(&image, capture_copy(capture), capture->sidecar_size,
        &load, error, sizeof error) == 0 && load);
    p4_mod_save_load_commit(load);
    assert(known_key(state, 302, UINT32_MAX, slash.element) == 1 && known_key(state, 300, UINT32_MAX, slash.element) == 0);
    assert(p4_mod_save_reset_session(error, sizeof error) == 0);
    assert(known_key(state, 302, UINT32_MAX, slash.element) == 0);
    p4_mod_save_capture_free(capture);
    assert(!memcmp(image_bytes, image_copy, sizeof image_bytes));
    assert(p4_game_knowledge_boot_session_end() == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    p4_game_table_sources_destroy(sources); p4_game_tables_destroy(tables);
}
/* Defaulted side-ailment pairs: an aliased enemy's key without a row, a
 * retail enemy's side bit, both routes, and their schema 3 round trip. */
static void side_defaults(void)
{
    P4GameTables *tables = side_fixture();
    P4GameKnowledge *store = NULL;
    assert(p4_game_knowledge_create(tables, &store) == P4_TABLE_OK);
    /* Five row keys plus 300's defaulted chill; numb declares no default. */
    assert(p4_game_knowledge_count(store) == 6);
    const P4GameActionElement chill = ailment_leaf(22), numb = ailment_leaf(23);
    P4KnowledgeKey key;
    assert(p4_game_knowledge_resolve(store, 300, UINT32_MAX, &chill, &key) == P4_TABLE_OK &&
        key.knowledge_id == UINT32_MAX && key.profile_id == 5 && key.element_id == 22 &&
        key.channel_id == P4_CHANNEL_STATUS);
    P4GameActionElement chill_damage = chill;
    chill_damage.channel_id = P4_CHANNEL_DAMAGE;
    assert(p4_game_knowledge_resolve(store, 300, UINT32_MAX, &chill_damage, &key) == P4_TABLE_NOT_FOUND);
    assert(p4_game_knowledge_resolve(store, 200, 0, &chill, &key) == P4_TABLE_NOT_FOUND);
    for (size_t i = 0; i < p4_game_knowledge_count(store); ++i) {
        P4KnowledgeIdentity identity;
        int known;
        assert(p4_game_knowledge_identity_at(store, i, &identity, &known) == P4_TABLE_OK);
        assert(identity.defaulted == (identity.key.knowledge_id == UINT32_MAX && identity.key.element_id == 22));
    }
    /* One bit per retail enemy and defaulted side-ailment index (chill sorts first). */
    int known = 9;
    assert(p4_game_knowledge_side_observe(store, 200, 22) == P4_TABLE_OK);
    assert(p4_game_knowledge_side_test(store, 200, 22, &known) == P4_TABLE_OK && known == 1);
    assert(p4_game_knowledge_side_test(store, 201, 22, &known) == P4_TABLE_OK && known == 0);
    assert(p4_game_knowledge_side_test(store, 200, 22, NULL) == P4_TABLE_INVALID);
    assert(p4_game_knowledge_side_observe(store, 200, 23) == P4_TABLE_NOT_FOUND);
    assert(p4_game_knowledge_side_observe(store, 336, 22) == P4_TABLE_INVALID);
    assert(p4_game_knowledge_retail(store)->side[200] == 1u);
    P4KnowledgeRetail bits;
    memset(&bits, 0, sizeof bits);
    bits.side[7] = 2u; /* numb's index has no bit. */
    assert(p4_game_knowledge_replace(store, NULL, 0, &bits) == P4_TABLE_INVALID &&
        p4_game_knowledge_retail(store)->side[200] == 1u);
    bits.side[7] = 1u;
    assert(p4_game_knowledge_replace(store, NULL, 0, &bits) == P4_TABLE_OK &&
        p4_game_knowledge_retail(store)->side[7] == 1u && !p4_game_knowledge_retail(store)->side[200]);
    p4_game_knowledge_destroy(store);

    char error[512];
    uint8_t image_bytes[4100] = {0};
    put_word(image_bytes + 56, 0x2000); put_word(image_bytes + 60, 1);
    put_word(image_bytes + 65, UINT32_MAX);
    const P4ModSaveImage image = {image_bytes, sizeof image_bytes, "synthetic"};
    P4GameTableSources *sources = sources_for(tables, NULL, 1);
    assert(p4_game_knowledge_save_register(tables, sources) == P4_TABLE_OK);
    publish(tables);
    P4GameKnowledge *state = p4_game_knowledge_native_active();
    memset(retail_bitmap, 0, sizeof retail_bitmap);
    const unsigned writes = bitmap_writes;
    const P4GameActionFrame chill_frame = {.element = chill}, numb_frame = {.element = numb};
    /* An aliased enemy learns its defaulted key through the side-ailment row. */
    assert(ai_query(300, UINT32_MAX, chill_frame).mode == P4_AFFINITY_QUERY_PROVIDED);
    unsigned uses = runtime_uses;
    assert(observe(300, UINT32_MAX, chill_frame) == P4_TABLE_OK && runtime_uses == uses + 2);
    assert(known_key(state, 300, UINT32_MAX, chill) == 1 && bitmap_writes == writes);
    assert(ai_query(300, UINT32_MAX, chill_frame).mode == P4_AFFINITY_QUERY_ACTUAL);
    assert(analyze(300, UINT32_MAX, chill) == 1 && any_known(300, UINT32_MAX) == 1);
    /* A retail enemy without an alias learns its side bit on the same route. */
    assert(ai_query(200, 0, chill_frame).mode == P4_AFFINITY_QUERY_PROVIDED && analyze(200, 0, chill) == 0);
    uses = runtime_uses;
    assert(observe(200, 0, chill_frame) == P4_TABLE_OK && runtime_uses == uses + 1 && bitmap_writes == writes);
    assert(ai_query(200, 0, chill_frame).mode == P4_AFFINITY_QUERY_ACTUAL && analyze(200, 0, chill) == 1);
    uint32_t ailment = 9, seen = 9;
    ailment_affinity = P4_AFF_WEAK | 125u;
    assert(p4_game_knowledge_native_ailment(&defender, 22, &ailment, &seen) == P4_TABLE_OK &&
        ailment == P4_KNOWLEDGE_AILMENT_WEAK && seen == 1);
    ailment_affinity = 100u;
    /* Like a retail ailment, a side bit is not 0x001db740's any-known. */
    assert(any_known(200, 0) == 0);
    /* Without a default or a row there is no pair; an added enemy needs an alias. */
    assert(observe(200, 0, numb_frame) == P4_TABLE_NOT_FOUND);
    assert(observe(336, 0, chill_frame) == P4_TABLE_NOT_FOUND);

    /* The defaulted key carries its side-ailment reference, the retail bit its ledger entry. */
    P4ModSaveCapture *capture = NULL;
    assert(p4_mod_save_capture_image(&image, &capture, error, sizeof error) == 0 && capture);
    p4_game_knowledge_clear(state);
    assert(known_key(state, 300, UINT32_MAX, chill) == 0 && !p4_game_knowledge_retail(state)->side[200]);
    P4ModSaveLoad *load = NULL;
    assert(p4_mod_save_stage_image(&image, capture_copy(capture), capture->sidecar_size,
        &load, error, sizeof error) == 0 && load);
    p4_mod_save_load_commit(load); load = NULL;
    assert(known_key(state, 300, UINT32_MAX, chill) == 1 && p4_game_knowledge_retail(state)->side[200] == 1u);
    p4_mod_save_capture_free(capture); capture = NULL;
    /* A side-bit-only block round-trips too. */
    p4_game_knowledge_clear(state);
    assert(p4_game_knowledge_side_observe(state, 201, 22) == P4_TABLE_OK);
    assert(p4_mod_save_capture_image(&image, &capture, error, sizeof error) == 0 && capture);
    p4_game_knowledge_clear(state);
    assert(p4_mod_save_stage_image(&image, capture_copy(capture), capture->sidecar_size,
        &load, error, sizeof error) == 0 && load);
    p4_mod_save_load_commit(load); load = NULL;
    assert(p4_game_knowledge_retail(state)->side[201] == 1u && known_key(state, 300, UINT32_MAX, chill) == 0);
    p4_mod_save_capture_free(capture);
    assert(p4_mod_save_reset_session(error, sizeof error) == 0);
    assert(p4_game_knowledge_boot_session_end() == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    p4_game_table_sources_destroy(sources); p4_game_tables_destroy(tables);
}

/* Stand-in for Root's two passes over the same borrowed products. */
static P4GameTablesBootProduct root_cpu, root_runtime;
static int root_prepare(void *user) { (void)user; return p4_package_boot_provide(root_cpu.tables ? &root_cpu : NULL); }
static int root_runtime_prepare(void *user)
{
    (void)user;
    if (root_runtime.tables && p4_game_elements_requested(root_runtime.tables))
        assert(p4_game_tables_battle_complete_register("element", element_prepare, element_commit, NULL) == P4_TABLE_OK);
    return p4_package_boot_provide(root_runtime.tables ? &root_runtime : NULL);
}
/* The handler is registered before load; one publication serves the run. */
static void boot_handler(int expanded, P4PackageRegistry **registry, const P4PackageClient *client, const char *root_dir)
{
    P4GameTables *tables = expanded ? fixture() : p4_game_tables_create();
    assert(tables);
    if (!expanded) assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_OK && !p4_game_elements_requested(tables));
    P4GameTableSources *sources = sources_for(tables, NULL, 1);
    root_cpu = (P4GameTablesBootProduct){.tables = tables, .sources = sources};
    root_runtime = root_cpu;
    static const P4PackageHandler root = {.name = P4_GAME_TABLES_BOOT_PROVIDER, .prepare = root_prepare,
        .boot_phase = P4_PACKAGE_BOOT_TABLES, .runtime_prepare = root_runtime_prepare};
    assert(!p4_package_handler_register(&p4_game_knowledge_boot_provider)); /* Reversed on purpose. */
    assert(!p4_package_handler_register(&root));
    assert(!p4_package_registry_load(registry, client, root_dir, "knowledge.mod"));
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    assert(!p4_package_registry_publish() && p4_packages_count() == 1);
    assert(p4_game_knowledge_save_ready(tables) == (expanded ? P4_TABLE_OK : P4_TABLE_NOT_READY));
    const P4PackageProvider *failed = NULL; const char *reason = NULL;
    assert(!p4_package_registry_runtime_materialize(&failed, &reason) && !failed && !reason);
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK);
    assert(p4_game_knowledge_native_snapshot() == (expanded ? tables : NULL));
    assert(element_prepares == (unsigned)expanded && element_commits == (unsigned)expanded);
    if (expanded) {
        const P4GameActionFrame slash = {.element = element(65535, P4_CHANNEL_DAMAGE)};
        assert(observe(300, UINT32_MAX, slash) == P4_TABLE_OK);
        assert(known_key(p4_game_knowledge_native_active(), 301, UINT32_MAX, slash.element) == 1);
    }
    assert(p4_game_knowledge_boot_session_end() == P4_TABLE_OK);
    assert(p4_game_knowledge_save_ready(tables) == P4_TABLE_NOT_READY && !p4_game_knowledge_native_snapshot());
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    p4_game_table_sources_destroy(sources); p4_game_tables_destroy(tables);
    root_cpu = root_runtime = (P4GameTablesBootProduct){0};
}

int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    const int expanded = !strcmp(argv[1], "expanded");
    if (!expanded && strcmp(argv[1], "unexpanded")) return 2;
    static const P4PackageClient client = {.diagnostic_name = "knowledge fixture", .manifest_name = "mod.ini",
        .section = "[mod]", .kind = P4_PACKAGE_MOD};
    P4PackageRegistry *registry = NULL;
    boot_handler(expanded, &registry, &client, argv[2]);
    if (expanded) {
        retail_empty(); identity_and_channels(); persistence_identity(); observe_route();
        save_transactions(p4_packages_provider(0)); side_defaults();
    }
    p4_package_registry_retire();
    assert(!registry && !p4_packages_memory_usage().resident && !p4_packages_memory_usage().reserved);
    puts(expanded ? "game_knowledge: alias/profile/channel keys, resolver-route observation, completion participant, "
        "package handler and REQUIRED save block passed; no gameplay claim" :
        "game_knowledge: unexpanded tables leave the handler, save block and participant inert");
    return 0;
}
