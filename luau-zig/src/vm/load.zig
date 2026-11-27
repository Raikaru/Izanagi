//! Bytecode loader for the exact upstream serialization format (port of upstream
//! lvmload.cpp, MIT; see object.zig header). Accepts LBC_VERSION_MIN..MAX (3..14)
//! and type-info versions 1..3. Unlike upstream, every read is bounds checked and
//! malformed input is reported as an error instead of being trusted.

const std = @import("std");
const o = @import("object.zig");
const ldo = @import("do.zig");
const mem = @import("mem.zig");
const debug = @import("debug.zig");
const func = @import("func.zig");
const table = @import("table.zig");
const lstring = @import("string.zig");
const vmu = @import("vmutils.zig");
const api = @import("api.zig");

const State = o.State;
const TValue = o.TValue;
const Proto = o.Proto;
const TString = o.TString;

pub const LBC_VERSION_MIN = 3;
pub const LBC_VERSION_MAX = 14;
pub const LBC_VERSION_TARGET = 13;
pub const LBC_VERSION_CLASSES = 100;
pub const LBC_TYPE_VERSION_MIN = 1;
pub const LBC_TYPE_VERSION_MAX = 3;

const LBC_CONSTANT_NIL = 0;
const LBC_CONSTANT_BOOLEAN = 1;
const LBC_CONSTANT_NUMBER = 2;
const LBC_CONSTANT_STRING = 3;
const LBC_CONSTANT_IMPORT = 4;
const LBC_CONSTANT_TABLE = 5;
const LBC_CONSTANT_CLOSURE = 6;
const LBC_CONSTANT_VECTOR = 7;
const LBC_CONSTANT_TABLE_WITH_CONSTANTS = 8;
const LBC_CONSTANT_INTEGER = 9;
const LBC_CONSTANT_CLASS_SHAPE = 10;
const LBC_CONSTANT_VECTORD = 11;

const LPF_INLINABLE: u8 = 1 << 3;
const LFT_CALLTARGET = 0;
const LFT_CONSTRUCT = 1;

const Reader = struct {
    L: *State,
    data: []const u8,
    off: usize = 0,

    fn fail(r: *Reader) noreturn {
        debug.runerror(r.L, "malformed bytecode: unexpected end of data", .{});
    }
    fn bytes(r: *Reader, n: usize) []const u8 {
        if (n > r.data.len - r.off) r.fail();
        const s = r.data[r.off .. r.off + n];
        r.off += n;
        return s;
    }
    fn byte(r: *Reader) u8 {
        return r.bytes(1)[0];
    }
    fn int(r: *Reader, comptime T: type) T {
        return std.mem.readInt(T, r.bytes(@sizeOf(T))[0..@sizeOf(T)], .little);
    }
    fn f64v(r: *Reader) f64 {
        return @bitCast(r.int(u64));
    }
    fn f32v(r: *Reader) f32 {
        return @bitCast(r.int(u32));
    }
    fn varint(r: *Reader) u32 {
        var result: u32 = 0;
        var shift: u32 = 0;
        while (true) {
            const b = r.byte();
            if (shift < 32) result |= @as(u32, b & 127) << @intCast(shift);
            shift += 7;
            if (b & 128 == 0) break;
        }
        return result;
    }
    fn varint64(r: *Reader) u64 {
        var result: u64 = 0;
        var shift: u32 = 0;
        while (true) {
            const b = r.byte();
            if (shift < 64) result |= @as(u64, b & 127) << @intCast(shift);
            shift += 7;
            if (b & 128 == 0) break;
        }
        return result;
    }
};

const Ctx = struct {
    chunkname: [*:0]const u8,
    data: []const u8,
    env: c_int,
    result: c_int = 0,
    strings: ?[*]*TString = null,
    nstrings: usize = 0,
    capstrings: usize = 0,
    protos: ?[*]*Proto = null,
    nprotos: usize = 0,
    capprotos: usize = 0,
};

fn pushChunkError(L: *State, chunkname: [*:0]const u8, comptime fmt: []const u8, args: anytype) void {
    var chunkbuf: [o.LUA_IDSIZE]u8 = undefined;
    const id = debug.chunkid(&chunkbuf, std.mem.span(chunkname));
    var buf: [o.LUA_BUFFERSIZE]u8 = undefined;
    var w: std.Io.Writer = .fixed(buf[0 .. o.LUA_BUFFERSIZE - 1]);
    w.writeAll(id) catch {};
    w.print(fmt, args) catch {};
    api.pushstringRaw(L, w.buffered());
}

fn readString(r: *Reader, ctx: *Ctx) ?*TString {
    const id = r.varint();
    if (id == 0) return null;
    if (id - 1 >= ctx.nstrings) debug.runerror(r.L, "malformed bytecode: bad string reference", .{});
    return ctx.strings.?[id - 1];
}

const ImportCtx = struct { k: [*]TValue, id: u32 };
fn resolveImportBody(L: *State, ud: ?*anyopaque) void {
    const c: *ImportCtx = @ptrCast(@alignCast(ud.?));
    ldo.checkstack(L, 1);
    o.setnilvalue(&L.top[0]);
    L.top += 1;
    vmu.getimport(L, L.gt, c.k, L.top - 1, c.id, true);
}

fn resolveImportSafe(L: *State, k: [*]TValue, id: u32) void {
    if (L.gt.safeenv != 0) {
        var ic: ImportCtx = .{ .k = k, .id = id };
        const status = ldo.pcall(L, resolveImportBody, &ic, ldo.savestack(L, L.top), 0);
        if (status != 0) o.setnilvalue(&(L.top - 1)[0]);
    } else {
        ldo.checkstack(L, 1);
        o.setnilvalue(&L.top[0]);
        L.top += 1;
    }
}

fn loadsafe(L: *State, ctx: *Ctx) c_int {
    var r: Reader = .{ .L = L, .data = ctx.data };
    const version = r.byte();
    if (version == 0) {
        // compiler error blob: the message follows the zero byte
        pushChunkError(L, ctx.chunkname, "{s}", .{ctx.data[1..]});
        return 1;
    }
    if ((version < LBC_VERSION_MIN or version > LBC_VERSION_MAX) and version != LBC_VERSION_CLASSES) {
        pushChunkError(L, ctx.chunkname, ": bytecode version mismatch (expected [{d}..{d}], got {d})", .{ LBC_VERSION_MIN, LBC_VERSION_MAX, version });
        return 1;
    }
    var typesversion: u8 = 0;
    if (version >= 4) {
        typesversion = r.byte();
        if (typesversion < LBC_TYPE_VERSION_MIN or typesversion > LBC_TYPE_VERSION_MAX) {
            pushChunkError(L, ctx.chunkname, ": bytecode type version mismatch (expected [{d}..{d}], got {d})", .{ LBC_TYPE_VERSION_MIN, LBC_TYPE_VERSION_MAX, typesversion });
            return 1;
        }
    }
    const envt: *o.Table = if (ctx.env == 0) L.gt else o.hvalue(api.index2value(L, ctx.env));
    const source = lstring.newz(L, ctx.chunkname);

    const stringCount = r.varint();
    ctx.strings = mem.newArray(L, *TString, stringCount, 0);
    ctx.capstrings = stringCount;
    ctx.nstrings = 0;
    for (0..stringCount) |i| {
        const len = r.varint();
        ctx.strings.?[i] = lstring.new(L, r.bytes(len));
        ctx.nstrings = i + 1;
    }

    if (typesversion == 3) {
        // userdata type names; remapping only affects native code generation
        var index = r.byte();
        while (index != 0) {
            _ = readString(&r, ctx);
            index = r.byte();
        }
    }

    const protoCount = r.varint();
    ctx.protos = mem.newArray(L, *Proto, protoCount, 0);
    ctx.capprotos = protoCount;
    ctx.nprotos = 0;
    for (0..protoCount) |i| {
        var protoSize: u32 = 0;
        if (version >= 12) protoSize = r.varint();
        const protoStart = r.off;
        const p = func.newproto(L);
        // anchor: register before anything else can fail so that it is reachable
        ctx.protos.?[i] = p;
        ctx.nprotos = i + 1;
        p.source = source;
        p.bytecodeid = @intCast(i);
        p.funid = if (L.global.lastprotoid == 0) 0 else blk: {
            const id = L.global.lastprotoid;
            L.global.lastprotoid +%= 1;
            break :blk id;
        };
        p.maxstacksize = r.byte();
        p.numparams = r.byte();
        p.nups = r.byte();
        p.is_vararg = r.byte();
        if (version >= 4) {
            p.flags = r.byte();
            if (typesversion == 1) {
                const typesize = r.varint();
                _ = r.bytes(typesize);
            } else if (typesversion == 2 or typesversion == 3) {
                const typesize = r.varint();
                if (typesize != 0) {
                    const types = r.bytes(typesize);
                    const ti = mem.newArray(L, u8, typesize, p.gch.memcat);
                    @memcpy(ti[0..typesize], types);
                    p.typeinfo = ti;
                    p.sizetypeinfo = @intCast(typesize);
                }
            }
        }

        const sizecode = r.varint();
        p.code = mem.newArray(L, u32, sizecode, p.gch.memcat);
        p.sizecode = @intCast(sizecode);
        for (0..sizecode) |j| p.code[j] = r.int(u32);

        const sizek = r.varint();
        p.k = mem.newArray(L, TValue, sizek, p.gch.memcat);
        for (0..sizek) |j| p.k[j] = .{};
        p.sizek = @intCast(sizek);
        for (0..sizek) |j| {
            const kind = r.byte();
            switch (kind) {
                LBC_CONSTANT_NIL => {},
                LBC_CONSTANT_BOOLEAN => o.setbvalue(&p.k[j], r.byte() != 0),
                LBC_CONSTANT_NUMBER => o.setnvalue(&p.k[j], r.f64v()),
                LBC_CONSTANT_VECTOR => {
                    const x = r.f32v();
                    const y = r.f32v();
                    const z = r.f32v();
                    _ = r.f32v();
                    o.setvvalue(&p.k[j], x, y, z);
                },
                LBC_CONSTANT_VECTORD => {
                    const x = r.f64v();
                    const y = r.f64v();
                    const z = r.f64v();
                    _ = r.f64v();
                    o.setvvalue(&p.k[j], @floatCast(x), @floatCast(y), @floatCast(z));
                },
                LBC_CONSTANT_STRING => {
                    if (readString(&r, ctx)) |s| o.setsvalue(&p.k[j], s);
                },
                LBC_CONSTANT_IMPORT => {
                    const iid = r.int(u32);
                    resolveImportSafe(L, p.k, iid);
                    p.k[j] = (L.top - 1)[0];
                    L.top -= 1;
                },
                LBC_CONSTANT_TABLE => {
                    const keys = r.varint();
                    const h = table.new(L, 0, @intCast(keys));
                    o.sethvalue(&p.k[j], h);
                    for (0..keys) |_| {
                        const key = r.varint();
                        if (key >= sizek) debug.runerror(L, "malformed bytecode: bad constant reference", .{});
                        o.setnvalue(table.set(L, h, &p.k[key]), 0.0);
                    }
                },
                LBC_CONSTANT_TABLE_WITH_CONSTANTS => {
                    const keys = r.varint();
                    const h = table.new(L, 0, @intCast(keys));
                    o.sethvalue(&p.k[j], h);
                    const nilKeys = mem.newArray(L, u32, keys, 0);
                    var nilCount: usize = 0;
                    // nilKeys is a scratch array; errors below can only be malformed
                    // input or OOM, both of which abort loading entirely
                    for (0..keys) |_| {
                        const key = r.varint();
                        if (key >= sizek) debug.runerror(L, "malformed bytecode: bad constant reference", .{});
                        const val = table.set(L, h, &p.k[key]);
                        const constantIdx = r.int(i32);
                        if (constantIdx >= 0) {
                            if (constantIdx >= sizek) debug.runerror(L, "malformed bytecode: bad constant reference", .{});
                            const constant = &p.k[@intCast(constantIdx)];
                            if (o.ttisnil(constant)) {
                                nilKeys[nilCount] = key;
                                nilCount += 1;
                            } else {
                                val.* = constant.*;
                                continue;
                            }
                        }
                        o.setnvalue(val, 0.0);
                    }
                    for (0..nilCount) |idx| o.setnilvalue(table.set(L, h, &p.k[nilKeys[idx]]));
                    mem.freeArray(L, u32, nilKeys, keys, 0);
                },
                LBC_CONSTANT_CLOSURE => {
                    const fid = r.varint();
                    if (fid >= ctx.nprotos) debug.runerror(L, "malformed bytecode: bad proto reference", .{});
                    const fp = ctx.protos.?[fid];
                    const cl = func.newLclosure(L, fp.nups, envt, fp);
                    cl.preload = @intFromBool(cl.nupvalues > 0);
                    o.setclvalue(&p.k[j], cl);
                },
                LBC_CONSTANT_INTEGER => {
                    const isNegative = r.byte() != 0;
                    const magnitude = r.varint64();
                    o.setlvalue(&p.k[j], if (isNegative) @bitCast(~magnitude +% 1) else @bitCast(magnitude));
                },
                LBC_CONSTANT_CLASS_SHAPE => loadClassShape(&r, L, p, j, envt),
                else => debug.runerror(L, "malformed bytecode: unknown constant kind {d}", .{kind}),
            }
        }

        rewriteUdataOps(L, p);

        const sizep = r.varint();
        p.p = mem.newArray(L, *Proto, sizep, p.gch.memcat);
        for (0..sizep) |j| p.p[j] = p; // placeholder keeps the array traversable
        p.sizep = @intCast(sizep);
        for (0..sizep) |j| {
            const fid = r.varint();
            if (fid >= ctx.nprotos) debug.runerror(L, "malformed bytecode: bad proto reference", .{});
            p.p[j] = ctx.protos.?[fid];
        }

        p.linedefined = @intCast(r.varint());
        p.debugname = readString(&r, ctx);

        const lineinfo = r.byte();
        if (lineinfo != 0) {
            p.linegaplog2 = r.byte();
            if (p.linegaplog2 > 31 or sizecode == 0) debug.runerror(L, "malformed bytecode: bad line info", .{});
            const intervals: usize = ((sizecode - 1) >> @intCast(p.linegaplog2)) + 1;
            const absoffset: usize = (sizecode + 3) & ~@as(usize, 3);
            const sizelineinfo = absoffset + intervals * @sizeOf(c_int);
            const li = mem.newArray(L, u8, sizelineinfo, p.gch.memcat);
            p.lineinfo = li;
            p.sizelineinfo = @intCast(sizelineinfo);
            p.abslineinfo = @ptrCast(@alignCast(li + absoffset));
            var lastoffset: u8 = 0;
            for (0..sizecode) |j| {
                lastoffset +%= r.byte();
                li[j] = lastoffset;
            }
            var lastline: c_int = 0;
            for (0..intervals) |j| {
                lastline +%= r.int(i32);
                p.abslineinfo.?[j] = lastline;
            }
        }

        const debuginfo = r.byte();
        if (debuginfo != 0) {
            const sizelocvars = r.varint();
            const lv = mem.newArray(L, o.LocVar, sizelocvars, p.gch.memcat);
            for (0..sizelocvars) |j| lv[j] = .{ .varname = null, .startpc = 0, .endpc = 0, .reg = 0 };
            p.locvars = lv;
            p.sizelocvars = @intCast(sizelocvars);
            for (0..sizelocvars) |j| {
                lv[j].varname = readString(&r, ctx);
                lv[j].startpc = @intCast(r.varint());
                lv[j].endpc = @intCast(r.varint());
                lv[j].reg = r.byte();
            }
            const sizeupvalues = r.varint();
            const ups = mem.newArray(L, ?*TString, sizeupvalues, p.gch.memcat);
            for (0..sizeupvalues) |j| ups[j] = null;
            p.upvalues = ups;
            p.sizeupvalues = @intCast(sizeupvalues);
            for (0..sizeupvalues) |j| ups[j] = readString(&r, ctx);
        }

        if (version >= 11) {
            const fbsize = r.varint();
            if (fbsize > 0) {
                const fb = mem.newArray(L, o.FeedbackSlot, fbsize, p.gch.memcat);
                for (0..fbsize) |fi| fb[fi] = .{};
                p.feedbackvec = fb;
                p.feedbackvecsize = fbsize;
            }
            for (0..fbsize) |j| {
                const slottype = r.byte();
                const slot = &p.feedbackvec.?[j];
                slot.kind = slottype;
                if (slottype == LFT_CALLTARGET) {
                    slot.pc_or_shape = r.varint();
                } else if (slottype == LFT_CONSTRUCT) {
                    slot.pc_or_shape = r.varint();
                    slot.match = o.LCM_MISMATCH;
                } else debug.runerror(L, "malformed bytecode: unknown feedback slot type", .{});
            }
        }
        if (version >= 12) {
            if ((p.flags & LPF_INLINABLE) != 0) p.cost = r.varint64();
            if (protoSize > ctx.data.len - protoStart) r.fail();
            r.off = protoStart + protoSize;
        }
        validateProto(L, p, ctx.nprotos);
    }

    const mainid = r.varint();
    if (mainid >= ctx.nprotos) debug.runerror(L, "malformed bytecode: bad main proto", .{});
    const main = ctx.protos.?[mainid];
    @import("gc.zig").threadbarrier(L);
    const cl = func.newLclosure(L, 0, envt, main);
    ldo.checkstack(L, 1);
    o.setclvalue(&L.top[0], cl);
    L.top += 1;
    return 0;
}

/// Upstream getOpLength (BytecodeUtils.h).
fn opLength(op: u8) usize {
    const Op = @import("execute.zig").Op;
    return switch (@as(Op, @enumFromInt(op))) {
        .GETGLOBAL, .SETGLOBAL, .GETIMPORT, .GETTABLEKS, .SETTABLEKS, .NAMECALL, .JUMPIFEQ, .JUMPIFLE, .JUMPIFLT, .JUMPIFNOTEQ, .JUMPIFNOTLE, .JUMPIFNOTLT, .NEWTABLE, .SETLIST, .FORGLOOP, .LOADKX, .FASTCALL2, .FASTCALL2K, .FASTCALL3, .JUMPXEQKNIL, .JUMPXEQKB, .JUMPXEQKN, .JUMPXEQKS, .GETUDATAKS, .SETUDATAKS, .NAMECALLUDATA, .NEWCLASSMEMBER, .CALLFB, .CMPPROTO, .NEWCLASS, .CONSTRUCT, .GETPRIVATEFIELD, .SETPRIVATEFIELD => 2,
        else => 1,
    };
}

/// Upstream lvmload: string-keyed GETTABLEKS/SETTABLEKS/NAMECALL whose key has a
/// host atom (lua_Callbacks.useratom) become GETUDATAKS/SETUDATAKS/NAMECALLUDATA,
/// which dispatch to lua_registeruserdatadirectaccess callbacks and deoptimize back
/// otherwise. The aux word keeps the constant index in its low 16 bits.
fn rewriteUdataOps(L: *State, p: *Proto) void {
    const Op = @import("execute.zig").Op;
    const n: usize = @intCast(p.sizecode);
    var i: usize = 0;
    while (i < n) {
        const insn = p.code[i];
        const op: u8 = @truncate(insn);
        const target: ?Op = switch (@as(Op, @enumFromInt(op))) {
            .GETTABLEKS => .GETUDATAKS,
            .SETTABLEKS => .SETUDATAKS,
            .NAMECALL => .NAMECALLUDATA,
            else => null,
        };
        if (target) |t| {
            if (i + 1 >= n) debug.runerror(L, "malformed bytecode: truncated instruction", .{});
            const aux = p.code[i + 1];
            if (aux >= @as(u32, @intCast(p.sizek))) debug.runerror(L, "malformed bytecode: bad constant reference", .{});
            if (aux < 0x10000) {
                const kv = &p.k[aux];
                if (o.ttisstring(kv)) {
                    const s = o.tsvalue(kv);
                    lstring.updateatom(L, s);
                    if (s.atom >= 0) p.code[i] = (insn & 0xffffff00) | @intFromEnum(t);
                }
            }
        }
        i += opLength(op);
    }
}

fn kstring(L: *State, p: *Proto, idx: u32) *TString {
    if (idx >= @as(u32, @intCast(p.sizek)) or !o.ttisstring(&p.k[idx])) debug.runerror(L, "malformed bytecode: bad class member name", .{});
    return o.tsvalue(&p.k[idx]);
}

/// LBC_CONSTANT_CLASS_SHAPE (bytecode version 100, experimental classes).
fn loadClassShape(r: *Reader, L: *State, p: *Proto, j: usize, envt: *o.Table) void {
    const classname = kstring(L, p, r.varint());
    const numPrivateFields = r.byte();
    const numPrivateMethods = r.byte();
    const numProperties = r.varint();
    const numMethods = r.varint();
    const numMembers = numMethods + numProperties;
    if (numPrivateMethods > numMethods or numPrivateFields > numProperties) debug.runerror(L, "malformed bytecode: bad class shape", .{});
    const withoutPrivateMethods = numMembers - numPrivateMethods;
    const otm = mem.newArray(L, ?*TString, withoutPrivateMethods, L.activememcat);
    for (0..withoutPrivateMethods) |i| otm[i] = null;
    const m2o = table.new(L, 0, @intCast(withoutPrivateMethods - numPrivateFields));
    var idx: u32 = 0;
    while (idx < numProperties - numPrivateFields) : (idx += 1) {
        const name = kstring(L, p, r.varint());
        otm[idx] = name;
        o.setnvalue(table.setstr(L, m2o, name), @floatFromInt(idx));
    }
    idx = numProperties;
    while (idx < withoutPrivateMethods) : (idx += 1) {
        const name = kstring(L, p, r.varint());
        otm[idx] = name;
        o.setnvalue(table.setstr(L, m2o, name), @floatFromInt(idx));
    }
    m2o.readonly = 1;
    const c = @import("class.zig").newclass(L, classname, m2o, otm, numProperties, numMethods, numPrivateFields, numPrivateMethods, envt);
    o.setclassvalue(&p.k[j], c);
}

/// Light structural validation so that the interpreter never indexes outside the
/// constant/proto/register arrays of a well-formed-looking but corrupted proto.
fn validateProto(L: *State, p: *Proto, nprotos: usize) void {
    _ = nprotos;
    if (p.sizecode == 0) debug.runerror(L, "malformed bytecode: empty function", .{});
    if (p.numparams > p.maxstacksize and p.is_vararg == 0 and p.maxstacksize != 0)
        debug.runerror(L, "malformed bytecode: bad parameter count", .{});
}

fn loadBody(L: *State, ud: ?*anyopaque) void {
    const ctx: *Ctx = @ptrCast(@alignCast(ud.?));
    ctx.result = loadsafe(L, ctx);
}

/// luau_load: loads bytecode and pushes the main function (returns 0), or pushes an
/// error message (returns 1).
pub fn load(L: *State, chunkname: [*:0]const u8, data: []const u8, env: c_int) c_int {
    @import("gc.zig").checkGC(L);
    const g = L.global;
    const oldthreshold = g.GCthreshold;
    g.GCthreshold = std.math.maxInt(usize);
    var ctx: Ctx = .{ .chunkname = chunkname, .data = data, .env = env };
    const top = ldo.savestack(L, L.top);
    const status = if (data.len == 0) blk: {
        ctx.result = 1;
        break :blk @as(c_int, 0);
    } else ldo.rawrunprotected(L, loadBody, &ctx);
    if (ctx.strings) |s| mem.freeArray(L, *TString, s, ctx.capstrings, 0);
    if (ctx.protos) |p| mem.freeArray(L, *Proto, p, ctx.capprotos, 0);
    g.GCthreshold = oldthreshold;
    if (data.len == 0) {
        pushChunkError(L, chunkname, ": empty bytecode", .{});
        return 1;
    }
    if (status == o.LUA_ERRMEM) {
        L.top = ldo.restorestack(L, top);
        api.pushstringRaw(L, o.LUA_MEMERRMSG);
        return 1;
    }
    if (status != 0) {
        // malformed input: the error message is on top of the stack
        const msg = (L.top - 1)[0];
        L.top = ldo.restorestack(L, top);
        L.top[0] = msg;
        L.top += 1;
        return 1;
    }
    return ctx.result;
}
