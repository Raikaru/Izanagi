/* Real Core/VFS original inputs, CPU seal and guest materialization of the five
 * enemy banks. Synthetic banks do not prove native battle/model/AI behavior. */
#include "p4pc/game_enemies.h"
#include "p4pc/game_table_inputs.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define RETAIL P4_ENEMY_RETAIL_ROWS
static unsigned allocations, releases, original_uses, authored_uses;
static int fail_allocate, use_status;
static struct Fixture {
    P4GameEnemies *enemies, *empty;
    P4GameText *names;
    P4GameTableInput *unit, *model, *ai, *message;
    const char *mode;
    unsigned published;
} fixture;
static uint8_t metadata[RETAIL * P4_ENEMY_ROW_SIZE], affinity[RETAIL * P4_ENEMY_AFFINITY_SIZE];
static uint8_t model[RETAIL * P4_ENEMY_MODEL_SIZE], ai[RETAIL * P4_ENEMY_AI_SIZE];
static uint8_t names[RETAIL * P4_ENEMY_NAME_SIZE];
static void *allocate(void *user, uint32_t size, uint32_t *guest)
{
    (void)user;
    ++allocations;
    if (fail_allocate) return NULL;
    void *host = p4t_galloc(size, 16);
    if (host) *guest = P4_H2G(host);
    return host;
}
static void release(void *user, uint32_t guest, void *host, uint32_t size)
{
    (void)user;
    assert(size && guest == P4_H2G(host));
    ++releases;
}
static int original_use(const void *span, void *context)
{
    assert(span == p4_game_table_input_span(fixture.unit, 0) && context == fixture.unit);
    ++original_uses;
    return use_status;
}
static int authored_use(const P4GameEnemies *view, uint32_t id, void *context)
{
    assert(view == fixture.enemies && context == &fixture && id >= RETAIL);
    ++authored_uses;
    return use_status;
}
static int text_use(const P4GameText *view, uint32_t table, uint32_t id, uint32_t role, void *context)
{
    (void)view; (void)table; (void)id; (void)role; (void)context;
    return 0;
}
static void register_name(P4GameText *text, uint32_t id, size_t size)
{
    char bytes[32];
    assert(size && size <= sizeof bytes);
    memset(bytes, 'A' + (int)(id % 26u), size - 1);
    bytes[size - 1] = 0;
    assert(p4_game_tables_text_register(text, "test", P4_TEXT_ENEMY, id, P4_TEXT_NAME, 1,
        (const uint8_t *)bytes, size) == P4_TABLE_OK);
}
static P4GameEnemyDesc enemy(uint32_t id, uint32_t model_source, const char *key)
{
    P4GameEnemyDesc row = {.id = id, .model_source = model_source, .schema = 1, .revision = 3};
    const uint8_t seed = (uint8_t)(id - RETAIL);
    strcpy(row.key, key);
    memset(row.metadata, seed + 1, sizeof row.metadata);
    memset(row.affinity, seed + 11, sizeof row.affinity);
    memset(row.model, seed + 21, sizeof row.model);
    memset(row.ai, seed + 31, sizeof row.ai);
    return row;
}
static void release_inputs(void)
{
    p4_game_table_input_destroy(fixture.unit);
    p4_game_table_input_destroy(fixture.model);
    p4_game_table_input_destroy(fixture.ai);
    p4_game_table_input_destroy(fixture.message);
    fixture.unit = fixture.model = fixture.ai = fixture.message = NULL;
}
static int prepare(void *user)
{
    assert(!user && !allocations);
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_UNIT, 1048576, &fixture.unit, NULL));
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_MODEL, 1048576, &fixture.model, NULL));
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_AICALC, 1048576, &fixture.ai, NULL));
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_MSG, 1048576, &fixture.message, NULL));
    assert(!p4_game_table_input_seal(fixture.unit) && !p4_game_table_input_seal(fixture.model));
    assert(!p4_game_table_input_seal(fixture.ai) && !p4_game_table_input_seal(fixture.message));
    const P4GameTableInputSpan *unit0 = p4_game_table_input_span(fixture.unit, 0);
    const P4GameTableInputSpan *unit1 = p4_game_table_input_span(fixture.unit, 1);
    const P4GameTableInputSpan *model1 = p4_game_table_input_span(fixture.model, 1);
    const P4GameTableInputSpan *ai8 = p4_game_table_input_span(fixture.ai, 8);
    const P4GameTableInputSpan *msg2 = p4_game_table_input_span(fixture.message, 2);
    P4GameEnemies *enemies = fixture.enemies;
    if (strcmp(fixture.mode, "valid")) {
        assert(p4_game_enemies_seal(enemies, unit0, unit1, model1, ai8, msg2, fixture.names, 8) == P4_TABLE_INVALID);
        assert(p4_game_enemies_seal(fixture.empty, unit0, unit1, model1, ai8, msg2, NULL, 8) == P4_TABLE_INVALID);
        assert(!allocations && !p4_game_enemies_semantic_revision(enemies));
        return -1; /* The real Core transaction must roll back, never publish. */
    }
    /* Every bank is its own retail pointer slot, never a neighbouring block. */
    assert(p4_game_enemies_seal(enemies, unit1, unit0, model1, ai8, msg2, fixture.names, 8) == P4_TABLE_INVALID);
    assert(p4_game_enemies_seal(enemies, unit0, unit1, p4_game_table_input_span(fixture.model, 0), ai8, msg2,
        fixture.names, 8) == P4_TABLE_INVALID);
    assert(p4_game_enemies_seal(enemies, unit0, unit1, model1, p4_game_table_input_span(fixture.ai, 7), msg2,
        fixture.names, 8) == P4_TABLE_INVALID);
    assert(p4_game_enemies_seal(enemies, unit0, unit1, model1, ai8, p4_game_table_input_span(fixture.message, 1),
        fixture.names, 8) == P4_TABLE_INVALID);
    assert(p4_game_enemies_seal(enemies, unit0, unit1, model1, ai8, NULL, fixture.names, 8) == P4_TABLE_INVALID);
    /* UNIT0 and UNIT1 are one sealed UNIT.TBL, not two loads. */
    P4GameTableInput *other_unit = NULL;
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_UNIT, 1048576, &other_unit, NULL));
    assert(!p4_game_table_input_seal(other_unit));
    assert(p4_game_enemies_seal(enemies, unit0, p4_game_table_input_span(other_unit, 1), model1, ai8, msg2,
        fixture.names, 8) == P4_TABLE_INVALID);
    p4_game_table_input_destroy(other_unit);
    P4TextProfile message = {P4_TEXT_ENCODING_US_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4TextProfile raw = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4GameText *text = p4_game_tables_text_create(NULL, &message);
    assert(text && !p4_game_tables_text_seal(text, 1));
    assert(p4_game_enemies_seal(enemies, unit0, unit1, model1, ai8, msg2, text, 8) == P4_TABLE_INVALID);
    p4_game_tables_text_destroy(text);
    text = p4_game_tables_text_create(NULL, &raw);
    assert(text);
    assert(p4_game_enemies_seal(enemies, unit0, unit1, model1, ai8, msg2, text, 8) == P4_TABLE_NOT_READY);
    assert(p4_game_enemies_seal(enemies, unit0, unit1, model1, ai8, msg2, NULL, 8) == P4_TABLE_NOT_READY);
    p4_game_tables_text_destroy(text);
    /* The MSG2 stride is 21 bytes including NUL. */
    text = p4_game_tables_text_create(NULL, &raw);
    register_name(text, RETAIL, P4_ENEMY_NAME_SIZE + 1u);
    assert(text && !p4_game_tables_text_seal(text, 1));
    P4GameEnemies *other = p4_game_enemies_create(NULL);
    P4GameEnemyDesc row = enemy(RETAIL, 1, "test:long-name");
    assert(other && !p4_game_enemies_register(other, "test", &row));
    assert(p4_game_enemies_seal(other, unit0, unit1, model1, ai8, msg2, text, 8) == P4_TABLE_INVALID);
    p4_game_enemies_destroy(other);
    p4_game_tables_text_destroy(text);
    /* Dense IDs: no zero row is ever invented between retail and added rows. */
    other = p4_game_enemies_create(NULL);
    row = enemy(RETAIL + 1u, 1, "test:sparse");
    assert(other && !p4_game_enemies_register(other, "test", &row));
    assert(p4_game_enemies_seal(other, unit0, unit1, model1, ai8, msg2, fixture.names, 8) == P4_TABLE_INVALID);
    p4_game_enemies_destroy(other);
    other = p4_game_enemies_create(NULL);
    for (uint32_t id = RETAIL; id < RETAIL + 4u; ++id) {
        char key[32];
        snprintf(key, sizeof key, "test:unnamed-%u", (unsigned)id);
        row = enemy(id, 1, key);
        assert(other && !p4_game_enemies_register(other, "test", &row));
    }
    assert(p4_game_enemies_seal(other, unit0, unit1, model1, ai8, msg2, fixture.names, 8) == P4_TABLE_NOT_FOUND);
    assert(!p4_game_enemies_semantic_revision(other) && !p4_game_enemies_find(other, RETAIL));
    p4_game_enemies_destroy(other);
    /* An empty snapshot needs no names and keeps the retail domain. */
    assert(!p4_game_enemies_seal(fixture.empty, unit0, unit1, model1, ai8, msg2, NULL, 8));
    assert(!p4_game_enemies_validate_runtime_sources(fixture.empty));
    assert(!p4_game_enemies_seal(enemies, unit0, unit1, model1, ai8, msg2, fixture.names, 8));
    assert(!p4_game_enemies_seal(enemies, unit0, unit1, model1, ai8, msg2, fixture.names, 8));
    assert(p4_game_enemies_seal(enemies, unit0, unit1, model1, ai8, msg2, fixture.names, 9) == P4_TABLE_CONFLICT);
    assert(p4_game_enemies_seal(enemies, unit0, unit1, model1, ai8, msg2, NULL, 8) == P4_TABLE_CONFLICT);
    row = enemy(RETAIL + 3u, 1, "test:late");
    assert(p4_game_enemies_register(enemies, "test", &row) == P4_TABLE_FROZEN);
    assert(p4_game_enemies_bind_runtime_use(enemies, original_use, authored_use, &fixture) == P4_TABLE_FROZEN);
    assert(!allocations && p4_game_enemies_semantic_revision(enemies) == 8 && !p4_game_enemies_revision(enemies));
    assert(!p4_game_enemies_validate_runtime_sources(enemies) && !p4_game_enemies_native_admit(enemies));
    assert(p4_game_enemies_runtime_use(enemies, RETAIL) == P4_TABLE_NOT_READY);
    assert(p4_game_enemies_count(enemies) == 3 && p4_game_enemies_limit(enemies) == RETAIL + 3u);
    static const uint32_t sources[3] = {5, 7, RETAIL - 1u};
    for (uint32_t i = 0; i < 3; ++i) {
        const P4GameEnemyView *view = p4_game_enemies_at(enemies, i);
        uint32_t model_id = UINT32_MAX;
        assert(view && view == p4_game_enemies_find(enemies, RETAIL + i));
        assert(view->desc.id == RETAIL + i && !strcmp(view->owner, "test") && view->desc.revision == 3);
        assert(!p4_game_enemies_model_source(enemies, RETAIL + i, &model_id) && model_id == sources[i]);
    }
    uint32_t model_id = UINT32_MAX;
    assert(!p4_game_enemies_model_source(enemies, 10, &model_id) && model_id == 10);
    assert(p4_game_enemies_model_source(enemies, RETAIL + 3u, &model_id) == P4_TABLE_NOT_FOUND);
    assert(!p4_game_enemies_find(enemies, 10) && !p4_game_enemies_find(enemies, RETAIL + 3u));
    assert(!p4_game_enemies_metadata(enemies) && !p4_game_enemies_names(enemies));
    return 0;
}
static void publish(void *user) { assert(!user && !allocations); ++fixture.published; }
static void rollback(void *user) { assert(!user); release_inputs(); }
static int materialize(P4GameEnemies *enemies)
{
    return p4_game_enemies_materialize(enemies, metadata, sizeof metadata, affinity, sizeof affinity,
        model, sizeof model, ai, sizeof ai, names, sizeof names);
}
static void check_guest(void)
{
    P4GameEnemies *enemies = fixture.enemies;
    const uint32_t bases[5] = {p4_game_enemies_metadata(enemies), p4_game_enemies_affinity(enemies),
        p4_game_enemies_model(enemies), p4_game_enemies_ai(enemies), p4_game_enemies_names(enemies)};
    const uint8_t *retail[5] = {metadata, affinity, model, ai, names};
    const size_t strides[5] = {P4_ENEMY_ROW_SIZE, P4_ENEMY_AFFINITY_SIZE, P4_ENEMY_MODEL_SIZE,
        P4_ENEMY_AI_SIZE, P4_ENEMY_NAME_SIZE};
    for (unsigned bank = 0; bank < 5; ++bank) {
        assert(bases[bank] && !(bases[bank] & 15u));
        assert(!memcmp(P4_G2H(bases[bank]), retail[bank], RETAIL * strides[bank]));
    }
    for (uint32_t i = 0; i < 3; ++i) {
        const P4GameEnemyDesc *row = &p4_game_enemies_find(enemies, RETAIL + i)->desc;
        const uint8_t *added[4] = {row->metadata, row->affinity, row->model, row->ai};
        for (unsigned bank = 0; bank < 4; ++bank)
            assert(!memcmp((const uint8_t *)P4_G2H(bases[bank]) + row->id * strides[bank], added[bank], strides[bank]));
        const char *name = (const char *)P4_G2H(bases[4]) + row->id * P4_ENEMY_NAME_SIZE;
        const size_t length = i == 1 ? 1u : P4_ENEMY_NAME_SIZE - 1u;
        assert(strlen(name) == length && name[0] == 'A' + (char)(row->id % 26u));
        for (size_t tail = length; tail < P4_ENEMY_NAME_SIZE; ++tail) assert(!name[tail]);
    }
}
int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    if (p4t_guest_init()) return 77;
    P4GameTableAllocator allocator = {NULL, allocate, release};
    /* Declarations before any source exists. */
    P4GameEnemies *cpu = p4_game_enemies_create(NULL);
    uint32_t model_id;
    assert(cpu && !p4_game_enemies_native_admit(cpu) && p4_game_enemies_limit(cpu) == RETAIL);
    assert(p4_game_enemies_validate_runtime_sources(cpu) == P4_TABLE_NOT_READY);
    assert(p4_game_enemies_runtime_use(cpu, 0) == P4_TABLE_NOT_READY);
    assert(p4_game_enemies_model_source(cpu, 0, &model_id) == P4_TABLE_NOT_READY);
    assert(p4_game_enemies_bind_runtime_use(cpu, NULL, NULL, NULL) == P4_TABLE_INVALID);
    assert(p4_game_enemies_bind_runtime_use(cpu, original_use, authored_use, NULL) == P4_TABLE_INVALID);
    P4GameEnemyDesc row = enemy(RETAIL, 5, "test:shadow"), invalid = row;
    invalid.id = RETAIL - 1u;
    assert(p4_game_enemies_register(cpu, "test", &invalid) == P4_TABLE_INVALID);
    invalid.id = P4_ENEMY_MAX_ID + 1u;
    assert(p4_game_enemies_register(cpu, "test", &invalid) == P4_TABLE_INVALID);
    invalid = row; invalid.model_source = RETAIL;
    assert(p4_game_enemies_register(cpu, "test", &invalid) == P4_TABLE_INVALID);
    invalid = row; invalid.schema = 0;
    assert(p4_game_enemies_register(cpu, "test", &invalid) == P4_TABLE_INVALID);
    invalid = row; invalid.revision = 0;
    assert(p4_game_enemies_register(cpu, "test", &invalid) == P4_TABLE_INVALID);
    assert(p4_game_enemies_register(cpu, "other", &row) == P4_TABLE_INVALID);
    assert(!p4_game_enemies_register(cpu, "test", &row));
    assert(p4_game_enemies_native_admit(cpu) == P4_TABLE_NOT_FOUND);
    invalid = row; invalid.id = RETAIL + 1u;
    assert(p4_game_enemies_register(cpu, "test", &invalid) == P4_TABLE_CONFLICT);
    invalid = row; strcpy(invalid.key, "test:other");
    assert(p4_game_enemies_register(cpu, "test", &invalid) == P4_TABLE_CONFLICT);
    assert(p4_game_enemies_count(cpu) == 1 && p4_game_enemies_limit(cpu) == RETAIL + 1u);
    p4_game_enemies_destroy(cpu);

    P4TextProfile raw = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    fixture.names = p4_game_tables_text_create(NULL, &raw);
    assert(fixture.names && !p4_game_tables_text_bind_runtime_use(fixture.names, text_use, &fixture));
    register_name(fixture.names, RETAIL, P4_ENEMY_NAME_SIZE);
    register_name(fixture.names, RETAIL + 1u, 2);
    register_name(fixture.names, RETAIL + 2u, P4_ENEMY_NAME_SIZE);
    assert(!p4_game_tables_text_seal(fixture.names, 1));
    fixture.enemies = p4_game_enemies_create(&allocator);
    fixture.empty = p4_game_enemies_create(&allocator);
    assert(fixture.enemies && fixture.empty);
    assert(!p4_game_enemies_bind_runtime_use(fixture.enemies, original_use, authored_use, &fixture));
    assert(!p4_game_enemies_bind_runtime_use(fixture.enemies, original_use, authored_use, &fixture));
    assert(p4_game_enemies_bind_runtime_use(fixture.enemies, original_use, NULL, NULL) == P4_TABLE_CONFLICT);
    assert(!p4_game_enemies_bind_runtime_use(fixture.empty, original_use, NULL, NULL));
    /* Registration order is not ID order; the seal sorts before density checks. */
    static const uint32_t order[3] = {2, 0, 1}, sources[3] = {5, 7, RETAIL - 1u};
    for (unsigned i = 0; i < 3; ++i) {
        char key[32];
        snprintf(key, sizeof key, "test:enemy-%u", (unsigned)order[i]);
        row = enemy(RETAIL + order[i], sources[order[i]], key);
        assert(!p4_game_enemies_register(fixture.enemies, "test", &row));
    }
    fixture.mode = argv[1];
    static const P4PackageHandler handler = {.name = "enemy-input-fixture", .prepare = prepare,
        .publish = publish, .rollback = rollback, .boot_phase = P4_PACKAGE_BOOT_TABLES};
    assert(!p4_package_handler_register(&handler));
    int result = p4_package_registry_publish();
    if (strcmp(fixture.mode, "valid")) {
        assert(result && !fixture.published && !allocations && !p4_game_enemies_semantic_revision(fixture.enemies));
    } else {
        assert(!result && fixture.published == 1 && !allocations);
        memset(metadata, 0x55, sizeof metadata); memset(affinity, 0x41, sizeof affinity);
        memset(model, 0x4d, sizeof model); memset(ai, 0xa1, sizeof ai); memset(names, 0x4e, sizeof names);
        /* Empty snapshots verify the loaded banks and allocate nothing. */
        assert(!materialize(fixture.empty) && !allocations && p4_game_enemies_revision(fixture.empty) == 8);
        assert(!p4_game_enemies_metadata(fixture.empty) && !p4_game_enemies_names(fixture.empty));
        assert(!p4_game_enemies_runtime_use(fixture.empty, 10) && original_uses == 1);
        assert(p4_game_enemies_runtime_use(fixture.empty, RETAIL) == P4_TABLE_NOT_FOUND);
        /* Loaded banks that differ from the sealed source refuse before allocating. */
        uint8_t *banks[5] = {metadata, affinity, model, ai, names};
        const size_t last[5] = {sizeof metadata - 1, sizeof affinity - 1, sizeof model - 1,
            sizeof ai - 1, sizeof names - 1};
        for (unsigned bank = 0; bank < 5; ++bank) {
            banks[bank][last[bank]] ^= 1;
            assert(materialize(fixture.enemies) == P4_TABLE_CONFLICT);
            banks[bank][last[bank]] ^= 1;
        }
        assert(p4_game_enemies_materialize(fixture.enemies, metadata, sizeof metadata - 1, affinity,
            sizeof affinity, model, sizeof model, ai, sizeof ai, names, sizeof names) == P4_TABLE_CONFLICT);
        assert(!allocations && !p4_game_enemies_revision(fixture.enemies));
        fail_allocate = 1;
        assert(materialize(fixture.enemies) == P4_TABLE_NOMEM);
        fail_allocate = 0;
        assert(allocations == 1 && !p4_game_enemies_revision(fixture.enemies) &&
            !p4_game_enemies_metadata(fixture.enemies));
        assert(!materialize(fixture.enemies) && allocations == 2);
        assert(p4_game_enemies_revision(fixture.enemies) == 8);
        check_guest();
        /* Repeated calls verify the same bytes without another allocation. */
        assert(!materialize(fixture.enemies) && allocations == 2);
        names[0] ^= 1;
        assert(materialize(fixture.enemies) == P4_TABLE_CONFLICT);
        names[0] ^= 1;
        check_guest();
        assert(!p4_game_enemies_runtime_use(fixture.enemies, 10) && original_uses == 2);
        assert(!p4_game_enemies_runtime_use(fixture.enemies, RETAIL + 2u) && authored_uses == 1);
        assert(p4_game_enemies_runtime_use(fixture.enemies, RETAIL + 3u) == P4_TABLE_NOT_FOUND);
        use_status = 1;
        assert(p4_game_enemies_runtime_use(fixture.enemies, RETAIL) == P4_TABLE_INVALID);
        assert(p4_game_enemies_runtime_use(fixture.enemies, 0) == P4_TABLE_INVALID);
        use_status = 0;
    }
    p4_game_enemies_destroy(fixture.empty);
    p4_game_enemies_destroy(fixture.enemies);
    p4_game_tables_text_destroy(fixture.names);
    p4_package_registry_retire();
    release_inputs();
    assert(releases == (strcmp(fixture.mode, "valid") ? 0u : 1u));
    puts("game_enemies: real CPU source seal/row proof and guest bank copy exercised; not native gameplay acceptance");
    return 0;
}
