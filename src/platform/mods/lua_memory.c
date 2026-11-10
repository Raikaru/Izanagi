#include "lua_runtime.h"
#include "p4pc/guest.h"
#include "p4pc/mod_memory.h"
#include "p4pc/mod_numbers.h"
#include "p4pc/ps2_libc.h"
#include <stdlib.h>
#include <string.h>

typedef struct Buffer {
    P4LuaMod *owner;
    uint32_t address;
    size_t size, references;
    struct Buffer *next;
} Buffer;
typedef struct BufferHandle { Buffer *buffer; } BufferHandle;
typedef struct FunctionLease {
    P4ModMemoryRegion *region;
    struct FunctionLease *next;
} FunctionLease;
typedef struct Pin {
    P4LuaMod *owner;
    Buffer *buffer;
    FunctionLease *functions;
    uint32_t address;
    size_t size;
    int readonly;
} Pin;
typedef struct FunctionReference { uint32_t word; } FunctionReference;
static Buffer *buffers;

static void release(Buffer *buffer)
{
    if (buffer == NULL || --buffer->references != 0) return;
    Buffer **link = &buffers;
    while (*link != NULL && *link != buffer) link = &(*link)->next;
    if (*link != NULL) *link = buffer->next;
    buffer->owner->scratch_bytes -= buffer->size;
    p4pc_bytes_invalidate(P4_G2H(buffer->address), (uint32_t)buffer->size);
    p4_guest_stack_free(P4_G2H(buffer->address));
    free(buffer);
}

static void destroy_buffer(lua_State *L, void *userdata)
{
    (void)L;
    BufferHandle *handle = userdata;
    release(handle->buffer);
    handle->buffer = NULL;
}

static void destroy_pin(lua_State *L, void *userdata)
{
    (void)L;
    Pin *pin = userdata;
    while (pin->functions != NULL) {
        FunctionLease *lease = pin->functions;
        pin->functions = lease->next;
        p4_mod_memory_revoke(lease->region);
        free(lease);
    }
    release(pin->buffer);
    pin->buffer = NULL;
}

static Buffer *checked_buffer(lua_State *L, int index)
{
    BufferHandle *handle = p4_lua_userdata(L, index, P4_LUA_TAG_BUFFER, "guest buffer");
    if (handle->buffer == NULL || handle->buffer->owner != p4_luau_user(L))
        luaL_error(L, "guest buffer is released or belongs to another mod");
    return handle->buffer;
}

static Pin *checked_pin(lua_State *L, int index)
{
    Pin *pin = p4_lua_userdata(L, index, P4_LUA_TAG_PIN, "guest view pin");
    if (pin->owner != p4_luau_user(L)) luaL_error(L, "guest pin belongs to another mod");
    return pin;
}

static int release_buffer(lua_State *L)
{
    p4_lua_require_mutation(L);
    BufferHandle *handle = p4_lua_userdata(L, 1, P4_LUA_TAG_BUFFER, "guest buffer");
    destroy_buffer(L, handle);
    return 0;
}

static int buffer_index(lua_State *L)
{
    Buffer *buffer = checked_buffer(L, 1);
    const char *key = luaL_checkstring(L, 2);
    if (!strcmp(key, "address")) lua_pushunsigned(L, buffer->address);
    else if (!strcmp(key, "size")) lua_pushunsigned(L, (unsigned)buffer->size);
    else if (!strcmp(key, "release"))
        lua_getref(L, ((P4LuaMod *)p4_luau_user(L))->buffer_release_reference);
    else lua_pushnil(L);
    return 1;
}

static int pin_index(lua_State *L)
{
    Pin *pin = checked_pin(L, 1);
    const char *key = luaL_checkstring(L, 2);
    if (!strcmp(key, "address")) lua_pushunsigned(L, pin->address);
    else if (!strcmp(key, "size")) lua_pushunsigned(L, (unsigned)pin->size);
    else lua_pushnil(L);
    return 1;
}

static int allocate_buffer(lua_State *L)
{
    p4_lua_require_mutation(L);
    size_t size = (size_t)p4_lua_integer(L, 1, 1, 1024 * 1024);
    P4LuaMod *mod = p4_luau_user(L);
    if (size > 4u * 1024u * 1024u - mod->scratch_bytes)
        luaL_error(L, "mod scratch memory limit exceeded");
    BufferHandle *handle = lua_newuserdatataggedwithmetatable(L, sizeof *handle, P4_LUA_TAG_BUFFER);
    handle->buffer = NULL;
    Buffer *buffer = malloc(sizeof *buffer);
    if (buffer == NULL) luaL_error(L, "guest buffer metadata allocation failed");
    void *host = p4_guest_stack_alloc(size);
    if (host == NULL) { free(buffer); luaL_error(L, "guest scratch allocation failed"); }
    func_0043f9c8(host, 0, (uint32_t)size);
    *buffer = (Buffer){mod, P4_H2G(host), size, 1, buffers};
    buffers = buffer;
    mod->scratch_bytes += size;
    handle->buffer = buffer;
    return 1;
}

typedef struct Address { uint32_t value; Buffer *buffer; } Address;
static Address address(lua_State *L, int index, size_t size)
{
    if (lua_userdatatag(L, index) == P4_LUA_TAG_BUFFER) {
        Buffer *buffer = checked_buffer(L, index);
        if (size > buffer->size) luaL_error(L, "guest buffer span exceeds allocation");
        return (Address){buffer->address, buffer};
    }
    if (lua_userdatatag(L, index) == P4_LUA_TAG_PIN) {
        Pin *pin = checked_pin(L, index);
        if (size > pin->size) luaL_error(L, "guest span exceeds view pin");
        return (Address){pin->address, pin->buffer};
    }
    uint32_t a = (uint32_t)p4_lua_integer(L, index, 0, UINT32_MAX);
    P4LuaMod *mod = p4_luau_user(L);
    for (Buffer *buffer = buffers; buffer != NULL; buffer = buffer->next) {
        uint64_t offset = (uint64_t)a - buffer->address;
        if (offset <= buffer->size && size <= buffer->size - (size_t)offset) {
            if (buffer->owner != mod) luaL_error(L, "guest scratch belongs to another mod");
            return (Address){a, buffer};
        }
    }
    return (Address){a, NULL};
}

static unsigned char *memory(lua_State *L, Address a, size_t size, int write, const void *bytes)
{
    if(write)p4_lua_require_mutation(L);
    const char *error = p4_mod_memory_check(a.value, size, write, a.buffer != NULL, bytes);
    if (error != NULL) luaL_error(L, "%s", error);
    if(write&&size&&!a.buffer)p4_lua_semantic_use(p4_luau_user(L));
    return P4_G2H(a.value);
}

static int pin_span(lua_State *L)
{
    p4_lua_require_mutation(L);
    size_t size = (size_t)p4_lua_integer(L, 2, 1, 1024 * 1024);
    int readonly = lua_toboolean(L, 3);
    /* Allocate before resolving a numeric scratch address: GC may collect an
     * otherwise unreachable buffer, but can never leave a stale native pin. */
    Pin *pin = lua_newuserdatataggedwithmetatable(L, sizeof *pin, P4_LUA_TAG_PIN);
    memset(pin, 0, sizeof *pin);
    Address a = address(L, 1, size);
    (void)memory(L, a, size, 0, NULL);
    pin->owner = p4_luau_user(L);
    pin->address = a.value;
    pin->size = size;
    pin->readonly = readonly;
    pin->buffer = a.buffer;
    if (a.buffer != NULL) ++a.buffer->references;
    return 1;
}

static Address pinned_address(lua_State *L, size_t size, int write)
{
    Pin *pin = checked_pin(L, 1);
    size_t offset = (size_t)p4_lua_integer(L, 2, 0, 1024 * 1024);
    if (offset > pin->size || size > pin->size - offset)
        luaL_error(L, "field exceeds pinned guest layout");
    if (write && pin->readonly) luaL_error(L, "guest view is readonly");
    uint64_t a = (uint64_t)pin->address + offset;
    if (a > UINT32_MAX) luaL_error(L, "guest field address overflow");
    return (Address){(uint32_t)a, pin->buffer};
}

static int declare_functions(lua_State *L)
{
    p4_lua_require_mutation(L);
    Pin *pin = checked_pin(L, 1);
    size_t count = (size_t)p4_lua_integer(L, 3, 1, 262144);
    Address a = pinned_address(L, count * 4, 0);
    (void)memory(L, a, count * 4, 0, NULL);
    FunctionLease *lease = malloc(sizeof *lease);
    if (lease == NULL) luaL_error(L, "function field declaration allocation failed");
    lease->region = p4_mod_memory_declare(a.value, count * 4, P4_MOD_MEMORY_FUNCTION_WORDS);
    if (lease->region == NULL) {
        free(lease);
        luaL_error(L, "function fields require aligned bounded declaration capacity");
    }
    lease->next = pin->functions;
    pin->functions = lease;
    return 0;
}

static int function_index(lua_State *L)
{
    FunctionReference *reference = p4_lua_userdata(L, 1, P4_LUA_TAG_FUNCTION, "guest function reference");
    const char *key = luaL_checkstring(L, 2);
    uint32_t retail;
    if (!strcmp(key, "address") && p4_mod_memory_function_decode(reference->word, &retail) == 0)
        lua_pushunsigned(L, retail);
    else lua_pushnil(L);
    return 1;
}

static void push_function(lua_State *L, uint32_t word)
{
    if (word == 0) { lua_pushnil(L); return; }
    if (!p4_mod_memory_function_known(word)) luaL_error(L, "unknown guest function-pointer entry");
    FunctionReference *reference = lua_newuserdatataggedwithmetatable(L, sizeof *reference, P4_LUA_TAG_FUNCTION);
    reference->word = word;
}

int p4_lua_function_reference(lua_State *L)
{
    uint32_t retail = (uint32_t)p4_lua_integer(L, 1, 0, UINT32_MAX), word;
    if (p4_mod_memory_function_encode(retail, &word) != 0)
        luaL_error(L, "unknown retail function entry");
    push_function(L, word);
    return 1;
}

uint32_t p4_lua_function_address(lua_State *L, int index)
{
    if (lua_userdatatag(L, index) == P4_LUA_TAG_FUNCTION) {
        FunctionReference *reference = p4_lua_userdata(L, index, P4_LUA_TAG_FUNCTION, "guest function reference");
        uint32_t retail;
        if (p4_mod_memory_function_decode(reference->word, &retail) != 0)
            luaL_error(L, "function reference has no callable retail entry");
        return retail;
    }
    return (uint32_t)p4_lua_integer(L, index, 0, UINT32_MAX);
}

typedef struct Scalar { const char *name; unsigned size; int sign, real, function; } Scalar;
static const Scalar scalars[] = {
    {"u8",1,0,0,0}, {"u16",2,0,0,0}, {"u32",4,0,0,0}, {"u64",8,0,0,0},
    {"i8",1,1,0,0}, {"i16",2,1,0,0}, {"i32",4,1,0,0}, {"i64",8,1,0,0},
    {"f32",4,0,1,0}, {"f64",8,0,1,0}, {"fn",4,0,0,1}
};
static const Scalar *scalar(lua_State *L, int index)
{
    const char *name = luaL_checkstring(L, index);
    for (size_t i = 0; i < sizeof scalars / sizeof scalars[0]; ++i)
        if (!strcmp(name, scalars[i].name)) return &scalars[i];
    luaL_error(L, "unknown memory scalar: %s", name);
}

static int scalar_size(lua_State *L)
{
    lua_pushunsigned(L, scalar(L, 1)->size);
    return 1;
}

static void push_scalar(lua_State *L, const Scalar *s, const unsigned char *bytes)
{
    uint64_t bits = 0;
    for (unsigned i = 0; i < s->size; ++i) bits |= (uint64_t)bytes[i] << (i * 8);
    /* Copy all guest bits before any allocating Luau operation. */
    if (s->function) push_function(L, (uint32_t)bits);
    else if (s->real && s->size == 4) {
        uint32_t word = (uint32_t)bits;
        float value;
        memcpy(&value, &word, sizeof value);
        lua_pushnumber(L, p4_mod_f32_number(value));
    } else if (s->real) {
        double value;
        memcpy(&value, &bits, sizeof value);
        lua_pushnumber(L, p4_mod_f64_quiet(value));
    } else if (s->size == 8) p4_lua_push_wide(L, bits);
    else if (s->sign) {
        unsigned width = s->size * 8;
        int64_t value = (int64_t)bits;
        if (bits & (UINT64_C(1) << (width - 1))) value -= INT64_C(1) << width;
        lua_pushnumber(L, (double)value);
    } else lua_pushunsigned(L, (uint32_t)bits);
}

static void scalar_bytes(lua_State *L, int index, const Scalar *s, unsigned char bytes[8])
{
    uint64_t bits;
    if (s->function) {
        uint32_t word;
        if (lua_isnil(L, index)) word = 0;
        else if (lua_userdatatag(L, index) == P4_LUA_TAG_FUNCTION) {
            FunctionReference *reference = p4_lua_userdata(L, index, P4_LUA_TAG_FUNCTION, "guest function reference");
            word = reference->word;
            if (!p4_mod_memory_function_known(word)) luaL_error(L, "unknown function-pointer entry");
        } else {
            uint32_t retail = (uint32_t)p4_lua_integer(L, index, 0, UINT32_MAX);
            if (p4_mod_memory_function_encode(retail, &word) != 0)
                luaL_error(L, "unknown retail function entry");
        }
        bits = word;
    } else if (s->real) {
        luaL_checktype(L, index, LUA_TNUMBER);
        double value = lua_tonumber(L, index);
        if (s->size == 4) bits = p4_mod_f32_encode(value);
        else { value = p4_mod_f64_quiet(value); memcpy(&bits, &value, sizeof bits); }
    } else if (s->size == 8) bits = p4_lua_wide(L, index);
    else {
        unsigned width = s->size * 8;
        int64_t low = s->sign ? -(INT64_C(1) << (width - 1)) : 0;
        int64_t high = s->sign ? (INT64_C(1) << (width - 1)) - 1 : (INT64_C(1) << width) - 1;
        bits = (uint64_t)p4_lua_integer(L, index, low, high);
    }
    for (unsigned i = 0; i < s->size; ++i) bytes[i] = (unsigned char)(bits >> (i * 8));
}

static int read_scalar(lua_State *L)
{
    const Scalar *s = scalar(L, 1);
    Address a = address(L, 2, s->size);
    push_scalar(L, s, memory(L, a, s->size, 0, NULL));
    return 1;
}
static int write_scalar(lua_State *L)
{
    const Scalar *s = scalar(L, 1);
    unsigned char bytes[8];
    scalar_bytes(L, 3, s, bytes);
    Address a = address(L, 2, s->size);
    if (lua_userdatatag(L, 2) == P4_LUA_TAG_PIN && checked_pin(L, 2)->readonly)
        luaL_error(L, "guest view is readonly");
    func_0043f810(memory(L, a, s->size, 1, bytes), bytes, (uint32_t)s->size);
    return 0;
}
static int read_pinned(lua_State *L)
{
    const Scalar *s = scalar(L, 3);
    Address a = pinned_address(L, s->size, 0);
    push_scalar(L, s, memory(L, a, s->size, 0, NULL));
    return 1;
}
static int write_pinned(lua_State *L)
{
    const Scalar *s = scalar(L, 3);
    unsigned char bytes[8];
    scalar_bytes(L, 4, s, bytes);
    Address a = pinned_address(L, s->size, 1);
    func_0043f810(memory(L, a, s->size, 1, bytes), bytes, (uint32_t)s->size);
    return 0;
}

static size_t string_bound(lua_State *L, int index)
{
    return (size_t)p4_lua_integer(L, index, 1, 1024 * 1024);
}
typedef struct StringCopy { const char *bytes; size_t size; } StringCopy;
static int copy_string(lua_State *L)
{
    const StringCopy *copy = lua_touserdata(L, 1);
    lua_pushlstring(L, copy->bytes, copy->size);
    return 1;
}

static int push_guest_string(lua_State *L, Address a, size_t capacity)
{
    const char *bytes = (const char *)memory(L, a, capacity, 0, NULL);
    const char *end = memchr(bytes, 0, capacity);
    if (end == NULL) luaL_error(L, "guest string has no terminator within capacity");
    StringCopy copy = {bytes, (size_t)(end - bytes)};
    /* The numeric address itself does not root an owning buffer. Pin the
     * native resource over an allocating string copy, and release even on OOM. */
    lua_getref(L, ((P4LuaMod *)p4_luau_user(L))->string_copy_reference);
    lua_pushlightuserdata(L, &copy);
    if (a.buffer != NULL) ++a.buffer->references;
    int status = lua_pcall(L, 1, 1, 0);
    release(a.buffer);
    if (status != LUA_OK) lua_error(L);
    return 1;
}
static int read_string(lua_State *L)
{
    size_t capacity = string_bound(L, 2);
    return push_guest_string(L, address(L, 1, capacity), capacity);
}
static int read_pinned_string(lua_State *L)
{
    size_t capacity = string_bound(L, 3);
    return push_guest_string(L, pinned_address(L, capacity, 0), capacity);
}
static void store_string(lua_State *L, Address a, size_t capacity, int value_index)
{
    p4_lua_require_mutation(L);
    size_t size;
    const char *text = luaL_checklstring(L, value_index, &size);
    if (size >= capacity || memchr(text, 0, size) != NULL)
        luaL_error(L, "guest string does not fit capacity or contains NUL");
    /* Include the terminator in policy validation before the canonical copy. */
    const char *error = p4_mod_memory_check(a.value, size + 1, 1, a.buffer != NULL, text);
    if (error != NULL) luaL_error(L, "%s", error);
    if(!a.buffer)p4_lua_semantic_use(p4_luau_user(L));
    char *destination = P4_G2H(a.value);
    func_0043f810(destination, text, (uint32_t)(size + 1));
}
static int write_string(lua_State *L)
{
    size_t capacity = string_bound(L, 2);
    /* Any string coercion must precede resolving a numeric scratch address. */
    luaL_checktype(L, 3, LUA_TSTRING);
    Address a = address(L, 1, capacity);
    if (lua_userdatatag(L, 1) == P4_LUA_TAG_PIN && checked_pin(L, 1)->readonly)
        luaL_error(L, "guest view is readonly");
    store_string(L, a, capacity, 3);
    return 0;
}
static int write_pinned_string(lua_State *L)
{
    size_t capacity = string_bound(L, 3);
    luaL_checktype(L, 4, LUA_TSTRING);
    store_string(L, pinned_address(L, capacity, 1), capacity, 4);
    return 0;
}
static int read_bytes(lua_State *L)
{
    size_t size = (size_t)p4_lua_integer(L, 2, 0, 1024 * 1024);
    unsigned char *out = lua_newbuffer(L, size);
    Address a = address(L, 1, size);
    if (size != 0) memcpy(out, memory(L, a, size, 0, NULL), size);
    return 1;
}
static int write_bytes(lua_State *L)
{
    size_t size;
    const void *bytes = luaL_checkbuffer(L, 2, &size);
    if (size > 1024 * 1024) luaL_error(L, "guest byte write exceeds one MiB");
    Address a = address(L, 1, size);
    if (lua_userdatatag(L, 1) == P4_LUA_TAG_PIN && checked_pin(L, 1)->readonly)
        luaL_error(L, "guest view is readonly");
    if (size != 0) func_0043f810(memory(L, a, size, 1, bytes), bytes, (uint32_t)size);
    return 0;
}

static int construct_layout(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    int arguments = lua_gettop(L);
    if (mod->layout_factory_reference == LUA_NOREF) luaL_error(L, "layout API is not initialized");
    lua_getref(L, mod->layout_factory_reference);
    lua_insert(L, 1);
    lua_call(L, arguments, 1);
    return 1;
}

static void metatable(lua_State *L, int tag, lua_CFunction index, const char *name)
{
    lua_newtable(L);
    lua_pushcfunction(L, index, name); lua_setfield(L, -2, "__index");
    lua_pushstring(L, name); lua_setfield(L, -2, "__metatable");
    lua_setreadonly(L, -1, 1);
    lua_setuserdatametatable(L, tag);
}
void p4_lua_memory(lua_State *L)
{
    lua_pushcfunction(L, release_buffer, "guest_buffer.release");
    ((P4LuaMod *)p4_luau_user(L))->buffer_release_reference = lua_ref(L, -1);
    lua_pop(L, 1);
    lua_setuserdatadtor(L, P4_LUA_TAG_BUFFER, destroy_buffer);
    lua_setuserdatadtor(L, P4_LUA_TAG_PIN, destroy_pin);
    metatable(L, P4_LUA_TAG_BUFFER, buffer_index, "protected guest buffer");
    metatable(L, P4_LUA_TAG_PIN, pin_index, "protected guest view pin");
    metatable(L, P4_LUA_TAG_FUNCTION, function_index, "protected guest function reference");
    lua_pushcfunction(L, copy_string, "p4.memory.string_copy");
    ((P4LuaMod *)p4_luau_user(L))->string_copy_reference = lua_ref(L, -1);
    lua_pop(L, 1);
    /* Positional trusted-chunk arguments match layout_api.luau. They become
     * closure upvalues, not public p4.mem methods or another allocated table. */
    static const luaL_Reg helpers[P4_LUA_LAYOUT_HELPERS] = {
        {"pin", pin_span}, {"read", read_pinned},
        {"write", write_pinned}, {"read_string", read_pinned_string},
        {"write_string", write_pinned_string}, {"function_words", declare_functions}
    };
    P4LuaMod *mod = p4_luau_user(L);
    for (unsigned i = 0; i < P4_LUA_LAYOUT_HELPERS; ++i) {
        lua_pushcfunction(L, helpers[i].func, helpers[i].name);
        mod->layout_helpers[i] = lua_ref(L, -1);
        lua_pop(L, 1);
    }
    static const luaL_Reg methods[] = {
        {"read", read_scalar}, {"write", write_scalar}, {"sizeof", scalar_size},
        {"layout", construct_layout},
        {"read_string", read_string}, {"write_string", write_string},
        {"read_bytes", read_bytes}, {"write_bytes", write_bytes}, {"alloc", allocate_buffer},
        {NULL, NULL}
    };
    p4_lua_library(L, methods);
}
