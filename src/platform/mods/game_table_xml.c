#include "p4pc/game_table_xml.h"
#include "p4pc/packages.h"
#include "../../../third_party/yxml/yxml.h"
#include <stdio.h>
#include <string.h>

enum { ATTRIBUTE_LIMIT = 24, ATTRIBUTE_BYTES = 8192 };
typedef struct Attribute { const char *name; char *value; unsigned used; } Attribute;
typedef struct Node {
    char name[64], bytes[ATTRIBUTE_BYTES];
    size_t used, count;
    Attribute attributes[ATTRIBUTE_LIMIT];
} Node;
typedef struct Parser {
    const P4GameTableXmlTarget *target;
    yxml_t xml;
    unsigned char xml_stack[4096];
    Node node;
    char *error;
    size_t error_capacity;
    unsigned depth, root_checked, rows, doctype;
} Parser;
static int fail(Parser *parser, const char *reason)
{
    if (parser->error && parser->error_capacity && !*parser->error)
        snprintf(parser->error, parser->error_capacity, "%s (line%lu)", reason, (unsigned long)parser->xml.line);
    return P4_TABLE_INVALID;
}
static char *take(Node *node, const char *name)
{
    for (size_t i = 0; i < node->count; ++i)
        if (!strcmp(node->attributes[i].name, name)) {
            node->attributes[i].used = 1;
            return node->attributes[i].value;
        }
    return NULL;
}
static int number(const char *text, uint32_t *out)
{
    if (!text || !*text) return 0;
    unsigned base = 10;
    if (text[0] == '0' && text[1] == 'x') { base = 16; text += 2; if (!*text) return 0; }
    uint32_t value = 0;
    do {
        unsigned char c = (unsigned char)*text++;
        unsigned digit = c >= '0' && c <= '9' ? (unsigned)(c - '0') :
            c >= 'a' && c <= 'f' ? c - 'a' + 10u : c >= 'A' && c <= 'F' ? c - 'A' + 10u : 16u;
        if (digit >= base || value > (UINT32_MAX - digit) / base) return 0;
        value = value * base + digit;
    } while (*text);
    *out = value;
    return 1;
}
static int field(Node *node, const char *name, uint32_t *out) { return number(take(node, name), out); }
static int text(Node *node, const char *name, char *out, size_t capacity)
{
    const char *value = take(node, name);
    if (!value || !*value) return 0;
    size_t size = strlen(value);
    if (size >= capacity) return 0;
    memcpy(out, value, size + 1);
    return 1;
}
static int all_used(const Node *node)
{
    for (size_t i = 0; i < node->count; ++i) if (!node->attributes[i].used) return 0;
    return 1;
}
static int reference(Node *node, P4ResourceRef *reference)
{
    const char *owner = take(node, "icon-owner"), *key = take(node, "icon-key"), *revision = take(node, "icon-revision");
    if (!owner && !key && !revision) return 1;
    if (!owner || !*owner || !key || !*key || !number(revision, &reference->revision) || !reference->revision ||
        strlen(owner) >= sizeof reference->owner || strlen(key) >= sizeof reference->key) return 0;
    strcpy(reference->owner, owner); strcpy(reference->key, key);
    return 1;
}
static int hex_digit(unsigned char c)
{
    return c >= '0' && c <= '9' ? c - '0' :
        c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}
static int hex_bytes(const char *encoded, void *destination, size_t size)
{
    if (!encoded || !size || strlen(encoded) != size * 2u) return 0;
    uint8_t *out = destination;
    for (size_t i = 0; i < size; ++i) {
        int high = hex_digit((unsigned char)encoded[i * 2u]);
        int low = hex_digit((unsigned char)encoded[i * 2u + 1u]);
        if (high < 0 || low < 0) return 0;
        out[i] = (uint8_t)((unsigned)high * 16u + (unsigned)low);
    }
    return 1;
}
static int signed_byte(Node *node, const char *name, int8_t *out)
{
    const char *value = take(node, name);
    if (!value) return 0;
    int negative = *value == '-';
    uint32_t magnitude;
    if (!number(value + negative, &magnitude) || magnitude > (negative ? 128u : 127u)) return 0;
    *out = (int8_t)(negative ? -(int32_t)magnitude : (int32_t)magnitude);
    return 1;
}
static int optional_pair(Node *node, const char *first, const char *second,
    uint32_t *a, uint32_t *b, unsigned *present)
{
    const char *left = take(node, first), *right = take(node, second);
    *present = left || right;
    return !*present || (number(left, a) && number(right, b));
}
static int action_reference(Node *node, P4GameItemActionRef *reference)
{
    const char *owner = take(node, "action-owner"), *key = take(node, "action-key");
    const char *revision = take(node, "action-revision");
    if (!owner && !key && !revision) return 1;
    if (!owner || !*owner || !key || !*key || !number(revision, &reference->revision) || !reference->revision ||
        strlen(owner) >= sizeof reference->owner || strlen(key) >= sizeof reference->key) return 0;
    strcpy(reference->owner, owner); strcpy(reference->key, key);
    return 1;
}
static int record(Parser *parser)
{
    const P4GameTableXmlTarget *target = parser->target;
    Node *node = &parser->node;
    P4TableSourceEntry entry = {.schema = 1, .revision = target->source_revision};
    const void *binding_view = target->builder->tables;
    int result;
    if (!strcmp(node->name, "element")) {
        P4ElementDesc value = {.revision = target->source_revision};
        if (!field(node, "id", &value.id) || !field(node, "flags", &value.flags) ||
            !field(node, "family", &value.family) || !field(node, "order", &value.order) ||
            !text(node, "key", value.key, sizeof value.key) ||
            !text(node, "display-name", value.display_name, sizeof value.display_name) ||
            !text(node, "short-name", value.short_name, sizeof value.short_name) || !reference(node, &value.icon))
            return fail(parser, "Invalid element fields");
        if (!all_used(node)) return fail(parser, "Unknown element attribute");
        result = p4_game_tables_element(target->builder->tables, target->owner, &value);
        entry.domain = P4_TABLE_SOURCE_ELEMENT; entry.ids[0] = value.id; strcpy(entry.key, value.key);
    } else if (!strcmp(node->name, "channel")) {
        P4AffinityChannelDesc value = {.revision = target->source_revision};
        if (!field(node, "id", &value.id) || !field(node, "kind", &value.kind) ||
            !text(node, "key", value.key, sizeof value.key) || !text(node, "display-name", value.display_name, sizeof value.display_name))
            return fail(parser, "Invalid channel fields");
        if (!all_used(node)) return fail(parser, "Unknown channel attribute");
        result = p4_game_tables_channel(target->builder->tables, target->owner, &value);
        entry.domain = P4_TABLE_SOURCE_CHANNEL; entry.ids[0] = value.id; strcpy(entry.key, value.key);
    } else if (!strcmp(node->name, "affinity")) {
        P4AffinityRow value = {0};
        if (!field(node, "kind", &value.entity_kind) || !field(node, "entity", &value.entity_id) ||
            !field(node, "profile", &value.profile_id) || !field(node, "element", &value.element_id) ||
            !field(node, "channel", &value.channel_id) || !field(node, "value", &value.value) || !field(node, "scalar", &value.scalar))
            return fail(parser, "Invalid affinity fields");
        if (!all_used(node)) return fail(parser, "Unknown affinity attribute");
        result = p4_game_tables_affinity(target->builder->tables, target->owner, &value);
        entry.domain = P4_TABLE_SOURCE_AFFINITY; entry.ids[0] = value.entity_kind; entry.ids[1] = value.entity_id;
        entry.ids[2] = value.profile_id; entry.ids[3] = value.element_id; entry.ids[4] = value.channel_id;
        entry.revision = target->builder->registry_revision;
    } else if (!strcmp(node->name, "action")) {
        P4ActionElementBinding value = {.schema = 1, .revision = target->source_revision};
        if (!field(node, "source-kind", &value.source_kind) || !field(node, "source-id", &value.source_id) ||
            !field(node, "component", &value.component) || !field(node, "element", &value.element_id) ||
            !field(node, "channel", &value.channel_id) || !text(node, "key", value.key, sizeof value.key))
            return fail(parser, "Invalid action fields");
        if (!all_used(node)) return fail(parser, "Unknown action attribute");
        result = p4_game_tables_action_element(target->builder->tables, target->owner, &value);
        entry.domain = P4_TABLE_SOURCE_ACTION; entry.ids[0] = value.source_kind; entry.ids[1] = value.source_id;
        entry.ids[2] = value.component; strcpy(entry.key, value.key);
    } else if (!strcmp(node->name, "alias")) {
        P4AffinityProfileAlias value = {.schema = 1, .revision = target->source_revision};
        if (!field(node, "kind", &value.entity_kind) || !field(node, "entity", &value.entity_id) ||
            !field(node, "native-profile", &value.native_profile) || !field(node, "profile", &value.profile_id) ||
            !field(node, "knowledge", &value.knowledge_id) || !text(node, "key", value.key, sizeof value.key))
            return fail(parser, "Invalid alias fields");
        if (!all_used(node)) return fail(parser, "Unknown alias attribute");
        result = p4_game_tables_affinity_alias(target->builder->tables, target->owner, &value);
        entry.domain = P4_TABLE_SOURCE_ALIAS; entry.ids[0] = value.entity_kind; entry.ids[1] = value.entity_id;
        entry.ids[2] = value.native_profile; strcpy(entry.key, value.key);
    } else if (!strcmp(node->name, "effect")) {
        P4AffinityEffectBinding value = {.schema = 1, .revision = target->source_revision};
        if (!field(node, "source-kind", &value.source_kind) || !field(node, "source-id", &value.source_id) ||
            !field(node, "element", &value.element_id) || !field(node, "channel", &value.channel_id) ||
            !field(node, "flags", &value.flags) || !text(node, "key", value.key, sizeof value.key))
            return fail(parser, "Invalid effect fields");
        if (!all_used(node)) return fail(parser, "Unknown effect attribute");
        result = p4_game_tables_affinity_effect(target->builder->tables, target->owner, &value);
        entry.domain = P4_TABLE_SOURCE_EFFECT; entry.ids[0] = value.source_kind; entry.ids[1] = value.source_id;
        entry.ids[2] = value.element_id; entry.ids[3] = value.channel_id; strcpy(entry.key, value.key);
    } else if (!strcmp(node->name, "skill")) {
        P4GameSkillDesc value = {.schema = 1, .revision = target->source_revision};
        uint32_t auxiliary, group;
        if (!field(node, "id", &value.id) || !field(node, "active", &value.active) ||
            !field(node, "element", &value.element_id) || !field(node, "element-aux", &auxiliary) || auxiliary > UINT8_MAX ||
            !field(node, "inheritance-group", &group) || group > UINT8_MAX ||
            !signed_byte(node, "inheritance-rate", &value.inheritance_rate) ||
            !text(node, "key", value.key, sizeof value.key) ||
            !hex_bytes(take(node, "metadata"), value.metadata, sizeof value.metadata) ||
            !hex_bytes(take(node, "presentation"), value.presentation, sizeof value.presentation))
            return fail(parser, "Invalid skill fields or exact40/4-byte little-endian rows");
        if (!all_used(node)) return fail(parser, "Unknown skill attribute");
        value.element_aux = (uint8_t)auxiliary; value.inheritance_group = (uint8_t)group;
        P4GameSkills *skills = p4_game_tables_builder_skills(target->builder);
        if (!skills) return fail(parser, "Skill family is unavailable or already sealed");
        result = p4_game_skills_register(skills, target->owner, &value);
        binding_view = skills; entry.domain = P4_TABLE_SOURCE_SKILL;
        entry.ids[0] = value.id; strcpy(entry.key, value.key);
    } else if (!strcmp(node->name, "item")) {
        char key[P4_TABLE_KEY_SIZE];
        P4GameItemDesc value = {.revision = target->source_revision, .key = key};
        uint32_t required[2] = {0}, quantities[2] = {0}, right = P4_ITEM_MODEL_NONE, left = P4_ITEM_MODEL_NONE;
        unsigned gates[2];
        const char *right_text = take(node, "right-model"), *left_text = take(node, "left-model");
        if (!field(node, "id", &value.id) || !text(node, "key", key, sizeof key) ||
            !hex_bytes(take(node, "metadata"), &value.metadata, sizeof value.metadata) || !reference(node, &value.icon) ||
            !optional_pair(node, "require0-id", "require0-quantity", required, quantities, gates) ||
            !optional_pair(node, "require1-id", "require1-quantity", required + 1, quantities + 1, gates + 1) ||
            (right_text && !number(right_text, &right)) || (left_text && (!right_text || !number(left_text, &left))))
            return fail(parser, "Invalid item fields, dependencies or exact68-byte little-endian row");
        if (!all_used(node)) return fail(parser, "Unknown item attribute");
        P4GameItems *items = p4_game_tables_builder_items(target->builder);
        if (!items) return fail(parser, "Item family is unavailable or already sealed");
        result = p4_game_tables_items_register(items, target->owner, &value);
        for (uint32_t gate = 0; !result && gate < 2; ++gate)
            if (gates[gate]) result = p4_game_tables_items_requirement_register(items, target->owner,
                value.id, gate, required[gate], quantities[gate]);
        if (!result && right_text) {
            result = p4_game_tables_items_models_register(items, target->owner, value.id, right, left);
            if (!result) result = p4_game_tables_builder_request_input(target->builder, P4_TABLE_INPUT_MODEL);
        }
        binding_view = items; entry.domain = P4_TABLE_SOURCE_ITEM;
        entry.ids[0] = value.id; strcpy(entry.key, key);
    } else if (!strcmp(node->name, "category")) {
        char key[P4_TABLE_KEY_SIZE];
        P4GameItemCategoryDesc value = {.revision = target->source_revision, .key = key};
        if (!field(node, "id", &value.id) || !field(node, "filter-a", &value.filter_a) ||
            !field(node, "filter-b", &value.filter_b) || !field(node, "stock-policy", &value.stock_policy) ||
            !text(node, "key", key, sizeof key) || !reference(node, &value.icon) || !action_reference(node, &value.action))
            return fail(parser, "Invalid category fields or script-action reference");
        if (!all_used(node)) return fail(parser, "Unknown category attribute");
        P4GameItems *items = p4_game_tables_builder_items(target->builder);
        if (!items) return fail(parser, "Category family is unavailable or already sealed");
        result = p4_game_tables_items_category_register(items, target->owner, &value);
        binding_view = items; entry.domain = P4_TABLE_SOURCE_CATEGORY;
        entry.ids[0] = value.id; strcpy(entry.key, key);
    } else if (!strcmp(node->name, "enemy")) {
        P4GameEnemyDesc value = {.schema = 1, .revision = target->source_revision};
        if (!field(node, "id", &value.id) || !field(node, "model-source", &value.model_source) ||
            !text(node, "key", value.key, sizeof value.key) ||
            !hex_bytes(take(node, "metadata"), value.metadata, sizeof value.metadata) ||
            !hex_bytes(take(node, "affinity"), value.affinity, sizeof value.affinity) ||
            !hex_bytes(take(node, "model"), value.model, sizeof value.model) ||
            !hex_bytes(take(node, "ai"), value.ai, sizeof value.ai))
            return fail(parser, "Invalid enemy fields or exact60/32/232/164-byte little-endian rows");
        if (!all_used(node)) return fail(parser, "Unknown enemy attribute");
        P4GameEnemies *enemies = p4_game_tables_builder_enemies(target->builder);
        if (!enemies) return fail(parser, "Enemy family is unavailable or already sealed");
        result = p4_game_enemies_register(enemies, target->owner, &value);
        binding_view = enemies; entry.domain = P4_TABLE_SOURCE_ENEMY;
        entry.ids[0] = value.id; strcpy(entry.key, value.key);
    } else if (!strcmp(node->name, "persona")) {
        P4GamePersonaDesc value = {.schema = 1, .revision = target->source_revision};
        if (!field(node, "id", &value.id) || !field(node, "model-source", &value.model_source) ||
            !text(node, "key", value.key, sizeof value.key) ||
            !hex_bytes(take(node, "metadata"), value.metadata, sizeof value.metadata) ||
            !hex_bytes(take(node, "growth"), value.growth, sizeof value.growth) ||
            !hex_bytes(take(node, "model"), value.model, sizeof value.model) ||
            !hex_bytes(take(node, "candidates"), value.candidates, sizeof value.candidates))
            return fail(parser, "Invalid persona fields or exact14/70/88/4-byte little-endian rows");
        if (!all_used(node)) return fail(parser, "Unknown persona attribute");
        P4GamePersonas *personas = p4_game_tables_builder_personas(target->builder);
        if (!personas) return fail(parser, "Persona family is unavailable or already sealed");
        result = p4_game_personas_register(personas, target->owner, &value);
        binding_view = personas; entry.domain = P4_TABLE_SOURCE_PERSONA;
        entry.ids[0] = value.id; strcpy(entry.key, value.key);
    } else if (!strcmp(node->name, "string")) {
        uint32_t table, id, role;
        char *encoded = take(node, "bytes");
        if (!field(node, "table", &table) || !field(node, "id", &id) || !field(node, "role", &role) || !encoded)
            return fail(parser, "Invalid encoded string fields");
        size_t size = strlen(encoded) / 2u;
        /* Compress this consumed attribute in its own bounded owned buffer;
         * later attribute offsets stay unchanged. Registration copies once. */
        if (!hex_bytes(encoded, encoded, size) || !all_used(node))
            return fail(parser, "Invalid encoded string hex or unknown attribute");
        P4GameText *strings = p4_game_tables_builder_text(target->builder, role);
        if (!strings) return fail(parser, "String role is invalid or already sealed");
        result = p4_game_tables_text_register(strings, target->owner, table, id, role,
            target->source_revision, (const uint8_t *)encoded, size);
        binding_view = strings; entry.domain = P4_TABLE_SOURCE_STRING;
        entry.ids[0] = table; entry.ids[1] = id; entry.ids[2] = role;
    } else return fail(parser, "Unsupported table record domain");
    if (result) { fail(parser, p4_game_tables_error(result)); return result; }
    if (strlen(target->owner) >= sizeof entry.owner) return fail(parser, "Invalid source owner");
    strcpy(entry.owner, target->owner);
    result = p4_game_table_sources_bind(target->builder->sources, binding_view, &entry, target->source);
    if (result) { fail(parser, p4_game_tables_error(result)); return result; }
    ++parser->rows;
    return P4_TABLE_OK;
}
static int root(Parser *parser)
{
    uint32_t schema;
    if (strcmp(parser->node.name, "p4-tables") || !field(&parser->node, "schema", &schema) || schema != 1 || !all_used(&parser->node))
        return fail(parser, "Require p4-tables root with supported schema1");
    parser->root_checked = 1;
    return P4_TABLE_OK;
}
static int append(Node *node, const char *value, int terminate)
{
    size_t size = strlen(value), extra = terminate ? 1u : 0u;
    if (size + extra > sizeof node->bytes - node->used) return 0;
    memcpy(node->bytes + node->used, value, size + extra); node->used += size + extra;
    return 1;
}
static int initialize(Parser *parser, const P4GameTableXmlTarget *target,
    uint64_t size, char *error, size_t error_capacity)
{
    if (error && error_capacity) *error = 0;
    if (!target || !target->builder || !target->builder->tables || !target->builder->sources ||
        target->builder->sealing || target->builder->sealed || !target->owner || !*target->owner ||
        !target->source_revision || !target->builder->registry_revision || !size || size > P4_GAME_TABLE_SOURCE_XML_MAX)
        return P4_TABLE_INVALID;
    parser->target = target; parser->error = error; parser->error_capacity = error_capacity;
    parser->depth = parser->root_checked = parser->rows = parser->doctype = 0;
    yxml_init(&parser->xml, parser->xml_stack, sizeof parser->xml_stack);
    return P4_TABLE_OK;
}
static int feed(Parser *parser, const uint8_t *bytes, size_t size)
{
    static const char doctype[] = "<!DOCTYPE";
    for (size_t i = 0; i < size; ++i) {
        if (!bytes[i]) return fail(parser, "Embedded NUL in table source");
        parser->doctype = bytes[i] == (uint8_t)doctype[parser->doctype] ? parser->doctype + 1u : bytes[i] == '<' ? 1u : 0u;
        if (parser->doctype == sizeof doctype - 1u) return fail(parser, "Table XML DTDs are unsupported");
        yxml_ret_t token = yxml_parse(&parser->xml, bytes[i]);
        if (token < 0) return fail(parser, "Malformed table XML");
        if (token == YXML_ELEMSTART) {
            if (parser->depth == 1 && !parser->root_checked && root(parser)) return P4_TABLE_INVALID;
            if (++parser->depth > 2) return fail(parser, "Nested table records are unsupported");
            parser->node.used = parser->node.count = 0;
            if (strlen(parser->xml.elem) >= sizeof parser->node.name) return fail(parser, "Table record name is too long");
            strcpy(parser->node.name, parser->xml.elem);
        } else if (token == YXML_ATTRSTART) {
            Node *node = &parser->node;
            if (!parser->depth || node->count == ATTRIBUTE_LIMIT) return fail(parser, "Too many table attributes");
            for (size_t n = 0; n < node->count; ++n)
                if (!strcmp(node->attributes[n].name, parser->xml.attr)) return fail(parser, "Duplicate table attribute");
            Attribute *attribute = node->attributes + node->count++;
            attribute->used = 0;
            attribute->name = node->bytes + node->used;
            if (!append(node, parser->xml.attr, 1)) return fail(parser, "Table attributes exceed bound");
            attribute->value = node->bytes + node->used;
        } else if (token == YXML_ATTRVAL) {
            if (!parser->node.count || !append(&parser->node, parser->xml.data, 0)) return fail(parser, "Table attribute value exceeds bound");
        } else if (token == YXML_ATTREND) {
            if (!append(&parser->node, "", 1)) return fail(parser, "Table attribute value exceeds bound");
        } else if (token == YXML_CONTENT) {
            for (const char *p = parser->xml.data; *p; ++p)
                if (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') return fail(parser, "Table records use attributes, not text content");
        } else if (token == YXML_ELEMEND) {
            int result = parser->depth == 2 ? record(parser) : parser->root_checked ? P4_TABLE_OK : root(parser);
            if (result) return result;
            if (!parser->depth) return fail(parser, "Invalid table depth");
            --parser->depth;
        } else if (token != YXML_OK) return fail(parser, "Unsupported table XML processing instruction");
    }
    return P4_TABLE_OK;
}
static int finish(Parser *parser)
{
    if (yxml_eof(&parser->xml) != YXML_OK || parser->depth || !parser->root_checked || !parser->rows)
        return fail(parser, "Incomplete or empty table source");
    return P4_TABLE_OK;
}
int p4_game_tables_source_xml(const P4GameTableXmlTarget *target,
    const uint8_t *bytes, size_t size, char *error, size_t error_capacity)
{
    if (!bytes) return P4_TABLE_INVALID;
    Parser parser;
    int result = initialize(&parser, target, size, error, error_capacity);
    if (!result && target->content) p4_content_digest_update(target->content, bytes, size);
    if (!result) result = feed(&parser, bytes, size);
    return result ? result : finish(&parser);
}
int p4_game_tables_source_xml_file(const P4GameTableXmlTarget *target,
    P4PackageFile *file, char *error, size_t error_capacity)
{
    if (!file) return P4_TABLE_INVALID;
    uint64_t size = p4_package_file_size(file);
    Parser parser;
    int result = initialize(&parser, target, size, error, error_capacity);
    if (result) return result;
    uint8_t chunk[4096];
    for (uint64_t offset = 0; offset < size;) {
        size_t count = size - offset < sizeof chunk ? (size_t)(size - offset) : sizeof chunk;
        if (p4_package_file_read_at(file, offset, chunk, count)) return fail(&parser, "Could not read authorized table-source bytes");
        if (target->content) p4_content_digest_update(target->content, chunk, count);
        result = feed(&parser, chunk, count);
        if (result) return result;
        offset += count;
    }
    return finish(&parser);
}
