//! Fast builtins for vectors, integers, and basic type operations.
//! Semantics follow third_party/luau/VM/src/lbuiltins.cpp (MIT).
const std = @import("std");
const o = @import("object.zig");
const tm = @import("tm.zig");

pub fn run(L: *o.State, id: u8, result: o.StkId, arg0: *o.TValue, args: ?o.StkId, nparams: c_int, nresults: c_int) ?c_int {
    if (id == 1) return if (nparams >= 1 and nresults == 0 and !o.ttisnil(arg0) and !(o.ttisboolean(arg0) and o.bvalue(arg0) == 0)) 0 else null;
    if (id == 40 or id == 44) {
        if (nparams < 1 or nresults > 1) return null;
        o.setsvalue(&result[0], if (id == 40) L.global.ttname[o.ttype(arg0)] else tm.objtypenamestr(L, arg0));
        return 1;
    }
    if (id == 57) {
        if (nparams != 1 or nresults != 1) return null;
        const count: isize = @divExact(@as(isize, @intCast(@intFromPtr(L.base))) - @as(isize, @intCast(@intFromPtr(L.ci.func))), @sizeOf(o.TValue)) - @as(isize, L.ci.p.?.numparams) - 1;
        if (o.ttisnumber(arg0)) {
            const x = o.nvalue(arg0);
            if (!(x >= 1 and x < 2147483648.0)) return null;
            const i: isize = @intFromFloat(x);
            if (i > count) return null;
            o.setobj(&result[0], &(L.base - @as(usize, @intCast(count)))[@intCast(i - 1)]);
            return 1;
        }
        if (o.ttisstring(arg0) and o.tsvalue(arg0).len > 0 and o.tsvalue(arg0).data()[0] == '#') {
            o.setnvalue(&result[0], @floatFromInt(count));
            return 1;
        }
        return null;
    }
    if (id == 54) {
        if (nparams < 2 or nresults > 1 or !o.ttisnumber(arg0) or !o.ttisnumber(&args.?[0])) return null;
        if (nparams >= 3 and !o.ttisnumber(&args.?[1])) return null;
        const x: f32 = @floatCast(o.nvalue(arg0));
        const y: f32 = @floatCast(o.nvalue(&args.?[0]));
        const z: f32 = if (nparams >= 3) @floatCast(o.nvalue(&args.?[1])) else 0;
        o.setvvalue(&result[0], x, y, z);
        return 1;
    }
    if ((id >= 78 and id <= 88) or id == 90) {
        if (nparams < 1 or nresults > 1 or !o.ttisvector(arg0)) return null;
        const a = o.vvalue(arg0);
        var r = a;
        if (id == 78 or id == 79) {
            const sum = a[0]*a[0] + a[1]*a[1] + a[2]*a[2];
            const magnitude = @sqrt(sum);
            if (id == 78) { o.setnvalue(&result[0], magnitude); return 1; }
            const inv = @as(f32, 1) / magnitude;
            for (&r) |*v| v.* *= inv;
        } else if (id >= 82 and id <= 85) {
            for (&r) |*v| v.* = switch (id) { 82 => @floor(v.*), 83 => @ceil(v.*), 84 => @abs(v.*), else => if (v.* > 0) 1 else if (v.* < 0) -1 else 0 };
        } else {
            if (nparams < 2 or !o.ttisvector(&args.?[0])) return null;
            const b = o.vvalue(&args.?[0]);
            switch (id) {
                80 => r = .{ a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0] },
                81 => { o.setnvalue(&result[0], a[0]*b[0]+a[1]*b[1]+a[2]*b[2]); return 1; },
                86 => {
                    if (nparams < 3 or !o.ttisvector(&args.?[1])) return null;
                    const c = o.vvalue(&args.?[1]);
                    for (0..3) |i| if (!(b[i] <= c[i])) { return null; };
                    for (0..3) |i| r[i] = if (a[i] < b[i]) b[i] else if (a[i] > c[i]) c[i] else a[i];
                },
                87, 88 => {
                    for (0..3) |i| r[i] = if (if (id == 87) b[i] < r[i] else b[i] > r[i]) b[i] else r[i];
                    var j: c_int = 2;
                    while (j < nparams) : (j += 1) {
                        const v = &args.?[@intCast(j-1)];
                        if (!o.ttisvector(v)) return null;
                        const c = o.vvalue(v);
                        for (0..3) |i| r[i] = if (if (id == 87) c[i] < r[i] else c[i] > r[i]) c[i] else r[i];
                    }
                },
                90 => {
                    if (nparams < 3 or !o.ttisnumber(&args.?[1])) return null;
                    const t: f32 = @floatCast(o.nvalue(&args.?[1]));
                    for (0..3) |i| r[i] = if (t == 1) b[i] else a[i] + (b[i] - a[i]) * t;
                },
                else => return null,
            }
        }
        o.setvvalue(&result[0], r[0], r[1], r[2]);
        return 1;
    }
    if (id >= 94 and id <= 130) return integer(id, result, arg0, args, nparams, nresults);
    return null;
}

fn integer(id: u8, result: o.StkId, arg0: *o.TValue, args: ?o.StkId, nparams: c_int, nresults: c_int) ?c_int {
    if (nparams < 1 or nresults > 1) return null;
    if (id == 94) {
        if (!o.ttisnumber(arg0)) return null;
        const x = o.nvalue(arg0);
        if (x >= -9223372036854775808.0 and x < 9223372036854775808.0 and @trunc(x) == x) o.setlvalue(&result[0], @intFromFloat(x)) else o.setnilvalue(&result[0]);
        return 1;
    }
    if (!o.ttisinteger(arg0)) return null;
    const a = o.lvalue(arg0);
    const ua: u64 = @bitCast(a);
    if (id == 95) { o.setnvalue(&result[0], @floatFromInt(a)); return 1; }
    if (id == 96 or id == 111 or id >= 128) {
        const v: i64 = switch (id) { 96 => 0 -% a, 111 => ~a, 128 => @intCast(@ctz(ua)), 129 => @intCast(@clz(ua)), 130 => @bitCast(@byteSwap(ua)), else => unreachable };
        o.setlvalue(&result[0], v); return 1;
    }
    if (id == 109 or id == 110 or id == 112 or id == 127 or id == 101 or id == 102) {
        if ((id == 101 or id == 102) and nparams < 2) return null;
        var v = a;
        var i: c_int = 1;
        while (i < nparams) : (i += 1) {
            const p = &args.?[@intCast(i-1)];
            if (!o.ttisinteger(p)) return null;
            const b = o.lvalue(p);
            v = switch (id) { 109, 127 => v & b, 110 => v | b, 112 => v ^ b, 101 => @min(v,b), 102 => @max(v,b), else => unreachable };
        }
        if (id == 127) o.setbvalue(&result[0], v != 0) else o.setlvalue(&result[0], v);
        return 1;
    }
    if (nparams < 2 or !o.ttisinteger(&args.?[0])) return null;
    const b = o.lvalue(&args.?[0]);
    const ub: u64 = @bitCast(b);
    if (id >= 113 and id <= 120) {
        const v = switch (id) { 113 => a < b, 114 => a <= b, 115 => ua < ub, 116 => ua <= ub, 117 => a > b, 118 => a >= b, 119 => ua > ub, 120 => ua >= ub, else => unreachable };
        o.setbvalue(&result[0], v); return 1;
    }
    var v: i64 = undefined;
    switch (id) {
        97 => v = a +% b, 98 => v = a -% b, 99 => v = a *% b,
        100, 104 => { if (b == 0 or (a == std.math.minInt(i64) and b == -1)) return null; v = if (id == 100) @divTrunc(a,b) else @divFloor(a,b); },
        103, 107 => { if (b == 0) return null; v = if (a == std.math.minInt(i64) and b == -1) 0 else if (id == 103) @rem(a,b) else @mod(a,b); },
        105, 106 => { if (b == 0) return null; v = @bitCast(if (id == 105) ua / ub else ua % ub); },
        108 => { if (nparams < 3 or !o.ttisinteger(&args.?[1])) return null; const c = o.lvalue(&args.?[1]); if (b > c) return null; v = @min(@max(a,b),c); },
        121, 122, 123 => {
            if (b < -63 or b > 63) v = if (id == 123 and b > 63 and a < 0) -1 else 0 else {
                const shift: u6 = @intCast(if (b < 0) -b else b);
                v = if (id == 121) @bitCast(if (b < 0) ua >> shift else ua << shift) else if (id == 122) @bitCast(if (b < 0) ua << shift else ua >> shift) else if (b < 0) @bitCast(ua << shift) else a >> shift;
            }
        },
        124, 125 => { const s: u6 = @truncate(ub); v = @bitCast(if (id == 124) std.math.rotl(u64, ua, s) else std.math.rotr(u64, ua, s)); },
        126 => { if (nparams >= 3 and !o.ttisinteger(&args.?[1])) return null; const w: i64 = if (nparams >= 3) o.lvalue(&args.?[1]) else 1; if (b < 0 or b > 63 or w < 1 or w > 64 or b + w > 64) return null; const s: u6 = @intCast(b); const mask: u64 = if (w == 64) ~@as(u64,0) else (@as(u64,1) << @as(u6,@intCast(w))) - 1; v = @bitCast((ua >> s) & mask); },
        else => return null,
    }
    o.setlvalue(&result[0], v); return 1;
}

test "integer fast paths wrap and preserve fallback arguments" {
    var a = o.TValue{};
    var b = [_]o.TValue{.{}};
    var out = [_]o.TValue{.{}};
    o.setlvalue(&a, std.math.maxInt(i64));
    o.setlvalue(&b[0], 1);
    try std.testing.expectEqual(@as(?c_int, 1), integer(97, &out, &a, &b, 2, 1));
    try std.testing.expectEqual(std.math.minInt(i64), o.lvalue(&out[0]));
    o.setlvalue(&b[0], 0);
    try std.testing.expectEqual(@as(?c_int, null), integer(100, &out, &a, &b, 2, 1));
    try std.testing.expectEqual(std.math.minInt(i64), o.lvalue(&out[0]));
}

test "vector creation and fallback" {
    var state: o.State = undefined;
    var a = o.TValue{};
    var args = [_]o.TValue{ .{}, .{} };
    var result = [_]o.TValue{.{}};
    o.setnvalue(&a, 1);
    o.setnvalue(&args[0], 2);
    o.setnvalue(&args[1], 3);
    try std.testing.expectEqual(@as(?c_int, 1), run(&state, 54, &result, &a, &args, 3, 1));
    try std.testing.expectEqual(@as(f32, 3), o.vvalue(&result[0])[2]);
    o.setnilvalue(&args[1]);
    const before = result[0];
    try std.testing.expectEqual(@as(?c_int, null), run(&state, 54, &result, &a, &args, 3, 1));
    try std.testing.expectEqualSlices(u8, std.mem.asBytes(&before), std.mem.asBytes(&result[0]));
}

test "all integer IDs return expected values and preserve fallback state" {
    const cases = [_]struct { id: u8, expected: i64 }{
        .{ .id = 94, .expected = 6 }, .{ .id = 95, .expected = 6 },
        .{ .id = 96, .expected = -6 }, .{ .id = 97, .expected = 8 },
        .{ .id = 98, .expected = 4 }, .{ .id = 99, .expected = 12 },
        .{ .id = 100, .expected = 3 }, .{ .id = 101, .expected = 2 },
        .{ .id = 102, .expected = 8 }, .{ .id = 103, .expected = 0 },
        .{ .id = 104, .expected = 3 }, .{ .id = 105, .expected = 3 },
        .{ .id = 106, .expected = 0 }, .{ .id = 107, .expected = 0 },
        .{ .id = 108, .expected = 6 }, .{ .id = 109, .expected = 0 },
        .{ .id = 110, .expected = 14 }, .{ .id = 111, .expected = -7 },
        .{ .id = 112, .expected = 12 }, .{ .id = 113, .expected = 0 },
        .{ .id = 114, .expected = 0 }, .{ .id = 115, .expected = 0 },
        .{ .id = 116, .expected = 0 }, .{ .id = 117, .expected = 1 },
        .{ .id = 118, .expected = 1 }, .{ .id = 119, .expected = 1 },
        .{ .id = 120, .expected = 1 }, .{ .id = 121, .expected = 24 },
        .{ .id = 122, .expected = 1 }, .{ .id = 123, .expected = 1 },
        .{ .id = 124, .expected = 24 }, .{ .id = 125, .expected = -9223372036854775807 },
        .{ .id = 126, .expected = 1 }, .{ .id = 127, .expected = 0 },
        .{ .id = 128, .expected = 1 }, .{ .id = 129, .expected = 61 },
        .{ .id = 130, .expected = 432345564227567616 },
    };
    var state: o.State = undefined;
    for (cases) |case| {
        var a = o.TValue{};
        var args = [_]o.TValue{ .{}, .{} };
        var out = [_]o.TValue{.{}};
        if (case.id == 94) o.setnvalue(&a, 6) else o.setlvalue(&a, 6);
        o.setlvalue(&args[0], 2);
        o.setlvalue(&args[1], if (case.id == 126) 1 else 8);
        try std.testing.expectEqual(@as(?c_int, 1), run(&state, case.id, &out, &a, &args, 3, 1));
        if (case.id == 95) {
            try std.testing.expectEqual(@as(f64, 6), o.nvalue(&out[0]));
        } else if ((case.id >= 113 and case.id <= 120) or case.id == 127) {
            try std.testing.expectEqual(@as(c_int, @intCast(case.expected)), o.bvalue(&out[0]));
        } else {
            try std.testing.expectEqual(case.expected, o.lvalue(&out[0]));
        }
        o.setnilvalue(&a);
        const before = out[0];
        const before_args = args;
        try std.testing.expectEqual(@as(?c_int, null), run(&state, case.id, &out, &a, &args, 3, 1));
        try std.testing.expectEqualSlices(u8, std.mem.asBytes(&before), std.mem.asBytes(&out[0]));
        try std.testing.expectEqualSlices(u8, std.mem.asBytes(&before_args), std.mem.asBytes(&args));
        try std.testing.expect(o.ttisnil(&a));
    }
}

test "vector library IDs and guard failures" {
    var state: o.State = undefined;
    const ids = [_]u8{ 78,79,80,81,82,83,84,85,86,87,88,90 };
    for (ids) |id| {
        var a = o.TValue{};
        var args = [_]o.TValue{ .{}, .{} };
        var out = [_]o.TValue{.{}};
        o.setvvalue(&a, 3, 4, 0);
        o.setvvalue(&args[0], 1, 2, 3);
        if (id == 90) o.setnvalue(&args[1], 0.5) else o.setvvalue(&args[1], 5, 6, 7);
        try std.testing.expectEqual(@as(?c_int, 1), run(&state, id, &out, &a, &args, 3, 1));
        if (id == 78 or id == 81) {
            try std.testing.expectEqual(if (id == 78) @as(f64, 5) else 11, o.nvalue(&out[0]));
        } else {
            const expected: [3]f32 = switch (id) {
                79 => .{ 0.6, 0.8, 0 }, 80 => .{ 12, -9, 2 },
                82, 83, 84 => .{ 3, 4, 0 }, 85 => .{ 1, 1, 0 },
                86 => .{ 3, 4, 3 }, 87 => .{ 1, 2, 0 },
                88 => .{ 5, 6, 7 }, 90 => .{ 2, 3, 1.5 }, else => unreachable,
            };
            try std.testing.expectEqualDeep(expected, o.vvalue(&out[0]));
        }
        const before = out[0];
        const before_a = a;
        const before_args = args;
        try std.testing.expectEqual(@as(?c_int, null), run(&state, id, &out, &a, &args, 3, 2));
        try std.testing.expectEqualSlices(u8, std.mem.asBytes(&before), std.mem.asBytes(&out[0]));
        try std.testing.expectEqualSlices(u8, std.mem.asBytes(&before_a), std.mem.asBytes(&a));
        try std.testing.expectEqualSlices(u8, std.mem.asBytes(&before_args), std.mem.asBytes(&args));
    }
}

test "integer boundary and guard behavior" {
    var L: o.State = undefined;
    var a = o.TValue{};
    var b = [_]o.TValue{ .{}, .{} };
    var out = [_]o.TValue{.{}};
    o.setlvalue(&a, std.math.minInt(i64));
    o.setlvalue(&b[0], -1);
    for ([_]u8{ 100, 104 }) |id| {
        o.setlvalue(&out[0], 123);
        try std.testing.expectEqual(@as(?c_int, null), run(&L, id, &out, &a, &b, 2, 1));
        try std.testing.expectEqual(@as(i64, 123), o.lvalue(&out[0]));
    }
    for ([_]u8{ 103, 107 }) |id| {
        try std.testing.expectEqual(@as(?c_int, 1), run(&L, id, &out, &a, &b, 2, 1));
        try std.testing.expectEqual(@as(i64, 0), o.lvalue(&out[0]));
    }
    o.setlvalue(&b[0], 0);
    for ([_]u8{ 100, 103, 104, 105, 106, 107 }) |id| {
        o.setlvalue(&out[0], 123);
        try std.testing.expectEqual(@as(?c_int, null), run(&L, id, &out, &a, &b, 2, 1));
        try std.testing.expectEqual(@as(i64, 123), o.lvalue(&out[0]));
    }
    o.setlvalue(&b[0], 64);
    for ([_]u8{ 121, 122, 123, 124, 125 }) |id| {
        try std.testing.expectEqual(@as(?c_int, 1), run(&L, id, &out, &a, &b, 2, 1));
        try std.testing.expectEqual(@as(i64, if (id == 123) -1 else if (id == 124 or id == 125) std.math.minInt(i64) else 0), o.lvalue(&out[0]));
    }
    o.setlvalue(&b[0], 1);
    try std.testing.expectEqual(@as(?c_int, 1), run(&L, 98, &out, &a, &b, 2, 1));
    try std.testing.expectEqual(std.math.maxInt(i64), o.lvalue(&out[0]));
    try std.testing.expectEqual(@as(?c_int, 1), run(&L, 96, &out, &a, null, 1, 1));
    try std.testing.expectEqual(std.math.minInt(i64), o.lvalue(&out[0]));
}

test "assert truth, false and nil fallback" {
    var L: o.State = undefined;
    var a = o.TValue{};
    var out = [_]o.TValue{.{}};
    o.setlvalue(&out[0], 123);
    o.setbvalue(&a, true);
    try std.testing.expectEqual(@as(?c_int, 0), run(&L, 1, &out, &a, null, 1, 0));
    o.setbvalue(&a, false);
    try std.testing.expectEqual(@as(?c_int, null), run(&L, 1, &out, &a, null, 1, 0));
    try std.testing.expectEqual(@as(i64, 123), o.lvalue(&out[0]));
    o.setnilvalue(&a);
    try std.testing.expectEqual(@as(?c_int, null), run(&L, 1, &out, &a, null, 1, 0));
    try std.testing.expectEqual(@as(i64, 123), o.lvalue(&out[0]));
}

test "vector exceptional components and mixed arguments" {
    var L: o.State = undefined;
    var a = o.TValue{};
    var args = [_]o.TValue{ .{}, .{} };
    var out = [_]o.TValue{.{}};
    o.setvvalue(&a, -0.0, std.math.nan(f32), -2);
    try std.testing.expectEqual(@as(?c_int, 1), run(&L, 84, &out, &a, null, 1, 1));
    const v = o.vvalue(&out[0]);
    try std.testing.expectEqual(@as(u32, 0), @as(u32, @bitCast(v[0])));
    try std.testing.expect(std.math.isNan(v[1]));
    try std.testing.expectEqual(@as(f32, 2), v[2]);
    o.setvvalue(&args[0], 1, 2, 3);
    o.setnilvalue(&args[1]);
    for ([_]u8{ 87, 88 }) |id| {
        const before = out[0];
        try std.testing.expectEqual(@as(?c_int, null), run(&L, id, &out, &a, &args, 3, 1));
        try std.testing.expectEqualSlices(u8, std.mem.asBytes(&before), std.mem.asBytes(&out[0]));
    }
}

test "type and typeof use basic names and reject excess results" {
    var L: o.State = undefined;
    var global: o.Global = undefined;
    var names: [o.T_COUNT]o.TString = undefined;
    var ud: o.Udata = undefined;
    ud.tag = 0;
    ud.metatable = null;
    global.mt = @splat(null);
    global.lightuserdataname = @splat(null);
    for (&global.ttname, 0..) |*name, i| name.* = &names[i];
    L.global = &global;
    for (0..o.T_COUNT) |i| {
        var a = o.TValue{};
        a.tt = @intCast(i);
        if (i == o.T_USERDATA) a.value.gc = @ptrCast(&ud);
        var out = [_]o.TValue{.{}};
        for ([_]u8{ 40, 44 }) |id| {
            try std.testing.expectEqual(@as(?c_int, 1), run(&L, id, &out, &a, null, 1, 1));
            try std.testing.expectEqual(&names[i], o.tsvalue(&out[0]));
            const before = out[0];
            try std.testing.expectEqual(@as(?c_int, null), run(&L, id, &out, &a, null, 1, 2));
            try std.testing.expectEqualSlices(u8, std.mem.asBytes(&before), std.mem.asBytes(&out[0]));
        }
    }
}

test "select vararg count, index and fallback" {
    var L: o.State = undefined;
    var frame: o.CallInfo = undefined;
    var proto: o.Proto = undefined;
    var stack: [5]o.TValue = @splat(.{});
    var out = [_]o.TValue{.{}};
    var a = o.TValue{};
    var hash: extern struct { header: o.TString, data: [2]u8 } = undefined;
    hash.header.len = 1;
    hash.data = .{ '#', 0 };
    proto.numparams = 0;
    frame.p = &proto;
    frame.func = stack[1..].ptr;
    L.ci = &frame;
    L.base = stack[4..].ptr;
    o.setnvalue(&stack[2], 17);
    o.setnvalue(&stack[3], 23);
    o.setsvalue(&a, &hash.header);
    try std.testing.expectEqual(@as(?c_int, 1), run(&L, 57, &out, &a, null, 1, 1));
    try std.testing.expectEqual(@as(f64, 2), o.nvalue(&out[0]));
    o.setnvalue(&a, 2);
    try std.testing.expectEqual(@as(?c_int, 1), run(&L, 57, &out, &a, null, 1, 1));
    try std.testing.expectEqual(@as(f64, 23), o.nvalue(&out[0]));
    for ([_]f64{ -1, 0, 3 }) |index| {
        o.setnvalue(&a, index);
        const before = out[0];
        try std.testing.expectEqual(@as(?c_int, null), run(&L, 57, &out, &a, null, 1, 1));
        try std.testing.expectEqualSlices(u8, std.mem.asBytes(&before), std.mem.asBytes(&out[0]));
    }
}

test "typeof userdata and table metatables" {
    var L: o.State = undefined;
    var global: o.Global = undefined;
    var type_key: o.TString = undefined;
    var custom: o.TString = undefined;
    var ordinary: o.TString = undefined;
    var node = o.LuaNode{};
    var mt: o.Table = undefined;
    var table_value: o.Table = undefined;
    var userdata: o.Udata = undefined;
    var a = o.TValue{};
    var out = [_]o.TValue{.{}};
    type_key.hash = 0;
    global.mt = @splat(null);
    global.ttname = @splat(&ordinary);
    global.tmname[@intFromEnum(o.TMS.TYPE)] = &type_key;
    L.global = &global;
    node.key.tt = o.T_STRING;
    node.key.value.gc = @ptrCast(&type_key);
    o.setsvalue(&node.val, &custom);
    mt.lsizenode = 0;
    mt.node = @ptrCast(&node);
    userdata.tag = 0;
    userdata.metatable = &mt;
    o.setuvalue(&a, &userdata);
    try std.testing.expectEqual(@as(?c_int, 1), run(&L, 44, &out, &a, null, 1, 1));
    try std.testing.expectEqual(&custom, o.tsvalue(&out[0]));
    try std.testing.expectEqual(@as(?c_int, 1), run(&L, 40, &out, &a, null, 1, 1));
    try std.testing.expectEqual(&ordinary, o.tsvalue(&out[0]));
    table_value.metatable = &mt;
    o.sethvalue(&a, &table_value);
    try std.testing.expectEqual(@as(?c_int, 1), run(&L, 44, &out, &a, null, 1, 1));
    try std.testing.expectEqual(&ordinary, o.tsvalue(&out[0]));
    global.mt[o.T_TABLE] = &mt;
    try std.testing.expectEqual(@as(?c_int, 1), run(&L, 44, &out, &a, null, 1, 1));
    try std.testing.expectEqual(&custom, o.tsvalue(&out[0]));
}
