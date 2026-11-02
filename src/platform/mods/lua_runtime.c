#include "lua_runtime.h"
#include "p4pc/guest.h"
#include "p4pc/mod_events.h"
#include "p4pc/mod_hooks.h"
#include "p4pc/mod_memory.h"
#include "p4pc/os.h"
#include "p4pc/platform.h"
#include "mod_luau_api.h"
#include "script_prepare.h"
#include "native_bindings.h"
#include "p4pc/mod_failure.h"
#include "p4pc/mod_save.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LUA_MEMORY_LIMIT (8u * 1024u * 1024u)
#define CALLBACK_FUEL 4096u
#define FRAME_FUEL 8192u
#define STARTUP_FUEL 65536u
static P4LuaMod *active[P4_PACKAGE_MAX];
static int script_phase; /* 0 empty, 1 compiled only, 2 executing init, 3 initialized */
static const P4LuaNativeBindings *native_bindings;
int p4_lua_bind_native(const P4LuaNativeBindings *bindings)
{
    if (!bindings || !bindings->initialize || !bindings->before_destroy || script_phase ||
        (native_bindings && native_bindings != bindings)) return -1;
    native_bindings = bindings;
    return 0;
}
const P4PackageProvider *p4_mod_scripts_owner_provider(size_t owner)
{
    /* Failure is cold. Match the actual owning VM/pin, not whichever package
     * happens to occupy an ordinal in a different generation. */
    for(size_t i=0;i<P4_PACKAGE_MAX;i++){
        P4LuaMod *mod=active[i];
        if(mod&&mod->owner==owner&&p4_package_provider_selected(mod->provider))return mod->provider;
    }
    return NULL;
}
void p4_lua_semantic_use(P4LuaMod *mod)
{
    if(mod->semantic_used)return;
    if(p4_mod_save_touch(mod->provider))
        p4_mod_failure(mod->owner,"Actual script use could not record its selected provider provenance");
    mod->semantic_used=1; /* Process-scoped history survives load/New Game. */
}

static P4LuaThread *current_thread(P4LuaMod *mod)
{
    uint64_t key = p4pc_thread_context_key();
    unsigned slot = (unsigned)(key & 127u);
    if (slot >= sizeof mod->threads / sizeof mod->threads[0]) return NULL;
    P4LuaThread *thread = &mod->threads[slot];
    return thread->state != NULL && thread->key == key ? thread : NULL;
}

/* Count both invocation entries and VM safepoints. Counting entries prevents
 * a frame with many tiny callbacks from evading the aggregate limit. */
static int charge_frame(P4LuaMod *mod)
{
    uint32_t frame = p4pc_flip_count();
    if (!mod->has_budget_frame || frame != mod->budget_frame) {
        mod->has_budget_frame = 1;
        mod->budget_frame = frame;
        mod->frame_fuel = FRAME_FUEL;
    }
    if (!mod->frame_fuel) return 0;
    --mod->frame_fuel;
    return 1;
}

static int over_budget(P4LuaMod *mod, P4LuaThread *thread)
{
    if (mod->budget_failed) return 1;
    for (P4LuaFrame *frame = thread->frame; frame != NULL; frame = frame->previous)
        if (frame->fuel == 0) return 1;
    return 0;
}

static int interrupt(void *user, int gc)
{
    (void)gc;
    P4LuaMod *mod = user;
    P4LuaThread *thread = current_thread(mod);
    if (thread == NULL || thread->frame == NULL) return mod->budget_failed;
    if (!thread->frame->startup && !charge_frame(mod)) mod->budget_failed = 1;
    for (P4LuaFrame *frame = thread->frame; frame != NULL; frame = frame->previous)
        if (frame->fuel != 0) --frame->fuel;
    if (over_budget(mod, thread)) mod->budget_failed = 1;
    return mod->budget_failed;
}

typedef struct ThreadCreate { lua_State *state; int reference; } ThreadCreate;
static int create_thread(lua_State *L)
{
    ThreadCreate *created = lua_touserdata(L, 1);
    created->state = lua_newthread(L);
    created->reference = lua_ref(L, -1);
    return 0;
}

P4LuaThread *p4_lua_thread(P4LuaMod *mod)
{
    uint64_t key = p4pc_thread_context_key();
    unsigned slot = (unsigned)(key & 127u);
    if (slot >= sizeof mod->threads / sizeof mod->threads[0]) return NULL;
    P4LuaThread *thread = &mod->threads[slot];
    if (thread->state != NULL && thread->key == key) return thread;
    ThreadCreate created = {NULL, LUA_NOREF};
    if (lua_cpcall(mod->state, create_thread, &created) != LUA_OK) {
        fprintf(stderr, "p4pc: mod %s: cannot allocate coroutine-local Lua stack\n", mod->info->id);
        lua_pop(mod->state, 1);
        return NULL;
    }
    if (thread->reference != LUA_NOREF) lua_unref(mod->state, thread->reference);
    *thread = (P4LuaThread){.key = key, .state = created.state, .reference = created.reference};
    return thread;
}

P4LuaFrame *p4_lua_frame(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    P4LuaThread *thread = current_thread(mod);
    if (thread == NULL || thread->frame == NULL)
        luaL_error(L, "game API used outside a protected mod invocation");
    return thread->frame;
}

static int record_error(P4LuaMod *mod, const char *message)
{
    snprintf(mod->error, sizeof mod->error, "%s", message && message[0] ? message : "Unknown interpreter failure");
    return -1;
}
const char *p4_lua_error(const P4LuaMod *mod)
{
    return mod->error[0] ? mod->error : p4_luau_error(mod->vm);
}
int p4_lua_protected(P4LuaMod *mod, lua_State *L, int arguments, int results, int flags)
{
    P4LuaThread *thread = current_thread(mod);
    int base = lua_gettop(L) - arguments - 1;
    if (thread == NULL || thread->depth >= 64 || mod->budget_failed) {
        lua_settop(L, base);
        return record_error(mod, mod->budget_failed ? "Interpreter safepoint budget exhausted" :
                            "Interpreter native-coroutine nesting limit exceeded");
    }
    if(flags&P4_LUA_SEMANTIC)p4_lua_semantic_use(mod);
    int was_executing = thread->executing;
    if (!(flags & P4_LUA_STARTUP) && !charge_frame(mod)) {
        mod->budget_failed = 1;
        lua_settop(L, base);
        return record_error(mod, "Interpreter safepoint budget exhausted");
    }
    fenv_t caller_environment;
    fegetenv(&caller_environment);
    P4LuaFrame frame = {
        .previous = thread->frame,
        .fuel = flags & P4_LUA_STARTUP ? STARTUP_FUEL : CALLBACK_FUEL,
        .startup = (flags & P4_LUA_STARTUP) != 0,
        .no_native_calls = (flags & (P4_LUA_SYNCHRONOUS|P4_LUA_SNAPSHOT)) ||
            (thread->frame && thread->frame->no_native_calls),
        .snapshot = (flags & P4_LUA_SNAPSHOT) || (thread->frame && thread->frame->snapshot)
    };
    if (was_executing && frame.previous != NULL) frame.native_env = frame.previous->native_env;
    else frame.native_env = caller_environment;
    thread->frame = &frame;
    ++thread->depth;
    thread->executing = 1;
    fesetenv(FE_DFL_ENV);
    int status = lua_pcall(L, arguments, results, 0);
    if (over_budget(mod, thread)) mod->budget_failed = 1;
    if (mod->budget_failed) {
        fprintf(stderr, "p4pc: mod %s: interpreter safepoint budget exhausted\n", mod->info->id);
        record_error(mod, "Interpreter safepoint budget exhausted");
        lua_settop(L, base);
        status = LUA_ERRRUN;
    } else if (status != LUA_OK) {
        const char *message = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : "non-string Luau error";
        fprintf(stderr, "p4pc: mod %s: %s\n", mod->info->id, message);
        record_error(mod, message);
        lua_pop(L, 1);
    }
    thread->frame = frame.previous;
    --thread->depth;
    thread->executing = was_executing;
    if (was_executing) {
        if (frame.previous != NULL) frame.previous->native_env = frame.native_env;
        fesetenv(&caller_environment);
    } else fesetenv(&frame.native_env);
    return status == LUA_OK ? 0 : -1;
}
void p4_lua_require_mutation(lua_State *L)
{
    if(p4_lua_frame(L)->snapshot)
        luaL_error(L,"native or guest mutation is forbidden during a save snapshot");
}


void p4_lua_native_enter(lua_State *L, fenv_t *script_environment)
{
    P4LuaMod *mod = p4_luau_user(L);
    P4LuaFrame *frame = p4_lua_frame(L);
    if (frame->no_native_calls)
        luaL_error(L, "native game calls/continuations are forbidden in a synchronous action callback");
    p4_lua_semantic_use(mod);
    P4LuaThread *thread = current_thread(mod);
    fegetenv(script_environment);
    thread->executing = 0;
    fesetenv(&frame->native_env);
}

void p4_lua_native_leave(lua_State *L, const fenv_t *script_environment)
{
    P4LuaMod *mod = p4_luau_user(L);
    P4LuaFrame *frame = p4_lua_frame(L);
    P4LuaThread *thread = current_thread(mod);
    fegetenv(&frame->native_env);
    thread->executing = 1;
    fesetenv(script_environment);
}

static int invoke(lua_State *L)
{
    P4LuaInvocation *invocation = lua_touserdata(L, 1);
    lua_settop(L, 0);
    lua_pushlightuserdata(L, invocation->data);
    return invocation->function(L);
}

int p4_lua_invoke(P4LuaMod *mod, lua_CFunction function, void *data, int results, int flags)
{
    mod->error[0] = 0;
    P4LuaThread *thread = p4_lua_thread(mod);
    if (thread == NULL || !lua_checkstack(thread->state, 8))
        return record_error(mod, "Unable to prepare interpreter native-coroutine stack");
    P4LuaInvocation invocation = {function, data};
    lua_getref(thread->state, mod->invoke_reference);
    lua_pushlightuserdata(thread->state, &invocation);
    return p4_lua_protected(mod, thread->state, 1, results, flags);
}

static int log_message(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    size_t size;
    const char *message = luaL_checklstring(L, 1, &size);
    p4_lua_require_mutation(L);
    if (size > 4096) luaL_error(L, "mod log message exceeds 4096 bytes");
    fprintf(stderr, "p4pc: mod %s: %.*s\n", mod->info->id, (int)size, message);
    return 0;
}
static int wide_value(lua_State *L)
{
    uint32_t high = (uint32_t)p4_lua_integer(L, 1, 0, UINT32_MAX);
    uint32_t low = (uint32_t)p4_lua_integer(L, 2, 0, UINT32_MAX);
    p4_lua_push_wide(L, (uint64_t)high << 32 | low);
    return 1;
}
static int split_wide(lua_State *L)
{
    uint64_t value = p4_lua_wide(L, 1);
    lua_pushunsigned(L, (uint32_t)(value >> 32));
    lua_pushunsigned(L, (uint32_t)value);
    return 2;
}

static void initialize(lua_State *L, void *user)
{
    P4LuaMod *mod = user;
    mod->state = L;
    mod->main_reference = LUA_NOREF;
    lua_pushcfunction(L, invoke, "p4.private.invoke");
    mod->invoke_reference = lua_ref(L, -1);
    lua_pop(L, 1);
    lua_newtable(L);
    char version[32];
    snprintf(version, sizeof version, "%d.%d", P4_PACKAGE_API_MAJOR, P4_PACKAGE_API_MINOR);
    lua_pushstring(L, version); lua_setfield(L, -2, "api_version");
    lua_pushliteral(L, "Luau"); lua_setfield(L, -2, "language");
    lua_pushliteral(L, "0.744"); lua_setfield(L, -2, "language_version");
    lua_pushstring(L, mod->info->id); lua_setfield(L, -2, "id");
    lua_pushcfunction(L, log_message, "p4.log"); lua_setfield(L, -2, "log");
    lua_pushcfunction(L, wide_value, "p4.u64"); lua_setfield(L, -2, "u64");
    lua_pushcfunction(L, split_wide, "p4.split64"); lua_setfield(L, -2, "split64");
    p4_lua_memory(L); lua_setfield(L, -2, "mem");
    p4_lua_events(L); lua_setfield(L, -2, "events");
    p4_lua_input(L); lua_setfield(L, -2, "input");
    p4_lua_files(L); lua_setfield(L, -2, "files");
    p4_lua_save_library(L); lua_setfield(L, -2, "save");
    if (mod->native_bindings) mod->native_bindings->initialize(L);
    lua_setglobal(L, "p4");
    p4_lua_hooks(mod);
}

static int install_layout(lua_State *L)
{
    P4LuaMod *mod = lua_touserdata(L, 1);
    lua_xmove(mod->state, L, 1);
    for (unsigned i = 0; i < P4_LUA_LAYOUT_HELPERS; ++i) lua_getref(L, mod->layout_helpers[i]);
    lua_call(L, P4_LUA_LAYOUT_HELPERS, 1);
    luaL_checktype(L, -1, LUA_TFUNCTION);
    mod->layout_factory_reference = lua_ref(L, -1);
    for (unsigned i = 0; i < P4_LUA_LAYOUT_HELPERS; ++i) {
        lua_unref(L, mod->layout_helpers[i]);
        mod->layout_helpers[i] = LUA_NOREF;
    }
    return 0;
}

P4LuaMod *p4_lua_create(const P4PackageProvider *provider)
{
    const P4PackageInfo *info = p4_package_provider_info(provider);
    if (!info || info->kind != P4_PACKAGE_MOD ||
        p4_packages_provider(info->ordinal) != provider ||
        !(p4_package_provider_claims(provider) & P4_PACKAGE_CLAIM_SCRIPT))
        return NULL;
    if (p4_package_provider_retain(provider)) return NULL;
    P4LuaMod *mod = calloc(1, sizeof *mod);
    if (mod == NULL) { p4_package_provider_release(provider); return NULL; }
    mod->info = info;
    mod->owner = info->ordinal;
    mod->provider = provider;
    mod->native_bindings = native_bindings;
    mod->layout_factory_reference = LUA_NOREF;
    for (size_t i = 0; i < sizeof mod->threads / sizeof mod->threads[0]; ++i)
        mod->threads[i].reference = LUA_NOREF;
#ifdef P4_GUEST64
    if (p4_guest_init() != 0) goto fail;
#endif
    p4_luau_config config = {
        .memory_limit = LUA_MEMORY_LIMIT, .interrupt = interrupt, .user = mod,
        .initialize = initialize, .source_limit = 1024u * 1024u
    };
    fenv_t native_environment;
    fegetenv(&native_environment);
    fesetenv(FE_DFL_ENV);
    mod->vm = p4_luau_create(&config);
    fesetenv(&native_environment);
    if (mod->vm == NULL) {
        fprintf(stderr, "p4pc: mod %s: cannot create bounded Luau VM\n", info->id);
        goto fail;
    }
    fegetenv(&native_environment);
    fesetenv(FE_DFL_ENV);
    int status = p4_luau_load_source(mod->vm, "@p4/layout", p4_mod_layout_api, sizeof p4_mod_layout_api - 1);
    fesetenv(&native_environment);
    if (status != LUA_OK || p4_lua_invoke(mod, install_layout, mod, 0, 1) != 0) {
        fprintf(stderr, "p4pc: mod %s: layout API initialization failed: %s\n", info->id, p4_lua_error(mod));
        p4_lua_destroy(mod);
        return NULL;
    }
    return mod;
    fail:
    p4_package_provider_release(mod->provider);
    free(mod);
    return NULL;
}

static void require_callbacks_idle(void)
{
    if (p4_mod_hooks_quiescent() && p4_mod_events_quiescent()) return;
    fprintf(stderr, "p4pc: mods: VM teardown requires idle native hook/event callbacks\n");
    fflush(stderr);
    _Exit(EXIT_FAILURE);
}
static void require_vm_idle(const P4LuaMod *mod)
{
    if (mod == NULL) return;
    for (size_t i = 0; i < sizeof mod->threads / sizeof mod->threads[0]; ++i) {
        if (mod->threads[i].frame == NULL) continue;
        fprintf(stderr, "p4pc: mod %s: VM teardown while a native context is active\n", mod->info->id);
        fflush(stderr);
        _Exit(EXIT_FAILURE);
    }
}
static void destroy_idle_vm(P4LuaMod *mod)
{
    if (mod == NULL) return;
    p4_lua_save_destroy(mod);
    if (mod->vm && mod->native_bindings) mod->native_bindings->before_destroy(mod);
    p4_mod_events_remove_owner(mod->owner);
    p4_mod_hooks_remove_owner(mod->owner);
    p4_luau_destroy(mod->vm);
    p4_package_provider_release(mod->provider);
    free(mod);
}
void p4_lua_destroy(P4LuaMod *mod)
{
    if (mod == NULL) return;
    require_callbacks_idle();
    require_vm_idle(mod);
    destroy_idle_vm(mod);
}

static int retain_source(lua_State *L)
{
    P4LuaMod *mod = lua_touserdata(L, 1);
    lua_xmove(mod->state, L, 1);
    mod->main_reference = lua_ref(L, -1);
    return 0;
}
static int execute_source(lua_State *L)
{
    P4LuaMod *mod = lua_touserdata(L, 1);
    lua_getref(L, mod->main_reference);
    lua_call(L, 0, 0);
    return 0;
}
int p4_lua_compile(P4LuaMod *mod, const char *source, size_t size, const char *name)
{
    /* Compile/load only: neither a mod chunk nor a native game callback runs. */
    for (size_t i = 0; i < sizeof mod->threads / sizeof mod->threads[0]; ++i)
        if (mod->threads[i].frame != NULL) return record_error(mod, "Cannot compile while this mod has active calls");
    if (mod->main_reference != LUA_NOREF) lua_unref(mod->state, mod->main_reference);
    mod->main_reference = LUA_NOREF;
    mod->error[0] = 0;
    mod->budget_failed = mod->has_budget_frame = 0;
    mod->frame_fuel = 0;
    p4_luau_reset_interrupt(mod->vm);
    fenv_t native_environment;
    fegetenv(&native_environment);
    fesetenv(FE_DFL_ENV);
    int status = p4_luau_load_source(mod->vm, name, source, size);
    fesetenv(&native_environment);
    if (status != LUA_OK) return record_error(mod, p4_luau_error(mod->vm));
    return p4_lua_invoke(mod, retain_source, mod, 0, 1);
}
int p4_lua_execute(P4LuaMod *mod)
{
    if (mod->main_reference == LUA_NOREF) return record_error(mod, "No compiled main.luau retained");
    return p4_lua_invoke(mod, execute_source, mod, 0, 1);
}

int p4_mod_scripts_prepare(const P4PackageRegistry *registry, void *user)
{
    (void)user;
    if (script_phase >= 2) return -1;
    script_phase = 1;
    size_t admitted = p4_packages_count();
    /* New candidates share the exact immutable source backing. Rebind retained
     * compiled VMs without reading or compiling their source a second time. */
    for (size_t i = 0; i < P4_PACKAGE_MAX; ++i) {
        P4LuaMod *mod = active[i];
        if (!mod) continue;
        if (p4_packages_provider(mod->info->ordinal) == mod->provider) continue;
        const P4PackageProvider *replacement = NULL;
        for (size_t ordinal = 0; ordinal < admitted; ++ordinal) {
            const P4PackageProvider *candidate = p4_packages_provider(ordinal);
            if (p4_package_provider_same_source(mod->provider, candidate)) { replacement = candidate; break; }
        }
        if (!replacement) {
            p4_lua_destroy(mod);
            active[i] = NULL;
            continue;
        }
        if (p4_package_provider_retain(replacement)) return -1;
        const P4PackageProvider *previous = mod->provider;
        mod->provider = replacement;
        mod->info = p4_package_provider_info(replacement);
        mod->owner = mod->info->ordinal;
        p4_package_provider_release(previous);
    }
    for (size_t i = 0; i < p4_package_registry_count(registry); ++i) {
        const P4PackageProvider *provider = p4_package_registry_provider(registry, i);
        if (!(p4_package_provider_claims(provider) & P4_PACKAGE_CLAIM_SCRIPT)) continue;
        const P4PackageInfo *info = p4_package_provider_info(provider);
        size_t slot = 0, existing = 0;
        for (; existing < P4_PACKAGE_MAX; ++existing)
            if (active[existing] && active[existing]->provider == provider) break;
        if (existing != P4_PACKAGE_MAX) continue;
        while (slot < P4_PACKAGE_MAX && active[slot]) ++slot;
        const char *reason = "Unable to prepare bounded script state";
        if (slot == P4_PACKAGE_MAX) {
            if (p4_package_provider_refuse(provider, "Too many scripting mods")) return -1;
            continue;
        }
        P4PackageFile *file = NULL;
        int opened = p4_package_file_open(provider, "main.luau", 1024u * 1024u, &file);
        if (opened) {
            if (p4_package_provider_refuse(provider, "main.luau is missing, unreadable, nonregular or over one MiB")) return -1;
            continue;
        }
        size_t size = (size_t)p4_package_file_size(file);
        char empty = 0;
        char *source = size ? malloc(size) : &empty;
        int read = source ? p4_package_file_read_at(file, 0, source, size) : -1;
        int closed = p4_package_file_close(file);
        if (read || closed) {
            if (size) free(source);
            if (p4_package_provider_refuse(provider, "Unable to read or close bounded script source")) return -1;
            continue;
        }
        P4LuaMod *mod = p4_lua_create(provider);
        char name[80];
        snprintf(name, sizeof name, "@%s/main.luau", info->id);
        int status = mod ? p4_lua_compile(mod, source, size, name) : -1;
        if (size) free(source);
        if (!status) { active[slot] = mod; continue; }
        if (mod && p4_lua_error(mod)[0]) reason = p4_lua_error(mod);
        int refused = p4_package_provider_refuse(provider, reason);
        p4_lua_destroy(mod); /* No mod code has run, hence no queued game work. */
        if (refused) return -1;
    }
    return 0;
}
void p4_mod_scripts_start(void)
{
    if (script_phase == 3) return;
    if (script_phase == 2) p4_mod_failure(P4_MOD_NATIVE_OWNER, "Recursive mod initialization is unsafe");
    script_phase = 2;
    /* Pass1 inserted scripts in immutable global priority order; subsequent
     * admission passes only remove providers and retain that relative order. */
    for (size_t i = 0; i < P4_PACKAGE_MAX; ++i) {
        P4LuaMod *mod = active[i];
        if (mod && p4_lua_execute(mod))
            p4_mod_failure(mod->owner, p4_lua_error(mod));
    }
    script_phase = 3;
}
void p4_mod_scripts_discard_preflight(void *user)
{
    (void)user;
    p4_mod_scripts_stop();
}

void p4_mod_scripts_stop(void)
{
    require_callbacks_idle();
    for (size_t i = 0; i < P4_PACKAGE_MAX; ++i) require_vm_idle(active[i]);
    p4_mod_input_enabled = 0;
    for (size_t i = 0; i < P4_PACKAGE_MAX; ++i) {
        destroy_idle_vm(active[i]);
        active[i] = NULL;
    }
    /* VM userdata finalizers still reference tombstone handles until now. */
    p4_mod_events_clear();
    p4_mod_hooks_clear();
    p4_mod_memory_clear();
    script_phase = 0;
}
