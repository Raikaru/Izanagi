// C ABI regression for the optional buffer cage; also runnable against Luau 0.744.
// The test allocator is deliberately separate from the state's ordinary allocator.
#include "lua.h"
#include "lualib.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef LUAU_ZIG_CANDIDATE
extern "C" int luau_setfflag(const char*, int);
static bool set_cage_flag(bool enabled) { return luau_setfflag("LuauBufferCage", enabled) != 0; }
#else
#include "Luau/Common.h"
LUAU_FASTFLAG(LuauBufferCage)
static bool set_cage_flag(bool enabled) { FFlag::LuauBufferCage.value = enabled; return true; }
#endif

struct Cage {
    struct Block { void* ptr; size_t size; } blocks[64] = {};
    size_t live = 0;
    unsigned attempts = 0;
    bool fail = false;
    bool invalid = false;
};

static void* cage_alloc(void* ud, void* ptr, size_t oldsize, size_t size, int type)
{
    Cage& cage = *static_cast<Cage*>(ud);
    if (type != LUA_TBUFFER) cage.invalid = true;
    size_t slot = 0;
    if (ptr) {
        while (slot < 64 && cage.blocks[slot].ptr != ptr) ++slot;
        if (slot == 64) { cage.invalid = true; return nullptr; }
        if (cage.blocks[slot].size != oldsize) cage.invalid = true;
    } else {
        if (oldsize) cage.invalid = true;
        while (slot < 64 && cage.blocks[slot].ptr) ++slot;
        if (slot == 64) { cage.invalid = true; return nullptr; }
    }
    if (!size) {
        if (ptr) { cage.live -= cage.blocks[slot].size; cage.blocks[slot] = {}; std::free(ptr); }
        return nullptr;
    }
    ++cage.attempts;
    if (cage.fail) return nullptr;
    void* result = std::realloc(ptr, size);
    if (!result) return nullptr;
    if (ptr) cage.live -= cage.blocks[slot].size;
    cage.blocks[slot] = {result, size};
    cage.live += size;
    return result;
}

static int make_buffer(lua_State* L)
{
    auto* bytes = static_cast<unsigned char*>(lua_newbuffer(L, 65536));
    bytes[0] = 0x35;
    bytes[65535] = 0xa9;
    return 1;
}

static bool run_case(bool enabled)
{
    if (!set_cage_flag(enabled)) return false;
    Cage cage;
    lua_State* L = luaL_newstate();
    if (!L) return false;
    lua_setmemorycage(L, cage_alloc, &cage);
    bool ok = true;
    if (enabled) {
        cage.fail = true;
        lua_pushcfunction(L, make_buffer, "make_buffer");
        const int status = lua_pcall(L, 0, 1, 0);
        ok = status == LUA_ERRMEM && cage.attempts > 0 && cage.live == 0 && !cage.invalid;
        lua_settop(L, 0);
        cage.fail = false;
    }
    const size_t before = lua_totalbytes(L, -1);
    lua_pushcfunction(L, make_buffer, "make_buffer");
    const int status = lua_pcall(L, 0, 1, 0);
    if (status != LUA_OK) ok = false;
    if (status == LUA_OK) {
        size_t length = 0;
        auto* bytes = static_cast<unsigned char*>(lua_tobuffer(L, -1, &length));
        if (!bytes || length != 65536 || bytes[0] != 0x35 || bytes[65535] != 0xa9 ||
            lua_totalbytes(L, -1) < before + length) ok = false;
    }
    if (enabled ? (cage.live == 0 || cage.invalid) : (cage.attempts != 0 || cage.live != 0)) ok = false;
    lua_settop(L, 0);
    lua_gc(L, LUA_GCCOLLECT, 0);
    if (cage.live != 0 || cage.invalid) ok = false;
    lua_close(L);
    if (cage.live != 0 || cage.invalid) ok = false;
    return ok;
}

int main()
{
    const bool on = run_case(true);
    const bool off = run_case(false);
    set_cage_flag(false);
    if (!on || !off) { std::fprintf(stderr, "buffer cage boundary regression failed (on=%d off=%d)\n", on, off); return 1; }
    std::puts("buffer cage boundary regression passed");
    return 0;
}
