/* p4.battle.announce (capability battle.announce@1) and p4.battle.say
 * (battle.say@1): a banner in the battle's skill-name box, or a
 * "<speaker>: <line>" message window, at a unit's turn start or turn end
 * (battle_present.c). Allowed from events and other ordinary callbacks; refused
 * in snapshot callbacks (p4.battle.decide, authorizers, save snapshots), which
 * only read: a queued line there would announce a choice before the stage
 * applies or refuses it. */
#include "native_bindings.h"
#include "p4pc/battle_present.h"
#include <string.h>

static const char *text_field(lua_State *L, const char *api, const char *field, int limit, int required)
{
    lua_getfield(L, 1, field);
    if (lua_isnil(L, -1) && !required) return NULL;
    if (lua_type(L, -1) != LUA_TSTRING) luaL_error(L, "%s: %s must be a string", api, field);
    size_t length = 0;
    const char *text = lua_tolstring(L, -1, &length);
    if (!length || (int)length > limit || strlen(text) != length)
        luaL_error(L, "%s: %s must be 1-%d characters on one line (got %d)", api, field, limit, (int)length);
    return text;
}
static void refuse_in_snapshot(lua_State *L, const char *api)
{
    if (p4_lua_frame(L)->snapshot)
        luaL_error(L, "%s is refused in snapshot callbacks (p4.battle.decide, authorizers, save snapshots); "
                      "present from an event such as turn_start instead", api);
}

/* The shared spec: { unit, text, at?, frames? } plus say's speaker. */
static int present(lua_State *L, int kind)
{
    const char *api = kind == P4_PRESENT_SAY ? "p4.battle.say" : "p4.battle.announce";
    P4LuaMod *mod = p4_luau_user(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    refuse_in_snapshot(L, api);
    P4PresentRequest request = {.kind = kind, .moment = P4_PRESENT_TURN_END};
    lua_getfield(L, 1, "unit");
    if (!lua_istable(L, -1))
        luaL_error(L, "%s: unit must be a battle unit with a uid (an event's e.unit)", api);
    lua_getfield(L, -1, "uid");
    if (lua_type(L, -1) != LUA_TNUMBER)
        luaL_error(L, "%s: unit has no uid; pass the unit table an event gave you", api);
    request.uid = (uint32_t)p4_lua_integer(L, -1, 0, UINT32_MAX);
    request.text = text_field(L, api, "text", P4_PRESENT_TEXT_MAX, 1);
    if (kind == P4_PRESENT_SAY) request.speaker = text_field(L, api, "speaker", P4_PRESENT_SPEAKER_MAX, 0);
    lua_getfield(L, 1, "at");
    if (!lua_isnil(L, -1)) {
        const char *at = luaL_checkstring(L, -1);
        if (!strcmp(at, "turn_start")) request.moment = P4_PRESENT_TURN_START;
        else if (strcmp(at, "turn_end"))
            luaL_error(L, "%s: at must be \"turn_start\" or \"turn_end\" (got \"%s\")", api, at);
    }
    lua_getfield(L, 1, "frames");
    request.frames = !lua_isnil(L, -1) ? (int)p4_lua_integer(L, -1, P4_PRESENT_FRAMES_MIN, P4_PRESENT_FRAMES_MAX)
        : kind == P4_PRESENT_SAY ? 0 : 60;
    if (kind == P4_PRESENT_BANNER) {
        lua_getfield(L, 1, "hold");
        if (!lua_isnil(L, -1) && !lua_isboolean(L, -1)) luaL_error(L, "%s: hold must be a boolean", api);
        request.hold = lua_toboolean(L, -1);
    }
    const int status = p4_battle_present_queue((uint32_t)mod->owner, mod->info->id, &request);
    if (status == P4_PRESENT_BAD_TEXT)
        luaL_error(L, "%s: \"%s\"%s has a character the game cannot draw; use letters, digits and punctuation", api,
                   request.text, request.speaker ? " or its speaker" : "");
    lua_pushboolean(L, status == P4_PRESENT_OK);
    if (status == P4_PRESENT_OK) return 1;
    lua_pushstring(L, p4_battle_present_reason(status));
    return 2;
}
static int announce(lua_State *L) { return present(L, P4_PRESENT_BANNER); }
/* p4.battle.announce_clear(unit?) -> ended: this mod's held banners for that unit (all, without one). */
static int announce_clear(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    uint32_t uid = 0;
    refuse_in_snapshot(L, "p4.battle.announce_clear");
    if (!lua_isnoneornil(L, 1)) {
        luaL_checktype(L, 1, LUA_TTABLE);
        lua_getfield(L, 1, "uid");
        if (lua_type(L, -1) != LUA_TNUMBER) luaL_error(L, "p4.battle.announce_clear: unit has no uid");
        uid = (uint32_t)p4_lua_integer(L, -1, 1, UINT32_MAX);
    }
    lua_pushinteger(L, p4_battle_present_clear((uint32_t)mod->owner, uid));
    return 1;
}
static int say(lua_State *L) { return present(L, P4_PRESENT_SAY); }

void p4_lua_battle_present_initialize(lua_State *L)
{
    /* p4.battle may already hold other battle services (formation, decide): the
     * namespace becomes one read-only table with theirs and these. */
    lua_newtable(L);
    lua_getfield(L, -2, "battle");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            lua_pushvalue(L, -2);
            lua_insert(L, -2);
            lua_settable(L, -5);
        }
    }
    lua_pop(L, 1);
    lua_pushcfunction(L, announce, "p4.battle.announce");
    lua_setfield(L, -2, "announce");
    lua_pushcfunction(L, announce_clear, "p4.battle.announce_clear");
    lua_setfield(L, -2, "announce_clear");
    lua_pushcfunction(L, say, "p4.battle.say");
    lua_setfield(L, -2, "say");
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "battle");
}
void p4_lua_battle_present_close(P4LuaMod *mod)
{
    p4_battle_present_remove_owner((uint32_t)mod->owner);
}
