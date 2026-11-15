#ifndef P4PC_LUA_DATA_INTERNAL_H
#define P4PC_LUA_DATA_INTERNAL_H
#include "p4pc/game_data.h"
#include "p4pc/luau_c.h"
#include "lualib.h"
/* lua_data.c: the catalog readers, shared by main.luau and data.luau. */
const luaL_Reg *p4_lua_data_read_methods(void);
/* Pushes the full typed row; 0, or -1 (nothing pushed) outside the banks. */
int p4_lua_data_push_row(lua_State *L, P4DataKind kind, size_t index);
/* lua_data_edit.c: registers the data.luau runner with the edit layers. */
void p4_lua_data_edit_install(void);
/* The edit view's Luau source (run with the four primitives, it returns the
 * builder of `d`) and its primitives, shared by data.luau and
 * p4.data.session.edit: set, learn, row, lookup over p4_data_edit_*. */
const char *p4_lua_data_edit_prelude(size_t *size);
void p4_lua_data_edit_push_primitives(lua_State *L);
#endif
