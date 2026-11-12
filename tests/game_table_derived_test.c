/* [table-derived] over real package boot and synthetic disc inputs: producer
 * selection, base spans, row/span composition before any family reads them,
 * refusals, the admission rewrite of a loader buffer that still holds the
 * selected original, and the relocations of the PERSONA4 skill pairs and the
 * grown ENCOUNT blocks: the loader keeps the original blocks and battle-table
 * completion moves their slots to the new rows in guest memory. [table-delta]
 * deltas patch named fields after the producers, with or without one. */
#include "game_table_derived_internal.h"
#include "p4pc/game_table_admission_native.h"
#include "p4pc/game_table_boot.h"
#include "p4pc/game_table_derived.h"
#include "p4pc/game_tables_native.h"
#include "p4pc/game_damage_cover.h"
#include "p4pc/game_elements.h"
#include "p4pc/game_damage_cover.h"
#include "p4pc/game_side_ailment.h"
#include "p4pc/game_items_native.h"
#include "p4pc/game_encounters.h"
#include "p4pc/game_personas.h"
#include "p4pc/game_personas_save.h"
#include "p4pc/game_skills.h"
#include "p4pc/game_skills_save.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/packages.h"
#include "guest_test.h"
#include "package_boot_internal.h"
#include "p4pc/package_memory.h"
#include "p4pc/mod_setting_store.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const P4PackageHandler p4_game_tables_sources_boot_provider;
static const char *mode;
static unsigned produces, prepares, publishes, receipts, allocations, releases;
static uint8_t skill_rows[2][40], skill_elements[576 * 2], item_row[68], delta_row[40];
static const P4GameTablesBootProduct *product;
/* Golden's list length: 107 pairs and the (0, 0) terminator. */
#define PAIRS 108u
#define PAIR_SLOT 0x007644dcu
static uint8_t pair_list[(P4_PERSONA_SKILL_PAIR_MAX + 1u) * P4_PERSONA_SKILL_PAIR_SIZE];
/* Guest RAM for the loader's arena and, from HEAP on, the table heap. */
#define ARENA 0x01000000u
#define ARENA_SIZE 0x20000u
#define HEAP (ARENA + 0x10000u)
static uint32_t heap_next = HEAP;
static int native_phase;

_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{ fprintf(stderr, "Unexpected derived boot recovery: %s\n", reason); abort(); }
_Noreturn void p4_package_boot_source_failed(const P4PackageProvider *provider, const char *reason)
{ (void)provider; fprintf(stderr, "Unexpected derived origin recovery: %s\n", reason); abort(); }
/* CPU preparation never touches the guest heap; battle-table completion may. */
void *p4pc_heap_malloc(size_t size)
{
    if (!native_phase) { fputs("CPU derived preparation touched guest heap\n", stderr); abort(); }
    const uint32_t guest = heap_next;
    heap_next += ((uint32_t)size + 15u) & ~15u;
    assert(heap_next <= ARENA + ARENA_SIZE);
    ++allocations;
    return P4_G2H(guest);
}
void p4pc_heap_free(void *host)
{
    if (!native_phase) { fputs("CPU derived preparation released guest heap\n", stderr); abort(); }
    assert(host);
    ++releases;
}
int p4_game_tables_items_source_stage(const P4GameItems *items)
{ (void)items; fputs("Derived-only boot staged items\n", stderr); abort(); }
int p4_game_tables_items_boot_stage(P4GameItems *items)
{ (void)items; fputs("Derived-only boot staged items\n", stderr); abort(); }
int p4_game_tables_items_boot_unstage(P4GameItems *items)
{ (void)items; fputs("Unowned item raw-hook rollback\n", stderr); abort(); }
/* These unrelated native paths must not be used by this fixture. */
uint32_t RpRandom(void) { abort(); }
void p4_game_tables_text_draw(uint32_t stream, float x, float y, float depth,
    uint32_t color, uint32_t style, uint32_t effect, uint32_t flags, uint32_t multiline)
{
    (void)stream; (void)x; (void)y; (void)depth; (void)color; (void)style;
    (void)effect; (void)flags; (void)multiline; abort();
}
int p4_game_enemies_boot_stage(P4GameEnemies *enemies)
{ (void)enemies; fputs("Derived-only boot staged enemies\n", stderr); abort(); }
int p4_game_enemies_boot_unstage(P4GameEnemies *enemies)
{ (void)enemies; fputs("Unowned enemy raw-hook rollback\n", stderr); abort(); }
int p4_game_personas_boot_stage(P4GamePersonas *personas)
{ (void)personas; fputs("Derived-only boot staged personas\n", stderr); abort(); }
int p4_game_personas_boot_unstage(P4GamePersonas *personas)
{ (void)personas; fputs("Unowned persona raw-hook rollback\n", stderr); abort(); }
int p4_game_personas_save_register(const P4GamePersonas *personas, const P4GameTableSources *sources)
{ (void)personas; (void)sources; fputs("Derived-only boot registered a persona save block\n", stderr); abort(); }
int p4_game_personas_save_unregister(void) { return P4_TABLE_OK; }
int p4_game_skills_save_register(const P4GameSkills *skills, const P4GameTableSources *sources)
{ (void)skills; (void)sources; fputs("Derived-only boot registered a skill save block\n", stderr); abort(); }
int p4_game_skills_save_unregister(void) { return P4_TABLE_OK; }
int p4_game_elements_boot_stage(const P4GameTables *tables, const P4GameSkills *skills, const P4GameItems *items)
{ (void)tables; (void)skills; (void)items; fputs("Derived-only boot staged elements\n", stderr); abort(); }
/* No fixture source declares damage cover; publication finds no plan. */
int p4_game_damage_cover_prepare(const P4GameTables *tables, P4DamageCoverLoadOrderFn load_order)
{ (void)tables; (void)load_order; fputs("Unrequested damage cover plan\n", stderr); abort(); }
void p4_game_damage_cover_publish(void) { }
void p4_game_damage_cover_session_end(void) { }
/* Nor side ailments. */
int p4_game_side_ailment_prepare(const P4GameTables *tables)
{ (void)tables; fputs("Unrequested side-ailment plan\n", stderr); abort(); }
void p4_game_side_ailment_publish(void) { }
void p4_game_side_ailment_session_end(void) { }

static int is(const char *name) { return !strcmp(mode, name); }
static int pairs_mode(void) { return !strncmp(mode, "pairs", 5); }
static int encount_mode(void) { return !strncmp(mode, "encount", 7); }
static int delta_mode(void) { return !strncmp(mode, "delta", 5); }
static int combined_mode(void) { return !strncmp(mode, "delta-combined-", 15); }
static int combined_on(void) { return is("delta-combined-retail-on") || is("delta-combined-produced-on"); }
static int percent_mode(void) { return is("delta-percent-on") || is("delta-percent-off") || combined_mode(); }
static int percent_on(void) { return is("delta-percent-on") || combined_on(); }
static int rider_mode(void) { return !strncmp(mode, "delta-rider-", 12) || combined_mode(); }
static int rider_produced(void) { return is("delta-rider-produced-on") || is("delta-rider-produced-off") ||
    is("delta-combined-produced-on") || is("delta-combined-produced-off"); }
static int rider_on(void) { return is("delta-rider-retail-on") || is("delta-rider-produced-on") || combined_on(); }
static const uint32_t rider_ids[] = {0x99, 0x9a, 0x9c, 0x9e, 0xa0, 0xa4, 0xa6};
/* Match the synthetic input, or the distinct producer replacement. */
static void rider_row(uint8_t row[40], uint32_t id, int produced)
{
    for (unsigned i = 0; i < 40; ++i) row[i] = (uint8_t)(id + i + (produced ? 17 : 0));
    row[4] = 9; row[5] = 0; row[0x11] = 1;
    row[0x12] = 123; row[0x13] = 0;
    row[0x18] = 1; row[0x19] = produced ? 35 : 20;
    row[0x1c] = 0x10; row[0x1d] = row[0x1e] = row[0x1f] = 0;
}
/* The synthetic disc's ENCOUNT rows (game_table_derived_test.py) and the grown
 * ones each encount mode pushes: two more formations, one more selector,
 * weighted group and record group. */
enum { FORMATIONS = 4, SELECTORS = 2, WEIGHTED = 2, RECORDS = 2 };
static const uint32_t encount_spans[4] = {P4_ENCOUNT_FORMATION_SPAN, P4_ENCOUNT_SELECTOR_SPAN,
    P4_ENCOUNT_WEIGHTED_SPAN, P4_ENCOUNT_RECORD_SPAN};
static const uint32_t encount_strides[4] = {P4_ENCOUNT_FORMATION_SIZE, P4_ENCOUNT_SELECTOR_SIZE,
    P4_ENCOUNT_WEIGHTED_SIZE, P4_ENCOUNT_RECORD_SIZE};
static const uint32_t encount_loaded[4] = {FORMATIONS, SELECTORS, WEIGHTED, RECORDS};
static const uint32_t encount_slots[5] = {0x764504, 0x76450c, 0x764510, 0x764508, 0x764514};
static uint8_t encount_rows[4][(RECORDS + 1) * P4_ENCOUNT_RECORD_SIZE];
static size_t encount_sizes[4];
static void put_half(uint8_t *at, uint32_t value) { at[0] = (uint8_t)value; at[1] = (uint8_t)(value >> 8); }
static void put_pair(size_t pair, uint32_t original, uint32_t replacement)
{
    uint8_t *at = pair_list + pair * P4_PERSONA_SKILL_PAIR_SIZE;
    at[0] = (uint8_t)original; at[1] = (uint8_t)(original >> 8);
    at[2] = (uint8_t)replacement; at[3] = (uint8_t)(replacement >> 8);
}
/* A longer PERSONA4 list than the selected original; refusal modes break one
 * rule each: an open end, a zero half before the end, a skill changed twice,
 * too many pairs, or a skill no package declares. */
static int produce_pairs(const P4TableDerivedRequest *request, P4TableDerivedSink *sink)
{
    const uint8_t *bytes = NULL;
    size_t size = 0;
    /* The selected original: the synthetic disc's 16-byte block. */
    assert(!p4_table_derived_base(request, P4_TABLE_INPUT_PERSONA, P4_PERSONA_SKILL_PAIR_SPAN, &bytes, &size));
    assert(size == 16u);
    const size_t pairs = is("pairs-long") ? P4_PERSONA_SKILL_PAIR_MAX + 1u : PAIRS;
    memset(pair_list, 0, sizeof pair_list);
    for (size_t pair = 0; pair + 1 < pairs; ++pair) put_pair(pair, 1u + (uint32_t)pair, 200u + (uint32_t)pair);
    if (is("pairs-open")) put_pair(pairs - 1, 7, 8);
    if (is("pairs-gap")) put_pair(3, 0, 9);
    if (is("pairs-repeat")) put_pair(9, 3, 300);
    if (is("pairs-skill")) put_pair(5, 6, 600);
    const P4TableDerivedSpan span = {P4_TABLE_INPUT_PERSONA, P4_PERSONA_SKILL_PAIR_SPAN,
        pairs * P4_PERSONA_SKILL_PAIR_SIZE, pair_list};
    const int refused = is("pairs-open") || is("pairs-gap") || is("pairs-repeat") || is("pairs-long");
    assert(p4_table_derived_sink_span(sink, &span) == (refused ? -1 : 0));
    if (refused) return 0;
    static const uint8_t digest[P4_TABLE_DERIVED_DIGEST_BYTES] = {5, 6, 7, 8};
    return p4_table_derived_sink_fingerprint(sink, "synthetic install", digest);
}
/* Grown ENCOUNT blocks over the synthetic originals. Refusal modes break one
 * rule each: fewer rows than loaded, a partial row, a selector or weighted
 * entry naming a missing row, a step whose record group changes treasure
 * kind, or an enemy or item no package declares. */
static int produce_encount(const P4TableDerivedRequest *request, P4TableDerivedSink *sink)
{
    for (unsigned block = 0; block < 4; ++block) {
        const uint8_t *bytes = NULL;
        size_t size = 0;
        assert(!p4_table_derived_base(request, P4_TABLE_INPUT_ENCOUNT, encount_spans[block], &bytes, &size));
        assert(size == encount_loaded[block] * encount_strides[block]);
        memset(encount_rows[block], 0, sizeof encount_rows[block]);
        memcpy(encount_rows[block], bytes, size);
        encount_sizes[block] = size + (block ? 1u : 2u) * encount_strides[block];
    }
    uint8_t *formation = encount_rows[0] + FORMATIONS * P4_ENCOUNT_FORMATION_SIZE;
    put_half(formation + P4_ENCOUNT_FORMATION_ENEMIES, is("encount-enemy") ? 400u : 3u);
    put_half(formation + P4_ENCOUNT_FORMATION_SIZE + P4_ENCOUNT_FORMATION_ENEMIES, 335u);
    uint8_t *selector = encount_rows[1] + SELECTORS * P4_ENCOUNT_SELECTOR_SIZE;
    put_half(selector + P4_ENCOUNT_SELECTOR_WEIGHTED, is("encount-selector") ? WEIGHTED + 1u : WEIGHTED);
    put_half(selector + P4_ENCOUNT_SELECTOR_RECORDS, RECORDS);
    uint8_t *weighted = encount_rows[2] + WEIGHTED * P4_ENCOUNT_WEIGHTED_SIZE + P4_ENCOUNT_WEIGHTED_HEADER;
    put_half(weighted, is("encount-formation") ? FORMATIONS + 2u : FORMATIONS + 1u);
    put_half(weighted + 2, 10);
    uint8_t *record = encount_rows[3] + RECORDS * P4_ENCOUNT_RECORD_SIZE;
    put_half(record, 5);
    put_half(record + P4_ENCOUNT_RECORD_ITEM, is("encount-item") ? 1600u : 1022u);
    /* Selector 1's group 1 yielded only kind 0 treasure; now only kind 1. */
    if (is("encount-kind")) encount_rows[3][1u * P4_ENCOUNT_RECORD_SIZE + P4_ENCOUNT_RECORD_FLAGS] = 1;
    if (is("encount-short")) encount_sizes[0] = (FORMATIONS - 1u) * P4_ENCOUNT_FORMATION_SIZE;
    if (is("encount-partial")) encount_sizes[0] += 1u;
    const int refused = is("encount-short") || is("encount-partial");
    for (unsigned block = 0; block < 4; ++block) {
        const P4TableDerivedSpan span = {P4_TABLE_INPUT_ENCOUNT, encount_spans[block], encount_sizes[block],
            encount_rows[block]};
        assert(p4_table_derived_sink_span(sink, &span) == (refused && !block ? -1 : 0));
        if (refused) return 0;
    }
    static const uint8_t digest[P4_TABLE_DERIVED_DIGEST_BYTES] = {9, 10, 11, 12};
    return p4_table_derived_sink_fingerprint(sink, "synthetic install", digest);
}
static int produce(void *user, const P4TableDerivedRequest *request, P4TableDerivedSink *sink, char *error, size_t capacity)
{
    assert(user == &produces && request && request->provider && !strcmp(request->section_key, "golden"));
    ++produces;
    const uint8_t *bytes = NULL;
    size_t size = 0;
    /* Undeclared or missing spans are not readable. */
    assert(p4_table_derived_base(request, P4_TABLE_INPUT_UNIT, 0, &bytes, &size) == -1);
    assert(p4_table_derived_base(request, P4_TABLE_INPUT_SKILL, 2, &bytes, &size) == -1);
    assert(!p4_table_derived_base(request, P4_TABLE_INPUT_SKILL, 1, &bytes, &size) && size == 440u * 40u);
    assert(bytes[0] == 2 && bytes[11 * 40 + 0x1e] == 8);
    memcpy(skill_rows[0], bytes + 11 * 40, 40);
    memcpy(skill_rows[1], bytes + 12 * 40, 40);
    memcpy(delta_row, bytes + 2 * 40, 40); /* the row only the delta package changes */
    assert(!p4_table_derived_base(request, P4_TABLE_INPUT_SKILL, 0, &bytes, &size) && size == sizeof skill_elements);
    memcpy(skill_elements, bytes, size);
    assert(!p4_table_derived_base(request, P4_TABLE_INPUT_ITEM, 0, &bytes, &size) && size == 16u + 1536u * 68u);
    assert(bytes[1] == 6);
    memcpy(item_row, bytes + 16 + 7 * 68, 68);
    assert(!p4_table_derived_base(request, P4_TABLE_INPUT_MSG, 1, &bytes, &size) && size == 576u * 19u);
    assert(!bytes[3 * 19]);
    if (pairs_mode()) return produce_pairs(request, sink);
    if (encount_mode()) return produce_encount(request, sink);
    if (is("absent")) { snprintf(error, capacity, "no install found"); return 1; }
    if (is("no-fingerprint")) return 0;
    uint8_t rows[2][40];
    memset(rows, 0x5a, sizeof rows);
    /* The delta modes' row 12 is a formula-damage skill that rolls its own
     * hits (the disc's row is kind 0) and one ailment (status mode 1, mask
     * 0x10); row 11 keeps flags00 bit 1, a weapon skill's. Hit and
     * ailment-rate deltas are checked against these composed rows. */
    if (delta_mode()) {
        rows[1][0] = 0; rows[1][0x11] = 1; rows[1][0x18] = 1;
        rows[1][0x1c] = 0x10; rows[1][0x1d] = rows[1][0x1e] = rows[1][0x1f] = 0;
    }
    uint32_t ids[2] = {11, 12};
    if (is("order")) { ids[0] = 12; ids[1] = 11; }
    P4TableDerivedRows skill = {P4_TABLE_INPUT_SKILL, 1, is("stride") ? 44u : 40u, 2, ids, rows[0]};
    if (is("undeclared")) skill.input = P4_TABLE_INPUT_UNIT;
    if (is("past-end")) ids[1] = 440;
    const int refused = is("order") || is("stride") || is("undeclared") || is("past-end");
    assert(p4_table_derived_sink_rows(sink, &skill) == (refused ? -1 : 0));
    if (refused) {
        /* A refused sink stays refused: nothing more is accepted. */
        assert(p4_table_derived_sink_rows(sink, &skill) == -1);
        return 0;
    }
    if (percent_mode()) {
        /* A producer turns the synthetic disc's non-heal C0 into a heal.
         * max-heal must validate and patch this produced row, not the disc. */
        uint8_t heal[40];
        for (unsigned i = 0; i < sizeof heal; ++i) heal[i] = (uint8_t)(0xc0 + i);
        heal[4] = 3; heal[5] = 0; heal[0x11] = 2; heal[0x12] = 55; heal[0x13] = 0;
        const uint32_t heal_id = 0xc0;
        const P4TableDerivedRows produced_heal = {P4_TABLE_INPUT_SKILL, 1, 40, 1, &heal_id, heal};
        assert(!p4_table_derived_sink_rows(sink, &produced_heal));
    }
    if (rider_produced()) {
        uint8_t riders[7][40];
        for (unsigned i = 0; i < 7; ++i) rider_row(riders[i], rider_ids[i], 1);
        const P4TableDerivedRows produced_riders = {P4_TABLE_INPUT_SKILL, 1, 40, 7, rider_ids, riders[0]};
        assert(!p4_table_derived_sink_rows(sink, &produced_riders));
    }
    uint8_t elements[576 * 2];
    memcpy(elements, skill_elements, sizeof elements);
    elements[20 * 2] = 9;
    const P4TableDerivedSpan span = {P4_TABLE_INPUT_SKILL, 0, is("length") ? sizeof elements - 2u : sizeof elements, elements};
    assert(p4_table_derived_sink_span(sink, &span) == (is("length") ? -1 : 0));
    if (is("length")) return 0;
    uint8_t item[68];
    memset(item, 0xa7, sizeof item);
    const uint32_t item_id = 7, name_id = 3;
    const P4TableDerivedRows items = {P4_TABLE_INPUT_ITEM, 0, 68, 1, &item_id, item};
    assert(!p4_table_derived_sink_rows(sink, &items));
    uint8_t name[19] = "AGILAO";
    const P4TableDerivedRows names = {P4_TABLE_INPUT_MSG, is("no-stride") ? 0u : 1u, 19, 1, &name_id, name};
    /* MSG 0 has no admitted row stride: such a span is replaced whole. */
    assert(p4_table_derived_sink_rows(sink, &names) == (is("no-stride") ? -1 : 0));
    if (is("no-stride")) return 0;
    if (!strncmp(mode, "names", 5)) {
        /* Retail item names live in the executable; the producer names them
         * through the item-name channel. */
        uint32_t name_ids[2] = {7, 9};
        const char *titles[2] = {"Golden Tonic", "Moon Stone"};
        if (is("names-order")) name_ids[1] = 7;
        if (is("names-text")) titles[1] = "Moon\tStone";
        assert(p4_table_derived_sink_item_names(sink, name_ids, titles, 2) == (is("names") ? 0 : -1));
        if (!is("names")) return 0;
    }
    static const uint8_t digest[P4_TABLE_DERIVED_DIGEST_BYTES] = {1, 2, 3, 4};
    if (is("bad-label")) {
        assert(p4_table_derived_sink_fingerprint(sink, "", digest) == -1);
        return 0;
    }
    if (is("source") || is("source-oversize")) {
        static uint8_t raw[(4u << 20) + 1];
        const uint8_t *seen = NULL;
        size_t seen_size = 0;
        for (size_t i = 0; i < 64; ++i) raw[i] = (uint8_t)(0x40 + i);
        /* Nothing is exposed while the producer runs. */
        assert(p4_table_derived_source("test:golden", "init/cmpTable.bin", &seen, &seen_size) == -1);
        if (is("source-oversize")) {
            assert(p4_table_derived_sink_source(sink, "init/cmpTable.bin", raw, sizeof raw) == -1);
            return 0;
        }
        assert(!p4_table_derived_sink_source(sink, "init/cmpTable.bin", raw, 64));
    }
    return p4_table_derived_sink_fingerprint(sink, "synthetic install", digest);
}
static const P4TableDerivedProducer producer = {"test:golden", 3, produce, &produces};

static const P4GameTableInput *input_of(P4GameTableInputKind kind)
{
    for (size_t i = 0; i < product->input_count; ++i)
        if (p4_game_table_input_kind(product->inputs[i]) == kind) return product->inputs[i];
    return NULL;
}
static const uint8_t *row(P4GameTableInputKind kind, uint32_t span, uint32_t header, uint32_t stride, uint32_t id)
{
    return p4_game_table_input_span_bytes(p4_game_table_input_span(input_of(kind), span)) + header + (size_t)id * stride;
}
/* A party row's 32 learn events: the listed {level, skill} pairs (kind 1)
 * in order, then empty events. */
static void expect_learns(uint32_t party_row, const uint8_t (*learned)[2], unsigned count)
{
    const uint8_t *events = row(P4_TABLE_INPUT_PERSONA, 2, 0, 622, party_row) + 4;
    for (unsigned i = 0; i < 32; ++i) {
        const uint8_t *event = events + i * 4u;
        if (i < count) assert(event[0] == learned[i][0] && event[1] == 1 && event[2] == learned[i][1] && !event[3]);
        else assert(!event[0] && !event[1] && !event[2] && !event[3]);
    }
}
static int prepare(void *user)
{
    (void)user; ++prepares;
    const void *found = (const void *)1;
    assert(!p4_package_boot_product(P4_GAME_TABLES_BOOT_PROVIDER, &found));
    product = found;
    if (is("gated-off")) { assert(!product && !produces); return 0; }
    if (is("drain")) {
        /* Export the actual composed row, not an XML echo, for the production
         * amount-equation fixture. The path belongs to the synthetic case. */
        const char *path = getenv("P4PC_TEST_DRAIN_ROW");
        assert(product && produces == 1 && path && *path);
        FILE *output = fopen(path, "wb");
        assert(output);
        assert(fwrite(row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 0x65), 1, 40, output) == 40);
        assert(!fclose(output));
        return 0;
    }
    if (is("delta-only")) {
        /* Deltas compose without a producer; no ITEM delta, so no ITEM input. */
        assert(product && !produces && !p4_game_tables_derived_count() && p4_game_tables_derived_delta_count() == 2);
        assert(product->input_count == P4_TABLE_INPUT_COUNT - 1u && !input_of(P4_TABLE_INPUT_ITEM));
        const uint8_t *skill = row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 11);
        assert(skill[0x12] == 80 && !skill[0x13] && skill[0x1e] == 8 && !skill[0x04]);
        static const uint8_t c0[][2] = {{0, 10}, {1, 11}, {9, 12}};
        expect_learns(0, c0, 3);
        return 0;
    }
    if (is("delta-cost-bounds")) {
        assert(product && !produces && !p4_game_tables_derived_count() && p4_game_tables_derived_delta_count() == 2);
        const uint8_t *low = row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 0xc2);
        const uint8_t *high = row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 0xc5);
        uint8_t expected_low[40] = {0}, expected_high[40] = {0};
        expected_low[4] = 1;
        expected_high[4] = 0xe7; expected_high[5] = 0x03; /* 999, little-endian */
        assert(!memcmp(low, expected_low, sizeof expected_low));
        assert(!memcmp(high, expected_high, sizeof expected_high));
        return 0;
    }
    if (is("delta-percent-bounds")) {
        assert(product && !produces && !p4_game_tables_derived_count() && p4_game_tables_derived_delta_count() == 2);
        static const uint32_t ids[] = {0xc1, 0xc4};
        static const uint8_t costs[] = {6, 12}, percentages[] = {1, 100};
        for (unsigned n = 0; n < 2; ++n) {
            uint8_t heal[40];
            for (unsigned i = 0; i < sizeof heal; ++i) heal[i] = (uint8_t)(ids[n] + i);
            heal[4] = costs[n]; heal[5] = 0; heal[0x11] = 11; heal[0x12] = percentages[n]; heal[0x13] = 0;
            assert(!memcmp(row(P4_TABLE_INPUT_SKILL, 1, 0, 40, ids[n]), heal, sizeof heal));
            assert(!p4_game_tables_skill_max_heal(ids[n]));
        }
        return 0;
    }
    assert(product && produces == 1 && p4_game_tables_derived_count() == 1);
    assert(!product->skills && !product->items && !product->enemies && !product->personas);
    /* Every packed input is authenticated once one is declared. */
    assert(product->input_count == P4_TABLE_INPUT_COUNT);
    const P4GameTableInput *persona = input_of(P4_TABLE_INPUT_PERSONA);
    const P4GameTableInputSpan *pairs = p4_game_table_input_span(persona, P4_PERSONA_SKILL_PAIR_SPAN);
    static const uint8_t original_pairs[16];
    /* The loader keeps loading the selected original block. */
    assert(p4_game_table_input_span_size(pairs) == 16u &&
        !memcmp(p4_game_table_input_span_bytes(pairs), original_pairs, sizeof original_pairs));
    const P4GameTableInput *loaded = NULL;
    const uint8_t *relocated = NULL;
    size_t size = 0;
    if (is("pairs")) {
        assert(p4_game_tables_derived_relocated(P4_TABLE_INPUT_PERSONA, P4_PERSONA_SKILL_PAIR_SPAN, &loaded, &relocated, &size));
        assert(loaded == persona && size == PAIRS * P4_PERSONA_SKILL_PAIR_SIZE && !memcmp(relocated, pair_list, size));
        return 0;
    }
    if (is("encount")) {
        const P4GameTableInput *encount = input_of(P4_TABLE_INPUT_ENCOUNT);
        for (unsigned block = 0; block < 4; ++block) {
            const P4GameTableInputSpan *span = p4_game_table_input_span(encount, encount_spans[block]);
            assert(p4_game_table_input_span_size(span) == encount_loaded[block] * encount_strides[block]);
            assert(p4_game_tables_derived_relocated(P4_TABLE_INPUT_ENCOUNT, encount_spans[block], &loaded, &relocated, &size));
            assert(loaded == encount && size == encount_sizes[block] && !memcmp(relocated, encount_rows[block], size));
        }
        return 0;
    }
    assert(!p4_game_tables_derived_relocated(P4_TABLE_INPUT_PERSONA, P4_PERSONA_SKILL_PAIR_SPAN, &loaded, &relocated, &size));
    uint8_t expected[68];
    memset(expected, 0x5a, sizeof expected);
    if (is("delta") || is("delta-heal-on") || is("delta-heal-off") ||
        is("delta-percent-on") || is("delta-percent-off") || rider_mode()) {
        /* Producer's row 12 rolls its own hits and one ailment. Only "delta"
         * changes its hit range, damage kind and ailment rate. */
        uint8_t row12[40];
        memcpy(row12, expected, sizeof row12);
        row12[0] = 0; row12[0x11] = 1; row12[0x18] = 1;
        row12[0x1c] = 0x10; row12[0x1d] = row12[0x1e] = row12[0x1f] = 0;
        if (is("delta")) {
            row12[0x0f] = 2; row12[0x10] = 4; row12[0x11] = 14; row12[0x19] = 45;
        }
        assert(!memcmp(row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 12), row12, sizeof row12));
    } else assert(!memcmp(row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 12), expected, 40));
    if (is("delta")) {
        /* The delta patches the producer's row 11 (power and cost) after it,
         * and the disc's row 2 (accuracy and critical). */
        expected[0x12] = 80; expected[0x13] = 0; expected[0x04] = 6; expected[0x05] = 0;
        uint8_t row2[40];
        memcpy(row2, delta_row, sizeof row2);
        row2[0x0e] = 90; row2[0x25] = 20;
        assert(!memcmp(row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 2), row2, sizeof row2));
        assert(p4_game_tables_derived_delta_count() == 12);
        /* Moved 12 to 3 and re-sorted; 21 became 30; 31 added after 22 at 8;
         * 62 added to 0xC3 ends its run: the stale event after it is cleared. */
        static const uint8_t c0[][2] = {{0, 10}, {3, 12}, {5, 11}}, c6[][2] = {{0, 20}, {7, 30}, {8, 22}, {8, 31}},
            c3[][2] = {{0, 60}, {5, 62}};
        expect_learns(0, c0, 3);
        expect_learns(6, c6, 4);
        expect_learns(3, c3, 2);
    }
    assert(!memcmp(row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 11), expected, 40));
    if (is("delta-heal-on") || is("delta-heal-off")) {
        /* Match every byte of both full heals and the adjacent Media row.
         * On/off differs exclusively in the two authored 16-bit costs. */
        static const uint32_t ids[] = {0xc2, 0xc3, 0xc5};
        static const uint8_t old_costs[] = {18, 7, 30}, new_costs[] = {12, 7, 36};
        assert(p4_game_tables_derived_delta_count() == (is("delta-heal-on") ? 2u : 0u));
        for (unsigned n = 0; n < 3; ++n) {
            uint8_t heal[40];
            for (unsigned i = 0; i < sizeof heal; ++i) heal[i] = (uint8_t)(ids[n] + i);
            heal[4] = is("delta-heal-on") ? new_costs[n] : old_costs[n];
            heal[5] = 0;
            heal[0x11] = 11; heal[0x12] = 100; heal[0x13] = 0;
            assert(!memcmp(row(P4_TABLE_INPUT_SKILL, 1, 0, 40, ids[n]), heal, sizeof heal));
        }
    }
    if (percent_mode()) {
        static const uint32_t ids[] = {0xc0, 0xc1, 0xc3, 0xc4};
        static const uint8_t old_costs[] = {3, 6, 7, 12}, new_costs[] = {3, 6, 12, 24};
        static const uint8_t old_power[] = {55, 100, 50, 140}, new_power[] = {25, 50, 22, 42};
        const int on = percent_on();
        assert(p4_game_tables_derived_delta_count() == (on ? (combined_mode() ? 22u : 6u) : 0u));
        for (unsigned n = 0; n < 4; ++n) {
            uint8_t heal[40];
            for (unsigned i = 0; i < sizeof heal; ++i) heal[i] = (uint8_t)(ids[n] + i);
            heal[4] = on ? new_costs[n] : old_costs[n]; heal[5] = 0;
            heal[0x11] = on ? 11 : 2; heal[0x12] = on ? new_power[n] : old_power[n]; heal[0x13] = 0;
            assert(!memcmp(row(P4_TABLE_INPUT_SKILL, 1, 0, 40, ids[n]), heal, sizeof heal));
            assert(!p4_game_tables_skill_max_heal(ids[n])); /* not exposed during preparation */
        }
        /* Only the combined declaration changes the two full-heal costs. */
        for (unsigned n = 0; n < 2; ++n) {
            const uint32_t id = n ? 0xc5 : 0xc2;
            uint8_t heal[40];
            for (unsigned i = 0; i < sizeof heal; ++i) heal[i] = (uint8_t)(id + i);
            heal[4] = combined_on() ? (n ? 36 : 12) : (n ? 30 : 18);
            heal[5] = 0; heal[0x11] = 11; heal[0x12] = 100; heal[0x13] = 0;
            assert(!memcmp(row(P4_TABLE_INPUT_SKILL, 1, 0, 40, id), heal, sizeof heal));
        }
    }
    if (rider_mode()) {
        static const uint8_t power[] = {140, 125, 70, 70, 70, 70, 70};
        static const uint8_t cost[] = {22, 22, 15, 15, 15, 15, 15};
        assert(p4_game_tables_derived_delta_count() == (rider_on() ? (combined_mode() ? 22u : 14u) : 0u));
        for (unsigned n = 0; n < 7; ++n) {
            uint8_t expected_rider[40];
            rider_row(expected_rider, rider_ids[n], rider_produced());
            if (rider_on()) {
                expected_rider[4] = cost[n];
                expected_rider[0x12] = power[n];
            }
            assert(!memcmp(row(P4_TABLE_INPUT_SKILL, 1, 0, 40, rider_ids[n]), expected_rider, sizeof expected_rider));
        }
    }
    assert(row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 0)[0] == 2 && row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 13)[0] == 0);
    assert(row(P4_TABLE_INPUT_SKILL, 0, 0, 2, 20)[0] == 9 && row(P4_TABLE_INPUT_SKILL, 0, 0, 2, 10)[0] == 1);
    memset(expected, 0xa7, sizeof expected);
    assert(!memcmp(row(P4_TABLE_INPUT_ITEM, 0, 16, 68, 7), expected, 68) && row(P4_TABLE_INPUT_ITEM, 0, 16, 68, 6)[67] == 0 &&
        row(P4_TABLE_INPUT_ITEM, 0, 16, 68, 8)[0] == 0);
    assert(!memcmp(row(P4_TABLE_INPUT_MSG, 1, 0, 19, 3), "AGILAO", 7));
    if (is("names"))
        assert(!strcmp(p4_game_tables_derived_item_name(7), "Golden Tonic") &&
            !strcmp(p4_game_tables_derived_item_name(9), "Moon Stone") && !p4_game_tables_derived_item_name(8) &&
            p4_game_tables_derived_item_name_count() == 2);
    return 0;
}
static void publish(void *user) { (void)user; ++publishes; }
/* The loader's buffer: the selected original file (the composition reverted). */
static uint8_t *original(P4GameTableInputKind kind, size_t *size)
{
    const P4GameTableInput *input = input_of(kind);
    *size = p4_game_table_input_size(input);
    uint8_t *copy = malloc(*size);
    assert(copy);
    memcpy(copy, p4_game_table_input_bytes(input), *size);
    const size_t base = (size_t)(p4_game_table_input_span_bytes(p4_game_table_input_span(input, kind == P4_TABLE_INPUT_ITEM ? 0 : 1)) -
        p4_game_table_input_bytes(input));
    if (kind == P4_TABLE_INPUT_ITEM) memcpy(copy + base + 16 + 7 * 68, item_row, sizeof item_row);
    else {
        memcpy(copy + base + 11 * 40, skill_rows, sizeof skill_rows);
        if (is("delta")) memcpy(copy + base + 2 * 40, delta_row, sizeof delta_row);
        const size_t elements = (size_t)(p4_game_table_input_span_bytes(p4_game_table_input_span(input, 0)) -
            p4_game_table_input_bytes(input));
        memcpy(copy + elements, skill_elements, sizeof skill_elements);
    }
    return copy;
}
static void observe(const uint8_t *descriptor, const char *path, const uint8_t *bytes, uint32_t size,
    P4GameTableAdmissionReceipt *receipt)
{
    const P4GameTableInput *skill = input_of(P4_TABLE_INPUT_SKILL);
    assert(receipt && size == p4_game_table_input_size(skill) && !memcmp(bytes, p4_game_table_input_bytes(skill), size));
    assert(!p4_game_table_admission_consume(receipt, descriptor, path, bytes, size));
    ++receipts;
}
/* The unchanged packed loader (0x00193840) for PERSONA.TBL and ENCOUNT.TBL:
 * the shared admission, then the arena copies with their slot stores and
 * extent notes. */
static const uint32_t persona_slots[6] = {0x7644c4, 0x7644cc, 0x7644d4, 0x7644d8, PAIR_SLOT, 0x7644e0};
static P4GameTableInputKind loading;
static void observe_load(const uint8_t *descriptor, const char *path, const uint8_t *bytes, uint32_t size,
    P4GameTableAdmissionReceipt *receipt)
{
    const P4GameTableInput *input = input_of(loading);
    assert(receipt && size == p4_game_table_input_size(input) && !memcmp(bytes, p4_game_table_input_bytes(input), size));
    assert(!p4_game_table_admission_consume(receipt, descriptor, path, bytes, size));
    ++receipts;
}
static uint32_t word(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}
static void write_word(uint8_t *bytes, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) bytes[i] = (uint8_t)(value >> (8u * i));
}
static void admission(void)
{
    size_t size;
    uint8_t *item = original(P4_TABLE_INPUT_ITEM, &size);
    const P4GameTableInput *items = input_of(P4_TABLE_INPUT_ITEM);
    assert(p4_game_table_input_matches(items, item, size) && memcmp(item, p4_game_table_input_bytes(items), size));
    p4_game_table_admission_item("init/itemtbl.bin", item, (uint32_t)size);
    assert(!memcmp(item, p4_game_table_input_bytes(items), size));
    p4_game_table_admission_item("init/itemtbl.bin", item, (uint32_t)size); /* already composed */
    free(item);
    uint8_t descriptor[0xa4] = {0};
    write_word(descriptor + 4, 0x7644a4);
    write_word(descriptor + 12, 0x7644a8);
    uint8_t *skill = original(P4_TABLE_INPUT_SKILL, &size);
    assert(p4_game_table_input_matches(input_of(P4_TABLE_INPUT_SKILL), skill, size));
    p4_game_table_admission_packed(descriptor, "battle/SKILL.TBL", skill, (uint32_t)size, observe);
    p4_game_table_admission_packed(descriptor, "battle/SKILL.TBL", skill, (uint32_t)size, observe);
    assert(receipts == 2);
    free(skill);
}
static uint32_t load_file(P4GameTableInputKind kind, const uint32_t *slots, unsigned blocks, uint32_t arena)
{
    const P4GameTableInput *input = input_of(kind);
    uint8_t descriptor[0xa4] = {0};
    for (unsigned block = 0; block < blocks; ++block) write_word(descriptor + block * 8u + 4u, slots[block]);
    const size_t size = p4_game_table_input_size(input);
    uint8_t *bytes = malloc(size);
    assert(bytes);
    memcpy(bytes, p4_game_table_input_bytes(input), size);
    loading = kind;
    p4_game_table_admission_packed(descriptor, p4_game_table_input_label(kind), bytes, (uint32_t)size, observe_load);
    uint32_t offset = 0;
    for (unsigned block = 0; block < blocks; ++block) {
        const uint32_t block_size = word(bytes + offset);
        memcpy(P4_G2H(arena), bytes + offset + 4, block_size);
        P4_G32(slots[block]) = arena;
        p4_game_table_extent_note(slots[block], arena, block_size);
        arena += (block_size + 15u) & ~15u;
        offset += (block_size + 4u + 15u) & ~15u;
    }
    free(bytes);
    assert(arena <= HEAP);
    return arena;
}
/* Exercise the semantic skill consumer, not just the table-composition output.
 * These are boot-selected package variants, not switches within a live run. */
static void combined_skill_consumer(void)
{
    const P4GameTableInput *skill = input_of(P4_TABLE_INPUT_SKILL);
    const P4GameTableInput *effect = input_of(P4_TABLE_INPUT_EFFECT);
    const P4GameTableInput *message = input_of(P4_TABLE_INPUT_MSG);
    P4GameSkills *skills = p4_game_skills_create(NULL);
    assert(skills && skill && effect && message);
    assert(!p4_game_skills_seal(skills, p4_game_table_input_span(skill, 1),
        p4_game_table_input_span(skill, 0), p4_game_table_input_span(effect, 0),
        p4_game_table_input_span(message, 1), NULL, NULL, NULL, 1));
    const uint32_t heals[] = {0xc0, 0xc1, 0xc3, 0xc4, 0xc2, 0xc5};
    const uint8_t old_costs[] = {3, 6, 7, 12, 18, 30};
    const uint8_t new_costs[] = {3, 6, 12, 24, 12, 36};
    const uint8_t old_power[] = {55, 100, 50, 140, 100, 100};
    const uint8_t new_power[] = {25, 50, 22, 42, 100, 100};
    const int on = combined_on();
    P4GameSkillDefinition definition;
    for (unsigned i = 0; i < sizeof heals / sizeof *heals; ++i) {
        assert(!p4_game_skills_definition_find(skills, heals[i], &definition));
        assert(definition.active && definition.metadata && !definition.authored);
        assert(definition.metadata[4] == (on ? new_costs[i] : old_costs[i]));
        assert(!definition.metadata[5] && !definition.metadata[0x13]);
        assert(definition.metadata[0x11] == (i < 4 && !on ? 2 : 11));
        assert(definition.metadata[0x12] == (on ? new_power[i] : old_power[i]));
        assert(p4_game_tables_skill_max_heal(heals[i]) == (i < 4 && on));
    }
    const uint8_t power[] = {140, 125, 70, 70, 70, 70, 70};
    const uint8_t cost[] = {22, 22, 15, 15, 15, 15, 15};
    for (unsigned i = 0; i < sizeof rider_ids / sizeof *rider_ids; ++i) {
        assert(!p4_game_skills_definition_find(skills, rider_ids[i], &definition));
        assert(definition.active && definition.metadata && !definition.authored);
        assert(definition.metadata[4] == (on ? cost[i] : 9));
        assert(definition.metadata[0x12] == (on ? power[i] : 123));
        assert(definition.metadata[0x18] == 1);
        assert(definition.metadata[0x19] == (rider_produced() ? 35 : 20));
        assert(definition.metadata[0x1c] == 0x10);
    }
    p4_game_skills_destroy(skills);
}
/* Synthetic native-bank completion over the actual composed inputs. The
 * origin callbacks are installed but must never be used by publication. */
static int unused_original(const void *span, void *context)
{ (void)span; (void)context; abort(); }
static int unused_skill(const P4GameSkills *skills, uint32_t id, void *context)
{ (void)skills; (void)id; (void)context; abort(); }
static int unused_text(const P4GameText *text, uint32_t table, uint32_t id, uint32_t role, void *context)
{ (void)text; (void)table; (void)id; (void)role; (void)context; abort(); }
static void *skill_allocate(void *user, uint32_t size, uint32_t *guest)
{
    (void)user;
    void *host = p4pc_heap_malloc(size);
    if (host) *guest = P4_H2G(host);
    return host;
}
static void skill_release(void *user, uint32_t guest, void *host, uint32_t size)
{ (void)user; (void)guest; (void)size; p4pc_heap_free(host); }
static uint32_t copied_span(const P4GameTableInputSpan *span, uint32_t slot, uint32_t at)
{
    const size_t size = p4_game_table_input_span_size(span);
    assert(at + size <= HEAP);
    memcpy(P4_G2H(at), p4_game_table_input_span_bytes(span), size);
    p4_game_table_slot_publish(slot, at, (uint32_t)size);
    return at + (((uint32_t)size + 15u) & ~15u);
}
static void combined_expanded_consumer(void)
{
    if (p4t_guest_map(0x00764000u, 0x2000u) || p4t_guest_map(ARENA, ARENA_SIZE)) exit(77);
    native_phase = 1;
    const P4GameTableInput *skill = input_of(P4_TABLE_INPUT_SKILL);
    const P4GameTableInput *effect = input_of(P4_TABLE_INPUT_EFFECT);
    const P4GameTableInput *message = input_of(P4_TABLE_INPUT_MSG);
    const P4GameTableInputSpan *metadata = p4_game_table_input_span(skill, 1);
    const P4GameTableInputSpan *elements = p4_game_table_input_span(skill, 0);
    const P4GameTableInputSpan *presentation = p4_game_table_input_span(effect, 0);
    const P4GameTableInputSpan *original_names = p4_game_table_input_span(message, 1);
    P4GameTableAllocator allocator = {NULL, skill_allocate, skill_release};
    P4TextProfile raw = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4TextProfile msg = {P4_TEXT_ENCODING_US_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4GameTables *tables = p4_game_tables_create();
    P4GameText *names = p4_game_tables_text_create(&allocator, &raw);
    P4GameText *descriptions = p4_game_tables_text_create(&allocator, &msg);
    P4GameSkills *skills = p4_game_skills_create(&allocator);
    assert(tables && names && descriptions && skills && !p4_game_tables_freeze(tables, 1));
    assert(!p4_game_tables_text_bind_runtime_use(names, unused_text, skills));
    assert(!p4_game_tables_text_bind_runtime_use(descriptions, unused_text, skills));
    assert(!p4_game_skills_bind_runtime_use(skills, unused_original, unused_skill, tables));
    static const uint32_t anchors[] = {0xc0, 0xc2, 0, 0x99, 0xc0};
    for (uint32_t i = 0; i < 5; ++i) {
        P4GameSkillDesc desc = {.id = 576u + i, .active = 1, .element_id = 6,
            .schema = 1, .revision = 1, .derive = anchors[i]};
        snprintf(desc.key, sizeof desc.key, "test:heal-%u", (unsigned)i);
        if (i == 4) { desc.derive_fields = P4_SKILL_DERIVE_POWER; desc.derive_power = 37; }
        if (!desc.derive)
            memcpy(desc.metadata, p4_game_table_input_span_bytes(metadata) + 0xc0u * 40u, 40);
        assert(!p4_game_skills_register(skills, "test", &desc));
        assert(!p4_game_tables_text_register(names, "test", P4_TEXT_SKILL, desc.id,
            P4_TEXT_NAME, 1, (const uint8_t *)"Test", 5));
        assert(!p4_game_tables_text_register(descriptions, "test", P4_TEXT_SKILL, desc.id,
            P4_TEXT_DESCRIPTION, 1, (const uint8_t *)"Test", 5));
    }
    assert(!p4_game_tables_text_seal(names, 1) && !p4_game_tables_text_seal(descriptions, 1));
    assert(!p4_game_skills_seal(skills, metadata, elements, presentation, original_names,
        tables, names, descriptions, 1));
    P4GameSkillDefinition anchor, added;
    assert(!p4_game_skills_definition_find(skills, 0xc0, &anchor));
    assert(!p4_game_skills_definition_find(skills, 576, &added));
    assert(!memcmp(anchor.metadata, added.metadata, 40));
    assert(p4_game_tables_skill_max_heal_anchor(0xc0, anchor.metadata) == combined_on());
    assert(!p4_game_tables_skill_max_heal_anchor(0xc0, added.metadata));
    assert(!p4_game_skill_max_heal(576)); /* sealed is not live */
    assert(!p4_game_tables_text_materialize(names) && !p4_game_tables_text_materialize(descriptions));
    assert(!p4_game_skills_boot_stage(skills));
    uint32_t at = copied_span(metadata, 0x007644a8u, ARENA);
    at = copied_span(elements, 0x007644a4u, at);
    at = copied_span(presentation, 0x007644acu, at);
    copied_span(original_names, 0x00764538u, at);
    P4_GPDATA(uint32_t, 0xb438) = 8;
    p4_game_skills_packed_accept(15); /* synthetic loader's four observed banks */
    assert(!p4_game_tables_battle_complete());
    assert(p4_game_skills_active() == skills);
    assert(!memcmp(P4_G2H(p4_game_skills_metadata(skills) + 576u * 40u), added.metadata, 40));
    assert(p4_game_skill_max_heal(0xc0) == combined_on());
    assert(p4_game_skill_max_heal(576) == combined_on());
    assert(!p4_game_skill_max_heal(577) && !p4_game_skill_max_heal(578) &&
        !p4_game_skill_max_heal(579) && !p4_game_skill_max_heal(581));
    assert(p4_game_skill_max_heal(580) == combined_on());
    assert(((const uint8_t *)P4_G2H(p4_game_skills_metadata(skills) + 580u * 40u))[0x12] == 37);
    const uint32_t live = P4_G32(0x007644a8u);
    P4_G32(0x007644a8u) = ARENA; /* stale or replaced bank cannot inherit */
    assert(!p4_game_skill_max_heal(576));
    P4_G32(0x007644a8u) = live;
    assert(p4_game_skill_max_heal(576) == combined_on());
    /* A packed-table reload temporarily restores the loader's original slots.
     * No old expanded marker is used until the same bank is republished. */
    p4_game_skills_load_begin();
    at = copied_span(metadata, 0x007644a8u, ARENA);
    at = copied_span(elements, 0x007644a4u, at);
    at = copied_span(presentation, 0x007644acu, at);
    copied_span(original_names, 0x00764538u, at);
    assert(!p4_game_skill_max_heal(576));
    p4_game_skills_packed_accept(15);
    assert(!p4_game_tables_battle_complete());
    assert(P4_G32(0x007644a8u) == live);
    assert(p4_game_skill_max_heal(576) == combined_on());
    assert(!p4_game_skills_session_end(1));
    assert(!p4_game_skill_max_heal(576));
    assert(!p4_game_tables_battle_complete_clear());
    p4_game_skills_destroy(skills);
    p4_game_tables_text_destroy(descriptions);
    p4_game_tables_text_destroy(names);
    p4_game_tables_destroy(tables);
    native_phase = 0;
}
/* A slot the relocation moves: its stride, published rows and bytes, and the
 * zero guard after them. */
typedef struct Moved {
    uint32_t slot, stride, rows, loaded, guard;
    const uint8_t *bytes;
} Moved;
/* Battle-table completion (0x0010d480) after the eight packed loads. Without
 * a relocated span nothing moves. With one, each relocated slot points at a
 * heap copy of the producer's rows (the skill pairs followed by three zero
 * guard pairs) with its extent, every other slot keeps its arena block, a
 * reload's completion moves it again to the same copy, and session end puts
 * the loaded original back. */
static void completion(void)
{
    if (p4t_guest_map(0x00764000u, 0x2000u) || p4t_guest_map(ARENA, ARENA_SIZE)) exit(77);
    native_phase = 1;
    Moved moved[4];
    unsigned count = 0;
    if (is("pairs")) moved[count++] = (Moved){PAIR_SLOT, P4_PERSONA_SKILL_PAIR_SIZE, PAIRS, 4u,
        3u * P4_PERSONA_SKILL_PAIR_SIZE, pair_list};
    if (is("encount"))
        for (unsigned block = 0; block < 4; ++block)
            moved[count++] = (Moved){encount_slots[encount_spans[block]], encount_strides[block],
                (uint32_t)(encount_sizes[block] / encount_strides[block]), encount_loaded[block], 0, encount_rows[block]};
    /* Renamed items: one more guest block, read through datGetItemNameId. */
    const unsigned names = is("names") ? 1u : 0u;
    assert(!p4_game_tables_derived_item_name_stream(7));
    uint32_t all[11], copies[4] = {0};
    for (unsigned i = 0; i < 6; ++i) all[i] = persona_slots[i];
    for (unsigned i = 0; i < 5; ++i) all[6 + i] = encount_slots[i];
    for (unsigned load = 0; load < 2; ++load) {
        load_file(P4_TABLE_INPUT_ENCOUNT, encount_slots, 5, load_file(P4_TABLE_INPUT_PERSONA, persona_slots, 6, ARENA));
        P4_GPDATA(uint32_t, 0xb438) = 8;
        uint32_t before[11], base, rows;
        for (unsigned i = 0; i < 11; ++i) before[i] = P4_G32(all[i]);
        for (unsigned i = 0; i < count; ++i)
            assert(p4_game_table_extent_rows(moved[i].slot, moved[i].stride, &base, &rows) && rows == moved[i].loaded);
        assert(!p4_game_tables_battle_complete());
        assert(allocations == count + names);
        if (names) {
            assert(!strcmp((const char *)P4_G2H(p4_game_tables_derived_item_name_stream(7)), "Golden Tonic"));
            assert(!strcmp((const char *)P4_G2H(p4_game_tables_derived_item_name_stream(9)), "Moon Stone"));
            assert(!p4_game_tables_derived_item_name_stream(8) && !p4_game_tables_derived_item_name_stream(1536));
        }
        for (unsigned i = 0; i < 11; ++i) {
            unsigned m = 0;
            while (m < count && moved[m].slot != all[i]) ++m;
            if (m == count) { assert(P4_G32(all[i]) == before[i]); continue; }
            assert(p4_game_table_extent_rows(moved[m].slot, moved[m].stride, &base, &rows));
            assert(base != before[i] && base >= HEAP && base == P4_G32(all[i]) && rows == moved[m].rows);
            assert(!copies[m] || base == copies[m]);
            copies[m] = base;
            const uint8_t *copy = P4_G2H(base);
            assert(!memcmp(copy, moved[m].bytes, (size_t)rows * moved[m].stride));
            for (unsigned g = 0; g < moved[m].guard; ++g) assert(!copy[(size_t)rows * moved[m].stride + g]);
        }
    }
    assert(!p4_game_tables_derived_session_end(1));
    /* A live image gets each loaded original block back. */
    for (unsigned i = 0; i < count; ++i) {
        uint32_t base, rows;
        assert(p4_game_table_extent_rows(moved[i].slot, moved[i].stride, &base, &rows));
        assert(base == P4_G32(moved[i].slot) && base < HEAP && rows == moved[i].loaded);
    }
    assert(releases == count + names && !p4_game_tables_derived_item_name_stream(7));
    assert(!p4_game_tables_battle_complete_clear());
    native_phase = 0;
}
int main(int argc, char **argv)
{
    if (argc != 3 && argc != 4) return 2;
    mode = argv[1];
    const char *enabled = argc == 4 ? argv[3] : "derived.owner";
    const int success = is("good") || is("gated-off") || is("source") || is("pairs") || is("encount") || is("delta") ||
        is("delta-only") || is("names") || is("delta-heal-on") || is("delta-heal-off") || is("delta-cost-bounds") || is("delta-percent-on") || is("delta-percent-off") || is("delta-percent-bounds") || rider_mode() || is("drain");
    assert(!p4_game_tables_derived_register(&producer));
    assert(p4_game_tables_derived_register(&producer) == -1); /* duplicate id */
    static const P4PackageClient client = {.diagnostic_name = "table derived fixture", .manifest_name = "mod.ini",
        .section = "[mod]", .kind = P4_PACKAGE_MOD};
    static const char *const dependencies[] = {P4_GAME_TABLES_BOOT_PROVIDER, NULL};
    static const P4PackageHandler observer = {.name = "derived-product-fixture", .prepare = prepare, .publish = publish,
        .boot_phase = P4_PACKAGE_BOOT_UI, .dependencies = dependencies};
    assert(!p4_package_handler_register(&observer));
    assert(!p4_package_handler_register(&p4_game_tables_sources_boot_provider));
    assert(!p4_package_handler_register(&p4_mod_settings_boot_provider));
    if (is("drain")) {
        const char *root = getenv("P4PC_TEST_DRAIN_SETTINGS");
        const char *choice = getenv("P4PC_TEST_DRAIN_CHOICE");
        char reason[256];
        assert(root && choice && !p4_mod_setting_store_root(root));
        /* The drain fixture copies hard mode's PARTY KIT setting (`when = party_kit`). */
        assert(!p4_mod_setting_store("derived.delta", "party_kit", choice, reason, sizeof reason));
    }
    P4PackageRegistry *registry = NULL;
    assert(!p4_package_registry_load(&registry, &client, argv[2], enabled));
    const int result = p4_package_registry_publish();
    if (!success) {
        assert(result && !prepares && !publishes);
        /* No delta refusal reaches a producer except delta-weapon's: declarations
         * are refused while the sections boot, and the other compose and
         * reference refusals come from modes without a producer package. */
        const int produced = !is("bad-revision") && !is("unknown-producer") && !is("unknown-input") && !is("twice") &&
            (!delta_mode() || is("delta-weapon"));
        assert(produces == (unsigned)produced);
    } else {
        assert(!result && prepares == 1 && publishes == 1);
        if (combined_mode()) {
            combined_skill_consumer();
            combined_expanded_consumer();
        }
        if (percent_mode()) {
            const int on = percent_on();
            assert(p4_game_tables_skill_max_heal(0xc0) == on && p4_game_tables_skill_max_heal(0xc1) == on &&
                p4_game_tables_skill_max_heal(0xc3) == on && p4_game_tables_skill_max_heal(0xc4) == on);
            assert(!p4_game_tables_skill_max_heal(0xc2) && !p4_game_tables_skill_max_heal(0xc5) &&
                !p4_game_tables_skill_max_heal(0xffffffffu));
        }
        if (is("delta-percent-bounds"))
            assert(p4_game_tables_skill_max_heal(0xc1) && p4_game_tables_skill_max_heal(0xc4) &&
                !p4_game_tables_skill_max_heal(0xc0));
        if (is("good") || is("delta") || is("names")) admission();
        if (is("source")) {
            /* Exposed once the composed catalog is active: a read-only copy, by
             * producer id and name only. */
            const uint8_t *bytes = NULL;
            size_t size = 0;
            assert(!p4_table_derived_source("test:golden", "init/cmpTable.bin", &bytes, &size));
            assert(size == 64 && bytes[0] == 0x40 && bytes[63] == 0x40 + 63);
            assert(p4_table_derived_source("test:other", "init/cmpTable.bin", &bytes, &size) == -1);
            assert(p4_table_derived_source("test:golden", "init/other.bin", &bytes, &size) == -1);
        }
        if (is("good") || is("pairs") || is("encount") || is("delta") || is("names")) completion();
    }
    assert(!p4_game_tables_boot_session_end());
    assert(!p4_game_tables_skill_max_heal(0xc0));
    {
        const uint8_t *bytes = NULL;
        size_t size = 0;
        assert(p4_table_derived_source("test:golden", "init/cmpTable.bin", &bytes, &size) == -1);
    }
    assert(!p4_game_tables_derived_count() && !p4_game_tables_derived_delta_count());
    p4_package_registry_retire();
    assert(!registry && !p4_packages_memory_usage().resident && !p4_packages_memory_usage().reserved);
    printf("game_table_derived: %s passed\n", mode);
    return 0;
}
