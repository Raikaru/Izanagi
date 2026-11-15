#include "lua_runtime.h"

int64_t p4_lua_integer(lua_State *L, int index, int64_t low, int64_t high)
{
    /* Public scalar numbers are at most 32 bits. Wide values use buffers;
     * never cast a rounded double at the int64 boundary. */
    if (low < INT32_MIN || high > UINT32_MAX || low > high)
        luaL_error(L, "internal scalar bounds exceed the 32-bit API");
    luaL_checktype(L, index, LUA_TNUMBER);
    double number = lua_tonumber(L, index);
    if (!(number >= (double)low && number <= (double)high))
        luaL_argerror(L, index, "number outside scalar range");
    int64_t integer = (int64_t)number;
    if ((double)integer != number)
        luaL_argerror(L, index, "integer required");
    return integer;
}

uint64_t p4_lua_wide(lua_State *L, int index)
{
    size_t size;
    const unsigned char *bytes = luaL_checkbuffer(L, index, &size);
    if (size != 8) luaL_argerror(L, index, "64-bit values require an eight-byte buffer");
    uint64_t value = 0;
    for (unsigned i = 0; i < 8; ++i) value |= (uint64_t)bytes[i] << (i * 8);
    return value;
}

void p4_lua_push_wide(lua_State *L, uint64_t value)
{
    unsigned char *bytes = lua_newbuffer(L, 8);
    for (unsigned i = 0; i < 8; ++i) bytes[i] = (unsigned char)(value >> (i * 8));
}

void *p4_lua_userdata(lua_State *L, int index, int tag, const char *name)
{
    void *value = lua_touserdatatagged(L, index, tag);
    if (value == NULL) luaL_typeerror(L, index, name);
    return value;
}

void p4_lua_library(lua_State *L, const luaL_Reg *methods)
{
    lua_newtable(L);
    luaL_register(L, NULL, methods);
    lua_setreadonly(L, -1, 1);
}

void p4_lua_namespace_merge(lua_State *L, const char *name)
{
    lua_rawgetfield(L, -2, name);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_setreadonly(L, -1, 1);
        lua_rawsetfield(L, -2, name);
        return;
    }
    /* Refuse duplicate fields instead of silently replacing a provider. */
    if (!lua_istable(L, -1)) luaL_error(L, "p4.%s is not a namespace table", name);
    if (lua_getmetatable(L, -2)) luaL_error(L, "p4.%s: a merged table cannot carry a metatable", name);
    lua_setreadonly(L, -1, 0);
    for (lua_pushnil(L); lua_next(L, -3);) {
        lua_pushvalue(L, -2);
        lua_rawget(L, -4);
        if (!lua_isnil(L, -1)) luaL_error(L, "p4.%s.%s is defined by two initializers", name, lua_tostring(L, -3));
        lua_pop(L, 1);
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_rawset(L, -4);
    }
    lua_setreadonly(L, -1, 1);
    lua_pop(L, 2);
}
