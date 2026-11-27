//! The `luau` CLI's filesystem require configuration.
//!
//! Port of upstream CLI/src/ReplRequirer.cpp, CLI/include/Luau/ReplRequirer.h and the
//! require context setup of CLI/src/Repl.cpp (createCliRequireContext, setupState),
//! Luau 0.744, commit 32c28089305f488f901aaf1dbf3fd1cb65fa3d11. Portions derived from
//! Luau (MIT, Copyright (c) 2019-2026 Roblox Corporation); see
//! third_party/luau/LICENSE.txt.
//!
//! Behaviour:
//!   * Requiring is allowed from chunks named "=stdin" (the REPL; resolved as the
//!     virtual file "./stdin" in the working directory) and "@<file path>" (scripts and
//!     required modules). Anything else, e.g. `loadstring` chunks, fails with
//!     "require is not supported in this context".
//!   * Modules are files: "x" is "x.luau"/"x.lua" or "x/init.luau"/"x/init.lua"
//!     (see vfs_navigator.zig). Configuration files are ".luaurc" (JSON) and
//!     ".config.luau" (Luau, executed in a fresh sandboxed VM with a 2 s timeout).
//!     Alias values that are neither relative nor aliases must be absolute paths.
//!   * Chunkname "@<path as navigated>", load name and cache key the absolute path,
//!     so one file is loaded once however it is reached.
//!   * Each module runs on a new thread of the main thread (it does not inherit the
//!     requirer's environment) with `luaL_sandboxthread` applied: it reads the shared,
//!     read-only globals through a fresh writable global table of its own.
//!   * Errors raised: "could not read file '<abs path>'", "module must return a single
//!     value", "module can not yield", "error while running module: <message>",
//!     "unknown error while running module" (non-string error). A module whose source
//!     does not compile is not run: the compile error message (a string) becomes its
//!     result, exactly as upstream.
//!   * flags.LuauCyclicRequireShortCircuit: modules using `export` get a cyclic-require
//!     placeholder (library.createPlaceholder).
//! Not ported because the Zig engine has no such subsystem: native code generation,
//! coverage and counters tracking of required modules (upstream ReplRequirer hooks).

const std = @import("std");
const Allocator = std.mem.Allocator;
const Io = std.Io;
const vm = @import("../vm.zig");
const stdlib = @import("../stdlib.zig");
const aux = @import("../stdlib/auxiliary.zig");
const compiler = @import("../compiler.zig");
const flags = @import("flags.zig");
const nav = @import("navigation.zig");
const library = @import("library.zig");
const fu = @import("file_utils.zig");
const vfs_mod = @import("vfs_navigator.zig");

const State = vm.lua_State;
const NavigateResult = nav.NavigateResult;

/// ReplRequirer: the context (`ctx`) of the CLI configuration.
pub const Requirer = struct {
    io: Io,
    /// Compile options for required modules, read at every load (upstream `copts()`),
    /// so later changes by the host apply. Their `flags` also compile .config.luau files.
    options: *const compiler.Options,
    vfs: vfs_mod.VfsNavigator,

    /// createCliRequireContext: allocates the context as a userdata kept alive by the
    /// registry (keyed by its own address) for the lifetime of the state.
    pub fn create(L: *State, io: Io, options: *const compiler.Options) *Requirer {
        const ud = vm.lua_newuserdata(L, @sizeOf(Requirer)) orelse
            aux.errorf(L, "unable to allocate ReplRequirer", .{});
        const req: *Requirer = @ptrCast(@alignCast(ud));
        req.* = .{ .io = io, .options = options, .vfs = .init(io) };

        // Store the context in the registry to keep it alive for the lifetime of this
        // lua_State. Memory address is used as a key to avoid collisions.
        vm.lua_pushlightuserdata(L, req);
        vm.lua_insert(L, -2);
        vm.lua_settable(L, vm.LUA_REGISTRYINDEX);
        return req;
    }
};

fn requirer(ctx: ?*anyopaque) *Requirer {
    return @ptrCast(@alignCast(ctx.?));
}

fn convert(status: vfs_mod.NavigationStatus) NavigateResult {
    return switch (status) {
        .success => .success,
        .ambiguous => .ambiguous,
        .not_found => .not_found,
    };
}

fn isRequireAllowed(L: *State, ctx: ?*anyopaque, chunkname: []const u8) bool {
    _ = L;
    _ = ctx;
    return std.mem.eql(u8, chunkname, "=stdin") or (chunkname.len != 0 and chunkname[0] == '@');
}

fn reset(L: *State, ctx: ?*anyopaque, a: Allocator, chunkname: []const u8) Allocator.Error!NavigateResult {
    _ = L;
    const req = requirer(ctx);
    if (std.mem.eql(u8, chunkname, "=stdin")) return convert(try req.vfs.resetToStdIn(a));
    if (chunkname.len != 0 and chunkname[0] == '@') return convert(try req.vfs.resetToPath(a, chunkname[1..]));
    return .not_found;
}

fn jumpToAlias(L: *State, ctx: ?*anyopaque, a: Allocator, path: []const u8) Allocator.Error!NavigateResult {
    _ = L;
    if (!fu.isAbsolutePath(path)) return .not_found;
    return convert(try requirer(ctx).vfs.resetToPath(a, path));
}

fn toParent(L: *State, ctx: ?*anyopaque, a: Allocator) Allocator.Error!NavigateResult {
    _ = L;
    return convert(try requirer(ctx).vfs.toParent(a));
}

fn toChild(L: *State, ctx: ?*anyopaque, a: Allocator, name: []const u8) Allocator.Error!NavigateResult {
    _ = L;
    return convert(try requirer(ctx).vfs.toChild(a, name));
}

fn isModulePresent(L: *State, ctx: ?*anyopaque, a: Allocator) Allocator.Error!bool {
    _ = L;
    _ = a;
    const req = requirer(ctx);
    return fu.isFile(req.io, req.vfs.getFilePath());
}

fn getChunkname(L: *State, ctx: ?*anyopaque, a: Allocator) Allocator.Error!?[]const u8 {
    _ = L;
    return try std.mem.concat(a, u8, &.{ "@", requirer(ctx).vfs.getFilePath() });
}

fn getLoadname(L: *State, ctx: ?*anyopaque, a: Allocator) Allocator.Error!?[]const u8 {
    _ = L;
    _ = a;
    return requirer(ctx).vfs.getAbsoluteFilePath();
}

fn getCacheKey(L: *State, ctx: ?*anyopaque, a: Allocator) Allocator.Error!?[]const u8 {
    _ = L;
    _ = a;
    return requirer(ctx).vfs.getAbsoluteFilePath();
}

fn getConfigStatus(L: *State, ctx: ?*anyopaque, a: Allocator) Allocator.Error!nav.ConfigStatus {
    _ = L;
    return switch (try requirer(ctx).vfs.getConfigStatus(a)) {
        .absent => .absent,
        .ambiguous => .ambiguous,
        .present_json => .present_json,
        .present_luau => .present_luau,
    };
}

fn getConfig(L: *State, ctx: ?*anyopaque, a: Allocator) Allocator.Error!?[]const u8 {
    _ = L;
    return try requirer(ctx).vfs.getConfig(a);
}

fn getLuauConfigCompilerFlags(L: *State, ctx: ?*anyopaque) compiler.Flags {
    _ = L;
    return requirer(ctx).options.flags;
}

fn memoryError(L: *State) noreturn {
    aux.runerror(L, vm.LUA_MEMERRMSG);
}

fn load(L: *State, ctx: ?*anyopaque, a: Allocator, path: [:0]const u8, chunkname: [:0]const u8, loadname: [:0]const u8) c_int {
    _ = path;
    const req = requirer(ctx);

    // module needs to run in a new thread, isolated from the rest
    // note: we create ML on main thread so that it doesn't inherit environment of L
    const GL = vm.lua_mainthread(L);
    const ML = vm.lua_newthread(GL);
    vm.lua_xmove(GL, L, 1);

    // new thread needs to have the globals sandboxed
    stdlib.luaL_sandboxthread(ML);

    // Source and bytecode live in the resolution arena: nothing to release if a Lua
    // error unwinds, and nothing of them is used once the module starts running.
    const contents = (fu.readFile(req.io, a, loadname) catch memoryError(L)) orelse
        aux.errorf(L, "could not read file '{s}'", .{loadname});

    const compiled = compiler.compile(a, contents, req.options.*) catch memoryError(L);
    const status = vm.luau_load(ML, chunkname.ptr, compiled.bytecode.ptr, compiled.bytecode.len, 0);

    if (status == 0) {
        if (flags.LuauCyclicRequireShortCircuit and vm.lua_usesexport(ML, -1) != 0)
            library.createPlaceholder(L);

        const run_status = vm.lua_resume(ML, L, 0);

        if (run_status == 0) {
            if (vm.lua_gettop(ML) != 1)
                aux.errorf(L, "module must return a single value", .{});
        } else if (run_status == vm.LUA_YIELD) {
            aux.errorf(L, "module can not yield", .{});
        } else if (vm.lua_isstring(ML, -1) == 0) {
            aux.errorf(L, "unknown error while running module", .{});
        } else {
            aux.errorf(L, "error while running module: {s}", .{std.mem.span(vm.lua_tostring(ML, -1).?)});
        }
    }

    // add ML result to L stack
    vm.lua_xmove(ML, L, 1);

    // remove ML thread from L stack
    vm.lua_remove(L, -2);

    // added one value to L stack: module result
    return 1;
}

/// requireConfigInit: the CLI configuration.
pub const configuration: nav.Configuration = .{
    .is_require_allowed = &isRequireAllowed,
    .reset = &reset,
    .jump_to_alias = &jumpToAlias,
    .to_parent = &toParent,
    .to_child = &toChild,
    .is_module_present = &isModulePresent,
    .get_chunkname = &getChunkname,
    .get_loadname = &getLoadname,
    .get_cache_key = &getCacheKey,
    .get_config_status = &getConfigStatus,
    .get_config = &getConfig,
    .get_luau_config_compiler_flags = &getLuauConfigCompilerFlags,
    .load = &load,
};

/// Repl.cpp setupState's `luaopen_require(L, requireConfigInit, createCliRequireContext(L))`:
/// creates the CLI context and registers global `require`. Call after opening the
/// standard libraries and before `luaL_sandbox`. `allocator` backs the require arena;
/// `options` must outlive the state.
pub fn open(L: *State, io: Io, allocator: Allocator, options: *const compiler.Options) *Requirer {
    const req = Requirer.create(L, io, options);
    library.open(L, configuration, req, allocator);
    return req;
}
