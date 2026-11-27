//! Stack and call management, protected execution, coroutines (port of upstream
//! ldo.cpp plus luau_precall/luau_poscall/luau_finishop from lvmexecute.cpp, MIT;
//! see object.zig header). Matches upstream with all feature flags at their
//! default (off) values, which is the configuration used by production.

const std = @import("std");
const o = @import("object.zig");
const jmp = @import("jmp.zig");
const mem = @import("mem.zig");
const debug = @import("debug.zig");
const func = @import("func.zig");
const gc = @import("gc.zig");
const lstring = @import("string.zig");
const vmu = @import("vmutils.zig");
const exec = @import("execute.zig");
const flags = @import("flags.zig");

const State = o.State;
const TValue = o.TValue;
const StkId = o.StkId;
const CallInfo = o.CallInfo;
const Closure = o.Closure;

pub const Pfunc = *const fn (*State, ?*anyopaque) void;

pub const PCRLUA = 0;
pub const PCRC = 1;
pub const PCRYIELD = 2;
pub const C_CALL_YIELD: c_int = -1;

// ---------------------------------------------------------------------------
// Error recovery
// ---------------------------------------------------------------------------

const ProtCtx = struct {
    L: *State,
    f: Pfunc,
    ud: ?*anyopaque,
};

fn trampoline(p: ?*anyopaque) callconv(.c) void {
    const c: *ProtCtx = @ptrCast(@alignCast(p.?));
    c.f(c.L, c.ud);
}

pub noinline fn rawrunprotected(L: *State, f: Pfunc, ud: ?*anyopaque) c_int {
    const g = L.global;
    // GC callbacks can raise through the VM's non-local jump, bypassing Zig
    // defers in gc.step/fullgc. Nested protected calls inside the collector
    // must retain the guard; an outer handler restores the pre-call state.
    const was_collecting = g.gcinprogress;
    var lj: o.LongJmp = .{ .prev = g.errorjmp, .status = 0, .buf = .{} };
    g.errorjmp = &lj;
    var ctx: ProtCtx = .{ .L = L, .f = f, .ud = ud };
    _ = jmp.protect(&lj.buf, trampoline, &ctx);
    const vlj: *volatile o.LongJmp = &lj;
    g.gcinprogress = was_collecting;
    g.errorjmp = vlj.prev;
    return vlj.status;
}

pub fn throw(L: *State, errcode: c_int) noreturn {
    if (L.global.errorjmp) |lj| {
        const vlj: *volatile o.LongJmp = lj;
        vlj.status = errcode;
        jmp.jump(&lj.buf);
    }
    if (L.global.cb.panic) |p| p(L, errcode);
    @trap();
}

// ---------------------------------------------------------------------------
// Stack management
// ---------------------------------------------------------------------------

pub inline fn savestack(L: *State, p: StkId) isize {
    return o.sdiff(p, L.stack);
}
pub inline fn restorestack(L: *State, n: isize) StkId {
    return o.offs(L.stack, n);
}
pub inline fn saveci(L: *State, ci: *CallInfo) isize {
    return o.sdiff(@as([*]CallInfo, @ptrCast(ci)), L.base_ci);
}
pub inline fn restoreci(L: *State, n: isize) *CallInfo {
    return &L.base_ci[@intCast(n)];
}

fn correctstack(L: *State, oldstack: StkId) void {
    const newstack = L.stack;
    const fix = struct {
        inline fn f(p: StkId, old: StkId, new: StkId) StkId {
            return o.offs(new, o.sdiff(p, old));
        }
    }.f;
    L.top = fix(L.top, oldstack, newstack);
    var up = L.openupval;
    while (up) |uv| : (up = uv.threadnext) {
        uv.v = &fix(o.toStk(uv.v), oldstack, newstack)[0];
    }
    var ci: [*]CallInfo = L.base_ci;
    while (@intFromPtr(ci) <= @intFromPtr(L.ci)) : (ci += 1) {
        ci[0].top = fix(ci[0].top, oldstack, newstack);
        ci[0].base = fix(ci[0].base, oldstack, newstack);
        ci[0].func = fix(ci[0].func, oldstack, newstack);
    }
    L.base = fix(L.base, oldstack, newstack);
}

pub fn reallocstack(L: *State, newsize: c_int, fornewci: bool) void {
    if (newsize > o.MAX_STACK_SIZE) {
        if (fornewci) {
            const cip = &(@as([*]CallInfo, @ptrCast(L.ci)) - 1)[0];
            L.ci = cip;
            L.base = cip.base;
            L.top = cip.top;
        }
        throw(L, o.LUA_ERRMEM);
    }
    const oldstack = L.stack;
    const realsize = newsize + o.EXTRA_STACK;
    const newstack = mem.reallocArray(L, TValue, L.stack, @intCast(L.stacksize), @intCast(realsize), L.gch.memcat);
    var i: usize = @intCast(L.stacksize);
    while (i < @as(usize, @intCast(realsize))) : (i += 1) newstack[i] = .{};
    L.stack = newstack;
    L.stacksize = realsize;
    L.stack_last = newstack + @as(usize, @intCast(newsize));
    correctstack(L, oldstack);
}

pub fn reallocCI(L: *State, newsize: c_int) void {
    const oldci = L.base_ci;
    const idx = o.sdiff(@as([*]CallInfo, @ptrCast(L.ci)), oldci);
    const nb = mem.reallocArray(L, CallInfo, L.base_ci, @intCast(L.size_ci), @intCast(newsize), L.gch.memcat);
    L.base_ci = nb;
    L.size_ci = newsize;
    L.ci = &nb[@intCast(idx)];
    L.end_ci = &nb[@intCast(newsize - 1)];
}

pub inline fn getgrownstacksize(L: *State, n: c_int) c_int {
    return if (n <= L.stacksize) 2 * L.stacksize else L.stacksize + n;
}

pub inline fn stacklimitreached(L: *State, n: c_int) bool {
    return o.sdiff(L.stack_last, L.top) <= n;
}

pub fn growstack(L: *State, n: c_int) void {
    reallocstack(L, getgrownstacksize(L, n), false);
}

pub inline fn checkstack(L: *State, n: c_int) void {
    if (stacklimitreached(L, n)) growstack(L, n);
}

pub inline fn checkstackfornewci(L: *State, n: c_int) void {
    if (stacklimitreached(L, n)) reallocstack(L, getgrownstacksize(L, n), true);
}

pub inline fn incrTop(L: *State) void {
    checkstack(L, 1);
    L.top += 1;
}

/// lua_rawcheckstack
pub fn rawcheckstack(L: *State, size: c_int) void {
    checkstack(L, size);
    expandstacklimit(L, L.top + @as(usize, @intCast(size)));
}

pub inline fn expandstacklimit(L: *State, p: StkId) void {
    if (@intFromPtr(L.ci.top) < @intFromPtr(p)) L.ci.top = p;
}

/// Pushes a value, growing the stack (and the current frame limit) if necessary.
pub fn pushvalue(L: *State, v: *const TValue) void {
    gc.threadbarrier(L);
    if (@intFromPtr(L.top + 1) > @intFromPtr(L.ci.top)) {
        checkstack(L, 1);
        expandstacklimit(L, L.top + 1);
    }
    L.top[0] = v.*;
    L.top += 1;
}

pub fn growCI(L: *State) *CallInfo {
    const hardlimit = o.LUAI_MAXCALLS + (o.LUAI_MAXCALLS >> 3);
    if (L.size_ci >= hardlimit) throw(L, o.LUA_ERRERR);
    const request = L.size_ci * 2;
    reallocCI(L, if (L.size_ci >= o.LUAI_MAXCALLS) hardlimit else if (request < o.LUAI_MAXCALLS) request else o.LUAI_MAXCALLS);
    if (L.size_ci > o.LUAI_MAXCALLS) debug.runerror(L, "stack overflow", .{});
    L.ci = &(@as([*]CallInfo, @ptrCast(L.ci)) + 1)[0];
    return L.ci;
}

pub inline fn incr_ci(L: *State) *CallInfo {
    if (L.ci == L.end_ci) return growCI(L);
    L.ci = &(@as([*]CallInfo, @ptrCast(L.ci)) + 1)[0];
    return L.ci;
}

pub inline fn prevci(ci: *CallInfo) *CallInfo {
    return &(@as([*]CallInfo, @ptrCast(ci)) - 1)[0];
}

pub fn checkCstack(L: *State) void {
    const hardlimit = o.LUAI_MAXCCALLS + (o.LUAI_MAXCCALLS >> 3);
    if (L.nCcalls == o.LUAI_MAXCCALLS)
        debug.runerror(L, "C stack overflow", .{})
    else if (L.nCcalls >= hardlimit)
        throw(L, o.LUA_ERRERR);
}

pub inline fn isyielded(L: *State) bool {
    return L.status == o.LUA_YIELD or L.status == o.LUA_BREAK or L.status == o.SCHEDULED_REENTRY;
}

// ---------------------------------------------------------------------------
// Calls
// ---------------------------------------------------------------------------

/// luau_precall
pub fn precall(L: *State, funcp: StkId, nresults: c_int) c_int {
    if (!o.ttisfunction(&funcp[0])) vmu.tryfuncTM(L, funcp);
    const ccl = o.clvalue(&funcp[0]);
    const ci = incr_ci(L);
    ci.func = funcp;
    ci.p = if (ccl.isC != 0) null else ccl.p;
    ci.base = funcp + 1;
    ci.top = L.top + ccl.stacksize;
    ci.savedpc = null;
    ci.flags = 0;
    ci.nresults = nresults;
    ci.errfunc = 0;
    L.base = ci.base;
    checkstackfornewci(L, ccl.stacksize);

    if (ccl.isC == 0) {
        const p = ccl.p.?;
        var argi = L.top;
        const argend = L.base + p.numparams;
        while (@intFromPtr(argi) < @intFromPtr(argend)) : (argi += 1) o.setnilvalue(&argi[0]);
        L.top = if (p.is_vararg != 0) argi else L.ci.top;
        L.ci.savedpc = p.code;
        return PCRLUA;
    } else {
        const n = ccl.f.?(L);
        if (n < 0) return PCRYIELD;
        const cci = L.ci;
        const cip = prevci(cci);
        var res = cci.func;
        var vali = L.top - @as(usize, @intCast(n));
        const valend = L.top;
        var i: c_int = nresults;
        while (i != 0 and @intFromPtr(vali) < @intFromPtr(valend)) : (i -= 1) {
            res[0] = vali[0];
            res += 1;
            vali += 1;
        }
        while (i > 0) : (i -= 1) {
            o.setnilvalue(&res[0]);
            res += 1;
        }
        L.ci = cip;
        L.base = cip.base;
        L.top = res;
        return PCRC;
    }
}

/// luau_poscall
pub fn poscall(L: *State, first: StkId) void {
    const ci = L.ci;
    const cip = prevci(ci);
    var res = ci.func;
    var vali = first;
    const valend = L.top;
    var i: c_int = ci.nresults;
    while (i != 0 and @intFromPtr(vali) < @intFromPtr(valend)) : (i -= 1) {
        res[0] = vali[0];
        res += 1;
        vali += 1;
    }
    while (i > 0) : (i -= 1) {
        o.setnilvalue(&res[0]);
        res += 1;
    }
    L.ci = cip;
    L.base = cip.base;
    L.top = if (ci.nresults == o.LUA_MULTRET) res else cip.top;
}

/// luau_finishop: completes an instruction whose call yielded.
pub fn finishop(L: *State) void {
    const ci = L.ci;
    ci.flags &= ~o.CALLINFO_OPYIELD;
    const base = L.base;
    var pc = ci.savedpc.?;
    const insn = (pc - 1)[0];
    const op: u8 = @truncate(insn);
    if (op == @intFromEnum(exec.Op.FORGLOOP)) {
        const ra = base + @as(usize, (insn >> 8) & 0xff);
        ra[2] = ra[3];
        const d: i32 = @as(i32, @bitCast(insn)) >> 16;
        pc = if (o.ttisnil(&ra[3])) pc + 1 else offsPc(pc, d);
    } else unreachable;
    L.ci.savedpc = pc;
}

pub inline fn offsPc(pc: [*]const u32, d: i32) [*]const u32 {
    return if (d >= 0) pc + @as(usize, @intCast(d)) else pc - @as(usize, @intCast(-d));
}

fn performcall(L: *State, f: StkId, nresults: c_int, preparereentry: bool) void {
    if (precall(L, f, nresults) == PCRLUA) {
        L.ci.flags |= o.CALLINFO_RETURN;
        const oldactive = L.isactive;
        L.isactive = true;
        gc.threadbarrier(L);
        if (preparereentry) {
            L.status = o.SCHEDULED_REENTRY;
        } else {
            exec.execute(L);
        }
        if (!oldactive) L.isactive = false;
    }
}

/// Call made by an instruction that supports yielding (FORGLOOP); returns true if
/// the call yielded and the instruction must be completed by `finishop`.
pub fn performcally(L: *State, f: StkId, nresults: c_int) bool {
    L.nCcalls += 1;
    if (L.nCcalls >= o.LUAI_MAXCCALLS) checkCstack(L);
    L.baseCcalls += 1;
    const cioffset = saveci(L, L.ci);
    performcall(L, f, nresults, false);
    if (L.status != o.LUA_OK) {
        restoreci(L, cioffset).flags |= o.CALLINFO_OPYIELD;
        return true;
    }
    L.baseCcalls -= 1;
    L.nCcalls -= 1;
    gc.checkGC(L);
    return false;
}

pub fn callint(L: *State, f: StkId, nresults: c_int, preparereentry: bool) void {
    L.nCcalls += 1;
    if (L.nCcalls >= o.LUAI_MAXCCALLS) checkCstack(L);
    var fromyieldableccall = false;
    if (L.ci != &L.base_ci[0]) {
        const ccl = o.clvalue(&L.ci.func[0]);
        if (ccl.isC != 0 and ccl.cont != null) {
            fromyieldableccall = true;
            L.baseCcalls += 1;
        }
    }
    const funcoffset = savestack(L, f);
    const cioffset = saveci(L, L.ci);
    performcall(L, f, nresults, preparereentry);
    const yielded = isyielded(L);
    if (fromyieldableccall) {
        L.baseCcalls -= 1;
        if (yielded) {
            const callerci = restoreci(L, cioffset);
            callerci.top = restorestack(L, funcoffset) + @as(usize, @intCast(if (nresults != o.LUA_MULTRET) nresults else 0));
        }
    }
    if (nresults != o.LUA_MULTRET and !yielded) L.top = restorestack(L, funcoffset) + @as(usize, @intCast(nresults));
    L.nCcalls -= 1;
    gc.checkGC(L);
}

pub fn call(L: *State, f: StkId, nresults: c_int) void {
    callint(L, f, nresults, false);
}

/// Non-yieldable call.
pub fn callny(L: *State, f: StkId, nresults: c_int) void {
    L.nCcalls += 1;
    if (L.nCcalls >= o.LUAI_MAXCCALLS) checkCstack(L);
    const funcoffset = savestack(L, f);
    performcall(L, f, nresults, false);
    if (nresults != o.LUA_MULTRET) L.top = restorestack(L, funcoffset) + @as(usize, @intCast(nresults));
    L.nCcalls -= 1;
    gc.checkGC(L);
}

pub fn seterrorobj(L: *State, errcode: c_int, oldtop: StkId) void {
    switch (errcode) {
        o.LUA_ERRMEM => o.setsvalue(&oldtop[0], L.global.memerrmsg),
        o.LUA_ERRERR => o.setsvalue(&oldtop[0], L.global.errerrmsg),
        o.LUA_ERRSYNTAX, o.LUA_ERRRUN => oldtop[0] = (L.top - 1)[0],
        else => {},
    }
    L.top = oldtop + 1;
}

fn restoreStackLimit(L: *State) void {
    if (L.size_ci > o.LUAI_MAXCALLS) {
        const inuse = o.idiff(@as([*]CallInfo, @ptrCast(L.ci)), L.base_ci);
        if (inuse + 1 < o.LUAI_MAXCALLS) reallocCI(L, o.LUAI_MAXCALLS);
    }
}

fn callerrfunc(L: *State, ud: ?*anyopaque) void {
    const errfunc: StkId = @ptrCast(@alignCast(ud.?));
    L.top[0] = (L.top - 1)[0];
    (L.top - 1)[0] = errfunc[0];
    incrTop(L);
    callny(L, L.top - 2, 1);
}

pub fn pcall(L: *State, f: Pfunc, u: ?*anyopaque, old_top: isize, ef: isize) c_int {
    const oldnCcalls = L.nCcalls;
    const oldbaseCcalls = L.baseCcalls;
    const old_ci = saveci(L, L.ci);
    const oldactive = L.isactive;
    var status = rawrunprotected(L, f, u);
    if (status != 0) {
        var errstatus = status;
        if (ef != 0) {
            if (status != o.LUA_ERRRUN) seterrorobj(L, status, L.top);
            const err = rawrunprotected(L, callerrfunc, @ptrCast(restorestack(L, ef)));
            if (err == 0) {
                errstatus = o.LUA_ERRRUN;
            } else if (status == o.LUA_ERRMEM and err == o.LUA_ERRMEM) {
                errstatus = o.LUA_ERRMEM;
            } else {
                errstatus = o.LUA_ERRERR;
                status = o.LUA_ERRERR;
            }
        }
        if (!oldactive) L.isactive = false;
        const yieldable = L.nCcalls <= L.baseCcalls;
        L.nCcalls = oldnCcalls;
        L.baseCcalls = oldbaseCcalls;
        if (yieldable) {
            if (L.global.cb.debugprotectederror) |cb| {
                cb(L);
                if (L.status == o.LUA_BREAK) return 0;
            }
        }
        const oldtop = restorestack(L, old_top);
        func.close(L, oldtop);
        seterrorobj(L, errstatus, oldtop);
        L.ci = restoreci(L, old_ci);
        L.base = L.ci.base;
        restoreStackLimit(L);
    }
    return status;
}

// ---------------------------------------------------------------------------
// Coroutines
// ---------------------------------------------------------------------------

fn resumeContinue(L: *State) void {
    while ((L.status == o.LUA_OK or L.status == o.SCHEDULED_REENTRY) and L.ci != &L.base_ci[0]) {
        L.status = o.LUA_OK;
        const cl = o.curr_func(L);
        if (cl.isC != 0) {
            L.ci.flags &= ~o.CALLINFO_HANDLE;
            const n = cl.cont.?(L, 0);
            if (L.status == o.LUA_BREAK or L.status == o.LUA_YIELD) break;
            if (L.status == o.SCHEDULED_REENTRY) continue;
            poscall(L, L.top - @as(usize, @intCast(n)));
        } else {
            if (L.ci.flags & o.CALLINFO_OPYIELD != 0) finishop(L);
            exec.execute(L);
        }
    }
}

fn resumeBody(L: *State, ud: ?*anyopaque) void {
    var firstArg: StkId = @ptrCast(@alignCast(ud.?));
    if (L.status == o.LUA_OK) {
        if (firstArg == L.base) debug.runerror(L, "cannot resume dead coroutine", .{});
        const r = precall(L, firstArg - 1, o.LUA_MULTRET);
        if (L.status == o.SCHEDULED_REENTRY) {
            firstArg = L.base;
        } else {
            if (r != PCRLUA) return;
            L.ci.flags |= o.CALLINFO_RETURN;
        }
    }
    if (L.status != o.LUA_OK) {
        L.status = o.LUA_OK;
        const cl = o.curr_func(L);
        if (cl.isC != 0) {
            if (cl.cont == null) {
                poscall(L, firstArg);
            } else {
                L.base = L.ci.base;
            }
        } else {
            L.base = L.ci.base;
        }
    }
    resumeContinue(L);
}

fn resumeFindhandler(L: *State) ?*CallInfo {
    var ci: [*]CallInfo = @ptrCast(L.ci);
    while (@intFromPtr(ci) > @intFromPtr(L.base_ci)) : (ci -= 1) {
        if (ci[0].flags & o.CALLINFO_HANDLE != 0) return &ci[0];
    }
    return null;
}

fn resumeHandle(L: *State, ud: ?*anyopaque) void {
    var ci: *CallInfo = @ptrCast(@alignCast(ud.?));
    const cl = o.ci_func(ci);
    ci.flags &= ~o.CALLINFO_HANDLE;
    var status: c_int = L.status;
    L.status = o.LUA_OK;
    if (status != o.LUA_ERRRUN) seterrorobj(L, status, L.top);
    if (ci.errfunc != 0) {
        const old_ci = saveci(L, ci);
        const err = rawrunprotected(L, callerrfunc, @ptrCast(ci.base + @as(usize, @intCast(ci.errfunc - 1))));
        if (err == 0) {
            status = o.LUA_ERRRUN;
        } else if (status == o.LUA_ERRMEM and err == o.LUA_ERRMEM) {
            status = o.LUA_ERRMEM;
        } else {
            status = o.LUA_ERRERR;
        }
        seterrorobj(L, status, L.top - 1);
        ci = restoreci(L, old_ci);
        ci.errfunc = 0;
    }
    L.nCcalls = L.baseCcalls;
    L.ci = ci;
    func.close(L, L.ci.base);
    L.base = ci.base;
    ci.top = L.top;
    restoreStackLimit(L);
    const n = cl.cont.?(L, status);
    if (flags.LuauPcallErrorContinuationReentry and L.status == o.SCHEDULED_REENTRY) {
        resumeContinue(L);
        return;
    }
    if (L.status != o.LUA_OK) return;
    poscall(L, L.top - @as(usize, @intCast(n)));
    resumeContinue(L);
}

fn resumeError(L: *State, msg: []const u8, narg: c_int) c_int {
    L.top -= @as(usize, @intCast(narg));
    o.setsvalue(&L.top[0], lstring.new(L, msg));
    incrTop(L);
    return o.LUA_ERRRUN;
}

fn resumeStart(L: *State, from: ?*State, nargs: c_int) c_int {
    if (L.status != o.LUA_YIELD and L.status != o.LUA_BREAK and (L.status != 0 or L.ci != &L.base_ci[0]))
        return resumeError(L, "cannot resume non-suspended coroutine", nargs);
    L.nCcalls = if (from) |f| f.nCcalls else 0;
    if (L.nCcalls >= o.LUAI_MAXCCALLS) return resumeError(L, "C stack overflow", nargs);
    L.nCcalls += 1;
    L.baseCcalls = L.nCcalls;
    L.isactive = true;
    gc.threadbarrier(L);
    return o.LUA_OK;
}

fn resumeFinish(L: *State, status_in: c_int, oldnCcalls: c_int) c_int {
    var status = status_in;
    while (status != o.LUA_OK) {
        const ch = resumeFindhandler(L) orelse break;
        if (isyieldableRaw(L)) {
            if (L.global.cb.debugprotectederror) |cb| {
                cb(L);
                if (L.status == o.LUA_BREAK) {
                    status = o.LUA_OK;
                    break;
                }
            }
        }
        L.baseCcalls = @intCast(oldnCcalls);
        L.status = @intCast(status);
        status = rawrunprotected(L, resumeHandle, ch);
    }
    L.nCcalls = @intCast(oldnCcalls - 1);
    L.baseCcalls = L.nCcalls;
    L.isactive = false;
    if (status != o.LUA_OK) {
        L.status = @intCast(status);
        seterrorobj(L, status, L.top);
        L.ci.top = L.top;
    } else if (L.status == o.LUA_OK) {
        expandstacklimit(L, L.top);
    }
    return L.status;
}

pub fn resumeThread(L: *State, from: ?*State, nargs: c_int) c_int {
    const starterror = resumeStart(L, from, nargs);
    if (starterror != 0) return starterror;
    if (L.global.cb.preresume) |cb| cb(L);
    const oldnCcalls: c_int = L.nCcalls;
    const status = rawrunprotected(L, resumeBody, @ptrCast(L.top - @as(usize, @intCast(nargs))));
    const result = resumeFinish(L, status, oldnCcalls);
    if (L.global.cb.postresume) |cb| cb(L);
    return result;
}

pub fn resumeerror(L: *State, from: ?*State) c_int {
    const starterror = resumeStart(L, from, 1);
    if (starterror != 0) return starterror;
    if (L.global.cb.preresume) |cb| cb(L);
    const oldnCcalls: c_int = L.nCcalls;
    var status: c_int = o.LUA_ERRRUN;
    if (resumeFindhandler(L)) |ci| {
        L.status = @intCast(status);
        status = rawrunprotected(L, resumeHandle, ci);
    }
    const result = resumeFinish(L, status, oldnCcalls);
    if (L.global.cb.postresume) |cb| cb(L);
    return result;
}

pub fn yield(L: *State, nresults: c_int) c_int {
    if (L.nCcalls > L.baseCcalls) debug.runerror(L, "attempt to yield across metamethod/C-call boundary", .{});
    L.base = L.top - @as(usize, @intCast(nresults));
    L.status = o.LUA_YIELD;
    return -1;
}

pub fn breakThread(L: *State) c_int {
    if (L.nCcalls > L.baseCcalls) debug.runerror(L, "attempt to break across metamethod/C-call boundary", .{});
    L.status = o.LUA_BREAK;
    return -1;
}

inline fn isyieldableRaw(L: *State) bool {
    if (flags.LuauIsYieldableSimplify) return L.nCcalls == L.baseCcalls;
    return L.nCcalls <= L.baseCcalls;
}

pub fn isyieldable(L: *State) c_int {
    return @intFromBool(isyieldableRaw(L));
}

// ---------------------------------------------------------------------------
// Coroutine finalizers (coroutine.finally, upstream LuauCoroutineFinally)
// ---------------------------------------------------------------------------

pub const FinallyFinished: c_int = 0;
pub const FinallyError: c_int = 1;
pub const FinallyCancelled: c_int = 2;

const api = @import("api.zig");
const table = @import("table.zig");

/// luaD_preparefinalizestate: pushes (finalizers, count, result) and takes the list.
pub fn preparefinalizestate(L: *State, co: *State, finresult: c_int) void {
    const fins = co.finalizers.?;
    checkstack(L, 3);
    o.sethvalue(&L.top[0], fins);
    L.top += 1;
    o.setnvalue(&L.top[0], @floatFromInt(table.getn(fins)));
    L.top += 1;
    o.setnvalue(&L.top[0], @floatFromInt(finresult));
    L.top += 1;
    co.finalizers = null;
}

/// luaD_preparefinalize: also moves the coroutine's results (or error) over.
pub fn preparefinalize(L: *State, co: *State, finresult: c_int) void {
    const nres: c_int = if (finresult == FinallyFinished) o.idiff(co.top, co.base) else 1;
    if (api.lua_checkstack(L, nres + 3) == 0) debug.runerror(L, "too many results to invoke finalizer", .{});
    preparefinalizestate(L, co, finresult);
    api.lua_xmove(co, L, nres);
}

/// luaD_runfinalizers: runs the pending finalizers stored at stack slots 2..4 one
/// at a time through yieldable pcall continuations.
pub fn runfinalizers(L: *State, returnstatus: bool) c_int {
    const results = api.lua_gettop(L) - 4;
    const position = api.lua_tointegerx(L, 3, null);
    const status = api.lua_tointegerx(L, 4, null);
    if (position != 0) {
        if (api.lua_checkstack(L, results + 2) == 0) {
            api.lua_pushstring(L, "too many results to invoke finalizer");
            api.lua_error(L);
        }
        api.lua_pushinteger(L, position - 1);
        api.lua_replace(L, 3);
        _ = api.lua_rawgeti(L, 2, position);
        api.lua_pushstring(L, if (status == FinallyCancelled) "cancelled" else if (status == FinallyFinished) "finished" else "error");
        var i: c_int = 0;
        while (i < results) : (i += 1) api.lua_pushvalue(L, 5 + i);
        return api.lua_pcallyieldable(L, results + 1, 0, 0);
    }
    if (returnstatus) {
        o.setbvalue(&(L.base + 3)[0], status != FinallyError);
        return results + 1;
    }
    if (status == FinallyError) api.lua_error(L);
    return results;
}
