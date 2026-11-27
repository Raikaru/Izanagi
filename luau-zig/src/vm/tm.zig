//! Metamethod lookup (port of upstream ltm.cpp, MIT; see object.zig header).

const std = @import("std");
const o = @import("object.zig");
const lstring = @import("string.zig");
const table = @import("table.zig");

const State = o.State;
const Table = o.Table;
const TValue = o.TValue;
const TMS = o.TMS;

pub fn init(L: *State) void {
    const g = L.global;
    for (0..o.T_COUNT) |i| {
        g.ttname[i] = lstring.new(L, o.typenames[i]);
        lstring.fix(g.ttname[i]);
    }
    for (0..o.TM_N) |i| {
        g.tmname[i] = lstring.new(L, o.eventnames[i]);
        lstring.fix(g.tmname[i]);
    }
}

/// Upstream `fasttm`: cached absence for events up to TM_EQ.
pub fn fasttm(L: *State, et: ?*Table, e: TMS) ?*TValue {
    const t = et orelse return null;
    const ei = @intFromEnum(e);
    if (ei <= @intFromEnum(TMS.EQ)) {
        if (t.tmcache & (@as(u8, 1) << @intCast(ei)) != 0) return null;
        const v = table.getstr(t, L.global.tmname[ei]);
        if (o.ttisnil(v)) {
            t.tmcache |= @as(u8, 1) << @intCast(ei);
            return null;
        }
        return v;
    }
    const v = table.getstr(t, L.global.tmname[ei]);
    return if (o.ttisnil(v)) null else v;
}

pub fn metatableOf(L: *State, obj: *const TValue) ?*Table {
    return switch (o.ttype(obj)) {
        o.T_TABLE => o.hvalue(obj).metatable,
        o.T_USERDATA => o.uvalue(obj).metatable,
        o.T_CLASS => o.classvalue(obj).metatable,
        o.T_OBJECT => o.objectvalue(obj).lclass.instancemetatable,
        else => L.global.mt[o.ttype(obj)],
    };
}

/// Returns the metamethod or the nil object.
pub fn gettmbyobj(L: *State, obj: *const TValue, e: TMS) *TValue {
    const mt = metatableOf(L, obj) orelse return o.nilobject();
    return table.getstr(mt, L.global.tmname[@intFromEnum(e)]);
}

pub fn objtypenamestr(L: *State, obj: *const TValue) *o.TString {
    const g = L.global;
    if (o.ttisuserdata(obj) and o.uvalue(obj).tag != o.UTAG_PROXY and o.uvalue(obj).metatable != null) {
        const ty = table.getstr(o.uvalue(obj).metatable.?, g.tmname[@intFromEnum(TMS.TYPE)]);
        if (o.ttisstring(ty)) return o.tsvalue(ty);
        return g.ttname[o.ttype(obj)];
    }
    if (o.ttislightuserdata(obj)) {
        const tag = o.lightuserdatatag(obj);
        if (tag >= 0 and tag < o.LUA_LUTAG_LIMIT) {
            if (g.lightuserdataname[@intCast(tag)]) |name| return name;
        }
    }
    if (g.mt[o.ttype(obj)]) |mt| {
        const ty = table.getstr(mt, g.tmname[@intFromEnum(TMS.TYPE)]);
        if (o.ttisstring(ty)) return o.tsvalue(ty);
    }
    return g.ttname[o.ttype(obj)];
}

pub fn objtypename(L: *State, obj: *const TValue) []const u8 {
    return objtypenamestr(L, obj).slice();
}
