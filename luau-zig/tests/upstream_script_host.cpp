// Test-only script host adapted from Luau tests/Conformance.test.cpp at
// 32c28089305f488f901aaf1dbf3fd1cb65fa3d11. Licenses and the unchanged
// upstream source are in tests/upstream-0.744. It deliberately does not claim
// the upstream bytecode-graph/GC validations or C++ post-run assertions.
#include "lua.h"
#include "lualib.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <math.h>
#include <string>
#ifndef LUAU_CONFORMANCE_EMBED_HOST
#include <memory>
#include "upstream-0.744/tests/BufferCage.h"
#endif

#ifdef LUAU_ZIG_CANDIDATE
// Output is malloc-owned. Return 0=compiled, 77=unsupported option, 2=OOM.
extern "C" int zig_conformance_compile(const char*, size_t, int, int, int, int, const char*, char**, size_t*);
extern "C" int luau_setfflag(const char*, int);
extern "C" int luau_stdlib_setflag(const char*, int);
static bool set_library_flag(const char* name, bool value = true) { return luau_stdlib_setflag(name, value ? 1 : 0) != 0; }
static bool set_vm_flag(const char* name, bool value = true) { return luau_setfflag(name, value ? 1 : 0) != 0; }
static bool set_compiler_flag(const char*) { return true; }
static bool set_class_flags(bool value) { return set_vm_flag("DebugLuauUserDefinedClassesRuntime", value); }
#define LUAU_ASSERT(expr) do { if (!(expr)) { std::fprintf(stderr, "upstream callback assertion: %s\n", #expr); std::abort(); } } while (0)
#else
#include "luacode.h"
#include "Luau/Common.h"
LUAU_DYNAMIC_FASTFLAG(LuauTableRobustOom)
LUAU_FASTFLAG(LuauCoroutineFinally)
LUAU_FASTFLAG(LuauPcallErrorContinuationReentry)
LUAU_FASTFLAG(LuauExperimentalIfLocalSyntax)
LUAU_FASTFLAG(LuauCompileNoFoldVectorEqW)
LUAU_FASTFLAG(LuauGcTraceUdata)
LUAU_FASTFLAG(LuauBufferCage)
LUAU_DYNAMIC_FASTFLAG(LuauGcHeapShrinkFix)
LUAU_FASTFLAG(DebugLuauUserDefinedClasses)
LUAU_FASTFLAG(DebugLuauUserDefinedClassesRuntime)
LUAU_FASTFLAG(LuauEmitCallFeedback)
LUAU_FASTFLAG(LuauCompileCallFbRemap)
LUAU_FASTFLAG(LuauBytecodeCostModel)
LUAU_FASTFLAG(LuauSandboxFreezesVectorMetatable)
LUAU_FASTFLAG(LuauIntegerType2)
LUAU_FASTFLAG(LuauExportValueSyntax)
LUAU_FASTFLAG(LuauIntegerLibrary)
static bool set_vm_flag(const char* name, bool value = true)
{
    if (!std::strcmp(name, "LuauTableRobustOom")) DFFlag::LuauTableRobustOom.value = value;
    else if (!std::strcmp(name, "LuauCoroutineFinally")) FFlag::LuauCoroutineFinally.value = value;
    else if (!std::strcmp(name, "LuauPcallErrorContinuationReentry")) FFlag::LuauPcallErrorContinuationReentry.value = value;
    else if (!std::strcmp(name, "LuauGcTraceUdata")) FFlag::LuauGcTraceUdata.value = value;
    else if (!std::strcmp(name, "LuauBufferCage")) FFlag::LuauBufferCage.value = value;
    else if (!std::strcmp(name, "LuauGcHeapShrinkFix")) DFFlag::LuauGcHeapShrinkFix.value = value;
    else return false;
    return true;
}
static bool set_compiler_flag(const char* name)
{
    if (!std::strcmp(name, "LuauExperimentalIfLocalSyntax")) FFlag::LuauExperimentalIfLocalSyntax.value = true;
    else if (!std::strcmp(name, "LuauCompileNoFoldVectorEqW")) FFlag::LuauCompileNoFoldVectorEqW.value = true;
    else if (!std::strcmp(name, "LuauIntegerType2")) FFlag::LuauIntegerType2.value = true;
    else if (!std::strcmp(name, "LuauExportValueSyntax")) FFlag::LuauExportValueSyntax.value = true;
    else if (!std::strcmp(name, "LuauEmitCallFeedback")) FFlag::LuauEmitCallFeedback.value = true;
    else if (!std::strcmp(name, "LuauCompileCallFbRemap")) FFlag::LuauCompileCallFbRemap.value = true;
    else return false;
    return true;
}
static bool set_library_flag(const char* name, bool value = true)
{
    if (!std::strcmp(name, "LuauSandboxFreezesVectorMetatable")) FFlag::LuauSandboxFreezesVectorMetatable.value = value;
    else if (!std::strcmp(name, "LuauIntegerLibrary")) FFlag::LuauIntegerLibrary.value = value;
    else return false;
    return true;
}
static bool set_class_flags(bool value)
{
    FFlag::DebugLuauUserDefinedClasses.value = value;
    FFlag::DebugLuauUserDefinedClassesRuntime.value = value;
    FFlag::LuauEmitCallFeedback.value = value;
    FFlag::LuauBytecodeCostModel.value = value;
    return true;
}
#endif

static int optimizationLevel = 1;
static int debugLevel = 1;
static int coverageLevel = 0;
static const char* currentCase = "";

static int compile(const char* source, size_t size, bool testOptions, char** bytes, size_t* count)
{
#ifdef LUAU_ZIG_CANDIDATE
    return zig_conformance_compile(source, size, testOptions ? optimizationLevel : 1, testOptions ? debugLevel : 1, testOptions ? 1 : 0, testOptions ? coverageLevel : 0, currentCase, bytes, count);
#else
    lua_CompileOptions options = {};
    options.optimizationLevel = optimizationLevel;
    options.debugLevel = debugLevel;
    options.typeInfoLevel = 1;
    options.coverageLevel = coverageLevel;
    options.vectorPrecision = LUA_VECTOR_DOUBLE;
    static const char* userdataTypes[] = {"vec2", "color", "mat3", "vertex", nullptr};
    if (testOptions && !std::strcmp(currentCase, "NativeUserdata"))
        options.userdataTypes = userdataTypes;
    *bytes = luau_compile(source, size, testOptions ? &options : nullptr, count);
    return *bytes ? 0 : 2;
#endif
}

static int collect(lua_State* L)
{
    static const char* const opts[] = {"stop", "restart", "collect", "count", "isrunning", "step", "setgoal", "setstepmul", "setstepsize", nullptr};
    static const int codes[] = {LUA_GCSTOP, LUA_GCRESTART, LUA_GCCOLLECT, LUA_GCCOUNT, LUA_GCISRUNNING, LUA_GCSTEP, LUA_GCSETGOAL, LUA_GCSETSTEPMUL, LUA_GCSETSTEPSIZE};
    int op = luaL_checkoption(L, 1, "collect", opts);
    int result = lua_gc(L, codes[op], luaL_optinteger(L, 2, 0));
    if (codes[op] == LUA_GCSTEP || codes[op] == LUA_GCISRUNNING) lua_pushboolean(L, result);
    else lua_pushnumber(L, result);
    return 1;
}

static int loadstring_test(lua_State* L)
{
    size_t length = 0;
    const char* source = luaL_checklstring(L, 1, &length);
    const char* name = luaL_optstring(L, 2, source);
    lua_setsafeenv(L, LUA_ENVIRONINDEX, false);
    char* bytes = nullptr;
    size_t size = 0;
    int result = compile(source, length, false, &bytes, &size);
    if (result) luaL_error(L, "conformance compiler unavailable (%d)", result);
    result = luau_load(L, name, bytes, size, 0);
    std::free(bytes);
    if (!result) return 1;
    lua_pushnil(L);
    lua_insert(L, -2);
    return 2;
}

static int silence(lua_State*) { return 0; }

#include "upstream_cyield.inc"
#include "upstream_userdata.inc"
#include "upstream_coverage.inc"
#if LUA_VECTOR_SIZE == 4
#define lua_pushvector3(L, x, y, z) lua_pushvector(L, x, y, z, 0.0)
#else
#define lua_pushvector3(L, x, y, z) lua_pushvector(L, x, y, z)
#endif
#include "upstream_vector.inc"

// These reproduce the interpreter-only setupNativeHelpers behavior: no native
// compiler is enabled, so is_native is false and is_native_if_supported is true.
static int is_native(lua_State* L) { lua_pushboolean(L, 0); return 1; }
static int is_native_if_supported(lua_State* L) { lua_pushboolean(L, 1); return 1; }
static void setup_native_helpers(lua_State* L)
{
    lua_pushcfunction(L, is_native, "is_native"); lua_setglobal(L, "is_native");
    lua_pushcfunction(L, is_native_if_supported, "is_native_if_supported"); lua_setglobal(L, "is_native_if_supported");
}
static bool allocationsAllowed = true;
static unsigned allocationSizeLimit = 0;
static void* blockable_alloc(void*, void* p, size_t, size_t n)
{
    if (!n) { std::free(p); return nullptr; }
    return allocationsAllowed ? std::realloc(p, n) : nullptr;
}
static void* limited_alloc(void*, void* p, size_t, size_t n)
{
    if (!n) { std::free(p); return nullptr; }
    return n > (allocationSizeLimit ? allocationSizeLimit : 8u * 1024u * 1024u) ? nullptr : std::realloc(p, n);
}
static int block_allocations(lua_State* L)
{
    allocationsAllowed = !luaL_checkboolean(L, 1);
    return 0;
}
static int set_allocation_limit(lua_State* L)
{
    allocationSizeLimit = luaL_checkunsigned(L, 1);
    return 0;
}
static int make_lightuserdata(lua_State* L)
{
    if (lua_type(L, 1) == LUA_TNUMBER)
        lua_pushlightuserdata(L, reinterpret_cast<void*>(uintptr_t(luaL_checkunsigned(L, 1))));
    else {
        const void* p = lua_topointer(L, 1);
        LUAU_ASSERT(p);
        lua_pushlightuserdata(L, const_cast<void*>(p));
    }
    return 1;
}
static int run_finalizers(lua_State* L, bool toclose)
{
    lua_State* co = lua_tothread(L, 1);
    lua_State* thread = lua_newthread(L);
    lua_pushfinalizerfunction(thread, co, toclose ? 1 : 0);
    int status = lua_resume(thread, nullptr, 0);
    int results = status == LUA_OK ? lua_gettop(thread) : 1;
    lua_pushboolean(L, status == LUA_OK);
    lua_xmove(thread, L, results);
    return results + 1;
}
static int host_resume(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTHREAD);
    lua_State* co = lua_tothread(L, 1);
    int status = lua_resume(co, nullptr, 0);
    if (status == LUA_YIELD || status == LUA_BREAK) return 0;
    if (lua_hasfinalizers(co)) return run_finalizers(L, false);
    return 0;
}
static int host_cancel(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTHREAD);
    lua_State* co = lua_tothread(L, 1);
    if (lua_hasfinalizers(co)) return run_finalizers(L, true);
    lua_resetthread(co);
    lua_pushboolean(L, true);
    return 1;
}
static int throw_error(lua_State* L) { luaL_error(L, "oops"); }
static int resume_error(lua_State* L)
{
    lua_State* co = lua_tothread(L, 1);
    lua_xmove(L, co, 1);
    lua_resumeerror(co, L);
    return 0;
}
static int iterator_continuation(lua_State* L, int)
{
    int index = luaL_checkinteger(L, 2);
    lua_pushinteger(L, index + 1);
    lua_pushinteger(L, index + 1);
    return 2;
}
static int yielding_iterator(lua_State* L)
{
    int max = luaL_checkinteger(L, 1);
    int index = luaL_checkinteger(L, 2);
    if (index >= max) return 0;
    lua_pushinteger(L, index + 1);
    return lua_yield(L, 1);
}
static void setup_case(lua_State* L, const char* name)
{
    if (!std::strcmp(name, "Buffers") || !std::strcmp(name, "BuffersWithCage") || !std::strcmp(name, "Iter") || !std::strcmp(name, "VectorLibrary") || !std::strcmp(name, "Vector") || !std::strcmp(name, "Integers")) setup_native_helpers(L);
    if (!std::strcmp(name, "Vector")) setupVectorHelpers(L);
    if (!std::strcmp(name, "GC")) {
        lua_pushcfunction(L, block_allocations, "setblockallocations");
        lua_setglobal(L, "setblockallocations");
    }
    if (!std::strcmp(name, "Tables")) {
        lua_pushcfunction(L, make_lightuserdata, "makelud"); lua_setglobal(L, "makelud");
        lua_pushcfunction(L, set_allocation_limit, "setallocationlimit"); lua_setglobal(L, "setallocationlimit");
    }
    if (!std::strcmp(name, "CoroutineHost")) {
        lua_pushcfunction(L, host_resume, "hostresume"); lua_setglobal(L, "hostresume");
        lua_pushcfunction(L, host_cancel, "hostcancel"); lua_setglobal(L, "hostcancel");
    }
    if (!std::strcmp(name, "CYield")) setup_cyield(L);
    if (!std::strcmp(name, "Userdata")) setup_userdata(L);
    if (!std::strcmp(name, "Coverage")) setup_coverage(L);
    if (!std::strcmp(name, "PCall")) {
        lua_pushcfunction(L, throw_error, "cxxthrow"); lua_setglobal(L, "cxxthrow");
        lua_pushcfunction(L, resume_error, "resumeerror"); lua_setglobal(L, "resumeerror");
    }
    if (!std::strcmp(name, "Iter")) {
        lua_pushcclosurek(L, yielding_iterator, "cYieldingIterator", 0, iterator_continuation);
        lua_setglobal(L, "cYieldingIterator");
    }
}

static char* read_file(const char* path, size_t* size)
{
    FILE* f = std::fopen(path, "rb");
    if (!f) return nullptr;
    long n = std::fseek(f, 0, SEEK_END) ? -1 : std::ftell(f);
    if (n < 0 || std::fseek(f, 0, SEEK_SET)) { std::fclose(f); return nullptr; }
    char* data = static_cast<char*>(std::malloc(static_cast<size_t>(n) + 1));
    if (!data) { std::fclose(f); return nullptr; }
    if (std::fread(data, 1, static_cast<size_t>(n), f) != static_cast<size_t>(n)) {
        std::free(data); std::fclose(f); return nullptr;
    }
    std::fclose(f);
    data[n] = 0;
    *size = static_cast<size_t>(n);
    return data;
}

static bool write_file(const char* path, const char* bytes, size_t size)
{
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    bool ok = std::fwrite(bytes, 1, size, f) == size;
    return std::fclose(f) == 0 && ok;
}

#ifndef LUAU_CONFORMANCE_EMBED_HOST
int main(int argc, char** argv)
{
    if ((argc != 5 && argc != 7) || std::strlen(argv[4]) != 1 || argv[4][0] < '0' || argv[4][0] > '2' ||
        (argc == 7 && std::strcmp(argv[5], "--write-bytecode") && std::strcmp(argv[5], "--read-bytecode"))) {
        std::fprintf(stderr, "usage: upstream_script_host SOURCE CHUNK_NAME CASE OPTIMIZATION(0..2) [--write-bytecode|--read-bytecode PATH]\n"); return 2;
    }
    optimizationLevel = argv[4][0] - '0';
    currentCase = argv[3];
    if ((!std::strcmp(currentCase, "FeedbackEnabled") || !std::strcmp(currentCase, "InlinerEnabled")) &&
        (!set_compiler_flag("LuauEmitCallFeedback") || !set_compiler_flag("LuauCompileCallFbRemap"))) {
        std::fprintf(stderr, "feedback compiler flags unavailable\n"); return 77;
    }
    if (!std::strcmp(currentCase, "Classes") && !set_class_flags(true)) {
        std::fprintf(stderr, "class runtime flag unavailable\n"); return 77;
    }
    if (!std::strcmp(currentCase, "Integers") && !set_library_flag("LuauIntegerLibrary")) {
        std::fprintf(stderr, "integer library flag unavailable\n"); return 77;
    }
    const char* compilerFlag = !std::strcmp(currentCase, "IfLocal") ? "LuauExperimentalIfLocalSyntax" :
        !std::strcmp(currentCase, "Vector") ? "LuauCompileNoFoldVectorEqW" :
        (!std::strcmp(currentCase, "Integers") || !std::strcmp(currentCase, "NativeIntegerSpills") ||
         !std::strcmp(currentCase, "Types")) ? "LuauIntegerType2" : nullptr;
    if (compilerFlag && !set_compiler_flag(compilerFlag)) { std::fprintf(stderr, "unsupported compiler flag %s\n", compilerFlag); return 77; }
    if (!std::strcmp(argv[3], "Coverage")) { optimizationLevel = 1; coverageLevel = 2; }
    const char* flag = !std::strcmp(argv[3], "BuffersWithCage") ? "LuauBufferCage" :
        !std::strcmp(argv[3], "Tables") ? "LuauTableRobustOom" :
        (!std::strcmp(argv[3], "Coroutine") || !std::strcmp(argv[3], "CoroutineHost")) ? "LuauCoroutineFinally" :
        !std::strcmp(argv[3], "CYield") ? "LuauPcallErrorContinuationReentry" : nullptr;
    if (flag && !set_vm_flag(flag)) { std::fprintf(stderr, "unsupported VM flag %s\n", flag); return 77; }
#ifdef LUAU_BUFFER_CAGE_SUPPORTED
    std::unique_ptr<BufferCage> cage;
    if (!std::strcmp(currentCase, "BuffersWithCage")) {
        cage = std::make_unique<BufferCage>();
        if (!cage->base) { std::fprintf(stderr, "buffer cage reservation unavailable\n"); return 77; }
    }
#else
    if (!std::strcmp(currentCase, "BuffersWithCage")) {
        std::fprintf(stderr, "buffer cage unsupported on this platform\n"); return 77;
    }
#endif
    char* bytecode = nullptr;
    size_t bytecodeSize = 0;
    if (argc == 7 && !std::strcmp(argv[5], "--read-bytecode")) {
        bytecode = read_file(argv[6], &bytecodeSize);
        if (!bytecode || !bytecodeSize) { std::free(bytecode); std::fprintf(stderr, "cannot read bytecode\n"); return 2; }
    } else {
        size_t size = 0;
        char* source = read_file(argv[1], &size);
        if (!source) { std::fprintf(stderr, "cannot read fixture\n"); return 2; }
        int status = compile(source, size, true, &bytecode, &bytecodeSize);
        std::free(source);
        if (status) { std::fprintf(stderr, "compiler prerequisite unavailable (%d)\n", status); return status; }
        if (argc == 7 && !write_file(argv[6], bytecode, bytecodeSize)) {
            std::free(bytecode); std::fprintf(stderr, "cannot write bytecode\n"); return 2;
        }
    }
    int status = 0;
    lua_State* L = !std::strcmp(argv[3], "GC") ? lua_newstate(blockable_alloc, nullptr) :
                   (!std::strcmp(argv[3], "PCall") || !std::strcmp(argv[3], "Tables")) ? lua_newstate(limited_alloc, nullptr) : luaL_newstate();
    if (!L) { std::free(bytecode); std::fprintf(stderr, "cannot create VM\n"); return 2; }
    luaL_openlibs(L);
    luaL_Reg funcs[] = {{"collectgarbage", collect}, {"loadstring", loadstring_test}, {"print", silence}, {nullptr, nullptr}};
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    luaL_register(L, nullptr, funcs);
    lua_pop(L, 1);
    setup_case(L, argv[3]);
    if (!std::strcmp(currentCase, "Types")) {
        const char* path = std::getenv("LUAU_TEST_RTTI_SOURCE");
        size_t rttiSize = 0;
        char* rttiSource = path ? read_file(path, &rttiSize) : nullptr;
        if (!rttiSource) {
            std::fprintf(stderr, "upstream frontend RTTI input unavailable\n");
            std::free(bytecode); lua_close(L); return 77;
        }
        char* rttiCode = nullptr;
        size_t rttiCodeSize = 0;
        int rttiStatus = compile(rttiSource, rttiSize, false, &rttiCode, &rttiCodeSize);
        std::free(rttiSource);
        if (!rttiStatus) rttiStatus = luau_load(L, "=upstream-rtti", rttiCode, rttiCodeSize, 0);
        std::free(rttiCode);
        if (!rttiStatus) rttiStatus = lua_pcall(L, 0, 1, 0);
        if (rttiStatus || !lua_istable(L, -1)) {
            std::fprintf(stderr, "upstream frontend RTTI input failed (status=%d)\n", rttiStatus);
            std::free(bytecode); lua_close(L); return 1;
        }
        lua_setglobal(L, "RTTI");
    }
#ifdef LUAU_BUFFER_CAGE_SUPPORTED
    if (cage) lua_setmemorycage(L, &BufferCage::frealloc, cage.get());
#endif
    luaL_sandbox(L);
    luaL_sandboxthread(L);
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_setfield(L, -1, "_G");
    status = luau_load(L, argv[2], bytecode, bytecodeSize, 0);
    std::free(bytecode);
    if (!status) status = lua_resume(L, nullptr, 0);
    if (!std::strcmp(currentCase, "InlinerDefault") || !std::strcmp(currentCase, "InlinerEnabled")) {
        if (status || lua_gettop(L) != 1 || !lua_isnumber(L, -1)) {
            const char* error = lua_tostring(L, -1);
            std::fprintf(stderr, "status=%d result=%s\n", status, error ? error : "(non-number)");
            lua_close(L);
            return 1;
        }
        double result = lua_tonumber(L, -1);
        lua_close(L);
        std::printf("result=%.17g\n", result);
        return 0;
    }
    if (!std::strcmp(currentCase, "FeedbackDefault") || !std::strcmp(currentCase, "FeedbackEnabled") ||
        !std::strcmp(currentCase, "BytecodeDistribution")) {
        if (status) {
            const char* error = lua_tostring(L, -1);
            std::fprintf(stderr, "status=%d result=%s\n", status, error ? error : "(non-string)");
            lua_close(L);
            return 1;
        }
        int count = lua_gettop(L);
        lua_close(L);
        std::printf("results=%d\n", count);
        return count == 0 ? 0 : 1;
    }
    if (status || !lua_isstring(L, -1) || std::strcmp(lua_tostring(L, -1), "OK")) {
        const char* error = lua_tostring(L, -1);
        std::fprintf(stderr, "status=%d result=%s\nstacktrace:\n%s\n", status, error ? error : "(non-string)", lua_debugtrace(L));
        lua_close(L);
        return 1;
    }
#ifdef LUAU_BUFFER_CAGE_SUPPORTED
    if (cage && cage->lastAllocationType != LUA_TBUFFER) {
        std::fprintf(stderr, "buffer cage did not receive a buffer allocation\n");
        lua_close(L);
        return 1;
    }
#endif
    lua_close(L);
    std::puts("OK");
    return 0;
}
#endif
