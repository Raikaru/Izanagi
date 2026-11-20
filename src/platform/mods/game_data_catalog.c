/* p4.data runtime catalog (include/p4pc/game_data_catalog.h). */
#include "p4pc/game_data_catalog.h"
#include "game_items_internal.h"
#include "game_table_derived_internal.h"
#include "p4pc/game_data_edit.h"
#include "p4pc/game_enemies.h"
#include "p4pc/game_enemies_native.h"
#include "p4pc/game_items.h"
#include "p4pc/game_personas.h"
#include "p4pc/game_personas_native.h"
#include "p4pc/game_skills.h"
#include "p4pc/game_skills_native.h"
#include "p4pc/game_encounters.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/gmem.h"
#include "p4pc/content_digest.h"
#include "p4pc/vfs_source.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Bank pointer slots (config/game_tables.json) and their strides. */
enum {
    SLOT_SKILL_ELEMENT = 0x007644A4, SLOT_SKILL = 0x007644A8, SLOT_SKILL_NAME = 0x00764538,
    SLOT_ENEMY = 0x007644B4, SLOT_ENEMY_AFFINITY = 0x007644B8, SLOT_ENEMY_NAME = 0x00764534,
    SLOT_PERSONA = 0x007644C4, SLOT_PERSONA_GROWTH = 0x007644CC, SLOT_PERSONA_PARTY = 0x007644D4,
    SLOT_PERSONA_AFFINITY = 0x007644C8, SLOT_PERSONA_NAME = 0x00764530,
    SLOT_ITEM = 0x007642A0,
    SLOT_FORMATION = 0x00764504, SLOT_FLOOR = 0x0076450C, SLOT_ENCOUNTERS = 0x00764508, SLOT_TREASURE = 0x00764514,
    /* datGetItemNameId 0x001067F0: u32[1536] ELF string pointers. */
    ITEM_NAMES = 0x005E2200,
    /* FldPanel2dSelectEntry 0x0018A010's floor names, 27 bytes by step index. */
    FLOOR_NAMES = 0x005F4090
};
enum { ARCANA_NAME = 21, ARCANA_ROWS = 32, CATALOG_NAME_MAX = 32, MSG_MAX = 1u << 20 };
/* Retail ENCOUNT rows: selectors (floors), formations, encounter and treasure groups. */
enum { RETAIL_FLOORS = 160, RETAIL_FORMATIONS = 912, RETAIL_ENCOUNTERS = 56, RETAIL_TREASURES = 75 };

static struct Disc {
    int state; /* 0 not captured, 1 captured, -1 unavailable */
    char reason[160];
    P4DataKeys *keys[P4_DATA_KIND_COUNT];
    char arcana[ARCANA_ROWS][ARCANA_NAME + 1];
} disc;

/* Independent of the effective table input: only the selected disc layer,
 * captured before VFS mount and before any package can replace UNIT.TBL. */
static struct OriginalEnemies {
    int state;
    char reason[160];
    char source[128];
    char sha256[P4_CONTENT_DIGEST_HEX + 1];
    uint8_t levels[P4_ENEMY_RETAIL_ROWS];
} original_enemies;

static void capture_original_enemies(void)
{
    if (original_enemies.state) return;
    original_enemies.state = -1;
    snprintf(original_enemies.reason, sizeof original_enemies.reason, "the selected disc's battle/UNIT.TBL is unavailable");
    P4VfsSourcePlan *plan = NULL;
    if (vfs_source_prepare_selected() || vfs_source_plan_acquire(P4_VFS_DISC, "battle/UNIT.TBL", &plan) || !plan)
        return;
    const P4VfsSourceInfo *info = vfs_source_plan_info(plan);
    const uint64_t size = info ? info->size : 0;
    uint8_t *bytes = size && size <= (1u << 20) ? malloc((size_t)size) : NULL;
    if (!bytes || vfs_source_plan_read_at(plan, 0, bytes, (size_t)size)) {
        free(bytes);
        vfs_source_plan_release(plan);
        return;
    }
    size_t offset = 0, first_size = 0;
    const uint8_t *first = NULL;
    for (unsigned block = 0; block < 3; ++block) {
        if (offset > size || size - offset < 4u) goto invalid;
        uint32_t length = (uint32_t)bytes[offset] | (uint32_t)bytes[offset + 1] << 8 |
            (uint32_t)bytes[offset + 2] << 16 | (uint32_t)bytes[offset + 3] << 24;
        if (length > size - offset - 4u) goto invalid;
        if (!block) { first = bytes + offset + 4u; first_size = length; }
        uint64_t next = ((uint64_t)offset + 4u + length + 15u) & ~UINT64_C(15);
        if (next > size) goto invalid;
        offset = (size_t)next;
    }
    if (offset != size || first_size < P4_ENEMY_RETAIL_ROWS * P4_ENEMY_ROW_SIZE ||
        first_size % P4_ENEMY_ROW_SIZE || !info->key ||
        strlen(info->key) >= sizeof original_enemies.source) goto invalid;
    for (size_t i = 0; i < P4_ENEMY_RETAIL_ROWS; ++i)
        original_enemies.levels[i] = first[i * P4_ENEMY_ROW_SIZE + 3u];
    snprintf(original_enemies.source, sizeof original_enemies.source, "%s", info->key);
    p4_content_digest(bytes, (size_t)size, original_enemies.sha256);
    original_enemies.state = 1;
    free(bytes);
    vfs_source_plan_release(plan);
    return;
invalid:
    snprintf(original_enemies.reason, sizeof original_enemies.reason, "the selected disc's battle/UNIT.TBL has invalid packed enemy rows");
    free(bytes);
    vfs_source_plan_release(plan);
}

int p4_data_original_enemy_level(uint32_t id, uint8_t *level, const char **source,
    const char **sha256, const char **reason)
{
    if (reason) *reason = original_enemies.state < 0 ? original_enemies.reason :
        "the original enemy data was not captured at package boot";
    if (original_enemies.state != 1 || id >= P4_ENEMY_RETAIL_ROWS || !level) return -1;
    *level = original_enemies.levels[id];
    if (source) *source = original_enemies.source;
    if (sha256) *sha256 = original_enemies.sha256;
    if (reason) *reason = NULL;
    return 0;
}

static uint32_t u32le(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static int guest_range(uint32_t address, uint32_t size)
{
    return address >= 0x00100000u && address < 0x02000000u && size <= 0x02000000u - address;
}
/* A NUL-terminated guest string of at most capacity-1 bytes, or NULL. */
static const char *guest_string(uint32_t address, char *out, size_t capacity)
{
    if (!guest_range(address, 1)) return NULL;
    size_t length = 0;
    const char *text = P4_G2H(address);
    while (length + 1 < capacity && guest_range(address + (uint32_t)length, 1) && text[length]) {
        out[length] = text[length];
        ++length;
    }
    out[length] = 0;
    return out;
}
static void fixed_name(const uint8_t *field, size_t width, char *out, size_t capacity)
{
    size_t length = 0;
    while (length < width && length + 1 < capacity && field[length]) { out[length] = (char)field[length]; ++length; }
    out[length] = 0;
}
static int unavailable(const char *reason)
{
    disc.state = -1;
    snprintf(disc.reason, sizeof disc.reason, "%s", reason);
    return -1;
}

/* One fixed-width name bank of the disc's MSG.TBL as owned strings. */
static P4DataKeys *bank_keys(P4DataKind kind, const uint8_t *bank, size_t bytes, size_t width, size_t rows)
{
    if (bytes / width < rows) rows = bytes / width;
    char **names = calloc(rows ? rows : 1, sizeof *names);
    char *storage = malloc(rows * (width + 1) + 1);
    P4DataKeys *keys = NULL;
    if (names && storage) {
        for (size_t i = 0; i < rows; ++i) {
            names[i] = storage + i * (width + 1);
            fixed_name(bank + i * width, width, names[i], width + 1);
        }
        keys = p4_data_keys_create(kind, (const char *const *)names, rows);
    }
    free(storage);
    free(names);
    return keys;
}
static P4DataKeys *item_keys(void)
{
    char **names = calloc(P4_ITEM_RETAIL_COUNT, sizeof *names);
    char *storage = malloc(P4_ITEM_RETAIL_COUNT * CATALOG_NAME_MAX);
    P4DataKeys *keys = NULL;
    if (names && storage) {
        for (uint32_t id = 0; id < P4_ITEM_RETAIL_COUNT; ++id) {
            names[id] = storage + id * CATALOG_NAME_MAX;
            if (!guest_string(P4_G32(ITEM_NAMES + id * 4u), names[id], CATALOG_NAME_MAX)) names[id][0] = 0;
        }
        keys = p4_data_keys_create(P4_DATA_ITEM, (const char *const *)names, P4_ITEM_RETAIL_COUNT);
    }
    free(storage);
    free(names);
    return keys;
}
/* Floor names in the loaded executable; untranslated rows become "#id" keys. */
static P4DataKeys *floor_keys(void)
{
    const size_t bytes = (size_t)RETAIL_FLOORS * P4_DATA_FLOOR_NAME_SIZE;
    if (!guest_range(FLOOR_NAMES, (uint32_t)bytes)) return NULL;
    return bank_keys(P4_DATA_FLOOR, P4_G2H(FLOOR_NAMES), bytes, P4_DATA_FLOOR_NAME_SIZE, RETAIL_FLOORS);
}
/* Formations and groups have no names: every key is "p4:<kind>:#<id>". */
static P4DataKeys *unnamed_keys(P4DataKind kind, size_t rows)
{
    static const char empty[] = "";
    const char **names = malloc(rows * sizeof *names);
    if (!names) return NULL;
    for (size_t i = 0; i < rows; ++i) names[i] = empty;
    P4DataKeys *keys = p4_data_keys_create(kind, names, rows);
    free(names);
    return keys;
}
/* The disc's MSG.TBL: five packed blocks (u32 length, payload, 16-byte
 * alignment); 0 arcana (21), 1 skills (19), 2 enemies (21), 3 Personas (17).
 * The executable is the loaded image, untouched before any script runs. */
int p4_data_catalog_capture(void)
{
    if (disc.state) return disc.state > 0 ? 0 : -1;
    P4VfsSourcePlan *plan = NULL;
    if (vfs_source_prepare_selected() || vfs_source_plan_acquire(P4_VFS_DISC, "battle/MSG.TBL", &plan) || !plan)
        return unavailable("the disc's battle/MSG.TBL could not be opened");
    const P4VfsSourceInfo *info = vfs_source_plan_info(plan);
    const uint64_t size = info ? info->size : 0;
    uint8_t *bytes = size && size <= MSG_MAX ? malloc((size_t)size) : NULL;
    const int read = bytes ? vfs_source_plan_read_at(plan, 0, bytes, (size_t)size) : -1;
    vfs_source_plan_release(plan);
    if (read) { free(bytes); return unavailable("the disc's battle/MSG.TBL could not be read"); }
    const uint8_t *blocks[4] = {0};
    size_t sizes[4] = {0};
    for (size_t offset = 0, block = 0; block < 4; ++block) {
        if (size - offset < 4u) break;
        const uint32_t length = u32le(bytes + offset);
        if (length > size - offset - 4u) break;
        blocks[block] = bytes + offset + 4u; sizes[block] = length;
        offset = (offset + 4u + length + 15u) & ~(size_t)15u;
        if (offset > size) break;
    }
    if (!blocks[3]) { free(bytes); return unavailable("the disc's battle/MSG.TBL has no name banks"); }
    for (size_t i = 0; i < ARCANA_ROWS && (i + 1) * ARCANA_NAME <= sizes[0]; ++i)
        fixed_name(blocks[0] + i * ARCANA_NAME, ARCANA_NAME, disc.arcana[i], sizeof disc.arcana[i]);
    disc.keys[P4_DATA_SKILL] = bank_keys(P4_DATA_SKILL, blocks[1], sizes[1], 19, P4_SKILL_RETAIL_COUNT);
    disc.keys[P4_DATA_ENEMY] = bank_keys(P4_DATA_ENEMY, blocks[2], sizes[2], P4_ENEMY_NAME_SIZE, P4_ENEMY_RETAIL_ROWS);
    disc.keys[P4_DATA_PERSONA] = bank_keys(P4_DATA_PERSONA, blocks[3], sizes[3], P4_PERSONA_NAME_SIZE, P4_PERSONA_RETAIL_ROWS);
    disc.keys[P4_DATA_ITEM] = item_keys();
    disc.keys[P4_DATA_FLOOR] = floor_keys();
    disc.keys[P4_DATA_FORMATION] = unnamed_keys(P4_DATA_FORMATION, RETAIL_FORMATIONS);
    disc.keys[P4_DATA_ENCOUNTERS] = unnamed_keys(P4_DATA_ENCOUNTERS, RETAIL_ENCOUNTERS);
    disc.keys[P4_DATA_TREASURE] = unnamed_keys(P4_DATA_TREASURE, RETAIL_TREASURES);
    free(bytes);
    for (unsigned kind = 0; kind < P4_DATA_KIND_COUNT; ++kind)
        if (!disc.keys[kind]) {
            for (unsigned k = 0; k < P4_DATA_KIND_COUNT; ++k) { p4_data_keys_destroy(disc.keys[k]); disc.keys[k] = NULL; }
            return unavailable("not enough memory for the catalog keys");
        }
    disc.state = 1;
    return 0;
}
/* Keys come from the disc as the player owns it, before any package or
 * script can change a name; nothing is read without a script package or a
 * data layer. The data layers join the table seal here: this handler sorts
 * before game-tables.sources (same phase, by name), and p4_data_edits_prepare
 * refuses loudly if the table builder were already sealed. */
static int prepare(void *unused)
{
    (void)unused;
    if (p4_data_edits_prepare()) return -1;
    int needed = p4_data_edits_count() != 0;
    for (size_t i = 0, count = p4_packages_count(); i < count && !needed; ++i)
        needed = (p4_package_provider_claims(p4_packages_provider(i)) & P4_PACKAGE_CLAIM_SCRIPT) != 0;
    if (needed) {
        (void)p4_data_catalog_capture(); /* Failures are reported by the corresponding read. */
        capture_original_enemies();
    }
    return 0;
}
static void rollback(void *unused) { (void)unused; p4_data_edits_discard(); }
const P4PackageHandler p4_game_data_boot_provider = {
    .name = "game-data.catalog", .prepare = prepare, .rollback = rollback, .boot_phase = P4_PACKAGE_BOOT_TABLES
};

/* ---- banks: the loaded ones, or the table seal's working rows ---- */

static const P4DataBanks *seal;
void p4_data_catalog_seal_view(const P4DataBanks *banks) { seal = banks; }
typedef struct Bank { const uint8_t *base; uint32_t rows; } Bank;
static int seal_bank(uint32_t slot, Bank *out)
{
    switch (slot) {
    case SLOT_SKILL_ELEMENT: *out = (Bank){seal->skill_element, seal->skill_elements}; break;
    case SLOT_SKILL: *out = (Bank){seal->skill, seal->skills}; break;
    case SLOT_SKILL_NAME: *out = (Bank){seal->skill_name, seal->skill_names}; break;
    case SLOT_ENEMY: *out = (Bank){seal->enemy, seal->enemies}; break;
    case SLOT_ENEMY_AFFINITY: *out = (Bank){seal->enemy_affinity, seal->enemies}; break;
    case SLOT_ENEMY_NAME: *out = (Bank){seal->enemy_name, seal->enemy_names}; break;
    case SLOT_PERSONA: *out = (Bank){seal->persona, seal->personas}; break;
    case SLOT_PERSONA_GROWTH: *out = (Bank){seal->persona_growth, seal->personas}; break;
    case SLOT_PERSONA_PARTY: *out = (Bank){seal->party, seal->parties}; break;
    case SLOT_PERSONA_AFFINITY: *out = (Bank){seal->persona_affinity, seal->personas}; break;
    case SLOT_PERSONA_NAME: *out = (Bank){seal->persona_name, seal->persona_names}; break;
    case SLOT_FLOOR: *out = (Bank){seal->floor, seal->floors}; break;
    case SLOT_FORMATION: *out = (Bank){seal->formation, seal->formations}; break;
    case SLOT_ENCOUNTERS: *out = (Bank){seal->encounters, seal->encounter_groups}; break;
    case SLOT_TREASURE: *out = (Bank){seal->treasure, seal->treasures}; break;
    default: return 0;
    }
    return out->base && out->rows;
}
static int bank(uint32_t slot, uint32_t stride, Bank *out)
{
    if (seal) return seal_bank(slot, out);
    uint32_t base = 0, rows = 0;
    if (!p4_game_table_extent_rows(slot, stride, &base, &rows) || !rows) return 0;
    out->base = P4_G2H(base); out->rows = rows;
    return 1;
}
static const uint8_t *bank_row(uint32_t slot, uint32_t stride, uint32_t id)
{
    Bank b;
    return bank(slot, stride, &b) && id < b.rows ? b.base + (size_t)id * stride : NULL;
}

int p4_data_catalog_ready(const char **reason)
{
    const char *why = NULL;
    Bank b;
    if (disc.state == 0) why = "p4.data needs a script package at boot; the catalog was not captured";
    else if (disc.state < 0) why = disc.reason;
    else if (!seal && (!bank(SLOT_SKILL, P4_SKILL_ROW_SIZE, &b) || !P4_G32(SLOT_ITEM)))
        why = "the game has not loaded its tables yet";
    if (reason) *reason = why;
    return why ? -1 : 0;
}
int p4_data_catalog_live_banks(P4DataBanks *out)
{
    memset(out, 0, sizeof *out);
    if (seal || p4_data_catalog_ready(NULL)) return -1;
    Bank b;
    if (bank(SLOT_SKILL_ELEMENT, P4_SKILL_ELEMENT_SIZE, &b)) { out->skill_element = b.base; out->skill_elements = b.rows; }
    if (bank(SLOT_SKILL, P4_SKILL_ROW_SIZE, &b)) { out->skill = b.base; out->skills = b.rows; }
    if (bank(SLOT_SKILL_NAME, 19, &b)) { out->skill_name = b.base; out->skill_names = b.rows; }
    Bank affinity;
    if (bank(SLOT_ENEMY, P4_ENEMY_ROW_SIZE, &b) && bank(SLOT_ENEMY_AFFINITY, P4_ENEMY_AFFINITY_SIZE, &affinity)) {
        out->enemy = b.base; out->enemy_affinity = affinity.base;
        out->enemies = b.rows < affinity.rows ? b.rows : affinity.rows;
    }
    if (bank(SLOT_ENEMY_NAME, P4_ENEMY_NAME_SIZE, &b)) { out->enemy_name = b.base; out->enemy_names = b.rows; }
    Bank growth;
    if (bank(SLOT_PERSONA, P4_PERSONA_METADATA_SIZE, &b) && bank(SLOT_PERSONA_GROWTH, P4_PERSONA_GROWTH_SIZE, &growth) &&
        bank(SLOT_PERSONA_AFFINITY, P4_PERSONA_AFFINITY_SIZE, &affinity)) {
        out->persona = b.base; out->persona_growth = growth.base; out->persona_affinity = affinity.base;
        out->personas = b.rows < growth.rows ? b.rows : growth.rows;
        if (affinity.rows < out->personas) out->personas = affinity.rows;
    }
    if (bank(SLOT_PERSONA_PARTY, P4_DATA_PARTY_ROW_SIZE, &b)) { out->party = b.base; out->parties = b.rows; }
    if (bank(SLOT_PERSONA_NAME, P4_PERSONA_NAME_SIZE, &b)) { out->persona_name = b.base; out->persona_names = b.rows; }
    /* Added items read an immutable projection; only retail rows are written. */
    const P4GameItems *items = p4_game_tables_items_active();
    if (!(items && p4_game_tables_items_base(items)) && P4_G32(SLOT_ITEM)) {
        out->item = P4_G2H(P4_G32(SLOT_ITEM));
        out->items = P4_ITEM_RETAIL_COUNT;
    }
    if (bank(SLOT_FLOOR, P4_ENCOUNT_SELECTOR_SIZE, &b)) { out->floor = b.base; out->floors = b.rows; }
    if (bank(SLOT_FORMATION, P4_ENCOUNT_FORMATION_SIZE, &b)) { out->formation = b.base; out->formations = b.rows; }
    if (bank(SLOT_ENCOUNTERS, P4_ENCOUNT_WEIGHTED_SIZE, &b)) { out->encounters = b.base; out->encounter_groups = b.rows; }
    if (bank(SLOT_TREASURE, P4_ENCOUNT_RECORD_SIZE, &b)) { out->treasure = b.base; out->treasures = b.rows; }
    return 0;
}

static size_t added_count(P4DataKind kind)
{
    switch (kind) {
    case P4_DATA_SKILL: return p4_game_skills_count(p4_game_skills_active());
    case P4_DATA_ITEM: return p4_game_tables_items_count(p4_game_tables_items_active());
    default: return 0;
    }
}
/* Dungeon rows: the slot and stride of each ENCOUNT kind. */
static int encount_bank(P4DataKind kind, uint32_t *slot, uint32_t *stride)
{
    switch (kind) {
    case P4_DATA_FLOOR: *slot = SLOT_FLOOR; *stride = P4_ENCOUNT_SELECTOR_SIZE; return 1;
    case P4_DATA_FORMATION: *slot = SLOT_FORMATION; *stride = P4_ENCOUNT_FORMATION_SIZE; return 1;
    case P4_DATA_ENCOUNTERS: *slot = SLOT_ENCOUNTERS; *stride = P4_ENCOUNT_WEIGHTED_SIZE; return 1;
    case P4_DATA_TREASURE: *slot = SLOT_TREASURE; *stride = P4_ENCOUNT_RECORD_SIZE; return 1;
    default: return 0;
    }
}
static size_t retail_count(P4DataKind kind)
{
    uint32_t slot, stride;
    Bank b;
    switch (kind) {
    case P4_DATA_SKILL: return P4_SKILL_RETAIL_COUNT;
    case P4_DATA_ITEM: return P4_ITEM_RETAIL_COUNT;
    case P4_DATA_ENEMY: return p4_game_enemies_native_limit();   /* dense: retail then added */
    case P4_DATA_PERSONA: return p4_game_personas_native_limit();
    default:
        /* Dungeon rows are dense: as many as the loaded block holds. */
        return encount_bank(kind, &slot, &stride) && bank(slot, stride, &b) ? b.rows : 0;
    }
}
static size_t row_count(P4DataKind kind) { return retail_count(kind) + added_count(kind); }
uint32_t p4_data_catalog_id(P4DataKind kind, size_t index)
{
    const size_t retail = retail_count(kind);
    if (index < retail) return (uint32_t)index;
    index -= retail;
    if (kind == P4_DATA_SKILL) {
        const P4GameSkillView *view = p4_game_skills_at(p4_game_skills_active(), index);
        return view ? view->desc.id : UINT32_MAX;
    }
    if (kind == P4_DATA_ITEM) {
        const P4GameItemView *view = p4_game_tables_items_at(p4_game_tables_items_active(), index);
        return view ? view->id : UINT32_MAX;
    }
    return UINT32_MAX;
}
int p4_data_catalog_index(P4DataKind kind, uint32_t id, size_t *index)
{
    const size_t count = row_count(kind), retail = retail_count(kind);
    if (id < retail) { *index = id; return 0; }
    for (size_t i = retail; i < count; ++i)
        if (p4_data_catalog_id(kind, i) == id) { *index = i; return 0; }
    return -1;
}

/* Added rows carry owner:local table-source keys; p4.data shows them as
 * owner:kind:local. */
static char added_key[P4_TABLE_KEY_SIZE + 16];
static const char *authored_key(P4DataKind kind, const char *key)
{
    const char *colon = key ? strchr(key, ':') : NULL;
    if (!colon) return NULL;
    snprintf(added_key, sizeof added_key, "%.*s:%s:%s", (int)(colon - key), key, p4_data_kind_name(kind), colon + 1);
    return added_key;
}
const char *p4_data_catalog_key(P4DataKind kind, size_t index)
{
    const P4DataKeys *keys = (unsigned)kind < P4_DATA_KIND_COUNT ? disc.keys[kind] : NULL;
    if (keys && index < p4_data_keys_count(keys)) return p4_data_keys_at(keys, (uint32_t)index);
    const uint32_t id = p4_data_catalog_id(kind, index);
    switch (kind) {
    case P4_DATA_SKILL: {
        const P4GameSkillView *view = p4_game_skills_find(p4_game_skills_active(), id);
        return view ? authored_key(kind, view->desc.key) : NULL;
    }
    case P4_DATA_ENEMY: {
        const P4GameEnemyView *view = p4_game_enemies_find(p4_game_enemies_active(), id);
        return view ? authored_key(kind, view->desc.key) : NULL;
    }
    case P4_DATA_PERSONA: {
        const P4GamePersonaView *view = p4_game_personas_find(p4_game_personas_active(), id);
        return view ? authored_key(kind, view->desc.key) : NULL;
    }
    case P4_DATA_ITEM: {
        const P4GameItemView *view = p4_game_tables_items_find(p4_game_tables_items_active(), id);
        return view ? authored_key(kind, view->key) : NULL;
    }
    default:
        /* Dungeon rows past the disc's (a producer moved the block): "#id". */
        if (kind < P4_DATA_FLOOR || id == UINT32_MAX) return NULL;
        snprintf(added_key, sizeof added_key, "p4:%s:#%lu", p4_data_kind_name(kind), (unsigned long)id);
        return added_key;
    }
}

static uint32_t item_name_stream(uint32_t id)
{
    if (id < P4_ITEM_RETAIL_COUNT) return P4_G32(ITEM_NAMES + id * 4u);
    return p4_game_tables_items_text_binding(p4_game_tables_items_active(), P4_TEXT_ITEM, id, P4_TEXT_NAME);
}
const char *p4_data_catalog_name(P4DataKind kind, size_t index, char *out, size_t capacity)
{
    const uint32_t id = p4_data_catalog_id(kind, index);
    const uint8_t *row = NULL;
    const char *name = NULL;
    if (!capacity || id == UINT32_MAX) return NULL;
    switch (kind) {
    case P4_DATA_SKILL:
        if ((row = bank_row(SLOT_SKILL_NAME, 19, id))) { fixed_name(row, 19, out, capacity); name = out; }
        else name = guest_string(p4_game_skills_name(p4_game_skills_active(), id), out, capacity);
        break;
    case P4_DATA_ENEMY:
        if ((row = bank_row(SLOT_ENEMY_NAME, P4_ENEMY_NAME_SIZE, id))) { fixed_name(row, P4_ENEMY_NAME_SIZE, out, capacity); name = out; }
        break;
    case P4_DATA_PERSONA:
        if ((row = bank_row(SLOT_PERSONA_NAME, P4_PERSONA_NAME_SIZE, id))) { fixed_name(row, P4_PERSONA_NAME_SIZE, out, capacity); name = out; }
        break;
    case P4_DATA_ITEM: {
        /* A renamed retail item: the composed name, which datGetItemNameId
         * returns once battle-table completion published it. */
        const char *renamed = p4_game_tables_derived_item_name(id);
        const char *original = seal && id < P4_ITEM_RETAIL_COUNT ? p4_data_keys_name(disc.keys[kind], id) : NULL;
        if (renamed || seal) {
            snprintf(out, capacity, "%s", renamed ? renamed : original ? original : "");
            name = out;
        }
        else name = guest_string(item_name_stream(id), out, capacity);
        break;
    }
    case P4_DATA_FLOOR: {
        /* The executable's floor names; floor names are not editable. */
        const char *floor = p4_data_keys_name(disc.keys[kind], id);
        if (floor) { snprintf(out, capacity, "%s", floor); name = out; }
        break;
    }
    default: break;
    }
    return name && !p4_data_placeholder(name) ? name : NULL;
}

const char *p4_data_catalog_arcana(uint32_t arcana)
{
    return disc.state > 0 && arcana >= 1 && arcana < ARCANA_ROWS && disc.arcana[arcana][0] ? disc.arcana[arcana] : NULL;
}
int p4_data_catalog_arcana_parse(const char *name, uint32_t *arcana)
{
    char wanted[32], form[32];
    if (!name || !p4_data_match_form(name, wanted, sizeof wanted)) return -1;
    for (uint32_t i = 1; i < ARCANA_ROWS; ++i) {
        const char *candidate = p4_data_catalog_arcana(i);
        if (candidate && p4_data_match_form(candidate, form, sizeof form) && !strcmp(form, wanted)) { *arcana = i; return 0; }
    }
    return -1;
}

int p4_data_catalog_skill(size_t index, P4DataSkill *out)
{
    const uint32_t id = p4_data_catalog_id(P4_DATA_SKILL, index);
    if (id < P4_SKILL_RETAIL_COUNT) {
        const uint8_t *element = bank_row(SLOT_SKILL_ELEMENT, P4_SKILL_ELEMENT_SIZE, id);
        if (!element) return -1;
        const uint8_t *metadata = id < P4_SKILL_RETAIL_ACTIVE_COUNT ? bank_row(SLOT_SKILL, P4_SKILL_ROW_SIZE, id) : NULL;
        p4_data_skill_decode(id, element, metadata, out);
        return 0;
    }
    P4GameSkillDefinition definition;
    if (id == UINT32_MAX || p4_game_skills_definition_find(p4_game_skills_active(), id, &definition)) return -1;
    uint8_t element[2] = {P4_DATA_ELEMENT_NONE, 0};
    if (definition.retail_element) memcpy(element, definition.retail_element, sizeof element);
    else if (definition.authored && definition.authored->element_id < P4_DATA_ELEMENTS)
        element[0] = (uint8_t)definition.authored->element_id;
    p4_data_skill_decode(id, element, definition.active ? definition.metadata : NULL, out);
    return 0;
}
int p4_data_catalog_enemy(size_t index, P4DataEnemy *out)
{
    const uint32_t id = p4_data_catalog_id(P4_DATA_ENEMY, index);
    const uint8_t *row = bank_row(SLOT_ENEMY, P4_ENEMY_ROW_SIZE, id);
    const uint8_t *affinity = bank_row(SLOT_ENEMY_AFFINITY, P4_ENEMY_AFFINITY_SIZE, id);
    if (!row || !affinity) return -1;
    p4_data_enemy_decode(id, row, affinity, out);
    return 0;
}
int p4_data_catalog_persona(size_t index, P4DataPersona *out)
{
    const uint32_t id = p4_data_catalog_id(P4_DATA_PERSONA, index);
    const uint8_t *row = bank_row(SLOT_PERSONA, P4_PERSONA_METADATA_SIZE, id);
    const uint8_t *growth = bank_row(SLOT_PERSONA_GROWTH, P4_PERSONA_GROWTH_SIZE, id);
    const uint8_t *affinity = bank_row(SLOT_PERSONA_AFFINITY, P4_PERSONA_AFFINITY_SIZE, id);
    if (!row || !growth || !affinity) return -1;
    const uint8_t *party = id >= P4_DATA_PARTY_PERSONA_FIRST && id < P4_DATA_PARTY_PERSONA_FIRST + P4_DATA_PARTY_PERSONAS ?
        bank_row(SLOT_PERSONA_PARTY, P4_DATA_PARTY_ROW_SIZE, id - P4_DATA_PARTY_PERSONA_FIRST) : NULL;
    p4_data_persona_decode(id, row, growth, party, affinity, out);
    return 0;
}
int p4_data_catalog_item(size_t index, P4DataItem *out)
{
    const uint32_t id = p4_data_catalog_id(P4_DATA_ITEM, index);
    const P4GameItems *items = p4_game_tables_items_active();
    const uint8_t *row = NULL;
    if (seal) row = id < seal->items ? seal->item + (size_t)id * P4_ITEM_ROW_SIZE : NULL;
    else if (items && p4_game_tables_items_base(items)) row = (const uint8_t *)p4_game_tables_items_metadata(items, id);
    else if (id < P4_ITEM_RETAIL_COUNT && P4_G32(SLOT_ITEM)) row = P4_G2H(P4_G32(SLOT_ITEM) + id * P4_ITEM_ROW_SIZE);
    if (!row) return -1;
    p4_data_item_decode(id, row, out);
    return 0;
}
static const uint8_t *dungeon_row(P4DataKind kind, size_t index, uint32_t *id)
{
    uint32_t slot, stride;
    *id = p4_data_catalog_id(kind, index);
    return encount_bank(kind, &slot, &stride) ? bank_row(slot, stride, *id) : NULL;
}
int p4_data_catalog_floor(size_t index, P4DataFloor *out)
{
    uint32_t id;
    const uint8_t *row = dungeon_row(P4_DATA_FLOOR, index, &id);
    if (!row) return -1;
    p4_data_floor_decode(id, row, out);
    return 0;
}
int p4_data_catalog_formation(size_t index, P4DataFormation *out)
{
    uint32_t id;
    const uint8_t *row = dungeon_row(P4_DATA_FORMATION, index, &id);
    if (!row) return -1;
    p4_data_formation_decode(id, row, out);
    return 0;
}
int p4_data_catalog_encounters(size_t index, P4DataEncounters *out)
{
    uint32_t id;
    const uint8_t *row = dungeon_row(P4_DATA_ENCOUNTERS, index, &id);
    if (!row) return -1;
    p4_data_encounters_decode(id, row, out);
    return 0;
}
int p4_data_catalog_treasure(size_t index, P4DataTreasure *out)
{
    uint32_t id;
    const uint8_t *row = dungeon_row(P4_DATA_TREASURE, index, &id);
    if (!row) return -1;
    p4_data_treasure_decode(id, row, out);
    return 0;
}

/* ---- rows for lookup ---- */

static char row_name[CATALOG_NAME_MAX];
static const char *rows_name(void *user, size_t index)
{
    return p4_data_catalog_name((P4DataKind)(uintptr_t)user, index, row_name, sizeof row_name);
}
static const char *rows_key(void *user, size_t index) { return p4_data_catalog_key((P4DataKind)(uintptr_t)user, index); }
static void rows_detail(void *user, size_t index, char *out, size_t capacity)
{
    const P4DataKind kind = (P4DataKind)(uintptr_t)user;
    P4DataEnemy enemy;
    P4DataPersona persona;
    out[0] = 0;
    if (kind == P4_DATA_ENEMY && !p4_data_catalog_enemy(index, &enemy)) snprintf(out, capacity, "level %u", enemy.level);
    else if (kind == P4_DATA_PERSONA && !p4_data_catalog_persona(index, &persona)) snprintf(out, capacity, "level %u", persona.level);
    else snprintf(out, capacity, "id %lu", (unsigned long)p4_data_catalog_id(kind, index));
}
int p4_data_catalog_rows(P4DataKind kind, P4DataRows *rows)
{
    if ((unsigned)kind >= P4_DATA_KIND_COUNT || p4_data_catalog_ready(NULL)) return -1;
    *rows = (P4DataRows){
        .kind = kind, .count = row_count(kind), .keys = disc.keys[kind], .user = (void *)(uintptr_t)kind,
        .name = rows_name, .key = rows_key, .detail = rows_detail
    };
    return 0;
}
int p4_data_ref(P4DataKind kind, uint32_t id, P4DataRef *out)
{
    if (!out) return -1;
    memset(out, 0, sizeof *out);
    out->kind = kind; out->id = id;
    size_t index;
    if ((unsigned)kind >= P4_DATA_KIND_COUNT || p4_data_catalog_ready(NULL) || p4_data_catalog_index(kind, id, &index))
        return -1;
    const char *key = p4_data_catalog_key(kind, index);
    if (!key) return -1;
    snprintf(out->key, sizeof out->key, "%s", key);
    if (!p4_data_catalog_name(kind, index, out->name, sizeof out->name)) out->name[0] = 0;
    return 0;
}
