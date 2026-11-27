/* Shared helpers for the p4_luau sandbox behavioral witnesses.
 *
 * Every witness is a plain C program that links one implementation of the
 * production contract in include/p4pc/luau_c.h (plus the lua.h/lualib.h API it
 * re-exports): either the authoritative C++ bridge (src/platform/mods/luau_c.cpp
 * over vendored Luau 0.744) or a Zig candidate archive. The same source is the
 * gate for both; nothing here models or mocks the VM.
 *
 * Exit status: 0 all checks passed, 1 a check failed, 77 the witness cannot
 * observe the property on this platform (reported as SKIP, never as PASS). */
#ifndef P4PC_LUAU_SANDBOX_WITNESS_H
#define P4PC_LUAU_SANDBOX_WITNESS_H

#include "p4pc/luau_c.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WITNESS_SKIP 77

/* Returns 1 from the enclosing int function; main propagates it. */
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #condition); \
    return 1; } } while (0)
/* As CHECK, but also prints the VM's last diagnostic for context. */
#define CHECK_VM(vm, condition) do { if (!(condition)) { \
    fprintf(stderr, "%s:%d: CHECK failed: %s\n  p4_luau_error: %s\n", __FILE__, __LINE__, \
        #condition, p4_luau_error(vm)); \
    return 1; } } while (0)
#define SECTION(call) do { if ((call) != 0) { \
    fprintf(stderr, "section failed: %s\n", #call); return 1; } } while (0)

/* Production admission path: compile+load a named chunk, then p4_luau_run. */
static inline int witness_run(p4_luau *vm, const char *name, const char *source, int nresults)
{
    int status = p4_luau_load_source(vm, name, source, strlen(source));
    if (status == LUA_OK) status = p4_luau_run(vm, 0, nresults);
    return status;
}

static inline int witness_error_has(const p4_luau *vm, const char *needle)
{
    return strstr(p4_luau_error(vm), needle) != NULL;
}

/* Full collection from the host, as the production mod runtime does with
 * lua_gc outside any Lua frame. Upstream GC resizes are internally protected. */
static inline size_t witness_collect(p4_luau *vm)
{
    lua_gc(p4_luau_state(vm), LUA_GCCOLLECT, 0);
    return p4_luau_memory_used(vm);
}

/* The production mod runtime's pattern (lua_runtime.c create_thread): allocate
 * a host-owned coroutine under lua_cpcall and pin it with lua_ref. */
struct thread_create { lua_State *state; int reference; };
static inline int witness_create_thread(lua_State *L)
{
    struct thread_create *created = (struct thread_create *)lua_touserdata(L, 1);
    created->state = lua_newthread(L);
    created->reference = lua_ref(L, -1);
    return 0;
}

#endif
