/* Headless mod-only qualification runner: executes Luau source files in one
 * sandboxed p4_luau VM, in order, passing each chunk's results as the next
 * chunk's arguments. The host adds only emit(text) (one stdout line),
 * read_file(path) -> buffer|nil for fixture inputs, sha256(data) (the shared
 * native digest p4.sha256 uses), safepoints() (the VM's interrupt count, to
 * emulate the game's interpreter budget) and heap() (allocated bytes, against
 * the game's mod memory limit); there is no game,
 * no p4 provider and no budget. Scripts supply their own synthetic p4 fixture. */
#include "p4pc/luau_c.h"
#include "p4pc/content_digest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *load(const char *path, size_t *size)
{
    FILE *stream = fopen(path, "rb");
    if (!stream) return NULL;
    char *bytes = NULL;
    long length = fseek(stream, 0, SEEK_END) ? -1 : ftell(stream);
    if (length >= 0 && !fseek(stream, 0, SEEK_SET) && (bytes = malloc((size_t)length + 1)) != NULL) {
        if (fread(bytes, 1, (size_t)length, stream) != (size_t)length) {
            free(bytes);
            bytes = NULL;
        } else {
            bytes[length] = 0;
            *size = (size_t)length;
        }
    }
    fclose(stream);
    return bytes;
}
static int emit(lua_State *L)
{
    size_t size;
    const char *text = luaL_checklstring(L, 1, &size);
    fwrite(text, 1, size, stdout);
    fputc('\n', stdout);
    fflush(stdout);
    return 0;
}
static int read_file(lua_State *L)
{
    size_t size = 0;
    char *bytes = load(luaL_checkstring(L, 1), &size);
    if (!bytes) {
        lua_pushnil(L);
        return 1;
    }
    void *out = lua_newbuffer(L, size);
    memcpy(out, bytes, size);
    free(bytes);
    return 1;
}
static int sha256_digest(lua_State *L)
{
    size_t size = 0;
    const void *bytes = lua_type(L, 1) == LUA_TSTRING ? (const void *)lua_tolstring(L, 1, &size)
                                                      : luaL_checkbuffer(L, 1, &size);
    char hex[P4_CONTENT_DIGEST_HEX + 1], digest[32];
    p4_content_digest(bytes, size, hex);
    for (int i = 0; i < 32; ++i) {
        int high = hex[2 * i] <= '9' ? hex[2 * i] - '0' : hex[2 * i] - 'a' + 10;
        int low = hex[2 * i + 1] <= '9' ? hex[2 * i + 1] - '0' : hex[2 * i + 1] - 'a' + 10;
        digest[i] = (char)(high << 4 | low);
    }
    lua_pushlstring(L, digest, sizeof digest);
    return 1;
}
/* Every interrupt counts, VM safepoints and GC steps alike, as the game's
 * interpreter budget charges them (src/platform/mods/lua_runtime.c). */
static double interrupts;
static int interrupt(void *user, int gc)
{
    (void)user;
    (void)gc;
    interrupts += 1;
    return 0;
}
static int safepoints(lua_State *L)
{
    lua_pushnumber(L, interrupts);
    return 1;
}
/* heap([collect]): the VM's allocated bytes, everything the game's 8 MiB mod
 * limit counts (src/platform/mods/luau_c.cpp), live data and not yet collected
 * garbage alike; heap(true) first runs a full collection (live data only). */
static int heap(lua_State *L)
{
    if (lua_toboolean(L, 1)) lua_gc(L, LUA_GCCOLLECT, 0);
    lua_pushnumber(L, (double)lua_gc(L, LUA_GCCOUNT, 0) * 1024.0 + (double)lua_gc(L, LUA_GCCOUNTB, 0));
    return 1;
}
static void initialize(lua_State *L, void *user)
{
    (void)user;
    lua_pushcfunction(L, heap, "heap");
    lua_setglobal(L, "heap");
    lua_pushcfunction(L, emit, "emit");
    lua_setglobal(L, "emit");
    lua_pushcfunction(L, read_file, "read_file");
    lua_setglobal(L, "read_file");
    lua_pushcfunction(L, safepoints, "safepoints");
    lua_setglobal(L, "safepoints");
    lua_pushcfunction(L, sha256_digest, "sha256");
    lua_setglobal(L, "sha256");
}
int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: luau_script_runner script.luau... [-- argument...]\n");
        return 2;
    }
    p4_luau_config config = {256u * 1024u * 1024u, interrupt, NULL, initialize, 4u * 1024u * 1024u};
    p4_luau *vm = p4_luau_create(&config);
    if (!vm) {
        fprintf(stderr, "luau_script_runner: cannot create the VM\n");
        return 2;
    }
    lua_State *L = p4_luau_state(vm);
    int first_argument = argc;
    for (int i = 1; i < argc; ++i)
        if (!strcmp(argv[i], "--")) { first_argument = i + 1; break; }
    /* Arguments after "--" are the first chunk's arguments. */
    int carried = 0;
    for (int i = first_argument; i < argc; ++i, ++carried) lua_pushstring(L, argv[i]);
    for (int i = 1; i < argc && strcmp(argv[i], "--"); ++i) {
        size_t size = 0;
        char *source = load(argv[i], &size);
        char name[1100];
        snprintf(name, sizeof name, "@%s", argv[i]);
        if (!source) {
            fprintf(stderr, "luau_script_runner: cannot read %s\n", argv[i]);
            return 2;
        }
        int base = lua_gettop(L) - carried;
        int status = p4_luau_load_source(vm, name, source, size);
        free(source);
        if (status != LUA_OK) {
            fprintf(stderr, "luau_script_runner: %s\n", p4_luau_error(vm));
            return 1;
        }
        lua_insert(L, base + 1);
        if (p4_luau_run(vm, carried, LUA_MULTRET) != LUA_OK) {
            fprintf(stderr, "luau_script_runner: %s\n", p4_luau_error(vm));
            return 1;
        }
        carried = lua_gettop(L) - base;
    }
    p4_luau_destroy(vm);
    return 0;
}
