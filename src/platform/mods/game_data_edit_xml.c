/* data/edits.xml: the data-only twin of data.luau's p4.data.edit
 * (docs/modding/systems/data-and-edits.md). Same rows, names, fields and
 * layer rules; every record goes through the sink: the game's applies with
 * p4_data_edit_set/learn (game_data_edit.c), p4mod check's only decodes.
 * Nothing here reads the catalog or the tables itself.
 *
 *   <p4-edits schema="1">
 *     <skill name="Agilao" power="100" sp="3"/>
 *     <enemy name="Lying Hablerie" hp="300" fire="weak"><drop slot="1" item="Medicine" chance="50"/></enemy>
 *     <persona name="Tomoe"><learn level="12" skill="Bufula"/></persona>
 *     <item name="Medicine" price="100"/>
 *   </p4-edits> */
#include "game_data_edit_internal.h"
#include "p4pc/game_data_edit.h"
#include "../../../third_party/yxml/yxml.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { NAME_SIZE = 32, VALUE_SIZE = 128, ATTRIBUTES = 40 };

typedef struct Attribute {
    char name[NAME_SIZE], value[VALUE_SIZE];
    size_t length;
    int used;
} Attribute;
typedef struct Decoder {
    yxml_t xml;
    unsigned char stack[1024];
    unsigned depth;
    int root, row_open, row_applied;
    char element[NAME_SIZE], row_element[NAME_SIZE];
    Attribute attributes[ATTRIBUTES];
    size_t attribute_count, records;
    P4DataKind kind;
    size_t index;
    const P4DataEditsXmlSink *sink;
    char *error;
    size_t capacity;
} Decoder;

static int fail(Decoder *d, const char *format, ...)
{
    char reason[768];
    va_list args;
    va_start(args, format);
    vsnprintf(reason, sizeof reason, format, args);
    va_end(args);
    snprintf(d->error, d->capacity, "line %lu: %s", (unsigned long)d->xml.line, reason);
    return -1;
}
static const char *take(Decoder *d, const char *name)
{
    for (size_t i = 0; i < d->attribute_count; ++i)
        if (!strcmp(d->attributes[i].name, name)) { d->attributes[i].used = 1; return d->attributes[i].value; }
    return NULL;
}
static int unused(Decoder *d, const char *what)
{
    for (size_t i = 0; i < d->attribute_count; ++i)
        if (!d->attributes[i].used) return fail(d, "<%s> has no attribute \"%s\" (%s)", d->element, d->attributes[i].name, what);
    return 0;
}
/* A decimal number ("50", "12.5") or text. */
static P4DataEditValue value_of(const char *text)
{
    char *end = NULL;
    const double number = strtod(text, &end);
    const int numeric = *text && end && !*end && isfinite(number) &&
        (text[0] == '-' || text[0] == '.' || (text[0] >= '0' && text[0] <= '9'));
    return numeric ? (P4DataEditValue){0, number, NULL} : (P4DataEditValue){1, 0, text};
}
/* A whole number that fits uint32 (base 0: 0x hex and leading-0 octal, as strtoul), starting
 * with a digit: no sign, no space, no wrap of a larger value onto a small one. */
static int whole_u32(const char *text, int base, uint32_t *out)
{
    if (!text || *text < '0' || *text > '9') return 0;
    char *end = NULL;
    const unsigned long long value = strtoull(text, &end, base);
    if (!end || *end || value > UINT32_MAX) return 0;
    *out = (uint32_t)value;
    return 1;
}

/* <skill|persona|enemy|item|floor|formation|encounters|treasure
 * name=|key=|id= ...>: the row, then its fields; rename= sets the display
 * name (name= finds the row). */
typedef struct Field { const char *attribute, *path; } Field;
static const Field skill_fields[] = {
    {"rename", "name"}, {"power", "power"}, {"sp", "cost.sp"}, {"hp-percent", "cost.hp_percent"}, {"accuracy", "accuracy"},
    {"critical", "critical"}, {"hits-min", "hits.min"}, {"hits-max", "hits.max"}, {"element", "element"},
    {"ailment", "ailment.status"}, {"ailment-chance", "ailment.chance"},
}, enemy_fields[] = {
    {"rename", "name"}, {"level", "level"}, {"hp", "hp"}, {"sp", "sp"}, {"st", "stats.st"}, {"ma", "stats.ma"},
    {"en", "stats.en"}, {"ag", "stats.ag"}, {"lu", "stats.lu"}, {"exp", "exp"}, {"yen", "yen"},
}, persona_fields[] = {
    {"rename", "name"}, {"level", "level"}, {"st", "stats.st"}, {"ma", "stats.ma"}, {"en", "stats.en"},
    {"ag", "stats.ag"}, {"lu", "stats.lu"},
}, item_fields[] = {
    {"rename", "name"}, {"price", "price"}, {"attack", "attack"}, {"hit", "hit"}, {"defense", "defense"},
    {"evasion", "evasion"}, {"skill", "skill"}, {"sold", "shop.sold"}, {"unlock", "shop.unlock"},
}, floor_fields[] = {
    {"spawn", "spawn"}, {"refill", "refill"}, {"chests", "chests"}, {"encounters", "encounters"}, {"treasure", "treasure"},
}, encounters_fields[] = {
    {"normal-rate", "rates.normal"}, {"special-rate", "rates.special"}, {"rare-rate", "rates.rare"},
    {"alternate-normal-rate", "alternate.rates.normal"}, {"alternate-special-rate", "alternate.rates.special"},
    {"alternate-rare-rate", "alternate.rates.rare"},
};
#define FIELDS(list) list, sizeof list / sizeof list[0]
static const struct {
    const char *element;
    P4DataKind kind;
    const Field *fields;
    size_t count;
    const char *known;
} row_kinds[] = {
    {"skill", P4_DATA_SKILL, FIELDS(skill_fields),
        "rename, power, sp, hp-percent, accuracy, critical, hits-min, hits-max, element, ailment, ailment-chance"},
    {"persona", P4_DATA_PERSONA, FIELDS(persona_fields),
        "rename, level, st, ma, en, ag, lu, and an element or ailment name for each affinity"},
    {"enemy", P4_DATA_ENEMY, FIELDS(enemy_fields),
        "rename, level, hp, sp, st, ma, en, ag, lu, exp, yen, and an element or ailment name for each affinity"},
    {"item", P4_DATA_ITEM, FIELDS(item_fields), "rename, price, attack, hit, defense, evasion, skill, sold, from, unlock"},
    {"floor", P4_DATA_FLOOR, FIELDS(floor_fields), "spawn, refill, chests, encounters, treasure"},
    {"formation", P4_DATA_FORMATION, NULL, 0, "none; it holds <enemy slot= name=/>"},
    {"encounters", P4_DATA_ENCOUNTERS, FIELDS(encounters_fields),
        "normal-rate, special-rate, rare-rate and alternate-<the same>"},
    {"treasure", P4_DATA_TREASURE, NULL, 0, "none; it holds <record slot= item= weight= locked=/>"},
};
static int set(Decoder *d, const char *path, const char *text)
{
    const P4DataEditValue value = value_of(text);
    char reason[768];
    if (d->sink->set(d->kind, d->index, path, &value, reason, sizeof reason)) return fail(d, "%s", reason);
    ++d->records;
    return 0;
}
static int resolve(Decoder *d, P4DataKind kind, const char *query, size_t *index)
{
    char message[512];
    if (d->sink->resolve(kind, query, index, message, sizeof message)) return fail(d, "%s", message);
    return 0;
}
static int row_open(Decoder *d)
{
    size_t which = 0;
    while (which < sizeof row_kinds / sizeof row_kinds[0] && strcmp(d->element, row_kinds[which].element)) ++which;
    if (which == sizeof row_kinds / sizeof row_kinds[0])
        return fail(d, "<%s> is not a row (skill, persona, enemy, item, floor, formation, encounters or treasure)",
            d->element);
    d->kind = row_kinds[which].kind;
    const char *name = take(d, "name"), *key = take(d, "key"), *id = take(d, "id");
    if (!!name + !!key + !!id != 1) return fail(d, "<%s> needs exactly one of name, key or id", d->element);
    if (id) {
        uint32_t number;
        if (!whole_u32(id, 0, &number) || d->sink->index(d->kind, number, &d->index))
            return fail(d, "no %s has the id %s", d->element, id);
    } else if (resolve(d, d->kind, name ? name : key, &d->index)) return -1;
    /* <item from="M/D">: the item-store date as two fields. */
    const char *from = d->kind == P4_DATA_ITEM ? take(d, "from") : NULL;
    if (from) {
        uint32_t month, day;
        char text[16], month_text[16];
        const char *slash = strchr(from, '/');
        const size_t month_length = slash ? (size_t)(slash - from) : 0;
        if (!slash || month_length >= sizeof month_text)
            return fail(d, "<item> from=\"%s\" is not a month/day such as 4/20", from);
        memcpy(month_text, from, month_length);
        month_text[month_length] = 0;
        if (!whole_u32(month_text, 10, &month) || !whole_u32(slash + 1, 10, &day))
            return fail(d, "<item> from=\"%s\" is not a month/day such as 4/20", from);
        snprintf(text, sizeof text, "%lu", (unsigned long)month);
        if (set(d, "shop.from.month", text)) return -1;
        snprintf(text, sizeof text, "%lu", (unsigned long)day);
        if (set(d, "shop.from.day", text)) return -1;
    }
    const char *element_attribute = NULL;
    for (size_t i = 0; i < d->attribute_count; ++i) {
        Attribute *a = d->attributes + i;
        if (a->used) continue;
        const char *path = NULL;
        char affinity[NAME_SIZE + 16];
        for (size_t f = 0; f < row_kinds[which].count && !path; ++f)
            if (!strcmp(a->name, row_kinds[which].fields[f].attribute)) path = row_kinds[which].fields[f].path;
        uint32_t element;
        if (!path && (d->kind == P4_DATA_ENEMY || d->kind == P4_DATA_PERSONA) && !p4_data_element_parse(a->name, &element) &&
            element < P4_DATA_AFFINITY_COLUMNS) {
            snprintf(affinity, sizeof affinity, "affinities.%s", p4_data_element_name(element));
            path = affinity;
        }
        if (!path) {
            element_attribute = a->name;
            break;
        }
        a->used = 1;
        if (set(d, path, a->value)) return -1;
    }
    if (element_attribute)
        return fail(d, "<%s> has no attribute \"%s\" (%s)", d->element, element_attribute, row_kinds[which].known);
    d->row_applied = 1;
    ++d->records;
    return 0;
}
static int child(Decoder *d)
{
    char reason[768], path[32];
    if (d->kind == P4_DATA_ENEMY && (!strcmp(d->element, "drop") || !strcmp(d->element, "skill"))) {
        const char *slot = take(d, "slot");
        char *end = NULL;
        const unsigned long n = slot ? strtoul(slot, &end, 10) : 0;
        const int drop = d->element[0] == 'd';
        if (!n || !end || *end || n > (drop ? P4_DATA_ENEMY_DROPS : P4_DATA_ENEMY_SKILLS))
            return fail(d, "<%s> needs a slot from 1 to %u", d->element, drop ? P4_DATA_ENEMY_DROPS : P4_DATA_ENEMY_SKILLS);
        if (drop) {
            const char *item = take(d, "item"), *chance = take(d, "chance");
            if (!item && !chance) return fail(d, "<drop> sets item, chance or both");
            snprintf(path, sizeof path, "drops.%lu.item", n);
            if (item && set(d, path, item)) return -1;
            snprintf(path, sizeof path, "drops.%lu.chance", n);
            if (chance && set(d, path, chance)) return -1;
            return unused(d, "slot, item, chance");
        }
        const char *name = take(d, "name");
        if (!name) return fail(d, "<skill> needs a name (or \"none\")");
        snprintf(path, sizeof path, "skills.%lu", n);
        return set(d, path, name) || unused(d, "slot, name");
    }
    if (d->kind == P4_DATA_PERSONA) {
        static const struct { const char *element; P4DataLearnOp op; const char *value; } ops[] = {
            {"learn", P4_DATA_LEARN_ADD, "level"}, {"forget", P4_DATA_LEARN_REMOVE, NULL},
            {"move", P4_DATA_LEARN_MOVE, "level"}, {"replace", P4_DATA_LEARN_REPLACE, "with"},
        };
        for (size_t i = 0; i < sizeof ops / sizeof ops[0]; ++i) {
            if (strcmp(d->element, ops[i].element)) continue;
            const char *skill = take(d, "skill"), *value = ops[i].value ? take(d, ops[i].value) : NULL;
            size_t skill_index, other;
            uint32_t argument = 0;
            if (!skill || (ops[i].value && !value)) return fail(d, "<%s> needs skill%s%s", d->element, ops[i].value ? " and " : "",
                ops[i].value ? ops[i].value : "");
            if (resolve(d, P4_DATA_SKILL, skill, &skill_index)) return -1;
            if (ops[i].op == P4_DATA_LEARN_REPLACE) {
                if (resolve(d, P4_DATA_SKILL, value, &other)) return -1;
                argument = d->sink->id(P4_DATA_SKILL, other);
            } else if (value) {
                /* A level past uint32 is out of range, not a wrapped small level: the clamp keeps
                   it above 99 so the learnset rule refuses it with the range it allows. */
                char *end = NULL;
                const unsigned long long level = strtoull(value, &end, 10);
                if (!*value || *value < '0' || *value > '9' || !end || *end)
                    return fail(d, "<%s> level must be a whole number", d->element);
                argument = level > UINT32_MAX ? UINT32_MAX : (uint32_t)level;
            }
            if (unused(d, ops[i].value ? (ops[i].op == P4_DATA_LEARN_REPLACE ? "skill, with" : "skill, level") : "skill")) return -1;
            if (d->sink->learn(d->index, ops[i].op, d->sink->id(P4_DATA_SKILL, skill_index), argument, reason, sizeof reason))
                return fail(d, "%s", reason);
            ++d->records;
            return 0;
        }
    }
    if (d->kind == P4_DATA_ITEM && !strcmp(d->element, "unlock")) {
        const char *slot = take(d, "slot"), *material = take(d, "material"), *count = take(d, "count");
        char *end = NULL;
        const unsigned long n = slot ? strtoul(slot, &end, 10) : 0;
        if (!n || !end || *end || n > P4_DATA_ITEM_UNLOCKS)
            return fail(d, "<unlock> needs a slot from 1 to %u", P4_DATA_ITEM_UNLOCKS);
        if (!material && !count) return fail(d, "<unlock> sets material, count or both");
        snprintf(path, sizeof path, "shop.unlock.%lu.material", n);
        if (material && set(d, path, material)) return -1;
        snprintf(path, sizeof path, "shop.unlock.%lu.count", n);
        if (count && set(d, path, count)) return -1;
        return unused(d, "slot, material, count");
    }
    /* <formation><enemy slot= name=/>, <encounters><normal|special|rare slot=
     * formation= weight= alternate=/>, <treasure><record slot= item= weight=
     * locked=/>. */
    if (d->kind == P4_DATA_FORMATION && !strcmp(d->element, "enemy")) {
        const char *slot = take(d, "slot"), *name = take(d, "name");
        char *end = NULL;
        const unsigned long n = slot ? strtoul(slot, &end, 10) : 0;
        if (!n || !end || *end || n > P4_DATA_FORMATION_ENEMIES)
            return fail(d, "<enemy> needs a slot from 1 to %u", P4_DATA_FORMATION_ENEMIES);
        if (!name) return fail(d, "<enemy> needs a name (or \"none\")");
        snprintf(path, sizeof path, "enemies.%lu", n);
        return set(d, path, name) || unused(d, "slot, name");
    }
    if (d->kind == P4_DATA_ENCOUNTERS) {
        for (unsigned c = 0; c < P4_DATA_ENCOUNTER_CLASSES; ++c) {
            const char *class_name = p4_data_encounter_class_name((P4DataEncounterClass)c);
            if (strcmp(d->element, class_name)) continue;
            uint32_t first, count;
            p4_data_encounter_class_slots((P4DataEncounterClass)c, &first, &count);
            const char *slot = take(d, "slot"), *formation = take(d, "formation"), *weight = take(d, "weight");
            const char *alternate = take(d, "alternate");
            char *end = NULL;
            const unsigned long n = slot ? strtoul(slot, &end, 10) : 0;
            if (!n || !end || *end || n > count) return fail(d, "<%s> needs a slot from 1 to %u", class_name, (unsigned)count);
            if (alternate && strcmp(alternate, "true") && strcmp(alternate, "false"))
                return fail(d, "<%s> alternate is true or false", class_name);
            if (!formation && !weight) return fail(d, "<%s> sets formation, weight or both", class_name);
            const char *prefix = alternate && !strcmp(alternate, "true") ? "alternate." : "";
            snprintf(path, sizeof path, "%s%s.%lu.formation", prefix, class_name, n);
            if (formation && set(d, path, formation)) return -1;
            snprintf(path, sizeof path, "%s%s.%lu.weight", prefix, class_name, n);
            if (weight && set(d, path, weight)) return -1;
            return unused(d, "slot, formation, weight, alternate");
        }
    }
    if (d->kind == P4_DATA_TREASURE && !strcmp(d->element, "record")) {
        const char *slot = take(d, "slot"), *item = take(d, "item"), *weight = take(d, "weight"), *locked = take(d, "locked");
        char *end = NULL;
        const unsigned long n = slot ? strtoul(slot, &end, 10) : 0;
        if (!n || !end || *end || n > P4_DATA_TREASURE_RECORDS)
            return fail(d, "<record> needs a slot from 1 to %u", P4_DATA_TREASURE_RECORDS);
        if (!item && !weight && !locked) return fail(d, "<record> sets item, weight, locked or more");
        snprintf(path, sizeof path, "items.%lu.item", n);
        if (item && set(d, path, item)) return -1;
        snprintf(path, sizeof path, "items.%lu.weight", n);
        if (weight && set(d, path, weight)) return -1;
        snprintf(path, sizeof path, "items.%lu.locked", n);
        if (locked && set(d, path, locked)) return -1;
        return unused(d, "slot, item, weight, locked");
    }
    return fail(d, "<%s> cannot hold <%s>%s", d->row_element, d->element,
        d->kind == P4_DATA_ENEMY ? " (enemies hold <skill> and <drop>)" :
        d->kind == P4_DATA_PERSONA ? " (Personas hold <learn>, <forget>, <move> and <replace>)" :
        d->kind == P4_DATA_ITEM ? " (items hold <unlock>)" :
        d->kind == P4_DATA_FORMATION ? " (formations hold <enemy>)" :
        d->kind == P4_DATA_ENCOUNTERS ? " (encounter groups hold <normal>, <special> and <rare>)" :
        d->kind == P4_DATA_TREASURE ? " (treasure groups hold <record>)" : "");
}

/* Records apply when their attributes are complete: a row before its first
 * child or at its end, a child at its end. */
static int complete(Decoder *d)
{
    if (d->depth == 1) {
        char *end = NULL;
        const char *schema = take(d, "schema");
        if (strcmp(d->element, "p4-edits") || !schema || strtoul(schema, &end, 10) != 1 || !end || *end)
            return fail(d, "the root must be <p4-edits schema=\"1\">");
        if (unused(d, "schema")) return -1;
        d->root = 1;
        return 0;
    }
    if (d->depth == 2) {
        if (d->row_applied) return 0;
        return row_open(d);
    }
    return child(d);
}
int p4_data_edits_xml_decode(const uint8_t *bytes, size_t size, const P4DataEditsXmlSink *sink, char *error, size_t capacity)
{
    Decoder *d = calloc(1, sizeof *d);
    if (!d) { snprintf(error, capacity, "out of memory"); return -1; }
    d->sink = sink; d->error = error; d->capacity = capacity;
    yxml_init(&d->xml, d->stack, sizeof d->stack);
    int pending = 0, result = 0;
    for (size_t i = 0; i < size && !result; ++i) {
        if (!bytes[i]) { result = fail(d, "the file contains a NUL byte"); break; }
        const yxml_ret_t token = yxml_parse(&d->xml, bytes[i]);
        if (token < 0) { result = fail(d, "malformed XML"); break; }
        if (token == YXML_ELEMSTART) {
            if (pending && complete(d)) { result = -1; break; }
            pending = 1;
            if (++d->depth > 3) { result = fail(d, "records nest at most one level"); break; }
            if (d->depth > 1 && !d->root) { result = fail(d, "the root must be <p4-edits schema=\"1\">"); break; }
            if (strlen(d->xml.elem) >= NAME_SIZE) { result = fail(d, "element name too long"); break; }
            strcpy(d->element, d->xml.elem);
            if (d->depth == 2) { strcpy(d->row_element, d->element); d->row_applied = 0; }
            d->attribute_count = 0;
        } else if (token == YXML_ATTRSTART) {
            if (d->attribute_count == ATTRIBUTES) { result = fail(d, "too many attributes"); break; }
            if (strlen(d->xml.attr) >= NAME_SIZE) { result = fail(d, "attribute name too long"); break; }
            for (size_t n = 0; n < d->attribute_count && !result; ++n)
                if (!strcmp(d->attributes[n].name, d->xml.attr)) result = fail(d, "duplicate attribute %s", d->xml.attr);
            if (result) break;
            Attribute *a = d->attributes + d->attribute_count++;
            *a = (Attribute){0};
            strcpy(a->name, d->xml.attr);
        } else if (token == YXML_ATTRVAL) {
            Attribute *a = d->attributes + d->attribute_count - 1;
            const size_t length = strlen(d->xml.data);
            if (a->length + length >= VALUE_SIZE) { result = fail(d, "attribute %s is too long", a->name); break; }
            memcpy(a->value + a->length, d->xml.data, length + 1);
            a->length += length;
        } else if (token == YXML_CONTENT) {
            for (const char *p = d->xml.data; *p && !result; ++p)
                if (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') result = fail(d, "records use attributes, not text");
            if (result) break;
            if (pending) { pending = 0; if (complete(d)) { result = -1; break; } }
        } else if (token == YXML_ELEMEND) {
            if (pending) { pending = 0; if (complete(d)) { result = -1; break; } }
            if (d->depth == 2) d->row_applied = 0;
            if (d->depth == 3) strcpy(d->element, d->row_element);
            --d->depth;
        } else if (token != YXML_OK && token != YXML_ATTREND) {
            result = fail(d, "processing instructions are not supported");
        }
    }
    if (!result && (yxml_eof(&d->xml) != YXML_OK || d->depth || !d->root))
        result = fail(d, "the file ends early; it needs a complete <p4-edits schema=\"1\"> root");
    if (!result && !d->records) result = fail(d, "the file edits nothing");
    free(d);
    return result;
}
