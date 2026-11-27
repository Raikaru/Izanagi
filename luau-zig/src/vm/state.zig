//! Global and per-thread state creation/destruction (port of upstream lstate.cpp,
//! MIT; see object.zig header).

const std = @import("std");
const o = @import("object.zig");
const mem = @import("mem.zig");
const ldo = @import("do.zig");
const gc = @import("gc.zig");
const func = @import("func.zig");
const table = @import("table.zig");
const lstring = @import("string.zig");
const tm = @import("tm.zig");

const State = o.State;
const Global = o.Global;
const CallInfo = o.CallInfo;

fn stackInit(L1: *State, L: *State) void {
    const ci = mem.newArray(L, CallInfo, o.BASIC_CI_SIZE, L1.gch.memcat);
    L1.base_ci = ci;
    L1.size_ci = o.BASIC_CI_SIZE;
    L1.ci = &ci[0];
    L1.end_ci = &ci[o.BASIC_CI_SIZE - 1];
    const ssize = o.BASIC_STACK_SIZE + o.EXTRA_STACK;
    const stack = mem.newArray(L, o.TValue, ssize, L1.gch.memcat);
    for (0..ssize) |i| stack[i] = .{};
    L1.stack = stack;
    L1.stacksize = ssize;
    L1.top = stack;
    L1.stack_last = stack + (ssize - o.EXTRA_STACK);
    const c = L1.ci;
    c.* = .{
        .base = undefined,
        .func = L1.top,
        .top = undefined,
        .p = null,
        .savedpc = null,
        .errfunc = 0,
        .nresults = 0,
        .flags = 0,
    };
    o.setnilvalue(&L1.top[0]);
    L1.top += 1;
    L1.base = L1.top;
    c.base = L1.top;
    c.top = L1.top + o.LUA_MINSTACK;
}

fn freestack(L: *State, L1: *State) void {
    if (L1.size_ci > 0) mem.freeArray(L, CallInfo, L1.base_ci, @intCast(L1.size_ci), L1.gch.memcat);
    if (L1.stacksize > 0) mem.freeArray(L, o.TValue, L1.stack, @intCast(L1.stacksize), L1.gch.memcat);
}

fn openBody(L: *State, ud: ?*anyopaque) void {
    _ = ud;
    const g = L.global;
    stackInit(L, L);
    L.gt = table.new(L, 0, 2);
    o.sethvalue(&g.registry, table.new(L, 0, 2));
    lstring.resize(L, o.LUA_MINSTRTABSIZE);
    tm.init(L);
    g.memerrmsg = lstring.new(L, o.LUA_MEMERRMSG);
    lstring.fix(g.memerrmsg);
    g.errerrmsg = lstring.new(L, o.LUA_ERRERRMSG);
    lstring.fix(g.errerrmsg);
    g.GCthreshold = 4 * g.totalbytes;
}

fn closeState(L: *State) void {
    const g = L.global;
    if (L.stacksize > 0) func.close(L, L.stack);
    gc.freeall(L);
    if (g.strt.size > 0) mem.freeArray(L, ?*o.TString, g.strt.hash, @intCast(g.strt.size), 0);
    freestack(L, L);
    const lg: *o.LG = @fieldParentPtr("l", L);
    _ = g.frealloc(g.ud, lg, @sizeOf(o.LG), 0);
}

pub fn newstate(f: o.lua_Alloc, ud: ?*anyopaque) ?*State {
    // fail closed when errors could not be unwound safely
    if (@import("jmp.zig").shadowStackUnsupported()) return null;
    const raw = f(ud, null, 0, @sizeOf(o.LG)) orelse return null;
    const lg: *o.LG = @ptrCast(@alignCast(raw));
    @memset(std.mem.asBytes(lg), 0);
    const L = &lg.l;
    const g = &lg.g;
    L.gch.tt = o.T_THREAD;
    L.global = g;
    g.mainthread = L;
    g.frealloc = f;
    g.ud = ud;
    g.totalbytes = @sizeOf(o.LG);
    g.memcatbytes[0] = @sizeOf(o.LG);
    g.gcgoal = o.LUAI_GCGOAL;
    g.gcstepmul = o.LUAI_GCSTEPMUL;
    g.gcstepsize = o.LUAI_GCSTEPSIZE << 10;
    g.GCthreshold = std.math.maxInt(usize);
    g.lastprotoid = 1;
    g.lastclassid = 1;
    g.ptrenckey[0] = 1;
    g.cb = .{};
    g.registryfree = 0;
    g.allgc = null;
    g.errorjmp = null;
    g.currentwhite = 1 | (1 << 3); // WHITE0 | FIXED, as upstream
    L.gch.marked = g.currentwhite;
    g.gcstate = gc.GCSpause;
    g.uvhead.v = &g.uvhead.value;
    g.uvhead.prev = &g.uvhead;
    g.uvhead.next = &g.uvhead;
    g.gcstats = .{};
    if (ldo.rawrunprotected(L, openBody, null) != 0) {
        closeState(L);
        return null;
    }
    return L;
}

pub fn close(L_in: *State) void {
    const L = L_in.global.mainthread;
    closeState(L);
}

pub fn newthread(L: *State) *State {
    const L1 = mem.newGco(L, State, @sizeOf(State), o.T_THREAD);
    const hdr = L1.gch;
    @memset(std.mem.asBytes(L1), 0);
    L1.gch = hdr;
    L1.global = L.global;
    L1.activememcat = L.activememcat;
    L1.gt = L.gt;
    L1.singlestep = L.singlestep;
    stackInit(L1, L);
    return L1;
}

pub fn resetthread(L: *State) void {
    func.close(L, L.stack);
    const ci = &L.base_ci[0];
    ci.p = null;
    ci.func = L.stack;
    ci.base = ci.func + 1;
    ci.top = ci.base + o.LUA_MINSTACK;
    ci.savedpc = null;
    ci.flags = 0;
    ci.nresults = 0;
    o.setnilvalue(&ci.func[0]);
    L.ci = ci;
    if (L.size_ci != o.BASIC_CI_SIZE) ldo.reallocCI(L, o.BASIC_CI_SIZE);
    L.status = o.LUA_OK;
    L.base = L.ci.base;
    L.top = L.ci.base;
    L.nCcalls = 0;
    L.baseCcalls = 0;
    if (L.stacksize != o.BASIC_STACK_SIZE + o.EXTRA_STACK) ldo.reallocstack(L, o.BASIC_STACK_SIZE, false);
    for (0..@intCast(L.stacksize)) |i| o.setnilvalue(&L.stack[i]);
    L.finalizers = null;
}

pub fn isthreadreset(L: *State) bool {
    return L.ci == &L.base_ci[0] and L.base == L.top and L.status == o.LUA_OK;
}
