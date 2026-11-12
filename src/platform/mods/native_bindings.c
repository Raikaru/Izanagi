#include "p4pc/mod_bindings.h"
#ifdef P4_MOD_LUAU
#include "native_bindings.h"
#include "lua_data_internal.h"
static void initialize(lua_State *L)
{
    p4_lua_ui_initialize(L);
    p4_lua_authorize_initialize(L);
    p4_lua_services_initialize(L);
    p4_lua_battle_present_initialize(L);
    p4_lua_elements_initialize(L);
    p4_lua_data_initialize(L);
    p4_lua_audio_initialize(L);
    p4_lua_stages_initialize(L);
    p4_lua_passive_initialize(L);
    p4_lua_tables_initialize(L);
    p4_lua_battle_initialize(L);
    p4_lua_party_initialize(L);
    p4_lua_semantic_events_initialize(L);
    p4_lua_battle_decide_initialize(L);
    p4_lua_inventory_initialize(L);
    p4_lua_persona_initialize(L);
    p4_lua_story_initialize(L);
    p4_lua_forge_initialize(L);
    p4_lua_equipment_initialize(L);
}
static void before_destroy(P4LuaMod *mod)
{
    p4_lua_battle_decide_close(mod);
    p4_lua_semantic_events_close(mod);
    p4_lua_tables_close(mod);
    p4_lua_stages_close(mod);
    p4_lua_passive_close(mod);
    p4_lua_audio_close(mod);
    p4_lua_battle_close(mod);
    p4_lua_services_close(mod);
    p4_lua_inventory_close(mod);
    p4_lua_battle_present_close(mod);
    p4_lua_authorize_close(mod);
    p4_lua_party_close(mod);
    p4_lua_forge_close(mod);
    p4_lua_equipment_close(mod);
    p4_lua_ui_close(mod);
}
static const P4LuaNativeBindings game = {initialize, before_destroy};
#endif
int p4_mod_game_bindings_init(void)
{
    #ifdef P4_MOD_LUAU
    if(p4_lua_bind_native(&game))return -1;
    p4_lua_data_edit_install(); /* data.luau runs at table seal, before any script VM. */
    return p4_mod_script_failure_init();
    #else
    return 0; /* This build has no interpreter/native script services. */
    #endif
}
