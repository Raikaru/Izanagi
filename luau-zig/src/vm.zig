//! Public interface of the Zig Luau VM.
//!
//! The VM targets the upstream Luau bytecode format (third_party/luau,
//! Common/include/Luau/Bytecode.h) and lua.h host API. Lua C ABI functions are
//! exported under their upstream names (see vm/api.zig); Zig callers can use
//! this module's re-exports or vm.api. See tests/README.txt for coverage and
//! unsupported variants.
//!
//! Error model: like upstream built with LUA_USE_LONGJMP=1, raising functions do not
//! return; unwinding is done by vm/jmp.zig (Zig naked functions, no C). Callers must
//! not hold Zig `defer`-managed or non-VM resources across calls that may raise.
//!
//! Derived from Luau (MIT, Copyright (c) 2019-2024 Roblox Corporation) and Lua 5.1
//! (MIT, Copyright (C) 1994-2008 Lua.org, PUC-Rio); see third_party/luau/LICENSE.txt
//! and third_party/luau/lua_LICENSE.txt.

const std = @import("std");
const o = @import("vm/object.zig");
pub const api = @import("vm/api.zig");
const printf = @import("vm/printf.zig");
const numprint = @import("vm/numprint.zig");
const vmu = @import("vm/vmutils.zig");
const load = @import("vm/load.zig");

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

pub const lua_State = o.State;
pub const lua_CFunction = o.lua_CFunction;
pub const lua_Continuation = o.lua_Continuation;
pub const lua_Alloc = o.lua_Alloc;
pub const lua_Destructor = o.lua_Destructor;
pub const lua_Hook = o.lua_Hook;
pub const lua_Debug = o.lua_Debug;
pub const lua_Callbacks = o.lua_Callbacks;
pub const lua_UserdataDirectAccess = o.lua_UserdataDirectAccess;
pub const lua_UserdataDirectNamecall = o.lua_UserdataDirectNamecall;
pub const lua_CageAlloc = o.lua_CageAlloc;
pub const lua_UserdataDirectFieldGet = api.lua_UserdataDirectFieldGet;
pub const lua_Number = f64;
pub const lua_Integer = c_int;
pub const lua_Unsigned = c_uint;
pub const RawVaList = printf.RawVaList;
pub const VaReader = printf.VaReader;
pub const fflags = api.fflags;
pub const luau_setfflag = fflags.luau_setfflag;
pub const lua_Coverage = @import("vm/debug.zig").lua_Coverage;
pub const lua_UserdataMark = o.lua_UserdataMark;
pub const lua_EmbedderGc = o.lua_EmbedderGc;
pub const lua_EmbedderMark = o.lua_EmbedderMark;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

pub const LUA_MULTRET = o.LUA_MULTRET;
pub const LUA_REGISTRYINDEX = o.LUA_REGISTRYINDEX;
pub const LUA_ENVIRONINDEX = o.LUA_ENVIRONINDEX;
pub const LUA_GLOBALSINDEX = o.LUA_GLOBALSINDEX;
pub const LUA_OK = o.LUA_OK;
pub const LUA_YIELD = o.LUA_YIELD;
pub const LUA_ERRRUN = o.LUA_ERRRUN;
pub const LUA_ERRSYNTAX = o.LUA_ERRSYNTAX;
pub const LUA_ERRMEM = o.LUA_ERRMEM;
pub const LUA_ERRERR = o.LUA_ERRERR;
pub const LUA_BREAK = o.LUA_BREAK;
pub const LUA_CORUN = o.LUA_CORUN;
pub const LUA_COSUS = o.LUA_COSUS;
pub const LUA_CONOR = o.LUA_CONOR;
pub const LUA_COFIN = o.LUA_COFIN;
pub const LUA_COERR = o.LUA_COERR;
pub const LUA_TNONE = o.LUA_TNONE;
pub const LUA_TNIL: c_int = o.T_NIL;
pub const LUA_TBOOLEAN: c_int = o.T_BOOLEAN;
pub const LUA_TLIGHTUSERDATA: c_int = o.T_LIGHTUSERDATA;
pub const LUA_TNUMBER: c_int = o.T_NUMBER;
pub const LUA_TINTEGER: c_int = o.T_INTEGER;
pub const LUA_TVECTOR: c_int = o.T_VECTOR;
pub const LUA_TSTRING: c_int = o.T_STRING;
pub const LUA_TTABLE: c_int = o.T_TABLE;
pub const LUA_TFUNCTION: c_int = o.T_FUNCTION;
pub const LUA_TUSERDATA: c_int = o.T_USERDATA;
pub const LUA_TTHREAD: c_int = o.T_THREAD;
pub const LUA_TBUFFER: c_int = o.T_BUFFER;
pub const LUA_TCLASS: c_int = o.T_CLASS;
pub const LUA_TOBJECT: c_int = o.T_OBJECT;
pub const LUA_T_COUNT: c_int = o.T_COUNT;
pub const LUA_NOREF = o.LUA_NOREF;
pub const LUA_REFNIL = o.LUA_REFNIL;
pub const LUA_GCSTOP = o.LUA_GCSTOP;
pub const LUA_GCRESTART = o.LUA_GCRESTART;
pub const LUA_GCCOLLECT = o.LUA_GCCOLLECT;
pub const LUA_GCCOUNT = o.LUA_GCCOUNT;
pub const LUA_GCCOUNTB = o.LUA_GCCOUNTB;
pub const LUA_GCISRUNNING = o.LUA_GCISRUNNING;
pub const LUA_GCSTEP = o.LUA_GCSTEP;
pub const LUA_GCSETGOAL = o.LUA_GCSETGOAL;
pub const LUA_GCSETSTEPMUL = o.LUA_GCSETSTEPMUL;
pub const LUA_GCSETSTEPSIZE = o.LUA_GCSETSTEPSIZE;
pub const LUA_GCISPAUSED = o.LUA_GCISPAUSED;
pub const LUA_IDSIZE = o.LUA_IDSIZE;
pub const LUA_MINSTACK = o.LUA_MINSTACK;
pub const LUAI_MAXCSTACK = o.LUAI_MAXCSTACK;
pub const LUAI_MAXCALLS = o.LUAI_MAXCALLS;
pub const LUAI_MAXCCALLS = o.LUAI_MAXCCALLS;
pub const LUA_BUFFERSIZE = o.LUA_BUFFERSIZE;
pub const LUA_UTAG_LIMIT = o.LUA_UTAG_LIMIT;
pub const LUA_LUTAG_LIMIT = o.LUA_LUTAG_LIMIT;
pub const LUA_MEMORY_CATEGORIES = o.LUA_MEMORY_CATEGORIES;
pub const LUA_VECTOR_SIZE = o.LUA_VECTOR_SIZE;
pub const LUA_MEMERRMSG = o.LUA_MEMERRMSG;
pub const LUA_ERRERRMSG = o.LUA_ERRERRMSG;
pub const MAXSSIZE = o.MAXSSIZE;
pub const MAX_BUFFER_SIZE = o.MAX_BUFFER_SIZE;
pub const LBC_VERSION_MIN = load.LBC_VERSION_MIN;
pub const LBC_VERSION_MAX = load.LBC_VERSION_MAX;
pub const LBC_VERSION_TARGET = load.LBC_VERSION_TARGET;

// ---------------------------------------------------------------------------
// lua.h functions (C ABI exports, re-exported for Zig)
// ---------------------------------------------------------------------------

pub const lua_newstate = api.lua_newstate;
pub const lua_close = api.lua_close;
pub const lua_newthread = api.lua_newthread;
pub const lua_mainthread = api.lua_mainthread;
pub const lua_resetthread = api.lua_resetthread;
pub const lua_isthreadreset = api.lua_isthreadreset;
pub const lua_absindex = api.lua_absindex;
pub const lua_gettop = api.lua_gettop;
pub const lua_settop = api.lua_settop;
pub const lua_pushvalue = api.lua_pushvalue;
pub const lua_remove = api.lua_remove;
pub const lua_insert = api.lua_insert;
pub const lua_replace = api.lua_replace;
pub const lua_checkstack = api.lua_checkstack;
pub const lua_rawcheckstack = api.lua_rawcheckstack;
pub const lua_xmove = api.lua_xmove;
pub const lua_xpush = api.lua_xpush;
pub const lua_isnumber = api.lua_isnumber;
pub const lua_isstring = api.lua_isstring;
pub const lua_isinteger64 = api.lua_isinteger64;
pub const lua_iscfunction = api.lua_iscfunction;
pub const lua_isLfunction = api.lua_isLfunction;
pub const lua_isuserdata = api.lua_isuserdata;
pub const lua_type = api.lua_type;
pub const lua_typename = api.lua_typename;
pub const lua_equal = api.lua_equal;
pub const lua_rawequal = api.lua_rawequal;
pub const lua_lessthan = api.lua_lessthan;
pub const lua_tonumberx = api.lua_tonumberx;
pub const lua_tointegerx = api.lua_tointegerx;
pub const lua_tounsignedx = api.lua_tounsignedx;
pub const lua_tointeger64 = api.lua_tointeger64;
pub const lua_tovector = api.lua_tovector;
pub const lua_toboolean = api.lua_toboolean;
pub const lua_tolstring = api.lua_tolstring;
pub const lua_tostringatom = api.lua_tostringatom;
pub const lua_tolstringatom = api.lua_tolstringatom;
pub const lua_namecallatom = api.lua_namecallatom;
pub const lua_objlen = api.lua_objlen;
pub const lua_tocfunction = api.lua_tocfunction;
pub const lua_tolightuserdata = api.lua_tolightuserdata;
pub const lua_tolightuserdatatagged = api.lua_tolightuserdatatagged;
pub const lua_touserdata = api.lua_touserdata;
pub const lua_touserdatatagged = api.lua_touserdatatagged;
pub const lua_userdatatag = api.lua_userdatatag;
pub const lua_lightuserdatatag = api.lua_lightuserdatatag;
pub const lua_tothread = api.lua_tothread;
pub const lua_tobuffer = api.lua_tobuffer;
pub const lua_topointer = api.lua_topointer;
pub const lua_pushnil = api.lua_pushnil;
pub const lua_pushnumber = api.lua_pushnumber;
pub const lua_pushinteger = api.lua_pushinteger;
pub const lua_pushinteger64 = api.lua_pushinteger64;
pub const lua_pushunsigned = api.lua_pushunsigned;
pub const lua_pushvector = api.lua_pushvector;
pub const lua_pushlstring = api.lua_pushlstring;
pub const lua_pushstring = api.lua_pushstring;
pub const lua_pushvfstring = api.lua_pushvfstring;
pub const lua_pushcclosurek = api.lua_pushcclosurek;
pub const lua_pushboolean = api.lua_pushboolean;
pub const lua_pushthread = api.lua_pushthread;
pub const lua_pushlightuserdatatagged = api.lua_pushlightuserdatatagged;
pub const lua_newuserdatatagged = api.lua_newuserdatatagged;
pub const lua_newuserdatataggedwithmetatable = api.lua_newuserdatataggedwithmetatable;
pub const lua_newuserdatadtor = api.lua_newuserdatadtor;
pub const lua_newbuffer = api.lua_newbuffer;
pub const lua_gettable = api.lua_gettable;
pub const lua_getfield = api.lua_getfield;
pub const lua_rawgetfield = api.lua_rawgetfield;
pub const lua_rawget = api.lua_rawget;
pub const lua_rawgeti = api.lua_rawgeti;
pub const lua_rawgetptagged = api.lua_rawgetptagged;
pub const lua_createtable = api.lua_createtable;
pub const lua_setreadonly = api.lua_setreadonly;
pub const lua_getreadonly = api.lua_getreadonly;
pub const lua_setsafeenv = api.lua_setsafeenv;
pub const lua_getmetatable = api.lua_getmetatable;
pub const lua_getfenv = api.lua_getfenv;
pub const lua_settable = api.lua_settable;
pub const lua_setfield = api.lua_setfield;
pub const lua_rawsetfield = api.lua_rawsetfield;
pub const lua_rawset = api.lua_rawset;
pub const lua_rawseti = api.lua_rawseti;
pub const lua_rawsetptagged = api.lua_rawsetptagged;
pub const lua_setmetatable = api.lua_setmetatable;
pub const lua_setfenv = api.lua_setfenv;
pub const luau_load = api.luau_load;
pub const lua_call = api.lua_call;
pub const lua_pcall = api.lua_pcall;
pub const lua_cpcall = api.lua_cpcall;
pub const lua_callyieldable = api.lua_callyieldable;
pub const lua_pcallyieldable = api.lua_pcallyieldable;
pub const lua_yield = api.lua_yield;
pub const lua_break = api.lua_break;
pub const lua_resume = api.lua_resume;
pub const lua_resumeerror = api.lua_resumeerror;
pub const lua_status = api.lua_status;
pub const lua_isyieldable = api.lua_isyieldable;
pub const lua_getthreaddata = api.lua_getthreaddata;
pub const lua_setthreaddata = api.lua_setthreaddata;
pub const lua_costatus = api.lua_costatus;
pub const lua_hasfinalizers = api.lua_hasfinalizers;
pub const lua_pushfinalizerfunction = api.lua_pushfinalizerfunction;
pub const lua_addfinalizer = api.lua_addfinalizer;
pub const lua_gc = api.lua_gc;
pub const lua_memorydump = api.lua_memorydump;
pub const lua_setmemcat = api.lua_setmemcat;
pub const lua_totalbytes = api.lua_totalbytes;
pub const lua_allocationrate = api.lua_allocationrate;
pub const lua_error = api.lua_error;
pub const lua_next = api.lua_next;
pub const lua_rawiter = api.lua_rawiter;
pub const lua_concat = api.lua_concat;
pub const lua_setpointerencodekey = api.lua_setpointerencodekey;
pub const lua_encodepointer = api.lua_encodepointer;
pub const lua_clock = api.lua_clock;
pub const lua_setuserdatatag = api.lua_setuserdatatag;
pub const lua_setuserdatadtor = api.lua_setuserdatadtor;
pub const lua_getuserdatadtor = api.lua_getuserdatadtor;
pub const lua_setuserdatamark = api.lua_setuserdatamark;
pub const lua_setembeddergc = api.lua_setembeddergc;
pub const lua_weakref = api.lua_weakref;
pub const lua_weakunref = api.lua_weakunref;
pub const lua_getweakref = api.lua_getweakref;
pub const lua_setuserdatametatable = api.lua_setuserdatametatable;
pub const lua_getuserdatametatable = api.lua_getuserdatametatable;
pub const lua_getuserdataname = api.lua_getuserdataname;
pub const lua_registeruserdatadirectfieldget = api.lua_registeruserdatadirectfieldget;
pub const lua_registeruserdatadirectaccess = api.lua_registeruserdatadirectaccess;
pub const lua_userdatadirectfield_setnumber = api.lua_userdatadirectfield_setnumber;
pub const lua_userdatadirectfield_setvector = api.lua_userdatadirectfield_setvector;
pub const lua_userdatadirectfield_setboolean = api.lua_userdatadirectfield_setboolean;
pub const lua_userdatadirectfield_setinteger64 = api.lua_userdatadirectfield_setinteger64;
pub const lua_userdatadirectfield_setnil = api.lua_userdatadirectfield_setnil;
pub const lua_setlightuserdataname = api.lua_setlightuserdataname;
pub const lua_getlightuserdataname = api.lua_getlightuserdataname;
pub const lua_clonefunction = api.lua_clonefunction;
pub const lua_usesexport = api.lua_usesexport;
pub const lua_cleartable = api.lua_cleartable;
pub const lua_clonetable = api.lua_clonetable;
pub const lua_getallocf = api.lua_getallocf;
pub const lua_ref = api.lua_ref;
pub const lua_unref = api.lua_unref;
pub const lua_callhook = api.lua_callhook;
pub const lua_stackdepth = api.lua_stackdepth;
pub const lua_getinfo = api.lua_getinfo;
pub const lua_getargument = api.lua_getargument;
pub const lua_getlocal = api.lua_getlocal;
pub const lua_setlocal = api.lua_setlocal;
pub const lua_getupvalue = api.lua_getupvalue;
pub const lua_setupvalue = api.lua_setupvalue;
pub const lua_hascustomexecution = api.lua_hascustomexecution;
pub const lua_incustomexecution = api.lua_incustomexecution;
pub const lua_singlestep = api.lua_singlestep;
pub const lua_breakpoint = api.lua_breakpoint;
pub const lua_atbreakpoint = api.lua_atbreakpoint;
pub const lua_debugtrace = api.lua_debugtrace;
pub const lua_getcoverage = api.lua_getcoverage;
pub const lua_getcounters = api.lua_getcounters;
pub const lua_callbacks = api.lua_callbacks;
pub const lua_setmemorycage = api.lua_setmemorycage;
pub const luau_tablesizes = api.luau_tablesizes;
pub const luau_tableresizearray = api.luau_tableresizearray;
pub const luau_getsinglestep = api.luau_getsinglestep;
pub const luau_calldebuginterrupt = api.luau_calldebuginterrupt;
pub const luau_interruptnonyieldable = api.luau_interruptnonyieldable;
pub const luau_strbufstart = api.luau_strbufstart;
pub const luau_strbuffinish = api.luau_strbuffinish;
pub const luau_tableswap = api.luau_tableswap;
pub const luau_tableisdummynode = api.luau_tableisdummynode;
pub const luau_tablerawfind = api.luau_tablerawfind;
pub const luau_haseqtm = api.luau_haseqtm;
pub const luau_setbuiltinpcall = api.luau_setbuiltinpcall;
pub const luau_preparefinalize = api.luau_preparefinalize;
pub const luau_preparefinalizestate = api.luau_preparefinalizestate;
pub const luau_runfinalizers = api.luau_runfinalizers;
pub const luau_isinstance = api.luau_isinstance;
pub const luau_pushclassof = api.luau_pushclassof;

// ---------------------------------------------------------------------------
// lua.h macro equivalents
// ---------------------------------------------------------------------------

pub inline fn lua_upvalueindex(i: c_int) c_int {
    return LUA_GLOBALSINDEX - i;
}
pub inline fn lua_ispseudo(i: c_int) bool {
    return i <= LUA_REGISTRYINDEX;
}
pub inline fn lua_tonumber(L: *lua_State, i: c_int) f64 {
    return api.lua_tonumberx(L, i, null);
}
pub inline fn lua_tointeger(L: *lua_State, i: c_int) c_int {
    return api.lua_tointegerx(L, i, null);
}
pub inline fn lua_tounsigned(L: *lua_State, i: c_int) c_uint {
    return api.lua_tounsignedx(L, i, null);
}
pub inline fn lua_pop(L: *lua_State, n: c_int) void {
    api.lua_settop(L, -n - 1);
}
pub inline fn lua_newtable(L: *lua_State) void {
    api.lua_createtable(L, 0, 0);
}
pub inline fn lua_newuserdata(L: *lua_State, s: usize) ?*anyopaque {
    return api.lua_newuserdatatagged(L, s, 0);
}
pub inline fn lua_strlen(L: *lua_State, i: c_int) c_int {
    return api.lua_objlen(L, i);
}
pub inline fn lua_isfunction(L: *lua_State, n: c_int) bool {
    return api.lua_type(L, n) == LUA_TFUNCTION;
}
pub inline fn lua_istable(L: *lua_State, n: c_int) bool {
    return api.lua_type(L, n) == LUA_TTABLE;
}
pub inline fn lua_islightuserdata(L: *lua_State, n: c_int) bool {
    return api.lua_type(L, n) == LUA_TLIGHTUSERDATA;
}
pub inline fn lua_isnil(L: *lua_State, n: c_int) bool {
    return api.lua_type(L, n) == LUA_TNIL;
}
pub inline fn lua_isboolean(L: *lua_State, n: c_int) bool {
    return api.lua_type(L, n) == LUA_TBOOLEAN;
}
pub inline fn lua_isvector(L: *lua_State, n: c_int) bool {
    return api.lua_type(L, n) == LUA_TVECTOR;
}
pub inline fn lua_isthread(L: *lua_State, n: c_int) bool {
    return api.lua_type(L, n) == LUA_TTHREAD;
}
pub inline fn lua_isbuffer(L: *lua_State, n: c_int) bool {
    return api.lua_type(L, n) == LUA_TBUFFER;
}
pub inline fn lua_isnone(L: *lua_State, n: c_int) bool {
    return api.lua_type(L, n) == LUA_TNONE;
}
pub inline fn lua_isnoneornil(L: *lua_State, n: c_int) bool {
    return api.lua_type(L, n) <= LUA_TNIL;
}
pub inline fn lua_pushliteral(L: *lua_State, s: []const u8) void {
    api.lua_pushlstring(L, s.ptr, s.len);
}
pub inline fn lua_pushcfunction(L: *lua_State, f: lua_CFunction, debugname: ?[*:0]const u8) void {
    api.lua_pushcclosurek(L, f, debugname, 0, null);
}
pub inline fn lua_pushcclosure(L: *lua_State, f: lua_CFunction, debugname: ?[*:0]const u8, nup: c_int) void {
    api.lua_pushcclosurek(L, f, debugname, nup, null);
}
pub inline fn lua_pushlightuserdata(L: *lua_State, p: ?*anyopaque) void {
    api.lua_pushlightuserdatatagged(L, p, 0);
}
pub inline fn lua_setglobal(L: *lua_State, s: [*:0]const u8) void {
    api.lua_setfield(L, LUA_GLOBALSINDEX, s);
}
pub inline fn lua_getglobal(L: *lua_State, s: [*:0]const u8) c_int {
    return api.lua_getfield(L, LUA_GLOBALSINDEX, s);
}
pub inline fn lua_tostring(L: *lua_State, i: c_int) ?[*:0]const u8 {
    return api.lua_tolstring(L, i, null);
}
pub inline fn lua_getref(L: *lua_State, ref: c_int) c_int {
    return api.lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
}
pub inline fn lua_rawgetp(L: *lua_State, idx: c_int, p: ?*anyopaque) c_int {
    return api.lua_rawgetptagged(L, idx, p, 0);
}
pub inline fn lua_rawsetp(L: *lua_State, idx: c_int, p: ?*anyopaque) void {
    api.lua_rawsetptagged(L, idx, p, 0);
}

// ---------------------------------------------------------------------------
// Helpers for the standard library
// ---------------------------------------------------------------------------

/// Emits a C-variadic export `name(L, fmt, ...)` forwarding to `impl` with a
/// platform va_list. Use inside `comptime { }`.
pub fn exportVariadic2(comptime name: []const u8, comptime impl: fn (?*lua_State, [*:0]const u8, RawVaList) callconv(.c) c_int) void {
    api.exportVariadic2(?*lua_State, c_int, name, impl);
}

/// printf-compatible formatting of a va_list into `buf` (truncating); returns the
/// formatted slice.
pub fn formatv(buf: []u8, fmt: [*:0]const u8, ap: RawVaList) []u8 {
    var out: printf.Out = .{ .buf = buf };
    var reader: VaReader = .{ .ap = ap };
    printf.vformat(&out, fmt, &reader);
    return out.slice();
}

/// Upstream `^` / math.pow.
pub fn luai_numpow(a: f64, b: f64) f64 {
    return vmu.luai_numpow(a, b);
}

/// Upstream number formatting (tostring of numbers).
pub fn luai_num2str(buf: *[48]u8, n: f64) []u8 {
    return numprint.num2str(buf, n);
}

/// Upstream luai_int2str.
pub fn luai_int2str(buf: *[32]u8, n: i64) []u8 {
    return numprint.int2str(buf, n);
}

/// Upstream luaO_str2d (string -> number coercion); null when `s` is not a number.
pub fn luaO_str2d(s: [*:0]const u8) ?f64 {
    return numprint.str2d(std.mem.span(s));
}

comptime {
    // Zig's self-hosted x86_64 backend cannot assemble the VM's unwinding and
    // C-varargs entry code (no CET instruction encodings, no `.byte`, limited SSE
    // memory operands), so the VM only supports the LLVM backend there.
    if (@import("builtin").cpu.arch == .x86_64 and @import("builtin").zig_backend != .stage2_llvm)
        @compileError("luau-zig VM: x86_64 builds require the LLVM backend (-fllvm / use_llvm = true); the self-hosted backend is unsupported");
    // force analysis/export of every C ABI entry point
    _ = api;
}
