// Extracted unchanged callbacks from upstream Conformance.test.cpp at 32c28089; MIT, see upstream-0.744/LICENSE.txt.
int singleYield(lua_State* L)
{
    lua_pushnumber(L, 2);

    return lua_yield(L, 1);
}

int singleYieldContinuation(lua_State* L, int status)
{
    lua_pushnumber(L, 4);
    return 1;
}

int multipleYields(lua_State* L)
{
    lua_settop(L, 1); // Only 1 argument expected
    int base = luaL_checkinteger(L, 1);

    // current state
    int pos = 1;
    lua_pushinteger(L, pos);

    // return value
    lua_pushinteger(L, base + pos);
    return lua_yield(L, 1);
}

int multipleYieldsContinuation(lua_State* L, int status)
{
    // function arguments are still alive
    int base = luaL_checkinteger(L, 1);

    // function state is still alive
    int pos = luaL_checkinteger(L, 2) + 1;
    lua_pushinteger(L, pos);
    lua_replace(L, 2);

    if (pos < 4)
    {
        lua_pushinteger(L, base + pos);
        return lua_yield(L, 1);
    }
    else
    {
        lua_pushinteger(L, base + pos);
        return 1;
    }
}

int nestedMultipleYieldHelper(lua_State* L)
{
    int context = luaL_checkinteger(L, lua_upvalueindex(1));

    lua_pushinteger(L, 100 + context);
    return lua_yield(L, 1);
}

int nestedMultipleYieldHelperContinuation(lua_State* L, int status)
{
    int context = luaL_checkinteger(L, lua_upvalueindex(1));
    lua_pushinteger(L, 110 + context);
    return 1;
}

int nestedMultipleYieldHelperNonYielding(lua_State* L)
{
    int context = luaL_checkinteger(L, lua_upvalueindex(1));
    lua_pushinteger(L, 105 + context);
    return 1;
}

int multipleYieldsWithNestedCall(lua_State* L)
{
    lua_settop(L, 2); // Only 2 arguments expected
    bool nestedShouldYield = luaL_checkboolean(L, 2);

    lua_pushinteger(L, 0); // state

    lua_pushnumber(L, 5);
    if (nestedShouldYield)
        lua_pushcclosurek(L, nestedMultipleYieldHelper, nullptr, 1, nestedMultipleYieldHelperContinuation);
    else
        lua_pushcclosurek(L, nestedMultipleYieldHelperNonYielding, nullptr, 1, nullptr);

    return lua_callyieldable(L, 0, 1);
}

int multipleYieldsWithNestedCallContinuation(lua_State* L, int status)
{
    int state = luaL_checkinteger(L, 3);
    lua_pushinteger(L, state + 1);
    lua_replace(L, 3);

    if (state == 0)
    {
        return lua_yield(L, lua_gettop(L) - 3);
    }
    else if (state == 1)
    {
        lua_pushnumber(L, luaL_checkinteger(L, 1) + 200);
        return lua_yield(L, 1);
    }
    else
    {
        lua_pushnumber(L, luaL_checkinteger(L, 1) + 210);
        return 1;
    }
}

int passthroughCall(lua_State* L)
{
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    return lua_callyieldable(L, 2, 1);
}

int passthroughCallContinuation(lua_State* L, int status)
{
    LUAU_ASSERT(lua_gettop(L) == 4); // 3 original arguments and the return value
    LUAU_ASSERT(lua_tonumber(L, -1) == 0.5);
    return 1;
}

int passthroughCallMoreResults(lua_State* L)
{
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    return lua_callyieldable(L, 2, 10);
}

int passthroughCallMoreResultsContinuation(lua_State* L, int status)
{
    LUAU_ASSERT(lua_gettop(L) == 13); // 3 original arguments and 10 requested return values

    for (int i = 0; i < 9; i++)
    {
        LUAU_ASSERT(lua_isnil(L, -1));
        lua_pop(L, 1);
    }

    LUAU_ASSERT(lua_tonumber(L, -1) == 0.5);
    return 1;
}

int passthroughCallArgReuse(lua_State* L)
{
    return lua_callyieldable(L, 2, 1);
}

int passthroughCallArgReuseContinuation(lua_State* L, int status)
{
    LUAU_ASSERT(lua_gettop(L) == 1); // Original arguments were consumed, only return remains
    LUAU_ASSERT(lua_tonumber(L, -1) == 0.5);
    return 1;
}

int passthroughCallVaradic(lua_State* L)
{
    luaL_checkany(L, 1);
    return lua_callyieldable(L, lua_gettop(L) - 1, LUA_MULTRET);
}

int passthroughCallVaradicContinuation(lua_State* L, int status)
{
    return lua_gettop(L);
}

int passthroughCallWithState(lua_State* L)
{
    luaL_checkany(L, 1);
    int args = lua_gettop(L) - 1;

    lua_pushnumber(L, 42);
    lua_insert(L, 1);

    return lua_callyieldable(L, args, LUA_MULTRET);
}

int passthroughCallWithStateContinuation(lua_State* L, int status)
{
    LUAU_ASSERT(luaL_checkinteger(L, 1) == 42);

    return lua_gettop(L) - 1;
}

int pcallThenXCall(lua_State* L)
{
    luaL_checkany(L, 1);
    luaL_checkany(L, 2);

    luaL_checkstack(L, 3, "pcallThenCall");
    lua_pushinteger(L, 0); // state
    lua_pushinteger(L, 0); // multiplier

    lua_pushvalue(L, 1); // call first function
    return lua_pcallyieldable(L, 0, 1, 0);
}

int pcallThenXCallContinuation(lua_State* L, int status)
{
    luaL_checkstack(L, 1, "pcallThenCallContinuation");

    int pcallVariant = lua_tointeger(L, lua_upvalueindex(1));
    int state = luaL_checkinteger(L, 3);

    if (state == 0)
    {
        if (status != LUA_OK)
        {
            lua_pushinteger(L, -1);
            lua_replace(L, 4);
        }
        else
        {
            lua_replace(L, 4);
        }

        lua_pushinteger(L, 1);
        lua_replace(L, 3);

        lua_pushvalue(L, 2); // call second function
        return pcallVariant ? lua_pcallyieldable(L, 0, LUA_MULTRET, 0) : lua_callyieldable(L, 0, LUA_MULTRET);
    }

    int multiplier = luaL_checkinteger(L, 4);
    int value = -1;

    if (status != LUA_OK)
        LUAU_ASSERT(pcallVariant);
    else
        value = lua_tointeger(L, -1);

    lua_pushinteger(L, multiplier * value);
    return 1;
}


static void setup_cyield(lua_State* L)
{
            lua_pushcclosurek(L, singleYield, "singleYield", 0, singleYieldContinuation);
            lua_setglobal(L, "singleYield");

            lua_pushcclosurek(L, multipleYields, "multipleYields", 0, multipleYieldsContinuation);
            lua_setglobal(L, "multipleYields");

            lua_pushcclosurek(L, multipleYieldsWithNestedCall, "multipleYieldsWithNestedCall", 0, multipleYieldsWithNestedCallContinuation);
            lua_setglobal(L, "multipleYieldsWithNestedCall");

            lua_pushcclosurek(L, passthroughCall, "passthroughCall", 0, passthroughCallContinuation);
            lua_setglobal(L, "passthroughCall");

            lua_pushcclosurek(L, passthroughCallMoreResults, "passthroughCallMoreResults", 0, passthroughCallMoreResultsContinuation);
            lua_setglobal(L, "passthroughCallMoreResults");

            lua_pushcclosurek(L, passthroughCallArgReuse, "passthroughCallArgReuse", 0, passthroughCallArgReuseContinuation);
            lua_setglobal(L, "passthroughCallArgReuse");

            lua_pushcclosurek(L, passthroughCallVaradic, "passthroughCallVaradic", 0, passthroughCallVaradicContinuation);
            lua_setglobal(L, "passthroughCallVaradic");

            lua_pushcclosurek(L, passthroughCallWithState, "passthroughCallWithState", 0, passthroughCallWithStateContinuation);
            lua_setglobal(L, "passthroughCallWithState");

            lua_pushinteger(L, 0);
            lua_pushcclosurek(L, pcallThenXCall, "pcallThenCall", 1, pcallThenXCallContinuation);
            lua_setglobal(L, "pcallThenCall");

            lua_pushinteger(L, 1);
            lua_pushcclosurek(L, pcallThenXCall, "pcallThenPcall", 1, pcallThenXCallContinuation);
            lua_setglobal(L, "pcallThenPcall");
}
