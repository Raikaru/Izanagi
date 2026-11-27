// Extracted unchanged from upstream Conformance.test.cpp at 32c28089; MIT, see upstream-0.744/LICENSE.txt.
static void setup_coverage(lua_State* L)
{
            lua_pushcfunction(
                L,
                [](lua_State* L) -> int
                {
                    luaL_argexpected(L, lua_isLfunction(L, 1), 1, "function");

                    lua_newtable(L);
                    lua_getcoverage(
                        L,
                        1,
                        L,
                        [](void* context, const char* function, int linedefined, int depth, const int* hits, size_t size)
                        {
                            lua_State* L = static_cast<lua_State*>(context);

                            lua_newtable(L);

                            lua_pushstring(L, function);
                            lua_setfield(L, -2, "name");

                            lua_pushinteger(L, linedefined);
                            lua_setfield(L, -2, "linedefined");

                            lua_pushinteger(L, depth);
                            lua_setfield(L, -2, "depth");

                            for (size_t i = 0; i < size; ++i)
                                if (hits[i] != -1)
                                {
                                    lua_pushinteger(L, hits[i]);
                                    lua_rawseti(L, -2, int(i));
                                }

                            lua_rawseti(L, -2, lua_objlen(L, -2) + 1);
                        }
                    );

                    return 1;
                },
                "getcoverage"
            );
            lua_setglobal(L, "getcoverage");
}
