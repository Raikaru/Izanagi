//! The lua.h C API (port of upstream lapi.cpp, MIT; see object.zig header).
//!
//! Every function here is exported with the C ABI under its upstream name, so C/C++
//! hosts compiled against third_party/luau/VM/include/lua.h link against this VM.

const std = @import("std");
const builtin = @import("builtin");
const o = @import("object.zig");
const ldo = @import("do.zig");
const mem = @import("mem.zig");
const debug = @import("debug.zig");
const func = @import("func.zig");
const gc = @import("gc.zig");
const table = @import("table.zig");
const lstring = @import("string.zig");
const tm = @import("tm.zig");
const vmu = @import("vmutils.zig");
const state = @import("state.zig");
const load = @import("load.zig");
const printf = @import("printf.zig");

const State = o.State;
const TValue = o.TValue;
const StkId = o.StkId;

const LPF_USES_EXPORT: u8 = 1 << 4;

// ---------------------------------------------------------------------------
// Index helpers
// ---------------------------------------------------------------------------

fn getcurrenv(L: *State) *o.Table {
    if (L.ci == &L.base_ci[0]) return L.gt;
    return o.curr_func(L).env;
}

fn pseudo2addr(L: *State, idx: c_int) *TValue {
    const g = L.global;
    switch (idx) {
        o.LUA_REGISTRYINDEX => return &g.registry,
        o.LUA_ENVIRONINDEX => {
            o.sethvalue(&g.pseudotemp, getcurrenv(L));
            return &g.pseudotemp;
        },
        o.LUA_GLOBALSINDEX => {
            o.sethvalue(&g.pseudotemp, L.gt);
            return &g.pseudotemp;
        },
        else => {
            const f = o.curr_func(L);
            const n = o.LUA_GLOBALSINDEX - idx;
            return if (n <= f.nupvalues) &f.upvals()[@intCast(n - 1)] else o.nilobject();
        },
    }
}

/// index2addr: returns the nil object for absent positive indices.
pub fn index2value(L: *State, idx: c_int) *TValue {
    if (idx > 0) {
        const p = L.base + @as(usize, @intCast(idx - 1));
        if (@intFromPtr(p) >= @intFromPtr(L.top)) return o.nilobject();
        return &p[0];
    } else if (idx > o.LUA_REGISTRYINDEX) {
        return &(L.top - @as(usize, @intCast(-idx)))[0];
    } else {
        return pseudo2addr(L, idx);
    }
}

inline fn valid(v: *const TValue) bool {
    return v != o.nilobject();
}

fn ensureStack(L: *State, size: c_int) void {
    gc.threadbarrier(L);
    if (@intFromPtr(L.top + @as(usize, @intCast(@max(size, 0)))) > @intFromPtr(L.ci.top) and lua_checkstack(L, size) == 0) {
        pushstringRaw(L, "stack overflow");
        lua_error(L);
    }
}

/// Reserves and returns the next stack slot (api_incr_top with ensure_stack).
inline fn pushslot(L: *State) *TValue {
    ensureStack(L, 1);
    const s = &L.top[0];
    L.top += 1;
    return s;
}

pub fn pushstringRaw(L: *State, s: []const u8) void {
    ldo.checkstack(L, 1);
    o.setsvalue(&L.top[0], lstring.new(L, s));
    L.top += 1;
}

pub fn pushvalue(L: *State, v: *const TValue) void {
    const c = v.*;
    pushslot(L).* = c;
}

inline fn adjustresults(L: *State, nres: c_int) void {
    if (nres == o.LUA_MULTRET and @intFromPtr(L.top) >= @intFromPtr(L.ci.top)) L.ci.top = L.top;
}

// ---------------------------------------------------------------------------
// State manipulation
// ---------------------------------------------------------------------------

pub export fn lua_newstate(f: o.lua_Alloc, ud: ?*anyopaque) ?*State {
    return state.newstate(f, ud);
}

pub export fn lua_close(L: *State) void {
    state.close(L);
}

pub export fn lua_newthread(L: *State) *State {
    gc.checkGC(L);
    ensureStack(L, 1);
    const L1 = state.newthread(L);
    o.setthvalue(&L.top[0], L1);
    L.top += 1;
    if (L.global.cb.userthread) |cb| cb(L, L1);
    return L1;
}

pub export fn lua_mainthread(L: *State) *State {
    return L.global.mainthread;
}

pub export fn lua_resetthread(L: *State) void {
    state.resetthread(L);
}

pub export fn lua_isthreadreset(L: *State) c_int {
    return @intFromBool(state.isthreadreset(L));
}

// ---------------------------------------------------------------------------
// Basic stack manipulation
// ---------------------------------------------------------------------------

pub export fn lua_absindex(L: *State, idx: c_int) c_int {
    return if (idx > 0 or idx <= o.LUA_REGISTRYINDEX) idx else o.idiff(L.top, L.base) + idx + 1;
}

pub export fn lua_gettop(L: *State) c_int {
    return o.idiff(L.top, L.base);
}

pub export fn lua_settop(L: *State, idx: c_int) void {
    if (idx >= 0) {
        ensureStack(L, idx - o.idiff(L.top, L.base));
        const target = L.base + @as(usize, @intCast(idx));
        while (@intFromPtr(L.top) < @intFromPtr(target)) : (L.top += 1) o.setnilvalue(&L.top[0]);
        L.top = target;
    } else {
        L.top -= @as(usize, @intCast(-(idx + 1)));
    }
}

pub export fn lua_pushvalue(L: *State, idx: c_int) void {
    ensureStack(L, 1);
    const v = index2value(L, idx).*;
    L.top[0] = v;
    L.top += 1;
}

pub export fn lua_remove(L: *State, idx: c_int) void {
    var p = o.toStk(index2value(L, idx));
    p += 1;
    while (@intFromPtr(p) < @intFromPtr(L.top)) : (p += 1) (p - 1)[0] = p[0];
    L.top -= 1;
}

pub export fn lua_insert(L: *State, idx: c_int) void {
    gc.threadbarrier(L);
    const p = o.toStk(index2value(L, idx));
    var q = L.top;
    while (@intFromPtr(q) > @intFromPtr(p)) : (q -= 1) q[0] = (q - 1)[0];
    p[0] = L.top[0];
}

pub export fn lua_replace(L: *State, idx: c_int) void {
    gc.threadbarrier(L);
    const top = &(L.top - 1)[0];
    if (idx == o.LUA_ENVIRONINDEX) {
        const f = o.curr_func(L);
        f.env = o.hvalue(top);
        gc.barrier(L, &f.gch, top);
    } else if (idx == o.LUA_GLOBALSINDEX) {
        L.gt = o.hvalue(top);
    } else {
        index2value(L, idx).* = top.*;
        if (idx < o.LUA_GLOBALSINDEX) gc.barrier(L, &o.curr_func(L).gch, top);
    }
    L.top -= 1;
}

const GrowCtx = struct { size: c_int };
fn growBody(L: *State, ud: ?*anyopaque) void {
    const c: *GrowCtx = @ptrCast(@alignCast(ud.?));
    ldo.growstack(L, c.size);
}

pub export fn lua_checkstack(L: *State, size: c_int) c_int {
    if (size > o.LUAI_MAXCSTACK or o.idiff(L.top, L.base) + size > o.LUAI_MAXCSTACK) return 0;
    if (size > 0) {
        if (ldo.stacklimitreached(L, size)) {
            var ctx: GrowCtx = .{ .size = size };
            if (ldo.rawrunprotected(L, growBody, &ctx) != o.LUA_OK) return 0;
        }
        ldo.expandstacklimit(L, L.top + @as(usize, @intCast(size)));
    }
    return 1;
}

pub export fn lua_rawcheckstack(L: *State, size: c_int) void {
    ldo.rawcheckstack(L, size);
}

pub export fn lua_xmove(from: *State, to: *State, n: c_int) void {
    if (from == to) return;
    gc.threadbarrier(to);
    if (@intFromPtr(to.top + @as(usize, @intCast(n))) > @intFromPtr(to.ci.top) and lua_checkstack(to, n) == 0) {
        pushstringRaw(from, "stack overflow");
        lua_error(from);
    }
    const ttop = to.top;
    const ftop = from.top - @as(usize, @intCast(n));
    for (0..@intCast(n)) |i| ttop[i] = ftop[i];
    from.top = ftop;
    to.top = ttop + @as(usize, @intCast(n));
}

pub export fn lua_xpush(from: *State, to: *State, idx: c_int) void {
    gc.threadbarrier(to);
    if (@intFromPtr(to.top + 1) > @intFromPtr(to.ci.top) and lua_checkstack(to, 1) == 0) {
        pushstringRaw(from, "stack overflow");
        lua_error(from);
    }
    to.top[0] = index2value(from, idx).*;
    to.top += 1;
}

// ---------------------------------------------------------------------------
// Access functions (stack -> C)
// ---------------------------------------------------------------------------

pub export fn lua_isnumber(L: *State, idx: c_int) c_int {
    var n: TValue = .{};
    return @intFromBool(vmu.tonumber(index2value(L, idx), &n) != null);
}

pub export fn lua_isstring(L: *State, idx: c_int) c_int {
    const t = lua_type(L, idx);
    return @intFromBool(t == o.T_STRING or t == o.T_NUMBER);
}

pub export fn lua_isinteger64(L: *State, idx: c_int) c_int {
    return @intFromBool(o.ttisinteger(index2value(L, idx)));
}

pub export fn lua_iscfunction(L: *State, idx: c_int) c_int {
    const v = index2value(L, idx);
    return @intFromBool(o.ttisfunction(v) and o.clvalue(v).isC != 0);
}

pub export fn lua_isLfunction(L: *State, idx: c_int) c_int {
    const v = index2value(L, idx);
    return @intFromBool(o.ttisfunction(v) and o.clvalue(v).isC == 0);
}

pub export fn lua_isuserdata(L: *State, idx: c_int) c_int {
    const v = index2value(L, idx);
    return @intFromBool(o.ttisuserdata(v) or o.ttislightuserdata(v));
}

pub export fn lua_type(L: *State, idx: c_int) c_int {
    const v = index2value(L, idx);
    return if (valid(v)) v.tt else o.LUA_TNONE;
}

pub export fn lua_typename(L: *State, t: c_int) [*:0]const u8 {
    _ = L;
    if (t == o.LUA_TNONE) return "no value";
    if (t < 0 or t >= o.T_COUNT) return "?";
    return o.typenames[@intCast(t)].ptr;
}

pub export fn lua_equal(L: *State, index1: c_int, index2: c_int) c_int {
    const o1 = index2value(L, index1);
    const o2 = index2value(L, index2);
    if (!valid(o1) or !valid(o2)) return 0;
    if (o1.tt != o2.tt) return 0;
    return @intFromBool(vmu.equalval(L, o1, o2));
}

pub export fn lua_rawequal(L: *State, index1: c_int, index2: c_int) c_int {
    const o1 = index2value(L, index1);
    const o2 = index2value(L, index2);
    if (!valid(o1) or !valid(o2)) return 0;
    return @intFromBool(table.rawequalObj(o1, o2));
}

pub export fn lua_lessthan(L: *State, index1: c_int, index2: c_int) c_int {
    const o1 = index2value(L, index1);
    const o2 = index2value(L, index2);
    if (!valid(o1) or !valid(o2)) return 0;
    return @intFromBool(vmu.lessthan(L, o1, o2));
}

pub export fn lua_tonumberx(L: *State, idx: c_int, isnum: ?*c_int) f64 {
    var n: TValue = .{};
    if (vmu.tonumber(index2value(L, idx), &n)) |v| {
        if (isnum) |p| p.* = 1;
        return o.nvalue(v);
    }
    if (isnum) |p| p.* = 0;
    return 0;
}

pub export fn lua_tointegerx(L: *State, idx: c_int, isnum: ?*c_int) c_int {
    var n: TValue = .{};
    if (vmu.tonumber(index2value(L, idx), &n)) |v| {
        if (isnum) |p| p.* = 1;
        return o.num2int(o.nvalue(v));
    }
    if (isnum) |p| p.* = 0;
    return 0;
}

pub export fn lua_tounsignedx(L: *State, idx: c_int, isnum: ?*c_int) c_uint {
    var n: TValue = .{};
    if (vmu.tonumber(index2value(L, idx), &n)) |v| {
        if (isnum) |p| p.* = 1;
        return o.num2unsigned(o.nvalue(v));
    }
    if (isnum) |p| p.* = 0;
    return 0;
}

pub export fn lua_tointeger64(L: *State, idx: c_int, isinteger: ?*c_int) i64 {
    const v = index2value(L, idx);
    if (o.ttisinteger(v)) {
        if (isinteger) |p| p.* = 1;
        return o.lvalue(v);
    }
    if (isinteger) |p| p.* = 0;
    return 0;
}

pub export fn lua_tovector(L: *State, idx: c_int) ?*const f32 {
    const v = index2value(L, idx);
    if (!o.ttisvector(v)) return null;
    // x, y live in the value union and z in extra[0], which are contiguous in TValue
    return @ptrCast(&v.value.v[0]);
}

pub export fn lua_toboolean(L: *State, idx: c_int) c_int {
    return @intFromBool(!o.l_isfalse(index2value(L, idx)));
}

pub export fn lua_tolstring(L: *State, idx: c_int, len: ?*usize) ?[*:0]const u8 {
    var v = index2value(L, idx);
    if (!o.ttisstring(v)) {
        if (!vmu.tostringInPlace(L, v)) {
            if (len) |p| p.* = 0;
            return null;
        }
        gc.checkGC(L);
        v = index2value(L, idx);
    }
    if (len) |p| p.* = o.tsvalue(v).len;
    return o.tsvalue(v).cstr();
}

pub export fn lua_tostringatom(L: *State, idx: c_int, atom: ?*c_int) ?[*:0]const u8 {
    const v = index2value(L, idx);
    if (!o.ttisstring(v)) return null;
    const s = o.tsvalue(v);
    if (atom) |a| {
        lstring.updateatom(L, s);
        a.* = s.atom;
    }
    return s.cstr();
}

pub export fn lua_tolstringatom(L: *State, idx: c_int, len: ?*usize, atom: ?*c_int) ?[*:0]const u8 {
    const v = index2value(L, idx);
    if (!o.ttisstring(v)) {
        if (len) |p| p.* = 0;
        return null;
    }
    const s = o.tsvalue(v);
    if (len) |p| p.* = s.len;
    if (atom) |a| {
        lstring.updateatom(L, s);
        a.* = s.atom;
    }
    return s.cstr();
}

pub export fn lua_namecallatom(L: *State, atom: ?*c_int) ?[*:0]const u8 {
    const s = L.namecall orelse return null;
    if (atom) |a| {
        lstring.updateatom(L, s);
        a.* = s.atom;
    }
    return s.cstr();
}

pub export fn lua_objlen(L: *State, idx: c_int) c_int {
    const v = index2value(L, idx);
    return switch (o.ttype(v)) {
        o.T_STRING => @intCast(o.tsvalue(v).len),
        o.T_USERDATA => o.uvalue(v).len,
        o.T_BUFFER => @intCast(o.bufvalue(v).len),
        o.T_TABLE => table.getn(o.hvalue(v)),
        else => 0,
    };
}

pub export fn lua_tocfunction(L: *State, idx: c_int) ?o.lua_CFunction {
    const v = index2value(L, idx);
    if (!o.ttisfunction(v) or o.clvalue(v).isC == 0) return null;
    return o.clvalue(v).f;
}

pub export fn lua_tolightuserdata(L: *State, idx: c_int) ?*anyopaque {
    const v = index2value(L, idx);
    return if (o.ttislightuserdata(v)) o.pvalue(v) else null;
}

pub export fn lua_tolightuserdatatagged(L: *State, idx: c_int, tag: c_int) ?*anyopaque {
    const v = index2value(L, idx);
    return if (o.ttislightuserdata(v) and o.lightuserdatatag(v) == tag) o.pvalue(v) else null;
}

pub export fn lua_touserdata(L: *State, idx: c_int) ?*anyopaque {
    const v = index2value(L, idx);
    if (o.ttisuserdata(v)) return o.uvalue(v).data();
    if (o.ttislightuserdata(v)) return o.pvalue(v);
    return null;
}

pub export fn lua_touserdatatagged(L: *State, idx: c_int, tag: c_int) ?*anyopaque {
    const v = index2value(L, idx);
    return if (o.ttisuserdata(v) and o.uvalue(v).tag == tag) o.uvalue(v).data() else null;
}

pub export fn lua_userdatatag(L: *State, idx: c_int) c_int {
    const v = index2value(L, idx);
    return if (o.ttisuserdata(v)) o.uvalue(v).tag else -1;
}

pub export fn lua_lightuserdatatag(L: *State, idx: c_int) c_int {
    const v = index2value(L, idx);
    return if (o.ttislightuserdata(v)) o.lightuserdatatag(v) else -1;
}

pub export fn lua_tothread(L: *State, idx: c_int) ?*State {
    const v = index2value(L, idx);
    return if (o.ttisthread(v)) o.thvalue(v) else null;
}

pub export fn lua_tobuffer(L: *State, idx: c_int, len: ?*usize) ?*anyopaque {
    const v = index2value(L, idx);
    if (!o.ttisbuffer(v)) return null;
    const b = o.bufvalue(v);
    if (len) |p| p.* = b.len;
    return b.data();
}

pub export fn lua_topointer(L: *State, idx: c_int) ?*const anyopaque {
    const v = index2value(L, idx);
    return switch (o.ttype(v)) {
        o.T_USERDATA => o.uvalue(v).data(),
        o.T_LIGHTUSERDATA => o.pvalue(v),
        else => if (o.iscollectable(v)) @ptrCast(o.gcvalue(v)) else null,
    };
}

// ---------------------------------------------------------------------------
// Push functions (C -> stack)
// ---------------------------------------------------------------------------

pub export fn lua_pushnil(L: *State) void {
    o.setnilvalue(pushslot(L));
}

pub export fn lua_pushnumber(L: *State, n: f64) void {
    o.setnvalue(pushslot(L), n);
}

pub export fn lua_pushinteger(L: *State, n: c_int) void {
    o.setnvalue(pushslot(L), @floatFromInt(n));
}

pub export fn lua_pushinteger64(L: *State, n: i64) void {
    o.setlvalue(pushslot(L), n);
}

pub export fn lua_pushunsigned(L: *State, u: c_uint) void {
    o.setnvalue(pushslot(L), @floatFromInt(u));
}

pub export fn lua_pushvector(L: *State, x: f32, y: f32, z: f32) void {
    o.setvvalue(pushslot(L), x, y, z);
}

pub export fn lua_pushlstring(L: *State, s: ?[*]const u8, len: usize) void {
    gc.checkGC(L);
    ensureStack(L, 1);
    const str = lstring.new(L, if (len == 0) "" else s.?[0..len]);
    o.setsvalue(&L.top[0], str);
    L.top += 1;
}

pub export fn lua_pushstring(L: *State, s: ?[*:0]const u8) void {
    if (s) |str| lua_pushlstring(L, str, std.mem.len(str)) else lua_pushnil(L);
}

pub export fn lua_pushvfstring(L: *State, fmt: [*:0]const u8, ap: printf.RawVaList) [*:0]const u8 {
    var buf: [o.LUA_BUFFERSIZE]u8 = undefined;
    var out: printf.Out = .{ .buf = buf[0 .. o.LUA_BUFFERSIZE - 1] };
    var reader: printf.VaReader = .{ .ap = ap };
    printf.vformat(&out, fmt, &reader);
    gc.checkGC(L);
    pushstringRaw(L, out.slice());
    return o.tsvalue(&(L.top - 1)[0]).cstr();
}

fn pushfstringImpl(L: *State, fmt: [*:0]const u8, ap: printf.RawVaList) callconv(.c) [*:0]const u8 {
    return lua_pushvfstring(L, fmt, ap);
}

comptime {
    exportVariadic2(*State, [*:0]const u8, "lua_pushfstringL", pushfstringImpl);
}

/// Emits a C-variadic symbol `name(a0, fmt, ...)` that forwards to `impl` with a
/// platform `va_list`.
pub fn exportVariadic2(comptime A0: type, comptime R: type, comptime name: []const u8, comptime impl: fn (A0, [*:0]const u8, printf.RawVaList) callconv(.c) R) void {
    const T = printf.VariadicThunk2(A0, R, impl);
    @export(&T.entry, .{ .name = name });
}

pub export fn lua_pushcclosurek(L: *State, f: o.lua_CFunction, debugname: ?[*:0]const u8, nup: c_int, cont: ?o.lua_Continuation) void {
    gc.checkGC(L);
    ensureStack(L, 1);
    const n: usize = @intCast(nup);
    const cl = func.newCclosure(L, n, getcurrenv(L));
    cl.f = f;
    cl.cont = cont;
    cl.debugname = if (debugname) |d| lstring.newz(L, d) else null;
    L.top -= n;
    const up = cl.upvals();
    for (0..n) |i| up[i] = L.top[i];
    o.setclvalue(&L.top[0], cl);
    L.top += 1;
}

pub export fn lua_pushboolean(L: *State, b: c_int) void {
    o.setbvalue(pushslot(L), b != 0);
}

pub export fn lua_pushthread(L: *State) c_int {
    o.setthvalue(pushslot(L), L);
    return @intFromBool(L.global.mainthread == L);
}

pub export fn lua_pushlightuserdatatagged(L: *State, p: ?*anyopaque, tag: c_int) void {
    o.setpvalue(pushslot(L), p, tag);
}

fn newudata(L: *State, s: usize, tag: c_int) *o.Udata {
    if (s > std.math.maxInt(c_int) - @sizeOf(o.Udata)) mem.toobig(L);
    const u = mem.newGcoTagged(L, o.Udata, gc.udataSize(s), o.T_USERDATA, tag);
    u.len = @intCast(s);
    u.metatable = null;
    u.tag = @intCast(tag);
    u.pad = .{ 0, 0, 0 };
    return u;
}

pub export fn lua_newuserdatatagged(L: *State, sz: usize, tag: c_int) ?*anyopaque {
    gc.checkGC(L);
    ensureStack(L, 1);
    const u = newudata(L, sz, tag);
    o.setuvalue(&L.top[0], u);
    L.top += 1;
    return u.data();
}

pub export fn lua_newuserdatataggedwithmetatable(L: *State, sz: usize, tag: c_int) ?*anyopaque {
    gc.checkGC(L);
    ensureStack(L, 1);
    const u = newudata(L, sz, tag);
    u.metatable = L.global.udatamt[@intCast(tag)];
    o.setuvalue(&L.top[0], u);
    L.top += 1;
    return u.data();
}

pub export fn lua_newuserdatadtor(L: *State, sz: usize, dtor: o.lua_Destructor) ?*anyopaque {
    gc.checkGC(L);
    ensureStack(L, 1);
    const as = if (sz < std.math.maxInt(usize) - @sizeOf(usize)) sz + @sizeOf(usize) else std.math.maxInt(usize);
    const u = newudata(L, as, o.UTAG_IDTOR);
    const raw: usize = @intFromPtr(dtor);
    @memcpy(u.data()[sz .. sz + @sizeOf(usize)], std.mem.asBytes(&raw));
    o.setuvalue(&L.top[0], u);
    L.top += 1;
    return u.data();
}

pub export fn lua_newbuffer(L: *State, sz: usize) ?*anyopaque {
    gc.checkGC(L);
    ensureStack(L, 1);
    if (sz > o.MAX_BUFFER_SIZE) mem.toobig(L);
    const b = mem.newGcoBuffer(L, o.Buffer, o.Buffer.header + sz);
    b.len = @intCast(sz);
    @memset(b.data()[0..sz], 0);
    o.setbufvalue(&L.top[0], b);
    L.top += 1;
    return b.data();
}

// ---------------------------------------------------------------------------
// Get functions (Lua -> stack)
// ---------------------------------------------------------------------------

pub export fn lua_gettable(L: *State, idx: c_int) c_int {
    const t = index2value(L, idx);
    vmu.gettable(L, t, &(L.top - 1)[0], L.top - 1);
    return (L.top - 1)[0].tt;
}

pub export fn lua_getfield(L: *State, idx: c_int, k: [*:0]const u8) c_int {
    ensureStack(L, 1);
    const t = index2value(L, idx).*;
    var key: TValue = .{};
    o.setsvalue(&key, lstring.newz(L, k));
    const slot = L.top;
    L.top += 1;
    o.setnilvalue(&slot[0]);
    vmu.gettable(L, &t, &key, L.top - 1);
    return (L.top - 1)[0].tt;
}

pub export fn lua_rawgetfield(L: *State, idx: c_int, k: [*:0]const u8) c_int {
    ensureStack(L, 1);
    const t = index2value(L, idx);
    const key = lstring.newz(L, k);
    const v = table.getstr(o.hvalue(t), key).*;
    L.top[0] = v;
    L.top += 1;
    return v.tt;
}

pub export fn lua_rawget(L: *State, idx: c_int) c_int {
    const t = index2value(L, idx);
    const top = &(L.top - 1)[0];
    top.* = table.get(o.hvalue(t), top).*;
    return top.tt;
}

pub export fn lua_rawgeti(L: *State, idx: c_int, n: c_int) c_int {
    ensureStack(L, 1);
    const t = index2value(L, idx);
    const v = table.getnum(o.hvalue(t), n).*;
    L.top[0] = v;
    L.top += 1;
    return v.tt;
}

pub export fn lua_rawgetptagged(L: *State, idx: c_int, p: ?*anyopaque, tag: c_int) c_int {
    ensureStack(L, 1);
    const t = index2value(L, idx);
    const v = table.getp(o.hvalue(t), p, tag).*;
    L.top[0] = v;
    L.top += 1;
    return v.tt;
}

pub export fn lua_createtable(L: *State, narray: c_int, nrec: c_int) void {
    gc.checkGC(L);
    ensureStack(L, 1);
    o.sethvalue(&L.top[0], table.new(L, narray, nrec));
    L.top += 1;
}

pub export fn lua_setreadonly(L: *State, idx: c_int, enabled: c_int) void {
    const v = index2value(L, idx);
    table.setreadonly(o.hvalue(v), enabled != 0);
}

pub export fn lua_getreadonly(L: *State, idx: c_int) c_int {
    return @intFromBool(o.hvalue(index2value(L, idx)).readonly != 0);
}

pub export fn lua_setsafeenv(L: *State, idx: c_int, enabled: c_int) void {
    o.hvalue(index2value(L, idx)).safeenv = @intFromBool(enabled != 0);
}

pub export fn lua_getmetatable(L: *State, objindex: c_int) c_int {
    ensureStack(L, 1);
    const v = index2value(L, objindex);
    // upstream: classes report the per-type metatable, not their constructor metatable
    const mt = (if (o.ttisclass(v)) L.global.mt[o.T_CLASS] else tm.metatableOf(L, v)) orelse return 0;
    o.sethvalue(&L.top[0], mt);
    L.top += 1;
    return 1;
}

pub export fn lua_getfenv(L: *State, idx: c_int) void {
    ensureStack(L, 1);
    const v = index2value(L, idx);
    switch (o.ttype(v)) {
        o.T_FUNCTION => o.sethvalue(&L.top[0], o.clvalue(v).env),
        o.T_THREAD => o.sethvalue(&L.top[0], o.thvalue(v).gt),
        else => o.setnilvalue(&L.top[0]),
    }
    L.top += 1;
}

// ---------------------------------------------------------------------------
// Set functions (stack -> Lua)
// ---------------------------------------------------------------------------

pub export fn lua_settable(L: *State, idx: c_int) void {
    const t = index2value(L, idx);
    vmu.settable(L, t, &(L.top - 2)[0], &(L.top - 1)[0]);
    L.top -= 2;
}

pub export fn lua_setfield(L: *State, idx: c_int, k: [*:0]const u8) void {
    const t = index2value(L, idx).*;
    var key: TValue = .{};
    o.setsvalue(&key, lstring.newz(L, k));
    vmu.settable(L, &t, &key, &(L.top - 1)[0]);
    L.top -= 1;
}

pub export fn lua_rawsetfield(L: *State, idx: c_int, k: [*:0]const u8) void {
    const t = o.hvalue(index2value(L, idx));
    if (t.readonly != 0) debug.readonlyerror(L);
    const key = lstring.newz(L, k);
    table.setstr(L, t, key).* = (L.top - 1)[0];
    gc.barriert(L, t, &(L.top - 1)[0]);
    L.top -= 1;
}

pub export fn lua_rawset(L: *State, idx: c_int) void {
    const t = o.hvalue(index2value(L, idx));
    if (t.readonly != 0) debug.readonlyerror(L);
    table.set(L, t, &(L.top - 2)[0]).* = (L.top - 1)[0];
    gc.barriert(L, t, &(L.top - 1)[0]);
    L.top -= 2;
}

pub export fn lua_rawseti(L: *State, idx: c_int, n: c_int) void {
    const t = o.hvalue(index2value(L, idx));
    if (t.readonly != 0) debug.readonlyerror(L);
    table.setnum(L, t, n).* = (L.top - 1)[0];
    gc.barriert(L, t, &(L.top - 1)[0]);
    L.top -= 1;
}

pub export fn lua_rawsetptagged(L: *State, idx: c_int, p: ?*anyopaque, tag: c_int) void {
    const t = o.hvalue(index2value(L, idx));
    if (t.readonly != 0) debug.readonlyerror(L);
    table.setp(L, t, p, tag).* = (L.top - 1)[0];
    gc.barriert(L, t, &(L.top - 1)[0]);
    L.top -= 1;
}

pub export fn lua_setmetatable(L: *State, objindex: c_int) c_int {
    const v = index2value(L, objindex);
    const top = &(L.top - 1)[0];
    const mt: ?*o.Table = if (o.ttisnil(top)) null else o.hvalue(top);
    switch (o.ttype(v)) {
        o.T_TABLE => {
            if (o.hvalue(v).readonly != 0) debug.readonlyerror(L);
            o.hvalue(v).metatable = mt;
            if (mt) |m| gc.objbarrier(L, &o.hvalue(v).gch, &m.gch);
        },
        o.T_USERDATA => {
            o.uvalue(v).metatable = mt;
            if (mt) |m| gc.objbarrier(L, &o.uvalue(v).gch, &m.gch);
        },
        else => L.global.mt[o.ttype(v)] = mt,
    }
    L.top -= 1;
    return 1;
}

pub export fn lua_setfenv(L: *State, idx: c_int) c_int {
    const v = index2value(L, idx);
    const env = o.hvalue(&(L.top - 1)[0]);
    var res: c_int = 1;
    switch (o.ttype(v)) {
        o.T_FUNCTION => o.clvalue(v).env = env,
        o.T_THREAD => o.thvalue(v).gt = env,
        else => res = 0,
    }
    if (res != 0) gc.objbarrier(L, v.value.gc, &env.gch);
    L.top -= 1;
    return res;
}

// ---------------------------------------------------------------------------
// Load and call
// ---------------------------------------------------------------------------

pub export fn luau_load(L: *State, chunkname: [*:0]const u8, data: [*]const u8, size: usize, env: c_int) c_int {
    return load.load(L, chunkname, data[0..size], env);
}

pub export fn lua_call(L: *State, nargs: c_int, nresults: c_int) void {
    if (nresults > nargs + 1) ensureStack(L, nresults - (nargs + 1));
    const f = L.top - @as(usize, @intCast(nargs + 1));
    ldo.call(L, f, nresults);
    adjustresults(L, nresults);
}

const CallS = struct { func: StkId, nresults: c_int };
fn fcall(L: *State, ud: ?*anyopaque) void {
    const c: *CallS = @ptrCast(@alignCast(ud.?));
    ldo.call(L, c.func, c.nresults);
}

pub export fn lua_pcall(L: *State, nargs: c_int, nresults: c_int, errfunc: c_int) c_int {
    if (nresults > nargs + 1) ensureStack(L, nresults - (nargs + 1));
    var ef: isize = 0;
    if (errfunc != 0) ef = ldo.savestack(L, o.toStk(index2value(L, errfunc)));
    var c: CallS = .{ .func = L.top - @as(usize, @intCast(nargs + 1)), .nresults = nresults };
    const status = ldo.pcall(L, fcall, &c, ldo.savestack(L, c.func), ef);
    adjustresults(L, nresults);
    return status;
}

const CCallS = struct { func: o.lua_CFunction, ud: ?*anyopaque };
fn fccall(L: *State, ud: ?*anyopaque) void {
    const c: *CCallS = @ptrCast(@alignCast(ud.?));
    if (lua_checkstack(L, 2) == 0) debug.runerror(L, "stack limit", .{});
    lua_pushcclosurek(L, c.func, null, 0, null);
    lua_pushlightuserdatatagged(L, c.ud, 0);
    ldo.call(L, L.top - 2, 0);
}

pub export fn lua_cpcall(L: *State, f: o.lua_CFunction, ud: ?*anyopaque) c_int {
    var c: CCallS = .{ .func = f, .ud = ud };
    return ldo.pcall(L, fccall, &c, ldo.savestack(L, L.top), 0);
}

pub export fn lua_callyieldable(L: *State, nargs: c_int, nresults: c_int) c_int {
    const cl = o.clvalue(&L.ci.func[0]);
    lua_call(L, nargs, nresults);
    if (ldo.isyielded(L)) return ldo.C_CALL_YIELD;
    return cl.cont.?(L, o.LUA_OK);
}

const YCall = struct { func: StkId, nresults: c_int };
fn ycallBody(L: *State, ud: ?*anyopaque) void {
    const c: *YCall = @ptrCast(@alignCast(ud.?));
    ldo.callint(L, c.func, c.nresults, ldo.isyieldable(L) != 0);
}

pub export fn lua_pcallyieldable(L: *State, nargs: c_int, nresults: c_int, errfunc: c_int) c_int {
    const cl = o.clvalue(&L.ci.func[0]);
    L.ci.errfunc = errfunc;
    L.ci.flags |= o.CALLINFO_HANDLE;
    var c: YCall = .{ .func = L.top - @as(usize, @intCast(nargs + 1)), .nresults = nresults };
    const savedfunc = ldo.savestack(L, c.func);
    const savederrfunc: isize = if (errfunc != 0) ldo.savestack(L, L.base + @as(usize, @intCast(errfunc - 1))) else 0;
    const status = ldo.pcall(L, ycallBody, &c, savedfunc, savederrfunc);
    ldo.expandstacklimit(L, L.top);
    if (status == 0 and ldo.isyielded(L)) return ldo.C_CALL_YIELD;
    L.ci.flags &= ~o.CALLINFO_HANDLE;
    return cl.cont.?(L, status);
}

// ---------------------------------------------------------------------------
// Coroutines
// ---------------------------------------------------------------------------

pub export fn lua_yield(L: *State, nresults: c_int) c_int {
    return ldo.yield(L, nresults);
}

pub export fn lua_break(L: *State) c_int {
    return ldo.breakThread(L);
}

pub export fn lua_resume(L: *State, from: ?*State, narg: c_int) c_int {
    return ldo.resumeThread(L, from, narg);
}

pub export fn lua_resumeerror(L: *State, from: ?*State) c_int {
    return ldo.resumeerror(L, from);
}

pub export fn lua_status(L: *State) c_int {
    return L.status;
}

pub export fn lua_isyieldable(L: *State) c_int {
    return ldo.isyieldable(L);
}

pub export fn lua_getthreaddata(L: *State) ?*anyopaque {
    return L.userdata;
}

pub export fn lua_setthreaddata(L: *State, data: ?*anyopaque) void {
    L.userdata = data;
}

pub export fn lua_costatus(L: *State, co: *State) c_int {
    if (co == L) return o.LUA_CORUN;
    if (co.status == o.LUA_YIELD) return o.LUA_COSUS;
    if (co.status == o.LUA_BREAK) return o.LUA_CONOR;
    if (co.status != 0) return o.LUA_COERR;
    if (co.ci != &co.base_ci[0]) return o.LUA_CONOR;
    if (co.top == co.base) return o.LUA_COFIN;
    return o.LUA_COSUS;
}

pub export fn lua_hasfinalizers(L: *State) c_int {
    return @intFromBool(L.finalizers != null);
}

fn runfinalizery(Lopt: ?*State) callconv(.c) c_int {
    const L = Lopt.?;
    const co = lua_tothread(L, o.LUA_GLOBALSINDEX - 1) orelse debug.runerror(L, "finalizer function has already been called", .{});
    const result = lua_tointegerx(L, o.LUA_GLOBALSINDEX - 2, null);
    if (co == L) debug.runerror(L, "cannot finalize running thread", .{});
    if (co.finalizers == null) debug.runerror(L, "coroutine has no pending finalizers", .{});
    lua_settop(L, 0);
    lua_pushvalue(L, o.LUA_GLOBALSINDEX - 1);
    o.setnilvalue(index2value(L, o.LUA_GLOBALSINDEX - 1));
    if (result == ldo.FinallyCancelled) {
        ldo.preparefinalizestate(L, co, result);
        state.resetthread(co);
    } else {
        ldo.preparefinalize(L, co, result);
    }
    return ldo.runfinalizers(L, false);
}

fn runfinalizercont(Lopt: ?*State, status: c_int) callconv(.c) c_int {
    const L = Lopt.?;
    if (status != o.LUA_OK) ldo.throw(L, status);
    return ldo.runfinalizers(L, false);
}

pub export fn lua_pushfinalizerfunction(L: *State, co: *State, toclose: c_int) void {
    ensureStack(L, 1);
    o.setthvalue(&L.top[0], co);
    L.top += 1;
    const r: c_int = if (toclose != 0) ldo.FinallyCancelled else if (co.status == o.LUA_OK) ldo.FinallyFinished else ldo.FinallyError;
    lua_pushinteger(L, r);
    lua_pushcclosurek(L, runfinalizery, "finalize", 2, runfinalizercont);
}

pub export fn lua_addfinalizer(L: *State, co: *State, idx: c_int) void {
    if (co == L.global.mainthread) debug.runerror(L, "cannot register a finalizer on the main thread", .{});
    const st = lua_costatus(L, co);
    if (st == o.LUA_COFIN or st == o.LUA_COERR) debug.runerror(L, "cannot register a finalizer on dead coroutine", .{});
    if (L.global.cb.userfinalizer) |cb| cb(L, co);
    if (co.finalizers == null) {
        co.finalizers = table.new(L, 1, 0);
        gc.objbarrier(L, &co.gch, &co.finalizers.?.gch);
    }
    const fins = co.finalizers.?;
    const v = index2value(L, idx).*;
    table.setnum(L, fins, table.getn(fins) + 1).* = v;
    gc.barriert(L, fins, &v);
}

// ---------------------------------------------------------------------------
// Garbage collection
// ---------------------------------------------------------------------------

pub export fn lua_gc(L: *State, what: c_int, data: c_int) c_int {
    const g = L.global;
    switch (what) {
        o.LUA_GCSTOP => g.GCthreshold = std.math.maxInt(usize),
        o.LUA_GCRESTART => g.GCthreshold = g.totalbytes,
        o.LUA_GCCOLLECT => gc.fullgc(L),
        o.LUA_GCCOUNT => return @intCast(g.totalbytes >> 10),
        o.LUA_GCCOUNTB => return @intCast(g.totalbytes & 1023),
        o.LUA_GCISRUNNING => return @intFromBool(g.GCthreshold != std.math.maxInt(usize)),
        o.LUA_GCSTEP => {
            // Upstream converts the signed argument to size_t before shifting.
            // Negative values therefore wrap to a large work budget, rather than
            // being treated as a zero-sized step.
            const amount = @as(usize, @bitCast(@as(isize, data))) << 10;
            const oldcredit: isize = if (g.gcstate == gc.GCSpause) 0 else @as(isize, @bitCast(g.GCthreshold -% g.totalbytes));
            g.GCthreshold = if (amount <= g.totalbytes) g.totalbytes - amount else 0;
            var actualwork: usize = 0;
            var res: c_int = 0;
            while (g.GCthreshold <= g.totalbytes) {
                const before = g.gcstate;
                actualwork += gc.step(L, false);
                if (g.gcstate == gc.GCSpause) {
                    res = 1;
                    break;
                }
                if (g.gcinprogress and before == g.gcstate) break; // re-entered from a finalizer
            }
            if (g.gcstate != gc.GCSpause) {
                const newthreshold: isize = @as(isize, @intCast(g.totalbytes + actualwork)) + oldcredit;
                g.GCthreshold = if (newthreshold < 0) 0 else @intCast(newthreshold);
            }
            return res;
        },
        o.LUA_GCSETGOAL => {
            const r = g.gcgoal;
            g.gcgoal = data;
            return r;
        },
        o.LUA_GCSETSTEPMUL => {
            const r = g.gcstepmul;
            g.gcstepmul = data;
            return r;
        },
        o.LUA_GCSETSTEPSIZE => {
            const r = g.gcstepsize >> 10;
            g.gcstepsize = data << 10;
            return r;
        },
        o.LUA_GCISPAUSED => return @intFromBool(g.gcstate == gc.GCSpause),
        else => return -1,
    }
    return 0;
}

/// JSON heap dump to a host C `FILE*` (vm/memdump.zig). Writes through libc stdio in
/// libc-linked (C-ABI bridge) builds; raises a Lua error in the libc-free build.
pub export fn lua_memorydump(L: *State, file: ?*anyopaque, categoryName: ?@import("memdump.zig").CategoryName) void {
    @import("memdump.zig").dump(L, file, categoryName);
}

pub export fn lua_setmemcat(L: *State, category: c_int) void {
    L.activememcat = @intCast(category);
}

pub export fn lua_totalbytes(L: *State, category: c_int) usize {
    return if (category < 0) L.global.totalbytes else L.global.memcatbytes[@intCast(category)];
}

pub export fn lua_allocationrate(L: *State) i64 {
    return gc.allocationrate(L);
}

// ---------------------------------------------------------------------------
// Miscellaneous
// ---------------------------------------------------------------------------

pub export fn lua_error(L: *State) noreturn {
    ldo.throw(L, o.LUA_ERRRUN);
}

pub export fn lua_next(L: *State, idx: c_int) c_int {
    ensureStack(L, 1);
    const t = o.hvalue(index2value(L, idx));
    if (table.next(L, t, L.top - 1)) {
        L.top += 1;
        return 1;
    }
    L.top -= 1;
    return 0;
}

pub export fn lua_rawiter(L: *State, idx: c_int, iter_in: c_int) c_int {
    ensureStack(L, 2);
    const h = o.hvalue(index2value(L, idx));
    var iter = iter_in;
    const sizearray = h.sizearray;
    while (@as(c_uint, @bitCast(iter)) < @as(c_uint, @bitCast(sizearray))) : (iter += 1) {
        const e = &h.arr()[@intCast(iter)];
        if (!o.ttisnil(e)) {
            o.setnvalue(&L.top[0], @floatFromInt(iter + 1));
            L.top[1] = e.*;
            L.top += 2;
            return iter + 1;
        }
    }
    const sizenode = h.sizenode();
    while (@as(c_uint, @bitCast(iter - sizearray)) < @as(c_uint, @bitCast(sizenode))) : (iter += 1) {
        const n = &h.node[@intCast(iter - sizearray)];
        if (!o.ttisnil(&n.val)) {
            o.getnodekey(&L.top[0], n);
            L.top[1] = n.val;
            L.top += 2;
            return iter + 1;
        }
    }
    return -1;
}

pub export fn lua_concat(L: *State, n: c_int) void {
    if (n >= 2) {
        gc.checkGC(L);
        vmu.concat(L, n, o.idiff(L.top, L.base) - 1);
        L.top -= @as(usize, @intCast(n - 1));
    } else if (n == 0) {
        ensureStack(L, 1);
        o.setsvalue(&L.top[0], lstring.new(L, ""));
        L.top += 1;
    }
}

pub export fn lua_setpointerencodekey(L: *State, a: u64, b: u64, c: u64, d: u64) void {
    const g = L.global;
    // two keys have historically made the encoding an identity
    const identity = (a == 1 and b == 0 and c == 0 and d == 0) or (a == 0 and b == 1 and c == 0 and d == 0);
    g.ptrencactive = !identity;
    // derive a 512-bit key from the provided seed
    for (0..4) |i| {
        g.ptrenckeynew[2 * i + 0] = siphash24(a, b, @intCast(i));
        g.ptrenckeynew[2 * i + 1] = siphash24(c, d, @intCast(i));
    }
}

// SipHash-2-4 round exactly as upstream lapi.cpp `siphashround`.
fn siphashRound(v: *[4]u64) void {
    v[0] +%= v[1];
    v[1] = std.math.rotl(u64, v[1], 13);
    v[1] ^= v[0];
    v[0] = std.math.rotl(u64, v[0], 32);
    v[2] +%= v[3];
    v[3] = std.math.rotl(u64, v[3], 16);
    v[3] ^= v[2];
    v[2] +%= v[1];
    v[1] = std.math.rotl(u64, v[1], 17);
    v[1] ^= v[2];
    v[2] = std.math.rotl(u64, v[2], 32);
    v[0] +%= v[3];
    v[3] = std.math.rotl(u64, v[3], 21);
    v[3] ^= v[0];
}

fn siphash24(k0: u64, k1: u64, m: u32) u64 {
    var v = [4]u64{ k0 ^ 0x736f6d6570736575, k1 ^ 0x646f72616e646f6d, k0 ^ 0x6c7967656e657261, k1 ^ 0x7465646279746573 };
    const m0: u64 = (@as(u64, 4) << 56) | m;
    v[3] ^= m0;
    siphashRound(&v);
    siphashRound(&v);
    v[0] ^= m0;
    v[2] ^= 0xff;
    siphashRound(&v);
    siphashRound(&v);
    siphashRound(&v);
    siphashRound(&v);
    return v[0] ^ v[1] ^ v[2] ^ v[3];
}

/// Feistel-network pointer encoding keyed by lua_setpointerencodekey (identity when
/// no key was set, as upstream).
pub export fn lua_encodepointer(L: *State, p: usize) usize {
    const g = L.global;
    if (!g.ptrencactive) return p;
    const halfbits = @bitSizeOf(usize) / 2;
    const mask: usize = (@as(usize, 1) << halfbits) - 1;
    var l0 = p & mask;
    var r0 = p >> halfbits;
    for (0..4) |i| {
        const f: usize = @truncate(siphash24(g.ptrenckeynew[i * 2], g.ptrenckeynew[i * 2 + 1], @truncate(r0)) & mask);
        const rnext = l0 ^ f;
        l0 = r0;
        r0 = rnext;
    }
    return (r0 << halfbits) | l0;
}

extern "kernel32" fn QueryPerformanceCounter(count: *i64) callconv(.winapi) c_int;
extern "kernel32" fn QueryPerformanceFrequency(freq: *i64) callconv(.winapi) c_int;

pub export fn lua_clock() f64 {
    if (builtin.os.tag == .windows) {
        var c: i64 = 0;
        var f: i64 = 1;
        _ = QueryPerformanceFrequency(&f);
        _ = QueryPerformanceCounter(&c);
        return @as(f64, @floatFromInt(c)) / @as(f64, @floatFromInt(f));
    } else if (builtin.os.tag == .linux) {
        var ts: std.os.linux.timespec = undefined;
        _ = std.os.linux.clock_gettime(.MONOTONIC, &ts);
        return @as(f64, @floatFromInt(ts.sec)) + @as(f64, @floatFromInt(ts.nsec)) * 1e-9;
    } else if (builtin.os.tag.isDarwin()) {
        // Darwin has no stable syscall ABI: libSystem is the only supported OS
        // interface, and every Zig executable for Darwin links it. This is the one
        // place where the VM calls into libSystem.
        var ts: std.c.timespec = undefined;
        _ = std.c.clock_gettime(.MONOTONIC, &ts);
        return @as(f64, @floatFromInt(ts.sec)) + @as(f64, @floatFromInt(ts.nsec)) * 1e-9;
    } else {
        @compileError("lua_clock: no monotonic clock implementation for this OS");
    }
}

pub export fn lua_setuserdatatag(L: *State, idx: c_int, tag: c_int) void {
    o.uvalue(index2value(L, idx)).tag = @intCast(tag);
}

pub export fn lua_setuserdatadtor(L: *State, tag: c_int, dtor: ?o.lua_Destructor) void {
    L.global.udatagc[@intCast(tag)] = dtor;
}

pub export fn lua_getuserdatadtor(L: *State, tag: c_int) ?o.lua_Destructor {
    return L.global.udatagc[@intCast(tag)];
}

/// Embedder GC integration (upstream LuauGcTraceUdata). The userdata mark callback
/// is invoked when GC traces a tagged userdata while the flag is enabled; the
/// embedder GC callback and the weak registry are honoured whenever they are set.
pub export fn lua_setuserdatamark(L: *State, tag: c_int, markfn: ?o.lua_UserdataMark) void {
    L.global.udatamark[@intCast(tag)] = markfn;
}

pub export fn lua_setembeddergc(L: *State, f: ?o.lua_EmbedderGc) void {
    L.global.embeddergc = f;
}

fn weakregistry(L: *State) *o.Table {
    const g = L.global;
    if (!o.ttistable(&g.weakregistry)) {
        const wt = table.new(L, 0, 0);
        o.sethvalue(&g.weakregistry, wt);
        const mt = table.new(L, 0, 1);
        o.setsvalue(table.setstr(L, mt, g.tmname[@intFromEnum(o.TMS.MODE)]), lstring.new(L, "v"));
        wt.metatable = mt;
    }
    return o.hvalue(&g.weakregistry);
}

pub export fn lua_weakref(L: *State, idx: c_int) c_int {
    const g = L.global;
    const p = index2value(L, idx);
    if (o.ttisnil(p)) return o.LUA_REFNIL;
    const v = p.*;
    const wr = weakregistry(L);
    var ref: c_int = undefined;
    var slot: *TValue = undefined;
    if (g.weakregistryfree != 0) {
        ref = g.weakregistryfree;
        slot = table.setnum(L, wr, ref);
        g.weakregistryfree = @intFromFloat(o.nvalue(slot));
    } else {
        g.weakregistrytop += 1;
        ref = g.weakregistrytop;
        slot = table.setnum(L, wr, ref);
    }
    slot.* = v;
    gc.barriert(L, wr, &v);
    return ref;
}

pub export fn lua_weakunref(L: *State, ref: c_int) c_int {
    if (ref <= o.LUA_REFNIL) return o.LUA_NOREF;
    const g = L.global;
    const slot = table.setnum(L, weakregistry(L), ref);
    o.setnvalue(slot, @floatFromInt(g.weakregistryfree));
    g.weakregistryfree = ref;
    return o.LUA_NOREF;
}

pub export fn lua_getweakref(L: *State, ref: c_int) c_int {
    ensureStack(L, 1);
    const v = table.getnum(weakregistry(L), ref).*;
    L.top[0] = v;
    L.top += 1;
    return v.tt;
}

pub export fn lua_setuserdatametatable(L: *State, tag: c_int) void {
    L.global.udatamt[@intCast(tag)] = o.hvalue(&(L.top - 1)[0]);
    L.top -= 1;
}

pub export fn lua_getuserdatametatable(L: *State, tag: c_int) void {
    ensureStack(L, 1);
    if (L.global.udatamt[@intCast(tag)]) |h| o.sethvalue(&L.top[0], h) else o.setnilvalue(&L.top[0]);
    L.top += 1;
}

pub export fn lua_getuserdataname(L: *State, tag: c_int) [*:0]const u8 {
    if (L.global.udatamt[@intCast(tag)]) |mt| {
        const t = table.getstr(mt, L.global.tmname[@intFromEnum(o.TMS.TYPE)]);
        if (o.ttisstring(t)) return o.tsvalue(t).cstr();
    }
    return "userdata";
}

/// lua_registeruserdatadirectaccess (experimental upstream API): snapshots the
/// __index/__newindex/__namecall metamethods of the tag's registered metatable and
/// routes GETUDATAKS/SETUDATAKS/NAMECALLUDATA through the callbacks. Returns 0 when
/// the tag has no metatable (lua_setuserdatametatable).
pub export fn lua_registeruserdatadirectaccess(L: *State, tag: c_int, get: ?o.lua_UserdataDirectAccess, set: ?o.lua_UserdataDirectAccess, namecall: ?o.lua_UserdataDirectNamecall) c_int {
    gc.threadbarrier(L);
    const t: usize = @intCast(tag);
    const h = L.global.udatamt[t] orelse return 0;
    const d = &L.global.udatadirect[t];
    if (tm.fasttm(L, h, .INDEX)) |v| {
        d.indextm = v.*;
        d.index = get;
    }
    if (tm.fasttm(L, h, .NEWINDEX)) |v| {
        d.newindextm = v.*;
        d.newindex = set;
    }
    if (tm.fasttm(L, h, .NAMECALL)) |v| {
        d.namecalltm = v.*;
        d.namecall = namecall;
    }
    return 1;
}

pub const lua_UserdataDirectFieldGet = *const fn (?*anyopaque, ?*anyopaque) callconv(.c) void;

pub export fn lua_registeruserdatadirectfieldget(L: *State, tag: c_int, field: [*:0]const u8, f: lua_UserdataDirectFieldGet) void {
    const g = L.global;
    const t: usize = @intCast(tag);
    if (g.udatadirectfields[t] == null) g.udatadirectfields[t] = table.new(L, 0, 1);
    const ts = lstring.newz(L, field);
    lstring.fix(ts);
    o.setpvalue(table.setstr(L, g.udatadirectfields[t].?, ts), @ptrFromInt(@intFromPtr(f)), 0);
}

pub export fn lua_userdatadirectfield_setnumber(result: *TValue, n: f64) void {
    o.setnvalue(result, n);
}
pub export fn lua_userdatadirectfield_setvector(result: *TValue, x: f32, y: f32, z: f32) void {
    o.setvvalue(result, x, y, z);
}
pub export fn lua_userdatadirectfield_setboolean(result: *TValue, b: c_int) void {
    o.setbvalue(result, b != 0);
}
pub export fn lua_userdatadirectfield_setinteger64(result: *TValue, n: i64) void {
    o.setlvalue(result, n);
}
pub export fn lua_userdatadirectfield_setnil(result: *TValue) void {
    o.setnilvalue(result);
}

pub export fn lua_setlightuserdataname(L: *State, tag: c_int, name: [*:0]const u8) void {
    const t: usize = @intCast(tag);
    if (L.global.lightuserdataname[t] == null) {
        const s = lstring.newz(L, name);
        lstring.fix(s);
        L.global.lightuserdataname[t] = s;
    }
}

pub export fn lua_getlightuserdataname(L: *State, tag: c_int) ?[*:0]const u8 {
    return if (L.global.lightuserdataname[@intCast(tag)]) |s| s.cstr() else null;
}

pub export fn lua_clonefunction(L: *State, idx: c_int) void {
    gc.checkGC(L);
    ensureStack(L, 1);
    const cl = o.clvalue(index2value(L, idx));
    const ncl = func.newLclosure(L, cl.nupvalues, L.gt, cl.p.?);
    const src = o.clvalue(index2value(L, idx));
    for (0..src.nupvalues) |i| ncl.upvals()[i] = src.upvals()[i];
    o.setclvalue(&L.top[0], ncl);
    L.top += 1;
}

pub export fn lua_usesexport(L: *State, idx: c_int) c_int {
    const v = index2value(L, idx);
    if (!o.ttisfunction(v) or o.clvalue(v).isC != 0) return 0;
    return @intFromBool(o.clvalue(v).p.?.flags & LPF_USES_EXPORT != 0);
}

pub export fn lua_cleartable(L: *State, idx: c_int) void {
    const t = o.hvalue(index2value(L, idx));
    if (t.readonly != 0) debug.readonlyerror(L);
    table.clear(t);
}

pub export fn lua_clonetable(L: *State, idx: c_int) void {
    gc.checkGC(L);
    ensureStack(L, 1);
    const t = table.clone(L, o.hvalue(index2value(L, idx)));
    o.sethvalue(&L.top[0], t);
    L.top += 1;
}

pub export fn lua_getallocf(L: *State, ud: ?*?*anyopaque) o.lua_Alloc {
    if (ud) |p| p.* = L.global.ud;
    return L.global.frealloc;
}

pub export fn lua_ref(L: *State, idx: c_int) c_int {
    const g = L.global;
    const p = index2value(L, idx);
    if (o.ttisnil(p)) return o.LUA_REFNIL;
    const v = p.*;
    const reg = o.hvalue(&g.registry);
    const ref: c_int = if (g.registryfree != 0) g.registryfree else table.getn(reg) + 1;
    const slot = table.setnum(L, reg, ref);
    if (g.registryfree != 0) g.registryfree = @intFromFloat(o.nvalue(slot));
    slot.* = v;
    gc.barriert(L, reg, &v);
    return ref;
}

pub export fn lua_unref(L: *State, ref: c_int) c_int {
    if (ref <= o.LUA_REFNIL) return o.LUA_NOREF;
    const g = L.global;
    const slot = table.getnum(o.hvalue(&g.registry), ref);
    if (slot != o.nilobject()) {
        o.setnvalue(slot, @floatFromInt(g.registryfree));
        g.registryfree = ref;
    }
    return o.LUA_NOREF;
}

// ---------------------------------------------------------------------------
// Debug API
// ---------------------------------------------------------------------------

pub export fn lua_callhook(L: *State, hook: o.lua_Hook, userdata: ?*anyopaque) void {
    debug.callhook(L, hook, userdata);
}

pub export fn lua_stackdepth(L: *State) c_int {
    return debug.stackdepth(L);
}

pub export fn lua_getinfo(L: *State, level: c_int, what: [*:0]const u8, ar: *o.lua_Debug) c_int {
    return debug.getinfo(L, level, what, ar);
}

pub export fn lua_getargument(L: *State, level: c_int, n: c_int) c_int {
    return debug.getargument(L, level, n);
}

pub export fn lua_getlocal(L: *State, level: c_int, n: c_int) ?[*:0]const u8 {
    return debug.getlocal(L, level, n);
}

pub export fn lua_setlocal(L: *State, level: c_int, n: c_int) ?[*:0]const u8 {
    return debug.setlocal(L, level, n);
}

fn auxUpvalue(fi: *TValue, n: c_int, val: **TValue) ?[*:0]const u8 {
    if (!o.ttisfunction(fi)) return null;
    const f = o.clvalue(fi);
    if (f.isC != 0) {
        if (!(1 <= n and n <= f.nupvalues)) return null;
        val.* = &f.upvals()[@intCast(n - 1)];
        return "";
    }
    const p = f.p.?;
    if (!(1 <= n and n <= p.nups)) return null;
    const r = &f.upvals()[@intCast(n - 1)];
    val.* = if (o.ttisupval(r)) o.upvalue(r).v else r;
    if (!(1 <= n and n <= p.sizeupvalues)) return "";
    if (p.upvalues.?[@intCast(n - 1)]) |s| return s.cstr();
    return "";
}

pub export fn lua_getupvalue(L: *State, funcindex: c_int, n: c_int) ?[*:0]const u8 {
    ensureStack(L, 1);
    var val: *TValue = undefined;
    const name = auxUpvalue(index2value(L, funcindex), n, &val) orelse return null;
    L.top[0] = val.*;
    L.top += 1;
    return name;
}

pub export fn lua_setupvalue(L: *State, funcindex: c_int, n: c_int) ?[*:0]const u8 {
    var val: *TValue = undefined;
    const fi = index2value(L, funcindex);
    const name = auxUpvalue(fi, n, &val) orelse return null;
    L.top -= 1;
    val.* = L.top[0];
    gc.barrier(L, fi.value.gc, &L.top[0]);
    return name;
}

pub export fn lua_hascustomexecution(L: *State, level: c_int) c_int {
    _ = L;
    _ = level;
    return 0;
}

pub export fn lua_incustomexecution(L: *State, level: c_int) c_int {
    _ = L;
    _ = level;
    return 0;
}

pub export fn lua_singlestep(L: *State, enabled: c_int) void {
    L.singlestep = enabled != 0;
}

pub export fn lua_breakpoint(L: *State, funcindex: c_int, line: c_int, enabled: c_int) c_int {
    const f = o.clvalue(index2value(L, funcindex));
    return debug.setbreakpoint(L, f.p.?, line, enabled != 0);
}

pub export fn lua_atbreakpoint(L: *State) c_int {
    return @intFromBool(debug.onbreak(L));
}

pub export fn lua_debugtrace(L: *State) [*:0]const u8 {
    return debug.debugtrace(L);
}

pub export fn lua_getcoverage(L: *State, funcindex: c_int, context: ?*anyopaque, callback: debug.lua_Coverage) void {
    const f = o.clvalue(index2value(L, funcindex));
    debug.getcoverage(L, f.p.?, context, callback);
}

/// Counters are produced only by native code generation (ecb.getcounterdata); like
/// upstream without native code, there is nothing to report.
pub export fn lua_getcounters(L: *State, funcindex: c_int, context: ?*anyopaque, fv: ?*const anyopaque, cv: ?*const anyopaque) void {
    _ = L;
    _ = funcindex;
    _ = context;
    _ = fv;
    _ = cv;
}

pub export fn lua_callbacks(L: *State) *o.lua_Callbacks {
    return &L.global.cb;
}

/// Records the buffer cage allocator; buffers are allocated from it while the
/// LuauBufferCage flag is enabled (upstream behavior; the flag is off by default).
pub export fn lua_setmemorycage(L: *State, alloc: ?o.lua_CageAlloc, ud: ?*anyopaque) void {
    L.global.cagealloc = alloc;
    L.global.cageud = ud;
}

// ---------------------------------------------------------------------------
// VM internals exposed to the standard library (not part of lua.h)
// ---------------------------------------------------------------------------

pub export fn luau_tablesizes(L: *State, idx: c_int, sizearray: *c_int, sizenode: *c_int) void {
    const t = o.hvalue(index2value(L, idx));
    sizearray.* = t.sizearray;
    sizenode.* = if (table.dummy(t)) 0 else t.sizenode();
}

pub export fn luau_tableresizearray(L: *State, idx: c_int, n: c_int) void {
    table.resizearray(L, o.hvalue(index2value(L, idx)), n);
}

pub export fn luau_getsinglestep(L: *State) c_int {
    return @intFromBool(L.singlestep);
}

pub export fn luau_calldebuginterrupt(L: *State, co: *State) void {
    if (L.global.cb.debuginterrupt) |hook| debug.callhook(L, hook, co);
}

pub export fn luau_interruptnonyieldable(L: *State) void {
    if (L.global.cb.interrupt) |cb| {
        L.nCcalls += 1;
        cb(L, -1);
        L.nCcalls -= 1;
    }
}

/// Feature flags read by the VM (upstream FFlag defaults; production leaves them off).
pub const fflags = @import("flags.zig");
comptime {
    _ = fflags.luau_setfflag;
}

/// luaL_Strbuf support: pushes an uninterned string buffer of `size` bytes.
pub export fn luau_strbufstart(L: *State, size: usize) [*]u8 {
    ensureStack(L, 1);
    const ts = lstring.bufstart(L, size);
    o.setsvalue(&L.top[0], ts);
    L.top += 1;
    return ts.data();
}

/// Replaces the buffer at `idx` with the interned string of its first `used` bytes.
pub export fn luau_strbuffinish(L: *State, idx: c_int, used: usize) void {
    gc.checkGC(L);
    const slot = index2value(L, idx);
    const ts = o.tsvalue(slot);
    if (used == ts.len) {
        o.setsvalue(slot, lstring.buffinish(L, ts));
    } else {
        const s = lstring.new(L, ts.data()[0..used]);
        o.setsvalue(index2value(L, idx), s);
    }
}

pub export fn luau_tableswap(L: *State, idx: c_int, i: c_int, j: c_int) void {
    const a = o.hvalue(index2value(L, idx)).arr();
    const tmp = a[@intCast(i)];
    a[@intCast(i)] = a[@intCast(j)];
    a[@intCast(j)] = tmp;
}

pub export fn luau_tableisdummynode(L: *State, idx: c_int) bool {
    return table.dummy(o.hvalue(index2value(L, idx)));
}

/// luaH_rawfind: index of the first raw-equal value at or after `init`, 0 if absent.
pub export fn luau_tablerawfind(L: *State, tidx: c_int, init: c_int, vidx: c_int) c_int {
    const t = o.hvalue(index2value(L, tidx));
    const v = index2value(L, vidx);
    if (@as(c_uint, @bitCast(init -% 1)) >= @as(c_uint, @bitCast(t.sizearray))) {
        if (table.dummy(t) or init < 1) return 0;
        var i = init;
        while (true) : (i += 1) {
            const e = table.getnum(t, i);
            if (o.ttisnil(e)) return 0;
            if (table.rawequalObj(e, v)) return i;
        }
    }
    if (o.ttisnil(v)) return 0;
    var i: c_int = init - 1;
    while (i < t.sizearray) : (i += 1) {
        const e = &t.arr()[@intCast(i)];
        if (o.ttisnil(e)) break;
        if (table.rawequalObj(e, v)) return i + 1;
    }
    return 0;
}

pub export fn luau_haseqtm(L: *State, idx: c_int) bool {
    const v = index2value(L, idx);
    return o.iscollectable(v) and !o.ttisnil(tm.gettmbyobj(L, v, .EQ));
}

pub export fn luau_setbuiltinpcall(L: *State, idx: c_int, xpcall: bool) void {
    const cl = o.clvalue(index2value(L, idx));
    if (xpcall) L.global.builtinXpcall = cl else L.global.builtinPcall = cl;
}

pub export fn luau_preparefinalize(L: *State, co: *State, result: c_int) void {
    ldo.preparefinalize(L, co, result);
}

pub export fn luau_preparefinalizestate(L: *State, co: *State, result: c_int) void {
    ldo.preparefinalizestate(L, co, result);
}

pub export fn luau_runfinalizers(L: *State, returnstatus: bool) c_int {
    return ldo.runfinalizers(L, returnstatus);
}

/// class.isinstance support: true when the value at objidx is an object whose class
/// (or an ancestor) is the class at clsidx.
pub export fn luau_isinstance(L: *State, objidx: c_int, clsidx: c_int) bool {
    const obj = index2value(L, objidx);
    const c = index2value(L, clsidx);
    if (!o.ttisobject(obj) or !o.ttisclass(c)) return false;
    return @import("class.zig").isinstance(o.objectvalue(obj), o.classvalue(c));
}

/// class.classof support: pushes the class of the object at idx, or nil.
pub export fn luau_pushclassof(L: *State, idx: c_int) void {
    const v = index2value(L, idx);
    const c: ?*o.LuauClass = if (o.ttisobject(v)) o.objectvalue(v).lclass else null;
    const slot = pushslot(L);
    if (c) |x| o.setclassvalue(slot, x) else o.setnilvalue(slot);
}
