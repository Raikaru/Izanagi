//! Incremental garbage collector (port of upstream lgc.cpp, MIT; see object.zig
//! header).
//!
//! Tri-color incremental mark & sweep with the same states (pause, propagate,
//! propagateagain, atomic, sweep), write barriers, active-thread rescans, weak
//! tables ('k', 'v', 's'), open-upvalue handling, step sizing (gcstepsize,
//! gcstepmul, assist debt), heap goal and PI-controlled heap trigger, stack and
//! string-table shrinking, userdata destructors, LuauGcTraceUdata marking hooks and
//! embedder GC, and interrupt notifications with the GC state.
//!
//! Difference from upstream: objects live in a singly linked all-objects list
//! instead of size-class pages, so sweep work is measured per object
//! (GC_SWEEPPAGESTEPCOST per object) rather than per page block.

const std = @import("std");
const o = @import("object.zig");
const mem = @import("mem.zig");
const table = @import("table.zig");
const func = @import("func.zig");
const lstring = @import("string.zig");
const ldo = @import("do.zig");
const flags = @import("flags.zig");

const State = o.State;
const Global = o.Global;
const GCObject = o.GCObject;
const TValue = o.TValue;

pub const GCSpause: u8 = 0;
pub const GCSpropagate: u8 = 1;
pub const GCSpropagateagain: u8 = 2;
pub const GCSatomic: u8 = 3;
pub const GCSsweep: u8 = 4;

const WHITE0BIT: u8 = 1 << 0;
const WHITE1BIT: u8 = 1 << 1;
const BLACKBIT: u8 = 1 << 2;
pub const FIXEDBIT: u8 = lstring.FIXEDBIT;
const WHITEBITS: u8 = WHITE0BIT | WHITE1BIT;
const maskmarks: u8 = ~(BLACKBIT | WHITEBITS);

const GC_SWEEPPAGESTEPCOST = 16;

// ---------------------------------------------------------------------------
// Color helpers (lgc.h)
// ---------------------------------------------------------------------------

pub inline fn iswhite(x: *const GCObject) bool {
    return x.marked & WHITEBITS != 0;
}
pub inline fn isblack(x: *const GCObject) bool {
    return x.marked & BLACKBIT != 0;
}
pub inline fn isgray(x: *const GCObject) bool {
    return x.marked & (WHITEBITS | BLACKBIT) == 0;
}
inline fn otherwhite(g: *const Global) u8 {
    return g.currentwhite ^ WHITEBITS;
}
pub inline fn isdead(g: *const Global, v: *const GCObject) bool {
    return (v.marked & (WHITEBITS | FIXEDBIT)) == (otherwhite(g) & WHITEBITS);
}
pub inline fn changewhite(x: *GCObject) void {
    x.marked ^= WHITEBITS;
}
inline fn gray2black(x: *GCObject) void {
    x.marked |= BLACKBIT;
}
inline fn black2gray(x: *GCObject) void {
    x.marked &= ~BLACKBIT;
}
inline fn white2gray(x: *GCObject) void {
    x.marked &= ~WHITEBITS;
}
pub inline fn luaC_white(g: *const Global) u8 {
    return g.currentwhite & WHITEBITS;
}
inline fn makewhite(g: *const Global, x: *GCObject) void {
    x.marked = (x.marked & maskmarks) | luaC_white(g);
}
inline fn keepinvariant(g: *const Global) bool {
    return g.gcstate == GCSpropagate or g.gcstate == GCSpropagateagain or g.gcstate == GCSatomic;
}

inline fn gco(x: anytype) *GCObject {
    return &x.gch;
}

// ---------------------------------------------------------------------------
// Marking
// ---------------------------------------------------------------------------

inline fn markobject(g: *Global, obj: *GCObject) void {
    if (iswhite(obj)) reallymarkobject(g, obj);
}
inline fn markopt(g: *Global, x: anytype) void {
    if (x) |p| markobject(g, &p.gch);
}
inline fn markvalue(g: *Global, v: *const TValue) void {
    if (o.iscollectable(v) and v.tt != o.T_DEADKEY and iswhite(v.value.gc)) reallymarkobject(g, v.value.gc);
}
inline fn stringmark(s: ?*o.TString) void {
    if (s) |x| x.gch.marked &= ~WHITEBITS;
}

fn reallymarkobject(g: *Global, obj: *GCObject) void {
    white2gray(obj);
    switch (obj.tt) {
        o.T_STRING, o.T_BUFFER => gray2black(obj),
        o.T_USERDATA => {
            gray2black(obj);
            const u: *o.Udata = @ptrCast(@alignCast(obj));
            if (flags.LuauGcTraceUdata and u.tag < o.LUA_UTAG_LIMIT) {
                if (g.udatamark[u.tag]) |markfn| markfn(g.mainthread, u.data());
            }
            if (u.metatable) |mt| markobject(g, &mt.gch);
        },
        o.T_UPVAL => {
            const uv: *o.UpVal = @ptrCast(@alignCast(obj));
            markvalue(g, uv.v);
            if (!uv.isOpen()) gray2black(obj); // open upvalues are never black
        },
        o.T_FUNCTION, o.T_TABLE, o.T_THREAD, o.T_PROTO, o.T_CLASS, o.T_OBJECT => {
            obj.gclist = g.gray;
            g.gray = obj;
        },
        else => unreachable,
    }
}

fn tablemode(g: *Global, h: *o.Table) ?[]const u8 {
    const mt = h.metatable orelse return null;
    // gfasttm semantics: the absence of __mode is cached in tmcache
    if (mt.tmcache & (@as(u8, 1) << @intFromEnum(o.TMS.MODE)) != 0) return null;
    const mode = table.getstr(mt, g.tmname[@intFromEnum(o.TMS.MODE)]);
    if (o.ttisnil(mode)) {
        mt.tmcache |= @as(u8, 1) << @intFromEnum(o.TMS.MODE);
        return null;
    }
    if (o.ttisstring(mode)) return std.mem.sliceTo(o.tsvalue(mode).slice(), 0);
    return null;
}

inline fn removeentry(n: *o.LuaNode) void {
    if (n.key.tt >= o.T_STRING) n.key.tt = o.T_DEADKEY;
}

fn traversetable(g: *Global, h: *o.Table) bool {
    var weakkey = false;
    var weakvalue = false;
    if (h.metatable) |mt| markobject(g, &mt.gch);
    if (tablemode(g, h)) |mode| {
        weakkey = std.mem.indexOfScalar(u8, mode, 'k') != null;
        weakvalue = std.mem.indexOfScalar(u8, mode, 'v') != null;
        if (weakkey or weakvalue) {
            h.gch.gclist = g.weak;
            g.weak = &h.gch;
        }
    }
    if (weakkey and weakvalue) return true;
    if (!weakvalue) {
        if (h.array) |a| {
            var i: usize = @intCast(h.sizearray);
            while (i > 0) {
                i -= 1;
                markvalue(g, &a[i]);
            }
        }
    }
    if (h.node != o.dummynode()) {
        var i: usize = @intCast(h.sizenode());
        while (i > 0) {
            i -= 1;
            const n = &h.node[i];
            if (o.ttisnil(&n.val)) {
                removeentry(n);
            } else {
                if (!weakkey) markvalue(g, o.keyAsValue(&n.key));
                if (!weakvalue) markvalue(g, &n.val);
            }
        }
    }
    return weakkey or weakvalue;
}

fn traverseproto(g: *Global, f: *o.Proto) void {
    stringmark(f.source);
    stringmark(f.debugname);
    for (0..@intCast(f.sizek)) |i| markvalue(g, &f.k[i]);
    if (f.upvalues) |ups| {
        for (0..@intCast(f.sizeupvalues)) |i| stringmark(ups[i]);
    }
    for (0..@intCast(f.sizep)) |i| markobject(g, &f.p[i].gch);
    if (f.locvars) |lv| {
        for (0..@intCast(f.sizelocvars)) |i| stringmark(lv[i].varname);
    }
}

fn traverseclosure(g: *Global, cl: *o.Closure) void {
    markobject(g, &cl.env.gch);
    const up = cl.upvals();
    if (cl.isC != 0) {
        stringmark(cl.debugname);
        for (0..cl.nupvalues) |i| markvalue(g, &up[i]);
    } else {
        markobject(g, &cl.p.?.gch);
        for (0..cl.nupvalues) |i| {
            if (o.ttisupval(&up[i])) markobject(g, up[i].value.gc) else markvalue(g, &up[i]);
        }
    }
}

fn traversestack(g: *Global, l: *State) void {
    markobject(g, &l.gt.gch);
    stringmark(l.namecall);
    if (l.finalizers) |f| markobject(g, &f.gch);
    var p = l.stack;
    while (@intFromPtr(p) < @intFromPtr(l.top)) : (p += 1) markvalue(g, &p[0]);
    var up = l.openupval;
    while (up) |uv| : (up = uv.threadnext) {
        uv.markedopen = 1;
        markobject(g, &uv.gch);
    }
}

fn clearstack(l: *State) void {
    const stack_end = l.stack + @as(usize, @intCast(l.stacksize));
    var p = l.top;
    while (@intFromPtr(p) < @intFromPtr(stack_end)) : (p += 1) o.setnilvalue(&p[0]);
}

fn shrinkstack(L: *State) void {
    var lim = L.top;
    var ci: [*]o.CallInfo = L.base_ci;
    while (@intFromPtr(ci) <= @intFromPtr(L.ci)) : (ci += 1) {
        if (@intFromPtr(lim) < @intFromPtr(ci[0].top)) lim = ci[0].top;
    }
    const ci_used = o.idiff(@as([*]o.CallInfo, @ptrCast(L.ci)), L.base_ci);
    const s_used = o.idiff(lim, L.stack);
    if (L.size_ci > o.LUAI_MAXCALLS) return;
    if (3 * ci_used < L.size_ci and 2 * o.BASIC_CI_SIZE < L.size_ci) ldo.reallocCI(L, @divTrunc(L.size_ci, 2));
    if (3 * s_used < L.stacksize and 2 * (o.BASIC_STACK_SIZE + o.EXTRA_STACK) < L.stacksize)
        ldo.reallocstack(L, @divTrunc(L.stacksize, 2), false);
}

fn shrinkBody(L: *State, ud: ?*anyopaque) void {
    _ = ud;
    shrinkstack(L);
}

fn shrinkstackprotected(L: *State) void {
    _ = ldo.rawrunprotected(L, shrinkBody, null);
}

fn propagatemark(g: *Global) usize {
    const obj = g.gray.?;
    gray2black(obj);
    g.gray = obj.gclist;
    switch (obj.tt) {
        o.T_TABLE => {
            const h: *o.Table = @ptrCast(@alignCast(obj));
            if (traversetable(g, h)) black2gray(obj);
            return @sizeOf(o.Table) + @sizeOf(TValue) * @as(usize, @intCast(h.sizearray)) +
                @sizeOf(o.LuaNode) * (if (h.node == o.dummynode()) 0 else @as(usize, @intCast(h.sizenode())));
        },
        o.T_FUNCTION => {
            const cl: *o.Closure = @ptrCast(@alignCast(obj));
            traverseclosure(g, cl);
            return func.closureSize(cl);
        },
        o.T_THREAD => {
            const th: *State = @ptrCast(@alignCast(obj));
            const active = th.isactive or th == g.mainthread;
            traversestack(g, th);
            if (active) {
                obj.gclist = g.grayagain;
                g.grayagain = obj;
                black2gray(obj);
            }
            if (!active or g.gcstate == GCSatomic) clearstack(th);
            if (g.gcstate == GCSpropagate) shrinkstackprotected(th);
            return @sizeOf(State) + @sizeOf(TValue) * @as(usize, @intCast(th.stacksize)) + @sizeOf(o.CallInfo) * @as(usize, @intCast(th.size_ci));
        },
        o.T_CLASS => {
            const c: *o.LuauClass = @ptrCast(@alignCast(obj));
            stringmark(c.name);
            markopt(g, c.super);
            markopt(g, c.memberstooffset);
            if (c.offsettomember) |otm| {
                for (0..c.offsettomemberlength()) |i| stringmark(otm[i]);
            }
            if (c.staticmembers) |sm| {
                for (0..c.numstaticmembers()) |i| markvalue(g, &sm[i]);
            }
            markopt(g, c.metatable);
            markopt(g, c.instancemetatable);
            return @sizeOf(o.LuauClass) + c.numstaticmembers() * @sizeOf(TValue) + c.offsettomemberlength() * @sizeOf(?*o.TString);
        },
        o.T_OBJECT => {
            const ob: *o.LuauObject = @ptrCast(@alignCast(obj));
            markobject(g, &ob.lclass.gch);
            if (ob.members) |m| {
                for (0..ob.numberofmembers) |i| markvalue(g, &m[i]);
            }
            return @sizeOf(o.LuauObject) + ob.numberofmembers * @sizeOf(TValue);
        },
        o.T_PROTO => {
            const p: *o.Proto = @ptrCast(@alignCast(obj));
            traverseproto(g, p);
            return @sizeOf(o.Proto) + @sizeOf(u32) * @as(usize, @intCast(p.sizecode)) + @sizeOf(*o.Proto) * @as(usize, @intCast(p.sizep)) +
                @sizeOf(TValue) * @as(usize, @intCast(p.sizek)) + @as(usize, @intCast(p.sizelineinfo)) +
                @sizeOf(o.LocVar) * @as(usize, @intCast(p.sizelocvars)) + @sizeOf(?*o.TString) * @as(usize, @intCast(p.sizeupvalues)) +
                @as(usize, @intCast(p.sizetypeinfo));
        },
        else => unreachable,
    }
}

fn propagateall(g: *Global) usize {
    var work: usize = 0;
    while (g.gray != null) work += propagatemark(g);
    return work;
}

/// Strings are values and never cleared from weak tables; checking them marks them.
fn iscleared(v: *const TValue) bool {
    if (!o.iscollectable(v)) return false;
    const obj = v.value.gc;
    if (obj.tt == o.T_STRING) {
        obj.marked &= ~WHITEBITS;
        return false;
    }
    return iswhite(obj);
}

const ResizeCtx = struct { t: *o.Table, n: c_int };
fn resizeBody(L: *State, ud: ?*anyopaque) void {
    const c: *ResizeCtx = @ptrCast(@alignCast(ud.?));
    table.resizehash(L, c.t, c.n);
}

fn cleartable(L: *State, list: ?*GCObject) usize {
    var work: usize = 0;
    var l = list;
    while (l) |obj| {
        const h: *o.Table = @ptrCast(@alignCast(obj));
        l = obj.gclist;
        work += @sizeOf(o.Table) + @sizeOf(TValue) * @as(usize, @intCast(h.sizearray)) +
            @sizeOf(o.LuaNode) * (if (h.node == o.dummynode()) 0 else @as(usize, @intCast(h.sizenode())));
        if (h.array) |a| {
            var i: usize = @intCast(h.sizearray);
            while (i > 0) {
                i -= 1;
                if (iscleared(&a[i])) o.setnilvalue(&a[i]);
            }
        }
        var activevalues: c_int = 0;
        if (h.node != o.dummynode()) {
            var i: usize = @intCast(h.sizenode());
            while (i > 0) {
                i -= 1;
                const n = &h.node[i];
                if (!o.ttisnil(&n.val)) {
                    if (iscleared(o.keyAsValue(&n.key)) or iscleared(&n.val)) {
                        o.setnilvalue(&n.val);
                        removeentry(n);
                    } else activevalues += 1;
                }
            }
        }
        if (h.readonly == 0) {
            if (tablemode(L.global, h)) |mode| {
                if (std.mem.indexOfScalar(u8, mode, 's') != null and activevalues < @divTrunc(h.sizenode() * 3, 8)) {
                    var ctx: ResizeCtx = .{ .t = h, .n = activevalues };
                    _ = ldo.rawrunprotected(L, resizeBody, &ctx);
                }
            }
        }
    }
    return work;
}

// ---------------------------------------------------------------------------
// Freeing
// ---------------------------------------------------------------------------

pub fn udataSize(len: usize) usize {
    return o.Udata.header + len;
}

fn freeudata(L: *State, u: *o.Udata) void {
    var dtor: ?o.lua_Destructor = null;
    if (u.tag < o.LUA_UTAG_LIMIT) {
        dtor = L.global.udatagc[u.tag];
    } else if (u.tag == o.UTAG_IDTOR) {
        const len: usize = @intCast(u.len);
        var raw: usize = 0;
        @memcpy(std.mem.asBytes(&raw), u.data()[len - @sizeOf(usize) .. len]);
        dtor = @ptrFromInt(raw);
    }
    if (dtor) |d| d(L, u.data());
    mem.freeGco(L, u, udataSize(@intCast(u.len)), u.gch.memcat);
}

pub fn freethread(L: *State, L1: *State) void {
    if (L.global.cb.userthread) |cb| cb(null, L1);
    mem.freeArray(L, o.CallInfo, L1.base_ci, @intCast(L1.size_ci), L1.gch.memcat);
    mem.freeArray(L, TValue, L1.stack, @intCast(L1.stacksize), L1.gch.memcat);
    mem.freeGco(L, L1, @sizeOf(State), L1.gch.memcat);
}

fn freeobj(L: *State, obj: *GCObject) void {
    switch (obj.tt) {
        o.T_PROTO => func.freeproto(L, @ptrCast(@alignCast(obj))),
        o.T_FUNCTION => func.freeclosure(L, @ptrCast(@alignCast(obj))),
        o.T_UPVAL => func.freeupval(L, @ptrCast(@alignCast(obj))),
        o.T_TABLE => table.free(L, @ptrCast(@alignCast(obj))),
        o.T_THREAD => freethread(L, @ptrCast(@alignCast(obj))),
        o.T_STRING => lstring.free(L, @ptrCast(obj)),
        o.T_USERDATA => freeudata(L, @ptrCast(@alignCast(obj))),
        o.T_BUFFER => {
            const b: *o.Buffer = @ptrCast(@alignCast(obj));
            mem.freeGco(L, b, o.Buffer.header + b.len, b.gch.memcat);
        },
        o.T_CLASS => @import("class.zig").freeclass(L, @ptrCast(@alignCast(obj))),
        o.T_OBJECT => @import("class.zig").freeobject(L, @ptrCast(@alignCast(obj))),
        else => unreachable,
    }
}

const StrResizeCtx = struct { n: c_int };
fn strResizeBody(L: *State, ud: ?*anyopaque) void {
    const c: *StrResizeCtx = @ptrCast(@alignCast(ud.?));
    lstring.resize(L, c.n);
}

fn stringresizeprotected(L: *State, newsize: c_int) void {
    var ctx: StrResizeCtx = .{ .n = newsize };
    _ = ldo.rawrunprotected(L, strResizeBody, &ctx);
}

fn shrinkbuffers(L: *State) void {
    const g = L.global;
    if (g.strt.nuse < @as(u32, @intCast(@divTrunc(g.strt.size, 4))) and g.strt.size > o.LUA_MINSTRTABSIZE * 2)
        stringresizeprotected(L, @divTrunc(g.strt.size, 2));
}

fn shrinkbuffersfull(L: *State) void {
    const g = L.global;
    var hashsize = g.strt.size;
    while (g.strt.nuse < @as(u32, @intCast(@divTrunc(hashsize, 4))) and hashsize > o.LUA_MINSTRTABSIZE * 2) hashsize = @divTrunc(hashsize, 2);
    if (hashsize != g.strt.size) stringresizeprotected(L, hashsize);
}

/// Frees every object (lua_close).
pub fn freeall(L: *State) void {
    const g = L.global;
    g.gcinprogress = true;
    while (g.allgc) |obj| {
        g.allgc = obj.next;
        freeobj(L, obj);
    }
}

// ---------------------------------------------------------------------------
// Collection phases
// ---------------------------------------------------------------------------

fn markmt(g: *Global) void {
    for (g.mt) |mt| markopt(g, mt);
}
fn marktaggetmt(g: *Global) void {
    for (g.udatamt) |mt| markopt(g, mt);
}
fn markudatadirectfields(g: *Global) void {
    for (g.udatadirectfields) |t| markopt(g, t);
}
fn markudatadirectaccess(g: *Global) void {
    for (&g.udatadirect) |*d| {
        markvalue(g, &d.indextm);
        markvalue(g, &d.newindextm);
        markvalue(g, &d.namecalltm);
    }
}
fn markfastpcalls(g: *Global) void {
    markopt(g, g.builtinPcall);
    markopt(g, g.builtinXpcall);
}

fn embeddermarkref(Lopt: ?*State, ref: c_int) callconv(.c) void {
    const L = Lopt.?;
    if (ref <= o.LUA_REFNIL) return;
    const g = L.global;
    if (!o.ttistable(&g.weakregistry)) return;
    const slot = table.getnum(o.hvalue(&g.weakregistry), ref);
    if (o.iscollectable(slot) and iswhite(slot.value.gc)) reallymarkobject(g, slot.value.gc);
}

fn markroot(L: *State) void {
    const g = L.global;
    g.gray = null;
    g.grayagain = null;
    g.weak = null;
    markobject(g, &g.mainthread.gch);
    markobject(g, &g.mainthread.gt.gch);
    markvalue(g, &g.registry);
    markvalue(g, &g.weakregistry);
    if (g.embeddergc) |eg| eg(g.mainthread, null);
    markudatadirectaccess(g);
    markudatadirectfields(g);
    markfastpcalls(g);
    markmt(g);
    marktaggetmt(g);
    g.gcstate = GCSpropagate;
}

fn remarkupvals(g: *Global) usize {
    var work: usize = 0;
    var uv = g.uvhead.next.?;
    while (uv != &g.uvhead) : (uv = uv.next.?) {
        work += @sizeOf(o.UpVal);
        if (isgray(&uv.gch)) markvalue(g, uv.v);
    }
    return work;
}

fn clearupvals(L: *State) usize {
    const g = L.global;
    var work: usize = 0;
    var uv = g.uvhead.next.?;
    while (uv != &g.uvhead) {
        work += @sizeOf(o.UpVal);
        const next = uv.next.?;
        if (uv.markedopen != 0) {
            uv.markedopen = 0;
        } else {
            // the owning thread is dead: drop the upvalue from the dead thread's
            // list and close it unless the upvalue itself is dead too
            func.closeupval(L, uv, iswhite(&uv.gch));
        }
        uv = next;
    }
    return work;
}

fn atomic(L: *State) usize {
    const g = L.global;
    var work: usize = 0;
    work += remarkupvals(g);
    work += propagateall(g);
    g.gray = g.weak;
    g.weak = null;
    markobject(g, &L.gch);
    markmt(g);
    marktaggetmt(g);
    markudatadirectaccess(g);
    markudatadirectfields(g);
    markfastpcalls(g);
    work += propagateall(g);
    g.gray = g.grayagain;
    g.grayagain = null;
    work += propagateall(g);
    if (g.embeddergc) |eg| {
        eg(g.mainthread, embeddermarkref);
        while (g.gray != null) {
            work += propagateall(g);
            eg(g.mainthread, embeddermarkref);
        }
    }
    work += cleartable(L, g.weak);
    g.weak = null;
    work += clearupvals(L);
    g.currentwhite = otherwhite(g);
    g.sweepcursor = &g.allgc;
    g.gcstate = GCSsweep;
    return work;
}

fn sweepstep(L: *State, limit: usize) usize {
    const g = L.global;
    const deadmask = otherwhite(g);
    const newwhite = luaC_white(g);
    var cost: usize = 0;
    var p = g.sweepcursor.?;
    while (p.*) |obj| {
        if (cost >= limit) break;
        cost += GC_SWEEPPAGESTEPCOST;
        if ((obj.marked ^ WHITEBITS) & deadmask != 0) {
            obj.marked = (obj.marked & maskmarks) | newwhite;
            p = &obj.next;
        } else {
            p.* = obj.next;
            g.sweepcursor = p;
            freeobj(L, obj);
        }
    }
    g.sweepcursor = p;
    if (p.* == null) {
        g.sweepcursor = null;
        makewhite(g, &g.mainthread.gch);
        shrinkbuffers(L);
        g.gcstate = GCSpause;
    }
    return cost;
}

fn gcstep(L: *State, limit: usize) usize {
    const g = L.global;
    var cost: usize = 0;
    switch (g.gcstate) {
        GCSpause => markroot(L),
        GCSpropagate => {
            while (g.gray != null and cost < limit) cost += propagatemark(g);
            if (g.gray == null) {
                g.gray = g.grayagain;
                g.grayagain = null;
                g.gcstate = GCSpropagateagain;
            }
        },
        GCSpropagateagain => {
            while (g.gray != null and cost < limit) cost += propagatemark(g);
            if (g.gray == null) g.gcstate = GCSatomic;
        },
        GCSatomic => {
            g.gcstats.atomicstarttimestamp = clock();
            g.gcstats.atomicstarttotalsizebytes = g.totalbytes;
            cost = atomic(L);
        },
        GCSsweep => cost = sweepstep(L, limit),
        else => unreachable,
    }
    return cost;
}

fn clock() f64 {
    return @import("api.zig").lua_clock();
}

fn getheaptriggererroroffset(g: *Global) i64 {
    const diff: i64 = @as(i64, @intCast(g.gcstats.atomicstarttotalsizebytes)) - @as(i64, @intCast(g.gcstats.heapgoalsizebytes));
    const errorKb: i32 = @truncate(@divTrunc(diff, 1024));
    const slot = &g.gcstats.triggerterms[g.gcstats.triggertermpos % 32];
    const prev = slot.*;
    slot.* = errorKb;
    g.gcstats.triggerintegral +%= errorKb -% prev;
    g.gcstats.triggertermpos +%= 1;
    const Ku = 0.9;
    const Tu = 2.5;
    const Kp = 0.45 * Ku;
    const Ti = 0.8 * Tu;
    const Ki = 0.54 * Ku / Ti;
    const total = Kp * @as(f64, @floatFromInt(errorKb)) + Ki * @as(f64, @floatFromInt(g.gcstats.triggerintegral));
    return @intFromFloat(total * 1024);
}

fn getheapgrowth(current: usize, previous: usize) usize {
    return if (current > previous) current - previous else 0;
}

fn getheaptrigger(g: *Global, heapgoal: usize) usize {
    const durationthreshold = 1e-3;
    const allocationduration = g.gcstats.atomicstarttimestamp - g.gcstats.endtimestamp;
    if (allocationduration < durationthreshold) return heapgoal;
    const growth: f64 = if (flags.LuauGcHeapShrinkFix)
        @floatFromInt(getheapgrowth(g.gcstats.atomicstarttotalsizebytes, g.gcstats.endtotalsizebytes))
    else
        @floatFromInt(g.gcstats.atomicstarttotalsizebytes -% g.gcstats.endtotalsizebytes);
    const allocrate = growth / allocationduration;
    const markduration = g.gcstats.atomicstarttimestamp - g.gcstats.starttimestamp;
    const expectedgrowth: i64 = @intFromFloat(std.math.clamp(markduration * allocrate, -9.0e18, 9.0e18));
    const offset = getheaptriggererroroffset(g);
    const heaptrigger: i64 = @as(i64, @intCast(heapgoal)) -% (expectedgrowth +% offset);
    if (heaptrigger < @as(i64, @intCast(g.totalbytes))) return g.totalbytes;
    if (heaptrigger > @as(i64, @intCast(heapgoal))) return heapgoal;
    return @intCast(heaptrigger);
}

inline fn interrupt(L: *State, state: c_int) void {
    if (L.global.cb.interrupt) |cb| cb(L, state);
}

/// luaC_step: performs one incremental step; returns the amount of work in bytes.
pub fn step(L: *State, assist: bool) usize {
    const g = L.global;
    if (g.gcinprogress) return 0;
    g.gcinprogress = true;
    defer g.gcinprogress = false;
    const stepmul: usize = @intCast(@max(g.gcstepmul, 1));
    var lim: usize = @as(usize, @intCast(@max(g.gcstepsize, 0))) * stepmul / 100;
    const debt: usize = g.totalbytes -| g.GCthreshold;
    interrupt(L, 0);
    if (g.gcstate == GCSpause) g.gcstats.starttimestamp = clock();
    if (assist) {
        const need = debt * stepmul / 100;
        if (need > lim) lim = need;
    }
    const lastgcstate = g.gcstate;
    const work = gcstep(L, lim);
    const actualstepsize = work * 100 / stepmul;
    if (g.gcstate == GCSpause) {
        const heapgoal = (g.totalbytes / 100) * @as(usize, @intCast(@max(g.gcgoal, 0)));
        g.GCthreshold = getheaptrigger(g, heapgoal);
        g.gcstats.heapgoalsizebytes = heapgoal;
        g.gcstats.endtimestamp = clock();
        g.gcstats.endtotalsizebytes = g.totalbytes;
    } else {
        g.GCthreshold = g.totalbytes +| actualstepsize;
        if (g.GCthreshold >= debt) g.GCthreshold -= debt;
    }
    interrupt(L, lastgcstate);
    return actualstepsize;
}

pub inline fn checkGC(L: *State) void {
    if (L.global.totalbytes >= L.global.GCthreshold) _ = step(L, true);
}

/// luaC_fullgc
pub fn fullgc(L: *State) void {
    const g = L.global;
    if (g.gcinprogress) return;
    g.gcinprogress = true;
    defer g.gcinprogress = false;
    if (keepinvariant(g)) {
        g.sweepcursor = &g.allgc;
        g.gray = null;
        g.grayagain = null;
        g.weak = null;
        g.gcstate = GCSsweep;
    }
    while (g.gcstate != GCSpause) _ = gcstep(L, std.math.maxInt(usize));
    var uv = g.uvhead.next.?;
    while (uv != &g.uvhead) : (uv = uv.next.?) uv.markedopen = 0;
    markroot(L);
    while (g.gcstate != GCSpause) _ = gcstep(L, std.math.maxInt(usize));
    shrinkbuffersfull(L);
    const goal: usize = @intCast(@max(g.gcgoal, 0));
    const stepmul: usize = @intCast(@max(g.gcstepmul, 1));
    const heapgoalsizebytes = (g.totalbytes / 100) * goal;
    const factor: i64 = @as(i64, @intCast(goal * stepmul / 100)) - 100;
    const thr: i64 = @divTrunc(@as(i64, @intCast(g.totalbytes)) * factor, @as(i64, @intCast(stepmul)));
    g.GCthreshold = if (thr < @as(i64, @intCast(g.totalbytes))) g.totalbytes else @intCast(thr);
    g.gcstats.heapgoalsizebytes = heapgoalsizebytes;
    if (flags.LuauGcHeapShrinkFix) {
        g.gcstats.endtimestamp = clock();
        g.gcstats.endtotalsizebytes = g.totalbytes;
    }
}

pub fn allocationrate(L: *State) i64 {
    const g = L.global;
    const durationthreshold = 1e-3;
    if (g.gcstate <= GCSatomic) {
        const duration = clock() - g.gcstats.endtimestamp;
        if (duration < durationthreshold) return -1;
        const growth: f64 = if (flags.LuauGcHeapShrinkFix) @floatFromInt(getheapgrowth(g.totalbytes, g.gcstats.endtotalsizebytes)) else @floatFromInt(g.totalbytes -% g.gcstats.endtotalsizebytes);
        return @intFromFloat(@min(growth / duration, 9.0e18));
    }
    const duration = g.gcstats.atomicstarttimestamp - g.gcstats.endtimestamp;
    if (duration < durationthreshold) return -1;
    const growth: f64 = if (flags.LuauGcHeapShrinkFix) @floatFromInt(getheapgrowth(g.gcstats.atomicstarttotalsizebytes, g.gcstats.endtotalsizebytes)) else @floatFromInt(g.gcstats.atomicstarttotalsizebytes -% g.gcstats.endtotalsizebytes);
    return @intFromFloat(@min(growth / duration, 9.0e18));
}

// ---------------------------------------------------------------------------
// Barriers
// ---------------------------------------------------------------------------

fn barrierf(L: *State, obj: *GCObject, v: *GCObject) void {
    const g = L.global;
    if (keepinvariant(g)) reallymarkobject(g, v) else makewhite(g, obj);
}

fn barriertable(L: *State, t: *o.Table, v: *GCObject) void {
    const g = L.global;
    const obj = &t.gch;
    if (g.gcstate == GCSpropagateagain) {
        reallymarkobject(g, v);
        return;
    }
    black2gray(obj);
    obj.gclist = g.grayagain;
    g.grayagain = obj;
}

fn barrierback(L: *State, obj: *GCObject) void {
    const g = L.global;
    black2gray(obj);
    obj.gclist = g.grayagain;
    g.grayagain = obj;
}

/// luaC_barrier: `p` now references value `v`.
pub inline fn barrier(L: *State, p: *GCObject, v: *const TValue) void {
    if (o.iscollectable(v) and isblack(p) and iswhite(v.value.gc)) barrierf(L, p, v.value.gc);
}

/// luaC_barriert: table `t` now references value `v`.
pub inline fn barriert(L: *State, t: *o.Table, v: *const TValue) void {
    if (o.iscollectable(v) and isblack(&t.gch) and iswhite(v.value.gc)) barriertable(L, t, v.value.gc);
}

/// luaC_objbarrier: `p` now references object `obj`.
pub inline fn objbarrier(L: *State, p: *GCObject, obj: *GCObject) void {
    if (isblack(p) and iswhite(obj)) barrierf(L, p, obj);
}

/// luaC_barrierfast: table received many writes (SETLIST).
pub inline fn barrierfast(L: *State, t: *o.Table) void {
    if (isblack(&t.gch)) barrierback(L, &t.gch);
}

/// luaC_threadbarrier: about to write to L's stack outside of execution.
pub inline fn threadbarrier(L: *State) void {
    if (isblack(&L.gch)) barrierback(L, &L.gch);
}

/// luaC_upvalclosed
pub fn upvalclosed(L: *State, uv: *o.UpVal) void {
    const g = L.global;
    const obj = &uv.gch;
    if (isgray(obj)) {
        if (keepinvariant(g)) {
            gray2black(obj);
            barrier(L, obj, uv.v);
        } else {
            makewhite(g, obj);
        }
    }
}

pub fn statename(state: c_int) [*:0]const u8 {
    return switch (state) {
        GCSpause => "pause",
        GCSpropagate => "mark",
        GCSpropagateagain => "remark",
        GCSatomic => "atomic",
        GCSsweep => "sweep",
        else => "",
    };
}
