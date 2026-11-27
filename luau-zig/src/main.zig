//! Command-line source interpreter. Native code generation, profiling and
//! coverage are not implemented here.
const std = @import("std");
const luau = @import("luau");
const lua = luau.vm;
const libs = luau.stdlib;
const compiler = luau.compiler;
const Allocator = std.mem.Allocator;

const Host = struct {
    io: std.Io,
    allocator: Allocator,
    options: compiler.Options,
};

// A single allocator backs both compiler allocations and the VM. Lua supplies
// a type tag rather than an old size when ptr is null.
fn allocate(ud: ?*anyopaque, ptr: ?*anyopaque, old_size: usize, new_size: usize) callconv(.c) ?*anyopaque {
    const host: *Host = @ptrCast(@alignCast(ud.?));
    const a = host.allocator;
    const alignment: std.mem.Alignment = .@"16";
    const ra = @returnAddress();
    if (new_size == 0) {
        if (ptr) |p| a.rawFree(@as([*]u8, @ptrCast(p))[0..old_size], alignment, ra);
        return null;
    }
    const p = ptr orelse return a.rawAlloc(new_size, alignment, ra);
    const old = @as([*]u8, @ptrCast(p))[0..old_size];
    if (a.rawRemap(old, alignment, new_size, ra)) |moved| return moved;
    const fresh = a.rawAlloc(new_size, alignment, ra) orelse return null;
    @memcpy(fresh[0..@min(old_size, new_size)], old[0..@min(old_size, new_size)]);
    a.rawFree(old, alignment, ra);
    return fresh;
}

const LoadString = struct {
    name: [*:0]const u8,
    bytes: []const u8,
    status: c_int = 0,
    ref: c_int = lua.LUA_NOREF,
};

fn loadStringProtected(state: ?*lua.lua_State) callconv(.c) c_int {
    const L = state.?;
    const ctx: *LoadString = @ptrCast(@alignCast(lua.lua_touserdata(L, 1).?));
    ctx.status = lua.luau_load(L, ctx.name, ctx.bytes.ptr, ctx.bytes.len, 0);
    ctx.ref = lua.lua_ref(L, -1);
    return 0;
}

fn loadString(state: ?*lua.lua_State) callconv(.c) c_int {
    const L = state.?;
    var len: usize = 0;
    const source = libs.luaL_checklstring(L, 1, &len);
    const name = libs.luaL_optlstring(L, 2, source, null).?;
    lua.lua_setsafeenv(L, lua.LUA_ENVIRONINDEX, 0);
    var ud: ?*anyopaque = null;
    _ = lua.lua_getallocf(L, &ud);
    const host: *Host = @ptrCast(@alignCast(ud.?));
    var compiled = compiler.compile(host.allocator, source[0..len], host.options) catch {
        lua.lua_pushstring(L, "not enough memory");
        lua.lua_error(L);
    };
    var ctx: LoadString = .{ .name = name, .bytes = compiled.bytecode };
    const status = lua.lua_cpcall(L, &loadStringProtected, &ctx);
    compiled.deinit(host.allocator);
    if (status != lua.LUA_OK) lua.lua_error(L);
    _ = lua.lua_getref(L, ctx.ref);
    _ = lua.lua_unref(L, ctx.ref);
    if (ctx.status == 0) return 1;
    lua.lua_pushnil(L);
    lua.lua_insert(L, -2);
    return 2;
}

fn collectGarbage(state: ?*lua.lua_State) callconv(.c) c_int {
    const L = state.?;
    const option = libs.luaL_optlstring(L, 1, "collect", null).?;
    if (std.mem.eql(u8, std.mem.span(option), "collect")) {
        _ = lua.lua_gc(L, lua.LUA_GCCOLLECT, 0);
        return 0;
    }
    if (std.mem.eql(u8, std.mem.span(option), "count")) {
        const count = lua.lua_gc(L, lua.LUA_GCCOUNT, 0);
        lua.lua_pushnumber(L, @floatFromInt(count));
        return 1;
    }
    lua.lua_pushstring(L, "collectgarbage must be called with 'count' or 'collect'");
    lua.lua_error(L);
}

fn initialize(state: ?*lua.lua_State) callconv(.c) c_int {
    const L = state.?;
    const host: *Host = @ptrCast(@alignCast(lua.lua_touserdata(L, 1).?));
    libs.luaL_openlibs(L);
    lua.lua_pushcfunction(L, &loadString, "loadstring");
    lua.lua_setglobal(L, "loadstring");
    lua.lua_pushcfunction(L, &collectGarbage, "collectgarbage");
    lua.lua_setglobal(L, "collectgarbage");
    _ = luau.require.cli.open(L, host.io, host.allocator, &host.options);
    libs.luaL_sandbox(L);
    return 0;
}

const ThreadContext = struct {
    thread: ?*lua.lua_State = null,
    ref: c_int = lua.LUA_NOREF,
};

fn newScriptThread(state: ?*lua.lua_State) callconv(.c) c_int {
    const L = state.?;
    const ctx: *ThreadContext = @ptrCast(@alignCast(lua.lua_touserdata(L, 1).?));
    const thread = lua.lua_newthread(L);
    ctx.ref = lua.lua_ref(L, -1);
    ctx.thread = thread;
    return 0;
}

fn sandboxScriptThread(state: ?*lua.lua_State) callconv(.c) c_int {
    libs.luaL_sandboxthread(state.?);
    return 0;
}

const Run = struct {
    allocator: Allocator,
    name: [*:0]const u8,
    bytecode: []const u8,
    args: []const [:0]const u8,
    print_results: bool,
    status: c_int = 0,
    message: ?[]u8 = null,
};

fn captureError(L: *lua.lua_State, run: *Run) void {
    const text: []const u8 = if (lua.lua_type(L, -1) == lua.LUA_TSTRING) blk: {
        var len: usize = 0;
        const p = lua.lua_tolstring(L, -1, &len) orelse break :blk "unknown error";
        break :blk p[0..len];
    } else "error object is not a string";
    run.message = run.allocator.dupe(u8, text) catch null;
}

// This callback is invoked by lua_cpcall. Lua may unwind out of it; no Zig
// allocation requiring cleanup is held here. The outer frame owns Run/bytecode.
fn execute(state: ?*lua.lua_State) callconv(.c) c_int {
    const L = state.?;
    const run: *Run = @ptrCast(@alignCast(lua.lua_touserdata(L, 1).?));
    if (lua.luau_load(L, run.name, run.bytecode.ptr, run.bytecode.len, 0) != 0) {
        run.status = lua.LUA_ERRSYNTAX;
        captureError(L, run);
        return 0;
    }
    for (run.args) |arg| lua.lua_pushlstring(L, arg.ptr, arg.len);
    run.status = lua.lua_pcall(L, @intCast(run.args.len), if (run.print_results) lua.LUA_MULTRET else 0, 0);
    if (run.status != lua.LUA_OK) {
        captureError(L, run);
        return 0;
    }
    if (run.print_results) {
        const count = lua.lua_gettop(L) - 1; // argument 1 is the Run userdata
        if (count > 0) {
            _ = lua.lua_getglobal(L, "print");
            lua.lua_insert(L, 2);
            run.status = lua.lua_pcall(L, count, 0, 0);
            if (run.status != lua.LUA_OK) captureError(L, run);
        }
    }
    return 0;
}

const Outcome = struct {
    ok: bool,
    message: ?[]u8 = null,
    fn deinit(self: *Outcome, a: Allocator) void {
        if (self.message) |m| a.free(m);
    }
};

fn runSource(host: *Host, L: *lua.lua_State, name: [:0]const u8, source: []const u8, args: []const [:0]const u8, print_results: bool) !Outcome {
    var compiled = try compiler.compile(host.allocator, source, host.options);
    defer compiled.deinit(host.allocator);
    if (compiled.diagnostic) |d| return .{
        .ok = false,
        .message = try std.fmt.allocPrint(host.allocator, "{s}:{d}: {s}", .{ if (name.len > 0 and (name[0] == '@' or name[0] == '=')) name[1..] else name, d.location.begin.line + 1, d.message }),
    };
    if (args.len > std.math.maxInt(c_int)) return error.TooManyArguments;
    var run: Run = .{ .allocator = host.allocator, .name = name.ptr, .bytecode = compiled.bytecode, .args = args, .print_results = print_results };
    const status = lua.lua_cpcall(L, &execute, &run);
    if (status != lua.LUA_OK) {
        captureError(L, &run);
        lua.lua_pop(L, 1);
        return .{ .ok = false, .message = run.message };
    }
    return .{ .ok = run.status == lua.LUA_OK, .message = run.message };
}

fn output(io: std.Io, file: std.Io.File, text: []const u8) void {
    file.writeStreamingAll(io, text) catch {};
}
fn report(io: std.Io, file: std.Io.File, message: ?[]const u8) void {
    output(io, file, message orelse "not enough memory");
    output(io, file, "\n");
}

fn setRequireFlags(io: std.Io, a: Allocator, list: []const u8) !bool {
    var parts = std.mem.splitScalar(u8, list, ',');
    while (parts.next()) |part| {
        const separator = std.mem.indexOfScalar(u8, part, '=');
        const name = if (separator) |pos| part[0..pos] else part;
        const text = if (separator) |pos| part[pos + 1 ..] else "true";
        const value = if (std.mem.eql(u8, text, "true") or std.mem.eql(u8, text, "True"))
            true
        else if (std.mem.eql(u8, text, "false") or std.mem.eql(u8, text, "False"))
            false
        else {
            const message = try std.fmt.allocPrint(a, "Error: invalid flag value '{s}'.", .{text});
            defer a.free(message);
            report(io, .stderr(), message);
            return false;
        };
        if (!luau.require.flags.set(name, value)) {
            const message = try std.fmt.allocPrint(a, "Error: unsupported flag '{s}'; only require flags are supported.", .{name});
            defer a.free(message);
            report(io, .stderr(), message);
            return false;
        }
    }
    return true;
}

fn repl(io: std.Io, host: *Host, L: *lua.lua_State) !void {
    var input_buffer: [4096]u8 = undefined;
    var input = std.Io.File.stdin().readerStreaming(io, &input_buffer);
    var line: std.ArrayList(u8) = .empty;
    defer line.deinit(host.allocator);
    var source: std.ArrayList(u8) = .empty;
    defer source.deinit(host.allocator);
    while (true) {
        output(io, .stdout(), if (source.items.len == 0) "> " else ">> ");
        line.clearRetainingCapacity();
        var eof = false;
        while (true) {
            const byte = input.interface.takeByte() catch |err| switch (err) {
                error.EndOfStream => { eof = true; break; },
                else => return err,
            };
            if (byte == '\n') break;
            try line.append(host.allocator, byte);
        }
        if (eof and line.items.len == 0) break;
        if (source.items.len == 0) {
            const expr = try std.fmt.allocPrint(host.allocator, "return {s}", .{line.items});
            defer host.allocator.free(expr);
            var expression = try runSource(host, L, "=stdin", expr, &.{}, true);
            defer expression.deinit(host.allocator);
            if (expression.ok) continue;
        } else try source.append(host.allocator, '\n');
        try source.appendSlice(host.allocator, line.items);
        var result = try runSource(host, L, "=stdin", source.items, &.{}, true);
        defer result.deinit(host.allocator);
        if (!result.ok and result.message != null and std.mem.endsWith(u8, result.message.?, "<eof>") and !eof) continue;
        if (!result.ok) report(io, .stdout(), result.message);
        source.clearRetainingCapacity();
        if (eof) break;
    }
}

const usage =
    \\Usage: luau [options] [file list] [-a] [arg list]
    \\When no files are supplied an interactive REPL is started.
    \\  -h, --help         Display this message
    \\  -i, --interactive  Start a REPL after the last file
    \\  -O<n>              Optimization level 0-2 (default 1)
    \\  -g<n>              Debug level 0-2 (default 1)
    \\  -a, --program-args Pass remaining arguments to each script
    \\  --fflags=<list>    Set named require flags (name[=true|false], comma-separated)
    \\Native codegen, profiling and coverage are not supported.
    \\
;

fn readSource(io: std.Io, a: Allocator, path: []const u8) ![]u8 {
    const dir = std.Io.Dir.cwd();
    return dir.readFileAlloc(io, path, a, .unlimited) catch |original| {
        for ([_][]const u8{ ".luau", ".lua" }) |suffix| {
            const candidate = try std.fmt.allocPrint(a, "{s}{s}", .{ path, suffix });
            defer a.free(candidate);
            if (dir.readFileAlloc(io, candidate, a, .unlimited)) |bytes| return bytes else |_| {}
        }
        return original;
    };
}

pub fn main(init: std.process.Init) !u8 {
    const io = init.io;
    const a = init.gpa;
    const args = try init.minimal.args.toSlice(init.arena.allocator());
    var host: Host = .{ .io = io, .allocator = a, .options = .{ .type_info_level = 1 } };
    luau.require.flags.setAll(true);
    var files: std.ArrayList([]const u8) = .empty;
    defer files.deinit(a);
    var program_args: []const [:0]const u8 = &.{};
    var interactive = false;
    var i: usize = 1;
    while (i < args.len) : (i += 1) {
        const arg = args[i];
        if (std.mem.eql(u8, arg, "-h") or std.mem.eql(u8, arg, "--help")) {
            output(io, .stdout(), usage);
            return 0;
        } else if (std.mem.eql(u8, arg, "-i") or std.mem.eql(u8, arg, "--interactive")) {
            interactive = true;
        } else if (std.mem.eql(u8, arg, "-a") or std.mem.eql(u8, arg, "--program-args")) {
            program_args = args[i + 1 ..];
            break;
        } else if (std.mem.startsWith(u8, arg, "--fflags=")) {
            if (!try setRequireFlags(io, a, arg["--fflags=".len..])) return 1;
        } else if (std.mem.startsWith(u8, arg, "-O") or std.mem.startsWith(u8, arg, "-g")) {
            const level = std.fmt.parseInt(u8, arg[2..], 10) catch 255;
            if (level > 2) {
                report(io, .stderr(), if (arg[1] == 'O') "Error: Optimization level must be between 0 and 2 inclusive." else "Error: Debug level must be between 0 and 2 inclusive.");
                return 1;
            }
            if (arg[1] == 'O') host.options.optimization_level = level else host.options.debug_level = level;
        } else if (arg.len > 0 and arg[0] == '-') {
            const message = try std.fmt.allocPrint(a, "Error: Unrecognized or unsupported option '{s}'.", .{arg});
            defer a.free(message);
            report(io, .stderr(), message);
            return 1;
        } else try files.append(a, arg);
    }
    const global = lua.lua_newstate(&allocate, &host) orelse {
        report(io, .stderr(), "Error: unable to create Luau state");
        return 1;
    };
    defer lua.lua_close(global);
    if (lua.lua_cpcall(global, &initialize, &host) != lua.LUA_OK) {
        report(io, .stderr(), "Error: unable to initialize Luau standard libraries");
        return 1;
    }
    var failed = false;
    const count = @max(files.items.len, 1);
    for (0..count) |index| {
        var thread: ThreadContext = .{};
        if (lua.lua_cpcall(global, &newScriptThread, &thread) != lua.LUA_OK) {
            report(io, .stderr(), "Error: unable to create Luau script thread");
            return 1;
        }
        defer _ = lua.lua_unref(global, thread.ref);
        const L = thread.thread.?;
        if (lua.lua_cpcall(L, &sandboxScriptThread, null) != lua.LUA_OK) {
            report(io, .stderr(), "Error: unable to sandbox Luau script thread");
            return 1;
        }
        if (files.items.len != 0) {
            const filename = files.items[index];
            const source = readSource(io, a, filename) catch {
                const msg = try std.fmt.allocPrint(a, "Error opening {s}", .{filename});
                defer a.free(msg);
                report(io, .stderr(), msg);
                failed = true;
                continue;
            };
            defer a.free(source);
            const normalized = try luau.require.file_utils.normalizePath(a, filename);
            defer a.free(normalized);
            const chunk = try std.fmt.allocPrintSentinel(a, "@{s}", .{normalized}, 0);
            defer a.free(chunk);
            const text = if (std.mem.startsWith(u8, source, "#!"))
                source[(std.mem.indexOfScalar(u8, source, '\n') orelse source.len)..]
            else
                source;
            var result = try runSource(&host, L, chunk, text, program_args, false);
            defer result.deinit(a);
            if (!result.ok) {
                report(io, .stderr(), result.message);
                failed = true;
            }
        }
        if (files.items.len == 0 or (interactive and index + 1 == files.items.len)) try repl(io, &host, L);
    }
    return if (failed) 1 else 0;
}
