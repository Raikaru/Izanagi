//! Luau standard libraries for the Zig Luau engine.
//!
//! Translation of the vendored upstream Luau 0.744 libraries (third_party/luau/VM/src:
//! laux.cpp, linit.cpp, lbaselib.cpp, lcorolib.cpp, ltablib.cpp, lstrlib.cpp,
//! lmathlib.cpp, lutf8lib.cpp, lbitlib.cpp, lbuflib.cpp, loslib.cpp, ldblib.cpp,
//! lveclib.cpp, lintlib.cpp, lclasslib.cpp) onto the VM's public `lua.h` interface.
//! Portions derived from Luau (MIT, Copyright (c) 2019-2025 Roblox Corporation) and
//! Lua 5.x (MIT, Copyright (C) 1994-2008 Lua.org, PUC-Rio); see
//! third_party/luau/LICENSE.txt and third_party/luau/lua_LICENSE.txt.
//!
//! This file exports the upstream `lualib.h` C ABI (luaL_* auxiliary functions, every
//! luaopen_* entry point, luaL_openlibs, luaL_sandbox, luaL_sandboxthread, luaL_newstate).
//!
//! Two library sets:
//! - `luaL_openlibs`: the upstream linit.cpp set (base, coroutine, table, os, string, math,
//!   debug, utf8, bit32, buffer, vector, plus integer under LuauIntegerLibrary and class
//!   under DebugLuauUserDefinedClassesRuntime).
//! - `p4pc_openlibs_sandbox`: the game's restricted mod sandbox (base, coroutine, table,
//!   string, math, utf8, bit32, buffer). It never opens os, debug, vector, integer or class,
//!   whatever the flags say.
//! Fast flags: library-owned ones live in stdlib/flags.zig (C: luau_stdlib_setflag);
//! VM-owned ones in vm.fflags. All default to the vendored defaults (false).

const std = @import("std");
const api = @import("stdlib/api.zig");
const aux = @import("stdlib/auxiliary.zig");

pub const flags = @import("stdlib/flags.zig");
pub const base = @import("stdlib/base.zig");
pub const coroutine = @import("stdlib/coroutine.zig");
pub const table = @import("stdlib/table.zig");
pub const os = @import("stdlib/os.zig");
pub const string = @import("stdlib/string.zig");
pub const math = @import("stdlib/math.zig");
pub const debug = @import("stdlib/debug.zig");
pub const utf8 = @import("stdlib/utf8.zig");
pub const bit32 = @import("stdlib/bit32.zig");
pub const buffer = @import("stdlib/buffer.zig");
pub const vector = @import("stdlib/vector.zig");
pub const integer = @import("stdlib/integer.zig");
pub const class = @import("stdlib/class.zig");

pub const Strbuf = aux.Strbuf;
pub const Reg = aux.Reg;

const State = api.State;

// =======================================================================================
// Library entry points (lualib.h)

pub const LUA_COLIBNAME = "coroutine";
pub const LUA_TABLIBNAME = "table";
pub const LUA_OSLIBNAME = "os";
pub const LUA_STRLIBNAME = "string";
pub const LUA_BITLIBNAME = "bit32";
pub const LUA_BUFFERLIBNAME = "buffer";
pub const LUA_UTF8LIBNAME = "utf8";
pub const LUA_CLASSLIBNAME = "class";
pub const LUA_MATHLIBNAME = "math";
pub const LUA_DBLIBNAME = "debug";
pub const LUA_VECLIBNAME = "vector";
pub const LUA_INTLIBNAME = "integer";

pub export fn luaopen_base(L: ?*State) c_int {
    return base.open(L.?);
}
pub export fn luaopen_coroutine(L: ?*State) c_int {
    return coroutine.open(L.?);
}
pub export fn luaopen_table(L: ?*State) c_int {
    return table.open(L.?);
}
pub export fn luaopen_os(L: ?*State) c_int {
    return os.open(L.?);
}
pub export fn luaopen_string(L: ?*State) c_int {
    return string.open(L.?);
}
pub export fn luaopen_math(L: ?*State) c_int {
    return math.open(L.?);
}
pub export fn luaopen_debug(L: ?*State) c_int {
    return debug.open(L.?);
}
pub export fn luaopen_utf8(L: ?*State) c_int {
    return utf8.open(L.?);
}
pub export fn luaopen_bit32(L: ?*State) c_int {
    return bit32.open(L.?);
}
pub export fn luaopen_buffer(L: ?*State) c_int {
    return buffer.open(L.?);
}
pub export fn luaopen_vector(L: ?*State) c_int {
    return vector.open(L.?);
}
pub export fn luaopen_integer(L: ?*State) c_int {
    return integer.open(L.?);
}
pub export fn luaopen_class(L: ?*State) c_int {
    return class.open(L.?);
}

const Lib = struct { name: [*:0]const u8, func: api.CFunction };

fn openlist(L: *State, libs: []const Lib) void {
    for (libs) |lib| {
        api.pushcfunction(L, lib.func, null);
        api.pushlstring(L, api.span(lib.name));
        api.call(L, 1, 0);
    }
}

// linit.cpp lualibs_NOINTEGER / lualibs
const lualibs_NOINTEGER = [_]Lib{
    .{ .name = "", .func = &luaopen_base },
    .{ .name = LUA_COLIBNAME, .func = &luaopen_coroutine },
    .{ .name = LUA_TABLIBNAME, .func = &luaopen_table },
    .{ .name = LUA_OSLIBNAME, .func = &luaopen_os },
    .{ .name = LUA_STRLIBNAME, .func = &luaopen_string },
    .{ .name = LUA_MATHLIBNAME, .func = &luaopen_math },
    .{ .name = LUA_DBLIBNAME, .func = &luaopen_debug },
    .{ .name = LUA_UTF8LIBNAME, .func = &luaopen_utf8 },
    .{ .name = LUA_BITLIBNAME, .func = &luaopen_bit32 },
    .{ .name = LUA_BUFFERLIBNAME, .func = &luaopen_buffer },
    .{ .name = LUA_VECLIBNAME, .func = &luaopen_vector },
};
const lualibs = lualibs_NOINTEGER ++ [_]Lib{
    .{ .name = LUA_INTLIBNAME, .func = &luaopen_integer },
};

/// Upstream luaL_openlibs (linit.cpp), including the fast-flag dependent libraries.
pub export fn luaL_openlibs(L_: ?*State) void {
    const L = L_.?;
    if (flags.LuauIntegerLibrary) {
        openlist(L, &lualibs);
    } else {
        openlist(L, &lualibs_NOINTEGER);
    }
    if (api.vmflag("DebugLuauUserDefinedClassesRuntime")) {
        openlist(L, &[_]Lib{.{ .name = LUA_CLASSLIBNAME, .func = &luaopen_class }});
    }
}

// The game's mod sandbox (src/platform/mods/luau_c.cpp): no os/debug/vector/integer/class.
const p4pc_sandbox_libs = [_]Lib{
    .{ .name = "", .func = &luaopen_base },
    .{ .name = LUA_COLIBNAME, .func = &luaopen_coroutine },
    .{ .name = LUA_TABLIBNAME, .func = &luaopen_table },
    .{ .name = LUA_STRLIBNAME, .func = &luaopen_string },
    .{ .name = LUA_MATHLIBNAME, .func = &luaopen_math },
    .{ .name = LUA_UTF8LIBNAME, .func = &luaopen_utf8 },
    .{ .name = LUA_BITLIBNAME, .func = &luaopen_bit32 },
    .{ .name = LUA_BUFFERLIBNAME, .func = &luaopen_buffer },
};

/// Opens only the libraries the p4pc mod sandbox exposes (same order as the production
/// host). Never opens os/debug/vector/integer/class regardless of fast flags; the host
/// then applies luaL_sandbox + luaL_sandboxthread as before.
pub export fn p4pc_openlibs_sandbox(L_: ?*State) void {
    openlist(L_.?, &p4pc_sandbox_libs);
}

/// Sets a library-owned fast flag by upstream name; returns 0 for unknown names.
pub export fn luau_stdlib_setflag(name: [*:0]const u8, value: c_int) c_int {
    return @intFromBool(flags.set(api.span(name), value != 0));
}

/// Sets all library tables, the string (and, under LuauSandboxFreezesVectorMetatable, the
/// vector) metatable and the globals table read-only and activates safeenv (linit.cpp).
pub export fn luaL_sandbox(L_: ?*State) void {
    const L = L_.?;
    // set all libraries to read-only
    api.pushnil(L);
    while (api.next(L, api.GLOBALSINDEX)) {
        if (api.istable(L, -1)) api.setreadonly(L, -1, true);
        api.pop(L, 1);
    }

    // set all builtin metatables to read-only
    api.pushlstring(L, "");
    if (api.getmetatable(L, -1)) {
        api.setreadonly(L, -1, true);
        api.pop(L, 2);
    } else {
        api.pop(L, 1);
    }

    if (flags.LuauSandboxFreezesVectorMetatable) {
        api.pushvector(L, 0.0, 0.0, 0.0);
        if (api.getmetatable(L, -1)) {
            api.setreadonly(L, -1, true);
            api.pop(L, 2);
        } else {
            api.pop(L, 1);
        }
    }

    // set globals to readonly and activate safeenv since the env is immutable
    api.setreadonly(L, api.GLOBALSINDEX, true);
    api.setsafeenv(L, api.GLOBALSINDEX, true);
}

/// Gives the thread a fresh global table that proxies reads to the original one.
pub export fn luaL_sandboxthread(L_: ?*State) void {
    const L = L_.?;
    // create new global table that proxies reads to original table
    api.newtable(L);

    api.newtable(L);
    api.pushvalue(L, api.GLOBALSINDEX);
    api.setfield(L, -2, "__index");
    api.setreadonly(L, -1, true);

    api.setmetatable(L, -2);

    // we can set safeenv now although it's important to set it to false if code is loaded twice into the thread
    api.replace(L, api.GLOBALSINDEX);
    api.setsafeenv(L, api.GLOBALSINDEX, true);
}

/// Default allocator for luaL_newstate (upstream: realloc/free). 16-byte alignment matches
/// the strictest VM object alignment.
fn l_alloc(ud: ?*anyopaque, ptr: ?*anyopaque, osize: usize, nsize: usize) callconv(.c) ?*anyopaque {
    _ = ud;
    const a = std.heap.smp_allocator;
    const al: std.mem.Alignment = .@"16";
    const ra = @returnAddress();
    if (nsize == 0) {
        if (ptr) |p| a.rawFree(@as([*]u8, @ptrCast(p))[0..osize], al, ra);
        return null;
    }
    // a fresh allocation passes a type tag in osize
    const p = ptr orelse return a.rawAlloc(nsize, al, ra);
    const old = @as([*]u8, @ptrCast(p))[0..osize];
    if (a.rawRemap(old, al, nsize, ra)) |np| return np;
    const np = a.rawAlloc(nsize, al, ra) orelse return null;
    const keep = @min(osize, nsize);
    @memcpy(np[0..keep], old[0..keep]);
    a.rawFree(old, al, ra);
    return np;
}

pub export fn luaL_newstate() ?*State {
    return api.newstate(&l_alloc, null);
}

// =======================================================================================
// Auxiliary library C ABI (lualib.h)

fn regs(l: [*]const Reg) usize {
    var n: usize = 0;
    while (l[n].name != null) n += 1;
    return n;
}

pub export fn luaL_register(L_: ?*State, libname: ?[*:0]const u8, l: [*]const Reg) void {
    const L = L_.?;
    const n = regs(l);
    if (libname) |name| aux.openlib(L, name, @intCast(n));
    for (l[0..n]) |r| {
        api.pushcfunction(L, r.func.?, r.name);
        api.setfield(L, -2, r.name.?);
    }
}

pub export fn luaL_getmetafield(L: ?*State, obj: c_int, e: [*:0]const u8) c_int {
    return @intFromBool(aux.getmetafield(L.?, obj, e));
}

pub export fn luaL_callmeta(L: ?*State, obj: c_int, e: [*:0]const u8) c_int {
    return @intFromBool(aux.callmeta(L.?, obj, e));
}

pub export fn luaL_typeerrorL(L: ?*State, narg: c_int, tname: [*:0]const u8) noreturn {
    aux.typeerror(L.?, narg, api.span(tname));
}

pub export fn luaL_argerrorL(L: ?*State, narg: c_int, extramsg: [*:0]const u8) noreturn {
    aux.argerror(L.?, narg, api.span(extramsg));
}

pub export fn luaL_checklstring(L: ?*State, narg: c_int, len: ?*usize) [*:0]const u8 {
    const s = aux.checklstringz(L.?, narg);
    if (len) |l| l.* = s.len;
    return s.ptr;
}

pub export fn luaL_optlstring(L: ?*State, narg: c_int, def: ?[*:0]const u8, len: ?*usize) ?[*:0]const u8 {
    if (api.isnoneornil(L.?, narg)) {
        if (len) |l| l.* = if (def) |d| api.span(d).len else 0;
        return def;
    }
    return luaL_checklstring(L, narg, len);
}

pub export fn luaL_checknumber(L: ?*State, narg: c_int) f64 {
    return aux.checknumber(L.?, narg);
}

pub export fn luaL_optnumber(L: ?*State, narg: c_int, def: f64) f64 {
    return aux.optnumber(L.?, narg, def);
}

pub export fn luaL_checkboolean(L: ?*State, narg: c_int) c_int {
    return @intFromBool(aux.checkboolean(L.?, narg));
}

pub export fn luaL_optboolean(L: ?*State, narg: c_int, def: c_int) c_int {
    if (api.isnoneornil(L.?, narg)) return def;
    return @intFromBool(aux.checkboolean(L.?, narg));
}

pub export fn luaL_checkinteger(L: ?*State, narg: c_int) c_int {
    return aux.checkinteger(L.?, narg);
}

pub export fn luaL_checkinteger64(L: ?*State, narg: c_int) i64 {
    return aux.checkinteger64(L.?, narg);
}

pub export fn luaL_optinteger(L: ?*State, narg: c_int, def: c_int) c_int {
    return aux.optinteger(L.?, narg, def);
}

pub export fn luaL_optinteger64(L: ?*State, narg: c_int, def: i64) i64 {
    return aux.optinteger64(L.?, narg, def);
}

pub export fn luaL_checkunsigned(L: ?*State, narg: c_int) c_uint {
    return aux.checkunsigned(L.?, narg);
}

pub export fn luaL_optunsigned(L: ?*State, narg: c_int, def: c_uint) c_uint {
    return aux.optunsigned(L.?, narg, def);
}

pub export fn luaL_checkvector(L: ?*State, narg: c_int) *const f32 {
    return &aux.checkvector(L.?, narg)[0];
}

pub export fn luaL_optvector(L: ?*State, narg: c_int, def: ?*const f32) ?*const f32 {
    if (api.isnoneornil(L.?, narg)) return def;
    return luaL_checkvector(L, narg);
}

pub export fn luaL_checkstack(L: ?*State, sz: c_int, msg: [*:0]const u8) void {
    aux.checkstack(L.?, sz, api.span(msg));
}

pub export fn luaL_checktype(L: ?*State, narg: c_int, t: c_int) void {
    aux.checktype(L.?, narg, t);
}

pub export fn luaL_checkany(L: ?*State, narg: c_int) void {
    aux.checkany(L.?, narg);
}

pub export fn luaL_newmetatable(L: ?*State, tname: [*:0]const u8) c_int {
    return @intFromBool(aux.newmetatable(L.?, tname));
}

pub export fn luaL_checkudata(L: ?*State, ud: c_int, tname: [*:0]const u8) ?*anyopaque {
    return aux.checkudata(L.?, ud, tname);
}

pub export fn luaL_checkudatatagged(L_: ?*State, ud: c_int, tag: c_int) ?*anyopaque {
    const L = L_.?;
    if (api.touserdatatagged(L, ud, tag)) |p| return p;
    const tname = api.getuserdataname(L, tag);
    aux.typeerror(L, ud, if (tname) |t| api.span(t) else "(null)");
}

pub export fn luaL_checkbuffer(L: ?*State, narg: c_int, len: ?*usize) ?*anyopaque {
    const b = aux.checkbuffer(L.?, narg);
    if (len) |l| l.* = b.len;
    return b.ptr;
}

pub export fn luaL_where(L: ?*State, lvl: c_int) void {
    aux.where(L.?, lvl);
}

/// luaL_errorL(L, fmt, ...): the C-variadic symbol is emitted by the VM's per-target
/// thunk (Zig's VaList is disabled on win64/aarch64 in the pinned nightly), which forwards
/// the raw va_list here.
fn luaL_errorL_impl(L_: ?*State, fmt: [*:0]const u8, ap: api.RawVaList) callconv(.c) c_int {
    const L = L_.?;
    aux.where(L, 1);
    _ = api.pushvfstring(L, fmt, ap);
    api.concat(L, 2);
    api.@"error"(L);
}

comptime {
    api.vm.exportVariadic2("luaL_errorL", luaL_errorL_impl);
}

pub export fn luaL_checkoption(L_: ?*State, narg: c_int, def: ?[*:0]const u8, lst: [*]const ?[*:0]const u8) c_int {
    const L = L_.?;
    const name = if (def) |d| aux.optlstring(L, narg, api.span(d)) else aux.checklstring(L, narg);
    const cname = name[0 .. std.mem.indexOfScalar(u8, name, 0) orelse name.len];
    var i: usize = 0;
    while (lst[i]) |opt| : (i += 1) {
        if (std.mem.eql(u8, api.span(opt), cname)) return @intCast(i);
    }
    const msg = aux.pushfmt(L, "invalid option '{s}'", .{cname});
    aux.argerror(L, narg, msg);
}

pub export fn luaL_tolstring(L_: ?*State, idx: c_int, len: ?*usize) [*:0]const u8 {
    const L = L_.?;
    _ = aux.tolstring(L, idx);
    const s = api.tolstringz(L, -1).?;
    if (len) |l| l.* = s.len;
    return s.ptr;
}

pub export fn luaL_findtable(L: ?*State, idx: c_int, fname: [*:0]const u8, szhint: c_int) ?[*:0]const u8 {
    const rest = aux.findtable(L.?, idx, api.span(fname), szhint) orelse return null;
    // `rest` is a suffix of the NUL-terminated fname
    return @ptrCast(rest.ptr);
}

pub export fn luaL_typename(L: ?*State, idx: c_int) [*:0]const u8 {
    // every source of type names (type name table, interned `__type` strings, literals)
    // is NUL-terminated
    return @ptrCast(aux.typenameOf(L.?, idx).ptr);
}

pub export fn luaL_traceback(L: ?*State, L1: ?*State, msg: ?[*:0]const u8, level: c_int) void {
    aux.traceback(L.?, L1.?, if (msg) |m| api.span(m) else null, level);
}

// Generic buffer manipulation

pub export fn luaL_buffinit(L: ?*State, B: *Strbuf) void {
    B.init(L.?);
}

pub export fn luaL_buffinitsize(L: ?*State, B: *Strbuf, size: usize) [*]u8 {
    return B.initsize(L.?, size);
}

pub export fn luaL_prepbuffsize(B: *Strbuf, size: usize) [*]u8 {
    return B.prepsize(size);
}

pub export fn luaL_addlstring(B: *Strbuf, s: [*]const u8, l: usize) void {
    B.addlstring(s[0..l]);
}

pub export fn luaL_addvalue(B: *Strbuf) void {
    B.addvalue();
}

pub export fn luaL_addvalueany(B: *Strbuf, idx: c_int) void {
    B.addvalueany(idx);
}

pub export fn luaL_pushresult(B: *Strbuf) void {
    B.pushresult();
}

pub export fn luaL_pushresultsize(B: *Strbuf, size: usize) void {
    B.pushresultsize(size);
}
