//! The bytecode interpreter (port of upstream lvmexecute.cpp luau_execute, MIT; see
//! object.zig header).
//!
//! Executes the exact upstream bytecode format (Luau/Bytecode.h, versions 3..14).
//! Inline caches and predictive table slots are optimizations in upstream; this
//! interpreter uses the generic path for those slot hints. FASTCALL builtins use
//! the upstream fast-path/fallback protocol. With upstream feature flags at their
//! production defaults, FASTPCALL falls back to the regular pcall call path.

const std = @import("std");
const o = @import("object.zig");
const ldo = @import("do.zig");
const debug = @import("debug.zig");
const tm = @import("tm.zig");
const table = @import("table.zig");
const func = @import("func.zig");
const gc = @import("gc.zig");
const vmu = @import("vmutils.zig");
const flags = @import("flags.zig");
const cls = @import("class.zig");
const fastcalls = @import("fastcalls.zig");

const State = o.State;
const TValue = o.TValue;
const StkId = o.StkId;
const Closure = o.Closure;

pub const Op = enum(u8) {
    NOP,
    BREAK,
    LOADNIL,
    LOADB,
    LOADN,
    LOADK,
    MOVE,
    GETGLOBAL,
    SETGLOBAL,
    GETUPVAL,
    SETUPVAL,
    CLOSEUPVALS,
    GETIMPORT,
    GETTABLE,
    SETTABLE,
    GETTABLEKS,
    SETTABLEKS,
    GETTABLEN,
    SETTABLEN,
    NEWCLOSURE,
    NAMECALL,
    CALL,
    RETURN,
    JUMP,
    JUMPBACK,
    JUMPIF,
    JUMPIFNOT,
    JUMPIFEQ,
    JUMPIFLE,
    JUMPIFLT,
    JUMPIFNOTEQ,
    JUMPIFNOTLE,
    JUMPIFNOTLT,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    POW,
    ADDK,
    SUBK,
    MULK,
    DIVK,
    MODK,
    POWK,
    AND,
    OR,
    ANDK,
    ORK,
    CONCAT,
    NOT,
    MINUS,
    LENGTH,
    NEWTABLE,
    DUPTABLE,
    SETLIST,
    FORNPREP,
    FORNLOOP,
    FORGLOOP,
    FORGPREP_INEXT,
    FASTCALL3,
    FORGPREP_NEXT,
    NATIVECALL,
    GETVARARGS,
    DUPCLOSURE,
    PREPVARARGS,
    LOADKX,
    JUMPX,
    FASTCALL,
    COVERAGE,
    CAPTURE,
    SUBRK,
    DIVRK,
    FASTCALL1,
    FASTCALL2,
    FASTCALL2K,
    FORGPREP,
    JUMPXEQKNIL,
    JUMPXEQKB,
    JUMPXEQKN,
    JUMPXEQKS,
    IDIV,
    IDIVK,
    GETUDATAKS,
    SETUDATAKS,
    NAMECALLUDATA,
    NEWCLASSMEMBER,
    CALLFB,
    CMPPROTO,
    FASTPCALL,
    NEWCLASS,
    CONSTRUCT,
    FINCONSTRUCT,
    GETPRIVATEFIELD,
    SETPRIVATEFIELD,
    _,
};

pub const LCT_VAL = 0;
pub const LCT_REF = 1;
pub const LCT_UPVAL = 2;

inline fn opA(i: u32) usize {
    return (i >> 8) & 0xff;
}
inline fn opB(i: u32) usize {
    return (i >> 16) & 0xff;
}
inline fn opC(i: u32) usize {
    return i >> 24;
}
inline fn opD(i: u32) i32 {
    return @as(i32, @bitCast(i)) >> 16;
}
inline fn opE(i: u32) i32 {
    return @as(i32, @bitCast(i)) >> 8;
}
inline fn jump(pc: [*]const u32, d: i32) [*]const u32 {
    return ldo.offsPc(pc, d);
}

/// VM_INTERRUPT: returns true if execution must stop (thread yielded/broke).
inline fn interrupt(L: *State, pc: [*]const u32) bool {
    if (L.global.cb.interrupt) |f| {
        L.ci.savedpc = pc + 1;
        f(L, -1);
        if (L.status != 0) {
            L.ci.savedpc = pc;
            return true;
        }
    }
    return false;
}

inline fn checkGC(L: *State, pc: [*]const u32) void {
    if (L.global.totalbytes >= L.global.GCthreshold) {
        L.ci.savedpc = pc;
        _ = gc.step(L, true);
    }
}

fn unsupported(L: *State, pc: [*]const u32, what: []const u8) noreturn {
    L.ci.savedpc = pc;
    debug.runerror(L, "unsupported bytecode operation: {s}", .{what});
}

pub fn execute(L: *State) void {
    if (L.singlestep) executeImpl(L, true) else executeImpl(L, false);
}

fn executeImpl(L: *State, comptime SingleStep: bool) void {
    std.debug.assert(o.isLua(L.ci));
    var pc: [*]const u32 = L.ci.savedpc.?;
    var cl: *Closure = o.clvalue(&L.ci.func[0]);
    var base: StkId = L.base;
    var k: [*]TValue = cl.p.?.k;
    // A patched userdata instruction redispatches at the same PC without
    // issuing a second debugger step, as upstream's VM_CONTINUE does.
    var skip_step = false;

    while (true) {
        if (SingleStep and !skip_step) {
            const op0: u8 = @truncate(pc[0]);
            if (L.global.cb.debugstep) |hook| {
                if (op0 != @intFromEnum(Op.PREPVARARGS) and op0 != @intFromEnum(Op.BREAK)) {
                    L.ci.savedpc = pc;
                    debug.callhook(L, hook, null);
                    base = L.base;
                    if (L.status != 0) return;
                }
            }
        }
        if (SingleStep) skip_step = false;
        var insn = pc[0];
        if (@as(u8, @truncate(insn)) == @intFromEnum(Op.BREAK)) {
            // breakpoint: run the debugbreak hook, then execute the original opcode
            const p = cl.p.?;
            const orig = p.debuginsn.?[@intCast(o.idiff(pc, @as([*]const u32, p.code)))];
            if (L.global.cb.debugbreak) |hook| {
                L.ci.savedpc = pc;
                debug.callhook(L, hook, null);
                base = L.base;
                if (L.status != 0) return;
            }
            insn = (insn & ~@as(u32, 0xff)) | orig;
        }
        switch (@as(Op, @enumFromInt(@as(u8, @truncate(insn))))) {
            .NOP => pc += 1,
            .BREAK => unsupported(L, pc + 1, "BREAK without a recorded original instruction"),
            .LOADNIL => {
                pc += 1;
                o.setnilvalue(&base[opA(insn)]);
            },
            .LOADB => {
                pc += 1;
                o.setbvalue(&base[opA(insn)], opB(insn) != 0);
                pc += opC(insn);
            },
            .LOADN => {
                pc += 1;
                o.setnvalue(&base[opA(insn)], @floatFromInt(opD(insn)));
            },
            .LOADK => {
                pc += 1;
                base[opA(insn)] = k[@intCast(opD(insn))];
            },
            .MOVE => {
                pc += 1;
                base[opA(insn)] = base[opB(insn)];
            },
            .GETGLOBAL => {
                const aux = pc[1];
                pc += 2;
                var g: TValue = .{};
                o.sethvalue(&g, cl.env);
                L.ci.savedpc = pc;
                vmu.gettable(L, &g, &k[aux], base + opA(insn));
                base = L.base;
            },
            .SETGLOBAL => {
                const aux = pc[1];
                pc += 2;
                var g: TValue = .{};
                o.sethvalue(&g, cl.env);
                L.ci.savedpc = pc;
                vmu.settable(L, &g, &k[aux], &base[opA(insn)]);
                base = L.base;
            },
            .GETUPVAL => {
                pc += 1;
                const ur = &cl.upvals()[opB(insn)];
                const v: *const TValue = if (o.ttisupval(ur)) o.upvalue(ur).v else ur;
                base[opA(insn)] = v.*;
            },
            .SETUPVAL => {
                pc += 1;
                const ur = &cl.upvals()[opB(insn)];
                const uv = o.upvalue(ur);
                uv.v.* = base[opA(insn)];
                gc.barrier(L, &uv.gch, &base[opA(insn)]);
            },
            .CLOSEUPVALS => {
                pc += 1;
                const ra = base + opA(insn);
                if (L.openupval) |uv| {
                    if (@intFromPtr(uv.v) >= @intFromPtr(ra)) func.close(L, ra);
                }
            },
            .GETIMPORT => {
                const kv = &k[@intCast(opD(insn))];
                if (!o.ttisnil(kv) and cl.env.safeenv != 0) {
                    base[opA(insn)] = kv.*;
                    pc += 2;
                } else {
                    const aux = pc[1];
                    pc += 2;
                    L.ci.savedpc = pc;
                    vmu.getimport(L, cl.env, k, base + opA(insn), aux, false);
                    base = L.base;
                }
            },
            .GETTABLE => {
                pc += 1;
                L.ci.savedpc = pc;
                vmu.gettable(L, &base[opB(insn)], &base[opC(insn)], base + opA(insn));
                base = L.base;
            },
            .SETTABLE => {
                pc += 1;
                L.ci.savedpc = pc;
                vmu.settable(L, &base[opB(insn)], &base[opC(insn)], &base[opA(insn)]);
                base = L.base;
            },
            .GETTABLEKS => {
                const aux = pc[1];
                pc += 2;
                const kidx: usize = aux;
                const rb = &base[opB(insn)];
                const kv = &k[kidx];
                if (o.ttisuserdata(rb)) {
                    const u = o.uvalue(rb);
                    if (L.global.udatadirectfields[u.tag]) |dispatch| {
                        const fptr = table.getstr(dispatch, o.tsvalue(kv));
                        if (!o.ttisnil(fptr)) {
                            const f: *const fn (?*anyopaque, ?*anyopaque) callconv(.c) void = @ptrFromInt(@intFromPtr(o.pvalue(fptr).?));
                            f(u.data(), &base[opA(insn)]);
                            continue;
                        }
                    }
                }
                if (o.ttisvector(rb)) {
                    const name = o.tsvalue(kv).data();
                    const ic = @as(i32, name[0] | ' ') - 'x';
                    if (ic >= 0 and ic < o.LUA_VECTOR_SIZE and name[1] == 0) {
                        o.setnvalue(&base[opA(insn)], o.vvalue(rb)[@intCast(ic)]);
                        continue;
                    }
                }
                L.ci.savedpc = pc;
                vmu.gettable(L, rb, kv, base + opA(insn));
                base = L.base;
            },
            .SETTABLEKS => {
                const aux = pc[1];
                pc += 2;
                const kidx: usize = aux;
                L.ci.savedpc = pc;
                vmu.settable(L, &base[opB(insn)], &k[kidx], &base[opA(insn)]);
                base = L.base;
            },
            .GETTABLEN => {
                pc += 1;
                var n: TValue = .{};
                o.setnvalue(&n, @floatFromInt(opC(insn) + 1));
                L.ci.savedpc = pc;
                vmu.gettable(L, &base[opB(insn)], &n, base + opA(insn));
                base = L.base;
            },
            .SETTABLEN => {
                pc += 1;
                var n: TValue = .{};
                o.setnvalue(&n, @floatFromInt(opC(insn) + 1));
                L.ci.savedpc = pc;
                vmu.settable(L, &base[opB(insn)], &n, &base[opA(insn)]);
                base = L.base;
            },
            .NEWCLOSURE => {
                pc += 1;
                const ra = base + opA(insn);
                const pv = cl.p.?.p[@intCast(opD(insn))];
                L.ci.savedpc = pc;
                const ncl = func.newLclosure(L, pv.nups, cl.env, pv);
                o.setclvalue(&ra[0], ncl);
                const up = ncl.upvals();
                for (0..pv.nups) |ui| {
                    const uinsn = pc[0];
                    pc += 1;
                    switch (opA(uinsn)) {
                        LCT_VAL => up[ui] = base[opB(uinsn)],
                        LCT_REF => o.setupvalue(&up[ui], func.findupval(L, &base[opB(uinsn)])),
                        LCT_UPVAL => up[ui] = cl.upvals()[opB(uinsn)],
                        else => unsupported(L, pc, "unknown upvalue capture type"),
                    }
                }
                L.ci.savedpc = pc;
                gc.checkGC(L);
                base = L.base;
            },
            .NAMECALL => {
                const aux = pc[1];
                pc += 2;
                const kidx: usize = aux;
                const ra = base + opA(insn);
                const rbv = base[opB(insn)];
                const kv = &k[kidx];
                if (o.ttistable(&rbv)) {
                    ra[1] = rbv;
                    L.ci.savedpc = pc;
                    vmu.gettable(L, &rbv, kv, ra);
                    base = L.base;
                    const ra2 = base + opA(insn);
                    if (o.ttisnil(&ra2[0])) debug.methoderror(L, &ra2[1], o.tsvalue(kv));
                } else {
                    const mt = if (o.ttisuserdata(&rbv)) o.uvalue(&rbv).metatable else L.global.mt[o.ttype(&rbv)];
                    if (tm.fasttm(L, mt, .NAMECALL)) |f| {
                        ra[1] = rbv;
                        ra[0] = f.*;
                        L.namecall = o.tsvalue(kv);
                    } else if (tm.fasttm(L, mt, .INDEX) != null and o.ttistable(tm.fasttm(L, mt, .INDEX).?)) {
                        // per-type metatable with an __index table: generic lookup, nil is an error
                        ra[1] = rbv;
                        L.ci.savedpc = pc;
                        vmu.gettable(L, &rbv, kv, ra);
                        base = L.base;
                        const ra2 = base + opA(insn);
                        if (o.ttisnil(&ra2[0])) debug.methoderror(L, &ra2[1], o.tsvalue(kv));
                    } else if (flags.DebugLuauUserDefinedClassesRuntime and o.ttisobject(&rbv)) {
                        // class instance: member lookup by name; a nil member is returned as is
                        const inst = o.objectvalue(&rbv);
                        const offv = table.getstr(inst.lclass.memberstooffset.?, o.tsvalue(kv));
                        if (o.ttisnil(offv)) {
                            L.ci.savedpc = pc;
                            cls.missingmembererror(L, &rbv, kv);
                        }
                        ra[1] = rbv;
                        ra[0] = cls.memberatoffset(inst, @intFromFloat(o.nvalue(offv))).*;
                    } else {
                        ra[1] = rbv;
                        L.ci.savedpc = pc;
                        vmu.gettable(L, &rbv, kv, ra);
                        base = L.base;
                        const ra2 = base + opA(insn);
                        if (o.ttisnil(&ra2[0])) debug.methoderror(L, &ra2[1], o.tsvalue(kv));
                    }
                }
            },
            .GETUDATAKS => {
                if (udataGet(L, &pc, &base, k, insn)) continue;
                deoptimizeUdata(pc, .GETTABLEKS);
                if (SingleStep) skip_step = true;
            },
            .SETUDATAKS => {
                if (udataSet(L, &pc, &base, k, insn)) continue;
                deoptimizeUdata(pc, .SETTABLEKS);
                if (SingleStep) skip_step = true;
            },
            .NAMECALLUDATA => {
                switch (udataNamecall(L, &pc, &base, k, insn)) {
                    .done => continue,
                    .yielded => return,
                    .fallback => {
                        deoptimizeUdata(pc, .NAMECALL);
                        if (SingleStep) skip_step = true;
                    },
                }
            },
            .CALL, .CALLFB => {
                if (interrupt(L, pc)) return;
                base = L.base;
                pc += if (@as(u8, @truncate(insn)) == @intFromEnum(Op.CALLFB)) 2 else 1;
                const ra = base + opA(insn);
                const nparams: c_int = @as(c_int, @intCast(opB(insn))) - 1;
                const nresults: c_int = @as(c_int, @intCast(opC(insn))) - 1;
                var argtop: StkId = if (nparams == o.LUA_MULTRET) L.top else ra + 1 + @as(usize, @intCast(nparams));
                if (!o.ttisfunction(&ra[0])) {
                    L.ci.savedpc = pc;
                    L.top = argtop;
                    vmu.tryfuncTM(L, ra);
                    argtop += 1;
                }
                const ccl = o.clvalue(&ra[0]);
                L.ci.savedpc = pc;
                const ci = ldo.incr_ci(L);
                ci.func = ra;
                ci.p = if (ccl.isC != 0) null else ccl.p;
                ci.base = ra + 1;
                ci.top = argtop + ccl.stacksize;
                ci.savedpc = null;
                ci.flags = 0;
                ci.nresults = nresults;
                ci.errfunc = 0;
                L.base = ci.base;
                L.top = argtop;
                ldo.checkstackfornewci(L, ccl.stacksize);
                if (ccl.isC == 0) {
                    const p = ccl.p.?;
                    var argi = L.top;
                    const argend = L.base + p.numparams;
                    while (@intFromPtr(argi) < @intFromPtr(argend)) : (argi += 1) o.setnilvalue(&argi[0]);
                    L.top = if (p.is_vararg != 0) argi else L.ci.top;
                    pc = p.code;
                    cl = ccl;
                    base = L.base;
                    k = p.k;
                } else {
                    const n = ccl.f.?(L);
                    if (n < 0) return;
                    const cci = L.ci;
                    const cip = ldo.prevci(cci);
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
                    L.top = if (nresults == o.LUA_MULTRET) res else cip.top;
                    base = L.base;
                }
            },
            .RETURN => {
                if (interrupt(L, pc)) return;
                base = L.base;
                pc += 1;
                const ra = base + opA(insn);
                const b: c_int = @as(c_int, @intCast(opB(insn))) - 1;
                const ci = L.ci;
                const cip = ldo.prevci(ci);
                var res = ci.func;
                var vali = ra;
                const valend = if (b == o.LUA_MULTRET) L.top else ra + @as(usize, @intCast(b));
                const nresults = ci.nresults;
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
                L.top = if (nresults == o.LUA_MULTRET) res else cip.top;
                if (ci.flags & o.CALLINFO_RETURN != 0) return;
                std.debug.assert(o.isLua(L.ci));
                pc = cip.savedpc.?;
                cl = o.clvalue(&cip.func[0]);
                base = L.base;
                k = cip.p.?.k;
            },
            .JUMP => {
                pc += 1;
                pc = jump(pc, opD(insn));
            },
            .JUMPBACK => {
                if (interrupt(L, pc)) return;
                checkGC(L, pc);
                base = L.base;
                pc += 1;
                pc = jump(pc, opD(insn));
            },
            .JUMPX => {
                if (interrupt(L, pc)) return;
                checkGC(L, pc);
                base = L.base;
                pc += 1;
                pc = jump(pc, opE(insn));
            },
            .JUMPIF => {
                pc += 1;
                if (!o.l_isfalse(&base[opA(insn)])) pc = jump(pc, opD(insn));
            },
            .JUMPIFNOT => {
                pc += 1;
                if (o.l_isfalse(&base[opA(insn)])) pc = jump(pc, opD(insn));
            },
            .JUMPIFEQ, .JUMPIFNOTEQ => {
                pc += 1;
                const aux = pc[0];
                const ra = &base[opA(insn)];
                const rb = &base[aux];
                const want = @as(u8, @truncate(insn)) == @intFromEnum(Op.JUMPIFEQ);
                var eq: bool = undefined;
                if (ra.tt != rb.tt) {
                    eq = false;
                } else switch (o.ttype(ra)) {
                    o.T_TABLE, o.T_USERDATA, o.T_OBJECT => {
                        L.ci.savedpc = pc;
                        eq = vmu.equalval(L, ra, rb);
                        base = L.base;
                    },
                    else => eq = vmu.equalval(L, ra, rb),
                }
                pc = if (eq == want) jump(pc, opD(insn)) else pc + 1;
            },
            .JUMPIFLE, .JUMPIFNOTLE, .JUMPIFLT, .JUMPIFNOTLT => {
                pc += 1;
                const aux = pc[0];
                const ra = &base[opA(insn)];
                const rb = &base[aux];
                const op: Op = @enumFromInt(@as(u8, @truncate(insn)));
                const islt = op == .JUMPIFLT or op == .JUMPIFNOTLT;
                const want = op == .JUMPIFLE or op == .JUMPIFLT;
                var r: bool = undefined;
                if (o.ttisnumber(ra) and o.ttisnumber(rb)) {
                    r = if (islt) o.nvalue(ra) < o.nvalue(rb) else o.nvalue(ra) <= o.nvalue(rb);
                } else {
                    L.ci.savedpc = pc;
                    r = if (islt) vmu.lessthan(L, ra, rb) else vmu.lessequal(L, ra, rb);
                    base = L.base;
                }
                pc = if (r == want) jump(pc, opD(insn)) else pc + 1;
            },
            .ADD, .SUB, .MUL, .DIV, .MOD, .POW, .IDIV => {
                pc += 1;
                arith(L, &pc, &base, insn, &base[opB(insn)], &base[opC(insn)]);
            },
            .ADDK, .SUBK, .MULK, .DIVK, .MODK, .POWK, .IDIVK => {
                pc += 1;
                arith(L, &pc, &base, insn, &base[opB(insn)], &k[opC(insn)]);
            },
            .SUBRK, .DIVRK => {
                pc += 1;
                arith(L, &pc, &base, insn, &k[opB(insn)], &base[opC(insn)]);
            },
            .AND => {
                pc += 1;
                const rb = &base[opB(insn)];
                base[opA(insn)] = if (o.l_isfalse(rb)) rb.* else base[opC(insn)];
            },
            .OR => {
                pc += 1;
                const rb = &base[opB(insn)];
                base[opA(insn)] = if (o.l_isfalse(rb)) base[opC(insn)] else rb.*;
            },
            .ANDK => {
                pc += 1;
                const rb = &base[opB(insn)];
                base[opA(insn)] = if (o.l_isfalse(rb)) rb.* else k[opC(insn)];
            },
            .ORK => {
                pc += 1;
                const rb = &base[opB(insn)];
                base[opA(insn)] = if (o.l_isfalse(rb)) k[opC(insn)] else rb.*;
            },
            .CONCAT => {
                pc += 1;
                const b: c_int = @intCast(opB(insn));
                const c: c_int = @intCast(opC(insn));
                L.ci.savedpc = pc;
                vmu.concat(L, c - b + 1, c);
                base = L.base;
                base[opA(insn)] = base[opB(insn)];
                gc.checkGC(L);
                base = L.base;
            },
            .NOT => {
                pc += 1;
                o.setbvalue(&base[opA(insn)], o.l_isfalse(&base[opB(insn)]));
            },
            .MINUS => {
                pc += 1;
                const rb = &base[opB(insn)];
                if (o.ttisnumber(rb)) {
                    o.setnvalue(&base[opA(insn)], -o.nvalue(rb));
                } else {
                    L.ci.savedpc = pc;
                    vmu.arith(L, base + opA(insn), rb, rb, .UNM);
                    base = L.base;
                }
            },
            .LENGTH => {
                pc += 1;
                L.ci.savedpc = pc;
                vmu.dolen(L, base + opA(insn), &base[opB(insn)]);
                base = L.base;
            },
            .NEWTABLE => {
                const aux = pc[1];
                pc += 2;
                const b = opB(insn);
                L.ci.savedpc = pc;
                const nh: c_int = if (b == 0) 0 else @as(c_int, 1) << @intCast(b - 1);
                o.sethvalue(&base[opA(insn)], table.new(L, @intCast(aux), nh));
                gc.checkGC(L);
                base = L.base;
            },
            .DUPTABLE => {
                pc += 1;
                L.ci.savedpc = pc;
                const kv = &k[@intCast(opD(insn))];
                o.sethvalue(&base[opA(insn)], table.clone(L, o.hvalue(kv)));
                gc.checkGC(L);
                base = L.base;
            },
            .SETLIST => {
                const index = pc[1];
                pc += 2;
                const ra = &base[opA(insn)];
                const rb = base + opB(insn);
                var c: c_int = @as(c_int, @intCast(opC(insn))) - 1;
                if (c == o.LUA_MULTRET) {
                    c = o.idiff(L.top, rb);
                    L.top = L.ci.top;
                }
                if (!o.ttistable(ra)) unsupported(L, pc, "SETLIST on a non-table");
                const h = o.hvalue(ra);
                const last: c_int = @as(c_int, @intCast(index)) + c - 1;
                if (last > h.sizearray) {
                    L.ci.savedpc = pc;
                    table.resizearray(L, h, last);
                }
                // `{f()}` with no results yields c == 0 on a table without an array
                // part; upstream's loop simply does nothing there
                if (c > 0) {
                    const arr = h.arr();
                    var i: usize = 0;
                    const first: usize = index;
                    while (i < @as(usize, @intCast(c))) : (i += 1) arr[first + i - 1] = rb[i];
                }
                gc.barrierfast(L, h);
            },
            .FORNPREP => {
                pc += 1;
                const ra = base + opA(insn);
                if (!o.ttisnumber(&ra[0]) or !o.ttisnumber(&ra[1]) or !o.ttisnumber(&ra[2])) {
                    L.ci.savedpc = pc;
                    vmu.prepareFORN(L, &ra[0], &ra[1], &ra[2]);
                }
                const limit = o.nvalue(&ra[0]);
                const step = o.nvalue(&ra[1]);
                const idx = o.nvalue(&ra[2]);
                const cont = if (step > 0) idx <= limit else limit <= idx;
                if (!cont) pc = jump(pc, opD(insn));
            },
            .FORNLOOP => {
                if (interrupt(L, pc)) return;
                checkGC(L, pc);
                base = L.base;
                pc += 1;
                const ra = base + opA(insn);
                const limit = o.nvalue(&ra[0]);
                const step = o.nvalue(&ra[1]);
                const idx = o.nvalue(&ra[2]) + step;
                o.setnvalue(&ra[2], idx);
                const cont = if (step > 0) idx <= limit else limit <= idx;
                if (cont) pc = jump(pc, opD(insn));
            },
            .FORGPREP => {
                pc += 1;
                var ra = base + opA(insn);
                if (!o.ttisfunction(&ra[0])) {
                    const mt: ?*o.Table = if (o.ttistable(&ra[0])) o.hvalue(&ra[0]).metatable else if (o.ttisuserdata(&ra[0])) o.uvalue(&ra[0]).metatable else null;
                    var iterfn = tm.fasttm(L, mt, .ITER);
                    if (iterfn == null and flags.DebugLuauUserDefinedClassesRuntime and o.ttisobject(&ra[0])) {
                        const f2 = tm.gettmbyobj(L, &ra[0], .ITER);
                        if (o.ttisnil(f2)) {
                            L.ci.savedpc = pc;
                            debug.typeerror(L, &ra[0], "iterate over");
                        }
                        iterfn = f2;
                    }
                    if (iterfn) |f| {
                        const fc = f.*;
                        ra[1] = ra[0];
                        ra[0] = fc;
                        L.top = ra + 2;
                        L.ci.savedpc = pc;
                        ldo.call(L, ra, 3);
                        base = L.base;
                        L.top = L.ci.top;
                        ra = base + opA(insn);
                        if (o.ttisnil(&ra[0])) debug.typeerror(L, &ra[0], "call");
                    } else if (tm.fasttm(L, mt, .CALL) != null) {
                        // table or userdata with __call, called during FORGLOOP
                    } else if (o.ttistable(&ra[0])) {
                        ra[1] = ra[0];
                        o.setpvalue(&ra[2], null, o.LU_TAG_ITERATOR);
                        o.setnilvalue(&ra[0]);
                    } else {
                        L.ci.savedpc = pc;
                        debug.typeerror(L, &ra[0], "iterate over");
                    }
                }
                pc = jump(pc, opD(insn));
            },
            .FORGLOOP => {
                if (interrupt(L, pc)) return;
                checkGC(L, pc);
                base = L.base;
                pc += 1;
                var ra = base + opA(insn);
                const aux = pc[0];
                if (o.ttisnil(&ra[0]) and o.ttistable(&ra[1])) {
                    const h = o.hvalue(&ra[1]);
                    var index: c_int = @intCast(@intFromPtr(o.pvalue(&ra[2])));
                    const sizearray = h.sizearray;
                    const iaux: i32 = @bitCast(aux);
                    if (iaux > 2) {
                        var i: usize = 2;
                        while (i < @as(usize, @intCast(iaux))) : (i += 1) o.setnilvalue(&ra[3 + i]);
                    }
                    if (iaux < 0 and (@as(c_uint, @bitCast(index)) >= @as(c_uint, @bitCast(sizearray)) or o.ttisnil(&h.arr()[@intCast(index)]))) {
                        pc += 1;
                        continue;
                    }
                    var found = false;
                    while (@as(c_uint, @bitCast(index)) < @as(c_uint, @bitCast(sizearray))) : (index += 1) {
                        const e = &h.arr()[@intCast(index)];
                        if (!o.ttisnil(e)) {
                            o.setpvalue(&ra[2], @ptrFromInt(@as(usize, @intCast(index + 1))), o.LU_TAG_ITERATOR);
                            o.setnvalue(&ra[3], @floatFromInt(index + 1));
                            ra[4] = e.*;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        const sizenode = h.sizenode();
                        while (@as(c_uint, @bitCast(index - sizearray)) < @as(c_uint, @bitCast(sizenode))) : (index += 1) {
                            const n = &h.node[@intCast(index - sizearray)];
                            if (!o.ttisnil(&n.val)) {
                                o.setpvalue(&ra[2], @ptrFromInt(@as(usize, @intCast(index + 1))), o.LU_TAG_ITERATOR);
                                o.getnodekey(&ra[3], n);
                                ra[4] = n.val;
                                found = true;
                                break;
                            }
                        }
                    }
                    pc = if (found) jump(pc, opD(insn)) else pc + 1;
                } else {
                    ra[5] = ra[2];
                    ra[4] = ra[1];
                    ra[3] = ra[0];
                    L.top = ra + 6;
                    L.ci.savedpc = pc;
                    const yielded = ldo.performcally(L, ra + 3, @intCast(aux & 0xff));
                    base = L.base;
                    if (yielded) return;
                    L.top = L.ci.top;
                    ra = base + opA(insn);
                    ra[2] = ra[3];
                    pc = if (o.ttisnil(&ra[3])) pc + 1 else jump(pc, opD(insn));
                }
            },
            .FORGPREP_INEXT => {
                pc += 1;
                const ra = base + opA(insn);
                if (cl.env.safeenv != 0 and o.ttistable(&ra[1]) and o.ttisnumber(&ra[2]) and o.nvalue(&ra[2]) == 0.0) {
                    o.setnilvalue(&ra[0]);
                    o.setpvalue(&ra[2], null, o.LU_TAG_ITERATOR);
                } else if (!o.ttisfunction(&ra[0])) {
                    L.ci.savedpc = pc;
                    debug.typeerror(L, &ra[0], "iterate over");
                }
                pc = jump(pc, opD(insn));
            },
            .FORGPREP_NEXT => {
                pc += 1;
                const ra = base + opA(insn);
                if (cl.env.safeenv != 0 and o.ttistable(&ra[1]) and o.ttisnil(&ra[2])) {
                    o.setnilvalue(&ra[0]);
                    o.setpvalue(&ra[2], null, o.LU_TAG_ITERATOR);
                } else if (!o.ttisfunction(&ra[0])) {
                    L.ci.savedpc = pc;
                    debug.typeerror(L, &ra[0], "iterate over");
                }
                pc = jump(pc, opD(insn));
            },
            .GETVARARGS => {
                pc += 1;
                const b: c_int = @as(c_int, @intCast(opB(insn))) - 1;
                const n: c_int = o.idiff(base, L.ci.func) - @as(c_int, L.ci.p.?.numparams) - 1;
                if (b == o.LUA_MULTRET) {
                    L.ci.savedpc = pc;
                    ldo.checkstack(L, n);
                    base = L.base;
                    const ra = base + opA(insn);
                    const src = base - @as(usize, @intCast(n));
                    var j: usize = 0;
                    while (j < @as(usize, @intCast(n))) : (j += 1) ra[j] = src[j];
                    L.top = ra + @as(usize, @intCast(n));
                } else {
                    const ra = base + opA(insn);
                    const src = base - @as(usize, @intCast(n));
                    var j: c_int = 0;
                    while (j < b and j < n) : (j += 1) ra[@intCast(j)] = src[@intCast(j)];
                    j = n;
                    while (j < b) : (j += 1) o.setnilvalue(&ra[@intCast(j)]);
                }
            },
            .DUPCLOSURE => {
                pc += 1;
                const ra = base + opA(insn);
                const kcl = o.clvalue(&k[@intCast(opD(insn))]);
                L.ci.savedpc = pc;
                var ncl = if (kcl.env == cl.env) kcl else func.newLclosure(L, kcl.nupvalues, cl.env, kcl.p.?);
                o.setclvalue(&ra[0], ncl);
                var ui: usize = 0;
                while (ui < kcl.nupvalues) {
                    const uinsn = pc[ui];
                    const uv: *const TValue = if (opA(uinsn) == LCT_VAL) &base[opB(uinsn)] else &cl.upvals()[opB(uinsn)];
                    if (ncl == kcl and table.rawequalObj(&ncl.upvals()[ui], uv)) {
                        ui += 1;
                        continue;
                    }
                    if (ncl == kcl and kcl.preload == 0) {
                        ncl = func.newLclosure(L, kcl.nupvalues, cl.env, kcl.p.?);
                        o.setclvalue(&ra[0], ncl);
                        ui = 0;
                        continue;
                    }
                    ncl.upvals()[ui] = uv.*;
                    gc.barrier(L, &ncl.gch, uv);
                    ui += 1;
                }
                ncl.preload = 0;
                if (kcl != ncl) {
                    gc.checkGC(L);
                    base = L.base;
                }
                pc += kcl.nupvalues;
            },
            .PREPVARARGS => {
                pc += 1;
                const numparams = opA(insn);
                L.ci.savedpc = pc;
                ldo.checkstack(L, @as(c_int, cl.stacksize) + @as(c_int, @intCast(numparams)));
                const fixed = L.base;
                base = L.top;
                for (0..numparams) |i| {
                    base[i] = fixed[i];
                    o.setnilvalue(&fixed[i]);
                }
                L.ci.base = base;
                L.ci.top = base + cl.stacksize;
                L.base = base;
                L.top = L.ci.top;
            },
            .LOADKX => {
                const aux = pc[1];
                pc += 2;
                base[opA(insn)] = k[aux];
            },
            .FASTCALL, .FASTCALL1, .FASTCALL2, .FASTCALL2K, .FASTCALL3 => {
                const op: Op = @enumFromInt(@as(u8, @truncate(insn)));
                const width: usize = if (op == .FASTCALL or op == .FASTCALL1) 1 else 2;
                const next = pc + width;
                const callpc = pc + 1 + opC(insn);
                const call = callpc[0];
                std.debug.assert(@as(u8, @truncate(call)) == @intFromEnum(Op.CALL));
                const ra = base + opA(call);
                const nresults: c_int = @as(c_int, @intCast(opC(call))) - 1;
                if (cl.env.safeenv != 0) {
                    const nparams: c_int = switch (op) {
                        .FASTCALL => if (opB(call) == 0) o.idiff(L.top, ra + 1) else @as(c_int, @intCast(opB(call))) - 1,
                        .FASTCALL1 => 1,
                        .FASTCALL2, .FASTCALL2K => 2,
                        .FASTCALL3 => 3,
                        else => unreachable,
                    };
                    const arg0: *TValue = if (op == .FASTCALL) &ra[1] else &base[opB(insn)];
                    var args: ?StkId = null;
                    switch (op) {
                        .FASTCALL => args = ra + 2,
                        .FASTCALL1 => {},
                        .FASTCALL2 => args = base + pc[1],
                        .FASTCALL2K => args = k + pc[1],
                        .FASTCALL3 => {
                            const aux = pc[1];
                            const top = L.top;
                            top[0] = base[aux & 0xff];
                            top[1] = base[(aux >> 8) & 0xff];
                            args = top;
                        },
                        else => unreachable,
                    }
                    L.ci.savedpc = next;
                    if (fastcalls.run(L, @intCast(opA(insn)), ra, arg0, args, nparams, nresults)) |n| {
                        std.debug.assert(n >= 0);
                        if (nresults == o.LUA_MULTRET)
                            L.top = ra + @as(usize, @intCast(n))
                        else if (op == .FASTCALL)
                            L.top = L.ci.top;
                        pc = callpc + 1;
                        continue;
                    }
                }
                pc = next;
            },
            .FASTPCALL => pc += 1,
            .COVERAGE => {
                const hits = opE(insn);
                const nh: u32 = @intCast(if (hits < (1 << 23) - 1) hits + 1 else hits);
                @constCast(pc)[0] = (pc[0] & 0xff) | (nh << 8);
                pc += 1;
            },
            .JUMPXEQKNIL => {
                pc += 1;
                const aux = pc[0];
                const isnil = o.ttisnil(&base[opA(insn)]);
                pc = if (isnil != (aux >> 31 != 0)) jump(pc, opD(insn)) else pc + 1;
            },
            .JUMPXEQKB => {
                pc += 1;
                const aux = pc[0];
                const ra = &base[opA(insn)];
                const r = o.ttisboolean(ra) and o.bvalue(ra) == @as(c_int, @intCast(aux & 1));
                pc = if (r != (aux >> 31 != 0)) jump(pc, opD(insn)) else pc + 1;
            },
            .JUMPXEQKN => {
                pc += 1;
                const aux = pc[0];
                const ra = &base[opA(insn)];
                const kv = &k[aux & 0xffffff];
                const r = o.ttisnumber(ra) and o.nvalue(ra) == o.nvalue(kv);
                pc = if (r != (aux >> 31 != 0)) jump(pc, opD(insn)) else pc + 1;
            },
            .JUMPXEQKS => {
                pc += 1;
                const aux = pc[0];
                const ra = &base[opA(insn)];
                const kv = &k[aux & 0xffffff];
                const r = o.ttisstring(ra) and o.gcvalue(ra) == o.gcvalue(kv);
                pc = if (r != (aux >> 31 != 0)) jump(pc, opD(insn)) else pc + 1;
            },
            .CMPPROTO => {
                const funid = pc[1];
                pc += 2;
                const ra = &base[opA(insn)];
                const match = o.ttisfunction(ra) and o.clvalue(ra).isC == 0 and o.clvalue(ra).p.?.funid == funid;
                if (!match) pc = jump(pc, opD(insn) - 1);
            },
            .NATIVECALL => unsupported(L, pc + 1, "NATIVECALL (native code generation is not supported)"),
            .CAPTURE => unsupported(L, pc + 1, "CAPTURE outside of a closure constructor"),
            .NEWCLASSMEMBER => {
                const aux = pc[1];
                pc += 2;
                const ra = &base[opA(insn)];
                const rc = &base[opC(insn)];
                L.ci.savedpc = pc;
                if (opB(insn) == 0) {
                    cls.addpublicstaticmember(L, o.classvalue(ra), o.tsvalue(&k[aux]), rc);
                } else {
                    cls.addprivatestaticmember(L, o.classvalue(ra), @truncate(aux), rc);
                }
            },
            .NEWCLASS => {
                const aux = pc[1];
                pc += 2;
                L.ci.savedpc = pc;
                const ra = &base[opA(insn)];
                ra.* = k[aux];
                const newcls = o.classvalue(ra);
                newcls.isopen = (opC(insn) & 1) != 0;
                const super = opB(insn);
                if (super != 0xff) {
                    const rb = &base[super];
                    if (!o.ttisclass(rb)) debug.typeerror(L, rb, "extend");
                    const inherited = cls.inheritclass(L, newcls, o.classvalue(rb));
                    o.setclassvalue(&base[opA(insn)], inherited);
                }
            },
            .CONSTRUCT => {
                const fbslot = pc[1];
                pc += 2;
                L.ci.savedpc = pc;
                const rbv = base[opB(insn)];
                cls.tryconstructobject(L, base + opA(insn), &rbv, fbslot);
                base = L.base;
            },
            .FINCONSTRUCT => {
                pc += 1;
                const arg = &base[opA(insn)];
                if (o.ttisobject(arg)) {
                    const skip = opC(insn);
                    const call = pc[skip];
                    const ra = base + opA(call);
                    const nresults: c_int = @as(c_int, @intCast(opC(call))) - 1;
                    const argv = arg.*;
                    ra[0] = argv;
                    var i: c_int = 1;
                    while (i < nresults) : (i += 1) o.setnilvalue(&ra[@intCast(i)]);
                    L.top = if (nresults == o.LUA_MULTRET) ra + 1 else L.ci.top;
                    pc += skip + 1;
                }
            },
            .GETPRIVATEFIELD => {
                const aux = pc[1];
                pc += 2;
                L.ci.savedpc = pc;
                const ra = base + opA(insn);
                const rb = &base[opB(insn)];
                if (!(o.ttisobject(rb) or o.ttisclass(rb))) debug.typeerror(L, rb, "access private field of");
                const clsuv = &cl.upvals()[opC(insn)];
                const clsv: *TValue = if (o.ttisupval(clsuv)) o.upvalue(clsuv).v else clsuv;
                if (!o.ttisclass(clsv)) debug.typeerror(L, clsv, "verify object is an instance of");
                const c = o.classvalue(clsv);
                const isstatic = (aux >> 8) & 1 != 0;
                const isnamecall = (aux >> 9) & 1 != 0;
                const rbv = rb.*;
                if (o.ttisobject(&rbv)) {
                    const inst = o.objectvalue(&rbv);
                    if (!cls.isinstance(inst, c)) debug.runerror(L, "attempt to illegally access private field", .{});
                    if (isnamecall) ra[1] = rbv;
                    if (isstatic) {
                        ra[0] = c.privatestaticmembers.?[aux & 0xff];
                    } else {
                        ra[0] = inst.members.?[(aux & 0xff) + c.ownprivatefieldsstart];
                    }
                } else {
                    const srccls = o.classvalue(&rbv);
                    if (!cls.issubclass(srccls, c)) debug.runerror(L, "attempt to illegally access private field of a class", .{});
                    if (!isstatic) debug.runerror(L, "attempt to access private instance field of a class", .{});
                    if (isnamecall) ra[1] = rbv;
                    ra[0] = c.privatestaticmembers.?[aux & 0xff];
                }
            },
            .SETPRIVATEFIELD => {
                const aux = pc[1];
                pc += 2;
                L.ci.savedpc = pc;
                const ra = &base[opA(insn)];
                const rb = &base[opB(insn)];
                if (!o.ttisobject(rb)) debug.typeerror(L, rb, "assign private field of");
                const clsuv = &cl.upvals()[opC(insn)];
                const clsv: *TValue = if (o.ttisupval(clsuv)) o.upvalue(clsuv).v else clsuv;
                if (!o.ttisclass(clsv)) debug.typeerror(L, clsv, "assign private field of");
                const inst = o.objectvalue(rb);
                const c = o.classvalue(clsv);
                if (!cls.isinstance(inst, c)) debug.runerror(L, "attempt to assign private field of an object with a different class", .{});
                const pindex = (aux & 0xff) + c.ownprivatefieldsstart;
                inst.members.?[pindex] = ra.*;
                gc.barrier(L, &inst.gch, ra);
            },
            else => unsupported(L, pc + 1, "invalid opcode"),
        }
    }
}

fn arith(L: *State, pc: *[*]const u32, base: *StkId, insn: u32, rb: *const TValue, rc: *const TValue) void {
    const ra = base.* + opA(insn);
    if (o.ttisnumber(rb) and o.ttisnumber(rc)) {
        const b = o.nvalue(rb);
        const c = o.nvalue(rc);
        const op: Op = @enumFromInt(@as(u8, @truncate(insn)));
        o.setnvalue(&ra[0], switch (op) {
            .ADD, .ADDK => b + c,
            .SUB, .SUBK, .SUBRK => b - c,
            .MUL, .MULK => b * c,
            .DIV, .DIVK, .DIVRK => b / c,
            .MOD, .MODK => vmu.luai_nummod(b, c),
            .POW, .POWK => vmu.luai_numpow(b, c),
            .IDIV, .IDIVK => vmu.luai_numidiv(b, c),
            else => unreachable,
        });
        return;
    }
    const op: Op = @enumFromInt(@as(u8, @truncate(insn)));
    const event: o.TMS = switch (op) {
        .ADD, .ADDK => .ADD,
        .SUB, .SUBK, .SUBRK => .SUB,
        .MUL, .MULK => .MUL,
        .DIV, .DIVK, .DIVRK => .DIV,
        .MOD, .MODK => .MOD,
        .POW, .POWK => .POW,
        .IDIV, .IDIVK => .IDIV,
        else => unreachable,
    };
    L.ci.savedpc = pc.*;
    vmu.arith(L, ra, rb, rc, event);
    base.* = L.base;
}

// ---------------------------------------------------------------------------
// Direct userdata access (GETUDATAKS / SETUDATAKS / NAMECALLUDATA)
// ---------------------------------------------------------------------------
//
// Port of the upstream handlers: when the userdata's tag has a direct access
// callback (lua_registeruserdatadirectaccess) and the corresponding metamethod
// was present at registration, the callback runs inside a C call frame for that
// metamethod; otherwise the instruction is permanently rewritten back to its
// generic form (aux slot cleared) and re-dispatched.

inline fn auxKv16(aux: u32) usize {
    return aux & 0xffff;
}
inline fn auxSlot(aux: u32) u16 {
    return @truncate(aux >> 16);
}

/// VM_PATCH_OP + VM_PATCH_AUX_SLOT(kidx, 0); pc still points at the instruction,
/// which the dispatch loop then executes in its generic form.
fn deoptimizeUdata(pc: [*]const u32, op: Op) void {
    const code: [*]u32 = @constCast(pc);
    code[0] = (code[0] & 0xffffff00) | @intFromEnum(op);
    code[1] = @intCast(auxKv16(code[1]));
}

/// luau_setupcci: a C call frame for the metamethod at `fun`.
fn setupcci(L: *State, nresults: c_int, fun: StkId) void {
    const ci = ldo.incr_ci(L);
    const fcl = o.clvalue(&fun[0]);
    ci.func = fun;
    ci.p = if (fcl.isC != 0) null else fcl.p;
    ci.base = fun + 1;
    ci.top = L.top + o.LUA_MINSTACK;
    ci.savedpc = null;
    ci.flags = 0;
    ci.nresults = nresults;
    ci.errfunc = 0;
    L.base = fun + 1;
    ldo.checkstackfornewci(L, o.LUA_MINSTACK);
}

fn patchSlot(pc: [*]const u32, expect: Op, kidx: usize, slot: u16) void {
    // pc points at the aux word; update only if not deoptimized meanwhile
    if (@as(u8, @truncate((pc - 1)[0])) != @intFromEnum(expect)) return;
    @constCast(pc)[0] = @as(u32, @intCast(kidx)) | (@as(u32, slot) << 16);
}

fn udataGet(L: *State, pcp: *[*]const u32, basep: *StkId, k: [*]TValue, insn: u32) bool {
    const pc = pcp.*;
    const aux = pc[1];
    const kidx = auxKv16(aux);
    const kv = &k[kidx];
    var rb = &basep.*[opB(insn)];
    if (!o.ttisuserdata(rb)) return false;
    const u = o.uvalue(rb);
    const utag: c_int = u.tag;
    const d = &L.global.udatadirect[u.tag];
    const cb = d.index orelse return false;
    if (o.ttisnil(&d.indextm)) return false;
    const udata = u.data();
    const npc = pc + 2;
    L.ci.savedpc = npc;
    ldo.checkstack(L, 3);
    basep.* = L.base;
    rb = &basep.*[opB(insn)];
    const top = L.top;
    top[0] = d.indextm;
    top[1] = rb.*;
    top[2] = kv.*;
    L.top += 3;
    L.nCcalls += 1;
    if (L.nCcalls >= o.LUAI_MAXCCALLS) ldo.checkCstack(L);
    setupcci(L, 1, top);
    var cachedslot: u16 = auxSlot(aux);
    cb(L, udata, o.tsvalue(kv).atom, &cachedslot, utag);
    if (cachedslot != auxSlot(aux)) patchSlot(npc - 1, .GETUDATAKS, kidx, cachedslot);
    const cip = ldo.prevci(L.ci);
    L.ci = cip;
    L.base = cip.base;
    L.nCcalls -= 1;
    basep.* = L.base;
    basep.*[opA(insn)] = (L.top - 1)[0];
    L.top = cip.top;
    pcp.* = npc;
    return true;
}

fn udataSet(L: *State, pcp: *[*]const u32, basep: *StkId, k: [*]TValue, insn: u32) bool {
    const pc = pcp.*;
    const aux = pc[1];
    const kidx = auxKv16(aux);
    const kv = &k[kidx];
    var rb = &basep.*[opB(insn)];
    if (!o.ttisuserdata(rb)) return false;
    const u = o.uvalue(rb);
    const utag: c_int = u.tag;
    const d = &L.global.udatadirect[u.tag];
    const cb = d.newindex orelse return false;
    if (o.ttisnil(&d.newindextm)) return false;
    const udata = u.data();
    const npc = pc + 2;
    L.ci.savedpc = npc;
    ldo.checkstack(L, 4);
    basep.* = L.base;
    rb = &basep.*[opB(insn)];
    const top = L.top;
    top[0] = d.newindextm;
    top[1] = rb.*;
    top[2] = kv.*;
    top[3] = basep.*[opA(insn)];
    L.top += 4;
    L.nCcalls += 1;
    if (L.nCcalls >= o.LUAI_MAXCCALLS) ldo.checkCstack(L);
    setupcci(L, 0, top);
    var cachedslot: u16 = auxSlot(aux);
    cb(L, udata, o.tsvalue(kv).atom, &cachedslot, utag);
    if (cachedslot != auxSlot(aux)) patchSlot(npc - 1, .SETUDATAKS, kidx, cachedslot);
    const cip = ldo.prevci(L.ci);
    L.ci = cip;
    L.base = cip.base;
    L.top = cip.top;
    L.nCcalls -= 1;
    basep.* = L.base;
    pcp.* = npc;
    return true;
}

const NamecallResult = enum { done, yielded, fallback };

fn udataNamecall(L: *State, pcp: *[*]const u32, basep: *StkId, k: [*]TValue, insn: u32) NamecallResult {
    var pc = pcp.*;
    const aux = pc[1];
    const kidx = auxKv16(aux);
    const kv = &k[kidx];
    const rbv = basep.*[opB(insn)];
    if (!o.ttisuserdata(&rbv)) return .fallback;
    const u = o.uvalue(&rbv);
    const utag: c_int = u.tag;
    const d = &L.global.udatadirect[u.tag];
    const cb = d.namecall orelse return .fallback;
    if (o.ttisnil(&d.namecalltm)) return .fallback;
    const udata = u.data();
    const ra = basep.* + opA(insn);
    ra[1] = rbv;
    ra[0] = d.namecalltm;
    const ncslot = pc + 1; // aux word of this instruction
    pc += 2;
    // the following CALL/CALLFB is executed here (first half of OP_CALL)
    const call = pc[0];
    pc += 1;
    if (@as(u8, @truncate(call)) == @intFromEnum(Op.CALLFB)) pc += 1;
    const nparams: c_int = @as(c_int, @intCast(opB(call))) - 1;
    const nresults: c_int = @as(c_int, @intCast(opC(call))) - 1;
    L.ci.savedpc = pc;
    L.namecall = o.tsvalue(kv);
    if (nparams != o.LUA_MULTRET) L.top = ra + 1 + @as(usize, @intCast(nparams));
    // namecalls do not increase the C call count and allow yielding
    setupcci(L, nresults, ra);
    var cachedslot: u16 = auxSlot(aux);
    const results = cb(L, udata, o.tsvalue(kv).atom, &cachedslot, utag);
    if (cachedslot != auxSlot(aux)) patchSlot(ncslot, .NAMECALLUDATA, kidx, cachedslot);
    pcp.* = pc;
    if (results < 0) return .yielded;
    const ci = L.ci;
    const cip = ldo.prevci(ci);
    var res = ci.func;
    var vali = L.top - @as(usize, @intCast(results));
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
    L.top = if (nresults == o.LUA_MULTRET) res else cip.top;
    basep.* = L.base;
    return .done;
}
