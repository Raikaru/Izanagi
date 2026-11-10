/* Native persona bank owner. The CPU-sealed snapshot is staged before raw table
 * exposure; the shared packed observer records the actual PERSONA/MODEL/MSG/UNIT
 * loads, and the `persona` completion participant relocates the six ID-indexed
 * banks at 0x0010d480. Without staging every query answers the retail domain. */
#include "p4pc/game_personas.h"
#include "p4pc/game_personas_native.h"
#include "p4pc/game_tables_native.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/package_boot.h"
#include "p4pc/gmem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* SLUS_217.82 005f66c0 descriptor slots: PERSONA0, PERSONA1, MODEL2, PERSONA5, MSG3, UNIT2. */
enum { PERSONA_BANKS = 6, PERSONA_ALL = (1u << PERSONA_BANKS) - 1u };
static const uint32_t slots[PERSONA_BANKS] = {
    0x007644c4u, 0x007644ccu, 0x007644d0u, 0x007644e0u, 0x00764530u, 0x007644c8u
};
static const uint32_t strides[PERSONA_BANKS] = {
    P4_PERSONA_METADATA_SIZE, P4_PERSONA_GROWTH_SIZE, P4_PERSONA_MODEL_SIZE, P4_PERSONA_CANDIDATES_SIZE, P4_PERSONA_NAME_SIZE,
    P4_PERSONA_AFFINITY_SIZE
};
typedef struct PersonaStage {
    P4GamePersonas *personas;
    uint32_t prepared;
} PersonaStage;
static PersonaStage staged;
static P4GamePersonas *active;
static uint32_t original[PERSONA_BANKS];
/* One diagnostic line per added persona per session, never for retail IDs. */
static uint8_t presented[(P4_PERSONA_MAX_ID >> 3) + 1];
/* Compendium rows of added personas: the save block caches IDs 0..255 at
 * 0x007981CC (48-byte DatPersonaRecord rows, save tag 16); IDs 256..511 use
 * this port-owned guest extension, allocated by the first expanded completion
 * and reused by later sessions. Their persistence is the persona save block. */
#define COMPENDIUM_RETAIL 0x007981CCu
#define COMPENDIUM_STRIDE 0x30u
static uint32_t compendium;
static int prepare_complete(void *user);
static void commit_complete(void *user);
static _Noreturn void refuse(int result)
{
    fprintf(stderr, "p4pc: persona table publication refused (%d): %s\n", result, p4_game_tables_error(result));
    abort();
}
static uint32_t relocated(const P4GamePersonas *personas, unsigned bank)
{
    switch (bank) {
    case 0: return p4_game_personas_metadata(personas);
    case 1: return p4_game_personas_growth(personas);
    case 2: return p4_game_personas_model(personas);
    case 3: return p4_game_personas_candidates(personas);
    case 4: return p4_game_personas_names(personas);
    default: return p4_game_personas_affinity(personas);
    }
}
int p4_game_personas_boot_stage(P4GamePersonas *personas)
{
    if (!personas) return P4_TABLE_INVALID;
    if (!p4_game_personas_semantic_revision(personas)) return P4_TABLE_NOT_READY;
    if (active || p4_game_personas_revision(personas)) return P4_TABLE_FROZEN;
    int result = p4_game_personas_native_admit(personas);
    if (!result) result = p4_game_personas_validate_runtime_sources(personas);
    if (result) return result;
    if (staged.personas) return staged.personas == personas ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    /* No added rows: the retail banks stay in place and nothing participates. */
    if (!p4_game_personas_count(personas)) return P4_TABLE_OK;
    result = p4_game_tables_battle_complete_register("persona", prepare_complete, commit_complete, NULL);
    if (result) return result;
    staged = (PersonaStage){personas, 0};
    return P4_TABLE_OK;
}
int p4_game_personas_boot_unstage(P4GamePersonas *exact)
{
    if (!exact) return P4_TABLE_INVALID;
    if (active || staged.prepared || p4_game_personas_revision(exact)) return P4_TABLE_FROZEN;
    if (!staged.personas) return P4_TABLE_OK;
    if (staged.personas != exact) return P4_TABLE_CONFLICT;
    int result = p4_game_tables_battle_complete_unregister("persona", prepare_complete, commit_complete, NULL);
    if (result) return result;
    staged = (PersonaStage){0};
    return P4_TABLE_OK;
}
P4GamePersonas *p4_game_personas_active(void) { return active; }
int p4_game_personas_expanded(void) { return active != NULL; }
uint32_t p4_game_personas_native_limit(void) { return active ? p4_game_personas_limit(active) : P4_PERSONA_RETAIL_ROWS; }
uint32_t p4_game_personas_native_declared_limit(void)
{
    const P4GamePersonas *personas = active ? active : staged.personas;
    return personas ? p4_game_personas_limit(personas) : P4_PERSONA_RETAIL_ROWS;
}
uint32_t p4_game_personas_native_model(uint32_t id)
{
    if (!active || id < P4_PERSONA_RETAIL_ROWS) return id;
    uint32_t model = 0;
    int result = p4_game_personas_model_source(active, id, &model);
    if (!result) result = p4_game_personas_runtime_use(active, id);
    if (result) {
        char reason[96];
        snprintf(reason, sizeof reason, "Persona %u source authorization failed (%d)", (unsigned)id, result);
        p4_package_boot_runtime_failed(reason);
    }
    if (!(presented[id >> 3] & (1u << (id & 7u)))) {
        presented[id >> 3] |= (uint8_t)(1u << (id & 7u));
        fprintf(stderr, "p4pc: added persona %u presents retail model %u\n", (unsigned)id, (unsigned)model);
    }
    return model;
}
uint32_t p4_game_personas_native_compendium(uint32_t id)
{
    /* Retail address arithmetic is kept for every ID without expansion. */
    if (!active || id < P4_PERSONA_RETAIL_ROWS) return COMPENDIUM_RETAIL + id * COMPENDIUM_STRIDE;
    if (id >= p4_game_personas_limit(active)) {
        char reason[96];
        snprintf(reason, sizeof reason, "Persona %u has no compendium row", (unsigned)id);
        p4_package_boot_runtime_failed(reason);
    }
    return compendium + (id - P4_PERSONA_RETAIL_ROWS) * COMPENDIUM_STRIDE;
}
void p4_game_personas_native_compendium_clear(void)
{
    if (active)
        memset(P4_G2H(compendium), 0, (size_t)(p4_game_personas_limit(active) - P4_PERSONA_RETAIL_ROWS) * COMPENDIUM_STRIDE);
}
int p4_game_personas_native_fusion_candidate(uint32_t id)
{
    return id < P4_PERSONA_FUSION_RETAIL_END || (id >= P4_PERSONA_RETAIL_ROWS && id < p4_game_personas_native_limit());
}
uint32_t p4_game_personas_native_fusion_next(uint32_t id)
{
    id = (id + 1u) & 0xFFFFu;
    return id == P4_PERSONA_FUSION_RETAIL_END ? P4_PERSONA_RETAIL_ROWS : id;
}
int p4_game_personas_packed_pending(void) { return staged.personas != NULL; }
uint32_t p4_game_personas_packed_scan(const P4GameTablePackedBlock *blocks, unsigned count)
{
    uint32_t mask = 0;
    if (!staged.personas) return 0;
    for (unsigned block = 0; block < count; ++block)
        for (unsigned bank = 0; bank < PERSONA_BANKS; ++bank) {
            if (blocks[block].slot != slots[bank]) continue;
            /* The sealed source spans are exactly the retail rows. */
            if (blocks[block].size != P4_PERSONA_RETAIL_ROWS * strides[bank]) refuse(P4_TABLE_INVALID);
            mask |= 1u << bank;
        }
    return mask;
}
void p4_game_personas_packed_accept(uint32_t mask)
{
    if (mask && !staged.personas) refuse(P4_TABLE_NOT_READY);
    staged.prepared |= mask;
}
static int prepare_complete(void *user)
{
    (void)user;
    if (!staged.personas || staged.prepared != PERSONA_ALL) return P4_TABLE_NOT_READY;
    if (!compendium) {
        compendium = p4_gport_alloc((P4_PERSONA_MAX_ID + 1u - P4_PERSONA_RETAIL_ROWS) * COMPENDIUM_STRIDE, 16);
        if (!compendium) return P4_TABLE_NOMEM;
    }
    uint32_t bases[PERSONA_BANKS], rows[PERSONA_BANKS];
    for (unsigned bank = 0; bank < PERSONA_BANKS; ++bank)
        if (!p4_game_table_extent_rows(slots[bank], strides[bank], &bases[bank], &rows[bank])) return P4_TABLE_INVALID;
    /* Semantic/source/name checks were completed by the early CPU seal. Only
     * copied-byte verification, allocation and binding remain. */
    int result = p4_game_personas_materialize(staged.personas,
        P4_GPTR(const uint8_t, bases[0]), (size_t)rows[0] * strides[0],
        P4_GPTR(const uint8_t, bases[1]), (size_t)rows[1] * strides[1],
        P4_GPTR(const uint8_t, bases[2]), (size_t)rows[2] * strides[2],
        P4_GPTR(const uint8_t, bases[3]), (size_t)rows[3] * strides[3],
        P4_GPTR(const uint8_t, bases[4]), (size_t)rows[4] * strides[4],
        P4_GPTR(const uint8_t, bases[5]), (size_t)rows[5] * strides[5]);
    if (result) return result;
    for (unsigned bank = 0; bank < PERSONA_BANKS; ++bank) original[bank] = bases[bank];
    return P4_TABLE_OK;
}
static void commit_complete(void *user)
{
    (void)user;
    /* One non-yielding game-thread boundary: all six banks were verified and
     * copied before any slot becomes visible to native consumers. Each store
     * publishes its extent in the same step. */
    const uint32_t rows = p4_game_personas_limit(staged.personas);
    for (unsigned bank = 0; bank < PERSONA_BANKS; ++bank)
        p4_game_table_slot_publish(slots[bank], relocated(staged.personas, bank), rows * strides[bank]);
    active = staged.personas;
    p4_game_personas_native_compendium_clear();
}
int p4_game_personas_session_end(int image_live)
{
    if (active && image_live) {
        for (unsigned bank = 0; bank < PERSONA_BANKS; ++bank)
            if (P4_G32(slots[bank]) != relocated(active, bank)) return P4_TABLE_CONFLICT;
        for (unsigned bank = 0; bank < PERSONA_BANKS; ++bank)
            p4_game_table_slot_publish(slots[bank], original[bank], P4_PERSONA_RETAIL_ROWS * strides[bank]);
    }
    active = NULL;
    staged = (PersonaStage){0};
    for (unsigned bank = 0; bank < PERSONA_BANKS; ++bank) original[bank] = 0;
    memset(presented, 0, sizeof presented);
    return P4_TABLE_OK;
}
