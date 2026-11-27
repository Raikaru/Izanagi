//! `.config.luau` execution and extraction.
//!
//! Port of upstream Config/src/LuauConfig.cpp and Config/include/Luau/LuauConfig.h
//! (Luau 0.744, commit 32c28089305f488f901aaf1dbf3fd1cb65fa3d11). Portions derived
//! from Luau (MIT, Copyright (c) 2019-2026 Roblox Corporation); see
//! third_party/luau/LICENSE.txt.
//!
//! The configuration source is compiled with default compile options and executed as
//! chunk "=config" on the main thread of a brand-new VM state that has the standard
//! libraries opened and then `luaL_sandbox` applied (read-only globals and libraries).
//! The state never shares anything with the requiring state and is closed afterwards.
//! An optional interrupt callback (the require library installs a timeout) runs during
//! execution. The returned table is serialized to plain values (string/number keys;
//! string/number/boolean/nested-table values) and its "luau" sub-table configures
//! languagemode, lint, linterrors, typeerrors, globals and aliases.
//!
//! Differences from upstream, all where upstream has undefined behaviour or crashes:
//!   * Library opening/sandboxing and chunk loading run under protected calls, so an
//!     out-of-memory there is reported as the configuration error instead of aborting.
//!   * A non-string error object raised by the configuration yields an empty message
//!     (upstream assigns a null `const char*` to std::string).
//!   * Tables nested deeper than the VM stack allows (including cyclic tables, which
//!     upstream recurses into forever) report "configuration table nesting is too deep".
//!   * Upstream iterates its DenseHashMap in hash order; this port uses the Lua table's
//!     traversal order. Only which error is reported first, when several entries are
//!     invalid, can differ.
//!   * The Zig compiler takes feature flags per call; callers pass the flags upstream
//!     would read from its process-global FFlags.

const std = @import("std");
const Allocator = std.mem.Allocator;
const vm = @import("../vm.zig");
const stdlib = @import("../stdlib.zig");
const compiler = @import("../compiler.zig");
const cfg = @import("config.zig");

const State = vm.lua_State;

/// Luau::kLuauConfigName
pub const luau_config_name = ".config.luau";

pub const Key = union(enum) {
    string: []const u8,
    number: f64,

    /// ConfigTableKey::toString (numbers through std::to_string, i.e. "%f").
    pub fn toString(self: Key, a: Allocator) Allocator.Error![]const u8 {
        return switch (self) {
            .string => |s| s,
            .number => |n| try std.fmt.allocPrint(a, "{d:.6}", .{n}),
        };
    }

    fn isString(self: Key, s: []const u8) bool {
        return switch (self) {
            .string => |k| std.mem.eql(u8, k, s),
            .number => false,
        };
    }
};

pub const Value = union(enum) {
    string: []const u8,
    number: f64,
    boolean: bool,
    table: Table,
};

pub const Entry = struct { key: Key, value: Value };

/// A serialized configuration table, entries in Lua traversal order.
pub const Table = struct {
    entries: []const Entry = &.{},

    pub fn findString(self: Table, key: []const u8) ?Value {
        for (self.entries) |e| {
            if (e.key.isString(key)) return e.value;
        }
        return null;
    }
};

/// InterruptCallbacks: `init` runs on the configuration state right before execution,
/// `interrupt` is installed as its lua_Callbacks.interrupt for the duration of it.
pub const InterruptCallbacks = struct {
    context: ?*anyopaque = null,
    init: ?*const fn (context: ?*anyopaque, L: *State) void = null,
    interrupt: ?*const fn (?*State, c_int) callconv(.c) void = null,
};

pub const ExtractResult = union(enum) {
    table: Table,
    err: []const u8,
};

fn topString(a: Allocator, L: *State) Allocator.Error![]const u8 {
    const s = vm.lua_tostring(L, -1) orelse return "";
    return try a.dupe(u8, std.mem.span(s));
}

fn setupState(L_: ?*State) callconv(.c) c_int {
    const L = L_.?;
    stdlib.luaL_openlibs(L);
    stdlib.luaL_sandbox(L);
    return 0;
}

const LoadArgs = struct {
    bytecode: []const u8,
    status: c_int = 0,
};

fn loadChunk(L_: ?*State) callconv(.c) c_int {
    const L = L_.?;
    const args: *LoadArgs = @ptrCast(@alignCast(vm.lua_tolightuserdata(L, 1).?));
    args.status = vm.luau_load(L, "=config", args.bytecode.ptr, args.bytecode.len, 0);
    return 1; // the loaded function, or the load error message
}

const SerializeResult = union(enum) {
    table: Table,
    err: []const u8,
};

/// serializeTable: consumes the table on top of the stack.
fn serializeTable(a: Allocator, L: *State) Allocator.Error!SerializeResult {
    defer vm.lua_pop(L, 1); // Remove table from stack after processing

    if (vm.lua_checkstack(L, 3) == 0) return .{ .err = "configuration table nesting is too deep" };

    var entries: std.ArrayList(Entry) = .empty;

    vm.lua_pushnil(L);
    while (vm.lua_next(L, -2) != 0) {
        // Process key
        const key: Key = switch (vm.lua_type(L, -2)) {
            vm.LUA_TNUMBER => .{ .number = vm.lua_tonumber(L, -2) },
            vm.LUA_TSTRING => .{ .string = try a.dupe(u8, std.mem.span(vm.lua_tostring(L, -2).?)) },
            else => {
                vm.lua_pop(L, 2);
                return .{ .err = "configuration table keys must be strings or numbers" };
            },
        };

        // Process value
        const value: Value = switch (vm.lua_type(L, -1)) {
            vm.LUA_TNUMBER => .{ .number = vm.lua_tonumber(L, -1) },
            vm.LUA_TSTRING => .{ .string = try a.dupe(u8, std.mem.span(vm.lua_tostring(L, -1).?)) },
            vm.LUA_TBOOLEAN => .{ .boolean = vm.lua_toboolean(L, -1) != 0 },
            vm.LUA_TTABLE => blk: {
                vm.lua_pushvalue(L, -1); // Copy table for recursive call
                switch (try serializeTable(a, L)) {
                    .table => |t| break :blk .{ .table = t },
                    .err => |e| {
                        vm.lua_pop(L, 2);
                        return .{ .err = e };
                    },
                }
            },
            else => {
                vm.lua_pop(L, 2);
                return .{ .err = try std.fmt.allocPrint(a, "configuration value for key \"{s}\" must be a string, number, boolean, or nested table", .{try key.toString(a)}) };
            },
        };

        try entries.append(a, .{ .key = key, .value = value });
        vm.lua_pop(L, 1); // Remove value from stack after processing
    }

    return .{ .table = .{ .entries = entries.items } };
}

fn executeAndExtractConfig(a: Allocator, L: *State, callbacks: InterruptCallbacks) Allocator.Error!ExtractResult {
    if (callbacks.init) |init| init(callbacks.context, L);
    vm.lua_callbacks(L).interrupt = callbacks.interrupt;
    const status = vm.lua_resume(L, null, 0);
    vm.lua_callbacks(L).interrupt = null;

    switch (status) {
        vm.LUA_OK => {},
        // debugging not supported, at least for now
        vm.LUA_BREAK, vm.LUA_YIELD => return .{ .err = "configuration execution cannot yield" },
        else => return .{ .err = try topString(a, L) },
    }

    if (vm.lua_gettop(L) != 1) return .{ .err = "configuration must return exactly one value" };
    if (vm.lua_type(L, -1) != vm.LUA_TTABLE) return .{ .err = "configuration did not return a table" };

    return switch (try serializeTable(a, L)) {
        .table => |t| .{ .table = t },
        .err => |e| .{ .err = e },
    };
}

/// Luau::extractConfig: executes `source` in a fresh sandboxed VM and returns the
/// serialized table it returns.
pub fn extractConfig(a: Allocator, source: []const u8, callbacks: InterruptCallbacks, compiler_flags: compiler.Flags) Allocator.Error!ExtractResult {
    var compiled = try compiler.compile(a, source, .{ .flags = compiler_flags });
    defer compiled.deinit(a);

    const L = stdlib.luaL_newstate() orelse return error.OutOfMemory;
    defer vm.lua_close(L);

    if (vm.lua_cpcall(L, &setupState, null) != vm.LUA_OK) return .{ .err = try topString(a, L) };

    var args: LoadArgs = .{ .bytecode = compiled.bytecode };
    vm.lua_pushcfunction(L, &loadChunk, null);
    vm.lua_pushlightuserdata(L, &args);
    if (vm.lua_pcall(L, 1, 1, 0) != vm.LUA_OK or args.status != 0) return .{ .err = try topString(a, L) };

    return executeAndExtractConfig(a, L, callbacks);
}

fn createLuauConfigFromLuauTable(a: Allocator, config: *cfg.Config, luau_table: Table, alias_options: ?cfg.AliasOptions) Allocator.Error!cfg.Error {
    for (luau_table.entries) |entry| {
        const key = switch (entry.key) {
            .string => |s| s,
            .number => return "configuration keys in \"luau\" table must be strings",
        };
        const v = entry.value;

        if (std.mem.eql(u8, key, "languagemode")) {
            const value = switch (v) {
                .string => |s| s,
                else => return "configuration value for key \"languagemode\" must be a string",
            };
            if (try cfg.parseModeString(a, &config.mode, value, false)) |err| return err;
        }

        if (std.mem.eql(u8, key, "lint")) {
            const lint = switch (v) {
                .table => |t| t,
                else => return "configuration value for key \"lint\" must be a table",
            };

            // Handle wildcard first to ensure overrides work as expected.
            if (lint.findString("*")) |value| {
                const enabled = switch (value) {
                    .boolean => |b| b,
                    else => return "configuration values in \"lint\" table must be booleans",
                };
                if (try cfg.parseLintRuleString(a, &config.enabled_lint, &config.fatal_lint, "*", if (enabled) "true" else "false", false)) |err|
                    return err;
            }

            for (lint.entries) |lint_entry| {
                const warning_name = switch (lint_entry.key) {
                    .string => |s| s,
                    .number => return "configuration keys in \"lint\" table must be strings",
                };
                if (std.mem.eql(u8, warning_name, "*")) continue; // Handled above

                const enabled = switch (lint_entry.value) {
                    .boolean => |b| b,
                    else => return "configuration values in \"lint\" table must be booleans",
                };
                if (try cfg.parseLintRuleString(a, &config.enabled_lint, &config.fatal_lint, warning_name, if (enabled) "true" else "false", false)) |err|
                    return err;
            }
        }

        if (std.mem.eql(u8, key, "linterrors")) {
            config.lint_errors = switch (v) {
                .boolean => |b| b,
                else => return "configuration value for key \"linterrors\" must be a boolean",
            };
        }

        if (std.mem.eql(u8, key, "typeerrors")) {
            config.type_errors = switch (v) {
                .boolean => |b| b,
                else => return "configuration value for key \"typeerrors\" must be a boolean",
            };
        }

        if (std.mem.eql(u8, key, "globals")) {
            const globals_table = switch (v) {
                .table => |t| t,
                else => return "configuration value for key \"globals\" must be an array of strings",
            };

            const size = globals_table.entries.len;
            const globals = try a.alloc([]const u8, size);
            @memset(globals, "");

            for (globals_table.entries) |g| {
                const index_key = switch (g.key) {
                    .number => |n| n,
                    .string => return "configuration array \"globals\" must only have numeric keys",
                };
                // static_cast<size_t>(key), then 1 <= index <= size
                if (!(index_key >= 1.0) or @trunc(index_key) > @as(f64, @floatFromInt(size)))
                    return "configuration array \"globals\" contains invalid numeric key";
                const index: usize = @intFromFloat(@trunc(index_key));

                globals[index - 1] = switch (g.value) {
                    .string => |s| s,
                    else => return "configuration value in \"globals\" table must be a string",
                };
            }

            config.globals = .fromOwnedSlice(globals);
        }

        if (std.mem.eql(u8, key, "aliases")) {
            const aliases = switch (v) {
                .table => |t| t,
                else => return "configuration value for key \"aliases\" must be a table",
            };

            for (aliases.entries) |alias| {
                const alias_key = switch (alias.key) {
                    .string => |s| s,
                    .number => return "configuration keys in \"aliases\" table must be strings",
                };
                const alias_value = switch (alias.value) {
                    .string => |s| s,
                    else => return "configuration values in \"aliases\" table must be strings",
                };
                if (try cfg.parseAlias(a, config, alias_key, alias_value, alias_options)) |err| return err;
            }
        }
    }

    return null;
}

fn parseLuauConfigTable(a: Allocator, config_table: Table, config: *cfg.Config, alias_options: ?cfg.AliasOptions) Allocator.Error!cfg.Error {
    const luau = config_table.findString("luau") orelse return null;
    const luau_table = switch (luau) {
        .table => |t| t,
        else => return "configuration value for key \"luau\" must be a table",
    };
    return createLuauConfigFromLuauTable(a, config, luau_table, alias_options);
}

/// Luau::extractLuauConfig: executes `source` and applies its "luau" table to `config`.
/// Returns an error message, or null on success.
pub fn extractLuauConfig(
    a: Allocator,
    source: []const u8,
    config: *cfg.Config,
    alias_options: ?cfg.AliasOptions,
    callbacks: InterruptCallbacks,
    compiler_flags: compiler.Flags,
) Allocator.Error!cfg.Error {
    return switch (try extractConfig(a, source, callbacks, compiler_flags)) {
        .err => |e| e,
        .table => |t| parseLuauConfigTable(a, t, config, alias_options),
    };
}
