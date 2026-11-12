/* p4.data pure layer: row decode at the documented offsets, name forms, key
 * assignment and lookup messages, on synthetic rows only (no retail data). */
#include "p4pc/game_data.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put32(uint8_t *p, uint32_t v) { put16(p, (uint16_t)v); put16(p + 2, (uint16_t)(v >> 16)); }

static void skills(void)
{
    uint8_t element[2] = {1, 0}, row[40] = {0};
    row[0x01] = 3; row[0x02] = 2; row[0x03] = 2; put16(row + 0x04, 8);
    row[0x08] = 0; row[0x09] = 2; row[0x0E] = 98; row[0x0F] = 1; row[0x10] = 3;
    row[0x11] = 1; put16(row + 0x12, 100); row[0x18] = 1; row[0x19] = 40; put32(row + 0x1C, 0x20);
    put16(row + 0x20, 0x05); put16(row + 0x22, 0x0800); row[0x24] = 3; row[0x25] = 20;
    P4DataSkill s;
    p4_data_skill_decode(2, element, row, &s);
    assert(s.id == 2 && s.element == 1 && !s.passive && s.usable == 3 && s.damage_stat == 2);
    assert(s.cost_kind == 2 && s.cost == 8 && s.accuracy == 98 && s.hits_min == 1 && s.hits_max == 3);
    assert(s.hp_effect == 1 && s.power == 100 && s.ailment_mode == 1 && s.ailment_chance == 40 && s.ailments == 0x20);
    assert(s.buffs == 5 && s.barriers == 0x800 && s.special == 3 && s.critical == 20);
    assert(!strcmp(p4_data_skill_kind(&s), "magic") && !strcmp(p4_data_skill_targets(&s), "one_foe"));
    row[0x08] = 1; p4_data_skill_decode(2, element, row, &s);
    assert(!strcmp(p4_data_skill_targets(&s), "all_foes"));
    row[0x08] = 0; row[0x09] = 1; row[0x0A] = 1; p4_data_skill_decode(2, element, row, &s);
    assert(!strcmp(p4_data_skill_targets(&s), "self"));
    row[0x0A] = 4; p4_data_skill_decode(2, element, row, &s);
    assert(!strcmp(p4_data_skill_targets(&s), "one_fallen_ally"));
    row[0x09] = 3; p4_data_skill_decode(2, element, row, &s);
    assert(!strcmp(p4_data_skill_targets(&s), "everyone"));
    static const struct { uint8_t element; const char *kind; } kinds[] = {
        {0, "physical"}, {3, "magic"}, {5, "almighty"}, {7, "magic"}, {9, "ailment"}, {16, "heal"}, {17, "support"}, {18, "special"}
    };
    for (size_t i = 0; i < sizeof kinds / sizeof kinds[0]; ++i) {
        element[0] = kinds[i].element;
        p4_data_skill_decode(2, element, row, &s);
        assert(!strcmp(p4_data_skill_kind(&s), kinds[i].kind));
    }
    const uint8_t passive[2] = {255, 1};
    p4_data_skill_decode(450, passive, NULL, &s);
    assert(s.passive && s.power == 0 && !strcmp(p4_data_skill_kind(&s), "passive"));
}

static void enemies_and_personas(void)
{
    uint8_t row[60] = {0}, affinity[32] = {0};
    put16(row, 0x40); row[2] = 1; row[3] = 22; put16(row + 4, 168); put16(row + 6, 80);
    memcpy(row + 8, "\x0d\x0c\x10\x08\x0c", 5);
    put16(row + 0x0E, 0x28); put16(row + 0x10, 0x1D8);
    put16(row + 0x1E, 260); put16(row + 0x20, 240);
    put16(row + 0x22, 0x502); row[0x24] = 140; put16(row + 0x2E, 0x503); row[0x30] = 20;
    row[0x38] = 1; row[0x39] = 98; put16(row + 0x3A, 53);
    put16(affinity + 1 * 2, 0x0800); put16(affinity + 2 * 2, 0x1000); put16(affinity + 6 * 2, 0x0100);
    put16(affinity + 7 * 2, 0x0200); put16(affinity + 4 * 2, 0x0400);
    P4DataEnemy e;
    p4_data_enemy_decode(2, row, affinity, &e);
    assert(e.flags == 0x40 && e.arcana == 1 && e.level == 22 && e.hp == 168 && e.sp == 80);
    assert(e.stats[0] == 13 && e.stats[4] == 12 && e.skills[0] == 0x28 && e.skills[1] == 0x1D8 && !e.skills[2]);
    assert(e.exp == 260 && e.yen == 240 && e.drops[0].item == 0x502 && e.drops[0].weight == 140);
    assert(e.drops[3].item == 0x503 && e.drops[3].weight == 20 && !e.drops[1].item);
    assert(e.attack_element == 1 && e.attack_accuracy == 98 && e.attack_power == 53);
    assert(p4_data_affinity_kind(0, e.affinity[0]) == P4_DATA_AFFINITY_NORMAL);
    assert(p4_data_affinity_kind(1, e.affinity[1]) == P4_DATA_AFFINITY_WEAK);
    assert(p4_data_affinity_kind(2, e.affinity[2]) == P4_DATA_AFFINITY_RESIST);
    assert(p4_data_affinity_kind(4, e.affinity[4]) == P4_DATA_AFFINITY_DRAIN);
    assert(p4_data_affinity_kind(6, e.affinity[6]) == P4_DATA_AFFINITY_NULL);
    assert(p4_data_affinity_kind(7, e.affinity[7]) == P4_DATA_AFFINITY_REPEL);
    assert(p4_data_affinity_kind(1, 0x0900) == P4_DATA_AFFINITY_NULL); /* null resolves first */
    assert(p4_data_affinity_rate(0x0014) == 100 && p4_data_affinity_rate(0x0800) == 125 &&
        p4_data_affinity_rate(0x1000) == 50 && p4_data_affinity_rate(0x0828) == 200 && p4_data_affinity_rate(0x2000) == 100);
    /* Ailment columns are chance rates; 0x20 is the no-effect flag. */
    assert(p4_data_affinity_kind(9, 0x2000) == P4_DATA_AFFINITY_NULL && p4_data_affinity_kind(9, 0x0014) == P4_DATA_AFFINITY_NORMAL);
    assert(p4_data_affinity_kind(9, 0x00FF) == P4_DATA_AFFINITY_WEAK && p4_data_affinity_kind(9, 0x000A) == P4_DATA_AFFINITY_RESIST);
    assert(p4_data_affinity_kind(9, 0x0000) == P4_DATA_AFFINITY_NULL && p4_data_affinity_kind(0, 0x0014) == P4_DATA_AFFINITY_NORMAL);
    uint16_t packed;
    for (uint32_t column = 0; column < P4_DATA_AFFINITY_COLUMNS; ++column)
        for (unsigned kind = P4_DATA_AFFINITY_NORMAL; kind <= P4_DATA_AFFINITY_DRAIN; ++kind) {
            const int encoded = p4_data_affinity_encode(column, (P4DataAffinity)kind, &packed);
            if (column >= 8 && kind >= P4_DATA_AFFINITY_REPEL) { assert(encoded); continue; }
            assert(!encoded && p4_data_affinity_kind(column, packed) == (P4DataAffinity)kind);
        }
    P4DataAffinity parsed;
    assert(!p4_data_affinity_parse("Absorb", &parsed) && parsed == P4_DATA_AFFINITY_DRAIN);
    assert(!p4_data_affinity_parse("immune", &parsed) && parsed == P4_DATA_AFFINITY_NULL && p4_data_affinity_parse("x", &parsed));

    uint8_t meta[14] = {0}, growth[70] = {0}, party[622] = {0};
    meta[2] = 1; meta[3] = 16; memcpy(meta + 4, "\x0a\x0e\x0c\x0b\x08", 5); put16(meta + 0x0A, 3);
    memcpy(growth, "\x0a\x14\x14\x0f\x0f", 5);
    growth[6] = 0; growth[7] = 1; put16(growth + 8, 0x31);       /* innate */
    growth[10] = 3; growth[11] = 1; put16(growth + 12, 0x1A);    /* base + 3 */
    growth[14] = 4; growth[15] = 0; put16(growth + 16, 0x1B);    /* empty kind: skipped */
    P4DataPersona p;
    p4_data_persona_decode(7, meta, growth, NULL, affinity, &p);
    assert(p.arcana == 1 && p.level == 16 && p.stats[1] == 14 && p.inheritance == 3 && p.growth[1] == 20);
    assert(!p.party && p.learn_count == 2 && p.learnset[0].level == 16 && p.learnset[0].skill == 0x31);
    assert(p.learnset[1].level == 19 && p.learnset[1].skill == 0x1A);
    put16(party, 2); put16(party + 2, 99);
    party[4] = 0; party[5] = 1; put16(party + 6, 0x0E);
    party[8] = 6; party[9] = 2; put16(party + 10, 0x0E);         /* forget: not a learn */
    party[12] = 6; party[13] = 1; put16(party + 14, 0xD6);
    party[20] = 9; party[21] = 1; put16(party + 22, 0x10);       /* after the empty event: ignored */
    p4_data_persona_decode(0xC0, meta, growth, party, affinity, &p);
    assert(p.party && p.character == 2 && p.learn_count == 2);
    assert(p.learnset[0].level == 0 && p.learnset[1].level == 6 && p.learnset[1].skill == 0xD6);
    assert(!strcmp(p4_data_character_name(2), "yosuke"));
}

static void items(void)
{
    uint8_t row[68] = {0};
    put32(row, 0); put32(row + 4, 2); put16(row + 8, 55); put16(row + 0x0A, 95);
    put16(row + 0x14, 1); put16(row + 0x16, 2); put16(row + 0x18, 3); put16(row + 0x1A, 4); put16(row + 0x1C, 5);
    put32(row + 0x24, 0x1D7); put32(row + 0x30, 0x40 | 0x400); put32(row + 0x34, 11800);
    P4DataItem item;
    p4_data_item_decode(0x2B, row, &item);
    assert(item.kind == 0 && item.attack == 55 && item.hit == 95 && item.skill == 0x1D7 && item.price == 11800);
    /* strength, magic, endurance, agility, luck from the row's St, En, Ma order */
    assert(item.stats[0] == 1 && item.stats[1] == 3 && item.stats[2] == 2 && item.stats[3] == 4 && item.stats[4] == 5);
    assert(item.users == ((1u << 1) | (1u << 5)) && !strcmp(p4_data_item_kind(&item), "weapon"));
    /* Weapon store gates: 10 Supple Metal (0x50D) sold; the second gate is off. */
    put32(row + 0x3C, 0x50D << 8 | 10); put32(row + 0x40, 0x500 << 8);
    p4_data_item_decode(0x3, row, &item);
    assert(!strcmp(p4_data_item_store(&item), "weapon") && p4_data_item_sold(&item));
    assert(item.unlock[0].material == 0x50D && item.unlock[0].count == 10 && !item.unlock[1].count);
    assert(p4_data_unlock_encode(item.unlock[0]) == (0x50Du << 8 | 10));
    /* Retail's never-sold gear: the Blank material, count 1. */
    put32(row + 0x3C, 0x500 << 8 | 1);
    p4_data_item_decode(0x1, row, &item);
    assert(!p4_data_item_sold(&item));
    put32(row, 3);
    row[0x38] = 3; row[0x39] = 31;
    p4_data_item_decode(0x302, row, &item);
    assert(!strcmp(p4_data_item_kind(&item), "consumable") && !strcmp(p4_data_item_store(&item), "item"));
    assert(item.month == 3 && item.day == 31 && !p4_data_item_sold(&item));
    row[0x38] = 4; row[0x39] = 1;
    p4_data_item_decode(0x302, row, &item);
    assert(p4_data_item_sold(&item));
    put32(row, 5);
    p4_data_item_decode(0x50D, row, &item);
    assert(!p4_data_item_store(&item) && !p4_data_item_sold(&item));
}

static void vocabulary(void)
{
    uint32_t value;
    assert(!p4_data_element_parse("Fire", &value) && value == 1);
    assert(!p4_data_element_parse("elec", &value) && value == 3);
    assert(!p4_data_element_parse("panic", &value) && value == 8);
    assert(p4_data_element_parse("plasma", &value));
    assert(!p4_data_ailment_parse("Poison", &value) && value == 0x20);
    assert(!p4_data_ailment_parse("confuse", &value) && value == 0x10);
    assert(!p4_data_ailment_parse("ko", &value) && value == 0x80000);
    assert(!strcmp(p4_data_ailment_name(0x04), "fear"));
    assert(!p4_data_character_parse("Teddie", &value) && value == 8);
    P4DataKind kind;
    assert(!p4_data_kind_parse("enemy", &kind) && kind == P4_DATA_ENEMY && p4_data_kind_parse("Enemy", &kind));
}

static void names(void)
{
    char out[64];
    assert(p4_data_match_form("God's Hand", out, sizeof out) == 8 && !strcmp(out, "godshand"));
    assert(p4_data_slug("God's Hand", out, sizeof out) && !strcmp(out, "gods-hand"));
    assert(p4_data_slug("Die for Me!", out, sizeof out) && !strcmp(out, "die-for-me"));
    assert(p4_data_slug("5-Iron", out, sizeof out) && !strcmp(out, "5-iron"));
    assert(p4_data_slug("  Mr. Edogawa ", out, sizeof out) && !strcmp(out, "mr-edogawa"));
    assert(!p4_data_slug("!!", out, sizeof out));
    assert(p4_data_placeholder("") && p4_data_placeholder("Blank") && p4_data_placeholder("blank"));
    assert(p4_data_placeholder("19C") && p4_data_placeholder("000") && p4_data_placeholder("0x1A2"));
    assert(!p4_data_placeholder("Bead") && !p4_data_placeholder("Agi") && !p4_data_placeholder("Face"));
    assert(!p4_data_placeholder("Rainy Sister 4") && !p4_data_placeholder("12345"));
    /* Untranslated floor rows hold only two-byte glyphs. */
    assert(p4_data_placeholder("\x80\x90\x80\x91\x80\x94") && !p4_data_placeholder("Yukiko's Castle 3F"));
}

static void dungeons(void)
{
    uint8_t selector[10] = {0};
    put16(selector, 48); selector[2] = 2; selector[3] = 6; selector[4] = 3; put16(selector + 6, 2);
    P4DataFloor floor;
    p4_data_floor_decode(8, selector, &floor);
    assert(floor.id == 8 && floor.encounters == 48 && floor.refill == 2 && floor.spawn == 6 && floor.chests == 3 &&
        floor.treasure == 2);
    uint8_t formation[24] = {0};
    put16(formation + 8, 0x2B); put16(formation + 10, 0x2C);
    P4DataFormation f;
    p4_data_formation_decode(17, formation, &f);
    assert(f.enemies[0] == 0x2B && f.enemies[1] == 0x2C && !f.enemies[2]);
    /* Rates: normal, special, rare for each day variant; then 2 x 30 slots. */
    uint8_t group[248] = {0};
    group[0] = 97; group[1] = 90; group[2] = 3; group[3] = 10; group[4] = 1; group[5] = 0;
    put16(group + 8, 16); put16(group + 10, 20);                        /* normal slot 1 */
    put16(group + 8 + 20 * 4, 882); put16(group + 8 + 20 * 4 + 2, 100); /* special slot 1 */
    put16(group + 8 + 30 * 4 + 25 * 4, 900); put16(group + 8 + 30 * 4 + 25 * 4 + 2, 7); /* alternate rare 1 */
    P4DataEncounters g;
    p4_data_encounters_decode(1, group, &g);
    assert(g.rates[0][P4_DATA_ENCOUNTER_NORMAL] == 97 && g.rates[1][P4_DATA_ENCOUNTER_NORMAL] == 90);
    assert(g.rates[0][P4_DATA_ENCOUNTER_SPECIAL] == 3 && g.rates[1][P4_DATA_ENCOUNTER_SPECIAL] == 10);
    assert(g.rates[0][P4_DATA_ENCOUNTER_RARE] == 1 && !g.rates[1][P4_DATA_ENCOUNTER_RARE]);
    assert(g.slots[0][0].formation == 16 && g.slots[0][0].weight == 20 && g.slots[0][20].formation == 882);
    assert(g.slots[1][25].formation == 900 && g.slots[1][25].weight == 7);
    uint32_t first, count;
    p4_data_encounter_class_slots(P4_DATA_ENCOUNTER_RARE, &first, &count);
    assert(first == 25 && count == 5 && !strcmp(p4_data_encounter_class_name(P4_DATA_ENCOUNTER_SPECIAL), "special"));
    assert(p4_data_encounter_slot_offset(1, 25) == 8 + 30 * 4 + 25 * 4);
    uint8_t records[348] = {0};
    put16(records, 10); put16(records + 2, 0x302); records[7] = 1;
    put16(records + 12, 5); put16(records + 14, 0x303);
    P4DataTreasure t;
    p4_data_treasure_decode(2, records, &t);
    assert(t.records[0].weight == 10 && t.records[0].item == 0x302 && t.records[0].locked);
    assert(t.records[1].item == 0x303 && !t.records[1].locked && !t.records[2].item);
    assert(!strcmp(p4_data_kind_name(P4_DATA_ENCOUNTERS), "encounters"));
}

/* Rows for the lookup tests: current names may differ from the disc's. */
typedef struct Table { const char *const *current; const P4DataKeys *keys; size_t count; } Table;
static const char *table_name(void *user, size_t index)
{
    const Table *t = user;
    const char *name = t->current[index];
    return name && !p4_data_placeholder(name) ? name : NULL;
}
static const char *table_key(void *user, size_t index) { return p4_data_keys_at(((const Table *)user)->keys, (uint32_t)index); }
static void table_detail(void *user, size_t index, char *out, size_t capacity)
{
    (void)user;
    snprintf(out, capacity, "level %u", (unsigned)(10 + index));
}

static void keys_and_lookup(void)
{
    static const char *const disc[] = {
        "Attack", "Agi", "Agilao", "Agidyne", "Blank", "19C", "Balzac", "Balzac", "Balzac 2", NULL, "Medicine", "Mudo"
    };
    const size_t count = sizeof disc / sizeof disc[0];
    P4DataKeys *keys = p4_data_keys_create(P4_DATA_SKILL, disc, count);
    assert(keys && p4_data_keys_count(keys) == count);
    assert(!strcmp(p4_data_keys_at(keys, 2), "p4:skill:agilao"));
    assert(!strcmp(p4_data_keys_at(keys, 4), "p4:skill:#4") && p4_data_keys_placeholder(keys, 4));
    assert(!strcmp(p4_data_keys_at(keys, 5), "p4:skill:#5"));
    assert(!strcmp(p4_data_keys_at(keys, 6), "p4:skill:balzac") && !strcmp(p4_data_keys_at(keys, 7), "p4:skill:balzac-2"));
    /* A later real "Balzac 2" cannot take a suffix key already issued. */
    assert(!strcmp(p4_data_keys_at(keys, 8), "p4:skill:balzac-2-2"));
    assert(!p4_data_keys_at(keys, 9) && !p4_data_keys_name(keys, 9) && !p4_data_keys_at(keys, (uint32_t)count));
    uint32_t id;
    assert(!p4_data_keys_find(keys, "p4:skill:mudo", &id) && id == 11 && p4_data_keys_find(keys, "p4:skill:zio", &id));

    /* A mod renamed Medicine; the key and the disc name still reach it. */
    static const char *const current[] = {
        "Attack", "Agi", "Agilao", "Agidyne", "Blank", "Frost Dive", "Balzac", "Balzac", "Balzac 2", NULL, "Herbal Medicine", "Mudo"
    };
    Table table = {current, keys, count};
    P4DataRows rows = {P4_DATA_SKILL, count, keys, &table, table_name, table_key, table_detail};
    char message[512];
    size_t index = 99;
    assert(!p4_data_lookup(&rows, "agilao", &index, message, sizeof message) && index == 2);
    assert(!p4_data_lookup(&rows, " AGI LAO ", &index, message, sizeof message) && index == 2);
    assert(!p4_data_lookup(&rows, "Herbal Medicine", &index, message, sizeof message) && index == 10);
    assert(!p4_data_lookup(&rows, "Medicine", &index, message, sizeof message) && index == 10);
    assert(!p4_data_lookup(&rows, "p4:skill:medicine", &index, message, sizeof message) && index == 10);
    /* A placeholder disc row named by a mod is found by its current name. */
    assert(!p4_data_lookup(&rows, "Frost Dive", &index, message, sizeof message) && index == 5);
    assert(p4_data_lookup(&rows, "Blank", &index, message, sizeof message) == 1);

    assert(p4_data_lookup(&rows, "Agilaoo", &index, message, sizeof message) == 1);
    static const char typo[] = "no skill named \"Agilaoo\". Did you mean \"Agilao\" (p4:skill:agilao), \"Agi\" (p4:skill:agi)?";
    assert(!strcmp(message, typo));
    assert(p4_data_lookup(&rows, "Agidine", &index, message, sizeof message) == 1);
    assert(strstr(message, "\"Agidyne\" (p4:skill:agidyne)"));
    assert(p4_data_lookup(&rows, "Zionga", &index, message, sizeof message) == 1);
    assert(strstr(message, "p4.data.find(\"skill\""));
    assert(p4_data_lookup(&rows, "Balzac", &index, message, sizeof message) == 2);
    assert(!strcmp(message, "2 skills are named \"Balzac\"; use a key instead: p4:skill:balzac (level 16),"
        " p4:skill:balzac-2 (level 17)"));
    assert(p4_data_lookup(&rows, "p4:skill:nope", &index, message, sizeof message) == 1 && strstr(message, "no skill has the key"));
    assert(p4_data_lookup(NULL, "x", &index, message, sizeof message) == -1);

    size_t near[3];
    assert(p4_data_suggest(&rows, "agi", near, 3) >= 2 && near[0] == 1);
    p4_data_keys_destroy(keys);
}

int main(void)
{
    skills();
    enemies_and_personas();
    items();
    vocabulary();
    names();
    dungeons();
    keys_and_lookup();
    puts("game_data: ok");
    return 0;
}
