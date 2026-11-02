/* Real Core/VFS original inputs, CPU seal and guest materialization of the five
 * persona banks. Synthetic banks do not prove native battle/model/AI behavior. */
#include "p4pc/game_personas.h"
#include "p4pc/game_table_inputs.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define RETAIL P4_PERSONA_RETAIL_ROWS
static unsigned allocations, releases, original_uses, authored_uses;
static int fail_allocate, use_status;
static struct Fixture {
    P4GamePersonas *personas, *empty;
    P4GameText *names;
    P4GameTableInput *persona, *model, *message;
    const char *mode;
    unsigned published;
} fixture;
static uint8_t metadata[RETAIL * P4_PERSONA_METADATA_SIZE], growth[RETAIL * P4_PERSONA_GROWTH_SIZE];
static uint8_t model[RETAIL * P4_PERSONA_MODEL_SIZE], candidates[RETAIL * P4_PERSONA_CANDIDATES_SIZE];
static uint8_t names[RETAIL * P4_PERSONA_NAME_SIZE];
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
    assert(span == p4_game_table_input_span(fixture.persona, 0) && context == fixture.persona);
    ++original_uses;
    return use_status;
}
static int authored_use(const P4GamePersonas *view, uint32_t id, void *context)
{
    assert(view == fixture.personas && context == &fixture && id >= RETAIL);
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
    assert(p4_game_tables_text_register(text, "test", P4_TEXT_PERSONA, id, P4_TEXT_NAME, 1,
        (const uint8_t *)bytes, size) == P4_TABLE_OK);
}
static P4GamePersonaDesc persona(uint32_t id, uint32_t model_source, const char *key)
{
    P4GamePersonaDesc row = {.id = id, .model_source = model_source, .schema = 1, .revision = 3};
    const uint8_t seed = (uint8_t)(id - RETAIL);
    strcpy(row.key, key);
    memset(row.metadata, seed + 1, sizeof row.metadata);
    memset(row.growth, seed + 11, sizeof row.growth);
    memset(row.model, seed + 21, sizeof row.model);
    memset(row.candidates, seed + 31, sizeof row.candidates);
    return row;
}
static void release_inputs(void)
{
    p4_game_table_input_destroy(fixture.persona);
    p4_game_table_input_destroy(fixture.model);
    p4_game_table_input_destroy(fixture.message);
    fixture.persona = fixture.model = fixture.message = NULL;
}
static int prepare(void *user)
{
    assert(!user && !allocations);
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_PERSONA, 1048576, &fixture.persona, NULL));
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_MODEL, 1048576, &fixture.model, NULL));
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_MSG, 1048576, &fixture.message, NULL));
    assert(!p4_game_table_input_seal(fixture.persona) && !p4_game_table_input_seal(fixture.model));
    assert(!p4_game_table_input_seal(fixture.message));
    const P4GameTableInputSpan *persona0 = p4_game_table_input_span(fixture.persona, 0);
    const P4GameTableInputSpan *persona1 = p4_game_table_input_span(fixture.persona, 1);
    const P4GameTableInputSpan *model2 = p4_game_table_input_span(fixture.model, 2);
    const P4GameTableInputSpan *persona5 = p4_game_table_input_span(fixture.persona, 5);
    const P4GameTableInputSpan *msg3 = p4_game_table_input_span(fixture.message, 3);
    P4GamePersonas *personas = fixture.personas;
    if (strcmp(fixture.mode, "valid")) {
        assert(p4_game_personas_seal(personas, persona0, persona1, model2, persona5, msg3, fixture.names, 8) == P4_TABLE_INVALID);
        assert(p4_game_personas_seal(fixture.empty, persona0, persona1, model2, persona5, msg3, NULL, 8) == P4_TABLE_INVALID);
        assert(!allocations && !p4_game_personas_semantic_revision(personas));
        return -1; /* The real Core transaction must roll back, never publish. */
    }
    /* Every bank is its own retail pointer slot, never a neighbouring block. */
    assert(p4_game_personas_seal(personas, persona1, persona0, model2, persona5, msg3, fixture.names, 8) == P4_TABLE_INVALID);
    assert(p4_game_personas_seal(personas, persona0, persona1, p4_game_table_input_span(fixture.model, 0), persona5, msg3,
        fixture.names, 8) == P4_TABLE_INVALID);
    assert(p4_game_personas_seal(personas, persona0, persona1, model2, p4_game_table_input_span(fixture.persona, 4), msg3,
        fixture.names, 8) == P4_TABLE_INVALID);
    assert(p4_game_personas_seal(personas, persona0, persona1, model2, persona5, p4_game_table_input_span(fixture.message, 1),
        fixture.names, 8) == P4_TABLE_INVALID);
    assert(p4_game_personas_seal(personas, persona0, persona1, model2, persona5, NULL, fixture.names, 8) == P4_TABLE_INVALID);
    /* PERSONA0, PERSONA1 and PERSONA5 are one sealed PERSONA.TBL, not two loads. */
    P4GameTableInput *other_unit = NULL;
    assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_PERSONA, 1048576, &other_unit, NULL));
    assert(!p4_game_table_input_seal(other_unit));
    assert(p4_game_personas_seal(personas, persona0, p4_game_table_input_span(other_unit, 1), model2, persona5, msg3,
        fixture.names, 8) == P4_TABLE_INVALID);
    p4_game_table_input_destroy(other_unit);
    P4TextProfile message = {P4_TEXT_ENCODING_US_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4TextProfile raw = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4GameText *text = p4_game_tables_text_create(NULL, &message);
    assert(text && !p4_game_tables_text_seal(text, 1));
    assert(p4_game_personas_seal(personas, persona0, persona1, model2, persona5, msg3, text, 8) == P4_TABLE_INVALID);
    p4_game_tables_text_destroy(text);
    text = p4_game_tables_text_create(NULL, &raw);
    assert(text);
    assert(p4_game_personas_seal(personas, persona0, persona1, model2, persona5, msg3, text, 8) == P4_TABLE_NOT_READY);
    assert(p4_game_personas_seal(personas, persona0, persona1, model2, persona5, msg3, NULL, 8) == P4_TABLE_NOT_READY);
    p4_game_tables_text_destroy(text);
    /* The MSG3 stride is 17 bytes including NUL. */
    text = p4_game_tables_text_create(NULL, &raw);
    register_name(text, RETAIL, P4_PERSONA_NAME_SIZE + 1u);
    assert(text && !p4_game_tables_text_seal(text, 1));
    P4GamePersonas *other = p4_game_personas_create(NULL);
    P4GamePersonaDesc row = persona(RETAIL, 1, "test:long-name");
    assert(other && !p4_game_personas_register(other, "test", &row));
    assert(p4_game_personas_seal(other, persona0, persona1, model2, persona5, msg3, text, 8) == P4_TABLE_INVALID);
    p4_game_personas_destroy(other);
    p4_game_tables_text_destroy(text);
    /* Dense IDs: no zero row is ever invented between retail and added rows. */
    other = p4_game_personas_create(NULL);
    row = persona(RETAIL + 1u, 1, "test:sparse");
    assert(other && !p4_game_personas_register(other, "test", &row));
    assert(p4_game_personas_seal(other, persona0, persona1, model2, persona5, msg3, fixture.names, 8) == P4_TABLE_INVALID);
    p4_game_personas_destroy(other);
    other = p4_game_personas_create(NULL);
    for (uint32_t id = RETAIL; id < RETAIL + 4u; ++id) {
        char key[32];
        snprintf(key, sizeof key, "test:unnamed-%u", (unsigned)id);
        row = persona(id, 1, key);
        assert(other && !p4_game_personas_register(other, "test", &row));
    }
    assert(p4_game_personas_seal(other, persona0, persona1, model2, persona5, msg3, fixture.names, 8) == P4_TABLE_NOT_FOUND);
    assert(!p4_game_personas_semantic_revision(other) && !p4_game_personas_find(other, RETAIL));
    p4_game_personas_destroy(other);
    /* An empty snapshot needs no names and keeps the retail domain. */
    assert(!p4_game_personas_seal(fixture.empty, persona0, persona1, model2, persona5, msg3, NULL, 8));
    assert(!p4_game_personas_validate_runtime_sources(fixture.empty));
    assert(!p4_game_personas_seal(personas, persona0, persona1, model2, persona5, msg3, fixture.names, 8));
    assert(!p4_game_personas_seal(personas, persona0, persona1, model2, persona5, msg3, fixture.names, 8));
    assert(p4_game_personas_seal(personas, persona0, persona1, model2, persona5, msg3, fixture.names, 9) == P4_TABLE_CONFLICT);
    assert(p4_game_personas_seal(personas, persona0, persona1, model2, persona5, msg3, NULL, 8) == P4_TABLE_CONFLICT);
    row = persona(RETAIL + 3u, 1, "test:late");
    assert(p4_game_personas_register(personas, "test", &row) == P4_TABLE_FROZEN);
    assert(p4_game_personas_bind_runtime_use(personas, original_use, authored_use, &fixture) == P4_TABLE_FROZEN);
    assert(!allocations && p4_game_personas_semantic_revision(personas) == 8 && !p4_game_personas_revision(personas));
    assert(!p4_game_personas_validate_runtime_sources(personas) && !p4_game_personas_native_admit(personas));
    assert(p4_game_personas_runtime_use(personas, RETAIL) == P4_TABLE_NOT_READY);
    assert(p4_game_personas_count(personas) == 3 && p4_game_personas_limit(personas) == RETAIL + 3u);
    static const uint32_t sources[3] = {5, 7, RETAIL - 1u};
    for (uint32_t i = 0; i < 3; ++i) {
        const P4GamePersonaView *view = p4_game_personas_at(personas, i);
        uint32_t model_id = UINT32_MAX;
        assert(view && view == p4_game_personas_find(personas, RETAIL + i));
        assert(view->desc.id == RETAIL + i && !strcmp(view->owner, "test") && view->desc.revision == 3);
        assert(!p4_game_personas_model_source(personas, RETAIL + i, &model_id) && model_id == sources[i]);
    }
    uint32_t model_id = UINT32_MAX;
    assert(!p4_game_personas_model_source(personas, 10, &model_id) && model_id == 10);
    assert(p4_game_personas_model_source(personas, RETAIL + 3u, &model_id) == P4_TABLE_NOT_FOUND);
    assert(!p4_game_personas_find(personas, 10) && !p4_game_personas_find(personas, RETAIL + 3u));
    assert(!p4_game_personas_metadata(personas) && !p4_game_personas_names(personas));
    return 0;
}
static void publish(void *user) { assert(!user && !allocations); ++fixture.published; }
static void rollback(void *user) { assert(!user); release_inputs(); }
static int materialize(P4GamePersonas *personas)
{
    return p4_game_personas_materialize(personas, metadata, sizeof metadata, growth, sizeof growth,
        model, sizeof model, candidates, sizeof candidates, names, sizeof names);
}
static void check_guest(void)
{
    P4GamePersonas *personas = fixture.personas;
    const uint32_t bases[5] = {p4_game_personas_metadata(personas), p4_game_personas_growth(personas),
        p4_game_personas_model(personas), p4_game_personas_candidates(personas), p4_game_personas_names(personas)};
    const uint8_t *retail[5] = {metadata, growth, model, candidates, names};
    const size_t strides[5] = {P4_PERSONA_METADATA_SIZE, P4_PERSONA_GROWTH_SIZE, P4_PERSONA_MODEL_SIZE,
        P4_PERSONA_CANDIDATES_SIZE, P4_PERSONA_NAME_SIZE};
    for (unsigned bank = 0; bank < 5; ++bank) {
        assert(bases[bank] && !(bases[bank] & 15u));
        assert(!memcmp(P4_G2H(bases[bank]), retail[bank], RETAIL * strides[bank]));
    }
    for (uint32_t i = 0; i < 3; ++i) {
        const P4GamePersonaDesc *row = &p4_game_personas_find(personas, RETAIL + i)->desc;
        const uint8_t *added[4] = {row->metadata, row->growth, row->model, row->candidates};
        for (unsigned bank = 0; bank < 4; ++bank)
            assert(!memcmp((const uint8_t *)P4_G2H(bases[bank]) + row->id * strides[bank], added[bank], strides[bank]));
        const char *name = (const char *)P4_G2H(bases[4]) + row->id * P4_PERSONA_NAME_SIZE;
        const size_t length = i == 1 ? 1u : P4_PERSONA_NAME_SIZE - 1u;
        assert(strlen(name) == length && name[0] == 'A' + (char)(row->id % 26u));
        for (size_t tail = length; tail < P4_PERSONA_NAME_SIZE; ++tail) assert(!name[tail]);
    }
}
int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    if (p4t_guest_init()) return 77;
    P4GameTableAllocator allocator = {NULL, allocate, release};
    /* Declarations before any source exists. */
    P4GamePersonas *cpu = p4_game_personas_create(NULL);
    uint32_t model_id;
    assert(cpu && !p4_game_personas_native_admit(cpu) && p4_game_personas_limit(cpu) == RETAIL);
    assert(p4_game_personas_validate_runtime_sources(cpu) == P4_TABLE_NOT_READY);
    assert(p4_game_personas_runtime_use(cpu, 0) == P4_TABLE_NOT_READY);
    assert(p4_game_personas_model_source(cpu, 0, &model_id) == P4_TABLE_NOT_READY);
    assert(p4_game_personas_bind_runtime_use(cpu, NULL, NULL, NULL) == P4_TABLE_INVALID);
    assert(p4_game_personas_bind_runtime_use(cpu, original_use, authored_use, NULL) == P4_TABLE_INVALID);
    P4GamePersonaDesc row = persona(RETAIL, 5, "test:shadow"), invalid = row;
    invalid.id = RETAIL - 1u;
    assert(p4_game_personas_register(cpu, "test", &invalid) == P4_TABLE_INVALID);
    invalid.id = P4_PERSONA_MAX_ID + 1u;
    assert(p4_game_personas_register(cpu, "test", &invalid) == P4_TABLE_INVALID);
    invalid = row; invalid.model_source = RETAIL;
    assert(p4_game_personas_register(cpu, "test", &invalid) == P4_TABLE_INVALID);
    invalid = row; invalid.schema = 0;
    assert(p4_game_personas_register(cpu, "test", &invalid) == P4_TABLE_INVALID);
    invalid = row; invalid.revision = 0;
    assert(p4_game_personas_register(cpu, "test", &invalid) == P4_TABLE_INVALID);
    assert(p4_game_personas_register(cpu, "other", &row) == P4_TABLE_INVALID);
    assert(!p4_game_personas_register(cpu, "test", &row));
    assert(p4_game_personas_native_admit(cpu) == P4_TABLE_NOT_FOUND);
    invalid = row; invalid.id = RETAIL + 1u;
    assert(p4_game_personas_register(cpu, "test", &invalid) == P4_TABLE_CONFLICT);
    invalid = row; strcpy(invalid.key, "test:other");
    assert(p4_game_personas_register(cpu, "test", &invalid) == P4_TABLE_CONFLICT);
    assert(p4_game_personas_count(cpu) == 1 && p4_game_personas_limit(cpu) == RETAIL + 1u);
    p4_game_personas_destroy(cpu);

    P4TextProfile raw = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    fixture.names = p4_game_tables_text_create(NULL, &raw);
    assert(fixture.names && !p4_game_tables_text_bind_runtime_use(fixture.names, text_use, &fixture));
    register_name(fixture.names, RETAIL, P4_PERSONA_NAME_SIZE);
    register_name(fixture.names, RETAIL + 1u, 2);
    register_name(fixture.names, RETAIL + 2u, P4_PERSONA_NAME_SIZE);
    assert(!p4_game_tables_text_seal(fixture.names, 1));
    fixture.personas = p4_game_personas_create(&allocator);
    fixture.empty = p4_game_personas_create(&allocator);
    assert(fixture.personas && fixture.empty);
    assert(!p4_game_personas_bind_runtime_use(fixture.personas, original_use, authored_use, &fixture));
    assert(!p4_game_personas_bind_runtime_use(fixture.personas, original_use, authored_use, &fixture));
    assert(p4_game_personas_bind_runtime_use(fixture.personas, original_use, NULL, NULL) == P4_TABLE_CONFLICT);
    assert(!p4_game_personas_bind_runtime_use(fixture.empty, original_use, NULL, NULL));
    /* Registration order is not ID order; the seal sorts before density checks. */
    static const uint32_t order[3] = {2, 0, 1}, sources[3] = {5, 7, RETAIL - 1u};
    for (unsigned i = 0; i < 3; ++i) {
        char key[32];
        snprintf(key, sizeof key, "test:persona-%u", (unsigned)order[i]);
        row = persona(RETAIL + order[i], sources[order[i]], key);
        assert(!p4_game_personas_register(fixture.personas, "test", &row));
    }
    fixture.mode = argv[1];
    static const P4PackageHandler handler = {.name = "persona-input-fixture", .prepare = prepare,
        .publish = publish, .rollback = rollback, .boot_phase = P4_PACKAGE_BOOT_TABLES};
    assert(!p4_package_handler_register(&handler));
    int result = p4_package_registry_publish();
    if (strcmp(fixture.mode, "valid")) {
        assert(result && !fixture.published && !allocations && !p4_game_personas_semantic_revision(fixture.personas));
    } else {
        assert(!result && fixture.published == 1 && !allocations);
        memset(metadata, 0x55, sizeof metadata); memset(growth, 0x41, sizeof growth);
        memset(model, 0x4d, sizeof model); memset(candidates, 0xa1, sizeof candidates); memset(names, 0x4e, sizeof names);
        /* Empty snapshots verify the loaded banks and allocate nothing. */
        assert(!materialize(fixture.empty) && !allocations && p4_game_personas_revision(fixture.empty) == 8);
        assert(!p4_game_personas_metadata(fixture.empty) && !p4_game_personas_names(fixture.empty));
        assert(!p4_game_personas_runtime_use(fixture.empty, 10) && original_uses == 1);
        assert(p4_game_personas_runtime_use(fixture.empty, RETAIL) == P4_TABLE_NOT_FOUND);
        /* Loaded banks that differ from the sealed source refuse before allocating. */
        uint8_t *banks[5] = {metadata, growth, model, candidates, names};
        const size_t last[5] = {sizeof metadata - 1, sizeof growth - 1, sizeof model - 1,
            sizeof candidates - 1, sizeof names - 1};
        for (unsigned bank = 0; bank < 5; ++bank) {
            banks[bank][last[bank]] ^= 1;
            assert(materialize(fixture.personas) == P4_TABLE_CONFLICT);
            banks[bank][last[bank]] ^= 1;
        }
        assert(p4_game_personas_materialize(fixture.personas, metadata, sizeof metadata - 1, growth,
            sizeof growth, model, sizeof model, candidates, sizeof candidates, names, sizeof names) == P4_TABLE_CONFLICT);
        assert(!allocations && !p4_game_personas_revision(fixture.personas));
        fail_allocate = 1;
        assert(materialize(fixture.personas) == P4_TABLE_NOMEM);
        fail_allocate = 0;
        assert(allocations == 1 && !p4_game_personas_revision(fixture.personas) &&
            !p4_game_personas_metadata(fixture.personas));
        assert(!materialize(fixture.personas) && allocations == 2);
        assert(p4_game_personas_revision(fixture.personas) == 8);
        check_guest();
        /* Repeated calls verify the same bytes without another allocation. */
        assert(!materialize(fixture.personas) && allocations == 2);
        names[0] ^= 1;
        assert(materialize(fixture.personas) == P4_TABLE_CONFLICT);
        names[0] ^= 1;
        check_guest();
        assert(!p4_game_personas_runtime_use(fixture.personas, 10) && original_uses == 2);
        assert(!p4_game_personas_runtime_use(fixture.personas, RETAIL + 2u) && authored_uses == 1);
        assert(p4_game_personas_runtime_use(fixture.personas, RETAIL + 3u) == P4_TABLE_NOT_FOUND);
        use_status = 1;
        assert(p4_game_personas_runtime_use(fixture.personas, RETAIL) == P4_TABLE_INVALID);
        assert(p4_game_personas_runtime_use(fixture.personas, 0) == P4_TABLE_INVALID);
        use_status = 0;
    }
    p4_game_personas_destroy(fixture.empty);
    p4_game_personas_destroy(fixture.personas);
    p4_game_tables_text_destroy(fixture.names);
    p4_package_registry_retire();
    release_inputs();
    assert(releases == (strcmp(fixture.mode, "valid") ? 0u : 1u));
    puts("game_personas: real CPU source seal/row proof and guest bank copy exercised; not native gameplay acceptance");
    return 0;
}
