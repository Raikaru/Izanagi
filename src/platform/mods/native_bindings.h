#ifndef P4PC_MOD_NATIVE_BINDINGS_H
#define P4PC_MOD_NATIVE_BINDINGS_H
#include "lua_runtime.h"
/* Game-owned optional native services, not pretend providers in core VM tests.
 * Descriptor has process lifetime and is installed before source admission. */
typedef struct P4LuaNativeBindings {
    /* The mutable p4 namespace is at stack top; leave stack depth unchanged. */
    void (*initialize)(lua_State *state);
    /* Outside callbacks, before tombstoning callbacks/VM userdata finalizers.
     * Detach native views synchronously or enter controlled native recovery. */
    void (*before_destroy)(P4LuaMod *mod);
} P4LuaNativeBindings;
int p4_lua_bind_native(const P4LuaNativeBindings *bindings);
void p4_lua_ui_initialize(lua_State *state);
void p4_lua_ui_close(P4LuaMod *mod);
/* p4.ui.sprite / p4.ui.overlay (lua_overlay.c), registered by p4_lua_ui_initialize. */
void p4_lua_overlay_initialize(lua_State *state);
int p4_lua_ui_sprite(lua_State *state);
int p4_lua_ui_overlay(lua_State *state);
void p4_lua_overlay_close(P4LuaMod *mod);
void p4_lua_authorize_initialize(lua_State *state);
void p4_lua_authorize_close(P4LuaMod *mod);
void p4_lua_services_initialize(lua_State *state);
void p4_lua_services_close(P4LuaMod *mod);
void p4_lua_damage_initialize(lua_State *state);
void p4_lua_damage_close(P4LuaMod *mod);
void p4_lua_elements_initialize(lua_State *state);
void p4_lua_audio_initialize(lua_State *state);
void p4_lua_audio_close(P4LuaMod *mod);
void p4_lua_tables_initialize(lua_State *state);
void p4_lua_tables_close(P4LuaMod *mod);
void p4_lua_battle_initialize(lua_State *state);
void p4_lua_battle_close(P4LuaMod *mod);
void p4_lua_party_initialize(lua_State *state);
void p4_lua_party_close(P4LuaMod *mod);
#endif
