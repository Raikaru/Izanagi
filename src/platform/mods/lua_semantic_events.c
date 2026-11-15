/* p4.events.on for the semantic battle events (battle_semantic.h): turn_start,
 * skill_used, damage, miss, ailment, knockdown, ko and revive. The game
 * bindings wrap the core p4.events.on: these names subscribe here, every
 * other name goes to the core events unchanged. Records are delivered at the
 * next frame safe point in commit order, each to the subscribers of its kind
 * in load order, as fresh read-only tables; callbacks run in semantic mode
 * like frame_start. Refs carry the catalog's key and current display name
 * (game_data_catalog.h) when the catalog is ready. */
#include "native_bindings.h"
#include "p4pc/battle_semantic.h"
#include "p4pc/game_data_catalog.h"
#include "p4pc/mod_events.h"
#include "p4pc/mod_failure.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUBSCRIPTION_META "p4.events.semantic"

typedef struct LuaSemantic {
    P4LuaMod *mod;
    int kind, enabled, removed;
    int callback, self;
    struct LuaSemantic *next;
} LuaSemantic;
static LuaSemantic *subscriptions;
static unsigned delivering;
static int installed;

static const char *const results[] = {"hit", "weak", "critical", "null", "drain"};
static const char *const causes[] = {"down", "weakness", "critical"};

static void push_ref(lua_State *L, P4DataKind kind, uint32_t id)
{
    P4DataRef ref;
    const int found = !p4_data_ref(kind, id, &ref);
    lua_createtable(L, 0, 3);
    lua_pushinteger(L, (int)id); lua_setfield(L, -2, "id");
    if (found) {
        lua_pushstring(L, ref.key); lua_setfield(L, -2, "key");
        if (ref.name[0]) { lua_pushstring(L, ref.name); lua_setfield(L, -2, "name"); }
    }
    lua_setreadonly(L, -1, 1);
}
static void push_unit(lua_State *L, const P4SemanticUnit *unit)
{
    lua_createtable(L, 0, 5);
    lua_pushstring(L, unit->side ? "enemy" : "party"); lua_setfield(L, -2, "side");
    lua_pushinteger(L, unit->id); lua_setfield(L, -2, "id");
    lua_pushunsigned(L, unit->uid); lua_setfield(L, -2, "uid");
    if (!unit->side) {
        const char *character = p4_data_character_name(unit->id);
        if (character) { lua_pushstring(L, character); lua_setfield(L, -2, "character"); }
    } else {
        P4DataRef ref;
        if (!p4_data_ref(P4_DATA_ENEMY, unit->id, &ref)) {
            lua_pushstring(L, ref.key); lua_setfield(L, -2, "key");
            if (ref.name[0]) { lua_pushstring(L, ref.name); lua_setfield(L, -2, "name"); }
        }
    }
    lua_setreadonly(L, -1, 1);
}
static void push_event(lua_State *L, const P4SemanticEvent *event)
{
    lua_createtable(L, 0, 12);
    lua_pushstring(L, p4_semantic_name(event->kind)); lua_setfield(L, -2, "name");
    lua_pushunsigned(L, event->sequence); lua_setfield(L, -2, "sequence");
    switch (event->kind) {
    case P4_SEMANTIC_TURN_START:
        push_unit(L, &event->source); lua_setfield(L, -2, "unit");
        lua_pushboolean(L, event->flag); lua_setfield(L, -2, "follow_up");
        break;
    case P4_SEMANTIC_ONE_MORE:
        push_unit(L, &event->source); lua_setfield(L, -2, "unit");
        lua_pushstring(L, causes[event->result]); lua_setfield(L, -2, "cause");
        break;
    case P4_SEMANTIC_REWARD:
        lua_pushinteger(L, event->exp); lua_setfield(L, -2, "exp");
        lua_pushinteger(L, event->amount); lua_setfield(L, -2, "money");
        lua_createtable(L, event->item_count, 0);
        for (int i = 0; i < event->item_count; ++i) {
            lua_createtable(L, 0, 2);
            push_ref(L, P4_DATA_ITEM, event->items[i]); lua_setfield(L, -2, "item");
            lua_pushinteger(L, event->item_counts[i]); lua_setfield(L, -2, "count");
            lua_setreadonly(L, -1, 1);
            lua_rawseti(L, -2, i + 1);
        }
        lua_setreadonly(L, -1, 1);
        lua_setfield(L, -2, "items");
        lua_createtable(L, event->party_count, 0);
        for (int i = 0; i < event->party_count; ++i) {
            lua_createtable(L, 0, 2);
            const char *character = p4_data_character_name(event->party[i]);
            if (character) { lua_pushstring(L, character); lua_setfield(L, -2, "character"); }
            lua_pushinteger(L, event->party_exp[i]); lua_setfield(L, -2, "exp");
            lua_setreadonly(L, -1, 1);
            lua_rawseti(L, -2, i + 1);
        }
        lua_setreadonly(L, -1, 1);
        lua_setfield(L, -2, "party");
        break;
    case P4_SEMANTIC_LEVEL_UP:
    case P4_SEMANTIC_SKILL_LEARNED: {
        const char *character = p4_data_character_name(event->target.id);
        if (character) { lua_pushstring(L, character); lua_setfield(L, -2, "character"); }
        if (event->persona) { push_ref(L, P4_DATA_PERSONA, event->persona); lua_setfield(L, -2, "persona"); }
        if (event->kind == P4_SEMANTIC_LEVEL_UP) {
            lua_pushinteger(L, event->level); lua_setfield(L, -2, "level");
            lua_pushinteger(L, event->from); lua_setfield(L, -2, "from");
        } else {
            push_ref(L, P4_DATA_SKILL, event->skill); lua_setfield(L, -2, "skill");
        }
        break;
    }
    case P4_SEMANTIC_SKILL_USED:
        push_unit(L, &event->source); lua_setfield(L, -2, "unit");
        lua_pushstring(L, event->action_kind == 1 ? "attack" : event->action_kind == 3 ? "item" : "skill");
        lua_setfield(L, -2, "kind");
        push_ref(L, P4_DATA_SKILL, event->skill); lua_setfield(L, -2, "skill");
        if (event->action_kind == 3) { push_ref(L, P4_DATA_ITEM, event->item); lua_setfield(L, -2, "item"); }
        break;
    default:
        push_unit(L, &event->source); lua_setfield(L, -2, "source");
        push_unit(L, &event->target); lua_setfield(L, -2, event->kind == P4_SEMANTIC_DAMAGE || event->kind == P4_SEMANTIC_MISS ? "target" : "unit");
        if (event->skill) { push_ref(L, P4_DATA_SKILL, event->skill); lua_setfield(L, -2, "skill"); }
        break;
    }
    if (event->kind == P4_SEMANTIC_DAMAGE) {
        lua_pushinteger(L, event->amount); lua_setfield(L, -2, "amount");
        lua_pushinteger(L, event->hp_after); lua_setfield(L, -2, "hp");
        lua_pushstring(L, results[event->result]); lua_setfield(L, -2, "result");
        lua_pushboolean(L, event->flag); lua_setfield(L, -2, "reflected");
        lua_pushboolean(L, event->endured); lua_setfield(L, -2, "endured");
    } else if (event->kind == P4_SEMANTIC_AILMENT) {
        lua_pushunsigned(L, event->status); lua_setfield(L, -2, "status");
        const char *ailment = p4_data_ailment_name(event->status);
        if (ailment) { lua_pushstring(L, ailment); lua_setfield(L, -2, "ailment"); }
        lua_pushstring(L, event->flag ? "applied" : "removed"); lua_setfield(L, -2, "change");
    }
    lua_setreadonly(L, -1, 1);
}

typedef struct Delivery { LuaSemantic *handler; const P4SemanticEvent *event; } Delivery;
static int invoke_semantic(lua_State *L)
{
    Delivery *delivery = lua_touserdata(L, 1);
    lua_getref(L, delivery->handler->callback);
    push_event(L, delivery->event);
    lua_call(L, 1, 0);
    return 0;
}
static void reap(void)
{
    for (LuaSemantic **link = &subscriptions; *link;) {
        LuaSemantic *s = *link;
        if (s->removed && !s->mod) { *link = s->next; free(s); }
        else link = &s->next;
    }
}
static int pump(void)
{
    P4SemanticEvent events[64];
    uint32_t lost = 0;
    const size_t count = p4_semantic_drain(events, sizeof events / sizeof *events, &lost);
    if (lost) fprintf(stderr, "p4pc: mods: %u semantic battle events were dropped (queue full)\n", (unsigned)lost);
    ++delivering;
    for (size_t i = 0; i < count; ++i) {
        for (LuaSemantic *s = subscriptions; s; s = s->next) {
            if (s->removed || !s->enabled || s->kind != events[i].kind) continue;
            Delivery delivery = {s, &events[i]};
            if (p4_lua_invoke(s->mod, invoke_semantic, &delivery, 0, P4_LUA_SEMANTIC)) {
                --delivering;
                p4_mod_failure(s->mod->owner, p4_lua_error(s->mod));
            }
        }
    }
    --delivering;
    reap();
    /* More than one batch: stay scheduled for the next boundary. */
    return count == sizeof events / sizeof *events;
}

static LuaSemantic *check_subscription(lua_State *L)
{
    LuaSemantic *s = *(LuaSemantic **)luaL_checkudata(L, 1, SUBSCRIPTION_META);
    if (!s || s->mod != p4_luau_user(L)) luaL_error(L, "foreign or removed event subscription");
    return s;
}
static int enable_semantic(lua_State *L)
{
    p4_lua_require_mutation(L);
    LuaSemantic *s = check_subscription(L);
    const int enabled = lua_toboolean(L, lua_upvalueindex(1));
    if (s->removed) luaL_error(L, "cannot enable a removed event subscription");
    if (enabled != s->enabled && p4_semantic_interest(s->kind, enabled ? 1 : -1))
        luaL_error(L, "battle event observer unavailable");
    s->enabled = enabled;
    return 0;
}
static int remove_semantic(lua_State *L)
{
    p4_lua_require_mutation(L);
    LuaSemantic **slot = luaL_checkudata(L, 1, SUBSCRIPTION_META);
    LuaSemantic *s = *slot;
    if (!s) return 0;
    if (s->mod != p4_luau_user(L)) luaL_error(L, "foreign event subscription");
    if (s->enabled) p4_semantic_interest(s->kind, -1);
    s->enabled = 0;
    s->removed = 1;
    lua_unref(L, s->callback);
    lua_unref(L, s->self);
    s->callback = s->self = LUA_NOREF;
    s->mod = NULL; /* freed outside delivery */
    *slot = NULL;
    if (!delivering) reap();
    return 0;
}
typedef struct SemanticRefs {
    int callback, self;
} SemanticRefs;

/* All allocating VM operations run before native registration, under a
 * protected call. The userdata cannot expose a native record until commit. */
static int prepare_semantic(lua_State *L)
{
    SemanticRefs *refs = lua_touserdata(L, 1);
    LuaSemantic **slot = lua_newuserdata(L, sizeof *slot);
    *slot = NULL;
    luaL_getmetatable(L, SUBSCRIPTION_META);
    lua_setmetatable(L, -2);
    refs->callback = lua_ref(L, 2);
    refs->self = lua_ref(L, -1);
    return 1;
}

static void release_semantic_refs(lua_State *L, const SemanticRefs *refs)
{
    lua_unref(L, refs->callback);
    lua_unref(L, refs->self);
}

static int subscribe(lua_State *L)
{
    const int kind = p4_semantic_kind(luaL_checkstring(L, 1));
    if (kind < 0) {
        /* Not a semantic battle event: the core p4.events.on. */
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_insert(L, 1);
        lua_call(L, lua_gettop(L) - 1, 1);
        return 1;
    }
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    if (mod->subscriptions >= 128) luaL_error(L, "mod lifetime subscription limit (128) exceeded");
    SemanticRefs refs = {LUA_NOREF, LUA_NOREF};
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_pushlightuserdata(L, &refs);
    lua_pushvalue(L, 2);
    if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
        release_semantic_refs(L, &refs);
        lua_error(L);
    }
    LuaSemantic **slot = lua_touserdata(L, -1);
    LuaSemantic *s = calloc(1, sizeof *s);
    if (!s) {
        release_semantic_refs(L, &refs);
        luaL_error(L, "cannot allocate an event subscription");
    }
    if (p4_semantic_interest(kind, 1)) {
        free(s);
        release_semantic_refs(L, &refs);
        luaL_error(L, "battle event observer unavailable");
    }
    *s = (LuaSemantic){.mod = mod, .kind = kind, .enabled = 1, .callback = refs.callback, .self = refs.self};
    *slot = s;
    LuaSemantic **link = &subscriptions;
    while (*link && (!(*link)->mod || (*link)->mod->owner <= mod->owner)) link = &(*link)->next;
    s->next = *link;
    *link = s;
    ++mod->subscriptions;
    return 1;
}

void p4_lua_semantic_events_initialize(lua_State *L)
{
    if (!installed) { p4_mod_events_set_semantic(pump); installed = 1; }
    luaL_newmetatable(L, SUBSCRIPTION_META);
    lua_newtable(L);
    lua_pushboolean(L, 1); lua_pushcclosure(L, enable_semantic, "p4.event.enable", 1); lua_setfield(L, -2, "enable");
    lua_pushboolean(L, 0); lua_pushcclosure(L, enable_semantic, "p4.event.disable", 1); lua_setfield(L, -2, "disable");
    lua_pushcfunction(L, remove_semantic, "p4.event.remove"); lua_setfield(L, -2, "remove");
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "__index");
    lua_pushliteral(L, "p4 event subscription"); lua_setfield(L, -2, "__metatable");
    lua_setreadonly(L, -1, 1);
    lua_pop(L, 1);
    /* p4.events becomes one read-only table: the core fields, with on routing
     * the semantic names here and the rest to the core subscribe. */
    lua_newtable(L);
    lua_getfield(L, -2, "events");
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_settable(L, -5);
    }
    lua_getfield(L, -1, "on");
    lua_pushcfunction(L, prepare_semantic, "p4.event.prepare");
    lua_pushcclosure(L, subscribe, "p4.events.on", 2);
    lua_setfield(L, -3, "on");
    lua_pop(L, 1);
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "events");
}
void p4_lua_semantic_events_close(P4LuaMod *mod)
{
    for (LuaSemantic *s = subscriptions; s; s = s->next) {
        if (s->mod != mod) continue;
        if (s->enabled) p4_semantic_interest(s->kind, -1);
        s->enabled = 0;
        s->removed = 1;
        s->mod = NULL;
    }
    if (!delivering) reap();
}
