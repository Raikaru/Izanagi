/* Current-playthrough completion latches, not pre-boss rescue-window bits.
 * 1..6: monthly PM_D scheduler after rescue. 7/8: the final two field
 * boss procedures, after their last encounter and post-battle events. */
#include "native_bindings.h"
#include <stdint.h>

extern uint32_t datGetFlag(int32_t bit);

static int dungeon_cleared(lua_State *L)
{
    static const int32_t cleared_flags[] = {
        0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x2B5, 0x2F
    };
    int dungeon = (int)p4_lua_integer(L, 1, 1, 8);
    lua_pushboolean(L, datGetFlag(cleared_flags[dungeon - 1]) != 0);
    return 1;
}

void p4_lua_story_initialize(lua_State *L)
{
    static const luaL_Reg story[] = {{"dungeon_cleared", dungeon_cleared}, {NULL, NULL}};
    p4_lua_library(L, story);
    lua_setfield(L, -2, "story");
}
