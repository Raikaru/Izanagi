#include "p4pc/luau_c.h"
#include "luacode.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <new>
#if defined(_WIN32) && !defined(_WIN64)
#include <malloc.h>
#endif

struct p4_luau {
    lua_State *state;
    p4_luau_config config;
    size_t used;
    bool interrupted;
    char error[2048];
};

static void record_error(p4_luau *vm, const char *message)
{
    if (!message) message = "Luau error (non-string error value)";
    size_t n = 0;
    while (n + 1 < sizeof(vm->error) && message[n]) ++n;
    std::memcpy(vm->error, message, n);
    vm->error[n] = 0;
}

static const char *error_message(lua_State *L)
{
    /* lua_tolstring can allocate for numbers: never coerce an arbitrary error
     * value after leaving the protected VM frame, especially at the cap. */
    return lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : nullptr;
}

static void *allocate(void *ud, void *ptr, size_t oldsize, size_t size)
{
    p4_luau *vm = static_cast<p4_luau *>(ud);
    /* Luau passes a type tag as oldsize on a fresh allocation. */
    if (!ptr) oldsize = 0;
    if (!size) {
#if defined(_WIN32) && !defined(_WIN64)
        _aligned_free(ptr);
#else
        std::free(ptr);
#endif
        vm->used -= oldsize;
        return nullptr;
    }
    size_t retained = vm->used - oldsize;
    if (size > vm->config.memory_limit - retained) return nullptr;
#if defined(_WIN32) && !defined(_WIN64)
    /* global_State::ecbdata is alignas(16). Win32's ordinary CRT malloc/realloc
     * guarantee only 8 bytes; all other supported profile allocators give 16. */
    void *result = _aligned_realloc(ptr, size, 16);
#else
    void *result = std::realloc(ptr, size);
#endif
    if (result) vm->used = retained + size;
    return result;
}

static p4_luau *owner(lua_State *L)
{
    void *ud = nullptr;
    (void)lua_getallocf(L, &ud);
    return static_cast<p4_luau *>(ud);
}

static void interrupt(lua_State *L, int gc)
{
    p4_luau *vm = owner(L);
    /* Upstream uses -1 for execution and nonnegative GC phase numbers. */
    int collecting = gc >= 0;
    if (vm->config.interrupt && vm->config.interrupt(vm->config.user, collecting))
        vm->interrupted = true;
    /* Collector callbacks may not raise errors. The next execution safepoint
     * observes the latch, even if a nested pcall caught the original error. */
    if (vm->interrupted && !collecting) luaL_error(L, "Execution budget exhausted");
}

static int initialize(lua_State *L)
{
    static const luaL_Reg libraries[] = {
        {"", luaopen_base}, {LUA_COLIBNAME, luaopen_coroutine},
        {LUA_TABLIBNAME, luaopen_table}, {LUA_STRLIBNAME, luaopen_string},
        {LUA_MATHLIBNAME, luaopen_math}, {LUA_UTF8LIBNAME, luaopen_utf8},
        {LUA_BITLIBNAME, luaopen_bit32}, {LUA_BUFFERLIBNAME, luaopen_buffer}
    };
    for (const luaL_Reg &lib : libraries) {
        lua_pushcfunction(L, lib.func, nullptr);
        lua_pushstring(L, lib.name);
        lua_call(L, 1, 0);
    }
    p4_luau *vm = owner(L);
    if (vm->config.initialize) vm->config.initialize(L, vm->config.user);
    luaL_sandbox(L);
    luaL_sandboxthread(L);
    /* Host globals may expose mutable metatable-backed views. The thread
     * environment is reused for multiple chunks, so GETIMPORT must not cache
     * values from those views, even during the first chunk. This changes only
     * the optimization flag; luaL_sandbox's readonly tables remain frozen. */
    lua_setsafeenv(L, LUA_GLOBALSINDEX, false);
    return 0;
}

extern "C" p4_luau *p4_luau_create(const p4_luau_config *config)
{
    if (!config || !config->memory_limit) return nullptr;
    p4_luau *vm = static_cast<p4_luau *>(std::calloc(1, sizeof(p4_luau)));
    if (!vm) return nullptr;
    vm->config = *config;
    if (!vm->config.source_limit) vm->config.source_limit = 1024 * 1024;
    vm->state = lua_newstate(allocate, vm);
    if (!vm->state) { std::free(vm); return nullptr; }
    if (lua_cpcall(vm->state, initialize, nullptr) != LUA_OK) {
        lua_close(vm->state);
        std::free(vm);
        return nullptr;
    }
    lua_callbacks(vm->state)->interrupt = interrupt;
    return vm;
}

extern "C" void p4_luau_destroy(p4_luau *vm)
{
    if (vm) { lua_close(vm->state); std::free(vm); }
}
extern "C" lua_State *p4_luau_state(p4_luau *vm) { return vm->state; }
extern "C" void *p4_luau_user(lua_State *L) { return owner(L)->config.user; }
extern "C" size_t p4_luau_memory_used(const p4_luau *vm) { return vm->used; }
extern "C" void p4_luau_reset_interrupt(p4_luau *vm) { vm->interrupted = false; }
extern "C" const char *p4_luau_error(const p4_luau *vm) { return vm->error; }

struct Load {
    const char *name;
    const char *bytes;
    size_t size;
    int ref;
    int status;
    p4_luau *vm;
};
static int load(lua_State *L)
{
    Load *ctx = static_cast<Load *>(lua_touserdata(L, 1));
    ctx->status = luau_load(L, ctx->name, ctx->bytes, ctx->size, 0);
    if (ctx->status != LUA_OK) {
        record_error(ctx->vm, error_message(L));
        /* Upstream luau_load returns 1, not a lua_Status. An internally compiled
         * error blob starts with zero; valid compiler output can only fail OOM. */
        ctx->status = ctx->size && ctx->bytes[0] == 0 ? LUA_ERRSYNTAX : LUA_ERRMEM;
    } else ctx->ref = lua_ref(L, -1);
    return 0;
}

static bool valid_source(p4_luau *vm, const char *name, const char *source, size_t size)
{
    vm->error[0] = 0;
    if (!name || (!source && size) || size > vm->config.source_limit) {
        record_error(vm, "invalid source arguments or source exceeds limit");
        return false;
    }
    return true;
}

/* This helper completes ALL C++ exception/RAII work before touching Lua. */
static char *compile(p4_luau *vm, const char *source, size_t size, size_t *outsize)
{
    try {
        return luau_compile(source, size, nullptr, outsize);
    } catch (const std::bad_alloc &) {
        record_error(vm, "compiler out of memory");
    } catch (const std::exception &error) {
        record_error(vm, error.what());
    } catch (...) {
        record_error(vm, "unknown compiler exception");
    }
    return nullptr;
}

extern "C" int p4_luau_check_source(p4_luau *vm, const char *name, const char *source, size_t size)
{
    if (!valid_source(vm, name, source, size)) return LUA_ERRSYNTAX;
    size_t byte_count = 0;
    char *bytes = compile(vm, source ? source : "", size, &byte_count);
    if (!bytes) {
        if (!vm->error[0]) record_error(vm, "compiler out of memory");
        return LUA_ERRMEM;
    }
    int status = LUA_OK;
    if (byte_count && bytes[0] == 0) {
        /* Compiler errors are a zero byte followed by a length-delimited
         * ":line: message", not a NUL-terminated string. Never load the blob. */
        const char *chunk = *name == '@' || *name == '=' ? name + 1 : name;
        size_t chunk_size = std::strlen(chunk);
        const size_t chunk_limit = sizeof(vm->error) / 2;
        if (chunk_size > chunk_limit) chunk += chunk_size - chunk_limit;
        size_t message_size = byte_count - 1;
        if (message_size >= sizeof(vm->error)) message_size = sizeof(vm->error) - 1;
        std::snprintf(vm->error, sizeof(vm->error), "%s%.*s", chunk,
                      static_cast<int>(message_size), bytes + 1);
        status = LUA_ERRSYNTAX;
    }
    std::free(bytes);
    return status;
}

extern "C" int p4_luau_load_source(p4_luau *vm, const char *name, const char *source, size_t size)
{
    if (!valid_source(vm, name, source, size)) return LUA_ERRSYNTAX;
    size_t byte_count = 0;
    char *bytes = compile(vm, source ? source : "", size, &byte_count);
    if (!bytes) {
        if (!vm->error[0]) record_error(vm, "compiler out of memory");
        return LUA_ERRMEM;
    }
    Load ctx = {name, bytes, byte_count, LUA_NOREF, LUA_OK, vm};
    int top = lua_gettop(vm->state);
    int status = lua_cpcall(vm->state, load, &ctx);
    std::free(bytes);
    if (status != LUA_OK) {
        record_error(vm, error_message(vm->state));
        lua_settop(vm->state, top);
        return status;
    }
    if (ctx.status != LUA_OK) return ctx.status;
    lua_getref(vm->state, ctx.ref);
    lua_unref(vm->state, ctx.ref);
    return LUA_OK;
}

extern "C" int p4_luau_run(p4_luau *vm, int nargs, int nresults)
{
    vm->error[0] = 0;
    int base = lua_gettop(vm->state) - nargs - 1;
    int status = lua_pcall(vm->state, nargs, nresults, 0);
    if (vm->interrupted) {
        record_error(vm, status != LUA_OK ? error_message(vm->state) : "Execution budget exhausted");
        lua_settop(vm->state, base);
        return LUA_ERRRUN;
    }
    if (status != LUA_OK) {
        record_error(vm, error_message(vm->state));
        lua_pop(vm->state, 1);
    }
    return status;
}
