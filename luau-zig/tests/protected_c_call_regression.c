/* Optimized Zig VM C-ABI regression: state creation, protected callbacks,
 * non-local error unwinding and destruction. No upstream VM is linked. */
#include "lua.h"
#include <stdio.h>
#include <stdlib.h>

struct context {
    size_t live_allocations;
    int allocator_error;
    int successes;
    int failures;
    int nested;
    int error_marker;
};

static void *allocate(void *user, void *ptr, size_t old_size, size_t new_size)
{
    struct context *ctx = user;
    (void)old_size; /* Fresh Luau allocations pass a type tag, not a size. */
    if (!new_size) {
        if (ptr) {
            if (!ctx->live_allocations) ctx->allocator_error = 1;
            else --ctx->live_allocations;
            free(ptr);
        }
        return NULL;
    }
    int fresh = ptr == NULL;
    void *result = realloc(ptr, new_size);
    if (result && fresh) ++ctx->live_allocations;
    return result;
}

static int success_callback(lua_State *L)
{
    struct context *ctx = lua_touserdata(L, 1);
    if (!ctx) {
        lua_pushstring(L, "missing protected callback userdata");
        lua_error(L);
    }
    ++ctx->successes;
    lua_pushinteger(L, 42); /* lua_cpcall discards callback results. */
    return 1;
}

static int error_callback(lua_State *L)
{
    struct context *ctx = lua_touserdata(L, 1);
    if (!ctx) {
        lua_pushstring(L, "missing protected callback userdata");
        lua_error(L);
    }
    ++ctx->failures;
    lua_pushlightuserdata(L, &ctx->error_marker);
    lua_error(L);
    return 0;
}

static int nested_callback(lua_State *L)
{
    struct context *ctx = lua_touserdata(L, 1);
    if (!ctx) {
        lua_pushstring(L, "missing nested callback userdata");
        lua_error(L);
    }
    int status = lua_cpcall(L, error_callback, ctx);
    if (status != LUA_ERRRUN || lua_gettop(L) != 2 ||
        lua_type(L, -1) != LUA_TLIGHTUSERDATA ||
        lua_touserdata(L, -1) != &ctx->error_marker) {
        lua_pushstring(L, "nested protected call lost its error value");
        lua_error(L);
    }
    lua_pop(L, 1);
    ++ctx->nested;
    return 0;
}

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "protected_c_call_regression:%d: %s\n", __LINE__, #expr); \
    failed = 1; goto cleanup; } } while (0)

int main(void)
{
    int failed = 0;
    struct context ctx = {0};
    lua_State *L = lua_newstate(allocate, &ctx);
    CHECK(L != NULL);
    CHECK(lua_gettop(L) == 0);
    CHECK(ctx.live_allocations > 0 && !ctx.allocator_error);

    /* The pre-existing caller stack must survive both success and longjmp. */
    lua_pushinteger(L, 73);
    CHECK(lua_cpcall(L, success_callback, &ctx) == LUA_OK);
    CHECK(ctx.successes == 1 && lua_gettop(L) == 1);
    CHECK(lua_type(L, 1) == LUA_TNUMBER && lua_tointeger(L, 1) == 73);

    CHECK(lua_cpcall(L, error_callback, &ctx) == LUA_ERRRUN);
    CHECK(ctx.failures == 1 && lua_gettop(L) == 2);
    CHECK(lua_type(L, -1) == LUA_TLIGHTUSERDATA);
    CHECK(lua_touserdata(L, -1) == &ctx.error_marker);
    CHECK(lua_tointeger(L, 1) == 73);
    lua_pop(L, 1);

    /* A nested error must unwind only its own protected frame. */
    CHECK(lua_cpcall(L, nested_callback, &ctx) == LUA_OK);
    CHECK(ctx.failures == 2 && ctx.nested == 1 && lua_gettop(L) == 1);
    CHECK(lua_tointeger(L, 1) == 73);
    CHECK(lua_cpcall(L, success_callback, &ctx) == LUA_OK);
    CHECK(ctx.successes == 2 && lua_gettop(L) == 1);
    CHECK(lua_tointeger(L, 1) == 73);

cleanup:
    if (L) lua_close(L);
    if (ctx.allocator_error || ctx.live_allocations != 0) {
        fprintf(stderr, "protected_c_call_regression: allocator imbalance: %zu live, invalid free=%d\n",
                ctx.live_allocations, ctx.allocator_error);
        failed = 1;
    }
    if (!failed) puts("protected C calls: state, normal return, exact error, nested unwind, close: PASS");
    return failed;
}
