//! Generic (slow-path) VM operations with metamethod support: indexing, arithmetic,
//! comparison, concatenation, length and call fallbacks (port of upstream
//! lvmutils.cpp and luaV_getimport from lvmload.cpp, MIT; see object.zig header).

const std = @import("std");
const o = @import("object.zig");
const ldo = @import("do.zig");
const debug = @import("debug.zig");
const tm = @import("tm.zig");
const table = @import("table.zig");
const lstring = @import("string.zig");
const numprint = @import("numprint.zig");
const gc = @import("gc.zig");
const flags = @import("flags.zig");
const cls = @import("class.zig");

const State = o.State;
const TValue = o.TValue;
const StkId = o.StkId;
const TMS = o.TMS;

const MAXTAGLOOP = 100;

pub fn luai_numpow(a: f64, b: f64) f64 {
    return @import("../luau_math.zig").pow(a, b);
}
pub inline fn luai_nummod(a: f64, b: f64) f64 {
    return a - @floor(a / b) * b;
}
pub inline fn luai_numidiv(a: f64, b: f64) f64 {
    return @floor(a / b);
}

/// luaV_tonumber: returns obj if it is a number, `n` if obj is a convertible string,
/// or null.
pub fn tonumber(obj: *const TValue, n: *TValue) ?*const TValue {
    if (o.ttisnumber(obj)) return obj;
    if (o.ttisstring(obj)) {
        if (numprint.str2d(o.tsvalue(obj).slice())) |num| {
            o.setnvalue(n, num);
            return n;
        }
    }
    return null;
}

/// luaV_tostring: converts a number in place to a string.
pub fn tostringInPlace(L: *State, obj: *TValue) bool {
    if (!o.ttisnumber(obj)) return false;
    var buf: [numprint.MAXNUM2STR]u8 = undefined;
    const s = numprint.num2str(&buf, o.nvalue(obj));
    o.setsvalue(obj, lstring.new(L, s));
    return true;
}

pub inline fn tostr(L: *State, obj: *TValue) bool {
    return o.ttisstring(obj) or tostringInPlace(L, obj);
}

fn callTMres(L: *State, res: StkId, f: *const TValue, p1: *const TValue, p2: *const TValue) StkId {
    const result = ldo.savestack(L, res);
    // operands may point into the stack, which checkstack can reallocate
    const fc = f.*;
    const a = p1.*;
    const b = p2.*;
    ldo.checkstack(L, 3);
    const top = L.top;
    top[0] = fc;
    top[1] = a;
    top[2] = b;
    L.top += 3;
    ldo.call(L, top, 1);
    const r = ldo.restorestack(L, result);
    L.top -= 1;
    r[0] = L.top[0];
    return r;
}

fn callTM(L: *State, f: *const TValue, p1: *const TValue, p2: *const TValue, p3: *const TValue) void {
    const fc = f.*;
    const a = p1.*;
    const b = p2.*;
    const c = p3.*;
    ldo.checkstack(L, 4);
    const top = L.top;
    top[0] = fc;
    top[1] = a;
    top[2] = b;
    top[3] = c;
    L.top += 4;
    ldo.call(L, top, 0);
}

/// luaV_gettable. `t` and `key` may point into the stack; `val` is a stack slot.
pub fn gettable(L: *State, t_in: *const TValue, key_in: *const TValue, val_in: StkId) void {
    // copy operands: metamethod calls may reallocate the stack
    var tcopy = t_in.*;
    const key = key_in.*;
    const valoff = ldo.savestack(L, val_in);
    var t: *const TValue = &tcopy;
    var loop: usize = 0;
    while (loop < MAXTAGLOOP) : (loop += 1) {
        var tmv: *const TValue = undefined;
        if (o.ttistable(t)) {
            const h = o.hvalue(t);
            const res = table.get(h, &key);
            if (!o.ttisnil(res)) {
                ldo.restorestack(L, valoff)[0] = res.*;
                return;
            }
            const f = tm.fasttm(L, h.metatable, .INDEX) orelse {
                o.setnilvalue(&ldo.restorestack(L, valoff)[0]);
                return;
            };
            tmv = f;
        } else if (flags.DebugLuauUserDefinedClassesRuntime and o.ttisobject(t)) {
            const inst = o.objectvalue(t);
            const offv = table.get(inst.lclass.memberstooffset.?, &key);
            if (o.ttisnil(offv)) cls.missingmembererror(L, t, &key);
            ldo.restorestack(L, valoff)[0] = cls.memberatoffset(inst, @intFromFloat(o.nvalue(offv))).*;
            return;
        } else if (flags.DebugLuauUserDefinedClassesRuntime and o.ttisclass(t)) {
            const c = o.classvalue(t);
            const offv = table.get(c.memberstooffset.?, &key);
            if (o.ttisnil(offv)) cls.missingmembererror(L, t, &key);
            const off: u32 = @intFromFloat(o.nvalue(offv));
            if (off < c.numberofinstancemembers) cls.missingmembererror(L, t, &key);
            ldo.restorestack(L, valoff)[0] = c.staticmembers.?[off - c.numberofinstancemembers];
            return;
        } else {
            const f = tm.gettmbyobj(L, t, .INDEX);
            if (o.ttisnil(f)) debug.indexerror(L, t, &key);
            tmv = f;
        }
        if (o.ttisfunction(tmv)) {
            const fcopy = tmv.*;
            _ = callTMres(L, ldo.restorestack(L, valoff), &fcopy, t, &key);
            return;
        }
        tcopy = tmv.*;
        t = &tcopy;
    }
    debug.runerror(L, "'__index' chain too long; possible loop", .{});
}

/// luaV_settable.
pub fn settable(L: *State, t_in: *const TValue, key_in: *const TValue, val_in: *const TValue) void {
    var tcopy = t_in.*;
    const key = key_in.*;
    const val = val_in.*;
    var t: *const TValue = &tcopy;
    var loop: usize = 0;
    while (loop < MAXTAGLOOP) : (loop += 1) {
        var tmv: *const TValue = undefined;
        if (o.ttistable(t)) {
            const h = o.hvalue(t);
            const oldval = table.get(h, &key);
            var usetm: ?*TValue = null;
            if (o.ttisnil(oldval)) usetm = tm.fasttm(L, h.metatable, .NEWINDEX);
            if (usetm == null) {
                if (h.readonly != 0) debug.readonlyerror(L);
                h.tmcache = 0;
                const slot = if (oldval != o.nilobject()) oldval else table.newkey(L, h, &key);
                slot.* = val;
                gc.barriert(L, h, &val);
                return;
            }
            tmv = usetm.?;
        } else if (flags.DebugLuauUserDefinedClassesRuntime and o.ttisobject(t)) {
            const inst = o.objectvalue(t);
            const offv = table.get(inst.lclass.memberstooffset.?, &key);
            if (o.ttisnil(offv)) cls.missingmembererror(L, t, &key);
            const off: u32 = @intFromFloat(o.nvalue(offv));
            if (off >= inst.lclass.numberofinstancemembers) debug.indexerror(L, t, &key);
            inst.members.?[off] = val;
            gc.barrier(L, &inst.gch, &val);
            return;
        } else {
            const f = tm.gettmbyobj(L, t, .NEWINDEX);
            if (o.ttisnil(f)) debug.indexerror(L, t, &key);
            tmv = f;
        }
        if (o.ttisfunction(tmv)) {
            const fcopy = tmv.*;
            callTM(L, &fcopy, t, &key, &val);
            return;
        }
        tcopy = tmv.*;
        t = &tcopy;
    }
    debug.runerror(L, "'__newindex' chain too long; possible loop", .{});
}

fn callBinTM(L: *State, p1: *const TValue, p2: *const TValue, res: StkId, event: TMS) bool {
    var f = tm.gettmbyobj(L, p1, event);
    if (o.ttisnil(f)) f = tm.gettmbyobj(L, p2, event);
    if (o.ttisnil(f)) return false;
    const fcopy = f.*;
    _ = callTMres(L, res, &fcopy, p1, p2);
    return true;
}

fn getCompTM(L: *State, mt1: ?*o.Table, mt2: ?*o.Table, event: TMS) ?*const TValue {
    const tm1 = tm.fasttm(L, mt1, event) orelse return null;
    if (mt1 == mt2) return tm1;
    const tm2 = tm.fasttm(L, mt2, event) orelse return null;
    if (table.rawequalObj(tm1, tm2)) return tm1;
    return null;
}

/// Calls the `event` metamethod shared by p1 and p2 and returns its truthiness; null when
/// they have no common handler.
fn callOrderTM(L: *State, p1: *const TValue, p2: *const TValue, event: TMS) ?bool {
    const tm1 = tm.gettmbyobj(L, p1, event);
    if (o.ttisnil(tm1)) return null;
    const tm2 = tm.gettmbyobj(L, p2, event);
    if (!table.rawequalObj(tm1, tm2)) return null;
    const fcopy = tm1.*;
    const a = p1.*;
    const b = p2.*;
    _ = callTMres(L, L.top, &fcopy, &a, &b);
    return !o.l_isfalse(&L.top[0]);
}

pub fn strcmp(ls: *const o.TString, rs: *const o.TString) c_int {
    if (ls == rs) return 0;
    const l = ls.slice();
    const r = rs.slice();
    // upstream compares the first bytes (of NUL-terminated data) first
    const l0: c_int = if (l.len > 0) l[0] else 0;
    const r0: c_int = if (r.len > 0) r[0] else 0;
    if (l0 != r0) return l0 - r0;
    const lmin = @min(l.len, r.len);
    switch (std.mem.order(u8, l[0..lmin], r[0..lmin])) {
        .lt => return -1,
        .gt => return 1,
        .eq => {},
    }
    return if (l.len == r.len) 0 else if (l.len < r.len) -1 else 1;
}

pub fn lessthan(L: *State, l: *const TValue, r: *const TValue) bool {
    if (l.tt != r.tt) debug.ordererror(L, l, r, .LT);
    if (o.ttisnumber(l)) return o.nvalue(l) < o.nvalue(r);
    if (o.ttisstring(l)) return strcmp(o.tsvalue(l), o.tsvalue(r)) < 0;
    return callOrderTM(L, l, r, .LT) orelse debug.ordererror(L, l, r, .LT);
}

pub fn lessequal(L: *State, l: *const TValue, r: *const TValue) bool {
    if (l.tt != r.tt) debug.ordererror(L, l, r, .LE);
    if (o.ttisnumber(l)) return o.nvalue(l) <= o.nvalue(r);
    if (o.ttisstring(l)) return strcmp(o.tsvalue(l), o.tsvalue(r)) <= 0;
    const lc = l.*;
    const rc = r.*;
    if (callOrderTM(L, &lc, &rc, .LE)) |le| return le;
    // try 'not (r < l)'
    const gt = callOrderTM(L, &rc, &lc, .LT) orelse debug.ordererror(L, &lc, &rc, .LE);
    return !gt;
}

/// luaV_equalval: values must have the same type.
pub fn equalval(L: *State, t1: *const TValue, t2: *const TValue) bool {
    var f: ?*const TValue = null;
    switch (o.ttype(t1)) {
        o.T_NIL => return true,
        o.T_NUMBER => return o.nvalue(t1) == o.nvalue(t2),
        o.T_INTEGER => return o.lvalue(t1) == o.lvalue(t2),
        o.T_VECTOR => return table.veceq(o.vvalue(t1), o.vvalue(t2)),
        o.T_BOOLEAN => return o.bvalue(t1) == o.bvalue(t2),
        o.T_LIGHTUSERDATA => return o.pvalue(t1) == o.pvalue(t2) and o.lightuserdatatag(t1) == o.lightuserdatatag(t2),
        o.T_USERDATA => {
            f = getCompTM(L, o.uvalue(t1).metatable, o.uvalue(t2).metatable, .EQ);
            if (f == null) return o.uvalue(t1) == o.uvalue(t2);
        },
        o.T_TABLE => {
            f = getCompTM(L, o.hvalue(t1).metatable, o.hvalue(t2).metatable, .EQ);
            if (f == null) return o.hvalue(t1) == o.hvalue(t2);
        },
        o.T_CLASS => return o.classvalue(t1) == o.classvalue(t2),
        o.T_OBJECT => {
            f = getCompTM(L, o.objectvalue(t1).lclass.instancemetatable, o.objectvalue(t2).lclass.instancemetatable, .EQ);
            if (f == null) return o.objectvalue(t1) == o.objectvalue(t2);
        },
        else => return o.gcvalue(t1) == o.gcvalue(t2),
    }
    const fcopy = f.?.*;
    const a = t1.*;
    const b = t2.*;
    _ = callTMres(L, L.top, &fcopy, &a, &b);
    return !o.l_isfalse(&L.top[0]);
}

/// luaV_concat: concatenates `total` values ending at register `last` of the frame.
pub fn concat(L: *State, total_in: c_int, last_in: c_int) void {
    var total = total_in;
    var last = last_in;
    while (true) {
        const top = L.base + @as(usize, @intCast(last + 1));
        var n: c_int = 2;
        const a = &(top - 2)[0];
        const b = &(top - 1)[0];
        if (!(o.ttisstring(a) or o.ttisnumber(a)) or !tostr(L, b)) {
            if (!callBinTM(L, a, b, top - 2, .CONCAT)) debug.concaterror(L, a, b);
        } else if (o.tsvalue(b).len == 0) {
            _ = tostr(L, a);
        } else {
            var tl: usize = o.tsvalue(b).len;
            n = 1;
            while (n < total and tostr(L, &(top - @as(usize, @intCast(n + 1)))[0])) : (n += 1) {
                const l: usize = o.tsvalue(&(top - @as(usize, @intCast(n + 1)))[0]).len;
                if (l > o.MAXSSIZE - tl) debug.runerror(L, "string length overflow", .{});
                tl += l;
            }
            var buf: [o.LUA_BUFFERSIZE]u8 = undefined;
            var ts: ?*o.TString = null;
            var dst: [*]u8 = &buf;
            if (tl >= o.LUA_BUFFERSIZE) {
                ts = lstring.bufstart(L, tl);
                dst = ts.?.data();
            }
            var off: usize = 0;
            var i: c_int = n;
            while (i > 0) : (i -= 1) {
                const s = o.tsvalue(&(top - @as(usize, @intCast(i)))[0]).slice();
                @memcpy(dst[off .. off + s.len], s);
                off += s.len;
            }
            const res = &(top - @as(usize, @intCast(n)))[0];
            if (ts) |t| {
                o.setsvalue(res, lstring.buffinish(L, t));
            } else {
                o.setsvalue(res, lstring.new(L, buf[0..off]));
            }
        }
        total -= n - 1;
        last -= n - 1;
        if (total <= 1) break;
    }
}

fn vecop(op: TMS, b: [3]f32, c: [3]f32) ?[3]f32 {
    var r: [3]f32 = undefined;
    for (0..3) |i| {
        r[i] = switch (op) {
            .ADD => b[i] + c[i],
            .SUB => b[i] - c[i],
            .MUL => b[i] * c[i],
            .DIV => b[i] / c[i],
            .IDIV => @floatCast(luai_numidiv(b[i], c[i])),
            .UNM => -b[i],
            else => return null,
        };
    }
    return r;
}

/// luaV_doarith.
pub fn arith(L: *State, ra: StkId, rb: *const TValue, rc: *const TValue, op: TMS) void {
    if (o.ttisvector(rb) and o.ttisvector(rc)) {
        if (vecop(op, o.vvalue(rb), o.vvalue(rc))) |r| {
            o.setvvalue(&ra[0], r[0], r[1], r[2]);
            return;
        }
    } else if (o.ttisvector(rb)) {
        var tmp: TValue = .{};
        if (tonumber(rc, &tmp)) |c| {
            const nc: f32 = @floatCast(o.nvalue(c));
            const vb = o.vvalue(rb);
            switch (op) {
                .MUL, .DIV, .IDIV => {
                    var r: [3]f32 = undefined;
                    for (0..3) |i| r[i] = switch (op) {
                        .MUL => vb[i] * nc,
                        .DIV => vb[i] / nc,
                        else => @floatCast(luai_numidiv(vb[i], nc)),
                    };
                    o.setvvalue(&ra[0], r[0], r[1], r[2]);
                    return;
                },
                else => {},
            }
        }
    } else if (o.ttisvector(rc)) {
        var tmp: TValue = .{};
        if (tonumber(rb, &tmp)) |b| {
            const nb: f32 = @floatCast(o.nvalue(b));
            const vc = o.vvalue(rc);
            switch (op) {
                .MUL, .DIV, .IDIV => {
                    var r: [3]f32 = undefined;
                    for (0..3) |i| r[i] = switch (op) {
                        .MUL => nb * vc[i],
                        .DIV => nb / vc[i],
                        else => @floatCast(luai_numidiv(nb, vc[i])),
                    };
                    o.setvvalue(&ra[0], r[0], r[1], r[2]);
                    return;
                },
                else => {},
            }
        }
    }
    var tb: TValue = .{};
    var tc: TValue = .{};
    if (tonumber(rb, &tb)) |b| {
        if (tonumber(rc, &tc)) |c| {
            const nb = o.nvalue(b);
            const nc = o.nvalue(c);
            o.setnvalue(&ra[0], switch (op) {
                .ADD => nb + nc,
                .SUB => nb - nc,
                .MUL => nb * nc,
                .DIV => nb / nc,
                .IDIV => luai_numidiv(nb, nc),
                .MOD => luai_nummod(nb, nc),
                .POW => luai_numpow(nb, nc),
                .UNM => -nb,
                else => unreachable,
            });
            return;
        }
    }
    const bcopy = rb.*;
    const ccopy = rc.*;
    if (!callBinTM(L, &bcopy, &ccopy, ra, op)) debug.aritherror(L, &bcopy, &ccopy, op);
}

/// luaV_dolen.
pub fn dolen(L: *State, ra: StkId, rb: *const TValue) void {
    var f: ?*const TValue = null;
    switch (o.ttype(rb)) {
        o.T_TABLE => {
            const h = o.hvalue(rb);
            f = tm.fasttm(L, h.metatable, .LEN);
            if (f == null) {
                o.setnvalue(&ra[0], @floatFromInt(table.getn(h)));
                return;
            }
        },
        o.T_STRING => {
            o.setnvalue(&ra[0], @floatFromInt(o.tsvalue(rb).len));
            return;
        },
        else => {
            const v = tm.gettmbyobj(L, rb, .LEN);
            f = if (o.ttisnil(v)) null else v;
        },
    }
    if (f == null) debug.typeerror(L, rb, "get length of");
    const fcopy = f.?.*;
    const bcopy = rb.*;
    const res = callTMres(L, ra, &fcopy, &bcopy, o.nilobject());
    if (!o.ttisnumber(&res[0])) debug.runerror(L, "'__len' must return a number", .{});
}

/// luaV_prepareFORN
pub fn prepareFORN(L: *State, plimit: *TValue, pstep: *TValue, pinit: *TValue) void {
    var tmp: TValue = .{};
    if (!o.ttisnumber(pinit)) {
        const r = tonumber(pinit, &tmp) orelse debug.forerror(L, pinit, "initial value");
        pinit.* = r.*;
    }
    if (!o.ttisnumber(plimit)) {
        const r = tonumber(plimit, &tmp) orelse debug.forerror(L, plimit, "limit");
        plimit.* = r.*;
    }
    if (!o.ttisnumber(pstep)) {
        const r = tonumber(pstep, &tmp) orelse debug.forerror(L, pstep, "step");
        pstep.* = r.*;
    }
}

/// luaV_tryfuncTM: replaces a non-function callee with its __call metamethod.
pub fn tryfuncTM(L: *State, f: StkId) void {
    const t = tm.gettmbyobj(L, &f[0], .CALL);
    if (!o.ttisfunction(t)) debug.typeerror(L, &f[0], "call");
    const tcopy = t.*;
    var p = L.top;
    while (@intFromPtr(p) > @intFromPtr(f)) : (p -= 1) p[0] = (p - 1)[0];
    L.top += 1;
    f[0] = tcopy;
}

/// luaV_getimport
pub fn getimport(L: *State, env: *o.Table, k: [*]TValue, res_in: StkId, id: u32, propagatenil: bool) void {
    const count = id >> 30;
    const id0 = (id >> 20) & 1023;
    const id1 = (id >> 10) & 1023;
    const id2 = id & 1023;
    const resp = ldo.savestack(L, res_in);
    var g: TValue = .{};
    o.sethvalue(&g, env);
    gettable(L, &g, &k[id0], res_in);
    if (count < 2) return;
    var res = ldo.restorestack(L, resp);
    if (!propagatenil or !o.ttisnil(&res[0])) gettable(L, &res[0], &k[id1], res);
    if (count < 3) return;
    res = ldo.restorestack(L, resp);
    if (!propagatenil or !o.ttisnil(&res[0])) gettable(L, &res[0], &k[id2], res);
}
