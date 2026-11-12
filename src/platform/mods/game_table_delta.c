/* [table-delta KEY]: a package's named-field deltas over the composed retail
 * rows (docs/modding/tables.md "Table deltas"). This file reads the section
 * and decodes its XML file; game_table_derived.c records the deltas and
 * applies them inside the derived compose stage. */
#include "game_table_derived_internal.h"
#include "p4pc/content_digest.h"
#include "p4pc/mod_setting_gate.h"
#include "p4pc/package_memory.h"
#include "p4pc/packages.h"
#include "../../../third_party/yxml/yxml.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A skill-delta has at most ten attributes: its id and nine fields. */
/* DELTA_ prefixes: Android's bionic <limits.h> defines NAME_MAX as a macro. */
enum { DELTA_FILE_MAX = 256u * 1024u, DELTA_MAX = 2048, ATTRIBUTES = 10, DELTA_NAME_MAX = 32, DELTA_VALUE_MAX = 16 };

typedef struct Attribute {
    char name[DELTA_NAME_MAX], value[DELTA_VALUE_MAX];
    size_t length;
    int used;
} Attribute;
typedef struct Decoder {
    yxml_t xml;
    unsigned char stack[1024];
    char element[DELTA_NAME_MAX];
    Attribute attributes[ATTRIBUTES];
    size_t attribute_count;
    unsigned depth, root;
    P4TableDelta *deltas;
    size_t count;
    char *error;
    size_t capacity;
} Decoder;

static int refuse(char *error, size_t capacity, const char *format, ...)
{
    if (error && capacity && !*error) {
        va_list args;
        va_start(args, format);
        vsnprintf(error, capacity, format, args);
        va_end(args);
    }
    return -1;
}
static int fail(Decoder *decoder, const char *reason)
{
    return refuse(decoder->error, decoder->capacity, "table-delta: %s (line %lu)", reason,
        (unsigned long)decoder->xml.line);
}
static const char *take(Decoder *decoder, const char *name)
{
    for (size_t i = 0; i < decoder->attribute_count; ++i)
        if (!strcmp(decoder->attributes[i].name, name)) {
            decoder->attributes[i].used = 1;
            return decoder->attributes[i].value;
        }
    return NULL;
}
/* Decimal or 0x hexadecimal u32, as table sources write them. */
static int number(const char *text, uint32_t *out)
{
    if (!text || !*text) return 0;
    unsigned base = 10;
    if (text[0] == '0' && text[1] == 'x') { base = 16; text += 2; if (!*text) return 0; }
    uint32_t value = 0;
    do {
        const unsigned char c = (unsigned char)*text++;
        const unsigned digit = c >= '0' && c <= '9' ? (unsigned)(c - '0') :
            c >= 'a' && c <= 'f' ? c - 'a' + 10u : c >= 'A' && c <= 'F' ? c - 'A' + 10u : 16u;
        if (digit >= base || value > (UINT32_MAX - digit) / base) return 0;
        value = value * base + digit;
    } while (*text);
    *out = value;
    return 1;
}
static int all_used(const Decoder *decoder)
{
    for (size_t i = 0; i < decoder->attribute_count; ++i) if (!decoder->attributes[i].used) return 0;
    return 1;
}
static int push(Decoder *decoder, P4TableDeltaKind kind, uint32_t row, uint32_t skill, uint32_t value)
{
    if (decoder->count == DELTA_MAX) return fail(decoder, "too many deltas");
    decoder->deltas[decoder->count++] = (P4TableDelta){kind, row, skill, value};
    return 0;
}
/* <skill-delta id= [power=] [cost=] [accuracy=] [critical=] [min-hits=]
 * [max-hits=] [split=] [ailment-rate=] [max-heal=]/>: one delta per named field.
 * <learn-delta persona= skill= level=/> moves a learn event, <learn-delta
 * persona= skill= replace=/> teaches another skill there, and <learn-delta
 * persona= add= level=/> adds one. Ranges are checked when the deltas are
 * recorded; hit, ailment-rate and max-heal fields also check the composed row. */
static int record(Decoder *decoder)
{
    if (!strcmp(decoder->element, "skill-delta")) {
        static const struct { const char *name; P4TableDeltaKind kind; } fields[] = {
            {"power", P4_TABLE_DELTA_SKILL_POWER}, {"cost", P4_TABLE_DELTA_SKILL_COST},
            {"accuracy", P4_TABLE_DELTA_SKILL_ACCURACY}, {"critical", P4_TABLE_DELTA_SKILL_CRITICAL},
            {"min-hits", P4_TABLE_DELTA_SKILL_MIN_HITS}, {"max-hits", P4_TABLE_DELTA_SKILL_MAX_HITS},
            {"split", P4_TABLE_DELTA_SKILL_SPLIT}, {"ailment-rate", P4_TABLE_DELTA_SKILL_AILMENT_RATE},
            {"max-heal", P4_TABLE_DELTA_SKILL_MAX_HEAL},
        };
        uint32_t id = 0, value = 0;
        if (!number(take(decoder, "id"), &id)) return fail(decoder, "skill-delta needs a numeric id");
        size_t named = 0;
        for (size_t i = 0; i < sizeof fields / sizeof fields[0]; ++i) {
            const char *text = take(decoder, fields[i].name);
            if (!text) continue;
            if (!number(text, &value)) return fail(decoder, "skill-delta field values are numbers");
            if (push(decoder, fields[i].kind, id, 0, value)) return -1;
            ++named;
        }
        if (!named)
            return fail(decoder, "skill-delta names no field (power, cost, accuracy, critical, min-hits, max-hits, split, "
                "ailment-rate or max-heal)");
    } else if (!strcmp(decoder->element, "learn-delta")) {
        uint32_t persona = 0, skill = 0, value = 0;
        if (!number(take(decoder, "persona"), &persona)) return fail(decoder, "learn-delta needs a numeric persona");
        const char *moved = take(decoder, "skill"), *added = take(decoder, "add");
        const char *level = take(decoder, "level"), *replace = take(decoder, "replace");
        P4TableDeltaKind kind;
        if (moved && !added && level && !replace) kind = P4_TABLE_DELTA_LEARN_LEVEL;
        else if (moved && !added && !level && replace) kind = P4_TABLE_DELTA_LEARN_REPLACE;
        else if (!moved && added && level && !replace) kind = P4_TABLE_DELTA_LEARN_ADD;
        else return fail(decoder, "learn-delta takes skill and level, skill and replace, or add and level");
        if (!number(moved ? moved : added, &skill) || !number(replace ? replace : level, &value))
            return fail(decoder, "learn-delta values are numbers");
        if (push(decoder, kind, persona, skill, value)) return -1;
    } else return fail(decoder, "unsupported table-delta record");
    if (!all_used(decoder)) return fail(decoder, "unknown table-delta attribute");
    return 0;
}
static int root(Decoder *decoder)
{
    uint32_t schema = 0;
    if (strcmp(decoder->element, "p4-table-deltas") || !number(take(decoder, "schema"), &schema) || schema != 1 ||
        !all_used(decoder))
        return fail(decoder, "require a p4-table-deltas root with schema 1");
    decoder->root = 1;
    return 0;
}
static int decode(Decoder *decoder, const uint8_t *bytes, size_t size)
{
    yxml_init(&decoder->xml, decoder->stack, sizeof decoder->stack);
    for (size_t i = 0; i < size; ++i) {
        if (!bytes[i]) return fail(decoder, "embedded NUL");
        const yxml_ret_t token = yxml_parse(&decoder->xml, bytes[i]);
        if (token < 0) return fail(decoder, "malformed XML");
        if (token == YXML_ELEMSTART) {
            if (decoder->depth == 1 && !decoder->root && root(decoder)) return -1;
            if (++decoder->depth > 2) return fail(decoder, "nested records are unsupported");
            if (strlen(decoder->xml.elem) >= DELTA_NAME_MAX) return fail(decoder, "record name is too long");
            strcpy(decoder->element, decoder->xml.elem);
            decoder->attribute_count = 0;
        } else if (token == YXML_ATTRSTART) {
            if (decoder->attribute_count == ATTRIBUTES) return fail(decoder, "too many attributes");
            if (strlen(decoder->xml.attr) >= DELTA_NAME_MAX) return fail(decoder, "attribute name is too long");
            for (size_t n = 0; n < decoder->attribute_count; ++n)
                if (!strcmp(decoder->attributes[n].name, decoder->xml.attr)) return fail(decoder, "duplicate attribute");
            Attribute *attribute = decoder->attributes + decoder->attribute_count++;
            *attribute = (Attribute){0};
            strcpy(attribute->name, decoder->xml.attr);
        } else if (token == YXML_ATTRVAL) {
            Attribute *attribute = decoder->attributes + decoder->attribute_count - 1;
            const size_t length = strlen(decoder->xml.data);
            if (attribute->length + length >= DELTA_VALUE_MAX) return fail(decoder, "attribute value is too long");
            memcpy(attribute->value + attribute->length, decoder->xml.data, length + 1);
            attribute->length += length;
        } else if (token == YXML_CONTENT) {
            for (const char *p = decoder->xml.data; *p; ++p)
                if (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') return fail(decoder, "records use attributes, not text");
        } else if (token == YXML_ELEMEND) {
            if (decoder->depth == 2 ? record(decoder) : decoder->root ? 0 : root(decoder)) return -1;
            --decoder->depth;
        } else if (token != YXML_OK && token != YXML_ATTREND)
            return fail(decoder, "processing instructions are unsupported");
    }
    if (yxml_eof(&decoder->xml) != YXML_OK || decoder->depth || !decoder->root || !decoder->count)
        return fail(decoder, "incomplete or empty delta file");
    return 0;
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
int p4_game_tables_delta_definition(const P4PackageProvider *provider, const P4PackageField *fields,
    size_t count, uint32_t *inputs, char *error, size_t capacity)
{
    if (error && capacity) *error = 0;
    if (!provider || !fields || !count || !inputs) return refuse(error, capacity, "Missing table-delta declaration");
    const char *file = NULL, *revision = NULL;
    size_t files = 0, revisions = 0;
    for (size_t i = 0; i < count; ++i) {
        const char *name = fields[i].key;
        if (!name) return refuse(error, capacity, "Missing table-delta declaration field");
        if (!strcmp(name, "file")) { file = fields[i].value; ++files; }
        else if (!strcmp(name, "revision")) { revision = fields[i].value; ++revisions; }
        else if (!gate_field(name)) return refuse(error, capacity, "Unknown table-delta declaration field %s", name);
    }
    if (files != 1 || revisions != 1 || !file || !*file) return refuse(error, capacity, "table-delta requires one file and revision");
    char *end = NULL;
    const unsigned long declared = revision && *revision >= '1' && *revision <= '9' ? strtoul(revision, &end, 10) : 0;
    if (!declared || !end || *end || declared > UINT32_MAX)
        return refuse(error, capacity, "table-delta requires a nonzero decimal revision");
    P4PackageFile *input = NULL;
    if (p4_package_file_open(provider, file, DELTA_FILE_MAX, &input))
        return refuse(error, capacity, "table-delta file must be an authorized contained regular file within 256 KiB");
    const uint64_t size = p4_package_file_size(input);
    /* One allocation: the file, then (aligned) the decoded deltas. */
    const size_t aligned = ((size_t)size + 15u) & ~(size_t)15u, total = aligned + DELTA_MAX * sizeof(P4TableDelta);
    if (!size || size > DELTA_FILE_MAX || p4_packages_memory_reserve(total)) {
        p4_package_file_close(input);
        return refuse(error, capacity, "table-delta file is empty or cannot be read");
    }
    uint8_t *bytes = malloc(total);
    if (!bytes) {
        p4_packages_memory_cancel(total);
        p4_package_file_close(input);
        return refuse(error, capacity, "table-delta allocation failed");
    }
    p4_packages_memory_commit(total);
    P4TableDelta *deltas = (P4TableDelta *)(void *)(bytes + aligned);
    int result = -1;
    if (p4_package_file_read_at(input, 0, bytes, (size_t)size)) refuse(error, capacity, "Could not read the table-delta file");
    else {
        Decoder decoder = {.deltas = deltas, .error = error, .capacity = capacity};
        if (!decode(&decoder, bytes, (size_t)size)) {
            P4ContentDigest digest;
            char hex[P4_CONTENT_DIGEST_HEX + 1];
            p4_content_digest_begin(&digest);
            p4_content_digest_update(&digest, bytes, (size_t)size);
            p4_content_digest_end(&digest, hex);
            result = p4_game_tables_derived_delta_add(provider, fields[0].argument, (uint32_t)declared, hex, deltas,
                decoder.count, inputs, error, capacity);
        }
    }
    free(bytes);
    p4_packages_memory_release(total);
    if (p4_package_file_close(input) && !result)
        result = refuse(error, capacity, "table-delta file changed or could not close after decoding");
    return result;
}
