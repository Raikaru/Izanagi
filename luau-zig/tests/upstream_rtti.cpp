// Test-only RTTI input for types.luau, built against the pinned upstream Analysis.
// populateRTTI is extracted unchanged from the pinned conformance source.
#include "Luau/Common.h"
#include "Luau/Type.h"
#include "Luau/BuiltinDefinitions.h"
#include "Luau/ModuleResolver.h"
#include "Luau/TypeInfer.h"
#include "Luau/Frontend.h"
#include "lua.h"
#include "lualib.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>

LUAU_FASTFLAG(LuauIntegerType2)
LUAU_FASTFLAG(DebugLuauForceOldSolver)
LUAU_FASTFLAG(LuauCoroutineFinally)
LUAU_FASTFLAG(LuauCoroutineFinallyAnalysis)
#include "upstream_populate_rtti.inc"

static void emit(lua_State* L, int index)
{
    index = lua_absindex(L, index);
    if (lua_istable(L, index)) {
        std::cout << "{";
        lua_pushnil(L);
        while (lua_next(L, index)) {
            const char* key = lua_tostring(L, -2);
            if (!key) std::abort();
            std::cout << "[" << std::quoted(key) << "]=";
            emit(L, -1);
            std::cout << ",";
            lua_pop(L, 1);
        }
        std::cout << "}";
    } else {
        const char* value = lua_tostring(L, index);
        if (!value) std::abort();
        std::cout << std::quoted(value);
    }
}

int main()
{
    FFlag::LuauIntegerType2.value = true;
    FFlag::LuauCoroutineFinallyAnalysis.value = FFlag::LuauCoroutineFinally.value;
    Luau::NullFileResolver fileResolver;
    Luau::NullConfigResolver configResolver;
    Luau::Frontend frontend{!FFlag::DebugLuauForceOldSolver ? Luau::SolverMode::New : Luau::SolverMode::Old, &fileResolver, &configResolver};
    Luau::registerBuiltinGlobals(frontend, frontend.globals);
    Luau::freeze(frontend.globals.globalTypes);
    lua_State* L = luaL_newstate();
    if (!L) return 1;
    lua_newtable(L);
    for (const auto& [name, binding] : frontend.globals.globalScope->bindings) {
        populateRTTI(L, binding.typeId);
        lua_setfield(L, -2, toString(name).c_str());
    }
    std::cout << "return ";
    emit(L, -1);
    std::cout << "\n";
    lua_close(L);
    return std::cout ? 0 : 1;
}
