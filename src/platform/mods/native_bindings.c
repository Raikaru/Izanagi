#include "p4pc/mod_bindings.h"
#ifdef P4_MOD_LUAU
#include "native_bindings.h"
static const P4LuaNativeBindings game = {p4_lua_ui_initialize, p4_lua_ui_close};
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
