/* Shipped Fusion Arts page + production story binding in the real Luau VM.
 * Only saved flag storage and the other game providers are synthetic. */
#include "../src/platform/mods/native_bindings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t flags[704];

uint32_t datGetFlag(int32_t bit)
{
    if (bit < 0 || bit >= 0x1600) abort();
    return (flags[bit / 8] >> (bit % 8)) & 1u;
}

/* Replace the current save's entire flag section, as a load or NG+ reset does.
 * No mod-side state participates in this gate. */
static int load_flags(lua_State *L)
{
    size_t size;
    const void *bytes = luaL_checkbuffer(L, 1, &size);
    if (size != sizeof flags) luaL_error(L, "expected saved flag section");
    memcpy(flags, bytes, size);
    return 0;
}

static void initialize(lua_State *L, void *user)
{
    (void)user;
    lua_newtable(L);
    p4_lua_story_initialize(L);
    lua_getfield(L, -1, "story");
    lua_setglobal(L, "native_story");
    lua_pop(L, 1);
    lua_pushcfunction(L, load_flags, "load_flags");
    lua_setglobal(L, "load_story_flags");
}

static int run_file(p4_luau *vm, const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file) { fprintf(stderr, "cannot open %s\n", path); return 1; }
    if (fseek(file, 0, SEEK_END)) { fclose(file); return 1; }
    long length = ftell(file);
    if (length < 0 || length > 1024 * 1024 || fseek(file, 0, SEEK_SET)) { fclose(file); return 1; }
    char *source = malloc((size_t)length + 1);
    if (!source) { fclose(file); return 1; }
    size_t count = fread(source, 1, (size_t)length, file);
    fclose(file);
    if (count != (size_t)length) { free(source); return 1; }
    source[count] = 0;
    int status = p4_luau_load_source(vm, path, source, count);
    free(source);
    if (status == LUA_OK) status = p4_luau_run(vm, 0, 0);
    if (status != LUA_OK) { fprintf(stderr, "%s: %s\n", path, p4_luau_error(vm)); return 1; }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 4) return 2;
    p4_luau_config config = {8 * 1024 * 1024, NULL, NULL, initialize, 1024 * 1024};
    p4_luau *vm = p4_luau_create(&config);
    if (!vm) return 1;
    int failed = run_file(vm, argv[1]) || run_file(vm, argv[2]) || run_file(vm, argv[3]);
    p4_luau_destroy(vm);
    return failed ? 1 : 0;
}
