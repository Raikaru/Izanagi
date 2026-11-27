//! The require-by-string runtime library: `require`/`proxyrequire` closures, the module
//! cache, registered modules and cyclic-require placeholders.
//!
//! Port of upstream Require/src/Require.cpp and Require/src/RequireImpl.cpp (Luau 0.744,
//! commit 32c28089305f488f901aaf1dbf3fd1cb65fa3d11). Portions derived from Luau (MIT,
//! Copyright (c) 2019-2026 Roblox Corporation); see third_party/luau/LICENSE.txt.
//!
//! Registry layout (identical to upstream, so hosts can share it):
//!   * `_MODULES`: cache key -> module result (one value per module, any type).
//!   * `_REGISTEREDMODULES`: lowercased "@path" -> value registered by the host; consulted
//!     before any resolution, by exact (case-insensitive) path.
//!   * `_CYCLIC_PLACEHOLDER_PROVIDED`: cache keys whose placeholder reached a cyclic requirer.
//!   * light userdata key: the shared, locked placeholder metatable.
//!
//! A require call: registered modules first; then the Configuration decides whether the
//! requiring chunk may require at all, the Navigator resolves the path, a cached result is
//! returned as is, otherwise `load` runs the module and its single result is cached under
//! the module's cache key. Modules returning more than one value fail with
//! "module must return a single value"; a module returning nothing yields no value and is
//! not cached. With flags.LuauCyclicRequireShortCircuit, a module that uses `export` is
//! represented in the cache by a locked, read-only placeholder while it loads: a cyclic
//! requirer receives it, any access raises "Cannot access/set the exported field ...",
//! and once the module finishes the placeholder is filled with the exports, frozen, and
//! becomes the module's result for everyone.
//!
//! Memory: each closure owns an arena (in its upvalue userdata, released by the userdata
//! destructor) that is reset at the start of every call. Navigation and the embedder's
//! callbacks allocate from it, so nothing leaks when a Lua error unwinds through Zig.

const std = @import("std");
const Allocator = std.mem.Allocator;
const vm = @import("../vm.zig");
const stdlib = @import("../stdlib.zig");
const aux = @import("../stdlib/auxiliary.zig");
const flags = @import("flags.zig");
const nav = @import("navigation.zig");

const State = vm.lua_State;
const Configuration = nav.Configuration;

/// Stores explicitly registered modules.
const registered_cache_table_key = "_REGISTEREDMODULES";
/// Stores the results of require calls.
const required_cache_table_key = "_MODULES";
/// Tracks which placeholders were actually returned to a cyclic requirer.
const cyclic_placeholder_provided_key = "_CYCLIC_PLACEHOLDER_PROVIDED";
/// Sentinel address used as a unique registry key for the shared placeholder metatable.
var cyclic_placeholder_metatable_sentinel: u8 = 0;

/// Bytes the per-closure arena keeps between calls.
const arena_retained_bytes = 64 * 1024;

/// Upvalue 1 of every require-like closure.
const ClosureState = struct {
    config: Configuration,
    arena: std.heap.ArenaAllocator,
};

fn destroyClosureState(L: ?*State, ud: ?*anyopaque) callconv(.c) void {
    _ = L;
    const state: *ClosureState = @ptrCast(@alignCast(ud.?));
    state.arena.deinit();
}

fn memoryError(L: *State) noreturn {
    aux.runerror(L, vm.LUA_MEMERRMSG);
}

/// The C-string view upstream passes around: text up to the first NUL.
fn cstr(s: []const u8) []const u8 {
    return s[0 .. std.mem.indexOfScalar(u8, s, 0) orelse s.len];
}

fn pushString(L: *State, s: []const u8) void {
    const c = cstr(s);
    vm.lua_pushlstring(L, c.ptr, c.len);
}

/// Pushes the ASCII-lowercased C-string view of `s` (transient storage is a Lua buffer).
fn pushLowercase(L: *State, s: []const u8) void {
    const c = cstr(s);
    const raw = vm.lua_newbuffer(L, c.len);
    const buf: [*]u8 = @ptrCast(raw.?);
    for (c, 0..) |ch, i| buf[i] = if ('A' <= ch and ch <= 'Z') ch + ('a' - 'A') else ch;
    vm.lua_pushlstring(L, buf, c.len);
    vm.lua_remove(L, -2);
}

fn findtable(L: *State, idx: c_int, name: []const u8) void {
    _ = aux.findtable(L, idx, name, 1);
}

const Resolved = union(enum) {
    cached,
    err: []const u8,
    module_read: struct {
        chunkname: []const u8,
        loadname: []const u8,
        cache_key: []const u8,
    },
};

fn isCached(L: *State, key: [:0]const u8) bool {
    findtable(L, vm.LUA_REGISTRYINDEX, required_cache_table_key);
    _ = vm.lua_getfield(L, -1, key.ptr);
    const cached = !vm.lua_isnil(L, -1);

    if (flags.LuauCyclicRequireShortCircuit and cached and vm.lua_istable(L, -1)) {
        // Check if the cached value is a placeholder (has the shared placeholder metatable).
        if (vm.lua_getmetatable(L, -1) == 1) {
            _ = vm.lua_rawgetp(L, vm.LUA_REGISTRYINDEX, &cyclic_placeholder_metatable_sentinel);
            if (vm.lua_rawequal(L, -1, -2) == 1) {
                // A cyclic require is accessing this placeholder — mark it as provided.
                findtable(L, vm.LUA_REGISTRYINDEX, cyclic_placeholder_provided_key);
                vm.lua_pushboolean(L, 1);
                vm.lua_setfield(L, -2, key.ptr);
                vm.lua_pop(L, 1);
            }
            vm.lua_pop(L, 2); // pop both metatables
        }
    }

    vm.lua_pop(L, 2);
    return cached;
}

fn resolveRequire(config: *const Configuration, L: *State, ctx: ?*anyopaque, a: Allocator, requirer_chunkname: []const u8, path: []const u8) Allocator.Error!Resolved {
    if (!config.is_require_allowed(L, ctx, requirer_chunkname))
        return .{ .err = "require is not supported in this context" };

    var navigation_context: nav.RuntimeNavigationContext = .{
        .config = config,
        .L = L,
        .ctx = ctx,
        .a = a,
        .requirer_chunkname = requirer_chunkname,
    };
    const navigator: nav.Navigator = .{ .nav = &navigation_context, .a = a };

    // Updates navigation_context while navigating through the given path.
    if (try navigator.navigate(path)) |reason|
        return .{ .err = try std.fmt.allocPrint(a, "error requiring module \"{s}\": {s}", .{ path, reason }) };

    if (!try navigation_context.isModulePresent())
        return .{ .err = "no module present at resolved path" };

    const cache_key = try navigation_context.getCacheKey() orelse
        return .{ .err = "could not get cache key for module" };

    const cache_key_z = try a.dupeSentinel(u8, cstr(cache_key), 0);
    if (isCached(L, cache_key_z)) {
        // Put cached result on top of stack before returning.
        _ = vm.lua_getfield(L, vm.LUA_REGISTRYINDEX, required_cache_table_key);
        _ = vm.lua_getfield(L, -1, cache_key_z.ptr);
        vm.lua_remove(L, -2);
        return .cached;
    }

    const chunkname = try navigation_context.getChunkname() orelse
        return .{ .err = "could not get chunkname for module" };

    const loadname = try navigation_context.getLoadname() orelse
        return .{ .err = "could not get loadname for module" };

    return .{ .module_read = .{ .chunkname = chunkname, .loadname = loadname, .cache_key = cache_key } };
}

fn checkRegisteredModules(L: *State, path: []const u8) bool {
    findtable(L, vm.LUA_REGISTRYINDEX, registered_cache_table_key);
    pushLowercase(L, path);
    _ = vm.lua_gettable(L, -2);
    if (vm.lua_isnil(L, -1)) {
        vm.lua_pop(L, 2);
        return false;
    }
    vm.lua_remove(L, -2);
    return true;
}

fn cyclicDependencyIndexError(L_: ?*State) callconv(.c) c_int {
    const L = L_.?;
    const key: []const u8 = if (vm.lua_tostring(L, 2)) |k| std.mem.span(k) else "unknown";
    aux.errorf(L, "Cannot access the exported field '{s}' because it has a cyclic dependency on its requiring module", .{key});
}

fn cyclicDependencyNewIndexError(L_: ?*State) callconv(.c) c_int {
    const L = L_.?;
    const key: []const u8 = if (vm.lua_tostring(L, 2)) |k| std.mem.span(k) else "unknown";
    aux.errorf(L, "Cannot set the exported field '{s}' because it has a cyclic dependency on its requiring module", .{key});
}

/// Returns the shared placeholder metatable, creating it on first use.
fn pushCyclicPlaceholderMetatable(L: *State) void {
    _ = vm.lua_rawgetp(L, vm.LUA_REGISTRYINDEX, &cyclic_placeholder_metatable_sentinel);
    if (!vm.lua_isnil(L, -1)) return;
    vm.lua_pop(L, 1);

    vm.lua_newtable(L);
    vm.lua_pushcfunction(L, &cyclicDependencyIndexError, "CyclicDependencyIndexError");
    vm.lua_setfield(L, -2, "__index");
    vm.lua_pushcfunction(L, &cyclicDependencyNewIndexError, "CyclicDependencyNewIndexError");
    vm.lua_setfield(L, -2, "__newindex");
    vm.lua_pushliteral(L, "The metatable is locked");
    vm.lua_setfield(L, -2, "__metatable");

    vm.lua_pushvalue(L, -1);
    vm.lua_rawsetp(L, vm.LUA_REGISTRYINDEX, &cyclic_placeholder_metatable_sentinel);
}

/// luarequire_lockplaceholder: locks the table at `idx` with the shared metatable whose
/// __index/__newindex raise, and makes it read-only.
pub fn lockPlaceholder(L: *State, idx_in: c_int) void {
    const idx = vm.lua_absindex(L, idx_in);
    pushCyclicPlaceholderMetatable(L);
    _ = vm.lua_setmetatable(L, idx);
    vm.lua_setreadonly(L, idx, 1);
}

/// luarequire_createplaceholder: caches a locked placeholder for the module being loaded
/// (its cache key is stack slot 2 of the require call). Call from `load` for modules
/// that use `export` (vm.lua_usesexport).
pub fn createPlaceholder(L: *State) void {
    const cache_key = stdlib.luaL_checklstring(L, 2, null);

    vm.lua_newtable(L);
    lockPlaceholder(L, -1);

    findtable(L, vm.LUA_REGISTRYINDEX, required_cache_table_key);
    vm.lua_pushvalue(L, -2);
    vm.lua_setfield(L, -2, cache_key);
    vm.lua_pop(L, 2);
}

/// luarequire_populateplaceholder: copies fields, metatable and read-only state of the
/// result table into the placeholder, replacing the lock metatable.
pub fn populatePlaceholder(L: *State, placeholder_idx_in: c_int, result_idx_in: c_int) void {
    const placeholder_idx = vm.lua_absindex(L, placeholder_idx_in);
    const result_idx = vm.lua_absindex(L, result_idx_in);

    // Unfreeze so we can write to the placeholder
    vm.lua_setreadonly(L, placeholder_idx, 0);

    // Copy all fields from the result table into the placeholder
    var iter: c_int = 0;
    while (true) {
        iter = vm.lua_rawiter(L, result_idx, iter);
        if (iter == -1) break;
        // key at -2, value at -1
        vm.lua_rawset(L, placeholder_idx);
    }

    // Copy the metatable from the result (if any)
    if (vm.lua_getmetatable(L, result_idx) == 0) vm.lua_pushnil(L);
    _ = vm.lua_setmetatable(L, placeholder_idx);

    // Freeze the populated placeholder.
    vm.lua_setreadonly(L, placeholder_idx, 1);
}

// Fixed stack slots below the load results:
//   (1) path, (2) cacheKey, (3) chunkname, (4) loadname
const require_stack_values: c_int = 4;

fn requireCont(L_: ?*State, status: c_int) callconv(.c) c_int {
    _ = status;
    const L = L_.?;
    std.debug.assert(vm.lua_gettop(L) >= require_stack_values);
    const num_results = vm.lua_gettop(L) - require_stack_values;
    const cache_key = stdlib.luaL_checklstring(L, 2, null);

    if (num_results > 1) aux.errorf(L, "module must return a single value", .{});

    if (flags.LuauCyclicRequireShortCircuit and num_results == 1) {
        const result_idx = require_stack_values + 1;

        // Check if the placeholder was actually provided to a cyclic requirer.
        findtable(L, vm.LUA_REGISTRYINDEX, cyclic_placeholder_provided_key);
        _ = vm.lua_getfield(L, -1, cache_key);
        const was_provided = vm.lua_toboolean(L, -1) != 0;
        vm.lua_pop(L, 1);

        // Clear the placeholder from the cache to reset the state.
        vm.lua_pushnil(L);
        vm.lua_setfield(L, -2, cache_key);
        vm.lua_pop(L, 1);

        // Upstream asserts the result is a table here (export modules always return
        // their export table); a non-table result is cached normally instead of being
        // copied into the placeholder.
        if (was_provided and vm.lua_istable(L, result_idx)) {
            // Retrieve the placeholder from the cache.
            findtable(L, vm.LUA_REGISTRYINDEX, required_cache_table_key);
            _ = vm.lua_getfield(L, -1, cache_key);

            // Populate the placeholder with the module's result. Cyclic importers already
            // hold the placeholder, so they see the result.
            populatePlaceholder(L, -1, result_idx);

            // Replace the result with the populated placeholder so the initial caller
            // gets the same object that cyclic importers and future cache hits receive.
            vm.lua_replace(L, result_idx);
            vm.lua_pop(L, 1);
        } else {
            // Cache the result normally (no cycle occurred).
            findtable(L, vm.LUA_REGISTRYINDEX, required_cache_table_key);
            vm.lua_pushvalue(L, result_idx);
            vm.lua_setfield(L, -2, cache_key);
            vm.lua_pop(L, 1);
        }
    } else if (num_results == 1) {
        // (-1) result
        _ = vm.lua_getfield(L, vm.LUA_REGISTRYINDEX, required_cache_table_key);
        // (-2) result, (-1) cache table
        vm.lua_pushvalue(L, -2);
        // (-3) result, (-2) cache table, (-1) result
        vm.lua_setfield(L, -2, cache_key);
        // (-2) result, (-1) cache table
        vm.lua_pop(L, 1);
        // (-1) result
    }

    return num_results;
}

fn requireInternal(L: *State, requirer_chunkname_in: []const u8) c_int {
    const state: *ClosureState = @ptrCast(@alignCast(vm.lua_touserdata(L, vm.lua_upvalueindex(1)) orelse
        aux.errorf(L, "unable to find require configuration", .{})));
    const ctx = vm.lua_tolightuserdata(L, vm.lua_upvalueindex(2));

    // Everything Zig allocates for this call lives in the closure arena; a Lua error
    // unwinding through here therefore leaks nothing. Nothing from a previous call is
    // still in use: an outer require only keeps values on its Lua stack while its module runs.
    _ = state.arena.reset(.{ .retain_with_limit = arena_retained_bytes });
    const a = state.arena.allocator();

    // proxyrequire passes the requirer's chunkname as argument 2: copy it before the
    // stack is trimmed so it cannot be collected during resolution.
    const requirer_chunkname = a.dupe(u8, cstr(requirer_chunkname_in)) catch memoryError(L);

    // Discard extra arguments, we only use path
    vm.lua_settop(L, 1);

    // (1) path
    const path_z = std.mem.span(stdlib.luaL_checklstring(L, 1, null));

    if (checkRegisteredModules(L, path_z)) return 1;

    const resolved = resolveRequire(&state.config, L, ctx, a, requirer_chunkname, path_z) catch memoryError(L);
    switch (resolved) {
        .cached => return 1,
        .err => |message| {
            pushString(L, message);
            vm.lua_error(L);
        },
        .module_read => |m| {
            // (1) path, (2) cacheKey, (3) chunkname, (4) loadname
            pushString(L, m.cache_key);
            pushString(L, m.chunkname);
            pushString(L, m.loadname);
        },
    }

    const chunkname = std.mem.span(vm.lua_tostring(L, 3).?);
    const loadname = std.mem.span(vm.lua_tostring(L, 4).?);

    std.debug.assert(vm.lua_gettop(L) == require_stack_values);

    const num_results = state.config.load(L, ctx, a, path_z, chunkname, loadname);
    if (num_results == -1) {
        if (vm.lua_gettop(L) != require_stack_values)
            aux.errorf(L, "stack cannot be modified when require yields", .{});

        return vm.lua_yield(L, 0);
    }

    return requireCont(L, vm.LUA_OK);
}

fn proxyRequire(L_: ?*State) callconv(.c) c_int {
    const L = L_.?;
    const requirer_chunkname = stdlib.luaL_checklstring(L, 2, null);
    return requireInternal(L, std.mem.span(requirer_chunkname));
}

fn require(L_: ?*State) callconv(.c) c_int {
    const L = L_.?;
    var ar: vm.lua_Debug = .{};
    var level: c_int = 1;

    while (true) {
        if (vm.lua_getinfo(L, level, "s", &ar) == 0)
            aux.errorf(L, "require is not supported in this context", .{});
        level += 1;
        if (ar.what.?[0] != 'C') break;
    }

    return requireInternal(L, std.mem.span(ar.source.?));
}

fn pushRequireClosureInternal(L: *State, config: Configuration, ctx: ?*anyopaque, allocator: Allocator, requirelike: vm.lua_CFunction, debugname: [*:0]const u8) c_int {
    if (config.validate()) |message| aux.errorf(L, "{s}", .{message});

    const ud = vm.lua_newuserdatadtor(L, @sizeOf(ClosureState), &destroyClosureState) orelse
        aux.errorf(L, "failed to allocate memory for require configuration", .{});
    const state: *ClosureState = @ptrCast(@alignCast(ud));
    state.* = .{ .config = config, .arena = .init(allocator) };

    vm.lua_pushlightuserdata(L, ctx);

    // require-like closure captures config and ctx as upvalues
    vm.lua_pushcclosurek(L, requirelike, debugname, 2, &requireCont);
    return 1;
}

/// luarequire_pushrequire: pushes a `require` closure for `config`/`ctx` without
/// registering it. `allocator` backs the closure's per-call arena.
pub fn pushRequire(L: *State, config: Configuration, ctx: ?*anyopaque, allocator: Allocator) c_int {
    return pushRequireClosureInternal(L, config, ctx, allocator, &require, "require");
}

/// luaopen_require: creates the `require` closure and sets it as global "require".
pub fn open(L: *State, config: Configuration, ctx: ?*anyopaque, allocator: Allocator) void {
    _ = pushRequire(L, config, ctx, allocator);
    vm.lua_setglobal(L, "require");
}

/// luarequire_pushproxyrequire: pushes `proxyrequire(path, chunkname)`, which resolves
/// `path` as if required from the module that `chunkname` names.
pub fn pushProxyRequire(L: *State, config: Configuration, ctx: ?*anyopaque, allocator: Allocator) c_int {
    return pushRequireClosureInternal(L, config, ctx, allocator, &proxyRequire, "proxyrequire");
}

/// luarequire_registermodule (lua_CFunction): `(path, result)`; afterwards requiring
/// `path` (must begin with '@', matched case-insensitively) returns `result` directly.
pub fn registerModule(L_: ?*State) callconv(.c) c_int {
    const L = L_.?;
    if (vm.lua_gettop(L) != 2)
        aux.errorf(L, "expected 2 arguments: aliased require path and desired result", .{});

    var len: usize = 0;
    const path = stdlib.luaL_checklstring(L, 1, &len);
    if (len == 0 or path[0] != '@')
        stdlib.luaL_argerrorL(L, 1, "path must begin with '@'");

    // Make path lowercase to ensure case-insensitive matching.
    pushLowercase(L, path[0..len]);
    vm.lua_replace(L, 1);

    findtable(L, vm.LUA_REGISTRYINDEX, registered_cache_table_key);
    // (1) path, (2) result, (3) cache table
    vm.lua_insert(L, 1);
    // (1) cache table, (2) path, (3) result
    vm.lua_settable(L, 1);
    // (1) cache table
    vm.lua_pop(L, 1);
    return 0;
}

/// luarequire_clearcacheentry (lua_CFunction): `(cacheKey)`; drops that cache entry.
pub fn clearCacheEntry(L_: ?*State) callconv(.c) c_int {
    const L = L_.?;
    const cache_key = stdlib.luaL_checklstring(L, 1, null);
    findtable(L, vm.LUA_REGISTRYINDEX, required_cache_table_key);
    vm.lua_pushnil(L);
    vm.lua_setfield(L, -2, cache_key);
    vm.lua_pop(L, 1);
    return 0;
}

/// luarequire_clearcache (lua_CFunction): drops every cache entry.
pub fn clearCache(L_: ?*State) callconv(.c) c_int {
    const L = L_.?;
    vm.lua_newtable(L);
    vm.lua_setfield(L, vm.LUA_REGISTRYINDEX, required_cache_table_key);
    return 0;
}
