#ifndef P4PC_LUA_RUNTIME_H
#define P4PC_LUA_RUNTIME_H
#include "p4pc/mods.h"
#include "p4pc/mod_script.h"
#include "p4pc/mod_failure.h"
#include "p4pc/luau_c.h"
#include <fenv.h>
#include <stdint.h>

enum {
    P4_LUA_TAG_BUFFER = 1,
    P4_LUA_TAG_PIN,
    P4_LUA_TAG_HOOK,
    P4_LUA_TAG_CONTEXT,
    P4_LUA_TAG_EVENT,
    P4_LUA_TAG_FUNCTION,
    P4_LUA_TAG_UI_PAGE,
    P4_LUA_TAG_AUTHORIZER,
    P4_LUA_TAG_CAMERA,
    P4_LUA_TAG_STAGE,
    P4_LUA_TAG_UI_SPRITE,
    P4_LUA_TAG_UI_OVERLAY,
    P4_LUA_TAG_UI_NATIVE_ROW
};
typedef struct P4LuaContext P4LuaContext;
enum { P4_LUA_STARTUP = 1, P4_LUA_SYNCHRONOUS = 2, P4_LUA_SEMANTIC = 4, P4_LUA_SNAPSHOT = 8 };
enum { P4_LUA_LAYOUT_HELPERS = 6 };
typedef struct P4LuaFrame {
    struct P4LuaFrame *previous;
    P4LuaContext *context;
    fenv_t native_env;
    unsigned fuel;
    int startup, no_native_calls, snapshot;
} P4LuaFrame;
typedef struct P4LuaThread {
    uint64_t key;
    lua_State *state;
    P4LuaFrame *frame;
    int reference, executing;
    unsigned depth;
} P4LuaThread;
struct P4LuaNativeBindings;
struct P4LuaUIPage;
struct P4LuaSave;
struct P4ModSettingDecl;
typedef struct P4LuaMod {
    p4_luau *vm;
    lua_State *state; /* idle control plane; callbacks use a native-coroutine Lua thread */
    const P4PackageInfo *info;
    const P4PackageProvider *provider;
    size_t owner, scratch_bytes;
    unsigned subscriptions; /* lifetime bound: native removed nodes remain tombstones */
    int string_copy_reference, buffer_release_reference, layout_factory_reference, invoke_reference;
    int layout_helpers[P4_LUA_LAYOUT_HELPERS];
    int file_read_reference;
    int main_reference;
    const struct P4LuaNativeBindings *native_bindings;
    struct P4LuaSave *save;
    struct P4LuaUIPage *ui_pages;
    int ui_reference, ui_closing;
    struct P4ModSettingDecl *settings; /* parsed [setting] declarations, on first p4.settings use */
    size_t setting_count;
    int settings_parsed;
    char error[1024]; /* Owned before the protected call pops its Lua error. */
    P4LuaThread threads[65];
    uint32_t budget_frame, pressure_frame;
    unsigned frame_fuel;
    int has_budget_frame, has_pressure_frame, budget_failed, semantic_used;
} P4LuaMod;
typedef struct P4LuaInvocation {
    lua_CFunction function;
    void *data;
} P4LuaInvocation;

P4LuaMod *p4_lua_create(const P4PackageProvider *provider);
void p4_lua_destroy(P4LuaMod *mod);
int p4_lua_compile(P4LuaMod *mod, const char *source, size_t size, const char *name);
int p4_lua_execute(P4LuaMod *mod);
const char *p4_lua_error(const P4LuaMod *mod);
/* Actual active VM-owned selected provider, borrowed; no ordinal/native guess. */
const P4PackageProvider *p4_mod_scripts_owner_provider(size_t owner);
void p4_lua_semantic_use(P4LuaMod *mod);
P4LuaThread *p4_lua_thread(P4LuaMod *mod);
P4LuaFrame *p4_lua_frame(lua_State *state);
int p4_lua_protected(P4LuaMod *mod, lua_State *state, int arguments, int results, int flags);
int p4_lua_invoke(P4LuaMod *mod, lua_CFunction function, void *data, int results, int flags);
/* Snapshot callbacks are pure: no guest/native mutation or game dispatch. */
void p4_lua_require_mutation(lua_State *state);
/* The hook invocation the current frame runs (lua_hooks.c), NULL outside one;
 * *table receives its context table reference. */
struct P4ModCall;
const struct P4ModCall *p4_lua_hook_call(lua_State *state, int *table);
void p4_lua_save_library(lua_State *state);
void p4_lua_save_destroy(P4LuaMod *mod);
/* C-owned copies under the p4.save value rules (nil/boolean/number/string and
 * acyclic tables without metatables). copy returns NULL with a bounded reason;
 * push may raise a Lua allocation error and makes every table read-only when asked. */
void *p4_lua_value_copy(lua_State *state, int index, size_t maximum, const char *what, char *error, size_t capacity);
void p4_lua_value_push(lua_State *state, const void *value, int readonly);
void p4_lua_value_free(void *value);
/* Restore the caller's game FP environment around an original/native call.
 * Native work does not consume Luau safepoint budget. */
void p4_lua_native_enter(lua_State *state, fenv_t *script_environment);
void p4_lua_native_leave(lua_State *state, const fenv_t *script_environment);

int64_t p4_lua_integer(lua_State *state, int index, int64_t low, int64_t high);
uint64_t p4_lua_wide(lua_State *state, int index);
void p4_lua_push_wide(lua_State *state, uint64_t value);
void *p4_lua_userdata(lua_State *state, int index, int tag, const char *name);
void p4_lua_library(lua_State *state, const luaL_Reg *methods);
/* Merge the top table into the namespace on the p4 table immediately below.
 * Both providers must use this helper; duplicate fields fail initialization.
 * The resulting namespace is readonly. */
void p4_lua_namespace_merge(lua_State *state, const char *name);
void p4_lua_memory(lua_State *state);
uint32_t p4_lua_function_address(lua_State *state, int index);
int p4_lua_function_reference(lua_State *state);
void p4_lua_hooks(P4LuaMod *mod);
void p4_lua_events(lua_State *state);
void p4_lua_input(lua_State *state);
void p4_lua_files(lua_State *state);
void p4_lua_settings(lua_State *state);
void p4_lua_lifecycle(lua_State *state);
void p4_lua_mods(lua_State *state);
void p4_lua_contract(lua_State *state);
/* Forgets the contracts a mod published; VM teardown only. */
void p4_lua_contract_destroy(P4LuaMod *mod);
void p4_lua_artifacts(lua_State *state);
#endif
