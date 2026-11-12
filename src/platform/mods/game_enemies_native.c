/* Native enemy bank owner. The CPU-sealed snapshot is staged before raw table
 * exposure; the shared packed observer records the actual UNIT/MODEL/AICALC/MSG
 * loads, and the `enemy` completion participant relocates the five ID-indexed
 * banks at 0x0010d480. Without staging every query answers the retail domain. */
#include "p4pc/game_enemies.h"
#include "p4pc/game_enemies_native.h"
#include "p4pc/game_tables_native.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/package_boot.h"
#include "p4pc/gmem.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* SLUS_217.82 005f66c0 descriptor slots: UNIT0, UNIT1, MODEL1, AICALC8, MSG2. */
enum { ENEMY_BANKS = 5, ENEMY_ALL = (1u << ENEMY_BANKS) - 1u };
static const uint32_t slots[ENEMY_BANKS] = {
    0x007644b4u, 0x007644b8u, 0x007644bcu, 0x007644c0u, 0x00764534u
};
static const uint32_t strides[ENEMY_BANKS] = {
    P4_ENEMY_ROW_SIZE, P4_ENEMY_AFFINITY_SIZE, P4_ENEMY_MODEL_SIZE, P4_ENEMY_AI_SIZE, P4_ENEMY_NAME_SIZE
};
typedef struct EnemyStage {
    P4GameEnemies *enemies;
    uint32_t prepared;
} EnemyStage;
static EnemyStage staged;
static P4GameEnemies *active;
static uint32_t original[ENEMY_BANKS];
/* One diagnostic line per added enemy per session, never for retail IDs. */
static uint8_t presented[(P4_ENEMY_MAX_ID >> 3) + 1];
static int prepare_complete(void *user);
static void commit_complete(void *user);
static _Noreturn void refuse(int result)
{
    fprintf(stderr, "p4pc: enemy table publication refused (%d): %s\n", result, p4_game_tables_error(result));
    abort();
}
static uint32_t relocated(const P4GameEnemies *enemies, unsigned bank)
{
    switch (bank) {
    case 0: return p4_game_enemies_metadata(enemies);
    case 1: return p4_game_enemies_affinity(enemies);
    case 2: return p4_game_enemies_model(enemies);
    case 3: return p4_game_enemies_ai(enemies);
    default: return p4_game_enemies_names(enemies);
    }
}
int p4_game_enemies_boot_stage(P4GameEnemies *enemies)
{
    if (!enemies) return P4_TABLE_INVALID;
    if (!p4_game_enemies_semantic_revision(enemies)) return P4_TABLE_NOT_READY;
    if (active || p4_game_enemies_revision(enemies)) return P4_TABLE_FROZEN;
    int result = p4_game_enemies_native_admit(enemies);
    if (!result) result = p4_game_enemies_validate_runtime_sources(enemies);
    if (result) return result;
    if (staged.enemies) return staged.enemies == enemies ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    /* No added rows: the retail banks stay in place and nothing participates. */
    if (!p4_game_enemies_count(enemies)) return P4_TABLE_OK;
    result = p4_game_tables_battle_complete_register("enemy", prepare_complete, commit_complete, NULL);
    if (result) return result;
    staged = (EnemyStage){enemies, 0};
    return P4_TABLE_OK;
}
int p4_game_enemies_boot_unstage(P4GameEnemies *exact)
{
    if (!exact) return P4_TABLE_INVALID;
    if (active || staged.prepared || p4_game_enemies_revision(exact)) return P4_TABLE_FROZEN;
    if (!staged.enemies) return P4_TABLE_OK;
    if (staged.enemies != exact) return P4_TABLE_CONFLICT;
    int result = p4_game_tables_battle_complete_unregister("enemy", prepare_complete, commit_complete, NULL);
    if (result) return result;
    staged = (EnemyStage){0};
    return P4_TABLE_OK;
}
P4GameEnemies *p4_game_enemies_active(void) { return active; }
int p4_game_enemies_expanded(void) { return active != NULL; }
uint32_t p4_game_enemies_native_limit(void) { return active ? p4_game_enemies_limit(active) : P4_ENEMY_RETAIL_ROWS; }
uint32_t p4_game_enemies_native_declared_limit(void)
{
    const P4GameEnemies *enemies = active ? active : staged.enemies;
    return enemies ? p4_game_enemies_limit(enemies) : P4_ENEMY_RETAIL_ROWS;
}
uint32_t p4_game_enemies_native_model(uint32_t id)
{
    if (!active || id < P4_ENEMY_RETAIL_ROWS) return id;
    uint32_t model = 0;
    int result = p4_game_enemies_model_source(active, id, &model);
    if (!result) result = p4_game_enemies_runtime_use(active, id);
    if (result) {
        char reason[96];
        snprintf(reason, sizeof reason, "Enemy %u source authorization failed (%d)", (unsigned)id, result);
        p4_package_boot_runtime_failed(reason);
    }
    if (!(presented[id >> 3] & (1u << (id & 7u)))) {
        presented[id >> 3] |= (uint8_t)(1u << (id & 7u));
        fprintf(stderr, "p4pc: added enemy %u presents retail model %u\n", (unsigned)id, (unsigned)model);
    }
    return model;
}
int p4_game_enemies_packed_pending(void) { return staged.enemies != NULL; }
uint32_t p4_game_enemies_packed_scan(const P4GameTablePackedBlock *blocks, unsigned count)
{
    uint32_t mask = 0;
    if (!staged.enemies) return 0;
    for (unsigned block = 0; block < count; ++block)
        for (unsigned bank = 0; bank < ENEMY_BANKS; ++bank) {
            if (blocks[block].slot != slots[bank]) continue;
            /* The sealed source spans are exactly the retail rows. */
            if (blocks[block].size != P4_ENEMY_RETAIL_ROWS * strides[bank]) refuse(P4_TABLE_INVALID);
            mask |= 1u << bank;
        }
    return mask;
}
void p4_game_enemies_packed_accept(uint32_t mask)
{
    if (mask && !staged.enemies) refuse(P4_TABLE_NOT_READY);
    staged.prepared |= mask;
}
static int prepare_complete(void *user)
{
    (void)user;
    if (!staged.enemies || staged.prepared != ENEMY_ALL) return P4_TABLE_NOT_READY;
    uint32_t bases[ENEMY_BANKS], rows[ENEMY_BANKS];
    for (unsigned bank = 0; bank < ENEMY_BANKS; ++bank)
        if (!p4_game_table_extent_rows(slots[bank], strides[bank], &bases[bank], &rows[bank])) return P4_TABLE_INVALID;
    /* Semantic/source/name checks were completed by the early CPU seal. Only
     * copied-byte verification, allocation and binding remain. */
    int result = p4_game_enemies_materialize(staged.enemies,
        P4_GPTR(const uint8_t, bases[0]), (size_t)rows[0] * strides[0],
        P4_GPTR(const uint8_t, bases[1]), (size_t)rows[1] * strides[1],
        P4_GPTR(const uint8_t, bases[2]), (size_t)rows[2] * strides[2],
        P4_GPTR(const uint8_t, bases[3]), (size_t)rows[3] * strides[3],
        P4_GPTR(const uint8_t, bases[4]), (size_t)rows[4] * strides[4]);
    if (result) return result;
    for (unsigned bank = 0; bank < ENEMY_BANKS; ++bank) original[bank] = bases[bank];
    return P4_TABLE_OK;
}
static void commit_complete(void *user)
{
    (void)user;
    /* One non-yielding game-thread boundary: all five banks were verified and
     * copied before any slot becomes visible to native consumers. Each store
     * publishes its extent in the same step. */
    const uint32_t rows = p4_game_enemies_limit(staged.enemies);
    for (unsigned bank = 0; bank < ENEMY_BANKS; ++bank)
        p4_game_table_slot_publish(slots[bank], relocated(staged.enemies, bank), rows * strides[bank]);
    active = staged.enemies;
}
int p4_game_enemies_session_end(int image_live)
{
    if (active && image_live) {
        for (unsigned bank = 0; bank < ENEMY_BANKS; ++bank)
            if (P4_G32(slots[bank]) != relocated(active, bank)) return P4_TABLE_CONFLICT;
        for (unsigned bank = 0; bank < ENEMY_BANKS; ++bank)
            p4_game_table_slot_publish(slots[bank], original[bank], P4_ENEMY_RETAIL_ROWS * strides[bank]);
    }
    active = NULL;
    staged = (EnemyStage){0};
    for (unsigned bank = 0; bank < ENEMY_BANKS; ++bank) original[bank] = 0;
    memset(presented, 0, sizeof presented);
    return P4_TABLE_OK;
}
