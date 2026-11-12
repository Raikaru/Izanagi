/* p4.data: the semantic catalog (docs/modding/systems/data-and-edits.md).
 *
 * Game-only, registered through the native bindings. Every answer is a fresh
 * table copied from the effective banks (include/p4pc/game_data_catalog.h):
 * no native calls, guest writes or RNG, so every function is legal in
 * synchronous UI callbacks and snapshot-mode callbacks. Unknown and ambiguous
 * names raise an error with suggestions or the candidates' keys. The same
 * readers serve data.luau (lua_data_edit.c), over the table seal's rows. */
#include "native_bindings.h"
#include "lua_data_internal.h"
#include "p4pc/game_data_catalog.h"
#include "p4pc/game_data_edit.h"
#include "p4pc/game_enemies.h"
#include <stdio.h>
#include <string.h>

static const char *const stat_names[P4_DATA_STATS] = {"st", "ma", "en", "ag", "lu"};

static void set_number(lua_State *L, const char *field, double value)
{
    lua_pushnumber(L, value);
    lua_setfield(L, -2, field);
}
static void set_string(lua_State *L, const char *field, const char *value)
{
    if (!value) return;
    lua_pushstring(L, value);
    lua_setfield(L, -2, field);
}
static void ready(lua_State *L, const char *function)
{
    const char *reason = NULL;
    if (p4_data_catalog_ready(&reason)) luaL_error(L, "p4.data.%s: %s", function, reason);
}
static P4DataKind kind_argument(lua_State *L, int index, const char *function)
{
    P4DataKind kind;
    const char *name = luaL_checkstring(L, index);
    if (p4_data_kind_parse(name, &kind))
        luaL_error(L, "p4.data.%s: unknown kind \"%s\" (skill, persona, enemy, item, floor, formation, encounters or "
            "treasure)", function, name);
    return kind;
}

/* {id, key, name}: the head of a full row, whose own `kind` is its category. */
static void push_head(lua_State *L, P4DataKind kind, size_t index, int fields)
{
    char name[32];
    lua_createtable(L, 0, fields);
    set_number(L, "id", p4_data_catalog_id(kind, index));
    set_string(L, "key", p4_data_catalog_key(kind, index));
    set_string(L, "name", p4_data_catalog_name(kind, index, name, sizeof name));
}
/* {kind, id, key, name} */
static void push_ref(lua_State *L, P4DataKind kind, size_t index)
{
    push_head(L, kind, index, 4);
    set_string(L, "kind", p4_data_kind_name(kind));
}
static void push_id_ref(lua_State *L, P4DataKind kind, uint32_t id)
{
    size_t index;
    if (!p4_data_catalog_index(kind, id, &index)) { push_ref(L, kind, index); return; }
    lua_createtable(L, 0, 2);
    set_string(L, "kind", p4_data_kind_name(kind));
    set_number(L, "id", id);
}
static void set_ref(lua_State *L, const char *field, P4DataKind kind, uint32_t id)
{
    push_id_ref(L, kind, id);
    lua_setfield(L, -2, field);
}
static void set_stats_u8(lua_State *L, const uint8_t *stats)
{
    lua_createtable(L, 0, P4_DATA_STATS);
    for (unsigned i = 0; i < P4_DATA_STATS; ++i) set_number(L, stat_names[i], stats[i]);
    lua_setfield(L, -2, "stats");
}
/* affinities: element -> kind; rates: element -> percent (damage taken, or
 * the ailment chance multiplier). */
static void set_affinities(lua_State *L, const uint16_t *packed)
{
    lua_createtable(L, 0, P4_DATA_AFFINITY_COLUMNS);
    for (uint32_t i = 0; i < P4_DATA_AFFINITY_COLUMNS; ++i)
        set_string(L, p4_data_element_name(i), p4_data_affinity_name(p4_data_affinity_kind(i, packed[i])));
    lua_setfield(L, -2, "affinities");
    lua_createtable(L, 0, P4_DATA_AFFINITY_COLUMNS);
    for (uint32_t i = 0; i < P4_DATA_AFFINITY_COLUMNS; ++i)
        set_number(L, p4_data_element_name(i), p4_data_affinity_rate(packed[i]));
    lua_setfield(L, -2, "rates");
}
/* Every named bit of an ailment mask, in table order. */
static void push_ailments(lua_State *L, uint32_t mask)
{
    lua_createtable(L, 4, 0);
    int n = 0;
    for (size_t i = 0; i < P4_DATA_AILMENTS; ++i) {
        const P4DataAilment *ailment = p4_data_ailment_at(i);
        if (!(mask & ailment->bit)) continue;
        lua_pushstring(L, ailment->name);
        lua_rawseti(L, -2, ++n);
    }
}

static void push_skill(lua_State *L, size_t index, const P4DataSkill *s)
{
    push_head(L, P4_DATA_SKILL, index, 16);
    set_string(L, "kind", p4_data_skill_kind(s));
    set_string(L, "element", s->element == P4_DATA_ELEMENT_NONE ? NULL : p4_data_element_name(s->element));
    lua_pushboolean(L, s->passive); lua_setfield(L, -2, "passive");
    if (s->passive) return;
    set_number(L, "power", s->power);
    if (s->cost_kind == 1 || s->cost_kind == 2) {
        lua_createtable(L, 0, 1);
        set_number(L, s->cost_kind == 2 ? "sp" : "hp_percent", s->cost);
        lua_setfield(L, -2, "cost");
    }
    set_number(L, "accuracy", s->accuracy);
    set_number(L, "critical", s->critical);
    set_string(L, "targets", p4_data_skill_targets(s));
    lua_createtable(L, 0, 2);
    set_number(L, "min", s->hits_min);
    set_number(L, "max", s->hits_max);
    lua_setfield(L, -2, "hits");
    if (s->ailment_mode == 1 && s->ailments) {
        lua_createtable(L, 0, 3);
        push_ailments(L, s->ailments);
        lua_rawgeti(L, -1, 1);
        lua_setfield(L, -3, "status");
        lua_setfield(L, -2, "statuses");
        set_number(L, "chance", s->ailment_chance);
        lua_setfield(L, -2, "ailment");
    } else if (s->ailment_mode == 2 && s->ailments) {
        push_ailments(L, s->ailments);
        lua_setfield(L, -2, "cures");
    }
    lua_createtable(L, 0, 2);
    lua_pushboolean(L, (s->usable & 1u) != 0); lua_setfield(L, -2, "field");
    lua_pushboolean(L, (s->usable & 2u) != 0); lua_setfield(L, -2, "battle");
    lua_setfield(L, -2, "usable");
}
static void push_enemy(lua_State *L, size_t index, const P4DataEnemy *e)
{
    push_head(L, P4_DATA_ENEMY, index, 14);
    set_string(L, "arcana", p4_data_catalog_arcana(e->arcana));
    set_number(L, "level", e->level);
    set_number(L, "hp", e->hp);
    set_number(L, "sp", e->sp);
    set_stats_u8(L, e->stats);
    set_affinities(L, e->affinity);
    lua_createtable(L, P4_DATA_ENEMY_SKILLS, 0);
    int n = 0;
    for (unsigned i = 0; i < P4_DATA_ENEMY_SKILLS; ++i) {
        if (!e->skills[i]) continue;
        push_id_ref(L, P4_DATA_SKILL, e->skills[i]);
        lua_rawseti(L, -2, ++n);
    }
    lua_setfield(L, -2, "skills");
    set_number(L, "exp", e->exp);
    set_number(L, "yen", e->yen);
    /* A battle draws one drop with probability sum(weight)/200, picked by
     * weight; with sums up to 200 each item's chance is weight/2 percent. */
    lua_createtable(L, P4_DATA_ENEMY_DROPS, 0);
    n = 0;
    for (unsigned i = 0; i < P4_DATA_ENEMY_DROPS; ++i) {
        if (!e->drops[i].item) continue;
        lua_createtable(L, 0, 2);
        set_ref(L, "item", P4_DATA_ITEM, e->drops[i].item);
        set_number(L, "chance", e->drops[i].weight / 2.0);
        lua_rawseti(L, -2, ++n);
    }
    lua_setfield(L, -2, "drops");
}
static void push_persona(lua_State *L, size_t index, const P4DataPersona *p)
{
    push_head(L, P4_DATA_PERSONA, index, 10);
    set_string(L, "arcana", p4_data_catalog_arcana(p->arcana));
    set_number(L, "level", p->level);
    set_stats_u8(L, p->stats);
    set_affinities(L, p->affinity);
    lua_createtable(L, (int)p->learn_count, 0);
    for (uint32_t i = 0; i < p->learn_count; ++i) {
        lua_createtable(L, 0, 2);
        set_number(L, "level", p->learnset[i].level);
        set_ref(L, "skill", P4_DATA_SKILL, p->learnset[i].skill);
        lua_rawseti(L, -2, (int)i + 1);
    }
    lua_setfield(L, -2, "learnset");
    if (p->party) set_string(L, "character", p4_data_character_name(p->character));
}
static void push_item(lua_State *L, size_t index, const P4DataItem *it)
{
    push_head(L, P4_DATA_ITEM, index, 12);
    set_string(L, "kind", p4_data_item_kind(it));
    set_number(L, "price", it->price);
    if (it->kind == 0) { set_number(L, "attack", it->attack); set_number(L, "hit", it->hit); }
    if (it->kind == 1) { set_number(L, "defense", it->defense); set_number(L, "evasion", it->evasion); }
    if (it->skill) set_ref(L, "skill", P4_DATA_SKILL, it->skill);
    const char *store = p4_data_item_store(it);
    if (store) {
        lua_createtable(L, 0, 4);
        set_string(L, "store", store);
        lua_pushboolean(L, p4_data_item_sold(it));
        lua_setfield(L, -2, "sold");
        if (it->kind == 3) {
            lua_createtable(L, 0, 2);
            set_number(L, "month", (uint32_t)(uint8_t)it->month);
            set_number(L, "day", (uint32_t)(uint8_t)it->day);
            lua_setfield(L, -2, "from");
        } else {
            /* Real gates only: the Blank gate is sold = false. */
            lua_createtable(L, P4_DATA_ITEM_UNLOCKS, 0);
            int gates = 0;
            for (unsigned i = 0; i < P4_DATA_ITEM_UNLOCKS; ++i) {
                if (!it->unlock[i].count || it->unlock[i].material == P4_DATA_ITEM_NEVER_MATERIAL) continue;
                lua_createtable(L, 0, 2);
                set_ref(L, "material", P4_DATA_ITEM, it->unlock[i].material);
                set_number(L, "count", it->unlock[i].count);
                lua_rawseti(L, -2, ++gates);
            }
            lua_setfield(L, -2, "unlock");
        }
        lua_setfield(L, -2, "shop");
    }
    if (it->kind > 2) return;
    lua_createtable(L, 0, P4_DATA_STATS);
    for (unsigned i = 0; i < P4_DATA_STATS; ++i) set_number(L, stat_names[i], it->stats[i]);
    lua_setfield(L, -2, "stats");
    lua_createtable(L, P4_DATA_CHARACTERS, 0);
    int n = 0;
    for (uint32_t c = 1; c <= P4_DATA_CHARACTERS; ++c) {
        if (!(it->users & (1u << (c - 1)))) continue;
        lua_pushstring(L, p4_data_character_name(c));
        lua_rawseti(L, -2, ++n);
    }
    lua_setfield(L, -2, "users");
}
/* {id, key, enemies}: the filled enemy slots in order. */
static void push_formation(lua_State *L, size_t index, const P4DataFormation *f)
{
    push_head(L, P4_DATA_FORMATION, index, 3);
    lua_createtable(L, P4_DATA_FORMATION_ENEMIES, 0);
    int n = 0;
    for (unsigned i = 0; i < P4_DATA_FORMATION_ENEMIES; ++i) {
        if (!f->enemies[i]) continue;
        push_id_ref(L, P4_DATA_ENEMY, f->enemies[i]);
        lua_rawseti(L, -2, ++n);
    }
    lua_setfield(L, -2, "enemies");
}
static void set_formation(lua_State *L, const char *field, uint32_t id)
{
    size_t index;
    P4DataFormation formation;
    if (p4_data_catalog_index(P4_DATA_FORMATION, id, &index) || p4_data_catalog_formation(index, &formation)) {
        set_ref(L, field, P4_DATA_FORMATION, id);
        return;
    }
    push_formation(L, index, &formation);
    lua_setfield(L, -2, field);
}
/* One day variant: rates and the three slot lists. Every slot is listed (an
 * empty one has weight 0 and no formation), so [n] is the slot to edit. */
static void push_variant(lua_State *L, const P4DataEncounters *g, uint32_t variant)
{
    lua_createtable(L, 0, P4_DATA_ENCOUNTER_CLASSES);
    for (unsigned c = 0; c < P4_DATA_ENCOUNTER_CLASSES; ++c)
        set_number(L, p4_data_encounter_class_name((P4DataEncounterClass)c), g->rates[variant][c]);
    lua_setfield(L, -2, "rates");
    for (unsigned c = 0; c < P4_DATA_ENCOUNTER_CLASSES; ++c) {
        uint32_t first, count;
        p4_data_encounter_class_slots((P4DataEncounterClass)c, &first, &count);
        lua_createtable(L, (int)count, 0);
        for (uint32_t s = 0; s < count; ++s) {
            const P4DataEncounter *slot = &g->slots[variant][first + s];
            lua_createtable(L, 0, 2);
            set_number(L, "weight", slot->weight);
            if (slot->formation) set_formation(L, "formation", slot->formation);
            lua_rawseti(L, -2, (int)s + 1);
        }
        lua_setfield(L, -2, p4_data_encounter_class_name((P4DataEncounterClass)c));
    }
}
static void push_encounters(lua_State *L, size_t index, const P4DataEncounters *g)
{
    push_head(L, P4_DATA_ENCOUNTERS, index, 7);
    push_variant(L, g, 0);
    lua_createtable(L, 0, 4);
    push_variant(L, g, 1);
    lua_setfield(L, -2, "alternate");
}
static void push_treasure(lua_State *L, size_t index, const P4DataTreasure *t)
{
    push_head(L, P4_DATA_TREASURE, index, 3);
    lua_createtable(L, P4_DATA_TREASURE_RECORDS, 0);
    for (uint32_t i = 0; i < P4_DATA_TREASURE_RECORDS; ++i) {
        lua_createtable(L, 0, 3);
        set_number(L, "weight", t->records[i].weight);
        if (t->records[i].item) set_ref(L, "item", P4_DATA_ITEM, t->records[i].item);
        lua_pushboolean(L, t->records[i].locked);
        lua_setfield(L, -2, "locked");
        lua_rawseti(L, -2, (int)i + 1);
    }
    lua_setfield(L, -2, "items");
}
static void push_floor(lua_State *L, size_t index, const P4DataFloor *f)
{
    push_head(L, P4_DATA_FLOOR, index, 8);
    set_number(L, "spawn", f->spawn);
    set_number(L, "refill", f->refill);
    set_number(L, "chests", f->chests);
    size_t at;
    P4DataEncounters group;
    P4DataTreasure treasure;
    if (!p4_data_catalog_index(P4_DATA_ENCOUNTERS, f->encounters, &at) && !p4_data_catalog_encounters(at, &group)) {
        push_encounters(L, at, &group);
        lua_setfield(L, -2, "encounters");
    }
    if (!p4_data_catalog_index(P4_DATA_TREASURE, f->treasure, &at) && !p4_data_catalog_treasure(at, &treasure)) {
        push_treasure(L, at, &treasure);
        lua_setfield(L, -2, "treasure");
    }
}
/* 0 pushed, -1 the row is outside the loaded banks. */
int p4_lua_data_push_row(lua_State *L, P4DataKind kind, size_t index)
{
    P4DataSkill skill;
    P4DataEnemy enemy;
    P4DataPersona persona;
    P4DataItem item;
    P4DataFloor floor;
    P4DataFormation formation;
    P4DataEncounters group;
    P4DataTreasure treasure;
    switch (kind) {
    case P4_DATA_SKILL: if (p4_data_catalog_skill(index, &skill)) return -1; push_skill(L, index, &skill); return 0;
    case P4_DATA_ENEMY: if (p4_data_catalog_enemy(index, &enemy)) return -1; push_enemy(L, index, &enemy); return 0;
    case P4_DATA_PERSONA: if (p4_data_catalog_persona(index, &persona)) return -1; push_persona(L, index, &persona); return 0;
    case P4_DATA_ITEM: if (p4_data_catalog_item(index, &item)) return -1; push_item(L, index, &item); return 0;
    case P4_DATA_FLOOR: if (p4_data_catalog_floor(index, &floor)) return -1; push_floor(L, index, &floor); return 0;
    case P4_DATA_FORMATION:
        if (p4_data_catalog_formation(index, &formation)) return -1;
        push_formation(L, index, &formation);
        return 0;
    case P4_DATA_ENCOUNTERS:
        if (p4_data_catalog_encounters(index, &group)) return -1;
        push_encounters(L, index, &group);
        return 0;
    case P4_DATA_TREASURE:
        if (p4_data_catalog_treasure(index, &treasure)) return -1;
        push_treasure(L, index, &treasure);
        return 0;
    default: return -1;
    }
}

static int lookup(lua_State *L, P4DataKind kind)
{
    const char *function = p4_data_kind_name(kind);
    ready(L, function);
    P4DataRows rows;
    size_t index = 0;
    if (p4_data_catalog_rows(kind, &rows)) luaL_error(L, "p4.data.%s: the catalog is unavailable", function);
    if (lua_type(L, 1) == LUA_TNUMBER) {
        const double value = lua_tonumber(L, 1);
        if (!(value >= 0.0 && value <= 4294967295.0) || value != (double)(uint32_t)value ||
            p4_data_catalog_index(kind, (uint32_t)value, &index))
            luaL_error(L, "p4.data.%s: no %s has the ID %g", function, function, value);
    } else {
        char message[768];
        const char *query = luaL_checkstring(L, 1);
        if (p4_data_lookup(&rows, query, &index, message, sizeof message))
            luaL_error(L, "p4.data.%s: %s", function, message);
    }
    if (p4_lua_data_push_row(L, kind, index)) luaL_error(L, "p4.data.%s: that row is outside the loaded tables", function);
    return 1;
}
static int data_skill(lua_State *L) { return lookup(L, P4_DATA_SKILL); }
static int data_persona(lua_State *L) { return lookup(L, P4_DATA_PERSONA); }
static int data_enemy(lua_State *L) { return lookup(L, P4_DATA_ENEMY); }
static int data_item(lua_State *L) { return lookup(L, P4_DATA_ITEM); }
static int data_floor(lua_State *L) { return lookup(L, P4_DATA_FLOOR); }
static int data_formation(lua_State *L) { return lookup(L, P4_DATA_FORMATION); }
static int data_encounters(lua_State *L) { return lookup(L, P4_DATA_ENCOUNTERS); }
static int data_treasure(lua_State *L) { return lookup(L, P4_DATA_TREASURE); }

static int data_original_enemy_level(lua_State *L)
{
    double value = luaL_checknumber(L, 1);
    if (!(value >= 0.0 && value < P4_ENEMY_RETAIL_ROWS) || value != (double)(uint32_t)value)
        luaL_error(L, "p4.data.original_enemy_level: no original enemy has ID %g", value);
    uint8_t level;
    const char *source = NULL, *sha256 = NULL, *reason = NULL;
    if (p4_data_original_enemy_level((uint32_t)value, &level, &source, &sha256, &reason))
        luaL_error(L, "p4.data.original_enemy_level: %s", reason);
    lua_createtable(L, 0, 4);
    set_number(L, "id", value);
    set_number(L, "level", level);
    set_string(L, "source", source);
    set_string(L, "sha256", sha256);
    return 1;
}

/* Filter fields: name (part of the name), kind, element, arcana, level
 * (a number or {min, max}). */
typedef struct Filter {
    char name[64];
    const char *kind;
    uint32_t element, arcana;
    double level_min, level_max;
    int has_element, has_arcana, has_level;
} Filter;
static void read_filter(lua_State *L, int index, P4DataKind kind, Filter *f)
{
    memset(f, 0, sizeof *f);
    f->level_min = 0; f->level_max = 1e9;
    if (lua_isnoneornil(L, index)) return;
    if (lua_type(L, index) == LUA_TSTRING) { p4_data_match_form(lua_tostring(L, index), f->name, sizeof f->name); return; }
    luaL_checktype(L, index, LUA_TTABLE);
    lua_getfield(L, index, "name");
    if (!lua_isnil(L, -1)) p4_data_match_form(luaL_checkstring(L, -1), f->name, sizeof f->name);
    lua_pop(L, 1);
    lua_getfield(L, index, "kind");
    if (!lua_isnil(L, -1)) f->kind = luaL_checkstring(L, -1);
    lua_pop(L, 1); /* The string stays reachable through the filter table. */
    lua_getfield(L, index, "element");
    if (!lua_isnil(L, -1)) {
        const char *element = luaL_checkstring(L, -1);
        if (p4_data_element_parse(element, &f->element)) luaL_error(L, "p4.data.find: unknown element \"%s\"", element);
        f->has_element = 1;
    }
    lua_pop(L, 1);
    lua_getfield(L, index, "arcana");
    if (!lua_isnil(L, -1)) {
        const char *arcana = luaL_checkstring(L, -1);
        if (p4_data_catalog_arcana_parse(arcana, &f->arcana)) luaL_error(L, "p4.data.find: unknown arcana \"%s\"", arcana);
        f->has_arcana = 1;
    }
    lua_pop(L, 1);
    lua_getfield(L, index, "level");
    if (lua_type(L, -1) == LUA_TNUMBER) { f->level_min = f->level_max = lua_tonumber(L, -1); f->has_level = 1; }
    else if (lua_type(L, -1) == LUA_TTABLE) {
        lua_getfield(L, -1, "min"); if (!lua_isnil(L, -1)) f->level_min = luaL_checknumber(L, -1); lua_pop(L, 1);
        lua_getfield(L, -1, "max"); if (!lua_isnil(L, -1)) f->level_max = luaL_checknumber(L, -1); lua_pop(L, 1);
        f->has_level = 1;
    } else if (!lua_isnil(L, -1)) luaL_error(L, "p4.data.find: level is a number or {min = ..., max = ...}");
    lua_pop(L, 1);
    /* A field that never applies to this kind is a mistake, not an empty result. */
    const int ranked = kind == P4_DATA_PERSONA || kind == P4_DATA_ENEMY;
    const char *wrong = f->has_element && kind != P4_DATA_SKILL ? "element applies to skills" :
        f->kind && ranked ? "kind applies to skills and items" :
        (f->has_arcana || f->has_level) && !ranked ? "arcana and level apply to Personas and enemies" : NULL;
    if (wrong) luaL_error(L, "p4.data.find: %s", wrong);
}
static int level_ok(const Filter *f, unsigned level) { return !f->has_level || (level >= f->level_min && level <= f->level_max); }
static int matches(const Filter *f, P4DataKind kind, size_t index, const char *name)
{
    char form[64];
    if (f->name[0] && (!p4_data_match_form(name, form, sizeof form) || !strstr(form, f->name))) return 0;
    if (!f->kind && !f->has_element && !f->has_arcana && !f->has_level) return 1;
    P4DataSkill skill;
    P4DataEnemy enemy;
    P4DataPersona persona;
    P4DataItem item;
    switch (kind) {
    case P4_DATA_SKILL:
        if (p4_data_catalog_skill(index, &skill)) return 0;
        return (!f->kind || !strcmp(f->kind, p4_data_skill_kind(&skill))) && (!f->has_element || skill.element == f->element) &&
            !f->has_arcana && !f->has_level;
    case P4_DATA_ENEMY:
        if (p4_data_catalog_enemy(index, &enemy)) return 0;
        return !f->kind && !f->has_element && (!f->has_arcana || enemy.arcana == f->arcana) && level_ok(f, enemy.level);
    case P4_DATA_PERSONA:
        if (p4_data_catalog_persona(index, &persona)) return 0;
        return !f->kind && !f->has_element && (!f->has_arcana || persona.arcana == f->arcana) && level_ok(f, persona.level);
    case P4_DATA_ITEM:
        if (p4_data_catalog_item(index, &item)) return 0;
        return (!f->kind || !strcmp(f->kind, p4_data_item_kind(&item))) && !f->has_element && !f->has_arcana && !f->has_level;
    default: return 0;
    }
}
static int list(lua_State *L, const char *function, int filtered)
{
    const P4DataKind kind = kind_argument(L, 1, function);
    ready(L, function);
    Filter filter;
    if (filtered) read_filter(L, 2, kind, &filter);
    else memset(&filter, 0, sizeof filter);
    P4DataRows rows;
    if (p4_data_catalog_rows(kind, &rows)) luaL_error(L, "p4.data.%s: the catalog is unavailable", function);
    lua_newtable(L);
    int n = 0;
    char name[32];
    for (size_t index = 0; index < rows.count; ++index) {
        /* Named kinds list their named rows; unnamed kinds list every row. */
        const int named = p4_data_catalog_name(kind, index, name, sizeof name) != NULL;
        if (!named) name[0] = 0;
        if ((!named && kind < P4_DATA_FORMATION) || !p4_data_catalog_key(kind, index)) continue;
        if (filtered && !matches(&filter, kind, index, name)) continue;
        push_ref(L, kind, index);
        lua_rawseti(L, -2, ++n);
    }
    return 1;
}
static int data_find(lua_State *L) { return list(L, "find", 1); }
static int data_all(lua_State *L) { return list(L, "all", 0); }

/* main.luau runs after the tables are built; edits belong in data.luau. */
static int data_edit(lua_State *L)
{
    luaL_error(L, "p4.data.edit: edits shape the tables while the game builds them, before main.luau runs; "
        "move this call to data.luau (a file next to main.luau)");
    return 0;
}
/* Every field two or more mods' data layers assigned: {kind, id, key, field,
 * owners (load order), winner (the last)}. */
static int data_conflicts(lua_State *L)
{
    const size_t count = p4_data_edits_conflict_count();
    lua_createtable(L, (int)count, 0);
    for (size_t i = 0; i < count; ++i) {
        P4DataConflict conflict;
        size_t index;
        if (p4_data_edits_conflict_at(i, &conflict)) break;
        lua_createtable(L, 0, 6);
        set_string(L, "kind", p4_data_kind_name(conflict.kind));
        set_number(L, "id", conflict.id);
        if (!p4_data_catalog_ready(NULL) && !p4_data_catalog_index(conflict.kind, conflict.id, &index))
            set_string(L, "key", p4_data_catalog_key(conflict.kind, index));
        set_string(L, "field", conflict.field);
        lua_createtable(L, (int)conflict.owner_count, 0);
        for (size_t o = 0; o < conflict.owner_count; ++o) {
            lua_pushstring(L, conflict.owners[o]);
            lua_rawseti(L, -2, (int)o + 1);
        }
        lua_setfield(L, -2, "owners");
        set_string(L, "winner", conflict.owners[conflict.owner_count - 1]);
        lua_rawseti(L, -2, (int)i + 1);
    }
    return 1;
}
static const luaL_Reg read_methods[] = {
    {"skill", data_skill}, {"persona", data_persona}, {"enemy", data_enemy}, {"item", data_item},
    {"floor", data_floor}, {"formation", data_formation}, {"encounters", data_encounters}, {"treasure", data_treasure},
    {"find", data_find}, {"all", data_all}, {"original_enemy_level", data_original_enemy_level}, {NULL, NULL}
};
const luaL_Reg *p4_lua_data_read_methods(void) { return read_methods; }

/* p4.data.session.edit(fn, { persist = boolean }): fn(d) writes the loaded
 * tables for this run through the same view as data.luau; a broken rule or
 * an error undoes the whole call. A save load or a new game undoes every
 * session edit, then re-applies the persisted ones the save carries. */
#define SESSION_VIEW "p4.data.session.view"
static int session_edit(lua_State *L)
{
    p4_lua_require_mutation(L);
    luaL_checktype(L, 1, LUA_TFUNCTION);
    int persist = 0;
    if (!lua_isnoneornil(L, 2)) {
        luaL_checktype(L, 2, LUA_TTABLE);
        lua_getfield(L, 2, "persist");
        persist = lua_toboolean(L, -1);
        lua_pop(L, 1);
    }
    if (persist && p4_data_session_save_register())
        luaL_error(L, "p4.data.session.edit: persist needs the save block, which this run could not register");
    P4LuaMod *mod = p4_luau_user(L);
    lua_getfield(L, LUA_REGISTRYINDEX, SESSION_VIEW);
    if (lua_isnil(L, -1)) {
        /* The data.luau view, built once per VM on its idle control state. */
        lua_pop(L, 1);
        size_t size;
        const char *source = p4_lua_data_edit_prelude(&size);
        if (p4_luau_load_source(mod->vm, "@p4/data-session", source, size) != LUA_OK)
            luaL_error(L, "p4.data.session.edit: the edit view failed to load: %s", p4_luau_error(mod->vm));
        lua_xmove(mod->state, L, 1);
        p4_lua_data_edit_push_primitives(L);
        lua_call(L, 4, 1);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, SESSION_VIEW);
    }
    char error[768];
    if (p4_data_session_begin(mod->info->id, persist, error, sizeof error)) luaL_error(L, "%s", error);
    if (lua_pcall(L, 0, 1, 0) != LUA_OK) { p4_data_session_end(0, NULL, 0); lua_error(L); }
    lua_pushvalue(L, 1);
    lua_insert(L, -2); /* fn, d */
    if (lua_pcall(L, 1, 0, 0) != LUA_OK) { p4_data_session_end(0, NULL, 0); lua_error(L); }
    if (p4_data_session_end(1, error, sizeof error)) luaL_error(L, "p4.data.session.edit: %s", error);
    return 0;
}
void p4_lua_data_initialize(lua_State *L)
{
    static const luaL_Reg main_methods[] = {{"edit", data_edit}, {"conflicts", data_conflicts}, {NULL, NULL}};
    static const luaL_Reg session_methods[] = {{"edit", session_edit}, {NULL, NULL}};
    /* Session edits persist through a native save block that must exist
     * before any save loads; boot (with the catalog) is that moment. */
    if (!p4_data_catalog_ready(NULL)) (void)p4_data_session_save_register();
    lua_newtable(L);
    luaL_register(L, NULL, read_methods);
    luaL_register(L, NULL, main_methods);
    lua_newtable(L);
    luaL_register(L, NULL, session_methods);
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "session");
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "data");
}
