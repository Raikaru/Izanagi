#ifndef P4PC_LUAU_C_H
#define P4PC_LUAU_C_H

#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "lua.h"
#include "lualib.h"

typedef struct p4_luau p4_luau;
/* At VM safepoints, including GC; nonzero latches budget exhaustion until the
 * host resets it. Must not throw, allocate through Lua, or reenter the state. */
typedef int (*p4_luau_interrupt)(void *user, int gc);
/* Register host functions before luaL_sandbox freezes libraries/globals. Runs
 * protected; C callbacks must not have C++ destructors across a Lua longjmp. */
typedef void (*p4_luau_initialize)(lua_State *state, void *user);
typedef struct p4_luau_config {
    size_t memory_limit; /* Hard VM allocator cap, including state/libraries. */
    p4_luau_interrupt interrupt;
    void *user;
    p4_luau_initialize initialize;
    size_t source_limit; /* Zero selects 1 MiB. Compiler scratch is host memory. */
} p4_luau_config;

/* NULL on initialization failure (including too-small memory cap). Opens only
 * base/coroutine/table/string/math/utf8/bit32/buffer, then uses Luau's upstream
 * sandbox + sandboxthread. Independent states; no OS/io/debug/require/loaders.
 * Normal Luau pcall, coroutine, environment and metatable semantics remain. */
p4_luau *p4_luau_create(const p4_luau_config *config);
void p4_luau_destroy(p4_luau *vm);
lua_State *p4_luau_state(p4_luau *vm);
/* Works on all coroutines created from this VM, without callback userdata. */
void *p4_luau_user(lua_State *state);
size_t p4_luau_memory_used(const p4_luau *vm);
void p4_luau_reset_interrupt(p4_luau *vm);

/* Compile SOURCE and discard private bytecode, without loading or running it.
 * The source limit applies; Lua stack/heap and the interrupt latch are unchanged.
 * Returns LUA_OK or LUA_ERR* with p4_luau_error diagnostics. name should be @path.
 * This checks syntax/compiler rules, not types; Analysis remains a tooling CLI. */
int p4_luau_check_source(p4_luau *vm, const char *name, const char *source, size_t size);

/* Compile SOURCE in-process, then load. No boundary API accepts bytecode.
 * Success pushes one function; failure restores the previous stack. Returns
 * LUA_OK or LUA_ERR*. name should be @path for source/line diagnostics. */
int p4_luau_load_source(p4_luau *vm, const char *name, const char *source, size_t size);
/* Standard lua_pcall stack contract, but failure pops/records the error. Sticky
 * budget exhaustion cannot be hidden from the host by a script-side pcall. */
int p4_luau_run(p4_luau *vm, int nargs, int nresults);
/* Last operation's diagnostic with source/line where available; empty on
 * success. Owned by vm until next check/load/run; at most 2047 bytes. */
const char *p4_luau_error(const p4_luau *vm);

#ifdef __cplusplus
}
#endif
#endif
