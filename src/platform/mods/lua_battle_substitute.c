/* p4.battle.substitute (capability battle.substitute@1): inside a before-hook on
 * btlAICommandSetActionCode (0x001DE640), replace the enemy's chosen command
 * with a self or all-party skill. The enemy AI carrier authorizes it first
 * (p4_game_elements_ai_native_substitute, the native function's only caller),
 * so with expanded elements the next decode of this action accepts the new
 * command; then the hook's args[3] carries the code into the decode. The
 * invocation is the live one lua_hooks.c runs, never a value the script
 * passes in. */
#include "lua_runtime.h"
#include "native_bindings.h"
#include "p4pc/game_elements_ai_native.h"
#include "p4pc/gmem.h"
#include "p4pc/mod_hooks.h"
#include "p4pc/table_result.h"
#include <string.h>

#define SET_ACTION_CODE 0x001DE640u
#define SET_ACTION_SIGNATURE "vppH"

int p4_lua_battle_substitute(lua_State *L)
{
    const uint32_t action = (uint32_t)p4_lua_integer(L, 1, 0, UINT32_MAX);
    const uint32_t skill = (uint32_t)p4_lua_integer(L, 2, 1, UINT16_MAX);
    p4_lua_require_mutation(L);
    if (p4_lua_frame(L)->no_native_calls)
        luaL_error(L, "p4.battle.substitute is refused in synchronous UI/action callbacks and save snapshots");
    int table = LUA_NOREF;
    const P4ModCall *call = p4_lua_hook_call(L, &table);
    if (call == NULL || call->phase != P4_MOD_BEFORE || call->function->address != SET_ACTION_CODE ||
        strcmp(call->function->signature, SET_ACTION_SIGNATURE) != 0)
        luaL_error(L, "p4.battle.substitute is valid only inside a before-hook on 0x001de640 (vppH)");
    if ((uint32_t)call->args[0].u != action)
        luaL_error(L, "p4.battle.substitute: action is not the action this hook invocation decodes");
    const int result = p4_game_elements_ai_native_substitute(P4_G2H(action), skill);
    if (result != P4_TABLE_OK) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, result == P4_TABLE_NOT_FOUND ? "skill is not an active skill"
            : result == P4_TABLE_CONFLICT ? "skill does not target its user or every unit its sides collect"
            : "the enemy AI carrier refused the substitution");
        return 2;
    }
    /* The before-hook copies the context's args back into the call. */
    lua_getref(L, table);
    lua_getfield(L, -1, "args");
    lua_pushunsigned(L, skill);
    lua_rawseti(L, -2, 3);
    lua_pop(L, 2);
    lua_pushboolean(L, 1);
    return 1;
}
