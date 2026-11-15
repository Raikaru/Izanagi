/* p4.ui.sprite and p4.ui.overlay (capabilities ui.sprite@1, ui.overlay@1):
 * thin bindings over include/p4pc/ui_overlay.h. Specifications are validated
 * and copied when set; drawing never calls Lua. Both are presentation only:
 * refused in save snapshots, allowed in synchronous UI callbacks (they call no
 * native code), never semantic use. Everything a mod registered is removed
 * when its VM closes; queued sprite packs retain pins until the queue is cleared. */
#include "native_bindings.h"
#include "p4pc/ui_overlay.h"
#include "p4pc/mod_resources_boot.h"
#include "p4pc/mod_services.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct LuaSprite {
    P4LuaMod *mod;
    int handle, width, height;
} LuaSprite;

enum { OVERLAY_ITEMS = 8, KEY_MAX = 60 };
typedef struct LuaOverlay {
    P4LuaMod *mod;
    int count, removed;
    char keys[OVERLAY_ITEMS][KEY_MAX];
    int handles[OVERLAY_ITEMS];
} LuaOverlay;

static int local_key(const char *key)
{
    size_t length = strlen(key);
    if (!length || length >= KEY_MAX) return 0;
    for (size_t i = 0; i < length; ++i) {
        char c = key[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) return 0;
    }
    return 1;
}

static P4LuaMod *presentation(lua_State *L, const char *what)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    if (mod->ui_closing) luaL_error(L, "%s after native UI shutdown", what);
    return mod;
}

static int refused(lua_State *L, const char *reason)
{
    lua_pushnil(L);
    lua_pushstring(L, reason);
    return 2;
}

static const char *ui_error(int result)
{
    switch (result) {
    case P4_UI_FULL: return "no free overlay capacity";
    case P4_UI_NOT_FOUND: return "no such frozen resource image";
    case P4_UI_NOT_POT: return "native sprites need a power-of-two image (put the art in an atlas)";
    default: return "refused";
    }
}

static double number_field(lua_State *L, int table, const char *name, int required, double fallback)
{
    lua_rawgetfield(L, table, name);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        if (required) luaL_error(L, "missing number field %s", name);
        return fallback;
    }
    if (lua_type(L, -1) != LUA_TNUMBER) luaL_error(L, "field %s must be a number", name);
    double value = lua_tonumber(L, -1);
    lua_pop(L, 1);
    if (!isfinite(value)) luaL_error(L, "field %s must be finite", name);
    return value;
}

static int integer_field(lua_State *L, int table, const char *name, int required, int fallback, int low, int high)
{
    double value = number_field(L, table, name, required, fallback);
    if (value != floor(value) || value < low || value > high)
        luaL_error(L, "field %s must be an integer in %d..%d", name, low, high);
    return (int)value;
}

static int flag_field(lua_State *L, int table, const char *name, int fallback)
{
    lua_rawgetfield(L, table, name);
    int value = fallback;
    if (!lua_isnil(L, -1)) {
        if (lua_type(L, -1) != LUA_TBOOLEAN) luaL_error(L, "field %s must be a boolean", name);
        value = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);
    return value;
}

/* p4.ui.sprite(image, {x, y, w, h}): image is this mod's [resource-image <id>:image]. */
int p4_lua_ui_sprite(lua_State *L)
{
    P4LuaMod *mod = presentation(L, "p4.ui.sprite");
    const char *image = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    if (!local_key(image)) luaL_argerror(L, 1, "image must be a local resource key (a-z 0-9 . _ -)");
    char key[160];
    if (snprintf(key, sizeof key, "%s:%s", mod->info->id, image) >= (int)sizeof key)
        luaL_argerror(L, 1, "resource key too long");
    int x = integer_field(L, 2, "x", 1, 0, 0, 4096), y = integer_field(L, 2, "y", 1, 0, 0, 4096);
    int w = integer_field(L, 2, "w", 1, 0, 1, 4096), h = integer_field(L, 2, "h", 1, 0, 1, 4096);
    const P4Resources *resources = p4_resources_images_runtime();
    const P4ResourceImage *found = resources ? p4_resources_find(resources, key) : NULL;
    if (!found) return refused(L, "no such frozen resource image");
    int handle = p4_ui_sprite_create((uint32_t)mod->owner, found->token, x, y, w, h);
    if (handle < 0) {
        if (handle == P4_UI_INVALID) luaL_error(L, "p4.ui.sprite rectangle is outside %s", key);
        return refused(L, ui_error(handle));
    }
    LuaSprite *sprite = lua_newuserdatataggedwithmetatable(L, sizeof *sprite, P4_LUA_TAG_UI_SPRITE);
    *sprite = (LuaSprite){mod, handle, w, h};
    return 1;
}

static int sprite_index(lua_State *L)
{
    LuaSprite *sprite = p4_lua_userdata(L, 1, P4_LUA_TAG_UI_SPRITE, "native sprite");
    const char *name = luaL_checkstring(L, 2);
    if (!strcmp(name, "width")) lua_pushinteger(L, sprite->width);
    else if (!strcmp(name, "height")) lua_pushinteger(L, sprite->height);
    else lua_pushnil(L);
    return 1;
}

static int parse_item(lua_State *L, int table, P4LuaMod *mod, int context, int fade, P4UiOverlayItem *item,
                      const char **key)
{
    luaL_checktype(L, table, LUA_TTABLE);
    lua_rawgetfield(L, table, "key");
    if (lua_type(L, -1) != LUA_TSTRING || !local_key(lua_tostring(L, -1)))
        luaL_error(L, "overlay item key must be a local key (a-z 0-9 . _ -)");
    *key = lua_tostring(L, -1);
    lua_pop(L, 1); /* The item table keeps the key alive. */
    *item = (P4UiOverlayItem){.context = context, .fade = fade, .rgb = 0xFFFFFFu, .alpha = 255, .scale = 1.0f};
    item->x = (float)number_field(L, table, "x", 1, 0);
    item->y = (float)number_field(L, table, "y", 1, 0);
    item->rgb = (uint32_t)integer_field(L, table, "color", 0, 0xFFFFFF, 0, 0xFFFFFF);
    item->alpha = (uint8_t)integer_field(L, table, "alpha", 0, 255, 0, 255);
    item->scale = (float)number_field(L, table, "scale", 0, 1.0);
    lua_rawgetfield(L, table, "align");
    if (!lua_isnil(L, -1)) {
        const char *align = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : "";
        item->align = !strcmp(align, "left") ? P4_UI_ALIGN_LEFT : !strcmp(align, "center") ? P4_UI_ALIGN_CENTER :
            !strcmp(align, "right") ? P4_UI_ALIGN_RIGHT : -1;
        if (item->align < 0) luaL_error(L, "overlay item align must be \"left\", \"center\" or \"right\"");
    }
    lua_pop(L, 1);
    lua_rawgetfield(L, table, "sprite");
    lua_rawgetfield(L, table, "text");
    int has_sprite = !lua_isnil(L, -2), has_text = !lua_isnil(L, -1);
    if (has_sprite == has_text) luaL_error(L, "overlay item %s needs exactly one of text or sprite", *key);
    if (has_sprite) {
        LuaSprite *sprite = p4_lua_userdata(L, -2, P4_LUA_TAG_UI_SPRITE, "native sprite");
        if (sprite->mod != mod) luaL_error(L, "foreign native sprite");
        item->kind = P4_UI_ITEM_SPRITE;
        item->sprite = sprite->handle;
    } else {
        if (lua_type(L, -1) != LUA_TSTRING) luaL_error(L, "overlay item text must be a string");
        item->kind = P4_UI_ITEM_TEXT;
        item->text = lua_tostring(L, -1);
        item->plate = flag_field(L, table, "plate", 0);
        item->width = integer_field(L, table, "width", 0, 0, 0, 640);
    }
    lua_pop(L, 2); /* The item table keeps the text alive. */
    return flag_field(L, table, "visible", 1);
}

static void remove_items(LuaOverlay *overlay)
{
    for (int i = 0; i < overlay->count; ++i)
        p4_ui_overlay_remove((uint32_t)overlay->mod->owner, overlay->handles[i]);
    overlay->count = 0;
    overlay->removed = 1;
}

/* p4.ui.overlay{context, fade?, items}: every item is validated before any is
 * registered; nil plus a reason when the items do not fit beside earlier
 * registrations, and nothing is registered then. */
int p4_lua_ui_overlay(lua_State *L)
{
    P4LuaMod *mod = presentation(L, "p4.ui.overlay");
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_rawgetfield(L, 1, "context");
    const char *where = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : "";
    int context = !strcmp(where, "camp") ? P4_UI_CONTEXT_CAMP : !strcmp(where, "hud") ? P4_UI_CONTEXT_HUD :
                  !strcmp(where, "screen") ? P4_UI_CONTEXT_SCREEN : -1;
    lua_pop(L, 1);
    if (context < 0) luaL_error(L, "p4.ui.overlay context must be \"camp\", \"hud\" or \"screen\"");
    int fade = flag_field(L, 1, "fade", 0);
    if (fade && context == P4_UI_CONTEXT_SCREEN) luaL_error(L, "p4.ui.overlay \"screen\" items never fade");
    lua_rawgetfield(L, 1, "items");
    luaL_checktype(L, -1, LUA_TTABLE);
    int items = lua_gettop(L);
    int count = lua_objlen(L, items);
    if (count < 1 || count > OVERLAY_ITEMS) luaL_error(L, "p4.ui.overlay takes 1..%d items", OVERLAY_ITEMS);
    P4UiOverlayItem parsed[OVERLAY_ITEMS];
    const char *keys[OVERLAY_ITEMS];
    int visible[OVERLAY_ITEMS];
    for (int i = 0; i < count; ++i) {
        lua_rawgeti(L, items, i + 1);
        visible[i] = parse_item(L, lua_gettop(L), mod, context, fade, &parsed[i], &keys[i]);
        lua_pop(L, 1); /* The items table keeps each item table, key and text alive. */
        for (int j = 0; j < i; ++j)
            if (!strcmp(keys[j], keys[i])) luaL_error(L, "duplicate overlay item key %s", keys[i]);
    }
    LuaOverlay *overlay = lua_newuserdatataggedwithmetatable(L, sizeof *overlay, P4_LUA_TAG_UI_OVERLAY);
    *overlay = (LuaOverlay){.mod = mod};
    for (int i = 0; i < count; ++i) {
        int handle = p4_ui_overlay_add((uint32_t)mod->owner, &parsed[i]);
        if (handle < 0) {
            remove_items(overlay);
            if (handle == P4_UI_INVALID)
                luaL_error(L, "overlay item %s is invalid (RAW text of 1..47 bytes, scale above 0 up to 8)", keys[i]);
            return refused(L, ui_error(handle));
        }
        if (!visible[i]) p4_ui_overlay_set_visible((uint32_t)mod->owner, handle, 0);
        strcpy(overlay->keys[i], keys[i]);
        overlay->handles[overlay->count++] = handle;
    }
    p4_mod_hud_status_changed();
    return 1;
}

static int item_handle(lua_State *L, LuaOverlay **out)
{
    LuaOverlay *overlay = p4_lua_userdata(L, 1, P4_LUA_TAG_UI_OVERLAY, "overlay");
    if (overlay->mod != p4_luau_user(L)) luaL_error(L, "foreign overlay");
    const char *key = luaL_checkstring(L, 2);
    *out = overlay;
    for (int i = 0; i < overlay->count; ++i)
        if (!strcmp(overlay->keys[i], key)) return overlay->handles[i];
    if (overlay->removed) luaL_error(L, "overlay was removed");
    luaL_error(L, "no overlay item %s", key);
    return -1;
}

static int overlay_set(lua_State *L)
{
    LuaOverlay *overlay;
    presentation(L, "overlay set");
    int handle = item_handle(L, &overlay);
    const char *text = luaL_checkstring(L, 3);
    lua_pushboolean(L, p4_ui_overlay_set_text((uint32_t)overlay->mod->owner, handle, text) == P4_UI_OK);
    return 1;
}

static int overlay_show(lua_State *L)
{
    LuaOverlay *overlay;
    presentation(L, "overlay show");
    int handle = item_handle(L, &overlay);
    luaL_checktype(L, 3, LUA_TBOOLEAN);
    p4_ui_overlay_set_visible((uint32_t)overlay->mod->owner, handle, lua_toboolean(L, 3));
    return 0;
}

static int overlay_remove(lua_State *L)
{
    LuaOverlay *overlay = p4_lua_userdata(L, 1, P4_LUA_TAG_UI_OVERLAY, "overlay");
    if (overlay->mod != p4_luau_user(L)) luaL_error(L, "foreign overlay");
    p4_lua_require_mutation(L);
    remove_items(overlay);
    p4_mod_hud_status_changed();
    return 0;
}

void p4_lua_overlay_initialize(lua_State *L)
{
    lua_newtable(L);
    lua_pushcfunction(L, sprite_index, "p4.ui.sprite.__index"); lua_setfield(L, -2, "__index");
    lua_pushliteral(L, "p4 native sprite"); lua_setfield(L, -2, "__metatable");
    lua_setreadonly(L, -1, 1);
    lua_setuserdatametatable(L, P4_LUA_TAG_UI_SPRITE);
    lua_newtable(L);
    lua_newtable(L);
    static const luaL_Reg overlay_methods[] = {
        {"set", overlay_set}, {"show", overlay_show}, {"remove", overlay_remove}, {NULL, NULL}
    };
    luaL_register(L, NULL, overlay_methods);
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "__index");
    lua_pushliteral(L, "p4 overlay"); lua_setfield(L, -2, "__metatable");
    lua_setreadonly(L, -1, 1);
    lua_setuserdatametatable(L, P4_LUA_TAG_UI_OVERLAY);
}

int p4_lua_ui_sprite_handle(lua_State *L, int index)
{
    LuaSprite *sprite = p4_lua_userdata(L, index, P4_LUA_TAG_UI_SPRITE, "native sprite");
    if (sprite->mod != p4_luau_user(L)) luaL_error(L, "foreign native sprite");
    return sprite->handle;
}

void p4_lua_overlay_close(P4LuaMod *mod)
{
    p4_ui_overlay_release_owner((uint32_t)mod->owner);
}
