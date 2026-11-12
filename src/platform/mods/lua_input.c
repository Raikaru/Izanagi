#include "p4pc/mod_script.h"
#include "../pad/pad_sdl.h"
#include "lua_runtime.h"
#include <stddef.h>
#include <string.h>

int p4_mod_input_enabled;
static uint16_t buttons, pressed, released;
static uint8_t axes[4] = {128, 128, 128, 128};
static uint32_t sample_frame;
static int valid;

void p4_mod_input_sample(const uint8_t packet[32], uint32_t frame)
{
    uint16_t next = (uint16_t)~(uint16_t)(packet[2] | (packet[3] << 8));
    if (!valid || frame != sample_frame) { pressed = released = 0; sample_frame = frame; }
    pressed |= (uint16_t)(next & ~buttons);
    released |= (uint16_t)(buttons & ~next);
    buttons = next;
    memcpy(axes, packet + 4, sizeof axes);
    valid = 1;
}

static int action(lua_State *L)
{
    p4_lua_require_mutation(L);
    static const struct { const char *name; uint16_t bit; } actions[] = {
        {"confirm", P4PC_PAD_CROSS}, {"cancel", P4PC_PAD_CIRCLE}, {"menu", P4PC_PAD_TRIANGLE}, {"help", P4PC_PAD_SQUARE},
        {"cross", P4PC_PAD_CROSS}, {"circle", P4PC_PAD_CIRCLE}, {"triangle", P4PC_PAD_TRIANGLE}, {"square", P4PC_PAD_SQUARE},
        {"up", P4PC_PAD_UP}, {"down", P4PC_PAD_DOWN}, {"left", P4PC_PAD_LEFT}, {"right", P4PC_PAD_RIGHT},
        {"start", P4PC_PAD_START}, {"select", P4PC_PAD_SELECT}, {"l1", P4PC_PAD_L1}, {"r1", P4PC_PAD_R1},
        {"l2", P4PC_PAD_L2}, {"r2", P4PC_PAD_R2}, {"l3", P4PC_PAD_L3}, {"r3", P4PC_PAD_R3}
    };
    const char *name = luaL_checkstring(L, 1);
    for (size_t i = 0; i < sizeof actions / sizeof actions[0]; i++) {
        if (!strcmp(name, actions[i].name)) {
            p4_mod_input_enabled = 1;
            lua_pushboolean(L, (buttons & actions[i].bit) != 0);
            lua_pushboolean(L, (pressed & actions[i].bit) != 0);
            lua_pushboolean(L, (released & actions[i].bit) != 0);
            return 3;
        }
    }
    luaL_error(L, "unknown semantic input action");
}

static int sticks(lua_State *L)
{
    p4_lua_require_mutation(L);
    p4_mod_input_enabled = 1;
    /* Packet order is RX RY LX LY; public order is left then right. The
     * result describes the delivered packet after bindings/touch/input fences. */
    lua_pushnumber(L, ((int)axes[2] - 128) / 128.0);
    lua_pushnumber(L, ((int)axes[3] - 128) / 128.0);
    lua_pushnumber(L, ((int)axes[0] - 128) / 128.0);
    lua_pushnumber(L, ((int)axes[1] - 128) / 128.0);
    return 4;
}

void p4_lua_input(lua_State *L)
{
    static const luaL_Reg methods[] = {{"action", action}, {"sticks", sticks}, {NULL, NULL}};
    p4_lua_library(L, methods);
}
