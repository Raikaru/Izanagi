// Extracted unchanged from upstream Conformance.test.cpp at 32c28089; MIT, see upstream-0.744/LICENSE.txt.
static int lua_vector_dot(lua_State* L)
{
    const LUA_VECTOR_TYPE* a = luaL_checkvector(L, 1);
    const LUA_VECTOR_TYPE* b = luaL_checkvector(L, 2);

    lua_pushnumber(L, a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
    return 1;
}

static int lua_vector_cross(lua_State* L)
{
    const LUA_VECTOR_TYPE* a = luaL_checkvector(L, 1);
    const LUA_VECTOR_TYPE* b = luaL_checkvector(L, 2);

    lua_pushvector3(L, a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
    return 1;
}

static int lua_vector_index(lua_State* L)
{
    const LUA_VECTOR_TYPE* v = luaL_checkvector(L, 1);
    const char* name = luaL_checkstring(L, 2);

    if (strcmp(name, "Magnitude") == 0)
    {
#if LUA_VECTOR_SIZE == 4
        lua_pushnumber(L, sqrtf(float(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3])));
#else
        lua_pushnumber(L, sqrtf(float(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])));
#endif
        return 1;
    }

    if (strcmp(name, "Unit") == 0)
    {
#if LUA_VECTOR_SIZE == 4
        float invSqrt = 1.0f / sqrtf(float(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3]));

        lua_pushvector(L, v[0] * invSqrt, v[1] * invSqrt, v[2] * invSqrt, v[3] * invSqrt);
#else
        float invSqrt = 1.0f / sqrtf(float(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));

        lua_pushvector(L, v[0] * invSqrt, v[1] * invSqrt, v[2] * invSqrt);
#endif
        return 1;
    }

    if (strcmp(name, "Dot") == 0)
    {
        lua_pushcfunction(L, lua_vector_dot, "Dot");
        return 1;
    }

    luaL_error(L, "%s is not a valid member of vector", name);
}

static int lua_vector_namecall(lua_State* L)
{
    if (const char* str = lua_namecallatom(L, nullptr))
    {
        if (strcmp(str, "Dot") == 0)
            return lua_vector_dot(L);

        if (strcmp(str, "Cross") == 0)
            return lua_vector_cross(L);
    }

    luaL_error(L, "%s is not a valid method of vector", luaL_checkstring(L, 1));
}


void setupVectorHelpers(lua_State* L)
{
    lua_pushvector3(L, 0.0f, 0.0f, 0.0f);
    luaL_newmetatable(L, "vector");

    lua_pushstring(L, "__index");
    lua_pushcfunction(L, lua_vector_index, nullptr);
    lua_settable(L, -3);

    lua_pushstring(L, "__namecall");
    lua_pushcfunction(L, lua_vector_namecall, nullptr);
    lua_settable(L, -3);

    lua_setreadonly(L, -1, true);
    lua_setmetatable(L, -2);
    lua_pop(L, 1);
}

