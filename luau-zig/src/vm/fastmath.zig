//! Numeric FASTCALL implementations derived from Luau's lbuiltins.cpp (MIT).
const std = @import("std");
const o = @import("object.zig");
const luau_math = @import("../luau_math.zig");

// Match the math library's Zig implementations, not the host C library.
const c = struct {
    fn acos(x: f64) f64 { return std.math.acos(x); }
    fn asin(x: f64) f64 { return std.math.asin(x); }
    fn atan(x: f64) f64 { return std.math.atan(x); }
    fn atan2(x: f64, y: f64) f64 { return std.math.atan2(x, y); }
    fn cosh(x: f64) f64 { return std.math.cosh(x); }
    fn cos(x: f64) f64 { return @cos(x); }
    fn exp(x: f64) f64 { return luau_math.exp(x); }
    fn fmod(x: f64, y: f64) f64 { return @rem(x, y); }
    fn frexp(x: f64, exponent: *c_int) f64 {
        if (x == 0 or std.math.isNan(x) or std.math.isInf(x)) {
            exponent.* = 0;
            return luau_math.quietNan(x);
        }
        const value = std.math.frexp(x);
        exponent.* = value.exponent;
        return value.significand;
    }
    fn ldexp(x: f64, exponent: c_int) f64 {
        return if (std.math.isNan(x)) luau_math.quietNan(x) else if (x == 0) x else std.math.ldexp(x, exponent);
    }
    fn log10(x: f64) f64 { return luau_math.log10(x); }
    fn log2(x: f64) f64 { return @log2(x); }
    fn log(x: f64) f64 { return @log(x); }
    fn modf(x: f64, integer: *f64) f64 {
        integer.* = @trunc(x);
        return if (std.math.isInf(x)) std.math.copysign(@as(f64, 0), x) else std.math.copysign(x - integer.*, x);
    }
    fn pow(x: f64, y: f64) f64 { return luau_math.pow(x, y); }
    fn sinh(x: f64) f64 { return luau_math.sinh(x); }
    fn sin(x: f64) f64 { return @sin(x); }
    fn tanh(x: f64) f64 { return luau_math.tanh(x); }
    fn tan(x: f64) f64 { return @tan(x); }
};

pub fn run(L: *o.State, id: u8, result: o.StkId, arg0: *o.TValue, args: ?o.StkId, nparams: c_int, nresults: c_int) ?c_int {
    _ = L;
    if (nparams < 1 or !o.ttisnumber(arg0)) return null;
    const x = o.nvalue(arg0);
    if (id == 14 or id == 20) {
        if (nresults > 2) return null;
        if (id == 14) {
            var exponent: c_int = 0;
            const fraction = c.frexp(x, &exponent);
            o.setnvalue(&result[0], fraction);
            o.setnvalue(&result[1], @floatFromInt(exponent));
        } else {
            var integer: f64 = undefined;
            const fraction = c.modf(x, &integer);
            o.setnvalue(&result[0], integer);
            o.setnvalue(&result[1], fraction);
        }
        return 2;
    }
    if (nresults > 1) return null;
    switch (id) {
        3, 4, 9, 16, 17, 21, 24, 25, 27 => {
            const uses_second = id == 17 or id == 21;
            const valid_second = nparams > 1 and args != null and o.ttisnumber(&args.?[0]);
            if ((!uses_second or (id == 17 and nparams == 1) or valid_second) and
                (id != 21 or valid_second))
            {
                const y = if (uses_second and valid_second) o.nvalue(&args.?[0]) else 0;
                const domain_id = if (id == 17 and y == 10) 16 else id;
                if (luau_math.domainError(domain_id, x, y)) |value| {
                    o.setnvalue(&result[0], value);
                    return 1;
                }
            }
        },
        else => {},
    }
    switch (id) {
        2 => o.setnvalue(&result[0], @abs(x)),
        3 => o.setnvalue(&result[0], c.acos(x)),
        4 => o.setnvalue(&result[0], c.asin(x)),
        6 => o.setnvalue(&result[0], c.atan(x)),
        7 => o.setnvalue(&result[0], @ceil(x)),
        8 => o.setnvalue(&result[0], c.cosh(x)),
        9 => o.setnvalue(&result[0], c.cos(x)),
        11 => o.setnvalue(&result[0], c.exp(x)),
        16 => o.setnvalue(&result[0], c.log10(x)),
        23 => o.setnvalue(&result[0], c.sinh(x)),
        24 => o.setnvalue(&result[0], c.sin(x)),
        26 => o.setnvalue(&result[0], c.tanh(x)),
        27 => o.setnvalue(&result[0], c.tan(x)),
        5, 13, 15, 21 => {
            if (nparams < 2) return null;
            const a = args.?;
            if (!o.ttisnumber(&a[0])) return null;
            const y = o.nvalue(&a[0]);
            const r = switch (id) {
                5 => c.atan2(x, y),
                13 => c.fmod(x, y),
                15 => c.ldexp(x, o.num2int(y)),
                21 => c.pow(x, y),
                else => unreachable,
            };
            o.setnvalue(&result[0], r);
        },
        17 => {
            var r = c.log(x);
            if (nparams > 1) {
                const a = args.?;
                if (!o.ttisnumber(&a[0])) return null;
                const base = o.nvalue(&a[0]);
                r = if (base == 2) c.log2(x) else if (base == 10) c.log10(x) else r / c.log(base);
            }
            o.setnvalue(&result[0], r);
        },
        10 => o.setnvalue(&result[0], x / (3.14159265358979323846 / 180.0)),
        12 => o.setnvalue(&result[0], @floor(x)),
        22 => o.setnvalue(&result[0], x * (3.14159265358979323846 / 180.0)),
        25 => o.setnvalue(&result[0], @sqrt(x)),
        18, 19 => {
            if (nparams < 2) return null;
            const a = args.?;
            var r = x;
            var i: usize = 0;
            while (i < @as(usize, @intCast(nparams - 1))) : (i += 1) {
                if (!o.ttisnumber(&a[i])) return null;
                const v = o.nvalue(&a[i]);
                if (if (id == 18) v > r else v < r) r = v;
            }
            o.setnvalue(&result[0], r);
        },
        28, 29, 31, 32, 33, 34, 35, 36, 37, 38, 39, 59 => {
            if (nparams < 2) return null;
            const a = args.?;
            if (id != 59 and !o.ttisnumber(&a[0])) return null;
            const y = o.nvalue(&a[0]);
            const n = o.num2unsigned(x);
            var r: u32 = 0;
            switch (id) {
                29, 31, 32, 33 => {
                    r = n;
                    var i: usize = 0;
                    while (i < @as(usize, @intCast(nparams - 1))) : (i += 1) {
                        if (!o.ttisnumber(&a[i])) return null;
                        const v = o.num2unsigned(o.nvalue(&a[i]));
                        r = switch (id) { 29, 33 => r & v, 31 => r | v, 32 => r ^ v, else => unreachable };
                    }
                    if (id == 33) {
                        o.setbvalue(&result[0], r != 0);
                        return 1;
                    }
                },
                28, 35, 36, 38, 39 => {
                    const s = o.num2int(y);
                    if (id == 35 or id == 38) {
                        const shift: u5 = @truncate(@as(u32, @bitCast(s)));
                        r = if (id == 35) std.math.rotl(u32, n, shift) else std.math.rotr(u32, n, shift);
                    } else {
                        if (s < 0 or s >= 32) return null;
                        const shift: u5 = @intCast(s);
                        r = switch (id) {
                            28 => @bitCast(@as(i32, @bitCast(n)) >> shift),
                            36 => n << shift,
                            39 => n >> shift,
                            else => unreachable,
                        };
                    }
                },
                34, 37, 59 => {
                    var f: c_int = undefined;
                    var w: c_int = 1;
                    var value: u32 = 0;
                    if (id == 59) {
                        const fw = o.num2int(y);
                        f = fw & 31;
                        w = (fw >> 5) + 1;
                    } else if (id == 34) {
                        f = o.num2int(y);
                        if (nparams >= 3) {
                            if (!o.ttisnumber(&a[1])) return null;
                            w = o.num2int(o.nvalue(&a[1]));
                        }
                    } else {
                        if (nparams < 3 or !o.ttisnumber(&a[1])) return null;
                        value = o.num2unsigned(y);
                        f = o.num2int(o.nvalue(&a[1]));
                        if (nparams >= 4) {
                            if (!o.ttisnumber(&a[2])) return null;
                            w = o.num2int(o.nvalue(&a[2]));
                        }
                    }
                    if (f < 0 or w <= 0 or f +% w > 32) return null;
                    const shift: u5 = @truncate(@as(u32, @bitCast(f)));
                    const width: u5 = @truncate(@as(u32, @bitCast(w -% 1)));
                    const mask: u32 = ~(@as(u32, 0xfffffffe) << width);
                    r = if (id == 37) (n & ~(mask << shift)) | ((value & mask) << shift) else (n >> shift) & mask;
                },
                else => unreachable,
            }
            o.setnvalue(&result[0], @floatFromInt(r));
        },
        30, 55, 56, 64 => {
            const n = o.num2unsigned(x);
            const r: u32 = switch (id) {
                30 => ~n,
                55 => @clz(n),
                56 => @ctz(n),
                64 => @byteSwap(n),
                else => unreachable,
            };
            o.setnvalue(&result[0], @floatFromInt(r));
        },
        47 => o.setnvalue(&result[0], if (x > 0) 1 else if (x < 0) -1 else 0),
        48 => o.setnvalue(&result[0], @round(x)),
        91 => o.setbvalue(&result[0], std.math.isNan(x)),
        92 => o.setbvalue(&result[0], std.math.isInf(x)),
        93 => o.setbvalue(&result[0], std.math.isFinite(x)),
        46, 89 => {
            if (nparams < 3) return null;
            const a = args.?;
            if (!o.ttisnumber(&a[0]) or !o.ttisnumber(&a[1])) return null;
            const y = o.nvalue(&a[0]);
            const z = o.nvalue(&a[1]);
            const r = if (id == 46) blk: {
                if (!(y <= z)) return null;
                const v = if (x < y) y else x;
                break :blk if (v > z) z else v;
            } else if (z == 1) y else x + (y - x) * z;
            o.setnvalue(&result[0], r);
        },
        else => return null,
    }
    switch (id) {
        3, 4, 5, 6, 8, 9, 11, 16, 17, 21, 23, 24, 25, 26, 27 => {
            const y = if ((id == 5 or id == 17 or id == 21) and nparams > 1 and o.ttisnumber(&args.?[0])) o.nvalue(&args.?[0]) else 0;
            o.setnvalue(&result[0], luau_math.transcendentalResult(if (id == 17 and y == 10) 16 else id, x, y, o.nvalue(&result[0])));
        },
        else => {},
    }
    return 1;
}

test "numeric fastcalls and fallback preserve arguments" {
    var state: o.State = undefined;
    var inputs = [_]o.TValue{ .{}, .{}, .{} };
    var output = [_]o.TValue{ .{}, .{} };
    o.setnvalue(&inputs[0], -3.5);
    try std.testing.expectEqual(@as(?c_int, 1), run(&state, 2, &output, &inputs[0], null, 1, 1));
    try std.testing.expectEqual(@as(f64, 3.5), o.nvalue(&output[0]));
    o.setnvalue(&inputs[0], 0x80000000);
    o.setnvalue(&inputs[1], 1);
    try std.testing.expectEqual(@as(?c_int, 1), run(&state, 28, &output, &inputs[0], inputs[1..].ptr, 2, 1));
    try std.testing.expectEqual(@as(f64, 0xc0000000), o.nvalue(&output[0]));
    o.setnvalue(&inputs[1], 32);
    try std.testing.expectEqual(@as(?c_int, null), run(&state, 28, &output, &inputs[0], inputs[1..].ptr, 2, 1));
    try std.testing.expectEqual(@as(f64, 0xc0000000), o.nvalue(&output[0]));
    o.setnvalue(&inputs[0], -0.5);
    try std.testing.expectEqual(@as(?c_int, 1), run(&state, 48, &output, &inputs[0], null, 1, -1));
    try std.testing.expectEqual(@as(f64, -1), o.nvalue(&output[0]));
}

test "every numeric builtin succeeds and guard failure is inert" {
    const ids = [_]u8{ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 46, 47, 48, 55, 56, 59, 64, 89, 91, 92, 93 };
    var state: o.State = undefined;
    for (ids) |id| {
        var input = [_]o.TValue{ .{}, .{}, .{} };
        var output = [_]o.TValue{ .{}, .{} };
        o.setnvalue(&input[0], if (id == 3 or id == 4) 0.5 else 4);
        o.setnvalue(&input[1], 2);
        o.setnvalue(&input[2], 3);
        const count = run(&state, id, &output, &input[0], input[1..].ptr, 3, -1);
        try std.testing.expectEqual(@as(?c_int, if (id == 14 or id == 20) 2 else 1), count);
        if (id == 33 or id >= 91) {
            const expected = id == 93;
            try std.testing.expectEqual(@as(c_int, @intFromBool(expected)), output[0].value.b);
        } else {
            const expected: f64 = switch (id) {
                2, 7, 12, 14, 18, 20, 37, 47, 48 => switch (id) { 14 => 0.5, 47 => 1, else => 4 },
                3 => c.acos(0.5), 4 => c.asin(0.5), 5 => c.atan2(4, 2), 6 => c.atan(4),
                8 => c.cosh(4), 9 => c.cos(4), 10 => 4.0 / (3.14159265358979323846 / 180.0),
                11 => c.exp(4), 13 => 0, 15 => 16, 16 => c.log10(4), 17 => c.log2(4),
                19 => 2, 21 => 16, 22 => 4 * (3.14159265358979323846 / 180.0),
                23 => c.sinh(4), 24 => c.sin(4), 25 => 2, 26 => c.tanh(4), 27 => c.tan(4),
                28, 38, 39 => 1, 29 => 0, 30 => 4294967291, 31 => 7, 32 => 5,
                34, 59 => 1, 35, 36 => 16, 46 => 3, 55 => 29, 56 => 2,
                64 => 67108864, 89 => -2, else => unreachable,
            };
            try std.testing.expectEqual(@as(u64, @bitCast(expected)), @as(u64, @bitCast(o.nvalue(&output[0]))));
        }
        if (id == 14 or id == 20) {
            const expected_second: f64 = if (id == 14) 3 else 0;
            try std.testing.expectEqual(@as(u64, @bitCast(expected_second)), @as(u64, @bitCast(o.nvalue(&output[1]))));
        }
        const before_output = output;
        const before_input = input;
        try std.testing.expectEqual(@as(?c_int, null), run(&state, id, &output, &input[0], null, 0, 1));
        try std.testing.expectEqualSlices(u8, std.mem.asBytes(&before_output), std.mem.asBytes(&output));
        try std.testing.expectEqualSlices(u8, std.mem.asBytes(&before_input), std.mem.asBytes(&input));
        try std.testing.expectEqual(@as(?c_int, null), run(&state, id, &output, &input[0], input[1..].ptr, 3, 3));
        try std.testing.expectEqualSlices(u8, std.mem.asBytes(&before_output), std.mem.asBytes(&output));
        try std.testing.expectEqualSlices(u8, std.mem.asBytes(&before_input), std.mem.asBytes(&input));
    }
}

test "numeric edge bits" {
    var state: o.State = undefined;
    var input = [_]o.TValue{ .{}, .{}, .{} };
    var output = [_]o.TValue{ .{}, .{} };
    const negative_zero: f64 = @bitCast(@as(u64, 0x8000000000000000));
    o.setnvalue(&input[0], negative_zero);
    try std.testing.expectEqual(@as(?c_int, 1), run(&state, 48, &output, &input[0], null, 1, 1));
    try std.testing.expectEqual(@as(u64, 0x8000000000000000), @as(u64, @bitCast(o.nvalue(&output[0]))));
    o.setnvalue(&input[0], 0.5);
    _ = run(&state, 48, &output, &input[0], null, 1, 1);
    try std.testing.expectEqual(@as(f64, 1), o.nvalue(&output[0]));
    o.setnvalue(&input[0], 0.49999999999999994);
    _ = run(&state, 48, &output, &input[0], null, 1, 1);
    try std.testing.expectEqual(@as(f64, 0), o.nvalue(&output[0]));
    o.setnvalue(&input[0], -1);
    _ = run(&state, 30, &output, &input[0], null, 1, 1);
    try std.testing.expectEqual(@as(f64, 0), o.nvalue(&output[0]));
    o.setnvalue(&input[0], std.math.inf(f64));
    _ = run(&state, 55, &output, &input[0], null, 1, 1);
    try std.testing.expectEqual(@as(f64, 32), o.nvalue(&output[0]));
    const nan: f64 = @bitCast(@as(u64, 0xfff8000000001234));
    o.setnvalue(&input[0], nan);
    _ = run(&state, 2, &output, &input[0], null, 1, 1);
    try std.testing.expectEqual(@as(u64, 0x7ff8000000001234), @as(u64, @bitCast(o.nvalue(&output[0]))));
}
