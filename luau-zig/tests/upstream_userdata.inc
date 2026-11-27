// Extracted unchanged callbacks from upstream Conformance.test.cpp at 32c28089; MIT, see upstream-0.744/LICENSE.txt.
const int kInt64Tag = 1;

static int64_t getInt64(lua_State* L, int idx)
{
    if (void* p = lua_touserdatatagged(L, idx, kInt64Tag))
        return *static_cast<int64_t*>(p);

    if (lua_isnumber(L, idx))
        return lua_tointeger(L, idx);

    luaL_typeerror(L, 1, "int64");
}

static void pushInt64(lua_State* L, int64_t value)
{
    void* p = lua_newuserdatatagged(L, sizeof(int64_t), kInt64Tag);

    luaL_getmetatable(L, "int64");
    lua_setmetatable(L, -2);

    *static_cast<int64_t*>(p) = value;
}


static void setup_userdata(lua_State* L)
{
            // create metatable with all the metamethods
            luaL_newmetatable(L, "int64");

            lua_pushliteral(L, "int64");
            lua_setfield(L, -2, "__type");

            // __index
            lua_pushcfunction(
                L,
                [](lua_State* L)
                {
                    void* p = luaL_checkudatatagged(L, 1, kInt64Tag);
                    const char* name = luaL_checkstring(L, 2);

                    if (strcmp(name, "value") == 0)
                    {
                        lua_pushnumber(L, double(*static_cast<int64_t*>(p)));
                        return 1;
                    }

                    luaL_error(L, "unknown field %s", name);
                },
                nullptr
            );
            lua_setfield(L, -2, "__index");

            // __newindex
            lua_pushcfunction(
                L,
                [](lua_State* L)
                {
                    void* p = luaL_checkudatatagged(L, 1, kInt64Tag);
                    const char* name = luaL_checkstring(L, 2);

                    if (strcmp(name, "value") == 0)
                    {
                        double value = luaL_checknumber(L, 3);
                        *static_cast<int64_t*>(p) = int64_t(value);
                        return 0;
                    }

                    luaL_error(L, "unknown field %s", name);
                },
                nullptr
            );
            lua_setfield(L, -2, "__newindex");

            // __eq
            lua_pushcfunction(
                L,
                [](lua_State* L)
                {
                    lua_pushboolean(L, getInt64(L, 1) == getInt64(L, 2));
                    return 1;
                },
                nullptr
            );
            lua_setfield(L, -2, "__eq");

            // __lt
            lua_pushcfunction(
                L,
                [](lua_State* L)
                {
                    lua_pushboolean(L, getInt64(L, 1) < getInt64(L, 2));
                    return 1;
                },
                nullptr
            );
            lua_setfield(L, -2, "__lt");

            // __le
            lua_pushcfunction(
                L,
                [](lua_State* L)
                {
                    lua_pushboolean(L, getInt64(L, 1) <= getInt64(L, 2));
                    return 1;
                },
                nullptr
            );
            lua_setfield(L, -2, "__le");

            // __add
            lua_pushcfunction(
                L,
                [](lua_State* L)
                {
                    pushInt64(L, getInt64(L, 1) + getInt64(L, 2));
                    return 1;
                },
                nullptr
            );
            lua_setfield(L, -2, "__add");

            // __sub
            lua_pushcfunction(
                L,
                [](lua_State* L)
                {
                    pushInt64(L, getInt64(L, 1) - getInt64(L, 2));
                    return 1;
                },
                nullptr
            );
            lua_setfield(L, -2, "__sub");

            // __mul
            lua_pushcfunction(
                L,
                [](lua_State* L)
                {
                    pushInt64(L, getInt64(L, 1) * getInt64(L, 2));
                    return 1;
                },
                nullptr
            );
            lua_setfield(L, -2, "__mul");

            // __div
            lua_pushcfunction(
                L,
                [](lua_State* L)
                {
                    // ideally we'd guard against 0 but it's a test so eh
                    pushInt64(L, getInt64(L, 1) / getInt64(L, 2));
                    return 1;
                },
                nullptr
            );
            lua_setfield(L, -2, "__div");

            // __idiv
            lua_pushcfunction(
                L,
                [](lua_State* L)
                {
                    // for testing we use different semantics here compared to __div: __idiv rounds to negative inf, __div truncates (rounds to zero)
                    // additionally, division loses precision here outside of 2^53 range
                    // we do not necessarily recommend this behavior in production code!
                    pushInt64(L, int64_t(floor(double(getInt64(L, 1)) / double(getInt64(L, 2)))));
                    return 1;
                },
                nullptr
            );
            lua_setfield(L, -2, "__idiv");

            // __mod
            lua_pushcfunction(
                L,
                [](lua_State* L)
                {
                    // ideally we'd guard against 0 and INT64_MIN but it's a test so eh
                    pushInt64(L, getInt64(L, 1) % getInt64(L, 2));
                    return 1;
                },
                nullptr
            );
            lua_setfield(L, -2, "__mod");

            // __pow
            lua_pushcfunction(
                L,
                [](lua_State* L)
                {
                    pushInt64(L, int64_t(pow(double(getInt64(L, 1)), double(getInt64(L, 2)))));
                    return 1;
                },
                nullptr
            );
            lua_setfield(L, -2, "__pow");

            // __unm
            lua_pushcfunction(
                L,
                [](lua_State* L)
                {
                    pushInt64(L, -getInt64(L, 1));
                    return 1;
                },
                nullptr
            );
            lua_setfield(L, -2, "__unm");

            // __tostring
            lua_pushcfunction(
                L,
                [](lua_State* L)
                {
                    int64_t value = getInt64(L, 1);
                    std::string str = std::to_string(value);
                    lua_pushlstring(L, str.c_str(), str.length());
                    return 1;
                },
                nullptr
            );
            lua_setfield(L, -2, "__tostring");

            // ctor
            lua_pushcfunction(
                L,
                [](lua_State* L)
                {
                    double v = luaL_checknumber(L, 1);
                    pushInt64(L, int64_t(v));
                    return 1;
                },
                "int64"
            );
            lua_setglobal(L, "int64");
}
