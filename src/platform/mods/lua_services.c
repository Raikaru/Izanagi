/* p4.clock, p4.scene, p4.encounter, p4.camera and p4.hud (API 1.4): thin
 * bindings over include/p4pc/mod_services.h. Queries work in any callback.
 * Requests, leases and hides change the game or its presentation: they are
 * refused in synchronous UI callbacks and save snapshots and record semantic
 * use of the mod. Work happens later at the frame boundary. */
#include "native_bindings.h"
#include "p4pc/mod_services.h"
#include "p4pc/gmem.h"
#include <math.h>
#include <string.h>

typedef struct LuaLease {
    P4LuaMod *mod;
    uint32_t generation;
} LuaLease;

static P4LuaMod *mutation(lua_State *L, const char *what)
{
    p4_lua_require_mutation(L);
    if (p4_lua_frame(L)->no_native_calls)
        luaL_error(L, "%s is not allowed in a synchronous UI callback; stage it and run it from an event", what);
    P4LuaMod *mod = p4_luau_user(L);
    p4_lua_semantic_use(mod);
    return mod;
}

static int refused(lua_State *L, const char *reason)
{
    lua_pushboolean(L, 0);
    lua_pushstring(L, reason ? reason : "refused");
    return 2;
}

static uint32_t field_u32(lua_State *L, int table, const char *name, int required, uint32_t fallback, uint32_t high)
{
    lua_rawgetfield(L, table, name);
    if (lua_isnil(L, -1)) {
        if (required) luaL_error(L, "missing field %s", name);
        lua_pop(L, 1);
        return fallback;
    }
    uint32_t value = (uint32_t)p4_lua_integer(L, -1, 0, high);
    lua_pop(L, 1);
    return value;
}

/* ---- p4.clock */

static int clock_wall(lua_State *L) { lua_pushnumber(L, (double)p4_mod_clock_wall_ns() / 1e9); return 1; }
static int clock_updates(lua_State *L)
{
    uint32_t updates;
    if (p4_mod_clock_updates(&updates)) luaL_error(L, "p4.clock.updates: the kwlnRunFrame seam is unavailable");
    lua_pushunsigned(L, updates);
    return 1;
}
static int clock_retail(lua_State *L) { lua_pushunsigned(L, P4_G32(0x00764290u)); return 1; }

/* ---- p4.scene */

static const char *const contexts[] = {"none", "title", "field", "dungeon", "battle", "event", "other"};

static int scene_state(lua_State *L)
{
    P4ModSceneState s;
    p4_mod_scene_state(&s);
    lua_createtable(L, 0, 12);
    lua_pushstring(L, contexts[s.context]); lua_setfield(L, -2, "context");
    lua_pushinteger(L, s.sequence); lua_setfield(L, -2, "sequence");
    lua_pushboolean(L, s.ready); lua_setfield(L, -2, "ready");
    if (!s.ready && s.reason) { lua_pushstring(L, s.reason); lua_setfield(L, -2, "reason"); }
    lua_pushboolean(L, s.control); lua_setfield(L, -2, "control");
    lua_pushboolean(L, s.event); lua_setfield(L, -2, "event");
    lua_pushboolean(L, s.menu); lua_setfield(L, -2, "menu");
    lua_pushboolean(L, s.pending); lua_setfield(L, -2, "pending");
    lua_pushunsigned(L, s.field); lua_setfield(L, -2, "field");
    lua_pushunsigned(L, s.room); lua_setfield(L, -2, "room");
    if (s.entry != 0xffffffffu) { lua_pushunsigned(L, s.entry); lua_setfield(L, -2, "entry"); }
    if (s.step != 0xffffffffu) { lua_pushunsigned(L, s.step); lua_setfield(L, -2, "step"); }
    lua_setreadonly(L, -1, 1);
    return 1;
}

static int scene_available(lua_State *L)
{
    uint32_t field = (uint32_t)p4_lua_integer(L, 1, 0, 999), room = (uint32_t)p4_lua_integer(L, 2, 0, 999);
    lua_pushboolean(L, p4_mod_scene_available(field, room));
    return 1;
}

static int scene_warp(lua_State *L)
{
    P4LuaMod *mod = mutation(L, "p4.scene.warp");
    luaL_checktype(L, 1, LUA_TTABLE);
    uint32_t field = field_u32(L, 1, "field", 1, 0, 999);
    uint32_t room = field_u32(L, 1, "room", 1, 0, 999);
    uint32_t entry = field_u32(L, 1, "entry", 0, 0, 0xffff);
    uint32_t direction = field_u32(L, 1, "direction", 0, 0, 0xffff);
    const char *reason = NULL;
    if (p4_mod_scene_warp(mod->owner, field, room, entry, direction, &reason)) return refused(L, reason);
    lua_pushboolean(L, 1);
    return 1;
}

static int scene_status(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    const char *detail = "";
    int outcome = -1;
    lua_pushstring(L, p4_mod_services_status(mod->owner, &detail, &outcome));
    lua_pushstring(L, detail);
    if (outcome >= 0) lua_pushinteger(L, outcome); else lua_pushnil(L);
    return 3;
}

/* ---- p4.calendar (calendar.pass@1) */

/* Same queue and status as p4.scene: p4.scene.status() reports the pass. */
static int calendar_pass(lua_State *L)
{
    P4LuaMod *mod = mutation(L, "p4.calendar.pass");
    const char *reason = NULL;
    if (p4_mod_calendar_pass(mod->owner, &reason)) return refused(L, reason);
    lua_pushboolean(L, 1);
    return 1;
}

/* ---- p4.encounter */

static int encounter_count(lua_State *L) { lua_pushunsigned(L, p4_mod_encounter_count()); return 1; }

static int encounter_info(lua_State *L)
{
    uint32_t id = (uint32_t)p4_lua_integer(L, 1, 0, 0xffff);
    uint16_t enemies[5], field, variant;
    if (p4_mod_encounter_info(id, enemies, &field, &variant)) { lua_pushnil(L); return 1; }
    lua_createtable(L, 0, 4);
    lua_pushunsigned(L, id); lua_setfield(L, -2, "id");
    lua_createtable(L, 5, 0);
    for (int i = 0; i < 5; ++i) { lua_pushunsigned(L, enemies[i]); lua_rawseti(L, -2, i + 1); }
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "enemies");
    lua_pushunsigned(L, field); lua_setfield(L, -2, "formation");
    lua_pushunsigned(L, variant); lua_setfield(L, -2, "variant");
    lua_setreadonly(L, -1, 1);
    return 1;
}

static int encounter_launch(lua_State *L)
{
    P4LuaMod *mod = mutation(L, "p4.encounter.launch");
    uint32_t id = (uint32_t)p4_lua_integer(L, 1, 0, 0xffff);
    const char *reason = NULL;
    if (p4_mod_encounter_launch(mod->owner, id, &reason)) return refused(L, reason);
    lua_pushboolean(L, 1);
    return 1;
}

/* ---- p4.camera */

static LuaLease *lease(lua_State *L)
{
    LuaLease *handle = p4_lua_userdata(L, 1, P4_LUA_TAG_CAMERA, "camera lease");
    if (handle->mod != p4_luau_user(L)) luaL_error(L, "foreign camera lease");
    return handle;
}

static int camera_acquire(lua_State *L)
{
    P4LuaMod *mod = mutation(L, "p4.camera.acquire");
    size_t length;
    const char *name = luaL_checklstring(L, 1, &length);
    if (!length || length > 47) luaL_argerror(L, 1, "lease name must be 1..47 bytes");
    const char *reason = NULL, *owner = NULL;
    uint32_t generation = p4_mod_camera_acquire(mod->owner, name, &reason, &owner);
    if (!generation) {
        lua_pushnil(L);
        lua_pushstring(L, reason ? reason : "refused");
        if (owner) lua_pushstring(L, owner); else lua_pushnil(L);
        return 3;
    }
    LuaLease *handle = lua_newuserdatataggedwithmetatable(L, sizeof *handle, P4_LUA_TAG_CAMERA);
    *handle = (LuaLease){mod, generation};
    return 1;
}

static float pose_field(lua_State *L, const char *name, float current)
{
    lua_rawgetfield(L, 2, name);
    float value = current;
    if (!lua_isnil(L, -1)) {
        luaL_checktype(L, -1, LUA_TNUMBER);
        value = (float)lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
    return value;
}

static int camera_set(lua_State *L)
{
    LuaLease *handle = lease(L);
    mutation(L, "camera lease set");
    luaL_checktype(L, 2, LUA_TTABLE);
    float pose[6] = {0};
    if (p4_mod_camera_get(handle->mod->owner, handle->generation, pose)) {
        /* Before the first drawn frame every component must be supplied. */
        for (int i = 0; i < 6; ++i) pose[i] = NAN;
    }
    static const char *const names[6] = {"x", "y", "z", "yaw", "pitch", "fov"};
    for (int i = 0; i < 6; ++i) pose[i] = pose_field(L, names[i], pose[i]);
    lua_pushboolean(L, p4_mod_camera_set(handle->mod->owner, handle->generation, pose) == 0);
    return 1;
}

static int camera_get(lua_State *L)
{
    LuaLease *handle = lease(L);
    float pose[6];
    if (p4_mod_camera_get(handle->mod->owner, handle->generation, pose)) { lua_pushnil(L); return 1; }
    static const char *const names[6] = {"x", "y", "z", "yaw", "pitch", "fov"};
    lua_createtable(L, 0, 6);
    for (int i = 0; i < 6; ++i) { lua_pushnumber(L, pose[i]); lua_setfield(L, -2, names[i]); }
    return 1;
}

static int camera_active(lua_State *L)
{
    LuaLease *handle = lease(L);
    const char *why = NULL;
    int active = p4_mod_camera_active(handle->mod->owner, handle->generation, &why);
    lua_pushboolean(L, active);
    if (why) lua_pushstring(L, why); else lua_pushnil(L);
    return 2;
}

static int camera_input(lua_State *L)
{
    LuaLease *handle = lease(L);
    mutation(L, "camera lease input");
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    lua_pushboolean(L, p4_mod_camera_input(handle->mod->owner, handle->generation, lua_toboolean(L, 2)) == 0);
    return 1;
}

static int camera_release(lua_State *L)
{
    LuaLease *handle = lease(L);
    mutation(L, "camera lease release");
    p4_mod_camera_release(handle->mod->owner, handle->generation);
    return 0;
}

static int camera_owner(lua_State *L)
{
    const char *owner = p4_mod_camera_owner();
    if (owner) lua_pushstring(L, owner); else lua_pushnil(L);
    return 1;
}

/* ---- p4.hud */

static const char *const hud_groups[] = {"field_banner", "field_map", "calendar", "party", "battle_commands"};

/* p4.pace changes only wall time: refused where mutations are, but it records
 * no semantic use, so it never tags a save. */
static int pace_set(lua_State *L)
{
    p4_lua_require_mutation(L);
    if (p4_lua_frame(L)->no_native_calls)
        luaL_error(L, "p4.pace.set is not allowed in a synchronous UI callback; stage it and run it from an event");
    const double rate = luaL_checknumber(L, 1);
    const unsigned quarters = rate == 0.25 ? 1u : rate == 0.5 ? 2u : rate == 1.0 ? 4u : rate == 2.0 ? 8u :
        rate == 4.0 ? 16u : 0u;
    if (!quarters) luaL_argerror(L, 1, "the rate must be 0.25, 0.5, 1, 2 or 4");
    const char *reason = NULL;
    P4LuaMod *mod = p4_luau_user(L);
    if (p4_mod_pace_set(mod->owner, quarters, &reason)) return refused(L, reason);
    lua_pushboolean(L, 1);
    return 1;
}

static int pace_state(lua_State *L)
{
    unsigned requested = 4, effective = 4;
    const char *held = NULL;
    p4_mod_pace_state(&requested, &effective, &held);
    lua_pushnumber(L, requested / 4.0);
    lua_pushnumber(L, effective / 4.0);
    if (held) lua_pushstring(L, held); else lua_pushnil(L);
    return 3;
}

static int hud_list(lua_State *L)
{
    int count = (int)(sizeof hud_groups / sizeof hud_groups[0]);
    lua_createtable(L, count, 0);
    for (int i = 0; i < count; ++i) {
        if (!p4_mod_hud_group_valid(hud_groups[i])) continue;
        lua_pushstring(L, hud_groups[i]);
        lua_rawseti(L, -2, i + 1);
    }
    lua_setreadonly(L, -1, 1);
    return 1;
}

static int hud_hide(lua_State *L)
{
    P4LuaMod *mod = mutation(L, "p4.hud.hide");
    const char *group = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    const char *reason = NULL;
    if (p4_mod_hud_hide(mod->owner, group, lua_toboolean(L, 2), &reason)) return refused(L, reason);
    lua_pushboolean(L, 1);
    return 1;
}

static int hud_hidden(lua_State *L)
{
    lua_pushboolean(L, p4_mod_hud_hidden(luaL_checkstring(L, 1)));
    return 1;
}

void p4_lua_services_initialize(lua_State *L)
{
    p4_mod_services_prepare();
    lua_newtable(L);
    lua_newtable(L);
    static const luaL_Reg lease_methods[] = {
        {"set", camera_set}, {"get", camera_get}, {"active", camera_active},
        {"input", camera_input}, {"release", camera_release}, {NULL, NULL}
    };
    luaL_register(L, NULL, lease_methods);
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "__index");
    lua_pushliteral(L, "p4 camera lease"); lua_setfield(L, -2, "__metatable");
    lua_setreadonly(L, -1, 1);
    lua_setuserdatametatable(L, P4_LUA_TAG_CAMERA);

    static const luaL_Reg clock[] = {
        {"wall", clock_wall}, {"updates", clock_updates}, {"retail", clock_retail}, {NULL, NULL}
    };
    p4_lua_library(L, clock);
    lua_setfield(L, -2, "clock");
    static const luaL_Reg scene[] = {
        {"state", scene_state}, {"available", scene_available}, {"warp", scene_warp}, {"status", scene_status}, {NULL, NULL}
    };
    p4_lua_library(L, scene);
    lua_setfield(L, -2, "scene");
    static const luaL_Reg encounter[] = {
        {"count", encounter_count}, {"info", encounter_info}, {"launch", encounter_launch}, {NULL, NULL}
    };
    p4_lua_library(L, encounter);
    lua_setfield(L, -2, "encounter");
    static const luaL_Reg camera[] = {{"acquire", camera_acquire}, {"owner", camera_owner}, {NULL, NULL}};
    p4_lua_library(L, camera);
    lua_setfield(L, -2, "camera");
    static const luaL_Reg hud[] = {{"groups", hud_list}, {"hide", hud_hide}, {"hidden", hud_hidden}, {NULL, NULL}};
    p4_lua_library(L, hud);
    lua_setfield(L, -2, "hud");
    static const luaL_Reg calendar[] = {{"pass", calendar_pass}, {NULL, NULL}};
    p4_lua_library(L, calendar);
    lua_setfield(L, -2, "calendar");
    static const luaL_Reg pace[] = {{"set", pace_set}, {"state", pace_state}, {NULL, NULL}};
    p4_lua_library(L, pace);
    lua_setfield(L, -2, "pace");
}

void p4_lua_services_close(P4LuaMod *mod)
{
    p4_mod_services_release_owner(mod->owner);
}
