/* p4.data edit layers (include/p4pc/game_data_edit.h).
 *
 * The layers write the composed table inputs in place, through the same
 * field layouts the catalog decodes (game_data.c), while the catalog reads
 * those inputs instead of the loaded banks (p4_data_catalog_seal_view). */
#include "p4pc/game_data_edit.h"
#include "game_data_edit_internal.h"
#include "game_table_derived_internal.h"
#include "game_table_inputs_internal.h"
#include "p4pc/game_table_boot.h"
#include "p4pc/game_table_derived.h"
#include "p4pc/game_data_catalog.h"
#include "p4pc/game_enemies.h"
#include "p4pc/game_items.h"
#include "p4pc/game_personas.h"
#include "p4pc/game_skills_native.h"
#include "p4pc/game_encounters.h"
#include "p4pc/package_memory.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* P4_DATA_EDIT_CHECK: p4mod check's build of this file (tools/p4mod_data_edits.c,
 * docs/modding/p4mod.md). The same field rules run, in the same order, over one
 * zeroed scratch row per bank, and nothing is written. A rule that needs the
 * disc's tables (a name, key or ID, a row's current bytes) calls unchecked()
 * and passes instead of deciding; p4mod reports those as left to p4mod test. */
#ifdef P4_DATA_EDIT_CHECK
enum { CHECKING = 1 };
static unsigned unchecked_count;
static char unchecked_first[192];
static void unchecked(const char *what)
{
    if (!unchecked_count++) snprintf(unchecked_first, sizeof unchecked_first, "%s", what);
}
#else
enum { CHECKING = 0 };
static inline void unchecked(const char *what) { (void)what; }
#endif

enum { OWNER_SIZE = 64, FIELD_SIZE = 96, CONFLICT_OWNERS = 8, LABEL_SIZE = 160 };

typedef struct Layer {
    const P4PackageProvider *provider;
    char owner[OWNER_SIZE];
    int luau, xml;
    char luau_hex[P4_CONTENT_DIGEST_HEX + 1], xml_hex[P4_CONTENT_DIGEST_HEX + 1], writes_hex[P4_CONTENT_DIGEST_HEX + 1];
} Layer;
/* Every field any layer assigned, with its writers in load order. */
typedef struct Entry {
    P4DataKind kind;
    uint32_t id;
    char field[FIELD_SIZE];
    uint16_t owners[CONFLICT_OWNERS];
    size_t owner_count;
} Entry;
typedef struct Bank { uint8_t *base; uint32_t rows; } Bank;

static struct {
    Layer *layers;
    Entry *entries;
    size_t count, capacity, entry_count, entry_capacity;
    P4DataEditRunner runner;
    int running;
    size_t current;
    P4ContentDigest writes;
    Bank skill_element, skill, skill_name, enemy, enemy_affinity, enemy_name;
    Bank persona, persona_growth, party, persona_affinity, persona_name, item;
    Bank floor, formation, encounters, treasure;
    P4GameTableInputKind encount; /* ENCOUNT, or COUNT when a producer relocated the dungeon blocks */
    P4DataBanks view;
    P4GameTableInput *inputs[P4_TABLE_INPUT_COUNT];
    unsigned touched;
} edits;
/* Session edits: the loaded banks' original bytes under every write (undone
 * in reverse), and the persisted records the save carries. */
typedef struct Undo { uint8_t *at; uint32_t size; size_t offset; } Undo;
enum { SESSION_UNDO_BYTES = 4u << 20 };
static struct {
    int active, persist;
    char owner[OWNER_SIZE];
    Undo *undo;
    size_t undo_count, undo_capacity, mark;
    uint8_t *saved;
    size_t saved_used, saved_capacity, saved_mark;
    P4DataSessionRecord *records;
    size_t record_count, record_capacity, record_mark;
} session;

static int refuse(char *error, size_t capacity, const char *format, ...)
{
    if (error && capacity) {
        va_list args;
        va_start(args, format);
        vsnprintf(error, capacity, format, args);
        va_end(args);
    }
    return -1;
}
static void *grow(void *array, size_t *capacity, size_t count, size_t element)
{
    if (count < *capacity) return array;
    const size_t next = *capacity ? *capacity * 2u : 8u, bytes = next * element;
    if (p4_packages_memory_reserve(bytes)) return NULL;
    void *replacement = malloc(bytes);
    if (!replacement) { p4_packages_memory_cancel(bytes); return NULL; }
    p4_packages_memory_commit(bytes);
    if (count) memcpy(replacement, array, count * element);
    free(array);
    if (*capacity) p4_packages_memory_release(*capacity * element);
    *capacity = next;
    return replacement;
}

void p4_data_edits_runner(P4DataEditRunner runner) { edits.runner = runner; }
size_t p4_data_edits_count(void) { return edits.count; }
const char *p4_data_edit_owner(void)
{
    if (session.active) return session.owner;
    return edits.running ? edits.layers[edits.current].owner : NULL;
}

#ifndef P4_DATA_EDIT_CHECK
static int select_layer(const P4PackageProvider *provider, uint32_t *inputs, char *error, size_t capacity)
{
    const int luau = p4_package_provider_has_file(provider, P4_DATA_EDIT_LUAU) > 0;
    const int xml = p4_package_provider_has_file(provider, P4_DATA_EDIT_XML) > 0;
    if (!luau && !xml) return 0;
    if (luau && !edits.runner)
        return refuse(error, capacity, "data.luau needs Luau scripting, which this build does not have; use data/edits.xml");
    const P4PackageInfo *info = p4_package_provider_info(provider);
    if (!info) return refuse(error, capacity, "data edits: package has no identity");
    Layer *layers = grow(edits.layers, &edits.capacity, edits.count, sizeof *edits.layers);
    if (!layers) return refuse(error, capacity, "data edits: package memory budget exhausted");
    edits.layers = layers;
    Layer *layer = edits.layers + edits.count++;
    memset(layer, 0, sizeof *layer);
    layer->provider = provider; layer->luau = luau; layer->xml = xml;
    snprintf(layer->owner, sizeof layer->owner, "%s", info->id);
    *inputs = 1u << P4_TABLE_INPUT_SKILL | 1u << P4_TABLE_INPUT_UNIT | 1u << P4_TABLE_INPUT_PERSONA |
        1u << P4_TABLE_INPUT_MSG | 1u << P4_TABLE_INPUT_ITEM | 1u << P4_TABLE_INPUT_ENCOUNT;
    return 1;
}

/* ---- banks ---- */

static int bind(Bank *bank, P4GameTableInputKind kind, uint32_t span, uint32_t header, uint32_t stride)
{
    P4GameTableInput *input = edits.inputs[kind];
    const P4GameTableInputSpan *s = input ? p4_game_table_input_span(input, span) : NULL;
    const size_t size = s ? p4_game_table_input_span_size(s) : 0;
    if (size < header) return -1;
    bank->base = input->bytes + p4_game_table_input_span_offset(s) + header;
    bank->rows = (uint32_t)((size - header) / stride);
    return 0;
}
/* A dungeon block: the composed span, or the producer's relocated rows the
 * native publisher moves its slot to (edited in place, input untouched). */
static int bind_encount(Bank *bank, uint32_t span, uint32_t stride, int *relocated)
{
    size_t size = 0;
    uint8_t *rows = p4_game_tables_derived_relocated_rows(P4_TABLE_INPUT_ENCOUNT, span, &size);
    if (!rows) return bind(bank, P4_TABLE_INPUT_ENCOUNT, span, 0, stride);
    *relocated = 1;
    bank->base = rows;
    bank->rows = (uint32_t)(size / stride);
    return 0;
}
static int bind_all(P4GameTableInput *const *inputs, size_t count)
{
    memset(edits.inputs, 0, sizeof edits.inputs);
    for (size_t i = 0; i < count; ++i) {
        const P4GameTableInputKind kind = p4_game_table_input_kind(inputs[i]);
        if ((unsigned)kind < P4_TABLE_INPUT_COUNT) edits.inputs[kind] = inputs[i];
    }
    if (bind(&edits.skill_element, P4_TABLE_INPUT_SKILL, 0, 0, P4_SKILL_ELEMENT_SIZE) ||
        bind(&edits.skill, P4_TABLE_INPUT_SKILL, 1, 0, P4_SKILL_ROW_SIZE) ||
        bind(&edits.skill_name, P4_TABLE_INPUT_MSG, 1, 0, 19) ||
        bind(&edits.enemy, P4_TABLE_INPUT_UNIT, 0, 0, P4_ENEMY_ROW_SIZE) ||
        bind(&edits.enemy_affinity, P4_TABLE_INPUT_UNIT, 1, 0, P4_ENEMY_AFFINITY_SIZE) ||
        bind(&edits.enemy_name, P4_TABLE_INPUT_MSG, 2, 0, P4_ENEMY_NAME_SIZE) ||
        bind(&edits.persona, P4_TABLE_INPUT_PERSONA, 0, 0, P4_PERSONA_METADATA_SIZE) ||
        bind(&edits.persona_growth, P4_TABLE_INPUT_PERSONA, 1, 0, P4_PERSONA_GROWTH_SIZE) ||
        bind(&edits.party, P4_TABLE_INPUT_PERSONA, 2, 0, P4_DATA_PARTY_ROW_SIZE) ||
        bind(&edits.persona_affinity, P4_TABLE_INPUT_UNIT, 2, 0, P4_PERSONA_AFFINITY_SIZE) ||
        bind(&edits.persona_name, P4_TABLE_INPUT_MSG, 3, 0, P4_PERSONA_NAME_SIZE) ||
        bind(&edits.item, P4_TABLE_INPUT_ITEM, 0, P4_ITEM_FILE_HEADER_SIZE, P4_ITEM_ROW_SIZE))
        return -1;
    int relocated = 0;
    if (bind_encount(&edits.floor, P4_ENCOUNT_SELECTOR_SPAN, P4_ENCOUNT_SELECTOR_SIZE, &relocated) ||
        bind_encount(&edits.formation, P4_ENCOUNT_FORMATION_SPAN, P4_ENCOUNT_FORMATION_SIZE, &relocated) ||
        bind_encount(&edits.encounters, P4_ENCOUNT_WEIGHTED_SPAN, P4_ENCOUNT_WEIGHTED_SIZE, &relocated) ||
        bind_encount(&edits.treasure, P4_ENCOUNT_RECORD_SPAN, P4_ENCOUNT_RECORD_SIZE, &relocated))
        return -1;
    /* 000761 relocates all four blocks together or none. */
    edits.encount = relocated ? P4_TABLE_INPUT_COUNT : P4_TABLE_INPUT_ENCOUNT;
    edits.view = (P4DataBanks){
        .skill_element = edits.skill_element.base, .skill_elements = edits.skill_element.rows,
        .skill = edits.skill.base, .skills = edits.skill.rows,
        .skill_name = edits.skill_name.base, .skill_names = edits.skill_name.rows,
        .enemy = edits.enemy.base, .enemy_affinity = edits.enemy_affinity.base, .enemy_name = edits.enemy_name.base,
        .enemies = edits.enemy.rows < edits.enemy_affinity.rows ? edits.enemy.rows : edits.enemy_affinity.rows,
        .enemy_names = edits.enemy_name.rows,
        .persona = edits.persona.base, .persona_growth = edits.persona_growth.base, .party = edits.party.base,
        .persona_affinity = edits.persona_affinity.base, .persona_name = edits.persona_name.base,
        .personas = edits.persona.rows, .parties = edits.party.rows, .persona_names = edits.persona_name.rows,
        .item = edits.item.base, .items = edits.item.rows,
        .floor = edits.floor.base, .floors = edits.floor.rows,
        .formation = edits.formation.base, .formations = edits.formation.rows,
        .encounters = edits.encounters.base, .encounter_groups = edits.encounters.rows,
        .treasure = edits.treasure.base, .treasures = edits.treasure.rows
    };
    if (edits.persona_growth.rows < edits.view.personas) edits.view.personas = edits.persona_growth.rows;
    if (edits.persona_affinity.rows < edits.view.personas) edits.view.personas = edits.persona_affinity.rows;
    return 0;
}
#else
/* Check build: one zeroed scratch row behind every bank. Its bytes decide
 * nothing: every rule that would read them calls unchecked() instead. */
static void check_bind(void)
{
    static uint8_t scratch[4096];
    memset(scratch, 0, sizeof scratch);
    Bank *const banks[] = {&edits.skill_element, &edits.skill, &edits.skill_name, &edits.enemy, &edits.enemy_affinity,
        &edits.enemy_name, &edits.persona, &edits.persona_growth, &edits.party, &edits.persona_affinity,
        &edits.persona_name, &edits.item, &edits.floor, &edits.formation, &edits.encounters, &edits.treasure};
    for (size_t i = 0; i < sizeof banks / sizeof banks[0]; ++i) *banks[i] = (Bank){scratch, 1};
    edits.view.enemies = edits.view.personas = 1;
    edits.encount = P4_TABLE_INPUT_ENCOUNT;
}
#endif

/* ---- the ledger ---- */

static const char *label(P4DataKind kind, uint32_t id, char *out, size_t capacity)
{
#ifdef P4_DATA_EDIT_CHECK
    (void)id; /* no catalog: the record's line names the row */
    snprintf(out, capacity, "%s", p4_data_kind_name(kind));
#else
    size_t index;
    char name[P4_DATA_NAME_SIZE];
    const char *key = !p4_data_catalog_index(kind, id, &index) ? p4_data_catalog_key(kind, index) : NULL;
    const char *shown = key ? p4_data_catalog_name(kind, index, name, sizeof name) : NULL;
    if (shown) snprintf(out, capacity, "%s \"%s\" (%s)", p4_data_kind_name(kind), shown, key);
    else if (key) snprintf(out, capacity, "%s %s", p4_data_kind_name(kind), key);
    else snprintf(out, capacity, "%s %lu", p4_data_kind_name(kind), (unsigned long)id);
#endif
    return out;
}
/* Session writes: keep the loaded bytes under them (plain heap, bounded). */
static int remember(uint8_t *at, uint32_t size)
{
    if (!session.active) return 0;
    if (session.saved_used + size > SESSION_UNDO_BYTES) return -1;
    if (session.undo_count == session.undo_capacity) {
        const size_t next = session.undo_capacity ? session.undo_capacity * 2u : 64u;
        Undo *undo = realloc(session.undo, next * sizeof *undo);
        if (!undo) return -1;
        session.undo = undo; session.undo_capacity = next;
    }
    if (session.saved_used + size > session.saved_capacity) {
        size_t next = session.saved_capacity ? session.saved_capacity * 2u : 4096u;
        while (next < session.saved_used + size) next *= 2u;
        uint8_t *saved = realloc(session.saved, next);
        if (!saved) return -1;
        session.saved = saved; session.saved_capacity = next;
    }
    memcpy(session.saved + session.saved_used, at, size);
    session.undo[session.undo_count++] = (Undo){at, size, session.saved_used};
    session.saved_used += size;
    return 0;
}
static int record(P4DataKind kind, uint32_t id, const char *field)
{
    if (session.active) return 0; /* the ledger reports data layers only */
    Entry *entry = NULL;
    for (size_t i = 0; i < edits.entry_count; ++i) {
        Entry *candidate = edits.entries + i;
        if (candidate->kind == kind && candidate->id == id && !strcmp(candidate->field, field)) { entry = candidate; break; }
    }
    if (!entry) {
        Entry *entries = grow(edits.entries, &edits.entry_capacity, edits.entry_count, sizeof *edits.entries);
        if (!entries) return -1;
        edits.entries = entries;
        entry = edits.entries + edits.entry_count++;
        memset(entry, 0, sizeof *entry);
        entry->kind = kind; entry->id = id;
        snprintf(entry->field, sizeof entry->field, "%s", field);
    }
    const uint16_t owner = (uint16_t)edits.current;
    if (entry->owner_count && entry->owners[entry->owner_count - 1] == owner) return 0;
    if (entry->owner_count == CONFLICT_OWNERS) memmove(entry->owners, entry->owners + 1, (CONFLICT_OWNERS - 1) * sizeof *entry->owners);
    else ++entry->owner_count;
    entry->owners[entry->owner_count - 1] = owner;
    return 0;
}
static void put(uint8_t *at, uint32_t width, uint32_t value)
{
    for (uint32_t i = 0; i < width; ++i) at[i] = (uint8_t)(value >> (8u * i));
}
static uint32_t get(const uint8_t *at, uint32_t width)
{
    uint32_t value = 0;
    for (uint32_t i = 0; i < width; ++i) value |= (uint32_t)at[i] << (8u * i);
    return value;
}
/* One field write: bytes, ledger and the layer's write digest. */
static int commit(P4DataKind kind, uint32_t id, const char *field, P4GameTableInputKind input,
    uint8_t *at, uint32_t width, uint32_t value, char *error, size_t capacity)
{
    if (CHECKING) return 0; /* the value passed its rules; nothing is written */
    if (record(kind, id, field)) return refuse(error, capacity, "data edits: package memory budget exhausted");
    if (remember(at, width)) return refuse(error, capacity, "session edits: out of memory");
    put(at, width, value);
    if ((unsigned)input < P4_TABLE_INPUT_COUNT) edits.touched |= 1u << input;
    const uint32_t words[4] = {(uint32_t)kind, id, value, width};
    p4_content_digest_update(&edits.writes, words, sizeof words);
    p4_content_digest_update(&edits.writes, field, strlen(field) + 1);
    return 0;
}

/* ---- values ---- */

static int whole(const P4DataEditValue *value, const char *field, const char *row, double minimum, double maximum,
    uint32_t *out, char *error, size_t capacity)
{
    if (!value || value->text)
        return refuse(error, capacity, "%s %s: expected a number, got \"%s\"", row, field, value && value->string ? value->string : "");
    if (!isfinite(value->number) || value->number != floor(value->number) || value->number < minimum || value->number > maximum)
        return refuse(error, capacity, "%s %s: %g is not a whole number from %g to %g", row, field, value->number, minimum, maximum);
    *out = (uint32_t)value->number;
    return 0;
}
static int text(const P4DataEditValue *value, const char *field, const char *row, const char **out, char *error, size_t capacity)
{
    if (!value || !value->text || !value->string)
        return refuse(error, capacity, "%s %s: expected a name, got a number", row, field);
    *out = value->string;
    return 0;
}
/* Whether the catalog has no row of kind with this ID; the check build cannot know. */
static int catalog_lacks(P4DataKind kind, uint32_t id)
{
#ifdef P4_DATA_EDIT_CHECK
    (void)kind; (void)id;
    unchecked("a row named by ID exists");
    return 0;
#else
    size_t index;
    return p4_data_catalog_index(kind, id, &index) != 0;
#endif
}
/* A row of another kind by name, key or ID; zero clears when allowed. */
static int reference(P4DataKind kind, const P4DataEditValue *value, const char *field, const char *row, int allow_none,
    uint32_t *id, char *error, size_t capacity)
{
    if (!value->text) {
        if (allow_none && value->number == 0) { *id = 0; return 0; }
        /* Above UINT32_MAX no row has the ID, and the cast would be undefined: refuse first. */
        if (!isfinite(value->number) || value->number < 1 || value->number > (double)UINT32_MAX ||
            value->number != floor(value->number) || catalog_lacks(kind, (uint32_t)value->number))
            return refuse(error, capacity, "%s %s: no %s has the ID %g", row, field, p4_data_kind_name(kind), value->number);
        *id = (uint32_t)value->number;
        return 0;
    }
    if (allow_none && (!strcmp(value->string, "none") || !value->string[0])) { *id = 0; return 0; }
#ifdef P4_DATA_EDIT_CHECK
    unchecked("a row named by name or key exists");
    *id = 0;
#else
    size_t index;
    P4DataRows rows;
    char message[512];
    if (p4_data_catalog_rows(kind, &rows)) return refuse(error, capacity, "%s %s: the catalog is unavailable", row, field);
    if (p4_data_lookup(&rows, value->string, &index, message, sizeof message))
        return refuse(error, capacity, "%s %s: %s", row, field, message);
    *id = p4_data_catalog_id(kind, index);
#endif
    return 0;
}
static int stat_index(const char *name)
{
    static const char *const stats[P4_DATA_STATS] = {"st", "ma", "en", "ag", "lu"};
    for (int i = 0; i < (int)P4_DATA_STATS; ++i) if (!strcmp(name, stats[i])) return i;
    return -1;
}
static int affinity(P4DataKind kind, uint32_t id, uint8_t *words, const char *path, const char *column_name,
    const P4DataEditValue *value, const char *row, P4GameTableInputKind input, char *error, size_t capacity)
{
    uint32_t column;
    if (p4_data_element_parse(column_name, &column) || column >= P4_DATA_AFFINITY_COLUMNS)
        return refuse(error, capacity, "%s %s: \"%s\" is not an affinity column (physical..dark, confusion..silence)",
            row, path, column_name);
    uint8_t *at = words + column * 2u;
    uint16_t packed;
    if (!value->text) {
        /* A percent keeps the flags: fire = 150 on a weak enemy stays weak. */
        uint32_t percent;
        if (whole(value, path, row, 0, 1275, &percent, error, capacity)) return -1;
        if (percent % 5u) return refuse(error, capacity, "%s %s: %u is not a multiple of 5 (the game stores rates in 5%% steps)",
            row, path, (unsigned)percent);
        packed = (uint16_t)((get(at, 2) & 0xFF00u) | percent / 5u);
    } else {
        P4DataAffinity kind_value;
        if (p4_data_affinity_parse(value->string, &kind_value))
            return refuse(error, capacity, "%s %s: \"%s\" is not normal, weak, resist, null, repel or drain", row, path, value->string);
        if (p4_data_affinity_encode(column, kind_value, &packed))
            return refuse(error, capacity, "%s %s: ailments can only be normal, weak, resist or null", row, path);
    }
    return commit(kind, id, path, input, at, 2, packed, error, capacity);
}
/* A display name in its fixed-width MSG row (19, 21 or 17 bytes with the
 * final NUL). The key keeps the disc's name. Plain ASCII only: the game's
 * two-byte glyphs have no agreed text form here. */
static int rename_row(P4DataKind kind, uint32_t id, const Bank *names, uint32_t width, const P4DataEditValue *value,
    const char *row, char *error, size_t capacity)
{
    const char *name;
    if (text(value, "name", row, &name, error, capacity)) return -1;
    const size_t length = strlen(name);
    if (!length || length >= width)
        return refuse(error, capacity, "%s name: \"%s\" must be 1 to %u characters", row, name, (unsigned)(width - 1u));
    for (size_t i = 0; i < length; ++i)
        if ((unsigned char)name[i] < 0x20 || (unsigned char)name[i] > 0x7E)
            return refuse(error, capacity, "%s name: \"%s\" may use only printable ASCII characters", row, name);
    if (CHECKING) return 0; /* a valid name; the name table and the write need the disc */
    if (id >= names->rows) return refuse(error, capacity, "%s name: outside the name table", row);
    if (record(kind, id, "name")) return refuse(error, capacity, "data edits: package memory budget exhausted");
    uint8_t *at = names->base + (size_t)id * width;
    if (remember(at, width)) return refuse(error, capacity, "session edits: out of memory");
    memset(at, 0, width);
    memcpy(at, name, length);
    edits.touched |= 1u << P4_TABLE_INPUT_MSG;
    const uint32_t words[2] = {(uint32_t)kind, id};
    p4_content_digest_update(&edits.writes, words, sizeof words);
    p4_content_digest_update(&edits.writes, at, width);
    return 0;
}

/* ---- fields ---- */

static int skill_set(uint32_t id, const char *path, const P4DataEditValue *value, char *error, size_t capacity)
{
    char row[LABEL_SIZE];
    label(P4_DATA_SKILL, id, row, sizeof row);
    if (id >= edits.skill_element.rows) return refuse(error, capacity, "%s: outside the skill table", row);
    uint8_t *element = edits.skill_element.base + (size_t)id * P4_SKILL_ELEMENT_SIZE;
    uint8_t *m = id < edits.skill.rows ? edits.skill.base + (size_t)id * P4_SKILL_ROW_SIZE : NULL;
    uint32_t v;
    if (!strcmp(path, "name")) return rename_row(P4_DATA_SKILL, id, &edits.skill_name, 19, value, row, error, capacity);
    if (!strcmp(path, "element")) {
        const char *name;
        uint32_t wanted;
        if (text(value, path, row, &name, error, capacity)) return -1;
        if (p4_data_element_parse(name, &wanted) || wanted > 7)
            return refuse(error, capacity, "%s element: \"%s\" is not an attack element (physical, fire, ice, electric, wind, "
                "almighty, light or dark)", row, name);
        if (CHECKING) unchecked("the skill is an attack skill whose element can change");
        else if (!m || element[0] > 7)
            return refuse(error, capacity, "%s element: only attack skills (physical..dark) can change element", row);
        return commit(P4_DATA_SKILL, id, path, P4_TABLE_INPUT_SKILL, element, 1, wanted, error, capacity);
    }
    if (CHECKING) unchecked("the skill is not passive");
    else if (!m) return refuse(error, capacity, "%s %s: passive skills have no %s", row, path, path);
    static const struct { const char *path; uint32_t offset, width, minimum, maximum; } fields[] = {
        {"power", 0x12, 2, 0, 9999}, {"accuracy", 0x0E, 1, 0, 100}, {"critical", 0x25, 1, 0, 100},
        {"hits.min", 0x0F, 1, 1, 9}, {"hits.max", 0x10, 1, 1, 9},
    };
    for (size_t i = 0; i < sizeof fields / sizeof fields[0]; ++i) {
        if (strcmp(path, fields[i].path)) continue;
        if (whole(value, path, row, fields[i].minimum, fields[i].maximum, &v, error, capacity)) return -1;
        return commit(P4_DATA_SKILL, id, path, P4_TABLE_INPUT_SKILL, m + fields[i].offset, fields[i].width, v, error, capacity);
    }
    if (!strcmp(path, "cost.sp") || !strcmp(path, "cost.hp_percent")) {
        const int sp = !strcmp(path, "cost.sp");
        if (whole(value, path, row, 0, sp ? 999 : 100, &v, error, capacity)) return -1;
        if (commit(P4_DATA_SKILL, id, "cost", P4_TABLE_INPUT_SKILL, m + 0x03, 1, sp ? 2u : 1u, error, capacity)) return -1;
        return commit(P4_DATA_SKILL, id, "cost", P4_TABLE_INPUT_SKILL, m + 0x04, 2, v, error, capacity);
    }
    if (!strcmp(path, "ailment.chance") || !strcmp(path, "ailment.status")) {
        if (CHECKING) unchecked("the skill inflicts an ailment");
        else if (m[0x18] != 1)
            return refuse(error, capacity, "%s %s: this skill inflicts no ailment", row, path);
        if (path[8] == 'c') {
            if (whole(value, path, row, 0, 100, &v, error, capacity)) return -1;
            return commit(P4_DATA_SKILL, id, path, P4_TABLE_INPUT_SKILL, m + 0x19, 1, v, error, capacity);
        }
        const char *name;
        if (text(value, path, row, &name, error, capacity)) return -1;
        if (p4_data_ailment_parse(name, &v) || v == 0x100000u)
            return refuse(error, capacity, "%s ailment.status: \"%s\" is not dizzy, rage, fear, silence, confusion, poison, "
                "exhaustion, enervation or ko", row, name);
        return commit(P4_DATA_SKILL, id, path, P4_TABLE_INPUT_SKILL, m + 0x1C, 4, v, error, capacity);
    }
    return refuse(error, capacity, "%s: no skill field \"%s\" (name, power, cost.sp, cost.hp_percent, accuracy, critical, "
        "hits.min, hits.max, element, ailment.status, ailment.chance)", row, path);
}
static int enemy_set(uint32_t id, const char *path, const P4DataEditValue *value, char *error, size_t capacity)
{
    char row[LABEL_SIZE];
    label(P4_DATA_ENEMY, id, row, sizeof row);
    if (id >= edits.view.enemies) return refuse(error, capacity, "%s: outside the enemy table", row);
    uint8_t *m = edits.enemy.base + (size_t)id * P4_ENEMY_ROW_SIZE;
    uint32_t v;
    if (!strcmp(path, "name"))
        return rename_row(P4_DATA_ENEMY, id, &edits.enemy_name, P4_ENEMY_NAME_SIZE, value, row, error, capacity);
    static const struct { const char *path; uint32_t offset, width, minimum, maximum; } fields[] = {
        {"level", 0x03, 1, 1, 99}, {"hp", 0x04, 2, 1, 65535}, {"sp", 0x06, 2, 0, 65535},
        {"exp", 0x1E, 2, 0, 65535}, {"yen", 0x20, 2, 0, 65535},
    };
    for (size_t i = 0; i < sizeof fields / sizeof fields[0]; ++i) {
        if (strcmp(path, fields[i].path)) continue;
        if (whole(value, path, row, fields[i].minimum, fields[i].maximum, &v, error, capacity)) return -1;
        return commit(P4_DATA_ENEMY, id, path, P4_TABLE_INPUT_UNIT, m + fields[i].offset, fields[i].width, v, error, capacity);
    }
    if (!strncmp(path, "stats.", 6) && stat_index(path + 6) >= 0) {
        if (whole(value, path, row, 1, 255, &v, error, capacity)) return -1;
        return commit(P4_DATA_ENEMY, id, path, P4_TABLE_INPUT_UNIT, m + 0x08 + stat_index(path + 6), 1, v, error, capacity);
    }
    if (!strncmp(path, "affinities.", 11))
        return affinity(P4_DATA_ENEMY, id, edits.enemy_affinity.base + (size_t)id * P4_ENEMY_AFFINITY_SIZE, path, path + 11,
            value, row, P4_TABLE_INPUT_UNIT, error, capacity);
    unsigned slot = 0;
    char rest[16] = "";
    if (sscanf(path, "skills.%u%15s", &slot, rest) == 1 && slot >= 1 && slot <= P4_DATA_ENEMY_SKILLS) {
        if (reference(P4_DATA_SKILL, value, path, row, 1, &v, error, capacity)) return -1;
        return commit(P4_DATA_ENEMY, id, path, P4_TABLE_INPUT_UNIT, m + 0x0E + (slot - 1) * 2u, 2, v, error, capacity);
    }
    if (sscanf(path, "drops.%u.%15s", &slot, rest) == 2 && slot >= 1 && slot <= P4_DATA_ENEMY_DROPS) {
        uint8_t *drop = m + 0x22 + (slot - 1) * 4u;
        if (!strcmp(rest, "item")) {
            if (reference(P4_DATA_ITEM, value, path, row, 1, &v, error, capacity)) return -1;
            return commit(P4_DATA_ENEMY, id, path, P4_TABLE_INPUT_UNIT, drop, 2, v, error, capacity);
        }
        if (!strcmp(rest, "chance")) {
            /* A weight in 1/200 (0x00243A30 draws rand % 200); 0.5% steps. */
            if (!value || value->text || !isfinite(value->number) || value->number < 0 || value->number > 100 ||
                value->number * 2 != floor(value->number * 2))
                return refuse(error, capacity, "%s %s: expected a percent from 0 to 100 in steps of 0.5", row, path);
            return commit(P4_DATA_ENEMY, id, path, P4_TABLE_INPUT_UNIT, drop + 2, 1, (uint32_t)(value->number * 2), error, capacity);
        }
    }
    return refuse(error, capacity, "%s: no enemy field \"%s\" (name, level, hp, sp, stats.st..lu, exp, yen, "
        "affinities.<element>, skills.1..8, drops.1..4.item, drops.1..4.chance)", row, path);
}
static int persona_set(uint32_t id, const char *path, const P4DataEditValue *value, char *error, size_t capacity)
{
    char row[LABEL_SIZE];
    label(P4_DATA_PERSONA, id, row, sizeof row);
    if (id >= edits.view.personas) return refuse(error, capacity, "%s: outside the Persona table", row);
    uint8_t *m = edits.persona.base + (size_t)id * P4_PERSONA_METADATA_SIZE;
    uint32_t v;
    if (!strcmp(path, "name"))
        return rename_row(P4_DATA_PERSONA, id, &edits.persona_name, P4_PERSONA_NAME_SIZE, value, row, error, capacity);
    if (!strcmp(path, "level")) {
        if (whole(value, path, row, 1, 99, &v, error, capacity)) return -1;
        return commit(P4_DATA_PERSONA, id, path, P4_TABLE_INPUT_PERSONA, m + 0x03, 1, v, error, capacity);
    }
    if (!strncmp(path, "stats.", 6) && stat_index(path + 6) >= 0) {
        if (whole(value, path, row, 1, 99, &v, error, capacity)) return -1;
        return commit(P4_DATA_PERSONA, id, path, P4_TABLE_INPUT_PERSONA, m + 0x04 + stat_index(path + 6), 1, v, error, capacity);
    }
    if (!strncmp(path, "affinities.", 11))
        return affinity(P4_DATA_PERSONA, id, edits.persona_affinity.base + (size_t)id * P4_PERSONA_AFFINITY_SIZE, path,
            path + 11, value, row, P4_TABLE_INPUT_UNIT, error, capacity);
    return refuse(error, capacity, "%s: no Persona field \"%s\" (name, level, stats.st..lu, affinities.<element>; "
        "learnset changes use learnset:add/remove/move/replace)", row, path);
}
/* Shops (game_data.h): a consumable's first day in the item store and gear's
 * weapon-store gates. sold writes the retail marks: false moves a consumable
 * to 3/31 or gives gear the Blank gate; true moves a never-sold consumable to
 * 4/1 or drops the Blank gate, keeping real dates and gates. The ledger names
 * the bytes: shop.from or shop.unlock. */
static int item_shop(uint32_t id, uint8_t *m, uint32_t kind, const char *path, const P4DataEditValue *value,
    const char *row, char *error, size_t capacity)
{
    if (CHECKING) unchecked("the item is gear or a consumable, the kind its shop fields need");
    else if (kind > 3)
        return refuse(error, capacity, "%s %s: only gear (the weapon store) and consumables (the item store) are sold",
            row, path);
    uint8_t *date = m + 0x38, *gates = m + 0x3C;
    const P4GameTableInputKind input = P4_TABLE_INPUT_ITEM;
    if (!strcmp(path, "shop.sold")) {
        const char *answer;
        if (text(value, path, row, &answer, error, capacity)) return -1;
        const int sold = !strcmp(answer, "true");
        if (!sold && strcmp(answer, "false")) return refuse(error, capacity, "%s %s: expected true or false", row, path);
        if (kind == 3) {
            const uint32_t never = (uint32_t)P4_DATA_ITEM_NEVER_MONTH | (uint32_t)P4_DATA_ITEM_NEVER_DAY << 8;
            if (sold && get(date, 2) != never) return 0;
            return commit(P4_DATA_ITEM, id, "shop.from", input, date, 2, sold ? 4u | 1u << 8 : never, error, capacity);
        }
        if (!sold) return commit(P4_DATA_ITEM, id, "shop.unlock", input, gates, 4,
            p4_data_unlock_encode((P4DataUnlock){P4_DATA_ITEM_NEVER_MATERIAL, 1}), error, capacity);
        for (unsigned i = 0; i < P4_DATA_ITEM_UNLOCKS; ++i) {
            P4DataUnlock gate = p4_data_unlock_decode(get(gates + i * 4u, 4));
            if (!gate.count || gate.material != P4_DATA_ITEM_NEVER_MATERIAL) continue;
            gate.count = 0;
            if (commit(P4_DATA_ITEM, id, "shop.unlock", input, gates + i * 4u, 4, p4_data_unlock_encode(gate), error,
                capacity)) return -1;
        }
        return 0;
    }
    if (!strcmp(path, "shop.from.month") || !strcmp(path, "shop.from.day")) {
        if (CHECKING) unchecked("the item is a consumable");
        else if (kind != 3)
            return refuse(error, capacity, "%s %s: only consumables have an item-store date; gear unlocks through "
                "shop.unlock", row, path);
        const int month = path[10] == 'm';
        uint32_t v;
        if (whole(value, path, row, 1, month ? 12 : 31, &v, error, capacity)) return -1;
        return commit(P4_DATA_ITEM, id, "shop.from", input, date + (month ? 0 : 1), 1, v, error, capacity);
    }
    if (!CHECKING && kind == 3 && !strncmp(path, "shop.unlock", 11))
        return refuse(error, capacity, "%s %s: only gear has unlock gates; consumables use shop.from", row, path);
    if (!strcmp(path, "shop.unlock")) {
        /* {} (none): no gates, so the weapon store lists it from the start. */
        if (!value->text || strcmp(value->string, "none"))
            return refuse(error, capacity, "%s %s: assign a list of { material = ..., count = n }, or {} for none", row,
                path);
        for (unsigned i = 0; i < P4_DATA_ITEM_UNLOCKS; ++i)
            if (commit(P4_DATA_ITEM, id, "shop.unlock", input, gates + i * 4u, 4,
                p4_data_unlock_encode((P4DataUnlock){P4_DATA_ITEM_NEVER_MATERIAL, 0}), error, capacity)) return -1;
        return 0;
    }
    unsigned slot;
    char rest[16];
    if (sscanf(path, "shop.unlock.%u.%15s", &slot, rest) == 2 && slot >= 1 && slot <= P4_DATA_ITEM_UNLOCKS) {
        uint8_t *at = gates + (slot - 1u) * 4u;
        P4DataUnlock gate = p4_data_unlock_decode(get(at, 4));
        uint32_t v;
        if (!strcmp(rest, "material")) {
            if (reference(P4_DATA_ITEM, value, path, row, 0, &v, error, capacity)) return -1;
            if (CHECKING) unchecked("the unlock material is a material");
            else if (v >= edits.item.rows || get(edits.item.base + (size_t)v * P4_ITEM_ROW_SIZE, 4) != 5)
                return refuse(error, capacity, "%s %s: not a material (the weapon store counts the materials sold to it)",
                    row, path);
            gate.material = (uint16_t)v;
        } else if (!strcmp(rest, "count")) {
            if (whole(value, path, row, 0, 255, &v, error, capacity)) return -1;
            gate.count = (uint8_t)v;
        } else {
            return refuse(error, capacity, "%s: no item field \"%s\" (shop.unlock[n].material, .count)", row, path);
        }
        return commit(P4_DATA_ITEM, id, "shop.unlock", input, at, 4, p4_data_unlock_encode(gate), error, capacity);
    }
    return refuse(error, capacity, "%s: no item field \"%s\" (shop.sold, shop.from.month, shop.from.day, "
        "shop.unlock[1..2].material, .count)", row, path);
}
static int item_set(uint32_t id, const char *path, const P4DataEditValue *value, char *error, size_t capacity)
{
    char row[LABEL_SIZE];
    label(P4_DATA_ITEM, id, row, sizeof row);
    if (session.active && (!edits.item.base || !strcmp(path, "name")))
        return refuse(error, capacity, "%s %s: %s change only while the game builds its tables (data.luau)", row, path,
            edits.item.base ? "item names" : "with added items, item rows");
    if (id >= edits.item.rows) return refuse(error, capacity, "%s: outside the item table", row);
    uint8_t *m = edits.item.base + (size_t)id * P4_ITEM_ROW_SIZE;
    const uint32_t kind = get(m, 4);
    uint32_t v;
    if (!strcmp(path, "name")) {
        /* Retail item names live in the executable: they go to the item-name
         * channel the override reads, not to ITEMTBL. */
        const char *name;
        if (text(value, path, row, &name, error, capacity)) return -1;
        if (CHECKING) {
            unchecked("the item is a retail item, whose name can change");
            if (!p4_game_tables_derived_item_name_valid(name))
                return refuse(error, capacity, "%s name: \"%s\" must be 1 to %u printable ASCII characters", row, name,
                    (unsigned)P4_TABLE_DERIVED_ITEM_NAME_MAX - 1u);
            return 0;
        }
        if (id >= P4_ITEM_RETAIL_COUNT || !p4_game_tables_derived_item_name_valid(name))
            return refuse(error, capacity, "%s name: \"%s\" must be 1 to %u printable ASCII characters", row, name,
                (unsigned)P4_TABLE_DERIVED_ITEM_NAME_MAX - 1u);
#ifndef P4_DATA_EDIT_CHECK
        if (record(P4_DATA_ITEM, id, path) || p4_game_tables_derived_item_name_set(id, name))
            return refuse(error, capacity, "data edits: package memory budget exhausted");
        const uint32_t words[2] = {(uint32_t)P4_DATA_ITEM, id};
        p4_content_digest_update(&edits.writes, words, sizeof words);
        p4_content_digest_update(&edits.writes, name, strlen(name) + 1u);
#endif
        return 0;
    }
    if (!strcmp(path, "price")) {
        if (whole(value, path, row, 0, 999999, &v, error, capacity)) return -1;
        return commit(P4_DATA_ITEM, id, path, P4_TABLE_INPUT_ITEM, m + 0x34, 4, v, error, capacity);
    }
    static const struct { const char *path; uint32_t offset, kind; } gear[] = {
        {"attack", 0x08, 0}, {"hit", 0x0A, 0}, {"defense", 0x10, 1}, {"evasion", 0x12, 1},
    };
    for (size_t i = 0; i < sizeof gear / sizeof gear[0]; ++i) {
        if (strcmp(path, gear[i].path)) continue;
        if (CHECKING) unchecked("the item is the gear kind the field needs");
        else if (kind != gear[i].kind)
            return refuse(error, capacity, "%s %s: only %s have %s", row, path, gear[i].kind ? "armor" : "weapons", path);
        if (whole(value, path, row, 0, 9999, &v, error, capacity)) return -1;
        return commit(P4_DATA_ITEM, id, path, P4_TABLE_INPUT_ITEM, m + gear[i].offset, 2, v, error, capacity);
    }
    if (!strcmp(path, "skill")) {
        if (CHECKING) unchecked("the item is a consumable or accessory");
        else if (kind != 2 && kind != 3)
            return refuse(error, capacity, "%s skill: only consumables (the skill they use) and accessories (the passive they "
                "grant) have one", row);
        if (reference(P4_DATA_SKILL, value, path, row, CHECKING || kind == 2, &v, error, capacity)) return -1;
        return commit(P4_DATA_ITEM, id, path, P4_TABLE_INPUT_ITEM, m + 0x24, 4, v, error, capacity);
    }
    if (!strncmp(path, "shop.", 5)) return item_shop(id, m, kind, path, value, row, error, capacity);
    return refuse(error, capacity, "%s: no item field \"%s\" (name, price, attack, hit, defense, evasion, skill, shop)", row,
        path);
}
/* ---- dungeons ---- */

static int truth(const P4DataEditValue *value, const char *path, const char *row, int *out, char *error, size_t capacity)
{
    if (value->text && !strcmp(value->string, "true")) { *out = 1; return 0; }
    if (value->text && !strcmp(value->string, "false")) { *out = 0; return 0; }
    return refuse(error, capacity, "%s %s: expected true or false", row, path);
}
static int formation_set(uint32_t id, const char *path, const P4DataEditValue *value, char *error, size_t capacity)
{
    char row[LABEL_SIZE];
    label(P4_DATA_FORMATION, id, row, sizeof row);
    if (id >= edits.formation.rows) return refuse(error, capacity, "%s: outside the formation table", row);
    uint8_t *m = edits.formation.base + (size_t)id * P4_ENCOUNT_FORMATION_SIZE;
    unsigned slot;
    char tail;
    if (sscanf(path, "enemies.%u%c", &slot, &tail) == 1 && slot >= 1 && slot <= P4_DATA_FORMATION_ENEMIES) {
        uint32_t enemy;
        if (reference(P4_DATA_ENEMY, value, path, row, 1, &enemy, error, capacity)) return -1;
        return commit(P4_DATA_FORMATION, id, path, edits.encount, m + P4_ENCOUNT_FORMATION_ENEMIES + (slot - 1u) * 2u, 2,
            enemy, error, capacity);
    }
    return refuse(error, capacity, "%s: no formation field \"%s\" (enemies.1..5)", row, path);
}
/* rates.<class>, <class>.N.formation|weight, and alternate.<the same>. A
 * variant keeps at least one rate above 0: the picker asserts on a zero sum. */
static int encounters_set(uint32_t id, const char *path, const P4DataEditValue *value, char *error, size_t capacity)
{
    char row[LABEL_SIZE];
    label(P4_DATA_ENCOUNTERS, id, row, sizeof row);
    if (id >= edits.encounters.rows) return refuse(error, capacity, "%s: outside the encounter groups", row);
    uint8_t *m = edits.encounters.base + (size_t)id * P4_ENCOUNT_WEIGHTED_SIZE;
    const uint32_t variant = !strncmp(path, "alternate.", 10);
    const char *field = path + (variant ? 10 : 0);
    for (unsigned c = 0; c < P4_DATA_ENCOUNTER_CLASSES; ++c) {
        const char *name = p4_data_encounter_class_name((P4DataEncounterClass)c);
        const size_t length = strlen(name);
        if (!strncmp(field, "rates.", 6) && !strcmp(field + 6, name)) {
            uint32_t v;
            if (whole(value, path, row, 0, 255, &v, error, capacity)) return -1;
            unsigned total = v;
            for (unsigned other = 0; other < P4_DATA_ENCOUNTER_CLASSES; ++other)
                if (other != c) total += m[p4_data_encounter_rate_offset(variant, (P4DataEncounterClass)other)];
            if (CHECKING) { if (!v) unchecked("the variant's other encounter rates are not all 0"); }
            else if (!total) return refuse(error, capacity, "%s %s: the three rates cannot all be 0", row, path);
            return commit(P4_DATA_ENCOUNTERS, id, path, edits.encount,
                m + p4_data_encounter_rate_offset(variant, (P4DataEncounterClass)c), 1, v, error, capacity);
        }
        if (strncmp(field, name, length) || field[length] != '.') continue;
        uint32_t first, count;
        p4_data_encounter_class_slots((P4DataEncounterClass)c, &first, &count);
        unsigned slot;
        char rest[16];
        const int parts = sscanf(field + length + 1, "%u.%15s", &slot, rest);
        if (parts < 1 || slot < 1 || slot > count)
            return refuse(error, capacity, "%s %s: %s holds slots 1 to %u", row, path, name, (unsigned)count);
        uint8_t *at = m + p4_data_encounter_slot_offset(variant, first + slot - 1u);
        char base[48];
        snprintf(base, sizeof base, "%s%s.%u", variant ? "alternate." : "", name, slot);
        uint32_t v;
        if (parts == 1) {
            /* {} empties the slot. */
            if (!value->text || strcmp(value->string, "none"))
                return refuse(error, capacity, "%s %s: assign { formation = ..., weight = n }, or {} to empty it", row, path);
            if (commit(P4_DATA_ENCOUNTERS, id, base, edits.encount, at, 2, 0, error, capacity)) return -1;
            return commit(P4_DATA_ENCOUNTERS, id, base, edits.encount, at + 2, 2, 0, error, capacity);
        }
        if (!strcmp(rest, "formation")) {
            if (reference(P4_DATA_FORMATION, value, path, row, 1, &v, error, capacity)) return -1;
            return commit(P4_DATA_ENCOUNTERS, id, base, edits.encount, at, 2, v, error, capacity);
        }
        if (!strcmp(rest, "weight")) {
            if (whole(value, path, row, 0, 65535, &v, error, capacity)) return -1;
            return commit(P4_DATA_ENCOUNTERS, id, base, edits.encount, at + 2, 2, v, error, capacity);
        }
        return refuse(error, capacity, "%s %s: a slot has formation and weight", row, path);
    }
    return refuse(error, capacity, "%s: no encounters field \"%s\" (rates.normal|special|rare, normal[1..20], "
        "special[1..5], rare[1..5] with formation and weight; alternate.<the same>)", row, path);
}
/* items.N.item|weight|locked, or items.N = {} to empty a record. */
static int treasure_set(uint32_t id, const char *path, const P4DataEditValue *value, char *error, size_t capacity)
{
    char row[LABEL_SIZE];
    label(P4_DATA_TREASURE, id, row, sizeof row);
    if (id >= edits.treasure.rows) return refuse(error, capacity, "%s: outside the treasure groups", row);
    unsigned slot;
    char rest[16];
    const int parts = sscanf(path, "items.%u.%15s", &slot, rest);
    if (parts < 1 || slot < 1 || slot > P4_DATA_TREASURE_RECORDS)
        return refuse(error, capacity, "%s: no treasure field \"%s\" (items[1..29] with item, weight and locked)", row,
            path);
    uint8_t *at = edits.treasure.base + (size_t)id * P4_ENCOUNT_RECORD_SIZE + (slot - 1u) * P4_ENCOUNT_RECORD_STRIDE;
    char base[24];
    snprintf(base, sizeof base, "items.%u", slot);
    uint32_t v;
    if (parts == 1) {
        if (!value->text || strcmp(value->string, "none"))
            return refuse(error, capacity, "%s %s: assign { item = ..., weight = n }, or {} to empty it", row, path);
        if (commit(P4_DATA_TREASURE, id, base, edits.encount, at, 2, 0, error, capacity)) return -1;
        return commit(P4_DATA_TREASURE, id, base, edits.encount, at + 2, 2, 0, error, capacity);
    }
    if (!strcmp(rest, "item")) {
        if (reference(P4_DATA_ITEM, value, path, row, 1, &v, error, capacity)) return -1;
        return commit(P4_DATA_TREASURE, id, base, edits.encount, at + 2, 2, v, error, capacity);
    }
    if (!strcmp(rest, "weight")) {
        if (whole(value, path, row, 0, 65535, &v, error, capacity)) return -1;
        return commit(P4_DATA_TREASURE, id, base, edits.encount, at, 2, v, error, capacity);
    }
    if (!strcmp(rest, "locked")) {
        int locked;
        if (truth(value, path, row, &locked, error, capacity)) return -1;
        return commit(P4_DATA_TREASURE, id, base, edits.encount, at + P4_ENCOUNT_RECORD_FLAGS, 1,
            (at[P4_ENCOUNT_RECORD_FLAGS] & ~1u) | (unsigned)locked, error, capacity);
    }
    return refuse(error, capacity, "%s %s: a record has item, weight and locked", row, path);
}
/* spawn, refill (shadows, at most the 15 field slots), chests (at most the
 * 8 chest slots), encounters and treasure (which group; through them, the
 * group's own fields: floors sharing a group share its edits). */
static int floor_set(uint32_t id, const char *path, const P4DataEditValue *value, char *error, size_t capacity)
{
    char row[LABEL_SIZE];
    label(P4_DATA_FLOOR, id, row, sizeof row);
    if (id >= edits.floor.rows) return refuse(error, capacity, "%s: outside the floor table", row);
    uint8_t *m = edits.floor.base + (size_t)id * P4_ENCOUNT_SELECTOR_SIZE;
    static const struct { const char *path; uint32_t offset, maximum; } counts[] = {
        {"refill", 2, 15}, {"spawn", 3, 15}, {"chests", 4, 8},
    };
    uint32_t v;
    for (size_t i = 0; i < sizeof counts / sizeof counts[0]; ++i) {
        if (strcmp(path, counts[i].path)) continue;
        if (whole(value, path, row, 0, counts[i].maximum, &v, error, capacity)) return -1;
        return commit(P4_DATA_FLOOR, id, path, edits.encount, m + counts[i].offset, 1, v, error, capacity);
    }
    if (!strcmp(path, "encounters") || !strcmp(path, "treasure")) {
        const int treasure = path[0] == 't';
        if (reference(treasure ? P4_DATA_TREASURE : P4_DATA_ENCOUNTERS, value, path, row, 0, &v, error, capacity))
            return -1;
        return commit(P4_DATA_FLOOR, id, path, edits.encount, m + (treasure ? P4_ENCOUNT_SELECTOR_RECORDS :
            P4_ENCOUNT_SELECTOR_WEIGHTED), 2, v, error, capacity);
    }
    if (!strncmp(path, "encounters.", 11))
        return encounters_set(get(m + P4_ENCOUNT_SELECTOR_WEIGHTED, 2), path + 11, value, error, capacity);
    if (!strncmp(path, "treasure.", 9))
        return treasure_set(get(m + P4_ENCOUNT_SELECTOR_RECORDS, 2), path + 9, value, error, capacity);
    return refuse(error, capacity, "%s: no floor field \"%s\" (spawn, refill, chests, encounters, treasure)", row, path);
}

/* A persisted session edit, kept as it was made (game_data_session.c). */
static int keep(P4DataKind kind, uint32_t id, unsigned learn, const char *path, const P4DataEditValue *value,
    uint32_t skill, uint32_t argument)
{
    if (!session.active || !session.persist) return 0;
    if (session.record_count == session.record_capacity) {
        const size_t next = session.record_capacity ? session.record_capacity * 2u : 16u;
        P4DataSessionRecord *records = realloc(session.records, next * sizeof *records);
        if (!records) return -1;
        session.records = records; session.record_capacity = next;
    }
    P4DataSessionRecord *r = session.records + session.record_count++;
    memset(r, 0, sizeof *r);
    snprintf(r->owner, sizeof r->owner, "%s", session.owner);
    r->kind = kind; r->id = id; r->learn = learn; r->skill = skill; r->argument = argument;
    if (path) snprintf(r->path, sizeof r->path, "%s", path);
    if (value) {
        r->text = value->text;
        r->number = value->number;
        if (value->text && value->string) snprintf(r->string, sizeof r->string, "%s", value->string);
    }
    return 0;
}
int p4_data_edit_set(P4DataKind kind, size_t index, const char *path, const P4DataEditValue *value,
    char *error, size_t capacity)
{
#ifdef P4_DATA_EDIT_CHECK
    check_bind();
    const uint32_t id = (uint32_t)index; /* no catalog: the scratch row */
#else
    if (!edits.running && !session.active)
        return refuse(error, capacity, "data edits run only while the game builds its tables (data.luau, data/edits.xml)");
#endif
    if (!path || !value) return refuse(error, capacity, "data edit: missing field or value");
#ifndef P4_DATA_EDIT_CHECK
    const uint32_t id = p4_data_catalog_id(kind, index);
#endif
    int result;
    switch (kind) {
    case P4_DATA_SKILL: result = skill_set(id, path, value, error, capacity); break;
    case P4_DATA_ENEMY: result = enemy_set(id, path, value, error, capacity); break;
    case P4_DATA_PERSONA: result = persona_set(id, path, value, error, capacity); break;
    case P4_DATA_ITEM: result = item_set(id, path, value, error, capacity); break;
    case P4_DATA_FLOOR: result = floor_set(id, path, value, error, capacity); break;
    case P4_DATA_FORMATION: result = formation_set(id, path, value, error, capacity); break;
    case P4_DATA_ENCOUNTERS: result = encounters_set(id, path, value, error, capacity); break;
    case P4_DATA_TREASURE: result = treasure_set(id, path, value, error, capacity); break;
    default: return refuse(error, capacity, "data edit: unknown kind");
    }
    if (!result && keep(kind, id, 0, path, value, 0, 0)) return refuse(error, capacity, "session edits: out of memory");
    return result;
}

/* ---- learnsets ---- */

/* Event layouts: a party Persona's PERSONA2 row keeps 32 {level, kind, skill}
 * events from +4, used up to the first empty kind and sorted by level (the
 * level-up scan, datPersonaPreviewLevelUp 0x0010BE60, reads them as one
 * ascending run); any other Persona's PERSONA1 row 16 {levels after the base,
 * kind, skill} events from +6. Kind 1 learns, 2 forgets. */
typedef struct Events { uint8_t *base; unsigned capacity; int party; uint32_t base_level; } Events;
static unsigned events_used(const Events *e)
{
    unsigned used = 0;
    if (e->party) { while (used < e->capacity && e->base[used * 4u + 1u]) ++used; return used; }
    for (unsigned i = 0; i < e->capacity; ++i) if (e->base[i * 4u + 1u] && get(e->base + i * 4u + 2u, 2)) used = i + 1;
    return used;
}
static int events_find(const Events *e, uint32_t skill)
{
    for (unsigned i = 0; i < e->capacity; ++i)
        if (e->base[i * 4u + 1u] == 1 && get(e->base + i * 4u + 2u, 2) == skill) return (int)i;
    return -1;
}
static void events_sort(Events *e)
{
    const unsigned used = events_used(e);
    for (unsigned i = 1; i < used; ++i)
        for (unsigned j = i; j && e->base[(j - 1) * 4u] > e->base[j * 4u]; --j) {
            uint8_t swap[4];
            memcpy(swap, e->base + (j - 1) * 4u, 4);
            memcpy(e->base + (j - 1) * 4u, e->base + j * 4u, 4);
            memcpy(e->base + j * 4u, swap, 4);
        }
}
static const char *skill_key(uint32_t skill, char *out, size_t capacity)
{
#ifdef P4_DATA_EDIT_CHECK
    const char *key = NULL;
#else
    size_t index;
    const char *key = !p4_data_catalog_index(P4_DATA_SKILL, skill, &index) ? p4_data_catalog_key(P4_DATA_SKILL, index) : NULL;
#endif
    if (key) snprintf(out, capacity, "learnset.%s", key);
    else snprintf(out, capacity, "learnset.%lu", (unsigned long)skill);
    return out;
}
int p4_data_edit_learn(size_t persona, P4DataLearnOp op, uint32_t skill, uint32_t value, char *error, size_t capacity)
{
#ifdef P4_DATA_EDIT_CHECK
    check_bind();
    const uint32_t id = (uint32_t)persona; /* no catalog: the scratch row */
#else
    if (!edits.running && !session.active)
        return refuse(error, capacity, "data edits run only while the game builds its tables (data.luau, data/edits.xml)");
    const uint32_t id = p4_data_catalog_id(P4_DATA_PERSONA, persona);
#endif
    char row[LABEL_SIZE], field[FIELD_SIZE], skill_row[LABEL_SIZE];
    label(P4_DATA_PERSONA, id, row, sizeof row);
    label(P4_DATA_SKILL, skill, skill_row, sizeof skill_row);
    if (id >= edits.view.personas) return refuse(error, capacity, "%s: outside the Persona table", row);
    Events e = {0};
    if (id >= P4_DATA_PARTY_PERSONA_FIRST && id - P4_DATA_PARTY_PERSONA_FIRST < edits.party.rows) {
        e = (Events){edits.party.base + (size_t)(id - P4_DATA_PARTY_PERSONA_FIRST) * P4_DATA_PARTY_ROW_SIZE + 4u, 32, 1, 0};
    } else {
        e = (Events){edits.persona_growth.base + (size_t)id * P4_PERSONA_GROWTH_SIZE + 6u, 16, 0,
            edits.persona.base[(size_t)id * P4_PERSONA_METADATA_SIZE + 3u]};
    }
    if (CHECKING) {
        /* The skills are catalog rows and the events the Persona's own: only the level's range is static. */
        unchecked("the learnset's skills, its current events and the Persona's base level");
        if ((op == P4_DATA_LEARN_ADD || op == P4_DATA_LEARN_MOVE) && value > 99)
            return refuse(error, capacity, "%s learnset: level %lu is outside %lu..99%s", row, (unsigned long)value,
                (unsigned long)e.base_level, e.base_level ? " (a Persona learns nothing below its base level)" : "");
        return op <= P4_DATA_LEARN_REPLACE ? 0 : refuse(error, capacity, "%s learnset: unknown change", row);
    }
    if (!skill || skill >= P4_SKILL_RETAIL_COUNT) return refuse(error, capacity, "%s learnset: %s is not a skill", row, skill_row);
    const int at = events_find(&e, skill);
    if (op != P4_DATA_LEARN_ADD && at < 0) return refuse(error, capacity, "%s learnset: it does not learn %s", row, skill_row);
    if (op == P4_DATA_LEARN_ADD && at >= 0) return refuse(error, capacity, "%s learnset: it already learns %s", row, skill_row);
    if ((op == P4_DATA_LEARN_ADD || op == P4_DATA_LEARN_MOVE) && (value > 99 || value < e.base_level))
        return refuse(error, capacity, "%s learnset: level %lu is outside %lu..99%s", row, (unsigned long)value,
            (unsigned long)e.base_level, e.base_level ? " (a Persona learns nothing below its base level)" : "");
    skill_key(skill, field, sizeof field);
    if (record(P4_DATA_PERSONA, id, field)) return refuse(error, capacity, "data edits: package memory budget exhausted");
    if (remember(e.base, e.capacity * 4u)) return refuse(error, capacity, "session edits: out of memory");
    uint8_t *event = at >= 0 ? e.base + (unsigned)at * 4u : NULL;
    switch (op) {
    case P4_DATA_LEARN_ADD: {
        unsigned slot = e.capacity;
        if (e.party) slot = events_used(&e) < e.capacity ? events_used(&e) : e.capacity;
        else for (unsigned i = 0; i < e.capacity && slot == e.capacity; ++i)
            if (!e.base[i * 4u + 1u] || !get(e.base + i * 4u + 2u, 2)) slot = i;
        if (slot == e.capacity) return refuse(error, capacity, "%s learnset: all %u learn slots are used", row, e.capacity);
        event = e.base + slot * 4u;
        event[0] = (uint8_t)(value - e.base_level); event[1] = 1; put(event + 2, 2, skill);
        break;
    }
    case P4_DATA_LEARN_REMOVE:
        if (e.party) {
            memmove(event, event + 4, (e.capacity - (unsigned)at - 1u) * 4u);
            memset(e.base + (e.capacity - 1u) * 4u, 0, 4);
        } else memset(event, 0, 4);
        break;
    case P4_DATA_LEARN_MOVE: event[0] = (uint8_t)(value - e.base_level); break;
    case P4_DATA_LEARN_REPLACE: {
        char other[LABEL_SIZE];
        if (!value || value >= P4_SKILL_RETAIL_COUNT) return refuse(error, capacity, "%s learnset: the replacement is not a skill", row);
        if (events_find(&e, value) >= 0)
            return refuse(error, capacity, "%s learnset: it already learns %s", row, label(P4_DATA_SKILL, value, other, sizeof other));
        put(event + 2, 2, value);
        break;
    }
    default: return refuse(error, capacity, "%s learnset: unknown change", row);
    }
    events_sort(&e);
    edits.touched |= 1u << P4_TABLE_INPUT_PERSONA;
    const uint32_t words[4] = {(uint32_t)P4_DATA_PERSONA, id, (uint32_t)op << 16 | skill, value};
    p4_content_digest_update(&edits.writes, words, sizeof words);
    if (keep(P4_DATA_PERSONA, id, (unsigned)op + 1u, NULL, NULL, skill, value))
        return refuse(error, capacity, "session edits: out of memory");
    return 0;
}

#ifndef P4_DATA_EDIT_CHECK
/* ---- layers ---- */

/* Cross-field rules a single write cannot check: hits.min <= hits.max and
 * an enemy's drop weights within the 200 the draw rolls against. */
static int invariants(char *error, size_t capacity)
{
    char row[LABEL_SIZE];
    for (uint32_t id = 0; id < edits.skill.rows; ++id) {
        const uint8_t *m = edits.skill.base + (size_t)id * P4_SKILL_ROW_SIZE;
        if (m[0x0F] > m[0x10])
            return refuse(error, capacity, "%s: hits.min %u is above hits.max %u", label(P4_DATA_SKILL, id, row, sizeof row),
                m[0x0F], m[0x10]);
    }
    for (uint32_t id = 0; id < edits.view.enemies; ++id) {
        const uint8_t *m = edits.enemy.base + (size_t)id * P4_ENEMY_ROW_SIZE;
        unsigned total = 0;
        for (unsigned slot = 0; slot < P4_DATA_ENEMY_DROPS; ++slot)
            if (get(m + 0x22 + slot * 4u, 2)) total += m[0x24 + slot * 4u];
        if (total > 200)
            return refuse(error, capacity, "%s: its drop chances add up to %g%%; at most 100%% can drop",
                label(P4_DATA_ENEMY, id, row, sizeof row), total / 2.0);
    }
    return 0;
}
static int read_file(const P4PackageProvider *provider, const char *path, uint64_t limit, uint8_t **bytes, size_t *size,
    char hex[P4_CONTENT_DIGEST_HEX + 1], char *error, size_t capacity)
{
    P4PackageFile *file = NULL;
    if (p4_package_file_open(provider, path, limit, &file))
        return refuse(error, capacity, "%s is unreadable or over %lu KiB", path, (unsigned long)(limit / 1024u));
    *size = (size_t)p4_package_file_size(file);
    *bytes = malloc(*size + 1u);
    int failed = !*bytes || p4_package_file_read_at(file, 0, *bytes, *size);
    if (p4_package_file_close(file)) failed = 1;
    if (failed) { free(*bytes); *bytes = NULL; return refuse(error, capacity, "%s could not be read", path); }
    (*bytes)[*size] = 0;
    P4ContentDigest digest;
    p4_content_digest_begin(&digest);
    p4_content_digest_update(&digest, *bytes, *size);
    p4_content_digest_end(&digest, hex);
    return 0;
}
/* data/edits.xml's records resolve against the catalog and write the running layer. */
static int xml_resolve(P4DataKind kind, const char *query, size_t *index, char *error, size_t capacity)
{
    P4DataRows rows;
    if (p4_data_catalog_rows(kind, &rows)) { snprintf(error, capacity, "the catalog is unavailable"); return -1; }
    return p4_data_lookup(&rows, query, index, error, capacity) ? -1 : 0;
}
static const P4DataEditsXmlSink xml_sink = {
    xml_resolve, p4_data_catalog_index, p4_data_catalog_id, p4_data_edit_set, p4_data_edit_learn
};
static int run_layer(size_t index, char *error, size_t capacity)
{
    Layer *layer = edits.layers + index;
    char reason[768] = "";
    int result = 0;
    edits.running = 1; edits.current = index;
    p4_content_digest_begin(&edits.writes);
    if (layer->xml) {
        uint8_t *bytes = NULL;
        size_t size = 0;
        result = read_file(layer->provider, P4_DATA_EDIT_XML, P4_DATA_EDIT_XML_MAX, &bytes, &size, layer->xml_hex,
            reason, sizeof reason);
        if (!result) result = p4_data_edits_xml_decode(bytes, size, &xml_sink, reason, sizeof reason);
        free(bytes);
        if (result) refuse(error, capacity, "%s: %s", P4_DATA_EDIT_XML, reason);
    }
    if (!result && layer->luau) {
        uint8_t *bytes = NULL;
        size_t size = 0;
        result = read_file(layer->provider, P4_DATA_EDIT_LUAU, P4_DATA_EDIT_LUAU_MAX, &bytes, &size, layer->luau_hex,
            reason, sizeof reason);
        if (!result) result = edits.runner(layer->provider, (const char *)bytes, size, reason, sizeof reason);
        free(bytes);
        if (result) refuse(error, capacity, "%s", reason);
    }
    if (!result && invariants(reason, sizeof reason)) result = refuse(error, capacity, "%s", reason);
    p4_content_digest_end(&edits.writes, layer->writes_hex);
    edits.running = 0;
    return result;
}
static int apply(P4GameTableInput *const *inputs, size_t count,
    void (*composed)(const P4GameTableInput *input), const P4PackageProvider **failed_provider)
{
    if (failed_provider) *failed_provider = NULL;
    if (!edits.count) return 0;
    char error[1024];
    const P4PackageProvider *failed = NULL;
    const char *why = NULL;
    if (p4_data_catalog_capture()) {
        p4_data_catalog_ready(&why);
        failed = edits.layers[0].provider;
        snprintf(error, sizeof error, "p4.data: %s", why ? why : "the catalog is unavailable");
    } else if (bind_all(inputs, count)) {
        failed = edits.layers[0].provider;
        snprintf(error, sizeof error, "p4.data: the game tables could not be prepared for edits");
    } else {
        edits.touched = 0;
        p4_data_catalog_seal_view(&edits.view);
        for (size_t i = 0; i < edits.count && !failed; ++i)
            if (run_layer(i, error, sizeof error)) failed = edits.layers[i].provider;
        p4_data_catalog_seal_view(NULL);
    }
    if (failed) {
        fprintf(stderr, "p4pc: data edits refused: %s\n", error);
        p4_package_provider_refuse(failed, error);
        if (failed_provider) *failed_provider = failed;
        return -1;
    }
    for (unsigned kind = 0; kind < P4_TABLE_INPUT_COUNT; ++kind)
        if ((edits.touched & (1u << kind)) && edits.inputs[kind]) composed(edits.inputs[kind]);
    return 0;
}

static void identity(P4ContentDigest *digest)
{
    static const char tag[] = "p4pc-data-edits-v1";
    p4_content_digest_update(digest, tag, sizeof tag);
    for (size_t i = 0; i < edits.count; ++i) {
        const Layer *layer = edits.layers + i;
        const P4PackageInfo *info = p4_package_provider_info(layer->provider);
        p4_content_digest_update(digest, layer->owner, strlen(layer->owner) + 1);
        p4_content_digest_update(digest, info ? info->version : "", strlen(info ? info->version : "") + 1);
        p4_content_digest_update(digest, layer->xml_hex, sizeof layer->xml_hex);
        p4_content_digest_update(digest, layer->luau_hex, sizeof layer->luau_hex);
        p4_content_digest_update(digest, layer->writes_hex, sizeof layer->writes_hex);
    }
}
static void reset(void)
{
    if (edits.capacity) p4_packages_memory_release(edits.capacity * sizeof *edits.layers);
    if (edits.entry_capacity) p4_packages_memory_release(edits.entry_capacity * sizeof *edits.entries);
    free(edits.layers);
    free(edits.entries);
    const P4DataEditRunner runner = edits.runner;
    memset(&edits, 0, sizeof edits);
    edits.runner = runner;
}
static size_t layer_count(void) { return edits.count; }
static const P4TableDerivedLayers hooks = {layer_count, apply, identity, reset};
void p4_data_edits_discard(void) { reset(); }
int p4_data_edits_prepare(void)
{
    reset(); /* a fresh admission pass */
    for (size_t i = 0, count = p4_packages_count(); i < count; ++i) {
        const P4PackageProvider *provider = p4_packages_provider(i);
        char error[240];
        uint32_t inputs = 0;
        const int selected = select_layer(provider, &inputs, error, sizeof error);
        if (!selected) continue;
        P4GameTablesDefinitionBuilder *builder = selected > 0 ? p4_game_tables_boot_builder() : NULL;
        if (selected > 0 && !builder) snprintf(error, sizeof error, "data edits: the game tables were already sealed");
        for (unsigned kind = 0; builder && kind < P4_TABLE_INPUT_COUNT; ++kind)
            if ((inputs & (1u << kind)) && p4_game_tables_builder_request_input(builder, (P4GameTableInputKind)kind)) {
                snprintf(error, sizeof error, "data edits: the game tables could not be requested");
                builder = NULL;
            }
        if (!builder) {
            fprintf(stderr, "p4pc: data edits refused: %s\n", error);
            reset();
            p4_package_provider_refuse(provider, error);
            return -1;
        }
        if (p4_package_provider_claim(provider, P4_PACKAGE_CLAIM_TABLES | P4_PACKAGE_CLAIM_SEMANTIC)) { reset(); return -1; }
    }
    if (edits.count) p4_game_tables_derived_layers(&hooks);
    return 0;
}

size_t p4_data_edits_conflict_count(void)
{
    size_t count = 0;
    for (size_t i = 0; i < edits.entry_count; ++i) count += edits.entries[i].owner_count > 1;
    return count;
}
int p4_data_edits_conflict_at(size_t index, P4DataConflict *out)
{
    for (size_t i = 0; i < edits.entry_count; ++i) {
        const Entry *entry = edits.entries + i;
        if (entry->owner_count < 2 || index--) continue;
        memset(out, 0, sizeof *out);
        out->kind = entry->kind; out->id = entry->id; out->field = entry->field;
        out->owner_count = entry->owner_count;
        for (size_t o = 0; o < entry->owner_count; ++o) out->owners[o] = edits.layers[entry->owners[o]].owner;
        return 0;
    }
    return -1;
}

/* ---- session edits ---- */

/* The loaded banks, written in place (guest memory the game reads). */
static uint8_t *mutable_bytes(const uint8_t *bytes) { return (uint8_t *)(uintptr_t)bytes; }
static int bind_live(void)
{
    P4DataBanks v;
    if (p4_data_catalog_live_banks(&v)) return -1;
    edits.skill_element = (Bank){mutable_bytes(v.skill_element), v.skill_elements};
    edits.skill = (Bank){mutable_bytes(v.skill), v.skills};
    edits.skill_name = (Bank){mutable_bytes(v.skill_name), v.skill_names};
    edits.enemy = (Bank){mutable_bytes(v.enemy), v.enemies};
    edits.enemy_affinity = (Bank){mutable_bytes(v.enemy_affinity), v.enemies};
    edits.enemy_name = (Bank){mutable_bytes(v.enemy_name), v.enemy_names};
    edits.persona = (Bank){mutable_bytes(v.persona), v.personas};
    edits.persona_growth = (Bank){mutable_bytes(v.persona_growth), v.personas};
    edits.party = (Bank){mutable_bytes(v.party), v.parties};
    edits.persona_affinity = (Bank){mutable_bytes(v.persona_affinity), v.personas};
    edits.persona_name = (Bank){mutable_bytes(v.persona_name), v.persona_names};
    edits.item = (Bank){mutable_bytes(v.item), v.items};
    edits.floor = (Bank){mutable_bytes(v.floor), v.floors};
    edits.formation = (Bank){mutable_bytes(v.formation), v.formations};
    edits.encounters = (Bank){mutable_bytes(v.encounters), v.encounter_groups};
    edits.treasure = (Bank){mutable_bytes(v.treasure), v.treasures};
    edits.view = v;
    edits.encount = P4_TABLE_INPUT_COUNT; /* no input: the loaded rows themselves */
    return 0;
}
static void undo_to(size_t mark, size_t saved_mark)
{
    while (session.undo_count > mark) {
        const Undo *undo = session.undo + --session.undo_count;
        memcpy(undo->at, session.saved + undo->offset, undo->size);
    }
    session.saved_used = saved_mark;
}
int p4_data_session_begin(const char *owner, int persist, char *error, size_t capacity)
{
    if (session.active || edits.running) return refuse(error, capacity, "session edits: an edit is already running");
    const char *why = NULL;
    if (p4_data_catalog_ready(&why)) return refuse(error, capacity, "p4.data.session: %s", why);
    if (bind_live()) return refuse(error, capacity, "p4.data.session: the loaded tables are not available");
    session.active = 1;
    session.persist = persist != 0;
    snprintf(session.owner, sizeof session.owner, "%s", owner ? owner : "");
    session.mark = session.undo_count;
    session.saved_mark = session.saved_used;
    session.record_mark = session.record_count;
    p4_content_digest_begin(&edits.writes);
    return 0;
}
int p4_data_session_end(int keep_writes, char *error, size_t capacity)
{
    if (!session.active) return refuse(error, capacity, "session edits: none is running");
    char reason[768];
    int result = 0;
    if (keep_writes && invariants(reason, sizeof reason)) result = refuse(error, capacity, "%s", reason);
    if (!keep_writes || result) {
        undo_to(session.mark, session.saved_mark);
        session.record_count = session.record_mark;
    }
    session.active = 0;
    return keep_writes ? result : 0;
}
size_t p4_data_session_count(void) { return session.undo_count; }
size_t p4_data_session_persisted(void) { return session.record_count; }
void p4_data_session_revert(void)
{
    undo_to(0, 0);
}
size_t p4_data_session_record_count(void) { return session.record_count; }
const P4DataSessionRecord *p4_data_session_record_at(size_t index)
{
    return index < session.record_count ? session.records + index : NULL;
}
void p4_data_session_records_clear(void) { session.record_count = 0; }
#else
unsigned p4_data_edit_check_unchecked(char *first, size_t capacity)
{
    const unsigned count = unchecked_count;
    if (first && capacity) snprintf(first, capacity, "%s", count ? unchecked_first : "");
    unchecked_count = 0;
    unchecked_first[0] = 0;
    return count;
}
#endif
