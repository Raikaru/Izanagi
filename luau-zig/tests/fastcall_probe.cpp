// Test-only differential host for FASTCALL. Both builds use the same protocol;
// the candidate links only the Zig compiler bridge and Zig VM.
#include "lua.h"
#include "lualib.h"
#include "luacode.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef LUAU_ZIG_CANDIDATE
extern "C" int zig_conformance_compile(const char*, size_t, int, int, int, int, const char*, char**, size_t*);
extern "C" int luau_setfflag(const char*, int);
extern "C" int luau_stdlib_setflag(const char*, int);
#else
#include "Luau/Common.h"
#endif

static unsigned interrupts;
static void on_interrupt(lua_State*, int gc) { if (gc < 0) ++interrupts; }

static std::string read_file(const char* path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error(std::string("cannot read ") + path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

static std::string hex(const void* bytes, size_t size)
{
    static const char digits[] = "0123456789abcdef";
    const auto* p = static_cast<const unsigned char*>(bytes);
    std::string out;
    out.reserve(size * 2);
    for (size_t i = 0; i < size; ++i) {
        out.push_back(digits[p[i] >> 4]);
        out.push_back(digits[p[i] & 15]);
    }
    return out;
}

static std::string value(lua_State* L, int index, std::set<const void*>& ancestors)
{
    index = lua_absindex(L, index);
    switch (lua_type(L, index)) {
    case LUA_TNIL: return "nil";
    case LUA_TBOOLEAN: return lua_toboolean(L, index) ? "bool:true" : "bool:false";
    case LUA_TNUMBER: {
        double number = lua_tonumber(L, index);
        uint64_t bits;
        std::memcpy(&bits, &number, sizeof(bits));
        std::ostringstream out;
        out << "number:" << std::hex << std::setw(16) << std::setfill('0') << bits;
        return out.str();
    }
    case LUA_TINTEGER: {
        int ok = 0;
        int64_t number = lua_tointeger64(L, index, &ok);
        if (!ok) throw std::runtime_error("invalid integer result");
        return "integer:" + std::to_string(number);
    }
    case LUA_TSTRING: {
        size_t size = 0;
        const char* data = lua_tolstring(L, index, &size);
        return "string:" + hex(data, size);
    }
    case LUA_TVECTOR: {
        const LUA_VECTOR_TYPE* v = lua_tovector(L, index);
        if (!v) throw std::runtime_error("invalid vector result");
        std::string result = "vector:";
        for (int i = 0; i < LUA_VECTOR_SIZE; ++i) {
            uint32_t bits;
            std::memcpy(&bits, &v[i], sizeof(bits));
            std::ostringstream out;
            out << std::hex << std::setw(8) << std::setfill('0') << bits;
            result += out.str() + ",";
        }
        return result;
    }
    case LUA_TBUFFER: {
        size_t size = 0;
        const void* data = lua_tobuffer(L, index, &size);
        if (!data && size) throw std::runtime_error("invalid buffer result");
        return "buffer:" + hex(data, size);
    }
    case LUA_TTABLE: {
        const void* identity = lua_topointer(L, index);
        if (!ancestors.insert(identity).second) return "table:<cycle>";
        std::vector<std::string> entries;
        lua_pushnil(L);
        while (lua_next(L, index)) {
            std::string key = value(L, -2, ancestors);
            std::string item = value(L, -1, ancestors);
            entries.push_back(key + "=" + item);
            lua_pop(L, 1);
        }
        ancestors.erase(identity);
        std::sort(entries.begin(), entries.end());
        std::string result = "table:{";
        for (const std::string& entry : entries) result += entry + ";";
        return result + "}";
    }
    default:
        throw std::runtime_error(std::string("unsupported result type: ") + lua_typename(L, lua_type(L, index)));
    }
}

static void set_flags(const std::string& flags)
{
    size_t start = 0;
    while (start < flags.size()) {
        size_t end = flags.find(',', start);
        if (end == std::string::npos) end = flags.size();
        std::string name = flags.substr(start, end - start);
        if (name.empty()) throw std::runtime_error("empty feature flag");
#ifdef LUAU_ZIG_CANDIDATE
        // Compiler-only flags are validated by zig_conformance_compile.
        (void)luau_setfflag(name.c_str(), 1);
        (void)luau_stdlib_setflag(name.c_str(), 1);
#else
        bool found = false;
        for (Luau::FValue<bool>* flag = Luau::FValue<bool>::list; flag; flag = flag->next) {
            if (name == flag->name) { flag->value = true; found = true; }
        }
        if (!found) throw std::runtime_error("unknown reference flag: " + name);
#endif
        start = end + 1;
    }
}

static std::vector<char> compile(const std::string& source, int id, const std::string& flags)
{
    size_t size = 0;
    char* bytes = nullptr;
#ifdef LUAU_ZIG_CANDIDATE
    std::string config = "Fastcall:" + std::to_string(id);
    if (!flags.empty()) config += "," + flags;
    int status = zig_conformance_compile(source.data(), source.size(), 1, 1, 1, 0, config.c_str(), &bytes, &size);
    if (status || !bytes || !size) throw std::runtime_error("candidate compiler failed: " + std::to_string(status));
#else
    lua_CompileOptions options = {};
    options.optimizationLevel = 1;
    options.debugLevel = 1;
    options.typeInfoLevel = 1;
    options.vectorPrecision = LUA_VECTOR_DOUBLE;
    if (id == 54) { options.vectorLib = "vector"; options.vectorCtor = "create"; }
    bytes = luau_compile(source.data(), source.size(), &options, &size);
    if (!bytes || !size) throw std::runtime_error("reference compiler failed");
#endif
    std::vector<char> result(bytes, bytes + size);
    std::free(bytes);
    return result;
}

int main(int argc, char** argv)
{
    try {
        if (argc != 8) throw std::runtime_error("expected setup source bytecode safe id flags observe");
        std::string setup = read_file(argv[1]);
        std::string source = read_file(argv[2]);
        bool safe = std::strcmp(argv[4], "1") == 0;
        if (!safe && std::strcmp(argv[4], "0") != 0) throw std::runtime_error("invalid safeenv mode");
        int id = std::stoi(argv[5]);
        if (id < 1 || id > 134) throw std::runtime_error("invalid builtin id");
        std::string flags = argv[6];
        set_flags(flags);
        lua_State* L = luaL_newstate();
        if (!L) throw std::runtime_error("cannot create VM");
        luaL_openlibs(L);
        luaL_sandbox(L);
        luaL_sandboxthread(L);
        std::vector<char> setupCode = compile(setup, id, flags);
        int setupStatus = luau_load(L, "=fastcall-setup", setupCode.data(), setupCode.size(), 0);
        if (!setupStatus) setupStatus = lua_pcall(L, 0, 0, 0);
        if (setupStatus) {
            const char* message = lua_tostring(L, -1);
            std::cerr << "setup failed: " << (message ? message : "(non-string)") << "\n";
            lua_close(L);
            return 2;
        }
        std::vector<char> bytes = compile(source, id, flags);
        std::ofstream output(argv[3], std::ios::binary);
        output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        output.close();
        if (!output) throw std::runtime_error("cannot write bytecode");
        lua_setsafeenv(L, LUA_GLOBALSINDEX, safe ? 1 : 0);
        int status = luau_load(L, "=fastcall-case", bytes.data(), bytes.size(), 0);
        interrupts = 0;
        lua_callbacks(L)->interrupt = on_interrupt;
        if (!status) {
            lua_pushnumber(L, 10);
            lua_pushnumber(L, 20);
            lua_pushnumber(L, 30);
            status = lua_resume(L, nullptr, 3);
        }
        lua_callbacks(L)->interrupt = nullptr;
        std::cout << "interrupts=" << interrupts << "\nstatus=" << status << "\n";
        if (id == 1 && status != 0) {
            const char* trace = lua_debugtrace(L);
            if (!trace) throw std::runtime_error("missing assertion traceback");
            std::cout << "trace=" << hex(trace, std::strlen(trace)) << "\n";
        }
        if (status != 0) {
            // lua_resume leaves failed frames and locals on the stack. The last
            // value is the actual error; keep it after capturing the traceback.
            lua_replace(L, 1);
            lua_settop(L, 1);
        }
        int count = lua_gettop(L);
        std::cout << "results=" << count << "\n";
        std::set<const void*> ancestors;
        for (int i = 1; i <= count; ++i) std::cout << "result=" << value(L, i, ancestors) << "\n";
        std::string observe = argv[7];
        size_t start = 0;
        while (start < observe.size()) {
            size_t end = observe.find(',', start);
            if (end == std::string::npos) end = observe.size();
            std::string name = observe.substr(start, end - start);
            lua_getglobal(L, name.c_str());
            std::cout << "global=" << name << ":" << value(L, -1, ancestors) << "\n";
            lua_pop(L, 1);
            start = end + 1;
        }
        lua_close(L);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 2;
    }
}
