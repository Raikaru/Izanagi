#include "lua_runtime.h"
#include "../files/vfs.h"
#include <string.h>

typedef struct FileRead { int fd; size_t size; } FileRead;
static int fill_buffer(lua_State *L)
{
    FileRead *read = lua_touserdata(L, 1);
    void *bytes = lua_newbuffer(L, read->size);
    if (read->size != 0 && vfs_read(read->fd, bytes, (int)read->size) != (int)read->size)
        luaL_error(L, "short VFS resource read");
    return 1;
}

static int read_file(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    size_t path_size;
    const char *path = luaL_checklstring(L, 1, &path_size);
    if (path_size == 0 || path_size >= 512 || memchr(path, '\0', path_size) != NULL)
        luaL_error(L, "VFS path must contain 1..511 bytes without NUL");
    int maximum = lua_isnoneornil(L, 2) ? 4 * 1024 * 1024 :
        (int)p4_lua_integer(L, 2, 0, 4 * 1024 * 1024);
    FileRead read = {.fd = vfs_open(path, 0)};
    if (read.fd < 0) luaL_error(L, "resource not found or VFS not mounted: %s", path);
    long long size = vfs_seek(read.fd, 0, 2);
    if (size < 0 || size > maximum || vfs_seek(read.fd, 0, 0) != 0) {
        vfs_close(read.fd);
        luaL_error(L, "resource exceeds read bound or cannot seek: %s", path);
    }
    read.size = (size_t)size;
    /* The cached C closure and light userdata do not allocate. All buffer
     * allocation/read work is protected before the descriptor is closed, so an
     * OOM or observer error cannot defer descriptor cleanup to garbage collection. */
    lua_getref(L, mod->file_read_reference);
    lua_pushlightuserdata(L, &read);
    int status = lua_pcall(L, 1, 1, 0);
    int closed = vfs_close(read.fd);
    if (status != LUA_OK) lua_error(L);
    if (closed != 0) luaL_error(L, "resource close failed: %s", path);
    return 1;
}

void p4_lua_files(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    lua_pushcfunction(L, fill_buffer, "p4.files.private.read");
    mod->file_read_reference = lua_ref(L, -1);
    lua_pop(L, 1);
    static const luaL_Reg methods[] = {{"read", read_file}, {NULL, NULL}};
    p4_lua_library(L, methods);
}
