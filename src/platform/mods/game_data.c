/* p4.data pure layer: row decode, name forms and keys (include/p4pc/game_data.h).
 *
 * Row layouts are the PS2 banks the game reads: SKILL0/SKILL1 (datCalc.c's
 * DatSkillEntry and the 0x0023D9B0 cost formula), UNIT0 (datCalcEnemyMoney
 * 0x002439C0 reads +0x20, the drop picker 0x00243A30 the four +0x22 slots),
 * PERSONA0/1/2 (datPersona accessors; PERSONA2 learn events as the table-delta
 * stage edits them) and ITEMTBL (P4ItemRow in game_items.h). */
#include "p4pc/game_data.h"
#include "p4pc/game_items.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t u16le(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static uint32_t u32le(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static int lower(int c) { return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c; }
static int alnum(int c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static int same_text(const char *a, const char *b)
{
    while (*a && lower((unsigned char)*a) == lower((unsigned char)*b)) { ++a; ++b; }
    return !*a && !*b;
}

static const char *const kinds[P4_DATA_KIND_COUNT] = {
    "skill", "persona", "enemy", "item", "floor", "formation", "encounters", "treasure"
};
const char *p4_data_kind_name(P4DataKind kind) { return (unsigned)kind < P4_DATA_KIND_COUNT ? kinds[kind] : NULL; }
int p4_data_kind_parse(const char *name, P4DataKind *kind)
{
    for (unsigned i = 0; name && i < P4_DATA_KIND_COUNT; ++i)
        if (!strcmp(name, kinds[i])) { *kind = (P4DataKind)i; return 0; }
    return -1;
}

/* Elements 8..15 are the ailment leaves in their affinity-column order. */
static const char *const elements[P4_DATA_ELEMENTS] = {
    "physical", "fire", "ice", "electric", "wind", "almighty", "light", "dark",
    "confusion", "poison", "fear", "rage", "dizzy", "exhaustion", "enervation", "silence",
    "heal", "support", "special"
};
static const struct { const char *alias; uint32_t element; } element_aliases[] = {
    {"phys", 0}, {"elec", 3}, {"electricity", 3}, {"panic", 8}, {"confuse", 8}, {"enrage", 11}, {"mute", 15}
};
const char *p4_data_element_name(uint32_t element) { return element < P4_DATA_ELEMENTS ? elements[element] : NULL; }
int p4_data_element_parse(const char *name, uint32_t *element)
{
    if (!name) return -1;
    for (uint32_t i = 0; i < P4_DATA_ELEMENTS; ++i)
        if (same_text(name, elements[i])) { *element = i; return 0; }
    for (size_t i = 0; i < sizeof element_aliases / sizeof element_aliases[0]; ++i)
        if (same_text(name, element_aliases[i].alias)) { *element = element_aliases[i].element; return 0; }
    return -1;
}

static const P4DataAilment ailments[P4_DATA_AILMENTS] = {
    {"dizzy", 0x01u}, {"rage", 0x02u}, {"fear", 0x04u}, {"silence", 0x08u}, {"confusion", 0x10u},
    {"poison", 0x20u}, {"exhaustion", 0x40u}, {"enervation", 0x80u}, {"ko", 0x80000u}, {"down", 0x100000u}
};
const P4DataAilment *p4_data_ailment_at(size_t index) { return index < P4_DATA_AILMENTS ? &ailments[index] : NULL; }
const char *p4_data_ailment_name(uint32_t bit)
{
    for (size_t i = 0; i < P4_DATA_AILMENTS; ++i) if (ailments[i].bit == bit) return ailments[i].name;
    return NULL;
}
int p4_data_ailment_parse(const char *name, uint32_t *bit)
{
    if (!name) return -1;
    for (size_t i = 0; i < P4_DATA_AILMENTS; ++i)
        if (same_text(name, ailments[i].name)) { *bit = ailments[i].bit; return 0; }
    uint32_t element;
    if (!p4_data_element_parse(name, &element) && element >= 8 && element < 16) {
        static const uint32_t by_element[8] = {0x10u, 0x20u, 0x04u, 0x02u, 0x01u, 0x40u, 0x80u, 0x08u};
        *bit = by_element[element - 8];
        return 0;
    }
    return -1;
}

static const char *const affinities[] = {"normal", "weak", "resist", "null", "repel", "drain"};
const char *p4_data_affinity_name(P4DataAffinity affinity)
{ return (unsigned)affinity < sizeof affinities / sizeof affinities[0] ? affinities[affinity] : NULL; }
int p4_data_affinity_parse(const char *name, P4DataAffinity *affinity)
{
    for (unsigned i = 0; name && i < sizeof affinities / sizeof affinities[0]; ++i)
        if (same_text(name, affinities[i])) { *affinity = (P4DataAffinity)i; return 0; }
    if (name && (same_text(name, "absorb") || same_text(name, "immune"))) {
        *affinity = same_text(name, "absorb") ? P4_DATA_AFFINITY_DRAIN : P4_DATA_AFFINITY_NULL;
        return 0;
    }
    return -1;
}
P4DataAffinity p4_data_affinity_kind(uint32_t column, uint16_t packed)
{
    const unsigned flags = packed >> 8;
    if (column >= 8) {
        const uint32_t rate = p4_data_affinity_rate(packed);
        if ((flags & 0x21u) || !rate) return P4_DATA_AFFINITY_NULL;
        return rate > 100u ? P4_DATA_AFFINITY_WEAK : rate < 100u ? P4_DATA_AFFINITY_RESIST : P4_DATA_AFFINITY_NORMAL;
    }
    if (flags & 0x01u) return P4_DATA_AFFINITY_NULL;
    if (flags & 0x02u) return P4_DATA_AFFINITY_REPEL;
    if (flags & 0x04u) return P4_DATA_AFFINITY_DRAIN;
    if (flags & 0x08u) return P4_DATA_AFFINITY_WEAK;
    if (flags & 0x10u) return P4_DATA_AFFINITY_RESIST;
    return P4_DATA_AFFINITY_NORMAL;
}
uint32_t p4_data_affinity_rate(uint16_t packed)
{
    /* p4_affinity_decode's rule (game_tables.c), without the registry. */
    const unsigned flags = packed >> 8, rate = (packed & 0xFFu) * 5u;
    if (rate) return rate;
    if (flags & 0x08u) return 125;
    if (flags & 0x27u) return 100;
    if (flags & 0x10u) return 50;
    return 0;
}
int p4_data_affinity_encode(uint32_t column, P4DataAffinity affinity, uint16_t *packed)
{
    static const uint16_t attack[] = {0x0014, 0x0800, 0x1000, 0x0100, 0x0200, 0x0400};
    static const uint16_t ailment[] = {0x0014, 0x00FF, 0x000A, 0x2000};
    if (column >= P4_DATA_AFFINITY_COLUMNS || (unsigned)affinity >= sizeof attack / sizeof attack[0]) return -1;
    if (column < 8) { *packed = attack[affinity]; return 0; }
    if ((unsigned)affinity >= sizeof ailment / sizeof ailment[0]) return -1;
    *packed = ailment[affinity];
    return 0;
}

static const char *const characters[P4_DATA_CHARACTERS] = {
    "protagonist", "yosuke", "chie", "yukiko", "rise", "kanji", "naoto", "teddie"
};
const char *p4_data_character_name(uint32_t character)
{ return character >= 1 && character <= P4_DATA_CHARACTERS ? characters[character - 1] : NULL; }
int p4_data_character_parse(const char *name, uint32_t *character)
{
    for (uint32_t i = 0; name && i < P4_DATA_CHARACTERS; ++i)
        if (same_text(name, characters[i])) { *character = i + 1; return 0; }
    return -1;
}

/* ---- rows ---- */

void p4_data_skill_decode(uint32_t id, const uint8_t element[2], const uint8_t *m, P4DataSkill *out)
{
    memset(out, 0, sizeof *out);
    out->id = id;
    out->element = element[0];
    if (!m) { out->passive = 1; return; }
    out->usable = m[0x01]; out->damage_stat = m[0x02]; out->cost_kind = m[0x03];
    out->cost = u16le(m + 0x04);
    out->target_scope = m[0x08]; out->target_side = m[0x09]; out->target_filter = m[0x0A];
    out->accuracy = m[0x0E]; out->hits_min = m[0x0F]; out->hits_max = m[0x10];
    out->hp_effect = m[0x11]; out->power = u16le(m + 0x12);
    out->sp_effect = m[0x14]; out->sp_power = u16le(m + 0x16);
    out->ailment_mode = m[0x18]; out->ailment_chance = m[0x19]; out->ailments = u32le(m + 0x1C);
    out->buffs = u16le(m + 0x20); out->barriers = u16le(m + 0x22);
    out->special = m[0x24]; out->critical = m[0x25];
}
const char *p4_data_skill_kind(const P4DataSkill *s)
{
    if (s->passive) return "passive";
    switch (s->element) {
    case 0: return "physical";
    case 5: return "almighty";
    case 16: return "heal";
    case 17: return "support";
    case 18: return "special";
    default: break;
    }
    if (s->element >= 8 && s->element < 16) return "ailment";
    return s->element < 8 ? "magic" : "other";
}
const char *p4_data_skill_targets(const P4DataSkill *s)
{
    if (s->passive) return "other";
    if (s->target_side == 3) return "everyone";
    if (s->target_side == 2) return s->target_scope ? "all_foes" : "one_foe";
    if (s->target_side != 1) return "other";
    if (s->target_filter & 4u) return s->target_scope ? "all_fallen_allies" : "one_fallen_ally";
    if (s->target_filter & 2u) return s->target_scope ? "all_fallen_allies" : "other";
    if (s->target_filter & 1u) return "self";
    return s->target_scope ? "all_allies" : "one_ally";
}

void p4_data_enemy_decode(uint32_t id, const uint8_t m[60], const uint8_t a[32], P4DataEnemy *out)
{
    memset(out, 0, sizeof *out);
    out->id = id;
    out->flags = u16le(m); out->arcana = m[0x02]; out->level = m[0x03];
    out->hp = u16le(m + 0x04); out->sp = u16le(m + 0x06);
    memcpy(out->stats, m + 0x08, P4_DATA_STATS);
    for (unsigned i = 0; i < P4_DATA_ENEMY_SKILLS; ++i) out->skills[i] = u16le(m + 0x0E + i * 2u);
    out->exp = u16le(m + 0x1E); out->yen = u16le(m + 0x20);
    for (unsigned i = 0; i < P4_DATA_ENEMY_DROPS; ++i) {
        out->drops[i].item = u16le(m + 0x22 + i * 4u);
        out->drops[i].weight = m[0x24 + i * 4u];
    }
    out->attack_element = m[0x38]; out->attack_accuracy = m[0x39]; out->attack_power = u16le(m + 0x3A);
    for (unsigned i = 0; i < P4_DATA_AFFINITY_COLUMNS; ++i) out->affinity[i] = u16le(a + i * 2u);
}

void p4_data_persona_decode(uint32_t id, const uint8_t m[14], const uint8_t g[70], const uint8_t *party,
    const uint8_t a[32], P4DataPersona *out)
{
    memset(out, 0, sizeof *out);
    out->id = id;
    out->flags = u16le(m); out->arcana = m[0x02]; out->level = m[0x03];
    memcpy(out->stats, m + 0x04, P4_DATA_STATS);
    out->inheritance = u16le(m + 0x0A);
    memcpy(out->growth, g, P4_DATA_STATS);
    if (party) {
        /* u16 character, u16 levels, then 32 events {level, kind, skill}
         * up to the first empty kind; kind 1 learns, kind 2 forgets. */
        out->party = 1;
        const uint16_t character = u16le(party);
        out->character = character <= P4_DATA_CHARACTERS ? (uint8_t)character : 0;
        for (unsigned i = 0; i < 32; ++i) {
            const uint8_t *event = party + 4 + i * 4u;
            if (!event[1]) break;
            if (event[1] != 1) continue;
            out->learnset[out->learn_count].level = event[0];
            out->learnset[out->learn_count++].skill = u16le(event + 2);
        }
    } else {
        /* Sixteen {levels after the base, kind, skill} events from +0x06. */
        for (unsigned i = 0; i < 16; ++i) {
            const uint8_t *event = g + 6 + i * 4u;
            const uint16_t skill = u16le(event + 2);
            if (event[1] != 1 || !skill) continue;
            const unsigned level = (unsigned)out->level + event[0];
            out->learnset[out->learn_count].level = (uint8_t)(level > 255u ? 255u : level);
            out->learnset[out->learn_count++].skill = skill;
        }
    }
    for (unsigned i = 0; i < P4_DATA_AFFINITY_COLUMNS; ++i) out->affinity[i] = u16le(a + i * 2u);
}

void p4_data_item_decode(uint32_t id, const uint8_t row[68], P4DataItem *out)
{
    P4ItemRow r;
    memcpy(&r, row, sizeof r);
    memset(out, 0, sizeof *out);
    out->id = id;
    out->kind = r.kind;
    out->attack = r.primary_value; out->hit = r.secondary_value;
    out->defense = r.middle_band_value; out->evasion = r.last_band_value;
    /* The row keeps strength, endurance, magic, agility, luck. */
    out->stats[0] = r.stat_bonus[0]; out->stats[1] = r.stat_bonus[2]; out->stats[2] = r.stat_bonus[1];
    out->stats[3] = r.stat_bonus[3]; out->stats[4] = r.stat_bonus[4];
    out->skill = r.granted_skill;
    out->users = (uint16_t)((r.category_mask >> 5) & 0xFFu);
    out->price = r.price;
    out->month = r.available_month;
    out->day = r.available_day;
    out->unlock[0] = p4_data_unlock_decode(r.first_moon_gate);
    out->unlock[1] = p4_data_unlock_decode(r.second_moon_gate);
}
const char *p4_data_item_kind(const P4DataItem *item)
{
    static const char *const names[] = {"weapon", "armor", "accessory", "consumable", "key", "material"};
    return item->kind < sizeof names / sizeof names[0] ? names[item->kind] : "other";
}
const char *p4_data_item_store(const P4DataItem *item)
{
    return item->kind <= 2 ? "weapon" : item->kind == 3 ? "item" : NULL;
}
int p4_data_item_sold(const P4DataItem *item)
{
    if (item->kind == 3) return item->month != P4_DATA_ITEM_NEVER_MONTH || item->day != P4_DATA_ITEM_NEVER_DAY;
    if (item->kind > 2) return 0;
    for (unsigned i = 0; i < P4_DATA_ITEM_UNLOCKS; ++i)
        if (item->unlock[i].count && item->unlock[i].material == P4_DATA_ITEM_NEVER_MATERIAL) return 0;
    return 1;
}
uint32_t p4_data_unlock_encode(P4DataUnlock unlock)
{
    return (uint32_t)(unlock.material & 0xFFFu) << 8 | unlock.count;
}
P4DataUnlock p4_data_unlock_decode(uint32_t word)
{
    return (P4DataUnlock){(uint16_t)((word >> 8) & 0xFFFu), (uint8_t)word};
}

/* ---- dungeons ---- */

void p4_data_floor_decode(uint32_t id, const uint8_t row[10], P4DataFloor *out)
{
    *out = (P4DataFloor){.id = id, .encounters = u16le(row), .refill = row[2], .spawn = row[3], .chests = row[4],
        .treasure = u16le(row + 6)};
}
void p4_data_formation_decode(uint32_t id, const uint8_t row[24], P4DataFormation *out)
{
    out->id = id;
    for (unsigned i = 0; i < P4_DATA_FORMATION_ENEMIES; ++i) out->enemies[i] = u16le(row + 8u + i * 2u);
}
uint32_t p4_data_encounter_rate_offset(uint32_t variant, P4DataEncounterClass value)
{
    return (uint32_t)value * 2u + variant;
}
uint32_t p4_data_encounter_slot_offset(uint32_t variant, uint32_t slot)
{
    return 8u + variant * P4_DATA_ENCOUNTER_SLOTS * 4u + slot * 4u;
}
void p4_data_encounters_decode(uint32_t id, const uint8_t row[248], P4DataEncounters *out)
{
    out->id = id;
    for (uint32_t v = 0; v < P4_DATA_ENCOUNTER_VARIANTS; ++v) {
        for (unsigned c = 0; c < P4_DATA_ENCOUNTER_CLASSES; ++c)
            out->rates[v][c] = row[p4_data_encounter_rate_offset(v, (P4DataEncounterClass)c)];
        for (uint32_t s = 0; s < P4_DATA_ENCOUNTER_SLOTS; ++s) {
            const uint8_t *slot = row + p4_data_encounter_slot_offset(v, s);
            out->slots[v][s] = (P4DataEncounter){u16le(slot), u16le(slot + 2)};
        }
    }
}
const char *p4_data_encounter_class_name(P4DataEncounterClass value)
{
    static const char *const names[P4_DATA_ENCOUNTER_CLASSES] = {"normal", "special", "rare"};
    return (unsigned)value < P4_DATA_ENCOUNTER_CLASSES ? names[value] : NULL;
}
void p4_data_encounter_class_slots(P4DataEncounterClass value, uint32_t *first, uint32_t *count)
{
    static const uint32_t firsts[P4_DATA_ENCOUNTER_CLASSES] = {0, 20, 25}, counts[P4_DATA_ENCOUNTER_CLASSES] = {20, 5, 5};
    *first = firsts[value]; *count = counts[value];
}
void p4_data_treasure_decode(uint32_t id, const uint8_t row[348], P4DataTreasure *out)
{
    out->id = id;
    for (uint32_t i = 0; i < P4_DATA_TREASURE_RECORDS; ++i) {
        const uint8_t *record = row + i * 12u;
        out->records[i] = (P4DataTreasureRecord){u16le(record), u16le(record + 2), (uint8_t)(record[7] & 1u)};
    }
}

/* ---- names ---- */

size_t p4_data_match_form(const char *name, char *out, size_t capacity)
{
    size_t length = 0;
    if (!capacity) return 0;
    for (const unsigned char *c = (const unsigned char *)(name ? name : ""); *c; ++c) {
        if (*c < 0x80 && !alnum(*c)) continue;
        if (length + 1 >= capacity) break;
        out[length++] = (char)lower(*c);
    }
    out[length] = 0;
    return length;
}
int p4_data_placeholder(const char *name)
{
    if (!name || !*name || same_text(name, "blank")) return 1;
    /* Untranslated rows (two-byte glyphs only): nothing a mod could type. */
    int typed = 0;
    for (const unsigned char *c = (const unsigned char *)name; *c && !typed; ++c)
        typed = (*c >= '0' && *c <= '9') || (lower(*c) >= 'a' && lower(*c) <= 'z');
    if (!typed) return 1;
    const char *digits = name;
    if (name[0] == '0' && (name[1] == 'x' || name[1] == 'X')) digits = name + 2;
    size_t length = 0;
    int decimal = 0;
    for (; digits[length]; ++length) {
        const int c = lower((unsigned char)digits[length]);
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return 0;
        decimal |= c <= '9';
    }
    /* "0x1A2", or a short hex ID with a digit ("19C", "000"; never "Bead"). */
    return length && (digits != name || (length <= 4 && decimal));
}
size_t p4_data_slug(const char *name, char *out, size_t capacity)
{
    size_t length = 0;
    int separator = 0;
    if (!capacity) return 0;
    for (const unsigned char *c = (const unsigned char *)(name ? name : ""); *c; ++c) {
        if (*c == '\'' || *c == '.') continue;
        if (!(*c < 0x80 && alnum(*c))) { separator = length != 0; continue; }
        if (length + (separator ? 2u : 1u) >= capacity) break;
        if (separator) out[length++] = '-';
        separator = 0;
        out[length++] = (char)lower(*c);
    }
    out[length] = 0;
    return length;
}

struct P4DataKeys {
    P4DataKind kind;
    size_t count;
    char **keys, **names;
    uint8_t *placeholder;
};
void p4_data_keys_destroy(P4DataKeys *keys)
{
    if (!keys) return;
    for (size_t i = 0; i < keys->count; ++i) { free(keys->keys[i]); free(keys->names[i]); }
    free(keys->keys); free(keys->names); free(keys->placeholder); free(keys);
}
static char *duplicate(const char *text)
{
    const size_t size = strlen(text) + 1;
    char *copy = malloc(size);
    if (copy) memcpy(copy, text, size);
    return copy;
}
static int key_taken(const P4DataKeys *keys, size_t before, const char *key)
{
    for (size_t i = 0; i < before; ++i) if (keys->keys[i] && !strcmp(keys->keys[i], key)) return 1;
    return 0;
}
P4DataKeys *p4_data_keys_create(P4DataKind kind, const char *const *names, size_t count)
{
    if ((unsigned)kind >= P4_DATA_KIND_COUNT || (count && !names)) return NULL;
    P4DataKeys *keys = calloc(1, sizeof *keys);
    if (!keys) return NULL;
    keys->kind = kind; keys->count = count;
    keys->keys = calloc(count ? count : 1, sizeof *keys->keys);
    keys->names = calloc(count ? count : 1, sizeof *keys->names);
    keys->placeholder = calloc(count ? count : 1, 1);
    if (!keys->keys || !keys->names || !keys->placeholder) { p4_data_keys_destroy(keys); return NULL; }
    for (size_t id = 0; id < count; ++id) {
        if (!names[id]) continue;
        char slug[64], key[96];
        keys->names[id] = duplicate(names[id]);
        if (!keys->names[id]) { p4_data_keys_destroy(keys); return NULL; }
        if (p4_data_placeholder(names[id]) || !p4_data_slug(names[id], slug, sizeof slug)) {
            keys->placeholder[id] = 1;
            snprintf(key, sizeof key, "p4:%s:#%lu", kinds[kind], (unsigned long)id);
        } else {
            snprintf(key, sizeof key, "p4:%s:%s", kinds[kind], slug);
            for (unsigned suffix = 2; key_taken(keys, id, key); ++suffix)
                snprintf(key, sizeof key, "p4:%s:%s-%u", kinds[kind], slug, suffix);
        }
        keys->keys[id] = duplicate(key);
        if (!keys->keys[id]) { p4_data_keys_destroy(keys); return NULL; }
    }
    return keys;
}
size_t p4_data_keys_count(const P4DataKeys *keys) { return keys ? keys->count : 0; }
const char *p4_data_keys_at(const P4DataKeys *keys, uint32_t id) { return keys && id < keys->count ? keys->keys[id] : NULL; }
const char *p4_data_keys_name(const P4DataKeys *keys, uint32_t id) { return keys && id < keys->count ? keys->names[id] : NULL; }
int p4_data_keys_placeholder(const P4DataKeys *keys, uint32_t id) { return !keys || id >= keys->count || keys->placeholder[id]; }
int p4_data_keys_find(const P4DataKeys *keys, const char *key, uint32_t *id)
{
    if (!keys || !key) return -1;
    for (size_t i = 0; i < keys->count; ++i)
        if (keys->keys[i] && !strcmp(keys->keys[i], key)) { *id = (uint32_t)i; return 0; }
    return -1;
}

static void match_add(P4DataMatch *out, size_t index)
{
    if (out->count < P4_DATA_MATCH_MAX) out->indices[out->count] = index;
    ++out->count;
}
void p4_data_match(const P4DataRows *rows, const char *query, P4DataMatch *out)
{
    char wanted[64], form[64];
    memset(out, 0, sizeof *out);
    if (!rows || !p4_data_match_form(query, wanted, sizeof wanted)) return;
    for (size_t index = 0; index < rows->count; ++index) {
        const char *current = rows->name(rows->user, index);
        if (!current) continue;
        p4_data_match_form(current, form, sizeof form);
        if (!strcmp(form, wanted)) match_add(out, index);
    }
    const P4DataKeys *keys = rows->keys;
    if (out->count || !keys) return;
    for (size_t index = 0; index < rows->count && index < keys->count; ++index) {
        if (!keys->names[index] || keys->placeholder[index] || !rows->name(rows->user, index)) continue;
        p4_data_match_form(keys->names[index], form, sizeof form);
        if (!strcmp(form, wanted)) match_add(out, index);
    }
}

/* Optimal string alignment distance, capped at 64 bytes per side. */
static unsigned distance(const char *a, const char *b)
{
    size_t n = strlen(a), m = strlen(b);
    if (n > 63) n = 63;
    if (m > 63) m = 63;
    unsigned rows[3][64];
    for (size_t j = 0; j <= m; ++j) rows[1][j] = (unsigned)j;
    unsigned *previous2 = rows[0], *previous = rows[1], *current = rows[2];
    for (size_t i = 1; i <= n; ++i) {
        current[0] = (unsigned)i;
        for (size_t j = 1; j <= m; ++j) {
            const unsigned cost = a[i - 1] != b[j - 1];
            unsigned best = previous[j] + 1;
            if (current[j - 1] + 1 < best) best = current[j - 1] + 1;
            if (previous[j - 1] + cost < best) best = previous[j - 1] + cost;
            if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1] && previous2[j - 2] + 1 < best)
                best = previous2[j - 2] + 1;
            current[j] = best;
        }
        unsigned *spare = previous2;
        previous2 = previous; previous = current; current = spare;
    }
    return previous[m];
}
size_t p4_data_suggest(const P4DataRows *rows, const char *query, size_t *indices, size_t max)
{
    char wanted[64], form[64];
    unsigned scores[8];
    size_t found = 0;
    if (max > 8) max = 8;
    const size_t length = rows ? p4_data_match_form(query, wanted, sizeof wanted) : 0;
    if (!length || !max) return 0;
    const unsigned limit = 2u * (unsigned)(length / 3u > 2u ? length / 3u : 2u);
    for (size_t index = 0; index < rows->count; ++index) {
        const char *current = rows->name(rows->user, index);
        const size_t size = current ? p4_data_match_form(current, form, sizeof form) : 0;
        if (!size) continue;
        unsigned score = 2u * distance(wanted, form);
        int eligible = score <= limit;
        /* A typed prefix ("agil") or an overlong name ("agilaoo") ranks a
         * little ahead of its plain distance; a substring a little less. */
        if (length >= 3 && (!strncmp(form, wanted, length) || !strncmp(wanted, form, size))) {
            score = score > 2u ? score - 2u : 1u;
            eligible = 1;
        } else if (length >= 4 && strstr(form, wanted)) {
            score = score > 1u ? score - 1u : 1u;
            eligible = 1;
        }
        if (!eligible) continue;
        size_t at;
        if (found < max) at = found++;
        else if (score < scores[max - 1]) at = max - 1;
        else continue;
        while (at && scores[at - 1] > score) { scores[at] = scores[at - 1]; indices[at] = indices[at - 1]; --at; }
        scores[at] = score; indices[at] = index;
    }
    return found;
}

static const char *plural(P4DataKind kind)
{
    static const char *const names[P4_DATA_KIND_COUNT] = {
        "skills", "Personas", "enemies", "items", "floors", "formations", "encounter groups", "treasure groups"
    };
    return names[kind];
}
static size_t append(char *message, size_t capacity, size_t used, const char *format, ...)
{
    if (used >= capacity) return used;
    va_list args;
    va_start(args, format);
    const int written = vsnprintf(message + used, capacity - used, format, args);
    va_end(args);
    return written < 0 ? used : used + (size_t)written;
}
static int key_index(const P4DataRows *rows, const char *key, size_t *index)
{
    for (size_t i = 0; i < rows->count; ++i) {
        const char *candidate = rows->key(rows->user, i);
        if (candidate && !strcmp(candidate, key)) { *index = i; return 0; }
    }
    return -1;
}
int p4_data_lookup(const P4DataRows *rows, const char *query, size_t *index, char *message, size_t capacity)
{
    if (!rows || (unsigned)rows->kind >= P4_DATA_KIND_COUNT || !query || !index || !message || !capacity) return -1;
    message[0] = 0;
    const char *kind = kinds[rows->kind];
    if (strchr(query, ':')) {
        if (!key_index(rows, query, index)) return 0;
        snprintf(message, capacity, "no %s has the key \"%s\". Keys look like \"p4:%s:<name>\"; "
            "p4.data.find(\"%s\", \"...\") lists them.", kind, query, kind, kind);
        return 1;
    }
    P4DataMatch match;
    p4_data_match(rows, query, &match);
    if (match.count == 1) { *index = match.indices[0]; return 0; }
    size_t used;
    if (match.count > 1) {
        used = append(message, capacity, 0, "%zu %s are named \"%s\"; use a key instead:", match.count, plural(rows->kind), query);
        for (size_t i = 0; i < match.count && i < P4_DATA_MATCH_MAX; ++i) {
            char detail[64] = "";
            if (rows->detail) rows->detail(rows->user, match.indices[i], detail, sizeof detail);
            used = append(message, capacity, used, "%s %s%s%s%s", i ? "," : "", rows->key(rows->user, match.indices[i]),
                detail[0] ? " (" : "", detail, detail[0] ? ")" : "");
        }
        if (match.count > P4_DATA_MATCH_MAX) append(message, capacity, used, ", ...");
        return 2;
    }
    size_t near[3];
    const size_t found = p4_data_suggest(rows, query, near, 3);
    used = append(message, capacity, 0, "no %s named \"%s\".", kind, query);
    for (size_t i = 0; i < found; ++i) {
        const char *name = rows->name(rows->user, near[i]);
        char shown[64];
        snprintf(shown, sizeof shown, "%s", name ? name : "");
        used = append(message, capacity, used, "%s\"%s\" (%s)", i ? ", " : " Did you mean ", shown, rows->key(rows->user, near[i]));
    }
    if (found) append(message, capacity, used, "?");
    else append(message, capacity, used, " p4.data.find(\"%s\", \"...\") searches by part of a name.", kind);
    return 1;
}
