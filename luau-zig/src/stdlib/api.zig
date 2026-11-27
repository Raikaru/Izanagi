//! Single adapter between the standard libraries and the Luau-Zig VM public interface.
//!
//! Every VM access made by `src/stdlib/**` goes through this file. The VM (`src/vm.zig`)
//! re-exports the upstream `lua.h` surface verbatim (C ABI, `lua_*` names), and raises
//! errors with a Lua-style non-local unwind: `lua_error` never returns, and every API call
//! that may run metamethods or allocate may unwind through the caller. Library code must
//! therefore never hold Zig-owned resources (heap memory, `defer` cleanups) across such
//! calls; transient storage lives on the Luau stack (see `aux.Strbuf`).
//!
//! Wrappers here translate C conventions (out-parameters, NUL-terminated strings, `int`
//! booleans) into Zig ones; they add no semantics of their own.

const std = @import("std");
pub const vm = @import("../vm.zig");

pub const State = vm.lua_State;
pub const CFunction = vm.lua_CFunction;
pub const Continuation = vm.lua_Continuation;
pub const Debug = vm.lua_Debug;
pub const Alloc = vm.lua_Alloc;

// ---------------------------------------------------------------------------------------
// Constants (mirrors lua.h / luaconf.h of the vendored Luau; these are ABI-level values)

pub const MULTRET: c_int = -1;
pub const MAXCSTACK: c_int = 8000; // LUAI_MAXCSTACK
pub const MAXCCALLS: c_int = 200; // LUAI_MAXCCALLS
pub const MINSTACK: c_int = 20; // LUA_MINSTACK
pub const IDSIZE = 256; // LUA_IDSIZE
pub const BUFFERSIZE = 512; // LUA_BUFFERSIZE
pub const MAXCAPTURES = 32; // LUA_MAXCAPTURES
pub const UTAG_LIMIT: c_int = 128; // LUA_UTAG_LIMIT
pub const LUTAG_LIMIT: c_int = 128; // LUA_LUTAG_LIMIT
pub const UTAG_PROXY: c_int = UTAG_LIMIT + 1; // ludata.h
pub const VECTOR_SIZE = 3; // LUA_VECTOR_SIZE
pub const MAXSSIZE: usize = 1 << 30; // lstring.h
pub const MAX_BUFFER_SIZE: usize = 1 << 30; // lbuffer.h

pub const REGISTRYINDEX: c_int = -MAXCSTACK - 2000;
pub const ENVIRONINDEX: c_int = -MAXCSTACK - 2001;
pub const GLOBALSINDEX: c_int = -MAXCSTACK - 2002;

pub inline fn upvalueindex(i: c_int) c_int {
    return GLOBALSINDEX - i;
}

// lua_Status
pub const OK: c_int = 0;
pub const YIELD: c_int = 1;
pub const ERRRUN: c_int = 2;
pub const ERRSYNTAX: c_int = 3;
pub const ERRMEM: c_int = 4;
pub const ERRERR: c_int = 5;
pub const BREAK: c_int = 6;

// lua_CoStatus
pub const CORUN: c_int = 0;
pub const COSUS: c_int = 1;
pub const CONOR: c_int = 2;
pub const COFIN: c_int = 3;
pub const COERR: c_int = 4;

// lua_Type (ORDER TYPE; LUA_VECTOR_DOUBLE == 0)
pub const TNONE: c_int = -1;
pub const TNIL: c_int = 0;
pub const TBOOLEAN: c_int = 1;
pub const TLIGHTUSERDATA: c_int = 2;
pub const TNUMBER: c_int = 3;
pub const TINTEGER: c_int = 4;
pub const TVECTOR: c_int = 5;
pub const TSTRING: c_int = 6;
pub const TTABLE: c_int = 7;
pub const TFUNCTION: c_int = 8;
pub const TUSERDATA: c_int = 9;
pub const TTHREAD: c_int = 10;
pub const TBUFFER: c_int = 11;
pub const TCLASS: c_int = 12;
pub const TOBJECT: c_int = 13;

// lua_GCOp
pub const GCSTOP: c_int = 0;
pub const GCRESTART: c_int = 1;
pub const GCCOLLECT: c_int = 2;
pub const GCCOUNT: c_int = 3;
pub const GCCOUNTB: c_int = 4;
pub const GCISRUNNING: c_int = 5;
pub const GCSTEP: c_int = 6;

pub const MEMERRMSG = "not enough memory";
pub const ERRERRMSG = "error in error handling";

// ---------------------------------------------------------------------------------------
// Pointer helpers

/// Normalizes a VM C-string result (optional, C or non-optional pointer) to an optional.
inline fn cstr(p: anytype) ?[*:0]const u8 {
    const info = @typeInfo(@TypeOf(p));
    if (info == .optional) {
        const v = p orelse return null;
        return @as([*:0]const u8, @ptrCast(v));
    }
    if (info == .pointer and info.pointer.size == .c) {
        if (p == null) return null;
    }
    return @as([*:0]const u8, @ptrCast(p));
}

pub inline fn span(p: [*:0]const u8) []const u8 {
    return std.mem.span(p);
}

// ---------------------------------------------------------------------------------------
// Stack manipulation

pub inline fn absindex(L: *State, idx: c_int) c_int {
    return vm.lua_absindex(L, idx);
}
pub inline fn gettop(L: *State) c_int {
    return vm.lua_gettop(L);
}
pub inline fn settop(L: *State, idx: c_int) void {
    vm.lua_settop(L, idx);
}
pub inline fn pop(L: *State, n: c_int) void {
    vm.lua_settop(L, -n - 1);
}
pub inline fn pushvalue(L: *State, idx: c_int) void {
    vm.lua_pushvalue(L, idx);
}
pub inline fn remove(L: *State, idx: c_int) void {
    vm.lua_remove(L, idx);
}
pub inline fn insert(L: *State, idx: c_int) void {
    vm.lua_insert(L, idx);
}
pub inline fn replace(L: *State, idx: c_int) void {
    vm.lua_replace(L, idx);
}
pub inline fn checkstack(L: *State, sz: c_int) bool {
    return vm.lua_checkstack(L, sz) != 0;
}
pub inline fn rawcheckstack(L: *State, sz: c_int) void {
    vm.lua_rawcheckstack(L, sz);
}
pub inline fn xmove(from: *State, to: *State, n: c_int) void {
    vm.lua_xmove(from, to, n);
}
pub inline fn xpush(from: *State, to: *State, idx: c_int) void {
    vm.lua_xpush(from, to, idx);
}

// ---------------------------------------------------------------------------------------
// Access functions

pub inline fn isnumber(L: *State, idx: c_int) bool {
    return vm.lua_isnumber(L, idx) != 0;
}
pub inline fn isstring(L: *State, idx: c_int) bool {
    return vm.lua_isstring(L, idx) != 0;
}
pub inline fn iscfunction(L: *State, idx: c_int) bool {
    return vm.lua_iscfunction(L, idx) != 0;
}
pub inline fn isinteger64(L: *State, idx: c_int) bool {
    return vm.lua_type(L, idx) == TINTEGER;
}
pub inline fn @"type"(L: *State, idx: c_int) c_int {
    return vm.lua_type(L, idx);
}
pub inline fn typename(L: *State, tp: c_int) [*:0]const u8 {
    return cstr(vm.lua_typename(L, tp)).?;
}
pub inline fn isnil(L: *State, idx: c_int) bool {
    return vm.lua_type(L, idx) == TNIL;
}
pub inline fn isnoneornil(L: *State, idx: c_int) bool {
    return vm.lua_type(L, idx) <= TNIL;
}
pub inline fn isnone(L: *State, idx: c_int) bool {
    return vm.lua_type(L, idx) == TNONE;
}
pub inline fn istable(L: *State, idx: c_int) bool {
    return vm.lua_type(L, idx) == TTABLE;
}
pub inline fn isfunction(L: *State, idx: c_int) bool {
    return vm.lua_type(L, idx) == TFUNCTION;
}
pub inline fn isboolean(L: *State, idx: c_int) bool {
    return vm.lua_type(L, idx) == TBOOLEAN;
}
pub inline fn equal(L: *State, a: c_int, b: c_int) bool {
    return vm.lua_equal(L, a, b) != 0;
}
pub inline fn rawequal(L: *State, a: c_int, b: c_int) bool {
    return vm.lua_rawequal(L, a, b) != 0;
}
pub inline fn lessthan(L: *State, a: c_int, b: c_int) bool {
    return vm.lua_lessthan(L, a, b) != 0;
}

/// lua_tonumberx: null when the value is not a number (or a string convertible to one).
pub inline fn tonumberx(L: *State, idx: c_int) ?f64 {
    var isnum: c_int = 0;
    const d = vm.lua_tonumberx(L, idx, &isnum);
    return if (isnum != 0) d else null;
}
pub inline fn tonumber(L: *State, idx: c_int) f64 {
    return vm.lua_tonumberx(L, idx, null);
}
pub inline fn tointegerx(L: *State, idx: c_int) ?c_int {
    var isnum: c_int = 0;
    const d = vm.lua_tointegerx(L, idx, &isnum);
    return if (isnum != 0) d else null;
}
pub inline fn tointeger(L: *State, idx: c_int) c_int {
    return vm.lua_tointegerx(L, idx, null);
}
pub inline fn tounsignedx(L: *State, idx: c_int) ?c_uint {
    var isnum: c_int = 0;
    const d = vm.lua_tounsignedx(L, idx, &isnum);
    return if (isnum != 0) d else null;
}
pub inline fn tointeger64(L: *State, idx: c_int) i64 {
    return vm.lua_tointeger64(L, idx, null);
}
pub inline fn tovector(L: *State, idx: c_int) ?*const [VECTOR_SIZE]f32 {
    const p = vm.lua_tovector(L, idx);
    if (p == null) return null;
    return @ptrCast(p.?);
}
pub inline fn toboolean(L: *State, idx: c_int) bool {
    return vm.lua_toboolean(L, idx) != 0;
}

/// lua_tolstring: converts numbers in place (like upstream); null for other non-strings.
pub inline fn tolstring(L: *State, idx: c_int) ?[]const u8 {
    var len: usize = 0;
    const p = cstr(vm.lua_tolstring(L, idx, &len)) orelse return null;
    return p[0..len];
}
/// Same as `tolstring` but also returns the NUL-terminated pointer.
pub inline fn tolstringz(L: *State, idx: c_int) ?[:0]const u8 {
    var len: usize = 0;
    const p = cstr(vm.lua_tolstring(L, idx, &len)) orelse return null;
    return p[0..len :0];
}
pub inline fn namecall(L: *State) ?[*:0]const u8 {
    return cstr(vm.lua_namecallatom(L, null));
}
pub inline fn touserdatatagged(L: *State, idx: c_int, tag: c_int) ?*anyopaque {
    return vm.lua_touserdatatagged(L, idx, tag);
}
pub inline fn getuserdataname(L: *State, tag: c_int) ?[*:0]const u8 {
    return cstr(vm.lua_getuserdataname(L, tag));
}
pub inline fn objlen(L: *State, idx: c_int) c_int {
    return vm.lua_objlen(L, idx);
}
pub inline fn touserdata(L: *State, idx: c_int) ?*anyopaque {
    return vm.lua_touserdata(L, idx);
}
pub inline fn userdatatag(L: *State, idx: c_int) c_int {
    return vm.lua_userdatatag(L, idx);
}
pub inline fn lightuserdatatag(L: *State, idx: c_int) c_int {
    return vm.lua_lightuserdatatag(L, idx);
}
pub inline fn getlightuserdataname(L: *State, tag: c_int) ?[*:0]const u8 {
    return cstr(vm.lua_getlightuserdataname(L, tag));
}
pub inline fn tothread(L: *State, idx: c_int) ?*State {
    return vm.lua_tothread(L, idx);
}
/// lua_tobuffer: slice over the buffer bytes, null when the value is not a buffer.
pub inline fn tobuffer(L: *State, idx: c_int) ?[]u8 {
    var len: usize = 0;
    const p = vm.lua_tobuffer(L, idx, &len) orelse return null;
    return @as([*]u8, @ptrCast(p))[0..len];
}
pub inline fn topointer(L: *State, idx: c_int) ?*const anyopaque {
    return vm.lua_topointer(L, idx);
}

// ---------------------------------------------------------------------------------------
// Push functions

pub inline fn pushnil(L: *State) void {
    vm.lua_pushnil(L);
}
pub inline fn pushnumber(L: *State, n: f64) void {
    vm.lua_pushnumber(L, n);
}
pub inline fn pushinteger(L: *State, n: c_int) void {
    vm.lua_pushinteger(L, n);
}
pub inline fn pushunsigned(L: *State, n: c_uint) void {
    vm.lua_pushunsigned(L, n);
}
pub inline fn pushlstring(L: *State, s: []const u8) void {
    vm.lua_pushlstring(L, s.ptr, s.len);
}
pub inline fn pushboolean(L: *State, b: bool) void {
    vm.lua_pushboolean(L, @intFromBool(b));
}
/// Returns true when L is the main thread.
pub inline fn pushthread(L: *State) bool {
    return vm.lua_pushthread(L) != 0;
}
pub inline fn pushcclosurek(L: *State, f: CFunction, debugname: ?[*:0]const u8, nup: c_int, cont: ?Continuation) void {
    vm.lua_pushcclosurek(L, f, debugname, nup, cont);
}
pub inline fn pushcfunction(L: *State, f: CFunction, debugname: ?[*:0]const u8) void {
    vm.lua_pushcclosurek(L, f, debugname, 0, null);
}
pub inline fn pushcclosure(L: *State, f: CFunction, debugname: ?[*:0]const u8, nup: c_int) void {
    vm.lua_pushcclosurek(L, f, debugname, nup, null);
}
pub inline fn newuserdatatagged(L: *State, sz: usize, tag: c_int) ?*anyopaque {
    return vm.lua_newuserdatatagged(L, sz, tag);
}
/// lua_newbuffer: pushes a zero-filled buffer object and returns its bytes.
pub inline fn newbuffer(L: *State, sz: usize) []u8 {
    const p = vm.lua_newbuffer(L, sz);
    if (sz == 0) return &[_]u8{};
    return @as([*]u8, @ptrCast(p.?))[0..sz];
}

// ---------------------------------------------------------------------------------------
// Get functions

pub inline fn gettable(L: *State, idx: c_int) c_int {
    return vm.lua_gettable(L, idx);
}
pub inline fn getfield(L: *State, idx: c_int, k: [*:0]const u8) c_int {
    return vm.lua_getfield(L, idx, k);
}
pub inline fn rawgetfield(L: *State, idx: c_int, k: [*:0]const u8) c_int {
    return vm.lua_rawgetfield(L, idx, k);
}
pub inline fn rawget(L: *State, idx: c_int) c_int {
    return vm.lua_rawget(L, idx);
}
pub inline fn rawgeti(L: *State, idx: c_int, n: c_int) c_int {
    return vm.lua_rawgeti(L, idx, n);
}
pub inline fn createtable(L: *State, narr: c_int, nrec: c_int) void {
    vm.lua_createtable(L, narr, nrec);
}
pub inline fn newtable(L: *State) void {
    vm.lua_createtable(L, 0, 0);
}
pub inline fn setreadonly(L: *State, idx: c_int, enabled: bool) void {
    vm.lua_setreadonly(L, idx, @intFromBool(enabled));
}
pub inline fn getreadonly(L: *State, idx: c_int) bool {
    return vm.lua_getreadonly(L, idx) != 0;
}
pub inline fn setsafeenv(L: *State, idx: c_int, enabled: bool) void {
    vm.lua_setsafeenv(L, idx, @intFromBool(enabled));
}
pub inline fn getmetatable(L: *State, idx: c_int) bool {
    return vm.lua_getmetatable(L, idx) != 0;
}
pub inline fn getfenv(L: *State, idx: c_int) void {
    vm.lua_getfenv(L, idx);
}

// ---------------------------------------------------------------------------------------
// Set functions

pub inline fn settable(L: *State, idx: c_int) void {
    vm.lua_settable(L, idx);
}
pub inline fn setfield(L: *State, idx: c_int, k: [*:0]const u8) void {
    vm.lua_setfield(L, idx, k);
}
pub inline fn rawsetfield(L: *State, idx: c_int, k: [*:0]const u8) void {
    vm.lua_rawsetfield(L, idx, k);
}
pub inline fn rawset(L: *State, idx: c_int) void {
    vm.lua_rawset(L, idx);
}
pub inline fn rawseti(L: *State, idx: c_int, n: c_int) void {
    vm.lua_rawseti(L, idx, n);
}
pub inline fn setmetatable(L: *State, idx: c_int) void {
    _ = vm.lua_setmetatable(L, idx);
}
pub inline fn setfenv(L: *State, idx: c_int) bool {
    return vm.lua_setfenv(L, idx) != 0;
}
pub inline fn setglobal(L: *State, k: [*:0]const u8) void {
    vm.lua_setfield(L, GLOBALSINDEX, k);
}

// ---------------------------------------------------------------------------------------
// Calls, coroutines, misc

pub inline fn call(L: *State, nargs: c_int, nresults: c_int) void {
    vm.lua_call(L, nargs, nresults);
}
pub inline fn pcallyieldable(L: *State, nargs: c_int, nresults: c_int, errfunc: c_int) c_int {
    return vm.lua_pcallyieldable(L, nargs, nresults, errfunc);
}
pub inline fn yield(L: *State, nresults: c_int) c_int {
    return vm.lua_yield(L, nresults);
}
pub inline fn @"break"(L: *State) c_int {
    return vm.lua_break(L);
}
pub inline fn @"resume"(co: *State, from: *State, narg: c_int) c_int {
    return vm.lua_resume(co, from, narg);
}
pub inline fn status(L: *State) c_int {
    return vm.lua_status(L);
}
pub inline fn isyieldable(L: *State) bool {
    return vm.lua_isyieldable(L) != 0;
}
pub inline fn costatus(L: *State, co: *State) c_int {
    return vm.lua_costatus(L, co);
}
pub inline fn newthread(L: *State) *State {
    return vm.lua_newthread(L);
}
// lua_Debug field accessors (C `const char*` members)
pub inline fn debugName(ar: *const Debug) ?[]const u8 {
    const p = cstr(ar.name) orelse return null;
    return span(p);
}
pub inline fn debugWhat(ar: *const Debug) []const u8 {
    return span(cstr(ar.what) orelse return "");
}
pub inline fn debugHasSource(ar: *const Debug) bool {
    return cstr(ar.source) != null;
}
pub inline fn debugShortSrc(ar: *const Debug) []const u8 {
    return span(cstr(ar.short_src) orelse return "");
}

pub inline fn resetthread(L: *State) void {
    vm.lua_resetthread(L);
}
pub inline fn gc(L: *State, what: c_int, data: c_int) c_int {
    return vm.lua_gc(L, what, data);
}
pub inline fn @"error"(L: *State) noreturn {
    vm.lua_error(L);
}
pub inline fn next(L: *State, idx: c_int) bool {
    return vm.lua_next(L, idx) != 0;
}
pub inline fn rawiter(L: *State, idx: c_int, iter: c_int) c_int {
    return vm.lua_rawiter(L, idx, iter);
}
pub inline fn concat(L: *State, n: c_int) void {
    vm.lua_concat(L, n);
}
pub inline fn encodepointer(L: *State, p: usize) usize {
    return vm.lua_encodepointer(L, p);
}
pub inline fn clock() f64 {
    return vm.lua_clock();
}
pub inline fn cleartable(L: *State, idx: c_int) void {
    vm.lua_cleartable(L, idx);
}
pub inline fn clonetable(L: *State, idx: c_int) void {
    vm.lua_clonetable(L, idx);
}
pub inline fn getinfo(L: *State, level: c_int, what: [*:0]const u8, ar: *Debug) bool {
    return vm.lua_getinfo(L, level, what, ar) != 0;
}
pub inline fn newstate(f: Alloc, ud: ?*anyopaque) ?*State {
    return vm.lua_newstate(f, ud);
}

/// C-ABI va_list parameter type for the current target (provided by the VM).
pub const RawVaList = vm.RawVaList;

/// lua_pushvfstring with a raw C va_list (used by the C-ABI `luaL_errorL` export).
pub inline fn pushvfstring(L: *State, fmt: [*:0]const u8, ap: RawVaList) [*:0]const u8 {
    return cstr(vm.lua_pushvfstring(L, fmt, ap)).?;
}

// ---------------------------------------------------------------------------------------
// VM internals the upstream libraries reach directly (ltablib/lcorolib/lmathlib use
// `LuaTable` fields and `lua_State` members). The VM exposes these as narrow helpers.

/// Array-part and hash-part slot counts of the table at `idx` (LuaTable::sizearray, sizenode).
pub inline fn tablesizes(L: *State, idx: c_int) struct { array: c_int, node: c_int } {
    var a: c_int = 0;
    var n: c_int = 0;
    vm.luau_tablesizes(L, idx, &a, &n);
    return .{ .array = a, .node = n };
}
/// luaH_resizearray on the table at `idx` (performance-only growth used by table.move).
pub inline fn tableresizearray(L: *State, idx: c_int, nsize: c_int) void {
    vm.luau_tableresizearray(L, idx, nsize);
}
/// lua_State::singlestep copy used by coroutine.resume (debugger single-step propagation).
pub inline fn copysinglestep(co: *State, from: *State) void {
    vm.lua_singlestep(co, vm.luau_getsinglestep(from));
}
/// Calls the global debuginterrupt callback (if any) for `co`, like luau_callhook in lcorolib.
pub inline fn calldebuginterrupt(L: *State, co: *State) void {
    vm.luau_calldebuginterrupt(L, co);
}
/// Runs the interrupt callback in a non-yieldable context (lstrlib `match` safepoint).
pub inline fn interruptnonyieldable(L: *State) void {
    vm.luau_interruptnonyieldable(L);
}

/// Pushes a fresh mutable string of `size` bytes (luaS_bufstart) and returns its storage.
pub inline fn strbufstart(L: *State, size: usize) [*]u8 {
    return vm.luau_strbufstart(L, size);
}
/// Replaces the mutable string at `idx` by the final string of `used` bytes
/// (luaS_buffinish when it is exactly full, a fresh copy otherwise; runs luaC_checkGC).
pub inline fn strbuffinish(L: *State, idx: c_int, used: usize) void {
    vm.luau_strbuffinish(L, idx, used);
}
/// Swaps t->array[i] and t->array[j] (0-based, inside sizearray) without readonly checks.
pub inline fn tableswap(L: *State, idx: c_int, i: c_int, j: c_int) void {
    vm.luau_tableswap(L, idx, i, j);
}
/// t->node == &luaH_dummynode
pub inline fn tableisdummynode(L: *State, idx: c_int) bool {
    return vm.luau_tableisdummynode(L, idx);
}
/// luaH_rawfind(t, init, v): 1-based index of the first raw-equal array/hash entry, or 0.
pub inline fn tablerawfind(L: *State, tidx: c_int, init: c_int, vidx: c_int) c_int {
    return vm.luau_tablerawfind(L, tidx, init, vidx);
}
/// iscollectable(v) && __eq metamethod present (luaT_gettmbyobj(v, TM_EQ) non-nil).
pub inline fn haseqtm(L: *State, idx: c_int) bool {
    return vm.luau_haseqtm(L, idx);
}

// coroutine.finally support (LuauCoroutineFinally); FinallyResult values from ldo.h
pub const FinallyFinished: c_int = 0;
pub const FinallyError: c_int = 1;
pub const FinallyCancelled: c_int = 2;
pub inline fn hasfinalizers(L: *State) bool {
    return vm.lua_hasfinalizers(L) != 0;
}
pub inline fn addfinalizer(L: *State, co: *State, idx: c_int) void {
    vm.lua_addfinalizer(L, co, idx);
}
pub inline fn preparefinalize(L: *State, co: *State, result: c_int) void {
    vm.luau_preparefinalize(L, co, result);
}
pub inline fn preparefinalizestate(L: *State, co: *State, result: c_int) void {
    vm.luau_preparefinalizestate(L, co, result);
}
pub inline fn runfinalizers(L: *State, returnstatus: bool) c_int {
    return vm.luau_runfinalizers(L, returnstatus);
}

/// Records the closure at idx as global->builtinPcall / builtinXpcall (LuauFastpcall).
pub inline fn setbuiltinpcall(L: *State, idx: c_int, xpcall: bool) void {
    vm.luau_setbuiltinpcall(L, idx, xpcall);
}

// class library (user-defined classes runtime)
pub inline fn isinstance(L: *State, objidx: c_int, clsidx: c_int) bool {
    return vm.luau_isinstance(L, objidx, clsidx);
}
pub inline fn pushclassof(L: *State, idx: c_int) void {
    vm.luau_pushclassof(L, idx);
}

/// Fast flags owned by the VM (vm.fflags = src/vm/flags.zig), e.g. LuauCoroutineFinally.
pub inline fn vmflag(comptime name: []const u8) bool {
    return @field(vm.fflags, name);
}

pub inline fn pushvector(L: *State, x: f32, y: f32, z: f32) void {
    vm.lua_pushvector(L, x, y, z);
}
pub inline fn pushinteger64(L: *State, n: i64) void {
    vm.lua_pushinteger64(L, n);
}
