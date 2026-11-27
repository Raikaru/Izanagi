//! Zig-facing Luau state: allocator-owned lifetime, protected source execution and typed
//! access to the values a chunk returns.
//!
//! Error unwinding: the VM reports errors by unwinding non-locally (see vm/jmp.zig): a
//! raising operation jumps straight to the innermost protected call and skips every Zig
//! frame in between, including their `defer`/`errdefer` blocks. This type therefore keeps
//! every VM operation that may allocate, grow the stack, run Luau code or raise (library
//! setup, chunk loading and execution, garbage collection) inside `lua_pcall`/`lua_cpcall`
//! callbacks whose frames own nothing. Outside them it only reads stack values without
//! conversion, shrinks the stack, and pushes two values onto an emptied stack (within the
//! `LUA_MINSTACK` slots the base frame always reserves, so no growth is needed). Zig
//! cleanup scopes such as the compiled bytecode's `defer` live above the protection
//! boundary, so a Luau error returns to them normally and surfaces as a Zig error.
//!
//! Allocator: `init` stores the allocator in a heap block owned by the state, and the VM
//! keeps that block's address as its `lua_Alloc` userdata. The address therefore never
//! moves while the VM lives. `State` is a single-owner value: it may be moved, but must
//! not be used or deinitialized through multiple copies. VM blocks use 16-byte alignment.
//! The allocator must outlive the state; it is also used for compilation inside `run`.
//!
//! Close: `deinit` closes the VM, which frees every Luau object, then frees the heap
//! block. Views (`Results`, `Value.string`, `errorValue`, `errorMessage`) must not be used
//! afterwards.
//!
//! Value lifetime: values are anchored on the VM stack, not copied. A successful `run`
//! leaves the chunk's results there; a VM failure leaves its error value. Failure before
//! entering the VM, such as compiler allocation failure, may leave no error value.
//! Anchored values survive garbage collection until the next `run`, `reset` or `deinit`,
//! which invalidate all views. Copy string bytes out to keep them longer.
//!
//! Yielding: chunks run inside a C boundary and cannot yield; `coroutine.yield` from
//! the chunk itself fails with `error.Runtime` (coroutines created by the chunk work).
//! Debug-break hooks installed through `L` surface as `error.Break`.
//!
//! Scope: this is not a wrapper of the whole C API and has no native callback
//! registration. Code that needs more uses the raw `L` handle and `vm.lua_*` directly and
//! must then uphold the same rules itself: native callbacks registered that way are not
//! made safe by this type, and any Zig cleanup they hold across a raising call is skipped.

const std = @import("std");
const Allocator = std.mem.Allocator;
const vm = @import("vm.zig");
const stdlib = @import("stdlib.zig");
const compiler = @import("compiler.zig");

const State = @This();

/// Raw VM handle for operations this type does not cover. Its stack holds the values of
/// the latest `run`; callers that push or pop through it invalidate `Results` and
/// `errorValue` views and must leave raising operations under a protected call.
L: *vm.lua_State,
host: *Host,
/// Registry reference of the protected chunk runner created during `init`.
runner_ref: c_int,

/// Heap-pinned allocator context: the VM keeps its address as the `lua_Alloc` userdata.
const Host = struct {
    allocator: Allocator,
    /// Set when the allocator refused a request; distinguishes allocation failure from an
    /// unsupported unwinding environment when `lua_newstate` returns null.
    alloc_failed: bool = false,
    /// Bumped whenever the stack values that `Results` views point at are released.
    generation: u32 = 0,
    /// Whether the value on top of the stack is the error of the latest `run`.
    failed: bool = false,

    fn track(host: *Host, block: ?[*]u8) ?[*]u8 {
        if (block == null) host.alloc_failed = true;
        return block;
    }
};

/// Strictest VM object alignment (matches the default `luaL_newstate` allocator).
const alignment: std.mem.Alignment = .@"16";

fn allocate(ud: ?*anyopaque, ptr: ?*anyopaque, osize: usize, nsize: usize) callconv(.c) ?*anyopaque {
    const host: *Host = @ptrCast(@alignCast(ud.?));
    const a = host.allocator;
    const ra = @returnAddress();
    if (nsize == 0) {
        if (ptr) |p| a.rawFree(@as([*]u8, @ptrCast(p))[0..osize], alignment, ra);
        return null;
    }
    // a fresh allocation passes a type tag in osize
    const p = ptr orelse return host.track(a.rawAlloc(nsize, alignment, ra));
    const old = @as([*]u8, @ptrCast(p))[0..osize];
    if (a.rawRemap(old, alignment, nsize, ra)) |moved| return moved;
    const fresh = host.track(a.rawAlloc(nsize, alignment, ra)) orelse return null;
    const keep = @min(osize, nsize);
    @memcpy(fresh[0..keep], old[0..keep]);
    a.rawFree(old, alignment, ra);
    return fresh;
}

pub const Options = struct {
    /// Open the standard libraries (`luaL_openlibs`).
    open_libs: bool = true,
    /// Make builtin metatables and globals read-only after setup (`luaL_sandbox`).
    sandbox: bool = false,
};

pub const InitError = error{
    OutOfMemory,
    /// The VM refuses to start because its errors could not be unwound safely here
    /// (for example a process running with user-mode shadow stacks).
    UnwindingUnsupported,
    /// Library setup raised something other than a memory error.
    SetupFailed,
};

const Setup = struct {
    options: Options,
    runner_ref: c_int = vm.LUA_NOREF,
};

/// Protected `init` body; runs under `lua_cpcall` and owns no Zig resources.
fn setupProtected(L_: ?*vm.lua_State) callconv(.c) c_int {
    const L = L_.?;
    const setup: *Setup = @ptrCast(@alignCast(vm.lua_touserdata(L, 1).?));
    if (setup.options.open_libs) stdlib.luaL_openlibs(L);
    vm.lua_pushcclosurek(L, &runProtected, "State.run", 0, null);
    setup.runner_ref = vm.lua_ref(L, -1);
    if (setup.options.sandbox) stdlib.luaL_sandbox(L);
    return 0;
}

/// Creates a state whose VM memory, and this type's bookkeeping, come from `allocator`.
/// `allocator` must outlive the state; release it with `deinit`.
pub fn init(allocator: Allocator, options: Options) InitError!State {
    const host = try allocator.create(Host);
    errdefer allocator.destroy(host);
    host.* = .{ .allocator = allocator };

    const L = vm.lua_newstate(&allocate, host) orelse
        return if (host.alloc_failed) error.OutOfMemory else error.UnwindingUnsupported;
    errdefer vm.lua_close(L);

    var setup: Setup = .{ .options = options };
    switch (vm.lua_cpcall(L, &setupProtected, &setup)) {
        vm.LUA_OK => {},
        vm.LUA_ERRMEM => return error.OutOfMemory,
        else => return error.SetupFailed,
    }
    return .{ .L = L, .host = host, .runner_ref = setup.runner_ref };
}

/// Closes the VM (freeing every object it allocated) and releases the bookkeeping.
pub fn deinit(self: *State) void {
    const allocator = self.host.allocator;
    vm.lua_close(self.L);
    allocator.destroy(self.host);
    self.* = undefined;
}

/// Releases the values of the latest `run` (results or error), invalidating every view
/// into them.
pub fn reset(self: *State) void {
    vm.lua_settop(self.L, 0); // shrinking the stack never raises
    self.host.generation +%= 1;
    self.host.failed = false;
}

pub const RunError = error{
    /// The Zig compiler or the VM ran out of memory.
    OutOfMemory,
    /// The source failed to parse or compile; `errorMessage` has the diagnostic.
    Compile,
    /// The VM rejected the compiled bytecode; `errorMessage` says why.
    Load,
    /// The chunk raised an error; `errorValue`/`errorMessage` return it.
    Runtime,
    /// An error was raised while handling another error.
    ErrorHandler,
    /// Execution stopped at a debugger break (only with hooks installed through `L`).
    Break,
    /// The VM returned a status this API does not know.
    Unexpected,
};

const Chunk = struct {
    host: *Host,
    name: [*:0]const u8,
    bytecode: []const u8,
    load_failed: bool = false,
    /// luau_load reports its own memory errors as a returned message, not a raise.
    load_out_of_memory: bool = false,
};

/// Protected `run` body: loads and calls the chunk, returning all of its results. It runs
/// under `lua_pcall` and owns no Zig resources, so a raise here unwinds only VM frames.
fn runProtected(L_: ?*vm.lua_State) callconv(.c) c_int {
    const L = L_.?;
    const chunk: *Chunk = @ptrCast(@alignCast(vm.lua_touserdata(L, 1).?));
    chunk.host.alloc_failed = false;
    if (vm.luau_load(L, chunk.name, chunk.bytecode.ptr, chunk.bytecode.len, 0) != 0) {
        chunk.load_failed = true;
        chunk.load_out_of_memory = chunk.host.alloc_failed;
        vm.lua_error(L); // message pushed by luau_load
    }
    vm.lua_call(L, 0, vm.LUA_MULTRET);
    return vm.lua_gettop(L) - 1; // everything above the chunk argument
}

/// Compiles `source` with the state's allocator and runs it in a protected call.
/// Releases the previous run's values first. On success the chunk's return values stay
/// anchored on the stack until the next `run`, `reset` or `deinit`. A VM error value,
/// when present, is readable through `errorValue`/`errorMessage`.
/// `chunkname` follows Luau conventions ("=name" or "@path").
pub fn run(self: *State, chunkname: [:0]const u8, source: []const u8, options: compiler.Options) RunError!Results {
    self.reset();
    const allocator = self.host.allocator;
    var output = try compiler.compile(allocator, source, options);
    defer output.deinit(allocator);

    // Compile errors also go through luau_load, which turns the error blob into the
    // same "chunkname:line: message" value upstream produces.
    var chunk: Chunk = .{ .host = self.host, .name = chunkname.ptr, .bytecode = output.bytecode };
    // Both pushes fit in the base frame's reserved slots of the now-empty stack.
    _ = vm.lua_getref(self.L, self.runner_ref);
    vm.lua_pushlightuserdatatagged(self.L, &chunk, 0);
    const status = vm.lua_pcall(self.L, 1, vm.LUA_MULTRET, 0);
    if (status == vm.LUA_OK) {
        const top = vm.lua_gettop(self.L);
        return .{ .L = self.L, .host = self.host, .generation = self.host.generation, .len = @intCast(top) };
    }
    self.host.failed = true;
    return switch (status) {
        vm.LUA_ERRMEM => error.OutOfMemory,
        vm.LUA_ERRRUN => if (!chunk.load_failed)
            error.Runtime
        else if (chunk.load_out_of_memory)
            error.OutOfMemory
        else if (output.ok())
            error.Load
        else
            error.Compile,
        vm.LUA_ERRSYNTAX => error.Compile,
        vm.LUA_ERRERR => error.ErrorHandler,
        vm.LUA_BREAK => error.Break,
        else => error.Unexpected,
    };
}

/// Runs a full garbage collection cycle in a protected call. Values of the latest `run`
/// stay anchored.
pub fn collectGarbage(self: *State) error{ OutOfMemory, Unexpected }!void {
    switch (vm.lua_cpcall(self.L, &collectProtected, null)) {
        vm.LUA_OK => {},
        else => |status| {
            vm.lua_settop(self.L, -2); // drop the error lua_cpcall left above our values
            return if (status == vm.LUA_ERRMEM) error.OutOfMemory else error.Unexpected;
        },
    }
}

fn collectProtected(L_: ?*vm.lua_State) callconv(.c) c_int {
    _ = vm.lua_gc(L_.?, vm.LUA_GCCOLLECT, 0);
    return 0;
}

/// The error value of the latest failed `run`, or null if it produced no VM error value.
pub fn errorValue(self: *const State) ?Value {
    if (!self.host.failed) return null;
    return valueAt(self.L, -1);
}

/// The error message of the latest failed `run` when the error value is a string. The
/// bytes stay valid until the next `run`, `reset` or `deinit`.
pub fn errorMessage(self: *const State) ?[]const u8 {
    const v = self.errorValue() orelse return null;
    return switch (v) {
        .string => |s| s,
        else => null,
    };
}

/// Luau value types (`lua_type`); non-exhaustive so new VM types never trap.
pub const Type = enum(c_int) {
    none = vm.LUA_TNONE,
    nil = vm.LUA_TNIL,
    boolean = vm.LUA_TBOOLEAN,
    lightuserdata = vm.LUA_TLIGHTUSERDATA,
    number = vm.LUA_TNUMBER,
    integer = vm.LUA_TINTEGER,
    vector = vm.LUA_TVECTOR,
    string = vm.LUA_TSTRING,
    table = vm.LUA_TTABLE,
    function = vm.LUA_TFUNCTION,
    userdata = vm.LUA_TUSERDATA,
    thread = vm.LUA_TTHREAD,
    buffer = vm.LUA_TBUFFER,
    class = vm.LUA_TCLASS,
    object = vm.LUA_TOBJECT,
    _,
};

/// A stack value read without conversion (reading never allocates or raises).
pub const Value = union(enum) {
    nil,
    boolean: bool,
    number: f64,
    integer: i64,
    /// Bytes of a string anchored on the state's stack; valid until the next `run`,
    /// `reset` or `deinit`. Not NUL-terminated as far as the slice is concerned.
    string: []const u8,
    /// Any other type; inspect it through the raw `L` handle if needed.
    other: Type,
};

fn valueAt(L: *vm.lua_State, idx: c_int) Value {
    const t: Type = @enumFromInt(vm.lua_type(L, idx));
    return switch (t) {
        .nil => .nil,
        .boolean => .{ .boolean = vm.lua_toboolean(L, idx) != 0 },
        .number => .{ .number = vm.lua_tonumberx(L, idx, null) },
        .integer => .{ .integer = vm.lua_tointeger64(L, idx, null) },
        .string => blk: {
            var len: usize = 0;
            // the atom variant never converts in place, unlike lua_tolstring
            const p = vm.lua_tolstringatom(L, idx, &len, null).?;
            break :blk .{ .string = p[0..len] };
        },
        else => .{ .other = t },
    };
}

/// View of the values a successful `run` returned. Valid until the next `run`, `reset`
/// or `deinit` of the state that produced it.
pub const Results = struct {
    L: *vm.lua_State,
    host: *const Host,
    generation: u32,
    len: usize,

    /// The `i`th returned value (0-based).
    pub fn get(self: Results, i: usize) Value {
        std.debug.assert(self.host.generation == self.generation); // used after run/reset
        std.debug.assert(i < self.len);
        return valueAt(self.L, @intCast(i + 1));
    }
};

const testing = std.testing;

test "run returns typed results" {
    var state = try State.init(testing.allocator, .{});
    defer state.deinit();
    const r = try state.run("=typed", "return 1.5, true, nil, 'abc', {}", .{});
    try testing.expectEqual(@as(usize, 5), r.len);
    try testing.expectEqual(Value{ .number = 1.5 }, r.get(0));
    try testing.expectEqual(Value{ .boolean = true }, r.get(1));
    try testing.expectEqual(@as(Value, .nil), r.get(2));
    try testing.expectEqualStrings("abc", r.get(3).string);
    try testing.expectEqual(Value{ .other = .table }, r.get(4));
    try testing.expectEqual(@as(?Value, null), state.errorValue());
}

test "compile and runtime errors surface as Zig errors with anchored messages" {
    var state = try State.init(testing.allocator, .{});
    defer state.deinit();

    try testing.expectError(error.Compile, state.run("=bad", "return +", .{}));
    try testing.expect(std.mem.indexOf(u8, state.errorMessage().?, "bad:1:") != null);

    try testing.expectError(error.Runtime, state.run("=boom", "error('kaboom')", .{}));
    try testing.expect(std.mem.endsWith(u8, state.errorMessage().?, "kaboom"));

    try testing.expectError(error.Runtime, state.run("=tbl", "error({})", .{}));
    try testing.expectEqual(Value{ .other = .table }, state.errorValue().?);
    try testing.expectEqual(@as(?[]const u8, null), state.errorMessage());

    // the state stays usable after errors
    const r = try state.run("=ok", "return 7", .{});
    try testing.expectEqual(Value{ .number = 7 }, r.get(0));
    try testing.expectEqual(@as(?Value, null), state.errorValue());
}

test "result strings stay anchored across garbage collection" {
    var state = try State.init(testing.allocator, .{});
    defer state.deinit();
    // built at run time, so once the chunk returns only the stack keeps it alive; the
    // testing allocator poisons freed memory, so a collected string would not compare equal
    const r = try state.run("=anchor", "return string.rep('ab', 64) .. tostring(42)", .{});
    const s = r.get(0).string;
    try state.collectGarbage();
    try state.collectGarbage();
    try testing.expectEqual(@as(usize, 130), s.len);
    try testing.expect(std.mem.startsWith(u8, s, "abab"));
    try testing.expect(std.mem.endsWith(u8, s, "ab42"));
    // a later run releases it; fresh views come from the new results
    const again = try state.run("=again", "return 'next'", .{});
    try testing.expectEqualStrings("next", again.get(0).string);
}

test "state can be moved after init because the allocator context is heap-pinned" {
    const Holder = struct { state: State };
    const original = try State.init(testing.allocator, .{});
    var holder: Holder = .{ .state = original }; // moved; `original` is no longer used
    defer holder.state.deinit();
    const r = try holder.state.run("=moved", "local t = {} for i = 1, 200 do t[i] = {i} end return #t", .{});
    try testing.expectEqual(Value{ .number = 200 }, r.get(0));
}

test "chunks cannot yield out of run" {
    var state = try State.init(testing.allocator, .{});
    defer state.deinit();
    try testing.expectError(error.Runtime, state.run("=yield", "coroutine.yield(1)", .{}));
    try testing.expect(std.mem.indexOf(u8, state.errorMessage().?, "attempt to yield") != null);
    const r = try state.run("=after", "return 1", .{});
    try testing.expectEqual(Value{ .number = 1 }, r.get(0));
}

test "sandboxed state rejects global writes" {
    var state = try State.init(testing.allocator, .{ .sandbox = true });
    defer state.deinit();
    try testing.expectError(error.Runtime, state.run("=sb", "string = nil", .{}));
    const r = try state.run("=sb", "return math.max(2, 3)", .{});
    try testing.expectEqual(Value{ .number = 3 }, r.get(0));
}

test "allocation failure is reported as error.OutOfMemory without leaks" {
    const source = "local t = {} for i = 1, 50 do t[i] = tostring(i) end return table.concat(t, ',')";
    var fail_index: usize = 0;
    var saw_init_failure = false;
    var saw_run_failure = false;
    while (true) : (fail_index += 1) {
        var failing = testing.FailingAllocator.init(testing.allocator, .{ .fail_index = fail_index });
        var state = State.init(failing.allocator(), .{}) catch |err| {
            try testing.expectEqual(error.OutOfMemory, err);
            saw_init_failure = true;
            continue;
        };
        defer state.deinit();
        const r = state.run("=oom", source, .{}) catch |err| {
            try testing.expectEqual(error.OutOfMemory, err);
            saw_run_failure = true;
            continue;
        };
        try testing.expect(std.mem.startsWith(u8, r.get(0).string, "1,2,3,"));
        break;
    }
    try testing.expect(saw_init_failure);
    try testing.expect(saw_run_failure);
}
