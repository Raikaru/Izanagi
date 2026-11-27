//! Prototypes, closures and upvalues (port of upstream lfunc.cpp, MIT; see
//! object.zig header).

const std = @import("std");
const o = @import("object.zig");
const mem = @import("mem.zig");
const gc = @import("gc.zig");

const State = o.State;
const Proto = o.Proto;
const Closure = o.Closure;
const UpVal = o.UpVal;
const Table = o.Table;

pub fn newproto(L: *State) *Proto {
    const f = mem.newGco(L, Proto, @sizeOf(Proto), o.T_PROTO);
    const hdr = f.gch;
    f.* = .{ .gch = hdr };
    return f;
}

pub fn newLclosure(L: *State, nelems: usize, e: *Table, p: *Proto) *Closure {
    const c = mem.newGco(L, Closure, Closure.sizeFor(nelems), o.T_FUNCTION);
    c.isC = 0;
    c.env = e;
    c.nupvalues = @intCast(nelems);
    c.stacksize = p.maxstacksize;
    c.preload = 0;
    c.f = null;
    c.cont = null;
    c.debugname = null;
    c.p = p;
    const up = c.upvals();
    for (0..nelems) |i| up[i] = .{};
    return c;
}

pub fn newCclosure(L: *State, nelems: usize, e: *Table) *Closure {
    const c = mem.newGco(L, Closure, Closure.sizeFor(nelems), o.T_FUNCTION);
    c.isC = 1;
    c.env = e;
    c.nupvalues = @intCast(nelems);
    c.stacksize = o.LUA_MINSTACK;
    c.preload = 0;
    c.f = null;
    c.cont = null;
    c.debugname = null;
    c.p = null;
    const up = c.upvals();
    for (0..nelems) |i| up[i] = .{};
    return c;
}

pub fn closureSize(c: *const Closure) usize {
    return Closure.sizeFor(c.nupvalues);
}

pub fn findupval(L: *State, level: *o.TValue) *UpVal {
    var pp: *?*UpVal = &L.openupval;
    while (pp.*) |p| {
        if (@intFromPtr(p.v) < @intFromPtr(level)) break;
        if (p.v == level) return p;
        pp = &p.threadnext;
    }
    const uv = mem.newGco(L, UpVal, @sizeOf(UpVal), o.T_UPVAL);
    uv.markedopen = 0;
    uv.value = .{};
    uv.v = level;
    uv.threadnext = pp.*;
    pp.* = uv;
    // link into the global list of open upvalues
    const head = &L.global.uvhead;
    uv.prev = head;
    uv.next = head.next;
    uv.next.?.prev = uv;
    head.next = uv;
    return uv;
}

/// Closes all open upvalues of L that point at or above `level`.
pub fn close(L: *State, level: o.StkId) void {
    while (L.openupval) |uv| {
        if (@intFromPtr(uv.v) < @intFromPtr(level)) break;
        L.openupval = uv.threadnext;
        closeupval(L, uv, false);
    }
}

/// luaF_closeupval: unlinks from the global open list; unless the upvalue is dead,
/// copies the value into the upvalue and fixes up its GC color.
pub fn closeupval(L: *State, uv: *UpVal, dead: bool) void {
    uv.next.?.prev = uv.prev;
    uv.prev.?.next = uv.next;
    uv.threadnext = null;
    if (dead) return;
    uv.value = uv.v.*;
    uv.v = &uv.value;
    gc.upvalclosed(L, uv);
}

pub fn freeproto(L: *State, f: *Proto) void {
    const mc = f.gch.memcat;
    mem.freeArray(L, u32, f.code, @intCast(f.sizecode), mc);
    mem.freeArray(L, *Proto, f.p, @intCast(f.sizep), mc);
    mem.freeArray(L, o.TValue, f.k, @intCast(f.sizek), mc);
    if (f.lineinfo) |li| mem.freeArray(L, u8, li, @intCast(f.sizelineinfo), mc);
    if (f.locvars) |lv| mem.freeArray(L, o.LocVar, lv, @intCast(f.sizelocvars), mc);
    if (f.upvalues) |uv| mem.freeArray(L, ?*o.TString, uv, @intCast(f.sizeupvalues), mc);
    if (f.debuginsn) |d| mem.freeArray(L, u8, d, @intCast(f.sizecode), mc);
    if (f.typeinfo) |t| mem.freeArray(L, u8, t, @intCast(f.sizetypeinfo), mc);
    if (f.feedbackvec) |fb| mem.freeArray(L, o.FeedbackSlot, fb, f.feedbackvecsize, mc);
    mem.freeGco(L, f, @sizeOf(Proto), mc);
}

pub fn freeclosure(L: *State, c: *Closure) void {
    mem.freeGco(L, c, closureSize(c), c.gch.memcat);
}

pub fn freeupval(L: *State, uv: *UpVal) void {
    mem.freeGco(L, uv, @sizeOf(UpVal), uv.gch.memcat);
}

pub fn getlocal(f: *const Proto, local_number: c_int, pc: c_int) ?*o.LocVar {
    const lv = f.locvars orelse return null;
    var n = local_number;
    var i: usize = 0;
    while (i < @as(usize, @intCast(f.sizelocvars))) : (i += 1) {
        if (pc >= lv[i].startpc and pc < lv[i].endpc) {
            n -= 1;
            if (n == 0) return &lv[i];
        }
    }
    return null;
}
