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
void p4_lua_authorize_initialize(lua_State *state);
void p4_lua_authorize_close(P4LuaMod *mod);
void p4_lua_services_initialize(lua_State *state);
void p4_lua_services_close(P4LuaMod *mod);
#endif
