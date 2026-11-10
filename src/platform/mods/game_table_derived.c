#include "game_table_derived_internal.h"
#include "game_table_inputs_internal.h"
#include "p4pc/game_encounters.h"
#include "p4pc/game_enemies.h"
#include "p4pc/game_items_native.h"
#include "p4pc/game_personas.h"
#include "p4pc/game_skills_native.h"
#include "p4pc/game_table_derived.h"
#include "p4pc/mod_setting_gate.h"
#include "p4pc/package_memory.h"
#include "p4pc/table_result.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { PRODUCER_MAX = 8, KEY_MAX = 64, LABEL_MAX = 48, REASON_MAX = 192, SPAN_MAX = 11 };

typedef struct Selection {
    const P4PackageProvider *provider;
    const P4TableDerivedProducer *producer;
    uint32_t inputs;
    char key[KEY_MAX];
} Selection;
typedef struct Replacement {
    size_t owner;
    P4GameTableInputKind input;
    uint32_t span, stride; /* stride 0: the whole span */
    size_t count;          /* rows; one whole span */
    uint32_t *ids;
    uint8_t *bytes;
    size_t size;           /* whole span bytes */
    int relocated;         /* a relocatable span: published natively, not composed */
} Replacement;
typedef struct Fingerprint {
    size_t owner;
    char label[LABEL_MAX];
    char hex[2 * P4_TABLE_DERIVED_DIGEST_BYTES + 1];
} Fingerprint;
typedef struct Source {
    size_t owner;
    char name[LABEL_MAX];
    uint8_t *bytes;
    size_t size;
    char hex[P4_CONTENT_DIGEST_HEX + 1];
} Source;
typedef struct Composed {
    const P4GameTableInput *input;
    char hex[P4_CONTENT_DIGEST_HEX + 1];
} Composed;
typedef struct Relocated {
    const P4GameTableInput *input; /* still loaded as the selected original */
    size_t replacement;
    char hex[P4_CONTENT_DIGEST_HEX + 1];
} Relocated;
/* Blocks a producer may give another length: no reader bounds them by a
 * retail count. The skill pairs end at their own terminator; ENCOUNT rows are
 * reached only by IDs other rows, field data or scripts name, so a relocated
 * block keeps at least its loaded rows (game_table_derived.h). */
typedef struct Relocatable {
    P4GameTableInputKind input;
    uint32_t span, stride;
} Relocatable;
static const Relocatable relocatable_spans[] = {
    {P4_TABLE_INPUT_PERSONA, P4_PERSONA_SKILL_PAIR_SPAN, P4_PERSONA_SKILL_PAIR_SIZE},
    {P4_TABLE_INPUT_ENCOUNT, P4_ENCOUNT_FORMATION_SPAN, P4_ENCOUNT_FORMATION_SIZE},
    {P4_TABLE_INPUT_ENCOUNT, P4_ENCOUNT_SELECTOR_SPAN, P4_ENCOUNT_SELECTOR_SIZE},
    {P4_TABLE_INPUT_ENCOUNT, P4_ENCOUNT_WEIGHTED_SPAN, P4_ENCOUNT_WEIGHTED_SIZE},
    {P4_TABLE_INPUT_ENCOUNT, P4_ENCOUNT_RECORD_SPAN, P4_ENCOUNT_RECORD_SIZE},
};
enum { RELOCATABLE = sizeof relocatable_spans / sizeof relocatable_spans[0] };
struct P4TableDerivedSink {
    P4TableDerivedRequest request;
    size_t owner, fingerprints;
    P4GameTableInput *const *inputs;
    size_t input_count;
    int refused;
    char reason[REASON_MAX];
};

static const P4TableDerivedProducer *producers[PRODUCER_MAX];
static size_t producer_count;
static const char *const input_names[P4_TABLE_INPUT_COUNT] = {
    "item", "skill", "unit", "msg", "persona", "encount", "effect", "model", "aicalc"
};
static struct {
    Selection *selections;
    Replacement *replacements;
    Fingerprint *fingerprints;
    Source *sources;
    size_t selection_count, replacement_count, fingerprint_count, source_count;
    size_t selection_capacity, replacement_capacity, fingerprint_capacity, source_capacity;
    size_t charged;
    int refused; /* a production was refused: no source is exposed */
    int exposed; /* the table boot published the composed catalog */
    size_t owners[P4_TABLE_INPUT_COUNT][SPAN_MAX]; /* selection index + 1 */
    Composed composed[P4_TABLE_INPUT_COUNT];
    size_t composed_count;
    Relocated relocated[RELOCATABLE];
    size_t relocated_count;
    P4TableDerivedSink *active;
} derived;

static void *charge(size_t bytes)
{
    if (!bytes || p4_packages_memory_reserve(bytes)) return NULL;
    void *memory = malloc(bytes);
    if (!memory) { p4_packages_memory_cancel(bytes); return NULL; }
    p4_packages_memory_commit(bytes);
    derived.charged += bytes;
    return memory;
}
static void uncharge(void *memory, size_t bytes)
{
    if (!memory) return;
    free(memory);
    p4_packages_memory_release(bytes);
    derived.charged -= bytes;
}
static int grow(void **array, size_t *capacity, size_t count, size_t element)
{
    if (count < *capacity) return 0;
    const size_t next = *capacity ? *capacity * 2u : 4u;
    void *replacement = charge(next * element);
    if (!replacement) return -1;
    if (count) memcpy(replacement, *array, count * element);
    uncharge(*array, *capacity * element);
    *array = replacement;
    *capacity = next;
    return 0;
}
static int bounded_id(const char *text, int qualified)
{
    const size_t length = text ? strlen(text) : 0;
    if (!length || length >= KEY_MAX) return 0;
    int colon = 0;
    for (size_t i = 0; i < length; ++i) {
        const char c = text[i];
        if (c == ':') { if (colon++ || !i || i + 1 == length) return 0; continue; }
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.')) return 0;
    }
    return !qualified || colon;
}
static int refuse_text(char *error, size_t capacity, const char *format, ...)
{
    if (error && capacity) {
        va_list args;
        va_start(args, format);
        vsnprintf(error, capacity, format, args);
        va_end(args);
    }
    return -1;
}

int p4_game_tables_derived_register(const P4TableDerivedProducer *producer)
{
    if (!producer || !bounded_id(producer->id, 1) || !producer->revision || !producer->produce ||
        derived.selection_count || producer_count == PRODUCER_MAX) return -1;
    for (size_t i = 0; i < producer_count; ++i)
        if (!strcmp(producers[i]->id, producer->id)) return -1;
    producers[producer_count++] = producer;
    return 0;
}

static int parse_inputs(const char *text, uint32_t *mask, char *error, size_t capacity)
{
    *mask = 0;
    for (const char *cursor = text ? text : ""; *cursor;) {
        while (*cursor == ' ' || *cursor == ',' || *cursor == '\t') ++cursor;
        if (!*cursor) break;
        const char *end = cursor;
        while (*end && *end != ' ' && *end != ',' && *end != '\t') ++end;
        unsigned kind = 0;
        while (kind < P4_TABLE_INPUT_COUNT && (strlen(input_names[kind]) != (size_t)(end - cursor) ||
            strncmp(input_names[kind], cursor, (size_t)(end - cursor)))) ++kind;
        if (kind == P4_TABLE_INPUT_COUNT)
            return refuse_text(error, capacity, "table-derived inputs: unknown input %.*s", (int)(end - cursor), cursor);
        if (*mask & (1u << kind)) return refuse_text(error, capacity, "table-derived inputs: %s is repeated", input_names[kind]);
        *mask |= 1u << kind;
        cursor = end;
    }
    return *mask ? 0 : refuse_text(error, capacity, "table-derived requires a nonempty inputs list");
}
static int gate_field(const char *name)
{
    if (!strcmp(name, P4_MOD_SETTING_GATE_FIELD)) return 1; /* evaluated by p4_mod_setting_section_gate */
#ifdef P4_MOD_COMPANION_GATE_FIELD
    if (!strcmp(name, P4_MOD_COMPANION_GATE_FIELD)) return 1;
#endif
#ifdef P4_MOD_COMPANION_SETTING_FIELD
    if (!strcmp(name, P4_MOD_COMPANION_SETTING_FIELD)) return 1;
#endif
    return 0;
}
int p4_game_tables_derived_select(const P4PackageProvider *provider, const P4PackageField *fields,
    size_t count, uint32_t *inputs, char *error, size_t capacity)
{
    if (!provider || !fields || !count || !inputs) return refuse_text(error, capacity, "Missing table-derived declaration");
    *inputs = 0;
    const char *key = fields[0].argument;
    if (!bounded_id(key, 0)) return refuse_text(error, capacity, "table-derived requires a bounded lowercase KEY");
    for (size_t i = 0; i < derived.selection_count; ++i)
        if (derived.selections[i].provider == provider)
            return refuse_text(error, capacity, "A package selects at most one table-derived producer");
    const char *producer_id = NULL, *revision = NULL, *input_list = NULL;
    size_t producers_seen = 0, revisions = 0, lists = 0;
    for (size_t i = 0; i < count; ++i) {
        const char *name = fields[i].key;
        if (!name) return refuse_text(error, capacity, "Missing table-derived declaration field");
        if (!strcmp(name, "producer")) { producer_id = fields[i].value; ++producers_seen; }
        else if (!strcmp(name, "revision")) { revision = fields[i].value; ++revisions; }
        else if (!strcmp(name, "inputs")) { input_list = fields[i].value; ++lists; }
        else if (!gate_field(name)) return refuse_text(error, capacity, "Unknown table-derived declaration field %s", name);
    }
    if (producers_seen != 1 || revisions != 1 || lists != 1)
        return refuse_text(error, capacity, "table-derived requires one producer, revision and inputs");
    const P4TableDerivedProducer *producer = NULL;
    for (size_t i = 0; i < producer_count && !producer; ++i)
        if (producer_id && !strcmp(producers[i]->id, producer_id)) producer = producers[i];
    if (!producer)
        return refuse_text(error, capacity, "table-derived producer %.64s is not part of this build", producer_id ? producer_id : "");
    char *end = NULL;
    const unsigned long declared = revision && *revision >= '1' && *revision <= '9' ? strtoul(revision, &end, 10) : 0;
    if (!declared || !end || *end || declared > UINT32_MAX)
        return refuse_text(error, capacity, "table-derived requires a nonzero decimal revision");
    if (declared != producer->revision)
        return refuse_text(error, capacity, "table-derived was written for %s revision %lu; this build has revision %u",
            producer->id, declared, (unsigned)producer->revision);
    uint32_t mask = 0;
    if (parse_inputs(input_list, &mask, error, capacity)) return -1;
    if (grow((void **)&derived.selections, &derived.selection_capacity, derived.selection_count, sizeof *derived.selections))
        return refuse_text(error, capacity, "table-derived selection allocation failed");
    Selection *selection = derived.selections + derived.selection_count++;
    *selection = (Selection){provider, producer, mask, ""};
    memcpy(selection->key, key, strlen(key) + 1);
    *inputs = mask;
    return 0;
}
size_t p4_game_tables_derived_count(void) { return derived.selection_count; }

/* Native row strides of the spans the family compilers already validate;
 * other spans are replaced whole. ITEM 0 is the raw file after its header. */
static uint32_t stride(P4GameTableInputKind input, uint32_t span, uint32_t *header)
{
    *header = 0;
    switch (input) {
    case P4_TABLE_INPUT_ITEM: *header = P4_ITEM_FILE_HEADER_SIZE; return span == 0 ? P4_ITEM_ROW_SIZE : 0;
    case P4_TABLE_INPUT_SKILL: return span == 0 ? P4_SKILL_ELEMENT_SIZE : span == 1 ? P4_SKILL_ROW_SIZE : 0;
    case P4_TABLE_INPUT_UNIT:
        return span == 0 ? P4_ENEMY_ROW_SIZE : span == 1 ? P4_ENEMY_AFFINITY_SIZE : span == 2 ? P4_PERSONA_AFFINITY_SIZE : 0;
    case P4_TABLE_INPUT_MSG: return span == 1 ? 19u : span == 2 ? P4_ENEMY_NAME_SIZE : span == 3 ? P4_PERSONA_NAME_SIZE : 0;
    case P4_TABLE_INPUT_PERSONA:
        return span == 0 ? P4_PERSONA_METADATA_SIZE : span == 1 ? P4_PERSONA_GROWTH_SIZE :
            span == 5 ? P4_PERSONA_CANDIDATES_SIZE : 0;
    case P4_TABLE_INPUT_EFFECT: return span == 0 ? 4u : 0;
    case P4_TABLE_INPUT_MODEL: return span == 1 ? P4_ENEMY_MODEL_SIZE : span == 2 ? P4_PERSONA_MODEL_SIZE : 0;
    case P4_TABLE_INPUT_AICALC: return span == 8 ? P4_ENEMY_AI_SIZE : 0;
    default: return 0;
    }
}
static int fail(P4TableDerivedSink *sink, const char *format, ...)
{
    if (!sink->refused) {
        va_list args;
        va_start(args, format);
        vsnprintf(sink->reason, sizeof sink->reason, format, args);
        va_end(args);
        sink->refused = 1;
    }
    return -1;
}
static int live(const P4TableDerivedSink *sink) { return sink && sink == derived.active && !sink->refused; }
static const P4GameTableInputSpan *declared_span(P4TableDerivedSink *sink, P4GameTableInputKind input, uint32_t span)
{
    if ((unsigned)input >= P4_TABLE_INPUT_COUNT ||
        !(derived.selections[sink->owner].inputs & (1u << (unsigned)input))) {
        fail(sink, "table-derived input %u is not declared in inputs", (unsigned)input);
        return NULL;
    }
    for (size_t i = 0; i < sink->input_count; ++i)
        if (p4_game_table_input_kind(sink->inputs[i]) == input) {
            const P4GameTableInputSpan *found = p4_game_table_input_span(sink->inputs[i], span);
            if (!found) fail(sink, "%s has no span %u", input_names[input], (unsigned)span);
            return found;
        }
    fail(sink, "%s was not prepared", input_names[input]);
    return NULL;
}
static int claim(P4TableDerivedSink *sink, P4GameTableInputKind input, uint32_t span)
{
    size_t *owner = &derived.owners[input][span];
    if (!*owner) { *owner = sink->owner + 1; return 0; }
    if (*owner == sink->owner + 1) return 0;
    const Selection *other = derived.selections + (*owner - 1);
    const P4PackageInfo *info = p4_package_provider_info(other->provider);
    fail(sink, "%s span %u is already replaced by %s", input_names[input], (unsigned)span, info ? info->id : "another package");
    p4_package_provider_refuse(other->provider, sink->reason);
    return -1;
}
int p4_table_derived_sink_rows(P4TableDerivedSink *sink, const P4TableDerivedRows *rows)
{
    if (!live(sink)) return -1;
    if (!rows || !rows->count || !rows->ids || !rows->bytes) return fail(sink, "empty table-derived row replacement");
    const P4GameTableInputSpan *span = declared_span(sink, rows->input, rows->span);
    if (!span) return -1;
    uint32_t header;
    const uint32_t native = stride(rows->input, rows->span, &header);
    if (!native) return fail(sink, "%s span %u has no row stride; replace the whole span", input_names[rows->input], (unsigned)rows->span);
    if (rows->row_size != native)
        return fail(sink, "%s span %u rows are %u bytes, not %u", input_names[rows->input], (unsigned)rows->span,
            (unsigned)native, (unsigned)rows->row_size);
    const size_t size = p4_game_table_input_span_size(span);
    const size_t available = size < header ? 0 : (size - header) / native;
    if (rows->count > available)
        return fail(sink, "%s span %u has %lu rows", input_names[rows->input], (unsigned)rows->span, (unsigned long)available);
    for (size_t i = 0; i < rows->count; ++i)
        if (rows->ids[i] >= available || (i && rows->ids[i] <= rows->ids[i - 1]))
            return fail(sink, "%s span %u row ids must be ascending existing rows (row %u)", input_names[rows->input],
                (unsigned)rows->span, (unsigned)rows->ids[i]);
    if (claim(sink, rows->input, rows->span)) return -1;
    if (grow((void **)&derived.replacements, &derived.replacement_capacity, derived.replacement_count,
        sizeof *derived.replacements)) return fail(sink, "table-derived allocation failed");
    uint32_t *ids = charge(rows->count * sizeof *ids);
    uint8_t *bytes = ids ? charge(rows->count * native) : NULL;
    if (!bytes) { uncharge(ids, rows->count * sizeof *ids); return fail(sink, "table-derived allocation failed"); }
    memcpy(ids, rows->ids, rows->count * sizeof *ids);
    memcpy(bytes, rows->bytes, rows->count * native);
    derived.replacements[derived.replacement_count++] =
        (Replacement){sink->owner, rows->input, rows->span, native, rows->count, ids, bytes};
    return 0;
}
static const Relocatable *relocatable(P4GameTableInputKind input, uint32_t span)
{
    for (size_t i = 0; i < RELOCATABLE; ++i)
        if (relocatable_spans[i].input == input && relocatable_spans[i].span == span) return relocatable_spans + i;
    return NULL;
}
static uint32_t half(const uint8_t *bytes) { return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8; }
/* PERSONA 4: the last pair is (0, 0) and no other pair has a zero half, so the
 * forward (original) and reverse (replacement) scans both stop on the last
 * pair; originals are distinct, so the level-up pick's eight-entry candidate
 * stack (0x0011E490) holds at most one pair per known skill. */
static int skill_pairs(P4TableDerivedSink *sink, const uint8_t *bytes, size_t size)
{
    const size_t pairs = size / P4_PERSONA_SKILL_PAIR_SIZE;
    if (size % P4_PERSONA_SKILL_PAIR_SIZE || !pairs || pairs > P4_PERSONA_SKILL_PAIR_MAX)
        return fail(sink, "persona span 4 needs 1 to %u four-byte skill pairs", P4_PERSONA_SKILL_PAIR_MAX);
    const uint8_t *last = bytes + size - P4_PERSONA_SKILL_PAIR_SIZE;
    if (half(last) || half(last + 2)) return fail(sink, "persona skill pairs must end with the pair (0, 0)");
    uint8_t seen[(P4_SKILL_MAX_ID >> 3) + 1] = {0};
    for (size_t pair = 0; pair + 1 < pairs; ++pair) {
        const uint8_t *at = bytes + pair * P4_PERSONA_SKILL_PAIR_SIZE;
        const uint32_t original = half(at), replacement = half(at + 2);
        if (!original || !replacement)
            return fail(sink, "persona skill pair %lu has a zero skill before the last pair", (unsigned long)pair);
        if (original > P4_SKILL_MAX_ID || replacement > P4_SKILL_MAX_ID)
            return fail(sink, "persona skill pair %lu names a skill above %u", (unsigned long)pair, P4_SKILL_MAX_ID);
        if (seen[original >> 3] & (1u << (original & 7u)))
            return fail(sink, "persona skill pair %lu changes skill %u a second time", (unsigned long)pair, (unsigned)original);
        seen[original >> 3] |= (uint8_t)(1u << (original & 7u));
    }
    return 0;
}
/* An ENCOUNT block keeps whole rows, at least as many as the block the loader
 * still loads: every ID a script or field step names there stays in range. */
static int encount_rows(P4TableDerivedSink *sink, const Relocatable *block, size_t loaded, size_t size)
{
    const size_t rows = size / block->stride;
    if (size % block->stride || rows < loaded / block->stride || rows > P4_ENCOUNT_ROW_MAX)
        return fail(sink, "encount span %u needs %lu to %u rows of %u bytes", (unsigned)block->span,
            (unsigned long)(loaded / block->stride), P4_ENCOUNT_ROW_MAX, (unsigned)block->stride);
    return 0;
}
int p4_table_derived_sink_span(P4TableDerivedSink *sink, const P4TableDerivedSpan *span)
{
    if (!live(sink)) return -1;
    if (!span || !span->bytes) return fail(sink, "empty table-derived span replacement");
    const P4GameTableInputSpan *base = declared_span(sink, span->input, span->span);
    if (!base) return -1;
    const Relocatable *relocated = relocatable(span->input, span->span);
    const size_t loaded = p4_game_table_input_span_size(base);
    if (!relocated && span->size != loaded)
        return fail(sink, "a length change for %s span %u is not admitted", input_names[span->input], (unsigned)span->span);
    if (relocated && (relocated->input == P4_TABLE_INPUT_PERSONA ? skill_pairs(sink, span->bytes, span->size) :
        encount_rows(sink, relocated, loaded, span->size))) return -1;
    if (claim(sink, span->input, span->span)) return -1;
    if (grow((void **)&derived.replacements, &derived.replacement_capacity, derived.replacement_count,
        sizeof *derived.replacements)) return fail(sink, "table-derived allocation failed");
    uint8_t *bytes = charge(span->size);
    if (!bytes) return fail(sink, "table-derived allocation failed");
    memcpy(bytes, span->bytes, span->size);
    derived.replacements[derived.replacement_count++] =
        (Replacement){sink->owner, span->input, span->span, 0, 1, NULL, bytes, span->size, relocated != NULL};
    return 0;
}
int p4_table_derived_sink_fingerprint(P4TableDerivedSink *sink, const char *label,
    const uint8_t digest[P4_TABLE_DERIVED_DIGEST_BYTES])
{
    if (!live(sink)) return -1;
    const size_t length = label ? strlen(label) : 0;
    if (!length || length >= LABEL_MAX || !digest) return fail(sink, "table-derived fingerprint needs a short label and a digest");
    for (size_t i = 0; i < length; ++i)
        if (label[i] < 0x20 || label[i] > 0x7e) return fail(sink, "table-derived fingerprint label must be printable ASCII");
    if (grow((void **)&derived.fingerprints, &derived.fingerprint_capacity, derived.fingerprint_count,
        sizeof *derived.fingerprints)) return fail(sink, "table-derived allocation failed");
    Fingerprint *entry = derived.fingerprints + derived.fingerprint_count++;
    entry->owner = sink->owner;
    memcpy(entry->label, label, length + 1);
    for (unsigned i = 0; i < P4_TABLE_DERIVED_DIGEST_BYTES; ++i) snprintf(entry->hex + 2u * i, 3, "%02x", digest[i]);
    ++sink->fingerprints;
    return 0;
}
int p4_table_derived_sink_source(P4TableDerivedSink *sink, const char *name, const uint8_t *bytes, size_t size)
{
    if (!live(sink)) return -1;
    const size_t length = name ? strlen(name) : 0;
    if (!length || length >= LABEL_MAX) return fail(sink, "table-derived source needs a short name");
    for (size_t i = 0; i < length; ++i)
        if (name[i] < 0x20 || name[i] > 0x7e) return fail(sink, "table-derived source name must be printable ASCII");
    if (!bytes || !size || size > P4_TABLE_DERIVED_SOURCE_MAX)
        return fail(sink, "table-derived source %s must be 1 byte to 4 MiB", name);
    for (size_t i = 0; i < derived.source_count; ++i)
        if (derived.sources[i].owner == sink->owner && !strcmp(derived.sources[i].name, name))
            return fail(sink, "table-derived source %s recorded twice", name);
    if (grow((void **)&derived.sources, &derived.source_capacity, derived.source_count, sizeof *derived.sources))
        return fail(sink, "table-derived allocation failed");
    uint8_t *copy = charge(size);
    if (!copy) return fail(sink, "table-derived allocation failed");
    memcpy(copy, bytes, size);
    Source *entry = derived.sources + derived.source_count++;
    entry->owner = sink->owner;
    memcpy(entry->name, name, length + 1);
    entry->bytes = copy;
    entry->size = size;
    P4ContentDigest digest;
    p4_content_digest_begin(&digest);
    p4_content_digest_update(&digest, copy, size);
    p4_content_digest_end(&digest, entry->hex);
    return 0;
}
int p4_table_derived_source(const char *producer_id, const char *name, const uint8_t **bytes, size_t *size)
{
    /* Only the catalog these sources were composed into, once the table boot
     * published it, and never after a refusal. */
    if (!producer_id || !name || !bytes || !size || derived.refused || !derived.exposed) return -1;
    for (size_t i = 0; i < derived.source_count; ++i) {
        const Source *source = derived.sources + i;
        if (strcmp(derived.selections[source->owner].producer->id, producer_id) || strcmp(source->name, name)) continue;
        *bytes = source->bytes;
        *size = source->size;
        return 0;
    }
    return -1;
}
void p4_game_tables_derived_expose(void)
{
    if (!derived.refused) derived.exposed = 1;
}
int p4_table_derived_base(const P4TableDerivedRequest *request, P4GameTableInputKind input,
    uint32_t span, const uint8_t **bytes, size_t *size)
{
    P4TableDerivedSink *sink = derived.active;
    if (!sink || request != &sink->request || !bytes || !size || (unsigned)input >= P4_TABLE_INPUT_COUNT ||
        !(derived.selections[sink->owner].inputs & (1u << (unsigned)input))) return -1;
    for (size_t i = 0; i < sink->input_count; ++i)
        if (p4_game_table_input_kind(sink->inputs[i]) == input) {
            const P4GameTableInputSpan *found = p4_game_table_input_span(sink->inputs[i], span);
            if (!found) return -1;
            *bytes = p4_game_table_input_span_bytes(found);
            *size = p4_game_table_input_span_size(found);
            return 0;
        }
    return -1;
}

static P4GameTableInput *input_of(P4GameTableInput *const *inputs, size_t count, P4GameTableInputKind kind)
{
    for (size_t i = 0; i < count; ++i)
        if (p4_game_table_input_kind(inputs[i]) == kind) return inputs[i];
    return NULL;
}
static int produce(size_t index, P4GameTableInput *const *inputs, size_t count, char *reason, size_t capacity)
{
    const Selection *selection = derived.selections + index;
    P4TableDerivedSink sink = {{selection->provider, selection->key}, index, 0, inputs, count, 0, ""};
    char error[REASON_MAX] = "";
    derived.active = &sink;
    const int result = selection->producer->produce(selection->producer->user, &sink.request, &sink, error, sizeof error);
    derived.active = NULL;
    if (sink.refused) return refuse_text(reason, capacity, "%s: %s", selection->producer->id, sink.reason);
    if (result == 1)
        return refuse_text(reason, capacity, "%s: source absent%s%s", selection->producer->id, *error ? ": " : "", error);
    if (result) return refuse_text(reason, capacity, "%s: %s", selection->producer->id, *error ? error : "production failed");
    if (!sink.fingerprints) return refuse_text(reason, capacity, "%s reported no source fingerprint", selection->producer->id);
    return 0;
}
static const Replacement *moved_block(P4GameTableInputKind input, uint32_t span)
{
    for (size_t i = 0; i < derived.replacement_count; ++i)
        if (derived.replacements[i].relocated && derived.replacements[i].input == input && derived.replacements[i].span == span)
            return derived.replacements + i;
    return NULL;
}
/* The rows an ENCOUNT block will have once published: its relocated bytes, or
 * the selected input's span. */
static size_t encount_block(P4GameTableInput *const *inputs, size_t count, uint32_t span, uint32_t stride,
    const uint8_t **bytes, const Replacement **moved)
{
    *moved = moved_block(P4_TABLE_INPUT_ENCOUNT, span);
    if (*moved) { *bytes = (*moved)->bytes; return (*moved)->size / stride; }
    const P4GameTableInputSpan *base = p4_game_table_input_span(input_of(inputs, count, P4_TABLE_INPUT_ENCOUNT), span);
    *bytes = base ? p4_game_table_input_span_bytes(base) : NULL;
    return base ? p4_game_table_input_span_size(base) / stride : 0;
}
/* The treasure kinds a record group can yield: bit k set when a record of
 * kind k (flag bit 0) has an item and weight. A missing group yields none. */
static uint32_t record_kinds(const uint8_t *records, size_t rows, uint32_t group)
{
    uint32_t kinds = 0;
    for (uint32_t record = 0; group < rows && record < P4_ENCOUNT_RECORD_COUNT; ++record) {
        const uint8_t *at = records + (size_t)group * P4_ENCOUNT_RECORD_SIZE + record * P4_ENCOUNT_RECORD_STRIDE;
        if (half(at) && half(at + P4_ENCOUNT_RECORD_ITEM)) kinds |= 1u << (at[P4_ENCOUNT_RECORD_FLAGS] & 1u);
    }
    return kinds;
}
/* With an ENCOUNT block relocated, every selector names an existing weighted
 * and record group and every weighted entry an existing formation. A saved
 * dungeon board (save tag 41, restored at 0x0015AC60) re-picks each treasure
 * from its step's record group by the treasure's kind (Encount_FindRecord
 * 0x00161C80, mode 0 or 1), and a kind without weight divides by zero; so
 * every step the selected selectors have keeps exactly the kinds its selected
 * record group had, and a board saved under either table set restores under
 * the other. 0 or the index + 1 of the selection whose block breaks a rule. */
static size_t encount_references(P4GameTableInput *const *inputs, size_t count, char *reason, size_t capacity)
{
    const uint8_t *formations, *selectors, *weighted, *records;
    const Replacement *formation_moved, *selector_moved, *weighted_moved, *record_moved;
    const size_t formation_rows = encount_block(inputs, count, P4_ENCOUNT_FORMATION_SPAN, P4_ENCOUNT_FORMATION_SIZE,
        &formations, &formation_moved);
    const size_t selector_rows = encount_block(inputs, count, P4_ENCOUNT_SELECTOR_SPAN, P4_ENCOUNT_SELECTOR_SIZE,
        &selectors, &selector_moved);
    const size_t weighted_rows = encount_block(inputs, count, P4_ENCOUNT_WEIGHTED_SPAN, P4_ENCOUNT_WEIGHTED_SIZE,
        &weighted, &weighted_moved);
    const size_t record_rows = encount_block(inputs, count, P4_ENCOUNT_RECORD_SPAN, P4_ENCOUNT_RECORD_SIZE,
        &records, &record_moved);
    (void)formations; (void)records;
    const Replacement *any = formation_moved ? formation_moved : selector_moved ? selector_moved :
        weighted_moved ? weighted_moved : record_moved;
    if (!any) return 0;
    for (size_t row = 0; row < selector_rows; ++row) {
        const uint8_t *at = selectors + row * P4_ENCOUNT_SELECTOR_SIZE;
        const uint32_t group = half(at + P4_ENCOUNT_SELECTOR_WEIGHTED), objects = half(at + P4_ENCOUNT_SELECTOR_RECORDS);
        if (group < weighted_rows && objects < record_rows) continue;
        refuse_text(reason, capacity, "encount selector %lu names weighted group %u of %lu and record group %u of %lu",
            (unsigned long)row, (unsigned)group, (unsigned long)weighted_rows, (unsigned)objects, (unsigned long)record_rows);
        return (selector_moved ? selector_moved : any)->owner + 1;
    }
    for (size_t row = 0; row < weighted_rows; ++row)
        for (uint32_t entry = 0; entry < P4_ENCOUNT_WEIGHTED_ENTRIES; ++entry) {
            const uint32_t formation = half(weighted + row * P4_ENCOUNT_WEIGHTED_SIZE + P4_ENCOUNT_WEIGHTED_HEADER + entry * 4u);
            if (formation < formation_rows) continue;
            refuse_text(reason, capacity, "encount weighted group %lu names formation %u of %lu",
                (unsigned long)row, (unsigned)formation, (unsigned long)formation_rows);
            return (weighted_moved ? weighted_moved : any)->owner + 1;
        }
    if (!selector_moved && !record_moved) return 0;
    const P4GameTableInput *encount = input_of(inputs, count, P4_TABLE_INPUT_ENCOUNT);
    const P4GameTableInputSpan *loaded_selectors = p4_game_table_input_span(encount, P4_ENCOUNT_SELECTOR_SPAN);
    const P4GameTableInputSpan *loaded_records = p4_game_table_input_span(encount, P4_ENCOUNT_RECORD_SPAN);
    const size_t steps = p4_game_table_input_span_size(loaded_selectors) / P4_ENCOUNT_SELECTOR_SIZE;
    const size_t loaded_rows = p4_game_table_input_span_size(loaded_records) / P4_ENCOUNT_RECORD_SIZE;
    for (size_t row = 0; row < steps && row < selector_rows; ++row) {
        const uint32_t was = half(p4_game_table_input_span_bytes(loaded_selectors) + row * P4_ENCOUNT_SELECTOR_SIZE +
            P4_ENCOUNT_SELECTOR_RECORDS), now = half(selectors + row * P4_ENCOUNT_SELECTOR_SIZE + P4_ENCOUNT_SELECTOR_RECORDS);
        const uint32_t before = record_kinds(p4_game_table_input_span_bytes(loaded_records), loaded_rows, was);
        const uint32_t after = record_kinds(records, record_rows, now);
        if (before == after) continue;
        refuse_text(reason, capacity, "encount selector %lu's record group %u yields treasure-kind mask %#x, not %#x like the selected group %u",
            (unsigned long)row, (unsigned)now, (unsigned)after, (unsigned)before, (unsigned)was);
        return (record_moved ? record_moved : selector_moved)->owner + 1;
    }
    return 0;
}
int p4_game_tables_derived_compose(P4GameTableInput *const *inputs, size_t count,
    const P4PackageProvider **failed_provider)
{
    if (failed_provider) *failed_provider = NULL;
    if (!derived.selection_count) return P4_TABLE_OK;
    if (derived.replacement_count || derived.composed_count || derived.relocated_count) return P4_TABLE_CONFLICT;
    for (size_t i = 0; i < derived.selection_count; ++i) {
        char reason[REASON_MAX];
        if (!produce(i, inputs, count, reason, sizeof reason)) continue;
        if (failed_provider) *failed_provider = derived.selections[i].provider;
        derived.refused = 1;
        fprintf(stderr, "p4pc: table-derived refused: %s\n", reason);
        p4_package_provider_refuse(derived.selections[i].provider, reason);
        return P4_TABLE_INVALID;
    }
    /* Every production is validated before any input byte changes. */
    char reason[REASON_MAX];
    const size_t referrer = encount_references(inputs, count, reason, sizeof reason);
    if (referrer) {
        const Selection *selection = derived.selections + (referrer - 1);
        if (failed_provider) *failed_provider = selection->provider;
        fprintf(stderr, "p4pc: table-derived refused: %s: %s\n", selection->producer->id, reason);
        p4_package_provider_refuse(selection->provider, reason);
        return P4_TABLE_INVALID;
    }
    for (size_t i = 0; i < derived.replacement_count; ++i) {
        const Replacement *replacement = derived.replacements + i;
        P4GameTableInput *input = input_of(inputs, count, replacement->input);
        const P4GameTableInputSpan *span = p4_game_table_input_span(input, replacement->span);
        if (!span) return P4_TABLE_INVALID;
        if (replacement->relocated) {
            /* The loader keeps loading the original block; the native
             * publisher moves its slot to this list instead. */
            if (derived.relocated_count == RELOCATABLE) return P4_TABLE_INVALID;
            Relocated *entry = derived.relocated + derived.relocated_count++;
            entry->input = input;
            entry->replacement = i;
            P4ContentDigest digest;
            p4_content_digest_begin(&digest);
            p4_content_digest_update(&digest, replacement->bytes, replacement->size);
            p4_content_digest_end(&digest, entry->hex);
            continue;
        }
        uint32_t header;
        stride(replacement->input, replacement->span, &header);
        uint8_t *target = input->bytes + span->offset;
        if (!replacement->stride) memcpy(target, replacement->bytes, span->size);
        else
            for (size_t row = 0; row < replacement->count; ++row)
                memcpy(target + header + (size_t)replacement->ids[row] * replacement->stride,
                    replacement->bytes + row * replacement->stride, replacement->stride);
        size_t slot = 0;
        while (slot < derived.composed_count && derived.composed[slot].input != input) ++slot;
        if (slot == derived.composed_count) derived.composed[derived.composed_count++].input = input;
    }
    for (size_t i = 0; i < derived.composed_count; ++i) {
        P4ContentDigest digest;
        p4_content_digest_begin(&digest);
        p4_content_digest_update(&digest, derived.composed[i].input->bytes, derived.composed[i].input->size);
        p4_content_digest_end(&digest, derived.composed[i].hex);
    }
    return P4_TABLE_OK;
}
/* The family IDs a relocated block names: PERSONA 4 skill halves, ENCOUNT
 * formation enemies and record-group items (0 is none in each). */
static const char *reference(const Replacement *block, size_t at, P4TableDerivedDomain *domain, uint32_t *id)
{
    if (block->input == P4_TABLE_INPUT_PERSONA) {
        if (at >= block->size / 2u) return NULL;
        *domain = P4_TABLE_DERIVED_SKILL;
        *id = half(block->bytes + at * 2u);
        return "skill";
    }
    if (block->span == P4_ENCOUNT_FORMATION_SPAN) {
        const size_t row = at / P4_ENCOUNT_FORMATION_ENEMY_COUNT, slot = at % P4_ENCOUNT_FORMATION_ENEMY_COUNT;
        if (row >= block->size / P4_ENCOUNT_FORMATION_SIZE) return NULL;
        *domain = P4_TABLE_DERIVED_ENEMY;
        *id = half(block->bytes + row * P4_ENCOUNT_FORMATION_SIZE + P4_ENCOUNT_FORMATION_ENEMIES + slot * 2u);
        return "enemy";
    }
    if (block->span == P4_ENCOUNT_RECORD_SPAN) {
        const size_t row = at / P4_ENCOUNT_RECORD_COUNT, record = at % P4_ENCOUNT_RECORD_COUNT;
        if (row >= block->size / P4_ENCOUNT_RECORD_SIZE) return NULL;
        *domain = P4_TABLE_DERIVED_ITEM;
        *id = half(block->bytes + row * P4_ENCOUNT_RECORD_SIZE + record * P4_ENCOUNT_RECORD_STRIDE + P4_ENCOUNT_RECORD_ITEM);
        return "item";
    }
    return NULL;
}
int p4_game_tables_derived_check_references(int (*known)(const void *context, P4TableDerivedDomain domain, uint32_t id),
    const void *context, const P4PackageProvider **failed_provider)
{
    if (failed_provider) *failed_provider = NULL;
    for (size_t i = 0; i < derived.relocated_count; ++i) {
        const Replacement *block = derived.replacements + derived.relocated[i].replacement;
        P4TableDerivedDomain domain;
        uint32_t id;
        const char *name;
        for (size_t at = 0; (name = reference(block, at, &domain, &id)) != NULL; ++at) {
            if (!id || known(context, domain, id)) continue;
            const Selection *selection = derived.selections + block->owner;
            char reason[REASON_MAX];
            snprintf(reason, sizeof reason, "%s: %s span %u names %s %u, which no package declares",
                selection->producer->id, input_names[block->input], (unsigned)block->span, name, (unsigned)id);
            fprintf(stderr, "p4pc: table-derived refused: %s\n", reason);
            p4_package_provider_refuse(selection->provider, reason);
            if (failed_provider) *failed_provider = selection->provider;
            return P4_TABLE_NOT_FOUND;
        }
    }
    return P4_TABLE_OK;
}
int p4_game_tables_derived_relocated(P4GameTableInputKind input, uint32_t span,
    const P4GameTableInput **loaded, const uint8_t **bytes, size_t *size)
{
    for (size_t i = 0; i < derived.relocated_count; ++i) {
        const Replacement *replacement = derived.replacements + derived.relocated[i].replacement;
        if (replacement->input != input || replacement->span != span) continue;
        if (loaded) *loaded = derived.relocated[i].input;
        if (bytes) *bytes = replacement->bytes;
        if (size) *size = replacement->size;
        return 1;
    }
    return 0;
}

static void identity_text(P4ContentDigest *digest, const char *text)
{
    p4_content_digest_update(digest, text, strlen(text) + 1);
}
static void identity_word(P4ContentDigest *digest, uint64_t value)
{
    uint8_t bytes[8];
    for (unsigned i = 0; i < 8; ++i) bytes[i] = (uint8_t)(value >> (8u * i));
    p4_content_digest_update(digest, bytes, sizeof bytes);
}
void p4_game_tables_derived_identity(P4ContentDigest *digest)
{
    identity_text(digest, "p4pc-table-derived-v1");
    identity_word(digest, derived.selection_count);
    for (size_t i = 0; i < derived.selection_count; ++i) {
        const Selection *selection = derived.selections + i;
        const P4PackageInfo *info = p4_package_provider_info(selection->provider);
        identity_text(digest, info ? info->id : "");
        identity_text(digest, selection->key);
        identity_text(digest, selection->producer->id);
        identity_word(digest, selection->producer->revision);
        identity_word(digest, selection->inputs);
        for (size_t j = 0; j < derived.fingerprint_count; ++j)
            if (derived.fingerprints[j].owner == i) {
                identity_text(digest, derived.fingerprints[j].label);
                identity_text(digest, derived.fingerprints[j].hex);
            }
        /* A recorded source reaches gameplay through p4.tables.source. */
        for (size_t j = 0; j < derived.source_count; ++j)
            if (derived.sources[j].owner == i) {
                identity_text(digest, derived.sources[j].name);
                identity_text(digest, derived.sources[j].hex);
            }
    }
    identity_word(digest, derived.composed_count);
    for (size_t i = 0; i < derived.composed_count; ++i) {
        identity_word(digest, (uint64_t)derived.composed[i].input->kind);
        identity_text(digest, derived.composed[i].hex);
    }
    identity_word(digest, derived.relocated_count);
    for (size_t i = 0; i < derived.relocated_count; ++i) {
        const Replacement *replacement = derived.replacements + derived.relocated[i].replacement;
        identity_word(digest, (uint64_t)replacement->input);
        identity_word(digest, replacement->span);
        identity_text(digest, derived.relocated[i].hex);
    }
}
static const Composed *composed(const P4GameTableInput *input)
{
    for (size_t i = 0; input && i < derived.composed_count; ++i)
        if (derived.composed[i].input == input) return derived.composed + i;
    return NULL;
}
int p4_game_tables_derived_matches(const P4GameTableInput *input, const void *bytes, size_t size)
{
    return composed(input) && bytes && size == input->size && !memcmp(bytes, input->bytes, size);
}
void p4_game_tables_derived_rewrite(const P4GameTableInput *input, uint8_t *bytes, size_t size)
{
    if (composed(input) && bytes && bytes != input->bytes && size == input->size && memcmp(bytes, input->bytes, size))
        memcpy(bytes, input->bytes, size);
}
void p4_game_tables_derived_reset(void)
{
    for (size_t i = 0; i < derived.replacement_count; ++i) {
        free(derived.replacements[i].ids);
        free(derived.replacements[i].bytes);
    }
    free(derived.replacements);
    free(derived.fingerprints);
    for (size_t i = 0; i < derived.source_count; ++i) free(derived.sources[i].bytes);
    free(derived.sources);
    free(derived.selections);
    if (derived.charged) p4_packages_memory_release(derived.charged);
    memset(&derived, 0, sizeof derived);
}
