//! Miscellaneous FASTCALL implementations, matching upstream lbuiltins.cpp (MIT).
const std = @import("std");
const builtin = @import("builtin");
const o = @import("object.zig");
const table = @import("table.zig");
const gc = @import("gc.zig");
const strings = @import("string.zig");
const num = @import("numprint.zig");
const lua_do = @import("do.zig");

fn needsGC(L: *o.State) bool {
    return L.global.totalbytes >= L.global.GCthreshold;
}

fn bufferOp(id: u8, result: o.StkId, arg0: *o.TValue, args: ?o.StkId, nparams: c_int, nresults: c_int) ?c_int {
    if (builtin.cpu.arch.endian() != .little) return null;
    const writing = switch (id) { 67, 70, 73, 75, 77, 132 => true, else => false };
    if (nparams < (if (writing) @as(c_int, 3) else 2) or nresults > (if (writing) @as(c_int, 0) else 1) or !o.ttisbuffer(arg0)) return null;
    const a = args.?;
    if (!o.ttisnumber(&a[0])) return null;
    if (writing and !(if (id == 132) o.ttisinteger(&a[1]) else o.ttisnumber(&a[1]))) return null;
    const size: usize = switch (id) { 65, 66, 67 => 1, 68, 69, 70 => 2, 71, 72, 73, 74, 75 => 4, 76, 77, 131, 132 => 8, else => unreachable };
    const b = o.bufvalue(arg0);
    const offset = o.num2int(o.nvalue(&a[0]));
    if (offset < 0 or @as(u64, @intCast(offset)) + size > b.len) return null;
    const p = b.data() + @as(usize, @intCast(offset));
    if (writing) {
        const bits: u64 = switch (id) {
            67, 70, 73 => o.num2unsigned(o.nvalue(&a[1])),
            75 => @as(u32, @bitCast(@as(f32, @floatCast(o.nvalue(&a[1]))))),
            77 => @bitCast(o.nvalue(&a[1])),
            132 => @bitCast(o.lvalue(&a[1])),
            else => unreachable,
        };
        for (0..size) |i| p[i] = @truncate(bits >> @as(u6, @intCast(i * 8)));
        return 0;
    }
    var bits: u64 = 0;
    for (0..size) |i| bits |= @as(u64, p[i]) << @as(u6, @intCast(i * 8));
    switch (id) {
        65 => o.setnvalue(&result[0], @floatFromInt(@as(i8, @bitCast(@as(u8, @truncate(bits)))))),
        66 => o.setnvalue(&result[0], @floatFromInt(@as(u8, @truncate(bits)))),
        68 => o.setnvalue(&result[0], @floatFromInt(@as(i16, @bitCast(@as(u16, @truncate(bits)))))),
        69 => o.setnvalue(&result[0], @floatFromInt(@as(u16, @truncate(bits)))),
        71 => o.setnvalue(&result[0], @floatFromInt(@as(i32, @bitCast(@as(u32, @truncate(bits)))))),
        72 => o.setnvalue(&result[0], @floatFromInt(@as(u32, @truncate(bits)))),
        74 => o.setnvalue(&result[0], @as(f32, @bitCast(@as(u32, @truncate(bits))))),
        76 => o.setnvalue(&result[0], @bitCast(bits)),
        131 => o.setlvalue(&result[0], @bitCast(bits)),
        else => unreachable,
    }
    return 1;
}

pub fn run(L: *o.State, id: u8, result: o.StkId, arg0: *o.TValue, args: ?o.StkId, nparams: c_int, nresults: c_int) ?c_int {
    if ((id >= 65 and id <= 77) or id == 131 or id == 132) return bufferOp(id, result, arg0, args, nparams, nresults);
    switch (id) {
        41 => {
            if (nparams < 2 or !o.ttisstring(arg0) or !o.ttisnumber(&args.?[0])) return null;
            const a = args.?;
            const s = o.tsvalue(arg0).slice();
            const i = o.num2int(o.nvalue(&a[0]));
            const j = if (nparams >= 3) (if (o.ttisnumber(&a[1])) o.num2int(o.nvalue(&a[1])) else 0) else i;
            if (i < 1 or j < i or j > s.len) return null;
            const count = j - i + 1;
            if (count != (if (nresults < 0) @as(c_int, 1) else nresults)) return null;
            for (0..@intCast(count)) |k| o.setnvalue(&result[k], @floatFromInt(s[@as(usize, @intCast(i - 1)) + k]));
            return count;
        },
        42 => {
            if (nparams < 0 or nparams >= 8 or nresults > 1 or needsGC(L)) return null;
            var bytes: [8]u8 = undefined;
            for (0..@intCast(nparams)) |i| {
                const v = if (i == 0) arg0 else &args.?[i - 1];
                if (!o.ttisnumber(v)) return null;
                const ch = o.num2int(o.nvalue(v));
                if (ch < 0 or ch > 255) return null;
                bytes[i] = @intCast(ch);
            }
            o.setsvalue(&result[0], strings.new(L, bytes[0..@intCast(nparams)]));
            return 1;
        },
        43, 58 => {
            if (nparams < 1 or nresults > 1) return null;
            const len: c_int = if (o.ttisstring(arg0)) @intCast(o.tsvalue(arg0).len) else if (id == 58 and o.ttistable(arg0)) table.getn(o.hvalue(arg0)) else return null;
            o.setnvalue(&result[0], @floatFromInt(len));
            return 1;
        },
        45 => {
            if (nparams < 3 or nresults > 1 or !o.ttisstring(arg0) or !o.ttisnumber(&args.?[0]) or !o.ttisnumber(&args.?[1]) or needsGC(L)) return null;
            const s = o.tsvalue(arg0).slice();
            const i = o.num2int(o.nvalue(&args.?[0]));
            const j = o.num2int(o.nvalue(&args.?[1]));
            if (i < 1 or j < i or j > s.len) return null;
            o.setsvalue(&result[0], strings.new(L, s[@intCast(i - 1)..@intCast(j)]));
            return 1;
        },
        49 => {
            if (nparams < 3 or nresults > 1 or !o.ttistable(arg0)) return null;
            const a = args.?;
            const key = &a[0];
            if (o.ttisnil(key) or (o.ttisnumber(key) and std.math.isNan(o.nvalue(key)))) return null;
            if (o.ttisvector(key)) for (o.vvalue(key)) |v| { if (std.math.isNan(v)) return null; };
            const t = o.hvalue(arg0);
            if (t.readonly != 0) return null;
            result[0] = arg0.*;
            table.set(L, t, key).* = a[1];
            gc.barriert(L, t, &a[1]);
            return 1;
        },
        50 => {
            if (nparams < 2 or nresults > 1 or !o.ttistable(arg0)) return null;
            result[0] = table.get(o.hvalue(arg0), &args.?[0]).*;
            return 1;
        },
        51 => {
            if (nparams < 2 or nresults > 1) return null;
            const equal = table.rawequalObj(arg0, &args.?[0]);
            o.setbvalue(&result[0], equal);
            return 1;
        },
        52 => {
            if (nparams != 2 or nresults > 0 or !o.ttistable(arg0)) return null;
            const t = o.hvalue(arg0);
            if (t.readonly != 0) return null;
            table.setnum(L, t, table.getn(t) + 1).* = args.?[0];
            gc.barriert(L, t, &args.?[0]);
            return 0;
        },
        53 => {
            if (nparams < 1 or nresults >= 0 or !o.ttistable(arg0)) return null;
            const t = o.hvalue(arg0);
            const n = if (nparams == 1) table.getn(t) else if (nparams == 3 and o.ttisnumber(&args.?[0]) and o.ttisnumber(&args.?[1]) and o.nvalue(&args.?[0]) == 1) o.num2int(o.nvalue(&args.?[1])) else -1;
            if (n < 0 or n > t.sizearray or o.sdiff(L.stack_last, result) < n or n + nparams > o.LUAI_MAXCSTACK) return null;
            for (0..@intCast(n)) |i| result[i] = t.arr()[i];
            lua_do.expandstacklimit(L, result + @as(usize, @intCast(n)));
            return n;
        },
        60 => {
            if (nparams < 1 or nresults > 1) return null;
            const mt = if (o.ttistable(arg0)) o.hvalue(arg0).metatable else if (o.ttisuserdata(arg0)) o.uvalue(arg0).metatable else L.global.mt[o.ttype(arg0)];
            if (mt) |m| {
                const protected = table.getstr(m, L.global.tmname[@intFromEnum(o.TMS.METATABLE)]);
                if (!o.ttisnil(protected)) result[0] = protected.* else o.sethvalue(&result[0], m);
            } else o.setnilvalue(&result[0]);
            return 1;
        },
        61 => {
            if (nparams < 2 or nresults > 1 or !o.ttistable(arg0) or !o.ttistable(&args.?[0])) return null;
            const t = o.hvalue(arg0);
            if (t.readonly != 0 or t.metatable != null) return null;
            const mt = o.hvalue(&args.?[0]);
            t.metatable = mt;
            gc.objbarrier(L, &t.gch, &mt.gch);
            o.sethvalue(&result[0], t);
            return 1;
        },
        62 => {
            if (nparams != 1 or nresults > 1) return null;
            const v: ?f64 = if (o.ttisnumber(arg0)) o.nvalue(arg0) else if (o.ttisstring(arg0)) num.str2d(o.tsvalue(arg0).slice()) else null;
            if (v) |n| o.setnvalue(&result[0], n) else o.setnilvalue(&result[0]);
            return 1;
        },
        63 => {
            if (nparams < 1 or nresults > 1) return null;
            const s = switch (o.ttype(arg0)) {
                o.T_NIL => L.global.ttname[o.T_NIL],
                o.T_BOOLEAN => strings.new(L, if (o.bvalue(arg0) != 0) "true" else "false"),
                o.T_STRING => o.tsvalue(arg0),
                o.T_NUMBER => blk: {
                    if (needsGC(L)) return null;
                    var buf: [num.MAXNUM2STR]u8 = undefined;
                    break :blk strings.new(L, num.num2str(&buf, o.nvalue(arg0)));
                },
                o.T_INTEGER => blk: {
                    if (needsGC(L)) return null;
                    var buf: [32]u8 = undefined;
                    break :blk strings.new(L, num.int2str(&buf, o.lvalue(arg0)));
                },
                else => return null,
            };
            o.setsvalue(&result[0], s);
            return 1;
        },
        133 => {
            if (nparams < 1 or nresults > 1 or !o.ttisnumber(arg0)) return null;
            const size = o.num2int(o.nvalue(arg0));
            if (size < 0 or needsGC(L)) return null;
            const t = table.new(L, size, 0);
            if (nparams >= 2) for (0..@intCast(size)) |i| { t.arr()[i] = args.?[0]; };
            o.sethvalue(&result[0], t);
            return 1;
        },
        134 => {
            if (nparams < 1 or nresults > 0 or !o.ttistable(arg0)) return null;
            const t = o.hvalue(arg0);
            if (t.readonly != 0) return null;
            table.clear(t);
            return 0;
        },
        else => return null,
    }
}

test "buffer fast paths and fallback preserve bytes" {
    var storage: [@sizeOf(o.Buffer) + 16]u8 align(@alignOf(o.Buffer)) = @splat(0);
    const b: *o.Buffer = @ptrCast(&storage);
    b.len = 8;
    var arg: o.TValue = .{};
    o.setbufvalue(&arg, b);
    var args = [_]o.TValue{ .{}, .{} };
    o.setnvalue(&args[0], 0);
    o.setnvalue(&args[1], 4660);
    var result: [1]o.TValue = .{.{}};
    var L: o.State = undefined;
    try std.testing.expectEqual(@as(?c_int, 0), run(&L, 70, &result, &arg, &args, 3, 0));
    try std.testing.expectEqual(@as(?c_int, 1), run(&L, 69, &result, &arg, &args, 2, 1));
    try std.testing.expectEqual(@as(f64, 4660), o.nvalue(&result[0]));
    o.setnvalue(&args[0], 7);
    try std.testing.expectEqual(@as(?c_int, null), run(&L, 70, &result, &arg, &args, 3, 0));
    try std.testing.expectEqual(@as(u8, 0), b.data()[7]);
}
