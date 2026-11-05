#include "p4pc/mod_bindings.h"
#ifdef P4_MOD_LUAU
#include "native_bindings.h"
static void initialize(lua_State *L)
{
    p4_lua_ui_initialize(L);
    p4_lua_authorize_initialize(L);
    p4_lua_services_initialize(L);
}
static void before_destroy(P4LuaMod *mod)
{
    p4_lua_services_close(mod);
    p4_lua_authorize_close(mod);
    p4_lua_ui_close(mod);
}
static const P4LuaNativeBindings game = {initialize, before_destroy};
#endif
int p4_mod_game_bindings_init(void)
{
    #ifdef P4_MOD_LUAU
    if(p4_lua_bind_native(&game))return -1;
    return p4_mod_script_failure_init();
    #else
    return 0; /* This build has no interpreter/native script services. */
    #endif
}
