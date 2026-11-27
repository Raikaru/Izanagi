// Test-only adapter over the pinned upstream compiler and VM; not an engine or
// a production bytecode admission path. The linked Luau code retains its MIT
// license in third_party/luau/LICENSE.txt and lua_LICENSE.txt.
#include "p4pc/luau_c.h"
#include "luacode.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

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

static int emit(lua_State* L)
{
    size_t length;
    const char* text = luaL_checklstring(L, 1, &length);
    std::fwrite(text, 1, length, stdout);
    std::fputc('\n', stdout);
    return 0;
}

static void initialize(lua_State* L, void*)
{
    lua_pushcfunction(L, emit, "emit");
    lua_setglobal(L, "emit");
}

struct Input { const char* bytes; size_t size; int status; int ref; };
static int load_chunk(lua_State* L)
{
    Input* input = static_cast<Input*>(lua_touserdata(L, 1));
    input->status = luau_load(L, "@bytecode-fixture", input->bytes, input->size, 0);
    if (input->status) {
        const char* error = lua_tostring(L, -1);
        std::fprintf(stderr, "load: %s\n", error ? error : "non-string error");
    } else {
        input->ref = lua_ref(L, -1);
    }
    return 0;
}

int main(int argc, char** argv)
{
    if ((argc != 4 || std::strcmp(argv[1], "compile")) &&
        (argc != 3 || std::strcmp(argv[1], "run"))) {
        std::fprintf(stderr, "usage: reference_bytecode compile SOURCE OUTPUT | run BYTECODE\n");
        return 2;
    }
    size_t size = 0;
    char* bytes = read_file(argv[2], &size);
    if (!bytes) { std::fprintf(stderr, "cannot read input\n"); return 2; }
    if (!std::strcmp(argv[1], "compile")) {
        size_t outsize = 0;
        char* out = luau_compile(bytes, size, nullptr, &outsize);
        std::free(bytes);
        if (!out) { std::fprintf(stderr, "compiler allocation failure\n"); return 2; }
        if (!outsize || !out[0]) {
            std::fprintf(stderr, "compile: %.*s\n", static_cast<int>(outsize ? outsize - 1 : 0), out + (outsize != 0));
            std::free(out);
            return 1;
        }
        FILE* f = std::fopen(argv[3], "wb");
        if (!f) { std::free(out); std::fprintf(stderr, "cannot open output\n"); return 2; }
        bool failed = std::fwrite(out, 1, outsize, f) != outsize;
        failed = std::fclose(f) != 0 || failed;
        std::free(out);
        if (failed) { std::fprintf(stderr, "cannot write output\n"); return 2; }
        return 0;
    }
    p4_luau_config config = {256u * 1024u * 1024u, nullptr, nullptr, initialize, 4u * 1024u * 1024u};
    p4_luau* vm = p4_luau_create(&config);
    if (!vm) { std::free(bytes); std::fprintf(stderr, "cannot create VM\n"); return 2; }
    lua_State* L = p4_luau_state(vm);
    Input input = {bytes, size, 0, LUA_NOREF};
    int status = lua_cpcall(L, load_chunk, &input);
    std::free(bytes);
    if (status || input.status) {
        if (status) std::fprintf(stderr, "load: %s\n", lua_tostring(L, -1));
        p4_luau_destroy(vm);
        return 1;
    }
    lua_getref(L, input.ref);
    lua_unref(L, input.ref);
    status = p4_luau_run(vm, 0, LUA_MULTRET);
    if (status) std::fprintf(stderr, "run: %s\n", p4_luau_error(vm));
    p4_luau_destroy(vm);
    return status ? 1 : 0;
}
