/* Read-only story facts backed by the game's flag storage.
 * cmmHasPendingUnlock (00247020) uses 0x11..0x17 for rescue windows;
 * FldStepBandFlagSet (0015a190) relates the same bits to dungeon bands. */
#include "native_bindings.h"
#include <stdint.h>

extern uint32_t datGetFlag(int32_t bit);

static int dungeon_cleared(lua_State *L)
{
    int dungeon = (int)p4_lua_integer(L, 1, 1, 7);
    lua_pushboolean(L, datGetFlag(0x10 + dungeon) != 0);
    return 1;
}

void p4_lua_story_initialize(lua_State *L)
{
    static const luaL_Reg story[] = {{"dungeon_cleared", dungeon_cleared}, {NULL, NULL}};
    p4_lua_library(L, story);
    lua_setfield(L, -2, "story");
}
