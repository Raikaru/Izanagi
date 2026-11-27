//! Auxiliary library (upstream VM/src/laux.cpp + lualib.h), translated to Zig.
//!
//! Portions derived from Luau (MIT, Copyright (c) 2019-2025 Roblox Corporation) and
//! Lua 5.x (MIT, Copyright (C) 1994-2008 Lua.org, PUC-Rio); see third_party/luau/LICENSE.txt
//! and third_party/luau/lua_LICENSE.txt.
//!
//! Zig-facing helpers take slices and Zig formatting; the C ABI `luaL_*` exports live in
//! `src/stdlib.zig` and forward here.

const std = @import("std");
const api = @import("api.zig");
const State = api.State;

pub const Reg = extern struct {
    name: ?[*:0]const u8,
    func: ?api.CFunction,
};

/// Adapts a Zig library function to the lua_CFunction ABI.
pub fn wrap(comptime f: fn (*State) c_int) api.CFunction {
    return &struct {
        fn thunk(L: ?*State) callconv(.c) c_int {
            return f(L.?);
        }
    }.thunk;
}

/// Adapts a Zig continuation to the lua_Continuation ABI.
pub fn wrapk(comptime f: fn (*State, c_int) c_int) api.Continuation {
    return &struct {
        fn thunk(L: ?*State, status: c_int) callconv(.c) c_int {
            return f(L.?, status);
        }
    }.thunk;
}

pub const Entry = struct { name: [*:0]const u8, func: api.CFunction };

pub fn entry(comptime name: [:0]const u8, comptime f: fn (*State) c_int) Entry {
    return .{ .name = name.ptr, .func = wrap(f) };
}

// =======================================================================================
// Formatting onto the Lua stack (replacement for lua_pushfstring in Zig callers)

/// Formats with std.fmt and pushes the result as a Lua string, with lua_pushfstring's
/// observable limits: upstream formats into a LUA_BUFFERSIZE (512) byte buffer with
/// vsnprintf and interns it with strlen, so the text is cut at its first NUL and at 511
/// bytes. Arguments may reference strings owned by values on the Lua stack; when the full
/// text does not fit locally the transient storage is a Luau buffer object, so allocation
/// failures unwind without leaking.
pub fn pushfmt(L: *State, comptime fmt: []const u8, args: anytype) []const u8 {
    // lua_pushfstring grows the stack without the LUAI_MAXCSTACK check (incr_top), so error
    // messages can still be pushed by luaL_checkstack at the stack limit; the long path
    // briefly holds a box plus the message
    api.rawcheckstack(L, 2);
    var small: [api.BUFFERSIZE]u8 = undefined;
    if (std.fmt.bufPrint(&small, fmt, args)) |s| {
        api.pushlstring(L, clampfstring(s));
    } else |_| {
        const n = std.fmt.count(fmt, args);
        const box = api.newbuffer(L, n);
        const s = std.fmt.bufPrint(box, fmt, args) catch unreachable;
        api.pushlstring(L, clampfstring(s));
        api.remove(L, -2);
    }
    return api.tolstring(L, -1).?;
}

fn clampfstring(s: []const u8) []const u8 {
    const z = std.mem.indexOfScalar(u8, s, 0) orelse s.len;
    return s[0..@min(z, api.BUFFERSIZE - 1)];
}

// =======================================================================================
// Error-report functions

fn currfuncname(L: *State) ?[]const u8 {
    var ar: api.Debug = undefined;
    // level 0 exists exactly when L->ci > L->base_ci; its name is the C closure debugname
    if (!api.getinfo(L, 0, "n", &ar)) return null;
    const s = api.debugName(&ar) orelse return null;
    if (std.mem.eql(u8, s, "__namecall")) {
        const nc = api.namecall(L) orelse return null;
        return api.span(nc);
    }
    return s;
}

/// luaL_where: pushes "short_src:line: " for the function at `level`, or "".
pub fn where(L: *State, level: c_int) void {
    var ar: api.Debug = undefined;
    if (api.getinfo(L, level, "sl", &ar) and ar.currentline > 0) {
        _ = pushfmt(L, "{s}:{d}: ", .{ api.debugShortSrc(&ar), ar.currentline });
        return;
    }
    api.rawcheckstack(L, 1);
    api.pushlstring(L, "");
}

/// luaL_error with Zig formatting: prefixes the caller position and raises.
pub fn errorf(L: *State, comptime fmt: []const u8, args: anytype) noreturn {
    where(L, 1);
    _ = pushfmt(L, fmt, args);
    api.concat(L, 2);
    api.@"error"(L);
}

/// Raises the message without position information (luaG_runerror from a C frame).
pub fn runerror(L: *State, msg: []const u8) noreturn {
    api.pushlstring(L, msg);
    api.@"error"(L);
}

pub fn readonlyerror(L: *State) noreturn {
    runerror(L, "attempt to modify a readonly table");
}

pub fn argerror(L: *State, narg: c_int, extramsg: []const u8) noreturn {
    if (currfuncname(L)) |fname|
        errorf(L, "invalid argument #{d} to '{s}' ({s})", .{ narg, fname, extramsg })
    else
        errorf(L, "invalid argument #{d} ({s})", .{ narg, extramsg });
}

pub fn typeerror(L: *State, narg: c_int, tname: []const u8) noreturn {
    const fname = currfuncname(L);
    if (api.@"type"(L, narg) != api.TNONE) {
        const got = typenameOf(L, narg);
        if (fname) |f|
            errorf(L, "invalid argument #{d} to '{s}' ({s} expected, got {s})", .{ narg, f, tname, got })
        else
            errorf(L, "invalid argument #{d} ({s} expected, got {s})", .{ narg, tname, got });
    } else {
        if (fname) |f|
            errorf(L, "missing argument #{d} to '{s}' ({s} expected)", .{ narg, f, tname })
        else
            errorf(L, "missing argument #{d} ({s} expected)", .{ narg, tname });
    }
}

fn tagerror(L: *State, narg: c_int, tag: c_int) noreturn {
    typeerror(L, narg, api.span(api.typename(L, tag)));
}

pub inline fn argcheck(L: *State, cond: bool, narg: c_int, extramsg: []const u8) void {
    if (!cond) argerror(L, narg, extramsg);
}

pub inline fn argexpected(L: *State, cond: bool, narg: c_int, tname: []const u8) void {
    if (!cond) typeerror(L, narg, tname);
}

// =======================================================================================
// Argument checks

pub fn checkoption(L: *State, narg: c_int, def: ?[]const u8, lst: []const []const u8) usize {
    const name = if (def) |d| optlstring(L, narg, d) else checklstring(L, narg);
    // upstream compares with strcmp: the option name ends at its first NUL
    const cname = name[0 .. std.mem.indexOfScalar(u8, name, 0) orelse name.len];
    for (lst, 0..) |opt, i| {
        if (std.mem.eql(u8, opt, cname)) return i;
    }
    const msg = pushfmt(L, "invalid option '{s}'", .{cname});
    argerror(L, narg, msg);
}

pub fn checkstack(L: *State, space: c_int, mes: []const u8) void {
    if (!api.checkstack(L, space))
        errorf(L, "stack overflow ({s})", .{mes});
}

pub fn checktype(L: *State, narg: c_int, t: c_int) void {
    if (api.@"type"(L, narg) != t) tagerror(L, narg, t);
}

pub fn checkany(L: *State, narg: c_int) void {
    if (api.@"type"(L, narg) == api.TNONE)
        errorf(L, "missing argument #{d}", .{narg});
}

pub fn checklstring(L: *State, narg: c_int) []const u8 {
    return api.tolstring(L, narg) orelse tagerror(L, narg, api.TSTRING);
}

pub fn checklstringz(L: *State, narg: c_int) [:0]const u8 {
    return api.tolstringz(L, narg) orelse tagerror(L, narg, api.TSTRING);
}

pub fn optlstring(L: *State, narg: c_int, def: []const u8) []const u8 {
    if (api.isnoneornil(L, narg)) return def;
    return checklstring(L, narg);
}

pub fn checknumber(L: *State, narg: c_int) f64 {
    return api.tonumberx(L, narg) orelse tagerror(L, narg, api.TNUMBER);
}

/// Script-visible output boundary (ruling LZ-NaN): the sign and payload of a NaN produced by VM
/// arithmetic depend on the CPU (x86-64 yields 0xfff8..., arm64 0x7ff8...), so every NaN a library
/// writes out as bytes or text becomes the canonical quiet NaN. VM values are never rewritten, and
/// finite numbers, infinities and signed zeros pass through unchanged.
pub fn outf64(x: f64) f64 {
    return if (std.math.isNan(x)) @as(f64, @bitCast(@as(u64, 0x7ff8000000000000))) else x;
}

pub fn outf32(x: f32) f32 {
    return if (std.math.isNan(x)) @as(f32, @bitCast(@as(u32, 0x7fc00000))) else x;
}

pub fn optnumber(L: *State, narg: c_int, def: f64) f64 {
    if (api.isnoneornil(L, narg)) return def;
    return checknumber(L, narg);
}

pub fn checkboolean(L: *State, narg: c_int) bool {
    if (!api.isboolean(L, narg)) tagerror(L, narg, api.TBOOLEAN);
    return api.toboolean(L, narg);
}

pub fn optboolean(L: *State, narg: c_int, def: bool) bool {
    if (api.isnoneornil(L, narg)) return def;
    return checkboolean(L, narg);
}

pub fn checkinteger(L: *State, narg: c_int) c_int {
    return api.tointegerx(L, narg) orelse tagerror(L, narg, api.TNUMBER);
}

pub fn optinteger(L: *State, narg: c_int, def: c_int) c_int {
    if (api.isnoneornil(L, narg)) return def;
    return checkinteger(L, narg);
}

pub fn checkinteger64(L: *State, narg: c_int) i64 {
    if (!api.isinteger64(L, narg)) tagerror(L, narg, api.TINTEGER);
    return api.tointeger64(L, narg);
}

pub fn optinteger64(L: *State, narg: c_int, def: i64) i64 {
    if (api.isnoneornil(L, narg)) return def;
    return checkinteger64(L, narg);
}

pub fn checkunsigned(L: *State, narg: c_int) c_uint {
    return api.tounsignedx(L, narg) orelse tagerror(L, narg, api.TNUMBER);
}

pub fn optunsigned(L: *State, narg: c_int, def: c_uint) c_uint {
    if (api.isnoneornil(L, narg)) return def;
    return checkunsigned(L, narg);
}

pub fn checkvector(L: *State, narg: c_int) *const [api.VECTOR_SIZE]f32 {
    return api.tovector(L, narg) orelse tagerror(L, narg, api.TVECTOR);
}

pub fn checkbuffer(L: *State, narg: c_int) []u8 {
    return api.tobuffer(L, narg) orelse tagerror(L, narg, api.TBUFFER);
}

pub fn checkthread(L: *State, narg: c_int) *State {
    const co = api.tothread(L, narg);
    argexpected(L, co != null, narg, "thread");
    return co.?;
}

// =======================================================================================
// Metatables and registration

pub fn getmetafield(L: *State, obj: c_int, event: [*:0]const u8) bool {
    if (!api.getmetatable(L, obj)) return false;
    api.pushlstring(L, api.span(event));
    _ = api.rawget(L, -2);
    if (api.isnil(L, -1)) {
        api.pop(L, 2);
        return false;
    }
    api.remove(L, -2);
    return true;
}

pub fn callmeta(L: *State, obj_: c_int, event: [*:0]const u8) bool {
    const obj = absIndex(L, obj_);
    if (!getmetafield(L, obj, event)) return false;
    api.pushvalue(L, obj);
    api.call(L, 1, 1);
    return true;
}

inline fn absIndex(L: *State, i: c_int) c_int {
    return if (i > 0 or i <= api.REGISTRYINDEX) i else api.gettop(L) + i + 1;
}

pub fn newmetatable(L: *State, tname: [*:0]const u8) bool {
    _ = api.getfield(L, api.REGISTRYINDEX, tname);
    if (!api.isnil(L, -1)) return false;
    api.pop(L, 1);
    api.newtable(L);
    api.pushvalue(L, -1);
    api.setfield(L, api.REGISTRYINDEX, tname);
    return true;
}

pub fn checkudata(L: *State, ud: c_int, tname: [*:0]const u8) *anyopaque {
    if (api.touserdata(L, ud)) |p| {
        if (api.getmetatable(L, ud)) {
            _ = api.getfield(L, api.REGISTRYINDEX, tname);
            if (api.rawequal(L, -1, -2)) {
                api.pop(L, 2);
                return p;
            }
        }
    }
    typeerror(L, ud, api.span(tname));
}

/// luaL_findtable; returns the problematic part of `fname` or null on success.
pub fn findtable(L: *State, idx: c_int, fname_: []const u8, szhint: c_int) ?[]const u8 {
    var fname = fname_;
    api.pushvalue(L, idx);
    while (true) {
        const dot = std.mem.indexOfScalar(u8, fname, '.');
        const part = fname[0 .. dot orelse fname.len];
        api.pushlstring(L, part);
        _ = api.rawget(L, -2);
        if (api.isnil(L, -1)) {
            api.pop(L, 1);
            api.createtable(L, 0, if (dot != null) 1 else szhint);
            api.pushlstring(L, part);
            api.pushvalue(L, -2);
            api.settable(L, -4);
        } else if (!api.istable(L, -1)) {
            api.pop(L, 2);
            return fname;
        }
        api.remove(L, -2);
        if (dot) |d| fname = fname[d + 1 ..] else return null;
    }
}

/// luaL_register for Zig entry tables.
pub fn register(L: *State, libname: ?[*:0]const u8, funcs: []const Entry) void {
    if (libname) |name| openlib(L, name, @intCast(funcs.len));
    for (funcs) |f| {
        api.pushcfunction(L, f.func, f.name);
        api.setfield(L, -2, f.name);
    }
}

/// Shared prologue of luaL_register: leaves _LOADED[libname] (or a fresh global table) on top.
pub fn openlib(L: *State, libname: [*:0]const u8, size: c_int) void {
    _ = findtable(L, api.REGISTRYINDEX, "_LOADED", 1);
    _ = api.getfield(L, -1, libname);
    if (!api.istable(L, -1)) {
        api.pop(L, 1);
        if (findtable(L, api.GLOBALSINDEX, api.span(libname), size) != null)
            errorf(L, "name conflict for module '{s}'", .{api.span(libname)});
        api.pushvalue(L, -1);
        api.setfield(L, -3, libname);
    }
    api.remove(L, -2);
}

// =======================================================================================
// Type names and tostring

/// luaT_objtypename semantics (luaL_typename): honours string `__type` in userdata
/// metatables (except newproxy userdata), lightuserdata tag names, and global type
/// metatables for non-table values.
pub fn typenameOf(L: *State, idx: c_int) []const u8 {
    const t = api.@"type"(L, idx);
    if (t == api.TNONE) return "no value";
    api.rawcheckstack(L, 2);
    if (t == api.TUSERDATA) {
        if (api.userdatatag(L, idx) != api.UTAG_PROXY and api.getmetatable(L, idx)) {
            const r = metatypename(L);
            return r orelse api.span(api.typename(L, t));
        }
    } else if (t == api.TLIGHTUSERDATA) {
        const tag = api.lightuserdatatag(L, idx);
        if (tag >= 0 and tag < api.LUTAG_LIMIT) {
            if (api.getlightuserdataname(L, tag)) |n| return api.span(n);
        }
        if (api.getmetatable(L, idx)) {
            if (metatypename(L)) |n| return n;
        }
    } else if (t != api.TTABLE) {
        if (api.getmetatable(L, idx)) {
            if (metatypename(L)) |n| return n;
        }
    }
    return api.span(api.typename(L, t));
}

/// Expects a metatable on top; pops it and returns its string `__type` (kept alive by the
/// metatable reference itself).
fn metatypename(L: *State) ?[]const u8 {
    _ = api.rawgetfield(L, -1, "__type");
    const r: ?[]const u8 = if (api.@"type"(L, -1) == api.TSTRING) api.tolstring(L, -1).? else null;
    api.pop(L, 2);
    return r;
}

/// Appends the Luau decimal form of an int64 to `buf` (luai_int2str).
pub fn int2str(buf: *[30]u8, l: i64) []const u8 {
    const neg = l < 0;
    var val: u64 = if (neg) ~@as(u64, @bitCast(l)) +% 1 else @bitCast(l);
    var pos: usize = buf.len;
    while (true) {
        pos -= 1;
        buf[pos] = '0' + @as(u8, @intCast(val % 10));
        val /= 10;
        if (val == 0) break;
    }
    if (neg) {
        pos -= 1;
        buf[pos] = '-';
    }
    return buf[pos..];
}

pub fn num2str(buf: *[48]u8, n: f64) []const u8 {
    return api.vm.luai_num2str(buf, n);
}

/// luaL_tolstring: pushes the string form of the value at idx and returns it.
pub fn tolstring(L: *State, idx: c_int) []const u8 {
    if (callmeta(L, idx, "__tostring")) {
        return api.tolstring(L, -1) orelse errorf(L, "'__tostring' must return a string", .{});
    }
    switch (api.@"type"(L, idx)) {
        api.TNIL => api.pushlstring(L, "nil"),
        api.TBOOLEAN => api.pushlstring(L, if (api.toboolean(L, idx)) "true" else "false"),
        api.TNUMBER => {
            var b: [48]u8 = undefined;
            api.pushlstring(L, num2str(&b, api.tonumber(L, idx)));
        },
        api.TVECTOR => {
            const v = api.tovector(L, idx).?;
            var s: [48 * api.VECTOR_SIZE]u8 = undefined;
            var n: usize = 0;
            for (v, 0..) |c, i| {
                if (i != 0) {
                    s[n] = ',';
                    s[n + 1] = ' ';
                    n += 2;
                }
                var b: [48]u8 = undefined;
                const part = num2str(&b, c);
                @memcpy(s[n..][0..part.len], part);
                n += part.len;
            }
            api.pushlstring(L, s[0..n]);
        },
        api.TSTRING => api.pushvalue(L, idx),
        api.TINTEGER => {
            var b: [30]u8 = undefined;
            api.pushlstring(L, int2str(&b, api.tointeger64(L, idx)));
        },
        else => {
            const ptr = api.topointer(L, idx);
            const enc = api.encodepointer(L, @intFromPtr(ptr));
            _ = pushfmt(L, "{s}: 0x{x:0>16}", .{ typenameOf(L, idx), @as(u64, enc) });
        },
    }
    return api.tolstring(L, -1).?;
}

/// luaL_traceback
pub fn traceback(L: *State, L1: *State, msg: ?[]const u8, level: c_int) void {
    var buf: Strbuf = undefined;
    buf.init(L);
    if (msg) |m| {
        buf.addlstring(m);
        buf.addlstring("\n");
    }
    var ar: api.Debug = undefined;
    var i = level;
    while (api.getinfo(L1, i, "sln", &ar)) : (i += 1) {
        if (std.mem.eql(u8, api.debugWhat(&ar), "C")) continue;
        if (api.debugHasSource(&ar)) buf.addlstring(api.debugShortSrc(&ar));
        if (ar.currentline > 0) {
            var line: [32]u8 = undefined;
            const s = std.fmt.bufPrint(&line, ":{d}", .{@as(c_uint, @intCast(ar.currentline))}) catch unreachable;
            buf.addlstring(s);
        }
        if (api.debugName(&ar)) |n| {
            buf.addlstring(" function ");
            buf.addlstring(n);
        }
        buf.addchar('\n');
    }
    buf.pushresult();
}

// =======================================================================================
// String buffer (luaL_Strbuf). Same layout as the C struct. As upstream, when the inline
// storage is exhausted the contents move into a mutable Luau string (luaS_bufstart) kept
// on the stack ("box"), so the GC owns the memory, an unwinding error releases it, and the
// final string is produced in place (luaS_buffinish) without an extra copy when full.

pub const Strbuf = extern struct {
    p: [*]u8,
    end: [*]u8,
    L: *State,
    storage: ?*anyopaque,
    buffer: [api.BUFFERSIZE]u8,

    pub fn init(B: *Strbuf, L: *State) void {
        B.p = &B.buffer;
        B.end = B.p + api.BUFFERSIZE;
        B.L = L;
        B.storage = null;
    }

    pub fn initsize(B: *Strbuf, L: *State, size: usize) [*]u8 {
        B.init(L);
        return B.prepsize(size);
    }

    fn base(B: *Strbuf) [*]u8 {
        return if (B.storage) |s| @ptrCast(s) else &B.buffer;
    }

    fn room(B: *const Strbuf) usize {
        return @intFromPtr(B.end) - @intFromPtr(B.p);
    }

    pub fn len(B: *Strbuf) usize {
        return @intFromPtr(B.p) - @intFromPtr(B.base());
    }

    fn extend(B: *Strbuf, additional: usize, boxloc: c_int) [*]u8 {
        const L = B.L;
        const old = B.base();
        const capacity = @intFromPtr(B.end) - @intFromPtr(old);
        const used = B.len();
        // getnextbuffersize
        if (std.math.maxInt(usize) - additional < capacity) errorf(L, "buffer too large", .{});
        const desired = capacity + additional;
        if (std.math.maxInt(usize) - desired < capacity) errorf(L, "buffer too large", .{});
        var nextsize = capacity + capacity / 2;
        if (nextsize < desired) nextsize = desired;

        const nb = api.strbufstart(L, nextsize); // raises "memory allocation error: block too big" above MAXSSIZE
        @memcpy(nb[0..used], old[0..used]);
        if (B.storage == null) {
            api.insert(L, boxloc);
        } else {
            api.replace(L, boxloc - 1);
        }
        B.storage = nb;
        B.p = nb + used;
        B.end = nb + nextsize;
        return B.p;
    }

    pub fn prepsize(B: *Strbuf, size: usize) [*]u8 {
        if (B.room() < size) return B.extend(size - B.room(), -1);
        return B.p;
    }

    pub fn addchar(B: *Strbuf, c: u8) void {
        if (B.room() < 1) _ = B.extend(1, -1);
        B.p[0] = c;
        B.p += 1;
    }

    pub fn addlstring(B: *Strbuf, s: []const u8) void {
        if (B.room() < s.len) _ = B.extend(s.len - B.room(), -1);
        @memcpy(B.p[0..s.len], s);
        B.p += s.len;
    }

    /// Adds the string/number on top of the stack and pops it (box, if any, is below it).
    pub fn addvalue(B: *Strbuf) void {
        const L = B.L;
        if (api.tolstring(L, -1)) |s| {
            if (B.room() < s.len) _ = B.extend(s.len - B.room(), -2);
            @memcpy(B.p[0..s.len], s);
            B.p += s.len;
            api.pop(L, 1);
        }
    }

    pub fn addvalueany(B: *Strbuf, idx: c_int) void {
        const L = B.L;
        switch (api.@"type"(L, idx)) {
            api.TNONE => unreachable,
            api.TNIL => B.addlstring("nil"),
            api.TBOOLEAN => B.addlstring(if (api.toboolean(L, idx)) "true" else "false"),
            api.TNUMBER => {
                var s: [48]u8 = undefined;
                B.addlstring(num2str(&s, api.tonumber(L, idx)));
            },
            api.TSTRING => B.addlstring(api.tolstring(L, idx).?),
            api.TINTEGER => {
                var s: [30]u8 = undefined;
                B.addlstring(int2str(&s, api.tointeger64(L, idx)));
            },
            else => {
                _ = tolstring(L, idx);
                B.addvalue();
            },
        }
    }

    pub fn pushresult(B: *Strbuf) void {
        const L = B.L;
        if (B.storage != null) {
            // the box is at the top: turn it into the final string in its own slot
            api.strbuffinish(L, -1, B.len());
        } else {
            api.pushlstring(L, B.buffer[0..B.len()]);
        }
    }

    pub fn pushresultsize(B: *Strbuf, size: usize) void {
        B.p += size;
        B.pushresult();
    }
};
