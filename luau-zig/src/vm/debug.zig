//! Runtime errors and debug information (port of upstream ldebug.cpp and the
//! luaO_chunkid helper, MIT; see object.zig header).

const std = @import("std");
const o = @import("object.zig");
const ldo = @import("do.zig");
const tm = @import("tm.zig");
const lstring = @import("string.zig");
const func = @import("func.zig");

const State = o.State;
const TValue = o.TValue;
const CallInfo = o.CallInfo;
const Proto = o.Proto;

pub fn pcRel(pc: ?[*]const u32, p: *const Proto) c_int {
    const cur = pc orelse return 0;
    if (cur == @as([*]const u32, p.code)) return 0;
    return o.idiff(cur, @as([*]const u32, p.code)) - 1;
}

pub fn getline(p: *const Proto, pc: c_int) c_int {
    const li = p.lineinfo orelse return 0;
    const upc: usize = @intCast(pc);
    return p.abslineinfo.?[upc >> @intCast(p.linegaplog2)] + li[upc];
}

pub fn currentpc(ci: *const CallInfo) c_int {
    return pcRel(ci.savedpc, ci.p.?);
}

pub fn currentline(ci: *const CallInfo) c_int {
    return getline(ci.p.?, currentpc(ci));
}

/// luaO_chunkid: writes a printable chunk name into `buf` (size LUA_IDSIZE) and
/// returns the resulting slice (which may alias `source`).
pub fn chunkid(buf: []u8, source: []const u8) []const u8 {
    const buflen = buf.len;
    if (source.len > 0 and source[0] == '=') {
        if (source.len <= buflen) return source[1..];
        @memcpy(buf[0 .. buflen - 1], source[1..buflen]);
        return buf[0 .. buflen - 1];
    } else if (source.len > 0 and source[0] == '@') {
        if (source.len <= buflen) return source[1..];
        @memcpy(buf[0..3], "...");
        const keep = buflen - 4;
        @memcpy(buf[3 .. 3 + keep], source[source.len - keep ..]);
        return buf[0 .. buflen - 1];
    } else {
        var len: usize = 0;
        while (len < source.len and source[len] != '\n' and source[len] != '\r' and source[len] != 0) len += 1;
        const room = buflen - "[string \"...\"]".len - 1;
        const truncated = (len < source.len and source[len] != 0) or len > room;
        if (len > room) len = room;
        var w: usize = 0;
        const pre = "[string \"";
        @memcpy(buf[w .. w + pre.len], pre);
        w += pre.len;
        @memcpy(buf[w .. w + len], source[0..len]);
        w += len;
        if (truncated) {
            @memcpy(buf[w .. w + 3], "...");
            w += 3;
        }
        @memcpy(buf[w .. w + 2], "\"]");
        w += 2;
        return buf[0..w];
    }
}

/// Formats into a LUA_BUFFERSIZE scratch buffer with vsnprintf-like truncation.
pub fn fmtbuf(buf: *[o.LUA_BUFFERSIZE]u8, comptime fmt: []const u8, args: anytype) []const u8 {
    var w: std.Io.Writer = .fixed(buf[0 .. o.LUA_BUFFERSIZE - 1]);
    w.print(fmt, args) catch {};
    return w.buffered();
}

fn pusherror(L: *State, msg: []const u8) void {
    const ci = L.ci;
    if (o.isLua(ci)) {
        const source = ci.p.?.source.?;
        var chunkbuf: [o.LUA_IDSIZE]u8 = undefined;
        const id = chunkid(&chunkbuf, source.slice());
        const line = currentline(ci);
        var out: [o.LUA_BUFFERSIZE]u8 = undefined;
        const s = fmtbuf(&out, "{s}:{d}: {s}", .{ id, line, msg });
        ldo.rawcheckstack(L, 1);
        o.setsvalue(&L.top[0], lstring.new(L, s));
        L.top += 1;
    } else {
        ldo.rawcheckstack(L, 1);
        o.setsvalue(&L.top[0], lstring.new(L, msg));
        L.top += 1;
    }
}

pub fn pusherrorstr(L: *State, msg: []const u8) void {
    pusherror(L, msg);
}

pub fn runerror(L: *State, comptime fmt: []const u8, args: anytype) noreturn {
    var buf: [o.LUA_BUFFERSIZE]u8 = undefined;
    const msg = fmtbuf(&buf, fmt, args);
    pusherror(L, msg);
    ldo.throw(L, o.LUA_ERRRUN);
}

pub fn typeerror(L: *State, obj: *const TValue, op: []const u8) noreturn {
    runerror(L, "attempt to {s} a {s} value", .{ op, tm.objtypename(L, obj) });
}

pub fn forerror(L: *State, obj: *const TValue, what: []const u8) noreturn {
    runerror(L, "invalid 'for' {s} (number expected, got {s})", .{ what, tm.objtypename(L, obj) });
}

pub fn concaterror(L: *State, p1: *const TValue, p2: *const TValue) noreturn {
    runerror(L, "attempt to concatenate {s} with {s}", .{ tm.objtypename(L, p1), tm.objtypename(L, p2) });
}

pub fn aritherror(L: *State, p1: *const TValue, p2: *const TValue, op: o.TMS) noreturn {
    const t1 = tm.objtypenamestr(L, p1);
    const t2 = tm.objtypenamestr(L, p2);
    const opname = o.eventnames[@intFromEnum(op)][2..];
    if (t1 == t2)
        runerror(L, "attempt to perform arithmetic ({s}) on {s}", .{ opname, t1.slice() })
    else
        runerror(L, "attempt to perform arithmetic ({s}) on {s} and {s}", .{ opname, t1.slice(), t2.slice() });
}

pub fn ordererror(L: *State, p1: *const TValue, p2: *const TValue, op: o.TMS) noreturn {
    const opname: []const u8 = if (op == .LT) "<" else if (op == .LE) "<=" else "==";
    runerror(L, "attempt to compare {s} {s} {s}", .{ tm.objtypename(L, p1), opname, tm.objtypename(L, p2) });
}

pub fn indexerror(L: *State, p1: *const TValue, p2: *const TValue) noreturn {
    const t1 = tm.objtypename(L, p1);
    if (o.ttisstring(p2) and o.tsvalue(p2).len <= 64) {
        runerror(L, "attempt to index {s} with '{s}'", .{ t1, std.mem.sliceTo(o.tsvalue(p2).slice(), 0) });
    }
    runerror(L, "attempt to index {s} with {s}", .{ t1, tm.objtypename(L, p2) });
}

pub fn methoderror(L: *State, p1: *const TValue, name: *const o.TString) noreturn {
    runerror(L, "attempt to call missing method '{s}' of {s}", .{ std.mem.sliceTo(name.slice(), 0), tm.objtypename(L, p1) });
}

pub fn readonlyerror(L: *State) noreturn {
    runerror(L, "attempt to modify a readonly table", .{});
}

// ---------------------------------------------------------------------------
// lua_getinfo and friends
// ---------------------------------------------------------------------------

fn getfuncname(cl: *const o.Closure) ?[*:0]const u8 {
    if (cl.isC != 0) {
        if (cl.debugname) |s| return s.cstr();
    } else if (cl.p.?.debugname) |s| return s.cstr();
    return null;
}

fn auxgetinfo(L: *State, what: [*:0]const u8, ar: *o.lua_Debug, f: *o.Closure, ci: ?*CallInfo) ?*o.Closure {
    _ = L;
    var cl: ?*o.Closure = null;
    var w = what;
    while (w[0] != 0) : (w += 1) {
        switch (w[0]) {
            's' => {
                if (f.isC != 0) {
                    ar.source = "=[C]";
                    ar.what = "C";
                    ar.linedefined = -1;
                    ar.short_src = "[C]";
                } else {
                    const p = if (ci) |c| c.p.? else f.p.?;
                    const source = p.source.?;
                    ar.source = source.cstr();
                    ar.what = "Lua";
                    ar.linedefined = p.linedefined;
                    const id = chunkid(ar.ssbuf[0..], source.slice());
                    if (id.ptr == @as([*]const u8, &ar.ssbuf)) {
                        ar.ssbuf[id.len] = 0;
                        ar.short_src = @ptrCast(&ar.ssbuf);
                    } else {
                        // chunkid returned a suffix of the NUL-terminated source string
                        ar.short_src = @ptrCast(id.ptr);
                    }
                }
            },
            'l' => {
                if (ci) |c| {
                    ar.currentline = if (o.isLua(c)) currentline(c) else -1;
                } else {
                    ar.currentline = if (f.isC != 0) -1 else f.p.?.linedefined;
                }
            },
            'u' => ar.nupvals = f.nupvalues,
            'a' => {
                if (f.isC != 0) {
                    ar.isvararg = 1;
                    ar.nparams = 0;
                } else {
                    const p = if (ci) |c| c.p.? else f.p.?;
                    ar.isvararg = p.is_vararg;
                    ar.nparams = p.numparams;
                }
            },
            'p' => {
                if (f.isC != 0) {
                    ar.protoid = 0;
                    ar.bytecodeid = -1;
                } else {
                    const p = if (ci) |c| c.p.? else f.p.?;
                    ar.protoid = @bitCast(p.funid);
                    ar.bytecodeid = p.bytecodeid;
                }
            },
            'n' => ar.name = if (ci) |c| getfuncname(o.ci_func(c)) else getfuncname(f),
            'f' => cl = f,
            else => {},
        }
    }
    return cl;
}

pub fn getinfo(L: *State, level: c_int, what: [*:0]const u8, ar: *o.lua_Debug) c_int {
    var f: ?*o.Closure = null;
    var ci: ?*CallInfo = null;
    const depth = o.idiff(@as([*]CallInfo, @ptrCast(L.ci)), L.base_ci);
    if (level < 0) {
        if (-level > o.idiff(L.top, L.base)) return 0;
        const fv = &o.offs(L.top, level)[0];
        if (!o.ttisfunction(fv)) return 0;
        f = o.clvalue(fv);
    } else if (level < depth) {
        const c = &(@as([*]CallInfo, @ptrCast(L.ci)) - @as(usize, @intCast(level)))[0];
        ci = c;
        f = o.clvalue(&c.func[0]);
    }
    if (f) |fun| {
        if (auxgetinfo(L, what, ar, fun, ci)) |fcl| {
            @import("gc.zig").threadbarrier(L);
            ldo.checkstack(L, 1);
            o.setclvalue(&L.top[0], fcl);
            ldo.incrTop(L);
        }
        return 1;
    }
    return 0;
}

pub fn stackdepth(L: *State) c_int {
    return o.idiff(@as([*]CallInfo, @ptrCast(L.ci)), L.base_ci);
}

fn ciAtLevel(L: *State, level: c_int) ?*CallInfo {
    if (level < 0 or level >= stackdepth(L)) return null;
    return &(@as([*]CallInfo, @ptrCast(L.ci)) - @as(usize, @intCast(level)))[0];
}

pub fn getargument(L: *State, level: c_int, n: c_int) c_int {
    const ci = ciAtLevel(L, level) orelse return 0;
    const fp = ci.p orelse return 0;
    if (n <= 0) return 0;
    if (n <= fp.numparams) {
        ldo.pushvalue(L, &ci.base[@intCast(n - 1)]);
        return 1;
    } else if (fp.is_vararg != 0 and n < o.idiff(ci.base, ci.func)) {
        ldo.pushvalue(L, &ci.func[@intCast(n)]);
        return 1;
    }
    return 0;
}

pub fn getlocal(L: *State, level: c_int, n: c_int) ?[*:0]const u8 {
    const ci = ciAtLevel(L, level) orelse return null;
    const fp = ci.p orelse return null;
    const v = func.getlocal(fp, n, currentpc(ci)) orelse return null;
    ldo.pushvalue(L, &ci.base[v.reg]);
    return v.varname.?.cstr();
}

pub fn setlocal(L: *State, level: c_int, n: c_int) ?[*:0]const u8 {
    const ci = ciAtLevel(L, level) orelse return null;
    var name: ?[*:0]const u8 = null;
    if (ci.p) |fp| {
        if (func.getlocal(fp, n, currentpc(ci))) |v| {
            ci.base[v.reg] = (L.top - 1)[0];
            name = v.varname.?.cstr();
        }
    }
    L.top -= 1;
    return name;
}

/// luau_callhook: invokes a debug hook with the thread state prepared for calls.
pub fn callhook(L: *State, hook: o.lua_Hook, userdata: ?*anyopaque) void {
    const base = o.sdiff(L.base, L.stack);
    const top = o.sdiff(L.top, L.stack);
    const ci_top = o.sdiff(L.ci.top, L.stack);
    const status = L.status;
    if (status == o.LUA_YIELD or status == o.LUA_BREAK) {
        L.status = 0;
        L.base = L.ci.base;
    }
    const cl = o.clvalue(&L.ci.func[0]);
    const oldsavedpc = L.ci.savedpc;
    if (L.ci.savedpc) |pc| {
        const p = L.ci.p.?;
        if (pc != @as([*]const u32, p.code) + @as(usize, @intCast(p.sizecode))) L.ci.savedpc = pc + 1;
    }
    ldo.checkstack(L, o.LUA_MINSTACK);
    L.ci.top = L.top + o.LUA_MINSTACK;
    var ar: o.lua_Debug = .{};
    ar.currentline = if (cl.isC != 0) -1 else getline(L.ci.p.?, pcRel(L.ci.savedpc, L.ci.p.?));
    ar.userdata = userdata;
    hook(L, &ar);
    L.ci.savedpc = oldsavedpc;
    L.ci.top = o.offs(L.stack, ci_top);
    L.top = o.offs(L.stack, top);
    if (status == o.LUA_YIELD and L.status != o.LUA_YIELD) {
        L.status = o.LUA_YIELD;
        L.base = o.offs(L.stack, base);
    } else if (status == o.LUA_BREAK) {
        L.status = o.LUA_BREAK;
        L.base = o.offs(L.stack, base);
    }
}

pub fn debugtrace(L: *State) [*:0]const u8 {
    const S = struct {
        var buf: [4096]u8 = undefined;
    };
    const limit1 = 10;
    const limit2 = 10;
    const depth = stackdepth(L);
    var w: std.Io.Writer = .fixed(S.buf[0 .. S.buf.len - 1]);
    var ar: o.lua_Debug = .{};
    var level: c_int = 0;
    while (getinfo(L, level, "sln", &ar) != 0) : (level += 1) {
        if (ar.source != null) w.writeAll(std.mem.span(ar.short_src.?)) catch {};
        if (ar.currentline > 0) w.print(":{d}", .{ar.currentline}) catch {};
        if (ar.name) |n| {
            w.writeAll(" function ") catch {};
            w.writeAll(std.mem.span(n)) catch {};
        }
        w.writeAll("\n") catch {};
        if (depth > limit1 + limit2 and level == limit1 - 1) {
            w.print("... (+{d} frames)\n", .{depth - limit1 - limit2}) catch {};
            level = depth - limit2 - 1;
        }
    }
    const n = w.buffered().len;
    S.buf[n] = 0;
    return @ptrCast(&S.buf);
}

// ---------------------------------------------------------------------------
// Breakpoints and coverage (upstream ldebug.cpp)
// ---------------------------------------------------------------------------

const OP_BREAK: u8 = 1;
const OP_PREPVARARGS: u8 = 65;
const OP_COVERAGE: u8 = 69;

pub fn breakpoint(L: *State, p: *Proto, line: c_int, enable: bool) void {
    if (p.lineinfo != null) {
        var i: usize = 0;
        while (i < @as(usize, @intCast(p.sizecode))) : (i += 1) {
            if (@as(u8, @truncate(p.code[i])) == OP_PREPVARARGS) continue;
            if (getline(p, @intCast(i)) != line) continue;
            if (p.debuginsn == null) {
                const n: usize = @intCast(p.sizecode);
                const d = @import("mem.zig").newArray(L, u8, n, p.gch.memcat);
                for (0..n) |j| d[j] = @truncate(p.code[j]);
                p.debuginsn = d;
            }
            const op: u8 = if (enable) OP_BREAK else p.debuginsn.?[i];
            p.code[i] = (p.code[i] & ~@as(u32, 0xff)) | op;
            break;
        }
    }
    for (0..@intCast(p.sizep)) |i| breakpoint(L, p.p[i], line, enable);
}

pub fn onbreak(L: *State) bool {
    if (L.ci == &L.base_ci[0]) return false;
    if (!o.isLua(L.ci)) return false;
    const pc = L.ci.savedpc orelse return false;
    return @as(u8, @truncate(pc[0])) == OP_BREAK;
}

fn getmaxline(p: *Proto) c_int {
    var result: c_int = -1;
    for (0..@intCast(p.sizecode)) |i| result = @max(result, getline(p, @intCast(i)));
    for (0..@intCast(p.sizep)) |i| result = @max(result, getmaxline(p.p[i]));
    return result;
}

fn getnextline(p: *Proto, line: c_int) c_int {
    var closest: c_int = -1;
    if (p.lineinfo != null) {
        for (0..@intCast(p.sizecode)) |i| {
            if (@as(u8, @truncate(p.code[i])) == OP_PREPVARARGS) continue;
            const candidate = getline(p, @intCast(i));
            if (candidate == line) return line;
            if (candidate > line and (closest == -1 or candidate < closest)) closest = candidate;
        }
    }
    for (0..@intCast(p.sizep)) |i| {
        const candidate = getnextline(p.p[i], line);
        if (candidate == line) return line;
        if (candidate > line and (closest == -1 or candidate < closest)) closest = candidate;
    }
    return closest;
}

pub fn setbreakpoint(L: *State, p: *Proto, line: c_int, enabled: bool) c_int {
    const target = getnextline(p, line);
    if (target != -1) breakpoint(L, p, target, enabled);
    return target;
}

pub const lua_Coverage = *const fn (?*anyopaque, ?[*:0]const u8, c_int, c_int, [*]const c_int, usize) callconv(.c) void;

fn getcoverageRec(p: *Proto, depth: c_int, buffer: [*]c_int, size: usize, context: ?*anyopaque, callback: lua_Coverage) void {
    for (0..size) |i| buffer[i] = -1;
    for (0..@intCast(p.sizecode)) |i| {
        const insn = p.code[i];
        if (@as(u8, @truncate(insn)) != OP_COVERAGE) continue;
        const line: usize = @intCast(getline(p, @intCast(i)));
        const hits: c_int = @as(i32, @bitCast(insn)) >> 8;
        if (line < size and buffer[line] < hits) buffer[line] = hits;
    }
    const name: ?[*:0]const u8 = if (p.debugname) |d| d.cstr() else null;
    callback(context, name, p.linedefined, depth, buffer, size);
    for (0..@intCast(p.sizep)) |i| getcoverageRec(p.p[i], depth + 1, buffer, size, context, callback);
}

pub fn getcoverage(L: *State, p: *Proto, context: ?*anyopaque, callback: lua_Coverage) void {
    const maxline = getmaxline(p);
    const size: usize = @intCast(maxline + 1);
    if (size == 0) return;
    const mem = @import("mem.zig");
    const buffer = mem.newArray(L, c_int, size, 0);
    getcoverageRec(p, 0, buffer, size, context, callback);
    mem.freeArray(L, c_int, buffer, size, 0);
}
