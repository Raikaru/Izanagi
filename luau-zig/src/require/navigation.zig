//! Require-by-string path navigation.
//!
//! Port of upstream Require/include/Luau/Require.h (luarequire_Configuration),
//! Require/src/Navigation.cpp (RuntimeNavigationContext, RuntimeErrorHandler,
//! RuntimeLuauConfigTimer), Require/src/RequireNavigator.cpp (Navigator),
//! Require/src/PathUtilities.cpp and Require/src/AliasCycleTracker.cpp (Luau 0.744,
//! commit 32c28089305f488f901aaf1dbf3fd1cb65fa3d11). Portions derived from Luau (MIT,
//! Copyright (c) 2019-2026 Roblox Corporation); see third_party/luau/LICENSE.txt.
//!
//! Resolution of a require path (backslashes are first turned into '/'):
//!   * "./x" and "../x": reset to the requirer, move to its parent, then walk the
//!     components ("." and empty components are skipped, ".." moves up, anything else
//!     moves to a child).
//!   * "@self/x": reset to the requirer and walk the components from there.
//!   * "@alias/x" (alias matched case-insensitively): the embedder's alias override is
//!     consulted first; otherwise configuration files are searched from the requirer's
//!     parent upwards (the first .luaurc / .config.luau defining the alias wins, every
//!     configuration passed on the way must parse). An alias value that is relative
//!     ("./", "../") is walked from the configuration's location, one that is itself an
//!     alias ("@other/...") is resolved recursively (cycles are reported), and anything
//!     else is handed to jump_to_alias (the CLI accepts absolute paths only). Aliases
//!     found nowhere go to the embedder's fallback, then fail.
//!   * anything else: "require path must start with a valid prefix: ./, ../, or @".
//! Every navigation failure is reported as `error requiring module "<path>": <reason>`.
//!
//! Zig interface notes: `Configuration` mirrors luarequire_Configuration with Zig
//! function pointers. Instead of the C write-into-buffer protocol, string getters return
//! slices allocated from the per-resolution arena they are handed (null = write
//! failure), and every callback may fail with error.OutOfMemory.

const std = @import("std");
const Allocator = std.mem.Allocator;
const vm = @import("../vm.zig");
const aux = @import("../stdlib/auxiliary.zig");
const compiler = @import("../compiler.zig");
const cfg = @import("config.zig");
const luau_config = @import("luau_config.zig");

const State = vm.lua_State;

/// luarequire_NavigateResult
pub const NavigateResult = enum { success, ambiguous, not_found };

/// luarequire_ConfigStatus
pub const ConfigStatus = enum {
    absent,
    /// Signals the presence of multiple configuration files.
    ambiguous,
    present_json,
    present_luau,
};

/// luarequire_Configuration. Required callbacks are non-optional fields; exactly one
/// of `get_alias` / `get_config` must be set (checked when the closure is created).
///
/// Every allocating callback receives the require library's per-resolution arena `a`;
/// memory obtained from it stays valid until the next require call through the same
/// closure begins and must not be freed individually.
pub const Configuration = struct {
    /// Returns whether requires are permitted from the given chunkname.
    is_require_allowed: *const fn (L: *State, ctx: ?*anyopaque, requirer_chunkname: []const u8) bool,

    /// Resets the internal state to point at the requirer module.
    reset: *const fn (L: *State, ctx: ?*anyopaque, a: Allocator, requirer_chunkname: []const u8) Allocator.Error!NavigateResult,

    /// Resets the internal state to point at an aliased module, given its exact path
    /// from a configuration file. Only called when an alias's path cannot be resolved
    /// relative to its configuration file.
    jump_to_alias: *const fn (L: *State, ctx: ?*anyopaque, a: Allocator, path: []const u8) Allocator.Error!NavigateResult,

    /// Initial alias override opportunity before configuration files are searched. On
    /// success the state must point at the aliased location.
    to_alias_override: ?*const fn (L: *State, ctx: ?*anyopaque, a: Allocator, alias_unprefixed: []const u8) Allocator.Error!NavigateResult = null,

    /// Final opportunity to resolve an alias not found in configuration files. On
    /// success the state must point at the aliased location.
    to_alias_fallback: ?*const fn (L: *State, ctx: ?*anyopaque, a: Allocator, alias_unprefixed: []const u8) Allocator.Error!NavigateResult = null,

    /// Navigates through the context by making mutations to the internal state.
    to_parent: *const fn (L: *State, ctx: ?*anyopaque, a: Allocator) Allocator.Error!NavigateResult,
    to_child: *const fn (L: *State, ctx: ?*anyopaque, a: Allocator, name: []const u8) Allocator.Error!NavigateResult,

    /// Returns whether the context is currently pointing at a module.
    is_module_present: *const fn (L: *State, ctx: ?*anyopaque, a: Allocator) Allocator.Error!bool,

    /// Chunkname of the current module (visible through the debug library); only called
    /// when is_module_present returned true. Null reports a write failure.
    get_chunkname: *const fn (L: *State, ctx: ?*anyopaque, a: Allocator) Allocator.Error!?[]const u8,

    /// Loadname identifying the current module, passed to `load`.
    get_loadname: *const fn (L: *State, ctx: ?*anyopaque, a: Allocator) Allocator.Error!?[]const u8,

    /// Cache key representing the current module.
    get_cache_key: *const fn (L: *State, ctx: ?*anyopaque, a: Allocator) Allocator.Error!?[]const u8,

    /// Whether a configuration file is present in the current context, and its syntax.
    get_config_status: *const fn (L: *State, ctx: ?*anyopaque, a: Allocator) Allocator.Error!ConfigStatus,

    /// Embedder-side alias lookup in the current configuration (disables internal
    /// configuration parsing). Null: alias not found.
    get_alias: ?*const fn (L: *State, ctx: ?*anyopaque, a: Allocator, alias: []const u8) Allocator.Error!?[]const u8 = null,

    /// Contents of the configuration file in the current context (enables internal
    /// parsing of .luaurc JSON and .config.luau). Null reports a read failure.
    get_config: ?*const fn (L: *State, ctx: ?*anyopaque, a: Allocator) Allocator.Error!?[]const u8 = null,

    /// Maximum milliseconds a .config.luau may execute; default 2000, negative = infinite.
    get_luau_config_timeout: ?*const fn (L: *State, ctx: ?*anyopaque) c_int = null,

    /// Zig-port addition: compiler feature flags used to compile .config.luau files.
    /// Upstream compiles them under its process-global FFlags; the Zig compiler takes
    /// flags per call. Default: all off.
    get_luau_config_compiler_flags: ?*const fn (L: *State, ctx: ?*anyopaque) compiler.Flags = null,

    /// Executes the module and places the result on the stack, returning the number of
    /// results pushed. Returning -1 directs the requiring thread to yield; it must later
    /// be resumed with the module result pushed onto its stack. May raise Lua errors.
    /// For modules that use `export`, call `library.createPlaceholder(L)` after loading
    /// to enable cyclic require support. `a` is the resolution arena; nothing allocated
    /// from it may be used after the module starts running (a nested require resets it).
    load: *const fn (L: *State, ctx: ?*anyopaque, a: Allocator, path: [:0]const u8, chunkname: [:0]const u8, loadname: [:0]const u8) c_int,

    /// validateConfig: the message upstream raises for an invalid configuration, or null.
    pub fn validate(self: *const Configuration) ?[:0]const u8 {
        if (self.get_alias != null and self.get_config != null)
            return "require configuration cannot define both get_alias and get_config";
        if (self.get_alias == null and self.get_config == null)
            return "require configuration is missing required function pointer: either get_alias or get_config (not both)";
        return null;
    }
};

/// RuntimeLuauConfigTimer, measured with the VM's monotonic lua_clock.
pub const LuauConfigTimer = struct {
    start_time: f64 = 0,
    /// Seconds; null = infinite.
    timeout: ?f64 = null,

    pub fn start(self: *LuauConfigTimer, timeout_ms: c_int) void {
        self.start_time = vm.lua_clock();
        self.timeout = if (timeout_ms < 0) null else @as(f64, @floatFromInt(timeout_ms)) / 1000.0;
    }

    pub fn isFinished(self: *const LuauConfigTimer) bool {
        const timeout = self.timeout orelse return false;
        return vm.lua_clock() - self.start_time >= timeout;
    }
};

const default_luau_config_timeout_ms: c_int = 2000;

pub const ConfigBehavior = enum { get_alias, get_config };

/// RuntimeNavigationContext: adapts a Configuration for one require resolution.
pub const RuntimeNavigationContext = struct {
    config: *const Configuration,
    L: *State,
    ctx: ?*anyopaque,
    a: Allocator,
    requirer_chunkname: []const u8,
    timer: LuauConfigTimer = .{},

    pub fn resetToRequirer(self: *RuntimeNavigationContext) Allocator.Error!NavigateResult {
        return self.config.reset(self.L, self.ctx, self.a, self.requirer_chunkname);
    }

    pub fn jumpToAlias(self: *RuntimeNavigationContext, path: []const u8) Allocator.Error!NavigateResult {
        return self.config.jump_to_alias(self.L, self.ctx, self.a, path);
    }

    pub fn toAliasOverride(self: *RuntimeNavigationContext, alias_unprefixed: []const u8) Allocator.Error!NavigateResult {
        const f = self.config.to_alias_override orelse return .not_found;
        return f(self.L, self.ctx, self.a, alias_unprefixed);
    }

    pub fn toAliasFallback(self: *RuntimeNavigationContext, alias_unprefixed: []const u8) Allocator.Error!NavigateResult {
        const f = self.config.to_alias_fallback orelse return .not_found;
        return f(self.L, self.ctx, self.a, alias_unprefixed);
    }

    pub fn toParent(self: *RuntimeNavigationContext) Allocator.Error!NavigateResult {
        return self.config.to_parent(self.L, self.ctx, self.a);
    }

    pub fn toChild(self: *RuntimeNavigationContext, component: []const u8) Allocator.Error!NavigateResult {
        return self.config.to_child(self.L, self.ctx, self.a, component);
    }

    pub fn isModulePresent(self: *RuntimeNavigationContext) Allocator.Error!bool {
        return self.config.is_module_present(self.L, self.ctx, self.a);
    }

    pub fn getChunkname(self: *RuntimeNavigationContext) Allocator.Error!?[]const u8 {
        return self.config.get_chunkname(self.L, self.ctx, self.a);
    }

    pub fn getLoadname(self: *RuntimeNavigationContext) Allocator.Error!?[]const u8 {
        return self.config.get_loadname(self.L, self.ctx, self.a);
    }

    pub fn getCacheKey(self: *RuntimeNavigationContext) Allocator.Error!?[]const u8 {
        return self.config.get_cache_key(self.L, self.ctx, self.a);
    }

    pub fn getConfigStatus(self: *RuntimeNavigationContext) Allocator.Error!ConfigStatus {
        return self.config.get_config_status(self.L, self.ctx, self.a);
    }

    pub fn getConfigBehavior(self: *const RuntimeNavigationContext) ConfigBehavior {
        return if (self.config.get_alias != null) .get_alias else .get_config;
    }

    pub fn getAlias(self: *RuntimeNavigationContext, alias: []const u8) Allocator.Error!?[]const u8 {
        return self.config.get_alias.?(self.L, self.ctx, self.a, alias);
    }

    pub fn getConfig(self: *RuntimeNavigationContext) Allocator.Error!?[]const u8 {
        return self.config.get_config.?(self.L, self.ctx, self.a);
    }

    fn luauConfigCompilerFlags(self: *const RuntimeNavigationContext) compiler.Flags {
        const f = self.config.get_luau_config_compiler_flags orelse return .{};
        return f(self.L, self.ctx);
    }

    /// luauConfigInit: starts the timer and attaches it to the configuration state.
    fn luauConfigInit(context: ?*anyopaque, config_L: *State) void {
        const self: *RuntimeNavigationContext = @ptrCast(@alignCast(context.?));
        const timeout = if (self.config.get_luau_config_timeout) |f| f(self.L, self.ctx) else default_luau_config_timeout_ms;
        self.timer.start(timeout);
        vm.lua_setthreaddata(config_L, &self.timer);
    }

    /// luauConfigInterrupt: raises once the configuration ran out of time. Threads the
    /// configuration creates (coroutines) carry no thread data upstream (a null
    /// dereference there); they share the main thread's timer here.
    fn luauConfigInterrupt(L_: ?*State, gc: c_int) callconv(.c) void {
        _ = gc;
        const L = L_.?;
        const data = vm.lua_getthreaddata(L) orelse vm.lua_getthreaddata(vm.lua_mainthread(L)) orelse return;
        const timer: *const LuauConfigTimer = @ptrCast(@alignCast(data));
        if (timer.isFinished()) aux.errorf(L, "configuration execution timed out", .{});
    }

    fn interruptCallbacks(self: *RuntimeNavigationContext) luau_config.InterruptCallbacks {
        return .{ .context = self, .init = &luauConfigInit, .interrupt = &luauConfigInterrupt };
    }
};

/// Navigation error message (null: no error).
pub const Error = ?[]const u8;

const PathType = enum { unsupported, relative_to_current, relative_to_parent, aliased };

fn getPathType(path: []const u8) PathType {
    if (std.mem.startsWith(u8, path, "./")) return .relative_to_current;
    if (std.mem.startsWith(u8, path, "../")) return .relative_to_parent;
    if (path.len >= 1 and path[0] == '@') return .aliased;
    return .unsupported;
}

const Split = struct { first: []const u8, second: []const u8 };

fn splitPath(path: []const u8) Split {
    const pos = std.mem.indexOfScalar(u8, path, '/') orelse return .{ .first = path, .second = "" };
    return .{ .first = path[0..pos], .second = path[pos + 1 ..] };
}

/// The alias name of "@alias/..." without the '@' (original case).
fn extractAlias(path: []const u8) []const u8 {
    // To ignore the '@' alias prefix when processing the alias
    if (std.mem.indexOfScalar(u8, path, '/')) |len| return path[1..len];
    return path[1..];
}

/// AliasCycleTracker
const AliasCycleTracker = struct {
    ordered: std.ArrayList([]const u8) = .empty,

    fn add(self: *AliasCycleTracker, a: Allocator, alias: []const u8) Allocator.Error!Error {
        for (self.ordered.items) |seen| {
            if (std.mem.eql(u8, seen, alias))
                return try std.fmt.allocPrint(a, "detected alias cycle ({s})", .{try self.stringifiedCycle(a, alias)});
        }
        try self.ordered.append(a, alias);
        return null;
    }

    fn stringifiedCycle(self: *const AliasCycleTracker, a: Allocator, repeated: []const u8) Allocator.Error![]const u8 {
        var result: std.ArrayList(u8) = .empty;
        var in_cycle = false;
        for (self.ordered.items) |item| {
            if (in_cycle) {
                try result.appendSlice(a, " -> @");
                try result.appendSlice(a, item);
            }
            if (std.mem.eql(u8, item, repeated)) {
                in_cycle = true;
                try result.append(a, '@');
                try result.appendSlice(a, item);
            }
        }
        try result.appendSlice(a, " -> @");
        try result.appendSlice(a, repeated);
        return result.items;
    }
};

/// Luau::Require::Navigator over a RuntimeNavigationContext.
pub const Navigator = struct {
    nav: *RuntimeNavigationContext,
    a: Allocator,

    /// Navigates to `path`; returns the failure reason (without the
    /// `error requiring module` prefix) or null on success.
    pub fn navigate(self: Navigator, path_in: []const u8) Allocator.Error!Error {
        const path = try self.a.dupe(u8, path_in);
        std.mem.replaceScalar(u8, path, '\\', '/');
        return self.navigateImpl(path);
    }

    fn navigateImpl(self: Navigator, path: []const u8) Allocator.Error!Error {
        const path_type = getPathType(path);

        if (path_type == .unsupported)
            return "require path must start with a valid prefix: ./, ../, or @";

        if (path_type == .aliased) {
            const alias = try cfg.toLower(self.a, extractAlias(path));

            if (try self.resetToRequirer()) |err| return err;

            if (std.mem.eql(u8, alias, "self")) {
                // If the alias is "@self", we immediately navigate directly from the
                // requirer's context. Neither embedder-defined nor user-defined alias
                // overrides are considered.
                return self.navigateThroughPath(path);
            }

            const override = try self.toAliasOverride(alias);
            if (override.err) |err| return err;
            if (override.overridden) return self.navigateThroughPath(path);

            var config: cfg.Config = .{};
            if (try self.navigateToAndPopulateConfig(alias, &config)) |err| return err;

            if (config.containsAlias(alias)) {
                var tracker: AliasCycleTracker = .{};
                if (try self.navigateToAlias(alias, &config, &tracker)) |err| return err;
                return self.navigateThroughPath(path);
            } else {
                if (try self.toAliasFallback(alias)) |err| return err;
                return self.navigateThroughPath(path);
            }
        }

        // relative_to_current or relative_to_parent
        if (try self.resetToRequirer()) |err| return err;
        if (try self.navigateToParent(null)) |err| return err;
        return self.navigateThroughPath(path);
    }

    fn navigateThroughPath(self: Navigator, path: []const u8) Allocator.Error!Error {
        var components = splitPath(path);
        if (path.len >= 1 and path[0] == '@') {
            // If the path is aliased, we ignore the alias: this function assumes that
            // navigation to an alias is handled by the caller.
            components = splitPath(components.second);
        }

        var previous_component: ?[]const u8 = null;
        while (!(components.first.len == 0 and components.second.len == 0)) {
            if (std.mem.eql(u8, components.first, ".") or components.first.len == 0) {
                components = splitPath(components.second);
                continue;
            } else if (std.mem.eql(u8, components.first, "..")) {
                if (try self.navigateToParent(previous_component)) |err| return err;
            } else {
                if (try self.navigateToChild(components.first)) |err| return err;
            }
            previous_component = components.first;
            components = splitPath(components.second);
        }

        return null;
    }

    fn navigateToAlias(self: Navigator, alias: []const u8, config: *const cfg.Config, tracker: *AliasCycleTracker) Allocator.Error!Error {
        const value = config.getAlias(alias).?.value;
        const path_type = getPathType(value);

        if (path_type == .relative_to_current or path_type == .relative_to_parent) {
            if (try self.navigateThroughPath(value)) |err| return err;
        } else if (path_type == .aliased) {
            if (try tracker.add(self.a, alias)) |err| return err;

            // Not lowercased: upstream looks the chained alias up in its original case.
            const next_alias = extractAlias(value);

            const override = try self.toAliasOverride(next_alias);
            if (override.err) |err| return err;
            if (override.overridden) return self.navigateThroughPath(value);

            if (config.containsAlias(next_alias)) {
                if (try self.navigateToAlias(next_alias, config, tracker)) |err| return err;
            } else {
                var parent_config: cfg.Config = .{};
                if (try self.navigateToAndPopulateConfig(next_alias, &parent_config)) |err| return err;

                if (parent_config.containsAlias(next_alias)) {
                    var fresh: AliasCycleTracker = .{};
                    if (try self.navigateToAlias(next_alias, &parent_config, &fresh)) |err| return err;
                } else {
                    if (try self.toAliasFallback(next_alias)) |err| return err;
                }
            }

            if (try self.navigateThroughPath(value)) |err| return err;
        } else {
            if (try self.jumpToAlias(value)) |err| return err;
        }

        return null;
    }

    fn navigateToAndPopulateConfig(self: Navigator, desired_alias: []const u8, config: *cfg.Config) Allocator.Error!Error {
        const a = self.a;
        while (!config.containsAlias(desired_alias)) {
            config.* = .{}; // Clear existing config data.

            switch (try self.nav.toParent()) {
                .ambiguous => return try std.fmt.allocPrint(a, "could not navigate up the ancestry chain during search for alias \"{s}\" (ambiguous)", .{desired_alias}),
                .not_found => break, // Not treated as an error: interpreted as reaching the root.
                .success => {},
            }

            const status = try self.nav.getConfigStatus();
            switch (status) {
                .absent => continue,
                .ambiguous => return try std.fmt.allocPrint(a, "could not resolve alias \"{s}\" (ambiguous configuration file)", .{desired_alias}),
                .present_json, .present_luau => {},
            }

            if (self.nav.getConfigBehavior() == .get_alias) {
                const alias_path = try self.nav.getAlias(desired_alias) orelse
                    return try std.fmt.allocPrint(a, "could not resolve alias \"{s}\"", .{desired_alias});
                try config.setAlias(a, desired_alias, alias_path, null);
                break;
            }

            const contents = try self.nav.getConfig() orelse
                return try std.fmt.allocPrint(a, "could not get configuration file contents to resolve alias \"{s}\"", .{desired_alias});

            const alias_options: cfg.AliasOptions = .{ .overwrite_aliases = false };

            if (status == .present_json) {
                if (try cfg.parseConfig(a, contents, config, .{ .alias_options = alias_options })) |err| return err;
            } else {
                if (try luau_config.extractLuauConfig(a, contents, config, alias_options, self.nav.interruptCallbacks(), self.nav.luauConfigCompilerFlags())) |err|
                    return err;
            }
        }

        return null;
    }

    fn ambiguousSuffix(result: NavigateResult) []const u8 {
        return if (result == .ambiguous) " (ambiguous)" else "";
    }

    fn resetToRequirer(self: Navigator) Allocator.Error!Error {
        const result = try self.nav.resetToRequirer();
        if (result == .success) return null;
        return try std.fmt.allocPrint(self.a, "could not reset to requiring context{s}", .{ambiguousSuffix(result)});
    }

    fn jumpToAlias(self: Navigator, alias_path: []const u8) Allocator.Error!Error {
        const result = try self.nav.jumpToAlias(alias_path);
        if (result == .success) return null;
        return try std.fmt.allocPrint(self.a, "could not jump to alias \"{s}\"{s}", .{ alias_path, ambiguousSuffix(result) });
    }

    fn navigateToParent(self: Navigator, previous_component: ?[]const u8) Allocator.Error!Error {
        const result = try self.nav.toParent();
        if (result == .success) return null;
        if (previous_component) |component|
            return try std.fmt.allocPrint(self.a, "could not get parent of component \"{s}\"{s}", .{ component, ambiguousSuffix(result) });
        return try std.fmt.allocPrint(self.a, "could not get parent of requiring context{s}", .{ambiguousSuffix(result)});
    }

    fn navigateToChild(self: Navigator, component: []const u8) Allocator.Error!Error {
        const result = try self.nav.toChild(component);
        if (result == .success) return null;
        return try std.fmt.allocPrint(self.a, "could not resolve child component \"{s}\"{s}", .{ component, ambiguousSuffix(result) });
    }

    const Override = struct { err: Error = null, overridden: bool = false };

    fn toAliasOverride(self: Navigator, alias_unprefixed: []const u8) Allocator.Error!Override {
        return switch (try self.nav.toAliasOverride(alias_unprefixed)) {
            .success => .{ .overridden = true },
            .not_found => .{},
            .ambiguous => .{ .err = try std.fmt.allocPrint(self.a, "@{s} is not a valid alias (ambiguous)", .{alias_unprefixed}) },
        };
    }

    fn toAliasFallback(self: Navigator, alias_unprefixed: []const u8) Allocator.Error!Error {
        const result = try self.nav.toAliasFallback(alias_unprefixed);
        if (result == .success) return null;
        return try std.fmt.allocPrint(self.a, "@{s} is not a valid alias{s}", .{ alias_unprefixed, ambiguousSuffix(result) });
    }
};

test "path helpers" {
    try std.testing.expectEqual(PathType.relative_to_current, getPathType("./a"));
    try std.testing.expectEqual(PathType.relative_to_parent, getPathType("../a"));
    try std.testing.expectEqual(PathType.aliased, getPathType("@a/b"));
    try std.testing.expectEqual(PathType.unsupported, getPathType("a/b"));
    try std.testing.expectEqual(PathType.unsupported, getPathType("."));
    try std.testing.expectEqualStrings("Alias", extractAlias("@Alias/x/y"));
    try std.testing.expectEqualStrings("self", extractAlias("@self"));
    const s = splitPath("a/b/c");
    try std.testing.expectEqualStrings("a", s.first);
    try std.testing.expectEqualStrings("b/c", s.second);
}

test "alias cycle message" {
    var arena_state = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena_state.deinit();
    const a = arena_state.allocator();
    var t: AliasCycleTracker = .{};
    try std.testing.expect((try t.add(a, "a")) == null);
    try std.testing.expect((try t.add(a, "b")) == null);
    try std.testing.expectEqualStrings("detected alias cycle (@a -> @b -> @a)", (try t.add(a, "a")).?);
}
