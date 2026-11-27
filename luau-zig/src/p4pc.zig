//! P4PC mod host bridge for the Zig Luau engine.
//!
//! Zig port of src/platform/mods/luau_c.cpp exporting the unchanged C ABI of
//! include/p4pc/luau_c.h (`p4_luau_*`). The VM, standard libraries and compiler are
//! the Zig implementations in vm.zig, stdlib.zig and compiler.zig; no C/C++ Luau
//! runtime or compiler is linked or invoked.
//!
//! Memory model (same as the C++ bridge):
//!   * Every VM allocation goes through `allocate`, which enforces the hard
//!     `memory_limit` cap over the exact byte sizes the VM requests (including the
//!     state and the opened libraries) and returns 16-byte aligned blocks on every
//!     target, including 32-bit Windows whose CRT malloc only guarantees 8.
//!   * Compiler scratch and the compiled blob are host memory (libc heap), not
//!     charged to the VM cap; they are released before the call returns. Admission
//!     is bounded by `source_limit` instead.
//!   * The bridge object itself is host memory.
//!
//! Error model: the VM unwinds Lua errors non-locally (vm/jmp.zig) without running
//! Zig cleanup. Functions here that may be unwound through (`interrupt`,
//! `initialize`, `load`) hold no `defer`-managed resources; host-memory resources
//! are only held across protected calls (`lua_cpcall`), which always return.
//!
//! Derived from the project's luau_c.cpp; uses Luau (MIT, Copyright (c) 2019-2025
//! Roblox Corporation), see third_party/luau/LICENSE.txt.

const std = @import("std");
const lua = @import("vm.zig");
const stdlib = @import("stdlib.zig");
const compiler = @import("compiler.zig");

const State = lua.lua_State;

const LUA_OK: c_int = lua.LUA_OK;
const LUA_ERRRUN: c_int = lua.LUA_ERRRUN;
const LUA_ERRSYNTAX: c_int = lua.LUA_ERRSYNTAX;
const LUA_ERRMEM: c_int = lua.LUA_ERRMEM;
const LUA_NOREF: c_int = lua.LUA_NOREF;
const LUA_TSTRING: c_int = lua.LUA_TSTRING;
const LUA_GLOBALSINDEX: c_int = lua.LUA_GLOBALSINDEX;

/// Host heap for the bridge object and compiler scratch, and backing store for the
/// capped VM allocator. Matches the C++ bridge's CRT heap. Only this C-ABI bridge
/// uses libc; vm.zig, stdlib.zig and compiler.zig never import this file.
const host = std.heap.c_allocator;
comptime {
    if (!@import("builtin").link_libc)
        @compileError("p4pc.zig is the C-ABI host bridge and requires linking libc (-lc)");
}

/// Alignment of every VM block. The C++ bridge relies on 16 (global_State::ecbdata
/// is alignas(16)); the Zig VM's objects need at most that.
const vm_alignment: std.mem.Alignment = .@"16";
comptime {
    std.debug.assert(@alignOf(State) <= vm_alignment.toByteUnits());
}

const default_source_limit: usize = 1024 * 1024;
const error_capacity = 2048;

// ---------------------------------------------------------------------------------
// Public C ABI types (include/p4pc/luau_c.h)

/// At VM safepoints, including GC; nonzero latches budget exhaustion until the host
/// resets it. Must not throw, allocate through Lua, or reenter the state.
pub const Interrupt = *const fn (user: ?*anyopaque, gc: c_int) callconv(.c) c_int;
/// Registers host functions before luaL_sandbox freezes libraries/globals. Runs
/// protected; callbacks must not hold cleanup-requiring resources across a Lua error.
pub const Initialize = *const fn (state: ?*State, user: ?*anyopaque) callconv(.c) void;

pub const Config = extern struct {
    /// Hard VM allocator cap, including state/libraries.
    memory_limit: usize,
    interrupt: ?Interrupt,
    user: ?*anyopaque,
    initialize: ?Initialize,
    /// Zero selects 1 MiB. Compiler scratch is host memory.
    source_limit: usize,
};

/// Opaque to C (`typedef struct p4_luau p4_luau`).
pub const P4Luau = extern struct {
    state: *State,
    config: Config,
    used: usize,
    interrupted: bool,
    error_text: [error_capacity]u8,
};

// ---------------------------------------------------------------------------------
// Internals

fn recordError(vm: *P4Luau, message: ?[*:0]const u8) void {
    const text: [*:0]const u8 = message orelse "Luau error (non-string error value)";
    var n: usize = 0;
    while (n + 1 < error_capacity and text[n] != 0) n += 1;
    @memcpy(vm.error_text[0..n], text[0..n]);
    vm.error_text[n] = 0;
}

fn errorMessage(L: *State) ?[*:0]const u8 {
    // lua_tolstring can allocate for numbers: never coerce an arbitrary error value
    // after leaving the protected VM frame, especially at the cap.
    return if (lua.lua_type(L, -1) == LUA_TSTRING) lua.lua_tolstring(L, -1, null) else null;
}

fn blockBytes(p: *anyopaque, len: usize) []u8 {
    return @as([*]u8, @ptrCast(p))[0..len];
}

/// lua_Alloc with an exact byte cap. The VM passes the exact block size as `oldsize`
/// on reallocation/free; a fresh allocation (ptr == null) may carry a type tag there.
fn allocate(ud: ?*anyopaque, ptr: ?*anyopaque, oldsize_in: usize, size: usize) callconv(.c) ?*anyopaque {
    const vm: *P4Luau = @ptrCast(@alignCast(ud.?));
    const oldsize: usize = if (ptr == null) 0 else oldsize_in;
    const ra = @returnAddress();
    if (size == 0) {
        if (ptr) |p| host.rawFree(blockBytes(p, oldsize), vm_alignment, ra);
        vm.used -= oldsize;
        return null;
    }
    const retained = vm.used - oldsize;
    if (size > vm.config.memory_limit - retained) return null;
    const result: [*]u8 = if (ptr) |p| blk: {
        const old = blockBytes(p, oldsize);
        if (host.rawRemap(old, vm_alignment, size, ra)) |moved| break :blk moved;
        // On failure the original block stays valid and accounting is unchanged.
        const fresh = host.rawAlloc(size, vm_alignment, ra) orelse return null;
        const keep = @min(oldsize, size);
        @memcpy(fresh[0..keep], old[0..keep]);
        host.rawFree(old, vm_alignment, ra);
        break :blk fresh;
    } else host.rawAlloc(size, vm_alignment, ra) orelse return null;
    vm.used = retained + size;
    return result;
}

fn owner(L: *State) *P4Luau {
    var ud: ?*anyopaque = null;
    _ = lua.lua_getallocf(L, &ud);
    return @ptrCast(@alignCast(ud.?));
}

/// luaL_error(L, "Execution budget exhausted"): position prefix, message, raise.
fn raiseBudgetExhausted(L: *State) noreturn {
    stdlib.luaL_where(L, 1);
    lua.lua_pushstring(L, "Execution budget exhausted");
    lua.lua_concat(L, 2);
    lua.lua_error(L);
}

fn interrupt(L_: ?*State, gc: c_int) callconv(.c) void {
    const L = L_.?;
    const vm = owner(L);
    // Upstream uses -1 for execution and nonnegative GC phase numbers.
    const collecting = gc >= 0;
    if (vm.config.interrupt) |callback| {
        if (callback(vm.config.user, @intFromBool(collecting)) != 0) vm.interrupted = true;
    }
    // Collector callbacks may not raise errors. The next execution safepoint observes
    // the latch, even if a nested pcall caught the original error.
    if (vm.interrupted and !collecting) raiseBudgetExhausted(L);
}

fn initialize(L_: ?*State) callconv(.c) c_int {
    const L = L_.?;
    // base/coroutine/table/string/math/utf8/bit32/buffer only: never os, debug,
    // vector, integer or class.
    stdlib.p4pc_openlibs_sandbox(L);
    const vm = owner(L);
    if (vm.config.initialize) |callback| callback(L, vm.config.user);
    stdlib.luaL_sandbox(L);
    stdlib.luaL_sandboxthread(L);
    // Host globals may expose mutable metatable-backed views. The thread environment
    // is reused for multiple chunks, so GETIMPORT must not cache values from those
    // views, even during the first chunk. This changes only the optimization flag;
    // luaL_sandbox's readonly tables remain frozen.
    lua.lua_setsafeenv(L, LUA_GLOBALSINDEX, 0);
    return 0;
}

const Load = struct {
    name: [*:0]const u8,
    bytes: []const u8,
    ref: c_int,
    status: c_int,
    vm: *P4Luau,
};

fn load(L_: ?*State) callconv(.c) c_int {
    const L = L_.?;
    const ctx: *Load = @ptrCast(@alignCast(lua.lua_touserdata(L, 1).?));
    ctx.status = lua.luau_load(L, ctx.name, ctx.bytes.ptr, ctx.bytes.len, 0);
    if (ctx.status != LUA_OK) {
        recordError(ctx.vm, errorMessage(L));
        // luau_load returns 1, not a lua_Status. An internally compiled error blob
        // starts with zero; valid compiler output can only fail OOM.
        ctx.status = if (ctx.bytes.len != 0 and ctx.bytes[0] == 0) LUA_ERRSYNTAX else LUA_ERRMEM;
    } else ctx.ref = lua.lua_ref(L, -1);
    return 0;
}

fn validSource(vm: *P4Luau, name: ?[*:0]const u8, source: ?[*]const u8, size: usize) bool {
    vm.error_text[0] = 0;
    if (name == null or (source == null and size != 0) or size > vm.config.source_limit) {
        recordError(vm, "invalid source arguments or source exceeds limit");
        return false;
    }
    return true;
}

/// Compiles with upstream default options into host memory. The compiler's only
/// failure mode is OutOfMemory; caller owns the result (`Output.deinit(host)`).
fn compileSource(vm: *P4Luau, source: ?[*]const u8, size: usize) ?compiler.Output {
    const text: []const u8 = if (source) |s| s[0..size] else "";
    return compiler.compile(host, text, .{}) catch {
        recordError(vm, "compiler out of memory");
        return null;
    };
}

/// snprintf(error, 2048, "%s%.*s", chunk, message_size, blob + 1) of the C++ bridge.
fn recordCompileError(vm: *P4Luau, name: [*:0]const u8, blob: []const u8) void {
    var chunk: []const u8 = std.mem.span(name);
    if (chunk.len != 0 and (chunk[0] == '@' or chunk[0] == '=')) chunk = chunk[1..];
    const chunk_limit = error_capacity / 2;
    if (chunk.len > chunk_limit) chunk = chunk[chunk.len - chunk_limit ..];
    var message = blob[1..];
    if (message.len >= error_capacity) message = message[0 .. error_capacity - 1];
    // %.*s stops at an embedded NUL.
    if (std.mem.findScalar(u8, message, 0)) |end| message = message[0..end];
    var n: usize = 0;
    for ([_][]const u8{ chunk, message }) |part| {
        const take = @min(part.len, error_capacity - 1 - n);
        @memcpy(vm.error_text[n..][0..take], part[0..take]);
        n += take;
    }
    vm.error_text[n] = 0;
}

// ---------------------------------------------------------------------------------
// C ABI

/// NULL on initialization failure (including too-small memory cap). Opens only
/// base/coroutine/table/string/math/utf8/bit32/buffer, then luaL_sandbox +
/// luaL_sandboxthread.
pub export fn p4_luau_create(config: ?*const Config) ?*P4Luau {
    const cfg = config orelse return null;
    if (cfg.memory_limit == 0) return null;
    const vm = host.create(P4Luau) catch return null;
    vm.* = .{
        .state = undefined,
        .config = cfg.*,
        .used = 0,
        .interrupted = false,
        .error_text = @splat(0),
    };
    if (vm.config.source_limit == 0) vm.config.source_limit = default_source_limit;
    vm.state = lua.lua_newstate(&allocate, vm) orelse {
        host.destroy(vm);
        return null;
    };
    if (lua.lua_cpcall(vm.state, &initialize, null) != LUA_OK) {
        lua.lua_close(vm.state);
        host.destroy(vm);
        return null;
    }
    lua.lua_callbacks(vm.state).interrupt = &interrupt;
    return vm;
}

pub export fn p4_luau_destroy(vm: ?*P4Luau) void {
    if (vm) |v| {
        lua.lua_close(v.state);
        host.destroy(v);
    }
}

pub export fn p4_luau_state(vm: *P4Luau) *State {
    return vm.state;
}

/// Works on all coroutines created from this VM, without callback userdata.
pub export fn p4_luau_user(L: *State) ?*anyopaque {
    return owner(L).config.user;
}

pub export fn p4_luau_memory_used(vm: *const P4Luau) usize {
    return vm.used;
}

pub export fn p4_luau_reset_interrupt(vm: *P4Luau) void {
    vm.interrupted = false;
}

pub export fn p4_luau_error(vm: *const P4Luau) [*:0]const u8 {
    return @ptrCast(&vm.error_text);
}

/// Compile SOURCE and discard private bytecode, without loading or running it. The
/// Lua stack/heap and the interrupt latch are unchanged.
pub export fn p4_luau_check_source(vm: *P4Luau, name: ?[*:0]const u8, source: ?[*]const u8, size: usize) c_int {
    if (!validSource(vm, name, source, size)) return LUA_ERRSYNTAX;
    var output = compileSource(vm, source, size) orelse return LUA_ERRMEM;
    defer output.deinit(host);
    const blob = output.bytecode;
    if (blob.len != 0 and blob[0] == 0) {
        // Compiler errors are a zero byte followed by a length-delimited
        // ":line: message", not a NUL-terminated string. Never load the blob.
        recordCompileError(vm, name.?, blob);
        return LUA_ERRSYNTAX;
    }
    return LUA_OK;
}

/// Compile SOURCE in-process, then load. Success pushes one function; failure
/// restores the previous stack.
pub export fn p4_luau_load_source(vm: *P4Luau, name: ?[*:0]const u8, source: ?[*]const u8, size: usize) c_int {
    if (!validSource(vm, name, source, size)) return LUA_ERRSYNTAX;
    var output = compileSource(vm, source, size) orelse return LUA_ERRMEM;
    defer output.deinit(host);
    var ctx: Load = .{ .name = name.?, .bytes = output.bytecode, .ref = LUA_NOREF, .status = LUA_OK, .vm = vm };
    const L = vm.state;
    const top = lua.lua_gettop(L);
    const status = lua.lua_cpcall(L, &load, &ctx);
    if (status != LUA_OK) {
        recordError(vm, errorMessage(L));
        lua.lua_settop(L, top);
        return status;
    }
    if (ctx.status != LUA_OK) return ctx.status;
    // lua_cpcall reserved two slots above `top` on this frame before calling `load`,
    // so pushing the result cannot grow the stack (and cannot raise) here.
    _ = lua.lua_getref(L, ctx.ref);
    _ = lua.lua_unref(L, ctx.ref);
    return LUA_OK;
}

/// Standard lua_pcall stack contract, but failure pops/records the error. Sticky
/// budget exhaustion cannot be hidden from the host by a script-side pcall.
pub export fn p4_luau_run(vm: *P4Luau, nargs: c_int, nresults: c_int) c_int {
    vm.error_text[0] = 0;
    const L = vm.state;
    const base = lua.lua_gettop(L) - nargs - 1;
    const status = lua.lua_pcall(L, nargs, nresults, 0);
    if (vm.interrupted) {
        recordError(vm, if (status != LUA_OK) errorMessage(L) else "Execution budget exhausted");
        lua.lua_settop(L, base);
        return LUA_ERRRUN;
    }
    if (status != LUA_OK) {
        recordError(vm, errorMessage(L));
        lua.lua_pop(L, 1);
    }
    return status;
}

comptime {
    // Root this library's C ABI: VM (lua.h) and stdlib (lualib.h) exports.
    _ = lua;
    _ = stdlib;
}
