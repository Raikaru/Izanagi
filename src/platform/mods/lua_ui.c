#include "native_bindings.h"
#include "p4pc/mod_settings.h"
#include "p4pc/settings_pages.h"
#include "p4pc/mod_status.h"
#include "p4pc/mod_services.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct P4LuaUIPage {
    struct P4LuaUIPage *next, *unlisted_next;
    P4LuaMod *mod;
    char key[P4_SETTINGS_PAGE_KEY_MAX], title[P4_SETTINGS_PAGE_LABEL_MAX];
    uint64_t revision;
    int registered, attached, adjustable;
} P4LuaUIPage;
/* Registered unlisted pages of every mod: only their own mod may link them. */
static P4LuaUIPage *unlisted_pages;
static const P4LuaUIPage *unlisted_page(const char *key)
{
    for (const P4LuaUIPage *page = unlisted_pages; page; page = page->unlisted_next)
        if (!strcmp(page->key, key)) return page;
    return NULL;
}
enum Query { COUNT, ROW, FIND, ADJUST, DETAIL, OPENED, CLOSED };
static const char *const callbacks[] = {"count", "row", "find", "adjust", "detail", "opened", "closed"};
typedef struct UIInvocation {
    P4LuaUIPage *page;
    enum Query query;
    P4SettingsPageRow *row;
    const char *key, *text;
    int index, direction, result;
} UIInvocation;

static const char *bounded_text(lua_State *L, int index, size_t capacity, size_t *length)
{
    luaL_checktype(L, index, LUA_TSTRING);
    size_t size;
    const char *text = lua_tolstring(L, index, &size);
    if (size >= capacity || memchr(text, 0, size)) luaL_error(L, "native UI text is oversized or contains NUL");
    if (length) *length = size;
    return text;
}
static const char *text_field(lua_State *L, int table, const char *name, size_t capacity, int required, size_t *length)
{
    lua_rawgetfield(L, table, name);
    if (lua_isnil(L, -1) && !required) { lua_pop(L, 1); return NULL; }
    const char *text = bounded_text(L, -1, capacity, length);
    lua_pop(L, 1);
    return text; /* The owning row/options table still holds the string. */
}
static int bool_field(lua_State *L, int table, const char *name)
{
    lua_rawgetfield(L, table, name);
    if (!lua_isnil(L, -1)) luaL_checktype(L, -1, LUA_TBOOLEAN);
    int value = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return value;
}
static int page_data(lua_State *L, P4LuaUIPage *page)
{
    lua_getref(L, page->mod->ui_reference);
    lua_rawgetptagged(L, -1, page, 0);
    lua_remove(L, -2);
    luaL_checktype(L, -1, LUA_TTABLE);
    return lua_gettop(L);
}
static void parse_row(lua_State *L, P4LuaUIPage *page, P4SettingsPageRow *out)
{
    luaL_checktype(L, -1, LUA_TTABLE);
    int table = lua_gettop(L);
    *out = (P4SettingsPageRow){0};
    out->key = text_field(L, table, "key", P4_SETTINGS_PAGE_KEY_MAX, 1, NULL);
    if (!out->key[0]) luaL_error(L, "native UI row key is empty");
    out->label = text_field(L, table, "label", P4_SETTINGS_PAGE_LABEL_MAX, 1, NULL);
    out->value = text_field(L, table, "value", P4_SETTINGS_PAGE_VALUE_MAX, 0, NULL);
    out->help = text_field(L, table, "help", P4_SETTINGS_PAGE_HELP_MAX, 0, NULL);
    const char *kind = text_field(L, table, "kind", 16, 1, NULL);
    if (!strcmp(kind, "info")) out->kind = P4_SETTINGS_PAGE_INFO;
    else if (!strcmp(kind, "toggle")) out->kind = P4_SETTINGS_PAGE_TOGGLE;
    else if (!strcmp(kind, "value")) out->kind = P4_SETTINGS_PAGE_VALUE;
    else if (!strcmp(kind, "action")) out->kind = P4_SETTINGS_PAGE_ACTION;
    else if (!strcmp(kind, "link")) out->kind = P4_SETTINGS_PAGE_LINK;
    else luaL_error(L, "unknown native UI row kind");
    if (!page->adjustable && out->kind != P4_SETTINGS_PAGE_INFO && out->kind != P4_SETTINGS_PAGE_LINK)
        luaL_error(L, "interactive native UI rows require an adjust callback");
    if (bool_field(L, table, "disabled")) out->flags |= P4_SETTINGS_PAGE_DISABLED;
    if (bool_field(L, table, "restart")) out->flags |= P4_SETTINGS_PAGE_RESTART;
    if (bool_field(L, table, "current")) out->flags |= P4_SETTINGS_PAGE_CURRENT;
    out->checked = bool_field(L, table, "checked");
    if (out->kind == P4_SETTINGS_PAGE_LINK) {
        out->target = text_field(L, table, "target", P4_SETTINGS_PAGE_KEY_MAX, 1, NULL);
        if (!out->target[0]) luaL_error(L, "native UI link target is empty");
        const P4LuaUIPage *target = unlisted_page(out->target);
        if (target && target->mod != page->mod) luaL_error(L, "native UI link to another mod's unlisted page");
    }
}
static int invoke_ui(lua_State *L)
{
    UIInvocation *call = lua_touserdata(L, 1);
    int data = page_data(L, call->page);
    lua_pushnil(L); lua_rawsetfield(L, data, "result");
    lua_rawgetfield(L, data, callbacks[call->query]);
    luaL_checktype(L, -1, LUA_TFUNCTION);
    int arguments = 0;
    if (call->query == ROW) { lua_pushinteger(L, call->index + 1); arguments = 1; }
    else if (call->query == FIND || call->query == ADJUST) {
        lua_pushstring(L, call->key);
        arguments = 1;
        if (call->query == ADJUST) { lua_pushinteger(L, call->direction); ++arguments; }
    }
    int result = call->query == COUNT || call->query == ROW || call->query == FIND || call->query == DETAIL;
    lua_call(L, arguments, result);
    if (result) {
        /* Native code copies borrowed strings before another provider call.
         * One private table owns this exact result, without copying large text. */
        lua_pushvalue(L, -1); lua_rawsetfield(L, data, "result");
    }
    if (call->query == COUNT)
        call->result = (int)p4_lua_integer(L, -1, 0, P4_SETTINGS_PAGE_ROWS_MAX);
    else if (call->query == FIND)
        call->result = lua_isnil(L, -1) ? -1 : (int)p4_lua_integer(L, -1, 1, P4_SETTINGS_PAGE_ROWS_MAX) - 1;
    else if (call->query == ROW) parse_row(L, call->page, call->row);
    else if (call->query == DETAIL) call->text = bounded_text(L, -1, P4_SETTINGS_PAGE_DETAIL_MAX, NULL);
    return 0;
}
static void query(UIInvocation *call)
{
    P4LuaMod *mod = call->page->mod;
    if (p4_lua_invoke(mod, invoke_ui, call, 0, P4_LUA_SYNCHRONOUS|P4_LUA_SEMANTIC))
        p4_mod_failure(mod->owner, p4_lua_error(mod));
}
static int page_count(void *user, uint64_t *revision)
{
    UIInvocation call = {.page = user, .query = COUNT};
    query(&call);
    *revision = call.page->revision;
    return call.result;
}
static int page_row(void *user, int index, P4SettingsPageRow *out)
{
    UIInvocation call = {.page = user, .query = ROW, .row = out, .index = index};
    query(&call);
    return 0;
}
static int page_find(void *user, const char *key)
{
    UIInvocation call = {.page = user, .query = FIND, .key = key};
    query(&call);
    return call.result;
}
static int page_adjust(void *user, const char *key, int direction)
{
    UIInvocation call = {.page = user, .query = ADJUST, .key = key, .direction = direction};
    query(&call);
    return 0;
}
static const char *page_detail(void *user, uint64_t *revision)
{
    UIInvocation call = {.page = user, .query = DETAIL};
    query(&call);
    *revision = call.page->revision;
    return call.text;
}
static int page_opened(void *user)
{
    UIInvocation call = {.page = user, .query = OPENED};
    query(&call);
    return 0;
}
static void page_closed(void *user)
{
    UIInvocation call = {.page = user, .query = CLOSED};
    query(&call);
}
static P4LuaUIPage *checked_page(lua_State *L)
{
    P4LuaUIPage *page = p4_lua_userdata(L, 1, P4_LUA_TAG_UI_PAGE, "native UI page");
    if (page->mod != p4_luau_user(L)) luaL_error(L, "foreign native UI page");
    return page;
}
static int open_page(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaUIPage *page = checked_page(L);
    lua_pushboolean(L, page->registered && !page->mod->ui_closing && p4_settings_page_open(page->key) == 0);
    return 1; /* Accepted request is not a claim that the native view is visible. */
}
static int refresh_page(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaUIPage *page = checked_page(L);
    if (!page->registered || page->mod->ui_closing) { lua_pushboolean(L, 0); return 1; }
    if (page->revision == UINT64_MAX) luaL_error(L, "native UI revision exhausted");
    ++page->revision;
    lua_pushboolean(L, 1);
    return 1;
}
static int page_index(lua_State *L)
{
    P4LuaUIPage *page = checked_page(L);
    const char *key = luaL_checkstring(L, 2);
    if (!strcmp(key, "key")) { lua_pushstring(L, page->key); return 1; }
    lua_getuserdatametatable(L, P4_LUA_TAG_UI_PAGE);
    lua_rawgetfield(L, -1, "methods");
    lua_pushvalue(L, 2); lua_rawget(L, -2);
    return 1;
}
static void page_destroyed(lua_State *L, void *user)
{
    (void)L;
    P4LuaUIPage *page = user;
    if (page->registered || page->attached) {
        fprintf(stderr, "p4pc: programming error: native page %s outlived its owning VM\n", page->key);
        _Exit(1);
    }
}
static int function_field(lua_State *L, const char *name, int required)
{
    lua_rawgetfield(L, 1, name);
    int present = !lua_isnil(L, -1);
    if (required || present) luaL_checktype(L, -1, LUA_TFUNCTION);
    lua_pop(L, 1);
    return present;
}
static void forget_unregistered(lua_State *L, P4LuaUIPage *page)
{
    page->mod->ui_pages = page->next; /* Creation never invokes a provider callback. */
    lua_getref(L, page->mod->ui_reference);
    lua_pushnil(L); lua_rawsetptagged(L, -2, page, 0);
    lua_pop(L, 1);
}
static int create_page(lua_State *L, int detail)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    if (mod->ui_closing || p4_lua_frame(L)->no_native_calls)
        luaL_error(L, "register native UI pages outside synchronous provider callbacks and shutdown");
    size_t key_size, title_size;
    const char *key = text_field(L, 1, "key", 60, 1, &key_size);
    const char *title = text_field(L, 1, "title", P4_SETTINGS_PAGE_LABEL_MAX, 1, &title_size);
    if (!key_size || !title_size) luaL_error(L, "native UI page key/title is empty");
    for (size_t i = 0; i < key_size; ++i) {
        unsigned char c = (unsigned char)key[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.'))
            luaL_error(L, "native UI local keys use ASCII letters/digits/underscore/hyphen/dot");
    }
    int present[sizeof callbacks / sizeof callbacks[0]];
    for (size_t i = 0; i < sizeof callbacks / sizeof callbacks[0]; ++i)
        present[i] = function_field(L, callbacks[i], detail ? i == DETAIL : i == COUNT || i == ROW || i == FIND);
    if (detail ? (present[COUNT] || present[ROW] || present[FIND] || present[ADJUST]) : present[DETAIL])
        luaL_error(L, "native UI detail and row callbacks cannot be mixed");
    /* listed=false (API 1.4): reachable through link rows only, no hub link. */
    lua_rawgetfield(L, 1, "listed");
    if (!lua_isnil(L, -1)) luaL_checktype(L, -1, LUA_TBOOLEAN);
    int listed = lua_isnil(L, -1) || lua_toboolean(L, -1);
    lua_pop(L, 1);
    if (mod->ui_reference == LUA_NOREF) {
        lua_newtable(L);
        mod->ui_reference = lua_ref(L, -1);
        lua_pop(L, 1);
    }
    P4LuaUIPage *page = lua_newuserdatataggedwithmetatable(L, sizeof *page, P4_LUA_TAG_UI_PAGE);
    memset(page, 0, sizeof *page);
    page->mod = mod;
    page->revision = 1;
    page->adjustable = present[ADJUST];
    int userdata = lua_gettop(L);
    int size = snprintf(page->key, sizeof page->key, "lua/%s/%s", mod->info->id, key);
    if (size < 0 || (size_t)size >= sizeof page->key) luaL_error(L, "namespaced native UI key is too long");
    memcpy(page->title, title, title_size + 1);
    lua_createtable(L, 0, 9);
    int data = lua_gettop(L);
    lua_pushvalue(L, userdata); lua_rawsetfield(L, data, "self");
    for (size_t i = 0; i < sizeof callbacks / sizeof callbacks[0]; ++i) {
        lua_rawgetfield(L, 1, callbacks[i]); lua_rawsetfield(L, data, callbacks[i]);
    }
    lua_getref(L, mod->ui_reference);
    lua_pushvalue(L, data); lua_rawsetptagged(L, -2, page, 0);
    lua_pop(L, 1);
    page->next = mod->ui_pages;
    mod->ui_pages = page; /* Private root keeps descriptors/callbacks alive, even if handle is dropped. */
    P4SettingsPage native = {
        .key = page->key, .title = page->title, .userdata = page,
        .count = detail ? NULL : page_count, .row = detail ? NULL : page_row,
        .find = detail ? NULL : page_find, .adjust = present[ADJUST] ? page_adjust : NULL,
        .detail = detail ? page_detail : NULL,
        .opened = present[OPENED] ? page_opened : NULL, .closed = present[CLOSED] ? page_closed : NULL
    };
    if (p4_settings_page_register(&native)) {
        forget_unregistered(L, page);
        luaL_error(L, "native UI page registration refused");
    }
    page->registered = 1;
    if (listed && p4_mod_settings_attach_page(mod, P4_MOD_SETTINGS_MOD, mod->info->id, page->key, page->title)) {
        if (p4_settings_page_unregister(page->key))
            p4_mod_failure(mod->owner, "Native UI attachment failed and its registered page could not be safely detached");
        page->registered = 0;
        forget_unregistered(L, page);
        luaL_error(L, "native Mods hub attachment refused");
    }
    page->attached = listed;
    if (!listed) {
        page->unlisted_next = unlisted_pages;
        unlisted_pages = page;
    }
    lua_pushvalue(L, userdata);
    return 1;
}
static int new_page(lua_State *L) { return create_page(L, 0); }
static int new_detail(lua_State *L) { return create_page(L, 1); }
static int go_back(lua_State *L) { p4_lua_require_mutation(L); lua_pushboolean(L, p4_settings_page_back() == 0); return 1; }
static int close_page(lua_State *L) { p4_lua_require_mutation(L); lua_pushboolean(L, p4_settings_page_close() == 0); return 1; }
static int active_page(lua_State *L) { lua_pushstring(L, p4_settings_page_active()); return 1; }
/* p4.ui.status(text|{lines}|nil, placement?): replaces this mod's status rows.
 * Returns false, keeping the previous rows, when a line is invalid or the
 * rows do not fit beside earlier-loaded mods' rows. */
static int set_status(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    const char *lines[P4_MOD_STATUS_ROWS];
    int count = 0;
    if (lua_type(L, 1) == LUA_TSTRING) {
        lines[count++] = lua_tostring(L, 1);
    } else if (lua_istable(L, 1)) {
        int length = lua_objlen(L, 1);
        if (length > P4_MOD_STATUS_ROWS) { lua_pushboolean(L, 0); return 1; }
        for (int i = 1; i <= length; ++i) {
            lua_rawgeti(L, 1, i);
            if (lua_type(L, -1) != LUA_TSTRING) luaL_error(L, "p4.ui.status lines must be strings");
            lines[count++] = lua_tostring(L, -1);
            lua_pop(L, 1); /* The table keeps each string alive. */
        }
    } else if (!lua_isnoneornil(L, 1)) {
        luaL_error(L, "p4.ui.status takes a string, a table of strings or nil");
    }
    const char *where = luaL_optstring(L, 2, "camp");
    int placement = !strcmp(where, "camp") ? P4_MOD_STATUS_CAMP : !strcmp(where, "hud") ? P4_MOD_STATUS_HUD : -1;
    if (placement < 0) luaL_error(L, "p4.ui.status placement must be \"camp\" or \"hud\"");
    if (count && mod->ui_closing) luaL_error(L, "p4.ui.status after native UI shutdown");
    lua_pushboolean(L, p4_mod_status_set((uint32_t)mod->owner, placement, lines, count) == 0);
    p4_mod_hud_status_changed();
    return 1;
}
void p4_lua_ui_initialize(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    mod->ui_reference = LUA_NOREF;
    lua_newtable(L);
    lua_pushcfunction(L, page_index, "p4.ui.page.__index"); lua_setfield(L, -2, "__index");
    lua_newtable(L);
    lua_pushcfunction(L, open_page, "p4.ui.page.open"); lua_setfield(L, -2, "open");
    lua_pushcfunction(L, refresh_page, "p4.ui.page.refresh"); lua_setfield(L, -2, "refresh");
    lua_setreadonly(L, -1, 1); lua_setfield(L, -2, "methods");
    lua_pushliteral(L, "p4 native UI page"); lua_setfield(L, -2, "__metatable");
    lua_setreadonly(L, -1, 1);
    lua_setuserdatametatable(L, P4_LUA_TAG_UI_PAGE);
    lua_setuserdatadtor(L, P4_LUA_TAG_UI_PAGE, page_destroyed);
    static const luaL_Reg methods[] = {
        {"page", new_page}, {"detail", new_detail}, {"back", go_back},
        {"close", close_page}, {"active", active_page}, {"status", set_status}, {NULL, NULL}
    };
    p4_lua_library(L, methods);
    lua_setfield(L, -2, "ui");
}
void p4_lua_ui_close(P4LuaMod *mod)
{
    mod->ui_closing = 1;
    p4_mod_status_set((uint32_t)mod->owner, P4_MOD_STATUS_CAMP, NULL, 0);
    p4_mod_hud_status_changed();
    if (mod->ui_pages) {
        for (P4LuaUIPage **slot = &unlisted_pages; *slot;)
            if ((*slot)->mod == mod) *slot = (*slot)->unlisted_next; else slot = &(*slot)->unlisted_next;
        if (p4_mod_settings_detach_owner(mod) < 0)
            p4_mod_failure(mod->owner, "Native Mods hub links could not be safely detached before VM shutdown");
        for (P4LuaUIPage *page = mod->ui_pages; page; page = page->next) {
            page->attached = 0;
            if (page->registered && p4_settings_page_unregister(page->key))
                p4_mod_failure(mod->owner, "Native UI page is still in use; its VM cannot be safely released");
            page->registered = 0;
        }
        mod->ui_pages = NULL;
    }
    if (mod->ui_reference != LUA_NOREF) {
        lua_unref(mod->state, mod->ui_reference);
        mod->ui_reference = LUA_NOREF;
    }
}
