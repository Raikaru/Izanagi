/* p4.audio: per-category volume of the port's mixer (API 1.5).
 *
 * Game-only, registered with p4.ui and p4.elements through the native bindings.
 * Each mod sets its own percent per category; the mixer gets the product of
 * every mod's factor, so no mod silently overrides another, and a mod's factors
 * return to 100 when its VM is torn down. The result multiplies inside the mix,
 * under the player's own volume in Config, which stays authoritative. Volumes
 * are mixer state, not game state: no guest write, no native game call and no
 * save tag, so every function is legal in synchronous UI callbacks too. */
#include "native_bindings.h"
#include "../sound/p4snd.h"
#include "p4pc/mod_memory.h"
#include "p4pc/packages.h"
#include <stdio.h>
#include <string.h>

typedef struct AudioRequest {
    const P4LuaMod *mod;
    uint8_t percent[P4SND_CAT_COUNT];
} AudioRequest;
static AudioRequest requests[P4_PACKAGE_MAX];
static const char *const category_names[] = {"music", "effects", "voice", "movie", NULL};

static int category_argument(lua_State *L, int index)
{
    return luaL_checkoption(L, index, NULL, category_names);
}
static AudioRequest *request_of(const P4LuaMod *mod, int create)
{
    AudioRequest *free_slot = NULL;
    for (size_t i = 0; i < P4_PACKAGE_MAX; ++i) {
        if (requests[i].mod == mod) return &requests[i];
        if (!requests[i].mod && !free_slot) free_slot = &requests[i];
    }
    if (!create || !free_slot) return NULL;
    free_slot->mod = mod;
    memset(free_slot->percent, 100, sizeof free_slot->percent);
    return free_slot;
}
/* The product of every mod's factor for one category; exactly 1 when all are 100. */
static void publish(int category)
{
    double gain = 1.0;
    for (size_t i = 0; i < P4_PACKAGE_MAX; ++i)
        if (requests[i].mod && requests[i].percent[category] != 100) gain *= requests[i].percent[category] / 100.0;
    if (gain != 1.0)
        fprintf(stderr, "p4pc: audio: %s volume %.3f from mods\n", category_names[category], gain);
    p4snd_set_category_gain(category, (float)gain);
}
static int audio_volume(lua_State *L)
{
    const int category = category_argument(L, 1);
    const AudioRequest *request = request_of(p4_luau_user(L), 0);
    lua_pushinteger(L, request ? request->percent[category] : 100);
    return 1;
}
static int audio_set_volume(lua_State *L)
{
    const int category = category_argument(L, 1);
    const int percent = (int)p4_lua_integer(L, 2, 0, 100);
    AudioRequest *request = request_of(p4_luau_user(L), 1);
    if (!request) {
        lua_pushboolean(L, 0);
        return 1;
    }
    if (request->percent[category] != percent) {
        request->percent[category] = (uint8_t)percent;
        publish(category);
    }
    lua_pushboolean(L, 1);
    return 1;
}
static int audio_tag_adxt(lua_State *L)
{
    const uint32_t handle = (uint32_t)p4_lua_integer(L, 1, 0, UINT32_MAX);
    const int category = category_argument(L, 2);
    /* The renderer word must be guest memory a mod may read before the mixer follows it. */
    const int readable = handle <= UINT32_MAX - 0x10u && !p4_mod_memory_check(handle + 0x0cu, 4, 0, 0, NULL);
    lua_pushboolean(L, readable && p4snd_adxt_set_category(handle, category) == 0);
    return 1;
}
void p4_lua_audio_initialize(lua_State *L)
{
    static const luaL_Reg methods[] = {
        {"volume", audio_volume}, {"set_volume", audio_set_volume}, {"tag_adxt", audio_tag_adxt}, {NULL, NULL}
    };
    p4_lua_library(L, methods);
    lua_setfield(L, -2, "audio");
}
void p4_lua_audio_close(P4LuaMod *mod)
{
    AudioRequest *request = request_of(mod, 0);
    if (!request) return;
    uint8_t changed[P4SND_CAT_COUNT];
    for (int c = 0; c < P4SND_CAT_COUNT; ++c) changed[c] = request->percent[c] != 100;
    memset(request, 0, sizeof *request);
    for (int c = 0; c < P4SND_CAT_COUNT; ++c)
        if (changed[c]) publish(c);
}
