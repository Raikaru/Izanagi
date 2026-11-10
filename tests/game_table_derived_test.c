/* [table-derived] over real package boot and synthetic disc inputs: producer
 * selection, base spans, row/span composition before any family reads them,
 * refusals, the admission rewrite of a loader buffer that still holds the
 * selected original, and the relocations of the PERSONA4 skill pairs and the
 * grown ENCOUNT blocks: the loader keeps the original blocks and battle-table
 * completion moves their slots to the new rows in guest memory. */
#include "game_table_derived_internal.h"
#include "p4pc/game_table_admission_native.h"
#include "p4pc/game_table_boot.h"
#include "p4pc/game_table_derived.h"
#include "p4pc/game_tables_native.h"
#include "p4pc/game_elements.h"
#include "p4pc/game_damage_cover.h"
#include "p4pc/game_side_ailment.h"
#include "p4pc/game_encounters.h"
#include "p4pc/game_personas.h"
#include "p4pc/game_personas_save.h"
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
static uint8_t skill_rows[2][40], skill_elements[576 * 2], item_row[68];
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
int p4_game_skills_boot_stage(P4GameSkills *skills)
{ (void)skills; fputs("Derived-only boot staged skills\n", stderr); abort(); }
int p4_game_skills_boot_unstage(P4GameSkills *skills)
{ (void)skills; fputs("Unowned skill raw-hook rollback\n", stderr); abort(); }
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
static int prepare(void *user)
{
    (void)user; ++prepares;
    const void *found = (const void *)1;
    assert(!p4_package_boot_product(P4_GAME_TABLES_BOOT_PROVIDER, &found));
    product = found;
    if (is("gated-off")) { assert(!product && !produces); return 0; }
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
    assert(!memcmp(row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 11), expected, 40) &&
        !memcmp(row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 12), expected, 40));
    assert(row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 0)[0] == 2 && row(P4_TABLE_INPUT_SKILL, 1, 0, 40, 13)[0] == 0);
    assert(row(P4_TABLE_INPUT_SKILL, 0, 0, 2, 20)[0] == 9 && row(P4_TABLE_INPUT_SKILL, 0, 0, 2, 10)[0] == 1);
    memset(expected, 0xa7, sizeof expected);
    assert(!memcmp(row(P4_TABLE_INPUT_ITEM, 0, 16, 68, 7), expected, 68) && row(P4_TABLE_INPUT_ITEM, 0, 16, 68, 6)[67] == 0 &&
        row(P4_TABLE_INPUT_ITEM, 0, 16, 68, 8)[0] == 0);
    assert(!memcmp(row(P4_TABLE_INPUT_MSG, 1, 0, 19, 3), "AGILAO", 7));
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
        assert(allocations == count);
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
    assert(releases == count);
    assert(!p4_game_tables_battle_complete_clear());
    native_phase = 0;
}
int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    mode = argv[1];
    const int success = is("good") || is("gated-off") || is("source") || is("pairs") || is("encount");
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
    P4PackageRegistry *registry = NULL;
    assert(!p4_package_registry_load(&registry, &client, argv[2], "derived.owner"));
    const int result = p4_package_registry_publish();
    if (!success) {
        assert(result && !prepares && !publishes);
        const int produced = !is("bad-revision") && !is("unknown-producer") && !is("unknown-input") && !is("twice");
        assert(produces == (unsigned)produced);
    } else {
        assert(!result && prepares == 1 && publishes == 1);
        if (is("good")) admission();
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
        if (is("good") || is("pairs") || is("encount")) completion();
    }
    assert(!p4_game_tables_boot_session_end());
    {
        const uint8_t *bytes = NULL;
        size_t size = 0;
        assert(p4_table_derived_source("test:golden", "init/cmpTable.bin", &bytes, &size) == -1);
    }
    assert(!p4_game_tables_derived_count());
    p4_package_registry_retire();
    assert(!registry && !p4_packages_memory_usage().resident && !p4_packages_memory_usage().reserved);
    printf("game_table_derived: %s passed\n", mode);
    return 0;
}
