/* p4.data: the semantic catalog over the effective game tables
 * (docs/modding/systems/data-and-edits.md).
 *
 * This header is the pure layer: exact PS2 row layouts decoded into typed
 * fields, the name forms used for matching and keys, and key assignment from
 * the unmodded disc names. No guest memory, file I/O or Luau here; the
 * runtime catalog (game_data_catalog.c) supplies the bytes. */
#ifndef P4PC_GAME_DATA_H
#define P4PC_GAME_DATA_H
#include <stddef.h>
#include <stdint.h>

/* Skills, Personas, enemies and items carry disc names. Floors carry the
 * executable's floor names; formations, encounter groups and treasure groups
 * are unnamed rows keyed by ID ("p4:formation:#12"). */
typedef enum P4DataKind {
    P4_DATA_SKILL, P4_DATA_PERSONA, P4_DATA_ENEMY, P4_DATA_ITEM,
    P4_DATA_FLOOR, P4_DATA_FORMATION, P4_DATA_ENCOUNTERS, P4_DATA_TREASURE, P4_DATA_KIND_COUNT
} P4DataKind;
/* "skill", "persona", "enemy", "item", "floor", "formation", "encounters",
 * "treasure"; NULL for an invalid kind. */
const char *p4_data_kind_name(P4DataKind kind);
/* 0 and *kind for a kind name, -1 otherwise. */
int p4_data_kind_parse(const char *name, P4DataKind *kind);

/* Retail element IDs 0..18 (SKILL0 byte 0, affinity columns 0..15). 255 is
 * "none" (passive and blank skills). Ailment elements 8..15 share their name
 * with the ailment they inflict. */
#define P4_DATA_ELEMENTS 19u
#define P4_DATA_AFFINITY_COLUMNS 16u
#define P4_DATA_ELEMENT_NONE 255u
const char *p4_data_element_name(uint32_t element);
/* Accepts every name above plus "phys", "elec", "electricity", "panic",
 * "confuse", "enrage", "mute"; case-insensitive. 0 found, -1 otherwise. */
int p4_data_element_parse(const char *name, uint32_t *element);

/* DatUnit.bad / SKILL1 +0x1C ailment bits. */
#define P4_DATA_AILMENTS 10u
typedef struct P4DataAilment { const char *name; uint32_t bit; } P4DataAilment;
const P4DataAilment *p4_data_ailment_at(size_t index);
const char *p4_data_ailment_name(uint32_t bit);
int p4_data_ailment_parse(const char *name, uint32_t *bit);

/* Packed UNIT1/UNIT2 words: high byte flags (0x01 null, 0x02 repel, 0x04
 * drain, 0x08 weak, 0x10 resist; 0x20 the ailment no-effect flag
 * datCalcInflictStatus reads), low byte the rate in 5% steps (20 is 100%;
 * 0 takes the flag's default: weak 125, null/repel/drain 100, resist 50). */
typedef enum P4DataAffinity {
    P4_DATA_AFFINITY_NORMAL, P4_DATA_AFFINITY_WEAK, P4_DATA_AFFINITY_RESIST,
    P4_DATA_AFFINITY_NULL, P4_DATA_AFFINITY_REPEL, P4_DATA_AFFINITY_DRAIN
} P4DataAffinity;
const char *p4_data_affinity_name(P4DataAffinity affinity);
int p4_data_affinity_parse(const char *name, P4DataAffinity *affinity);
/* Attack columns 0..7 by flag (retail rows set at most one; with several,
 * null, repel, drain, weak, resist in that order). Ailment columns 8..15 are chance rates: the no-effect
 * or null flag, or a zero rate, is null; otherwise above 100% is weak, below
 * resist. */
P4DataAffinity p4_data_affinity_kind(uint32_t column, uint16_t packed);
/* The rate in percent (0..1275), with the flag defaults for a zero rate. */
uint32_t p4_data_affinity_rate(uint16_t packed);
/* The word retail uses for a kind: attack columns the bare flag (weak 0x0800,
 * resist 0x1000, null 0x0100, repel 0x0200, drain 0x0400) or 100% normal
 * (0x0014); ailment columns 0x2000 null, 0x00FF weak (retail's most common
 * vulnerability, x12.75), 0x000A resist (50%), 0x0014 normal; repel and drain
 * have no ailment form (-1). */
int p4_data_affinity_encode(uint32_t column, P4DataAffinity affinity, uint16_t *packed);

/* Party members in DatUnit order: 1 protagonist .. 8 teddie. */
#define P4_DATA_CHARACTERS 8u
const char *p4_data_character_name(uint32_t character);
int p4_data_character_parse(const char *name, uint32_t *character);

/* ---- typed rows ---------------------------------------------------------- */

typedef struct P4DataSkill {
    uint32_t id;
    uint8_t element;        /* SKILL0 byte 0 */
    uint8_t passive;        /* no SKILL1 row: every field below is zero */
    uint8_t usable;         /* +0x01: bit0 field, bit1 battle */
    uint8_t damage_stat;    /* +0x02: 1 strength, 2 magic */
    uint8_t cost_kind;      /* +0x03: 1 percent of max HP, 2 SP */
    uint16_t cost;          /* +0x04 */
    uint8_t target_scope;   /* +0x08: 0 one, 1 all, 2 everyone */
    uint8_t target_side;    /* +0x09: bit0 allies, bit1 foes */
    uint8_t target_filter;  /* +0x0A: 1 self, 4 fallen ally */
    uint8_t accuracy;       /* +0x0E, percent */
    uint8_t hits_min;       /* +0x0F */
    uint8_t hits_max;       /* +0x10 */
    uint8_t hp_effect;      /* +0x11: 1 damage, 2 heal, ... */
    uint16_t power;         /* +0x12 */
    uint8_t sp_effect;      /* +0x14 */
    uint16_t sp_power;      /* +0x16 */
    uint8_t ailment_mode;   /* +0x18: 1 inflict, 2 cure */
    uint8_t ailment_chance; /* +0x19, percent */
    uint32_t ailments;      /* +0x1C, DatUnit.bad bits */
    uint16_t buffs;         /* +0x20 */
    uint16_t barriers;      /* +0x22 */
    uint8_t special;        /* +0x24 */
    uint8_t critical;       /* +0x25, percent */
} P4DataSkill;
/* element is the 2-byte SKILL0 row; metadata the 40-byte SKILL1 row or NULL
 * for a skill without one (passives). */
void p4_data_skill_decode(uint32_t id, const uint8_t element[2], const uint8_t *metadata, P4DataSkill *out);
/* "physical", "magic", "almighty", "ailment", "heal", "support", "special",
 * "passive". */
const char *p4_data_skill_kind(const P4DataSkill *skill);
/* "one_foe", "all_foes", "one_ally", "all_allies", "self", "one_fallen_ally",
 * "all_fallen_allies", "everyone" or "other". */
const char *p4_data_skill_targets(const P4DataSkill *skill);

#define P4_DATA_STATS 5u /* strength, magic, endurance, agility, luck */
#define P4_DATA_ENEMY_SKILLS 8u
#define P4_DATA_ENEMY_DROPS 4u
typedef struct P4DataEnemy {
    uint32_t id;
    uint16_t flags;          /* +0x00 */
    uint8_t arcana;          /* +0x02 */
    uint8_t level;           /* +0x03 */
    uint16_t hp, sp;         /* +0x04, +0x06 */
    uint8_t stats[P4_DATA_STATS]; /* +0x08 */
    uint16_t skills[P4_DATA_ENEMY_SKILLS]; /* +0x0E, 0 empty */
    uint16_t exp, yen;       /* +0x1E, +0x20 */
    struct { uint16_t item; uint8_t weight; } drops[P4_DATA_ENEMY_DROPS]; /* +0x22 + 4n; weight in 1/200 */
    uint8_t attack_element;  /* +0x38 */
    uint8_t attack_accuracy; /* +0x39 */
    uint16_t attack_power;   /* +0x3A */
    uint16_t affinity[P4_DATA_AFFINITY_COLUMNS]; /* UNIT1 packed words */
} P4DataEnemy;
void p4_data_enemy_decode(uint32_t id, const uint8_t metadata[60], const uint8_t affinity[32], P4DataEnemy *out);

#define P4_DATA_PERSONA_LEARNSET 32u
typedef struct P4DataPersona {
    uint32_t id;
    uint16_t flags;          /* PERSONA0 +0x00 */
    uint8_t arcana, level;   /* +0x02, +0x03 */
    uint8_t stats[P4_DATA_STATS]; /* +0x04 */
    uint16_t inheritance;    /* +0x0A */
    uint8_t growth[P4_DATA_STATS]; /* PERSONA1 +0x00: weighted level-up stat distribution */
    uint8_t party;           /* learnset from the PERSONA2 party block */
    uint8_t character;       /* party owner, 0 for a fusion Persona */
    uint32_t learn_count;
    struct { uint8_t level; uint16_t skill; } learnset[P4_DATA_PERSONA_LEARNSET]; /* absolute levels */
    uint16_t affinity[P4_DATA_AFFINITY_COLUMNS]; /* UNIT2 packed words */
} P4DataPersona;
/* party is the 622-byte PERSONA2 row of a party Persona or NULL; learn
 * levels are absolute either way (PERSONA1 stores levels after the base). */
void p4_data_persona_decode(uint32_t id, const uint8_t metadata[14], const uint8_t growth[70],
    const uint8_t *party, const uint8_t affinity[32], P4DataPersona *out);
/* PERSONA2 holds the party Personas from 0xC0 in ID order (table-delta's
 * PARTY_FIRST/PARTY_ROWS). */
#define P4_DATA_PARTY_PERSONA_FIRST 0xC0u
#define P4_DATA_PARTY_PERSONAS 24u
#define P4_DATA_PARTY_ROW_SIZE 622u

/* Shops have no stock table. The item store lists every consumable from its
 * date (+0x38 month, +0x39 day; months run April to March, so 3/31 is never
 * in practice); the weapon store lists gear once both unlock gates pass. A
 * gate (+0x3C, +0x40) is a material's item ID in bits 8..19 and a count in
 * bits 0..7: the material's total sold must reach the count; count 0 is no
 * gate. Retail marks gear it never sells with the Blank material 0x500 and
 * count 1, which nothing can sell. */
#define P4_DATA_ITEM_UNLOCKS 2u
#define P4_DATA_ITEM_NEVER_MATERIAL 0x500u
#define P4_DATA_ITEM_NEVER_MONTH 3
#define P4_DATA_ITEM_NEVER_DAY 31
typedef struct P4DataUnlock {
    uint16_t material; /* an item ID */
    uint8_t count;     /* 0: no gate */
} P4DataUnlock;
typedef struct P4DataItem {
    uint32_t id;
    uint32_t kind;           /* +0x00: 0 weapon .. 5 material */
    uint16_t attack, hit;    /* weapons: +0x08, +0x0A */
    uint16_t defense, evasion; /* armor: +0x10, +0x12 */
    int16_t stats[P4_DATA_STATS]; /* +0x14, reordered to strength, magic, endurance, agility, luck */
    uint32_t skill;          /* +0x24: used (items) or granted (gear) */
    uint16_t users;          /* +0x30 bits 5..12, as 1 << (character - 1) */
    uint32_t price;          /* +0x34 */
    int8_t month, day;       /* +0x38, +0x39: a consumable's first day in the item store */
    P4DataUnlock unlock[P4_DATA_ITEM_UNLOCKS]; /* +0x3C, +0x40: gear's weapon-store gates */
} P4DataItem;
void p4_data_item_decode(uint32_t id, const uint8_t row[68], P4DataItem *out);
/* "weapon", "armor", "accessory", "consumable", "key", "material" or "other". */
const char *p4_data_item_kind(const P4DataItem *item);
/* The store that lists the item: "weapon" (gear), "item" (consumables) or
 * NULL. sold: 0 when retail never lists it (the 3/31 date or a Blank gate). */
const char *p4_data_item_store(const P4DataItem *item);
int p4_data_item_sold(const P4DataItem *item);
/* A gate word from its material and count, and back. */
uint32_t p4_data_unlock_encode(P4DataUnlock unlock);
P4DataUnlock p4_data_unlock_decode(uint32_t word);

/* ---- dungeons (ENCOUNT.TBL, game_encounters.h) --------------------------- */

/* A floor is a field step index (FldStepIndex 0x0015A160): its ENCOUNT
 * selector row picks the encounter group (+0), the shadow refill (+2) and
 * spawn (+3) counts, the chest count (+4) and the treasure group (+6). Its
 * name is the executable's 27-byte row at 0x005F4090. */
#define P4_DATA_FLOOR_NAME_SIZE 27u
typedef struct P4DataFloor {
    uint32_t id;
    uint16_t encounters, treasure;
    uint8_t refill, spawn, chests;
} P4DataFloor;
void p4_data_floor_decode(uint32_t id, const uint8_t row[10], P4DataFloor *out);
/* A formation is one battle: up to five enemy IDs at +8 (0 is none). */
#define P4_DATA_FORMATION_ENEMIES 5u
typedef struct P4DataFormation {
    uint32_t id;
    uint16_t enemies[P4_DATA_FORMATION_ENEMIES];
} P4DataFormation;
void p4_data_formation_decode(uint32_t id, const uint8_t row[24], P4DataFormation *out);
/* An encounter group (Encount_PickEncounter 0x00161630): per day variant
 * (the alternate list on calendar days that select it) the normal, special
 * and rare rates out of their sum, then 30 {formation, weight} slots: 0..19
 * normal, 20..24 special, 25..29 rare. A slot with formation 0 is empty. */
#define P4_DATA_ENCOUNTER_VARIANTS 2u
#define P4_DATA_ENCOUNTER_SLOTS 30u
typedef enum P4DataEncounterClass {
    P4_DATA_ENCOUNTER_NORMAL, P4_DATA_ENCOUNTER_SPECIAL, P4_DATA_ENCOUNTER_RARE, P4_DATA_ENCOUNTER_CLASSES
} P4DataEncounterClass;
typedef struct P4DataEncounter { uint16_t formation, weight; } P4DataEncounter;
typedef struct P4DataEncounters {
    uint32_t id;
    uint8_t rates[P4_DATA_ENCOUNTER_VARIANTS][P4_DATA_ENCOUNTER_CLASSES];
    P4DataEncounter slots[P4_DATA_ENCOUNTER_VARIANTS][P4_DATA_ENCOUNTER_SLOTS];
} P4DataEncounters;
void p4_data_encounters_decode(uint32_t id, const uint8_t row[248], P4DataEncounters *out);
/* "normal", "special", "rare"; the first slot and the slot count of each. */
const char *p4_data_encounter_class_name(P4DataEncounterClass value);
void p4_data_encounter_class_slots(P4DataEncounterClass value, uint32_t *first, uint32_t *count);
/* Byte offsets in a group row: a rate, and a slot's u16 formation (+2 weight). */
uint32_t p4_data_encounter_rate_offset(uint32_t variant, P4DataEncounterClass value);
uint32_t p4_data_encounter_slot_offset(uint32_t variant, uint32_t slot);
/* A treasure group (Encount_FindRecord 0x00161C80): 29 weighted records of
 * {u16 weight, u16 item, ..., +7 bit 0 locked chest}; item 0 is empty. */
#define P4_DATA_TREASURE_RECORDS 29u
typedef struct P4DataTreasureRecord { uint16_t weight, item; uint8_t locked; } P4DataTreasureRecord;
typedef struct P4DataTreasure {
    uint32_t id;
    P4DataTreasureRecord records[P4_DATA_TREASURE_RECORDS];
} P4DataTreasure;
void p4_data_treasure_decode(uint32_t id, const uint8_t row[348], P4DataTreasure *out);

/* ---- names and keys ------------------------------------------------------ */

/* Match form: ASCII letters and digits lowercased, every other ASCII byte
 * dropped, other bytes kept. Writes a NUL-terminated string (truncated to
 * capacity) and returns its length. */
size_t p4_data_match_form(const char *name, char *out, size_t capacity);
/* Disc rows the game never shows: empty, "blank", short hex IDs ("19C",
 * "000") and "0x..." placeholders. */
int p4_data_placeholder(const char *name);
/* Key slug: lowercase ASCII words joined by '-', apostrophes and periods
 * dropped ("God's Hand" -> "gods-hand"). Empty when the name has no ASCII
 * letter or digit. */
size_t p4_data_slug(const char *name, char *out, size_t capacity);

/* Keys for one kind, computed once from the unmodded names in ID order:
 * "p4:<kind>:<slug>", with "-2", "-3" in ID order for repeated slugs and
 * "p4:<kind>:#<id>" for placeholders. names[i] may be NULL (no row). */
typedef struct P4DataKeys P4DataKeys;
P4DataKeys *p4_data_keys_create(P4DataKind kind, const char *const *names, size_t count);
void p4_data_keys_destroy(P4DataKeys *keys);
size_t p4_data_keys_count(const P4DataKeys *keys);
const char *p4_data_keys_at(const P4DataKeys *keys, uint32_t id); /* NULL beyond count */
/* Unmodded name of the row (NULL for no row); placeholders included. */
const char *p4_data_keys_name(const P4DataKeys *keys, uint32_t id);
int p4_data_keys_placeholder(const P4DataKeys *keys, uint32_t id);
/* 0 and *id for an exact key, -1 otherwise. */
int p4_data_keys_find(const P4DataKeys *keys, const char *key, uint32_t *id);

/* One kind's rows as the catalog sees them. Rows are indexed 0..count-1;
 * the first p4_data_keys_count(keys) indices are the retail IDs, added rows
 * follow. name returns the current display name, NULL for an absent or
 * placeholder row; key the row's key; detail a short distinguishing note
 * for ambiguity messages ("level 22"), possibly empty. Returned strings stay
 * valid until the next call on the same rows. */
typedef struct P4DataRows {
    P4DataKind kind;
    size_t count;
    const P4DataKeys *keys;
    void *user;
    const char *(*name)(void *user, size_t index);
    const char *(*key)(void *user, size_t index);
    void (*detail)(void *user, size_t index, char *out, size_t capacity);
} P4DataRows;
#define P4_DATA_MATCH_MAX 16u
typedef struct P4DataMatch {
    size_t count;                       /* total matches; indices holds the first P4_DATA_MATCH_MAX */
    size_t indices[P4_DATA_MATCH_MAX];
} P4DataMatch;
/* Exact match-form matches on the current names; with none, on the
 * unmodded names, so a renamed row stays reachable by its disc name. */
void p4_data_match(const P4DataRows *rows, const char *query, P4DataMatch *out);
/* Up to max nearest current names (edit distance on the match form, prefix
 * and substring matches first), best first. Returns how many were written. */
size_t p4_data_suggest(const P4DataRows *rows, const char *query, size_t *indices, size_t max);
/* A key ("p4:skill:agilao", "owner:item:storm-blade") or a display name.
 * 0 found; 1 no such row and 2 ambiguous, each with a readable message:
 *   no skill named "Agilaoo". Did you mean "Agilao" (p4:skill:agilao)?
 *   2 enemies are named "Rainy Sister 2": p4:enemy:rainy-sister-2 (level 30), ...
 * -1 invalid arguments. */
int p4_data_lookup(const P4DataRows *rows, const char *query, size_t *index, char *message, size_t capacity);
#endif
