//! Builtin call constant folding and builtin signature info.
//!
//! Translated from third_party/luau/Compiler/src/BuiltinFolding.cpp and
//! getBuiltinInfo in Builtins.cpp (Luau, MIT License; see
//! third_party/luau/LICENSE.txt). Upstream calls the host C math library; this
//! port uses Zig's std.math / float builtins (no C), so results of
//! transcendental functions may differ from a particular libm in the last ulp.
//! Double -> integer conversions follow the x86-64 truncation semantics that
//! upstream relies on (out-of-range values produce the "integer indefinite").

const std = @import("std");
const bc = @import("bytecode.zig");
const lex = @import("lexer.zig");
const an = @import("analysis.zig");
const luau_math = @import("../luau_math.zig");
const Constant = an.Constant;
const B = bc.Builtin;

const kPi: f64 = 3.14159265358979323846;
const kRadDeg: f64 = kPi / 180.0;
const kE: f64 = 2.71828182845904523536;
const kPhi: f64 = 1.61803398874989484820;
const kSqrt2: f64 = 1.41421356237309504880;
const kTau: f64 = 6.28318530717958647692;

const kStringCharFoldLimit = 128;

/// C++ int(double) as compiled for x86-64 (cvttsd2si): out of range / NaN yields INT_MIN.
pub fn cvtInt(d: f64) i32 {
    if (d > -2147483649.0 and d < 2147483648.0) return @intFromFloat(@trunc(d));
    return std.math.minInt(i32);
}

/// C++ int64_t(double) as compiled for x86-64: out of range / NaN yields INT64_MIN.
pub fn cvtInt64(d: f64) i64 {
    if (d >= -9223372036854775808.0 and d < 9223372036854775808.0) return @intFromFloat(@trunc(d));
    return std.math.minInt(i64);
}

fn bit32(v: f64) u32 {
    // convert through signed 64-bit integer to match runtime behavior and gracefully truncate negative integers
    return @truncate(@as(u64, @bitCast(cvtInt64(v))));
}

fn cnum(v: f64) Constant {
    return .{ .number = v };
}

fn ctype(c: Constant) Constant {
    return switch (c) {
        .nil => .{ .string = "nil" },
        .boolean => .{ .string = "boolean" },
        .number => .{ .string = "number" },
        .integer => .{ .string = "integer" },
        .vectorf, .vectord => .{ .string = "vector" },
        .string => .{ .string = "string" },
        else => .unknown,
    };
}

fn ctypeof(c: Constant) Constant {
    return switch (c) {
        .nil => .{ .string = "nil" },
        .boolean => .{ .string = "boolean" },
        .number => .{ .string = "number" },
        .integer => .{ .string = "integer" },
        .vectorf, .vectord => .unknown, // vector can have a custom typeof name at runtime
        .string => .{ .string = "string" },
        else => .unknown,
    };
}

fn allNumbers(args: []const Constant) bool {
    for (args) |a| if (a != .number) return false;
    return true;
}

fn unaryMath(args: []const Constant, comptime f: fn (f64) f64) Constant {
    if (args.len == 1 and args[0] == .number) return cnum(f(args[0].number));
    return .unknown;
}

fn mAbs(x: f64) f64 {
    return @abs(x);
}
fn mAcos(x: f64) f64 {
    return luau_math.domainError(3, x, 0) orelse luau_math.transcendentalResult(3, x, 0, std.math.acos(x));
}
fn mAsin(x: f64) f64 {
    return luau_math.domainError(4, x, 0) orelse luau_math.transcendentalResult(4, x, 0, std.math.asin(x));
}
fn mAtan(x: f64) f64 {
    return luau_math.transcendentalResult(6, x, 0, std.math.atan(x));
}
fn mCeil(x: f64) f64 {
    return @ceil(x);
}
fn mCosh(x: f64) f64 {
    return luau_math.transcendentalResult(8, x, 0, std.math.cosh(x));
}
fn mCos(x: f64) f64 {
    return luau_math.domainError(9, x, 0) orelse luau_math.transcendentalResult(9, x, 0, @cos(x));
}
fn mDeg(x: f64) f64 {
    return x / kRadDeg;
}
fn mExp(x: f64) f64 {
    // must equal runtime math.exp (upstream folds with the same libm exp the VM calls)
    return luau_math.transcendentalResult(11, x, 0, luau_math.exp(x));
}
fn mFloor(x: f64) f64 {
    return @floor(x);
}
fn mLog10(x: f64) f64 {
    return luau_math.domainError(16, x, 0) orelse luau_math.transcendentalResult(16, x, 0, luau_math.log10(x));
}
fn mRad(x: f64) f64 {
    return x * kRadDeg;
}
fn mSinh(x: f64) f64 {
    return luau_math.transcendentalResult(23, x, 0, luau_math.sinh(x));
}
fn mSin(x: f64) f64 {
    return luau_math.domainError(24, x, 0) orelse luau_math.transcendentalResult(24, x, 0, @sin(x));
}
fn mSqrt(x: f64) f64 {
    return luau_math.domainError(25, x, 0) orelse luau_math.transcendentalResult(25, x, 0, @sqrt(x));
}
fn mTanh(x: f64) f64 {
    return luau_math.transcendentalResult(26, x, 0, luau_math.tanh(x));
}
fn mTan(x: f64) f64 {
    return luau_math.domainError(27, x, 0) orelse luau_math.transcendentalResult(27, x, 0, @tan(x));
}
fn mRound(x: f64) f64 {
    // C round(): half-way cases away from zero
    return @round(x);
}

pub fn foldBuiltin(names: *lex.NameTable, bfid: B, args: []const Constant, vector_double_precision: bool) an.Error!Constant {
    const count = args.len;
    switch (bfid) {
        .MATH_ABS => return unaryMath(args, mAbs),
        .MATH_ACOS => return unaryMath(args, mAcos),
        .MATH_ASIN => return unaryMath(args, mAsin),
        .MATH_ATAN2 => if (count == 2 and allNumbers(args)) return cnum(luau_math.transcendentalResult(5, args[0].number, args[1].number, std.math.atan2(args[0].number, args[1].number))),
        .MATH_ATAN => return unaryMath(args, mAtan),
        .MATH_CEIL => return unaryMath(args, mCeil),
        .MATH_COSH => return unaryMath(args, mCosh),
        .MATH_COS => return unaryMath(args, mCos),
        .MATH_DEG => return unaryMath(args, mDeg),
        .MATH_EXP => return unaryMath(args, mExp),
        .MATH_FLOOR => return unaryMath(args, mFloor),
        // C fmod: remainder with the sign of the dividend
        .MATH_FMOD => if (count == 2 and allNumbers(args)) return cnum(@rem(args[0].number, args[1].number)),
        // Note: FREXP isn't folded since it returns multiple values
        .MATH_LDEXP => if (count == 2 and allNumbers(args)) {
            // std.math.ldexp can overflow zero to infinity; C ldexp preserves its sign.
            const x = args[0].number;
            return cnum(if (std.math.isNan(x)) luau_math.quietNan(x) else if (x == 0) x else std.math.ldexp(x, cvtInt(args[1].number)));
        },
        .MATH_LOG10 => return unaryMath(args, mLog10),
        .MATH_LOG => {
            if (count == 1 and args[0] == .number) return cnum(luau_math.domainError(17, args[0].number, 0) orelse luau_math.transcendentalResult(17, args[0].number, 0, @log(args[0].number)));
            if (count == 2 and allNumbers(args)) {
                if (args[1].number == 2.0) return cnum(luau_math.domainError(17, args[0].number, args[1].number) orelse luau_math.transcendentalResult(17, args[0].number, args[1].number, @log2(args[0].number)));
                if (args[1].number == 10.0) return cnum(luau_math.domainError(16, args[0].number, args[1].number) orelse luau_math.transcendentalResult(16, args[0].number, args[1].number, luau_math.log10(args[0].number)));
                return cnum(luau_math.domainError(17, args[0].number, args[1].number) orelse luau_math.transcendentalResult(17, args[0].number, args[1].number, @log(args[0].number) / @log(args[1].number)));
            }
        },
        .MATH_MAX, .MATH_MIN => if (count >= 1 and args[0] == .number) {
            var r = args[0].number;
            for (args[1..]) |a| {
                if (a != .number) return .unknown;
                if (bfid == .MATH_MAX) {
                    r = if (a.number > r) a.number else r;
                } else {
                    r = if (a.number < r) a.number else r;
                }
            }
            return cnum(r);
        },
        // Note: MODF isn't folded since it returns multiple values
        .MATH_POW => if (count == 2 and allNumbers(args)) return cnum(luau_math.domainError(21, args[0].number, args[1].number) orelse luau_math.transcendentalResult(21, args[0].number, args[1].number, luau_math.pow(args[0].number, args[1].number))),
        .MATH_RAD => return unaryMath(args, mRad),
        .MATH_SINH => return unaryMath(args, mSinh),
        .MATH_SIN => return unaryMath(args, mSin),
        .MATH_SQRT => return unaryMath(args, mSqrt),
        .MATH_TANH => return unaryMath(args, mTanh),
        .MATH_TAN => return unaryMath(args, mTan),
        .BIT32_ARSHIFT => if (count == 2 and allNumbers(args)) {
            const u = bit32(args[0].number);
            const s = cvtInt(args[1].number);
            if (@as(u32, @bitCast(s)) < 32) return cnum(@floatFromInt(@as(u32, @bitCast(@as(i32, @bitCast(u)) >> @intCast(s)))));
        },
        .BIT32_BAND, .BIT32_BOR, .BIT32_BXOR, .BIT32_BTEST => if (count >= 1 and args[0] == .number) {
            var r = bit32(args[0].number);
            for (args[1..]) |a| {
                if (a != .number) return .unknown;
                const v = bit32(a.number);
                switch (bfid) {
                    .BIT32_BOR => r |= v,
                    .BIT32_BXOR => r ^= v,
                    else => r &= v,
                }
            }
            if (bfid == .BIT32_BTEST) return .{ .boolean = r != 0 };
            return cnum(@floatFromInt(r));
        },
        .BIT32_BNOT => if (count == 1 and args[0] == .number) return cnum(@floatFromInt(~bit32(args[0].number))),
        .BIT32_EXTRACT => if (count >= 2 and args[0] == .number and args[1] == .number and (count == 2 or args[2] == .number)) {
            const u = bit32(args[0].number);
            const f = cvtInt(args[1].number);
            const w: i32 = if (count == 2) 1 else cvtInt(args[2].number);
            if (f >= 0 and w > 0 and @as(i64, f) + w <= 32) {
                const m: u32 = ~(@as(u32, 0xfffffffe) << @intCast(w - 1));
                return cnum(@floatFromInt((u >> @intCast(f)) & m));
            }
        },
        .BIT32_LROTATE => if (count == 2 and allNumbers(args)) {
            const u = bit32(args[0].number);
            const s = cvtInt(args[1].number);
            return cnum(@floatFromInt((u << @intCast(s & 31)) | (u >> @intCast((32 -% s) & 31))));
        },
        .BIT32_LSHIFT => if (count == 2 and allNumbers(args)) {
            const u = bit32(args[0].number);
            const s = cvtInt(args[1].number);
            if (@as(u32, @bitCast(s)) < 32) return cnum(@floatFromInt(u << @intCast(s)));
        },
        .BIT32_REPLACE => if (count >= 3 and args[0] == .number and args[1] == .number and args[2] == .number and (count == 3 or args[3] == .number)) {
            const n = bit32(args[0].number);
            const v = bit32(args[1].number);
            const f = cvtInt(args[2].number);
            const w: i32 = if (count == 3) 1 else cvtInt(args[3].number);
            if (f >= 0 and w > 0 and @as(i64, f) + w <= 32) {
                const m: u32 = ~(@as(u32, 0xfffffffe) << @intCast(w - 1));
                return cnum(@floatFromInt((n & ~(m << @intCast(f))) | ((v & m) << @intCast(f))));
            }
        },
        .BIT32_RROTATE => if (count == 2 and allNumbers(args)) {
            const u = bit32(args[0].number);
            const s = cvtInt(args[1].number);
            return cnum(@floatFromInt((u >> @intCast(s & 31)) | (u << @intCast((32 -% s) & 31))));
        },
        .BIT32_RSHIFT => if (count == 2 and allNumbers(args)) {
            const u = bit32(args[0].number);
            const s = cvtInt(args[1].number);
            if (@as(u32, @bitCast(s)) < 32) return cnum(@floatFromInt(u >> @intCast(s)));
        },
        .TYPE => if (count == 1 and args[0] != .unknown) return ctype(args[0]),
        .STRING_BYTE => {
            if (count == 1 and args[0] == .string) {
                if (args[0].string.len > 0) return cnum(@floatFromInt(args[0].string[0]));
            } else if (count == 2 and args[0] == .string and args[1] == .number) {
                const i = cvtInt(args[1].number);
                if (i > 0 and @as(u32, @intCast(i)) <= args[0].string.len) return cnum(@floatFromInt(args[0].string[@intCast(i - 1)]));
            }
        },
        .STRING_CHAR => if (count < kStringCharFoldLimit) {
            var buf: [kStringCharFoldLimit]u8 = undefined;
            for (args, 0..) |a, i| {
                if (a != .number) return .unknown;
                const ch = cvtInt(a.number);
                if (ch < 0 or ch > 255) return .unknown;
                buf[i] = @intCast(ch);
            }
            if (count == 0) return .{ .string = "" };
            return .{ .string = try names.getOrAdd(buf[0..count]) };
        },
        .STRING_LEN => if (count == 1 and args[0] == .string) return cnum(@floatFromInt(args[0].string.len)),
        .TYPEOF => if (count == 1 and args[0] != .unknown) return ctypeof(args[0]),
        .STRING_SUB => if (count >= 2 and args[0] == .string and args[1] == .number) {
            if (count >= 3 and args[2] != .number) return .unknown;

            const str = args[0].string;
            const len: i32 = @intCast(str.len);

            var start: i32 = cvtInt(args[1].number);
            var end: i32 = if (count >= 3) cvtInt(args[2].number) else len;

            // Relative string position: negative means back from end
            if (start < 0) start +%= len + 1;
            if (end < 0) end +%= len + 1;

            // If end is before the start of the string, substring is empty
            if (end < 1) return .{ .string = "" };

            // Start clamped to the start of the string, end is clamped to the end
            start = if (start < 1) 1 else start;
            end = if (end > len) len else end;

            if (start <= end) return .{ .string = try names.getOrAdd(str[@intCast(start - 1)..@intCast(end)]) };

            return .{ .string = "" };
        },
        .MATH_CLAMP => if (count == 3 and allNumbers(args)) {
            const min = args[1].number;
            const max = args[2].number;
            if (min <= max) {
                var v = args[0].number;
                v = if (v < min) min else v;
                v = if (v > max) max else v;
                return cnum(v);
            }
        },
        .MATH_SIGN => if (count == 1 and args[0] == .number) {
            const v = args[0].number;
            return cnum(if (v > 0.0) 1.0 else if (v < 0.0) -1.0 else 0.0);
        },
        .MATH_ROUND => return unaryMath(args, mRound),
        .VECTOR => if (count >= 2 and args[0] == .number and args[1] == .number) {
            const x = args[0].number;
            const y = args[1].number;
            if (vector_double_precision) {
                if (count == 2) return .{ .vectord = .{ x, y, 0.0, 0.0 } };
                if (count == 3 and args[2] == .number) return .{ .vectord = .{ x, y, args[2].number, 0.0 } };
                if (count == 4 and args[2] == .number and args[3] == .number) return .{ .vectord = .{ x, y, args[2].number, args[3].number } };
            } else {
                if (count == 2) return .{ .vectorf = .{ @floatCast(x), @floatCast(y), 0.0, 0.0 } };
                if (count == 3 and args[2] == .number) return .{ .vectorf = .{ @floatCast(x), @floatCast(y), @floatCast(args[2].number), 0.0 } };
                if (count == 4 and args[2] == .number and args[3] == .number)
                    return .{ .vectorf = .{ @floatCast(x), @floatCast(y), @floatCast(args[2].number), @floatCast(args[3].number) } };
            }
        },
        .MATH_LERP => if (count == 3 and allNumbers(args)) {
            const a = args[0].number;
            const b = args[1].number;
            const t = args[2].number;
            return cnum(if (t == 1.0) b else a + (b - a) * t);
        },
        .MATH_ISNAN => if (count == 1 and args[0] == .number) return .{ .boolean = std.math.isNan(args[0].number) },
        .MATH_ISINF => if (count == 1 and args[0] == .number) return .{ .boolean = std.math.isInf(args[0].number) },
        .MATH_ISFINITE => if (count == 1 and args[0] == .number) return .{ .boolean = std.math.isFinite(args[0].number) },
        else => {},
    }
    return .unknown;
}

pub fn foldBuiltinMath(index: []const u8) Constant {
    if (std.mem.eql(u8, index, "pi")) return cnum(kPi);
    if (std.mem.eql(u8, index, "huge")) return cnum(std.math.inf(f64));
    if (std.mem.eql(u8, index, "nan")) return cnum(std.math.nan(f64));
    if (std.mem.eql(u8, index, "e")) return cnum(kE);
    if (std.mem.eql(u8, index, "phi")) return cnum(kPhi);
    if (std.mem.eql(u8, index, "sqrt2")) return cnum(kSqrt2);
    if (std.mem.eql(u8, index, "tau")) return cnum(kTau);
    return .unknown;
}

pub const BuiltinInfo = struct {
    params: i32,
    results: i32,
    /// none-safe builtins are builtins that have the same behavior for arguments that are nil or none
    none_safe: bool = false,
};

pub fn getBuiltinInfo(bfid: B) BuiltinInfo {
    const ns = true;
    return switch (bfid) {
        .NONE => .{ .params = -1, .results = -1 },
        .ASSERT => .{ .params = -1, .results = -1 }, // assert() returns all values when first value is truthy
        .MATH_ABS, .MATH_ACOS, .MATH_ASIN => .{ .params = 1, .results = 1, .none_safe = ns },
        .MATH_ATAN2 => .{ .params = 2, .results = 1, .none_safe = ns },
        .MATH_ATAN, .MATH_CEIL, .MATH_COSH, .MATH_COS, .MATH_DEG, .MATH_EXP, .MATH_FLOOR => .{ .params = 1, .results = 1, .none_safe = ns },
        .MATH_FMOD => .{ .params = 2, .results = 1, .none_safe = ns },
        .MATH_FREXP => .{ .params = 1, .results = 2, .none_safe = ns },
        .MATH_LDEXP => .{ .params = 2, .results = 1, .none_safe = ns },
        .MATH_LOG10 => .{ .params = 1, .results = 1, .none_safe = ns },
        .MATH_LOG => .{ .params = -1, .results = 1 }, // 1 or 2 parameters
        .MATH_MAX, .MATH_MIN => .{ .params = -1, .results = 1 }, // variadic
        .MATH_MODF => .{ .params = 1, .results = 2, .none_safe = ns },
        .MATH_POW => .{ .params = 2, .results = 1, .none_safe = ns },
        .MATH_RAD, .MATH_SINH, .MATH_SIN, .MATH_SQRT, .MATH_TANH, .MATH_TAN => .{ .params = 1, .results = 1, .none_safe = ns },
        .BIT32_ARSHIFT => .{ .params = 2, .results = 1, .none_safe = ns },
        .BIT32_BAND => .{ .params = -1, .results = 1 }, // variadic
        .BIT32_BNOT => .{ .params = 1, .results = 1, .none_safe = ns },
        .BIT32_BOR, .BIT32_BXOR, .BIT32_BTEST => .{ .params = -1, .results = 1 }, // variadic
        .BIT32_EXTRACT => .{ .params = -1, .results = 1 }, // 2 or 3 parameters
        .BIT32_LROTATE, .BIT32_LSHIFT => .{ .params = 2, .results = 1, .none_safe = ns },
        .BIT32_REPLACE => .{ .params = -1, .results = 1 }, // 3 or 4 parameters
        .BIT32_RROTATE, .BIT32_RSHIFT => .{ .params = 2, .results = 1, .none_safe = ns },
        .TYPE => .{ .params = 1, .results = 1 },
        .STRING_BYTE => .{ .params = -1, .results = -1 }, // 1, 2 or 3 parameters
        .STRING_CHAR => .{ .params = -1, .results = 1 }, // variadic
        .STRING_LEN => .{ .params = 1, .results = 1, .none_safe = ns },
        .TYPEOF => .{ .params = 1, .results = 1 },
        .STRING_SUB => .{ .params = -1, .results = 1 }, // 2 or 3 parameters
        .MATH_CLAMP => .{ .params = 3, .results = 1, .none_safe = ns },
        .MATH_SIGN, .MATH_ROUND => .{ .params = 1, .results = 1, .none_safe = ns },
        .RAWSET => .{ .params = 3, .results = 1 },
        .RAWGET, .RAWEQUAL => .{ .params = 2, .results = 1 },
        .TABLE_INSERT => .{ .params = -1, .results = 0 }, // 2 or 3 parameters
        .TABLE_UNPACK => .{ .params = -1, .results = -1 }, // 1, 2 or 3 parameters
        .TABLE_CREATE => .{ .params = -1, .results = 1 }, // 1 or 2 parameters
        .TABLE_CLEAR => .{ .params = 1, .results = 0 },
        .VECTOR => .{ .params = -1, .results = 1 }, // 3 or 4 parameters in some configurations
        .BIT32_COUNTLZ, .BIT32_COUNTRZ => .{ .params = 1, .results = 1, .none_safe = ns },
        .SELECT_VARARG => .{ .params = -1, .results = -1 }, // variadic
        .RAWLEN => .{ .params = 1, .results = 1, .none_safe = ns },
        .BIT32_EXTRACTK => .{ .params = 3, .results = 1, .none_safe = ns },
        .GETMETATABLE => .{ .params = 1, .results = 1 },
        .SETMETATABLE => .{ .params = 2, .results = 1 },
        .TONUMBER => .{ .params = -1, .results = 1 }, // 1 or 2 parameters
        .TOSTRING => .{ .params = 1, .results = 1 },
        .BIT32_BYTESWAP => .{ .params = 1, .results = 1, .none_safe = ns },
        .BUFFER_READI8, .BUFFER_READU8, .BUFFER_READI16, .BUFFER_READU16, .BUFFER_READI32, .BUFFER_READU32, .BUFFER_READF32, .BUFFER_READF64 => .{ .params = 2, .results = 1, .none_safe = ns },
        .BUFFER_WRITEU8, .BUFFER_WRITEU16, .BUFFER_WRITEU32, .BUFFER_WRITEF32, .BUFFER_WRITEF64, .BUFFER_WRITEINTEGER => .{ .params = 3, .results = 0, .none_safe = ns },
        .BUFFER_READINTEGER => .{ .params = 2, .results = 1, .none_safe = ns },
        .VECTOR_MAGNITUDE, .VECTOR_NORMALIZE => .{ .params = 1, .results = 1, .none_safe = ns },
        .VECTOR_CROSS, .VECTOR_DOT => .{ .params = 2, .results = 1, .none_safe = ns },
        .VECTOR_FLOOR, .VECTOR_CEIL, .VECTOR_ABS, .VECTOR_SIGN => .{ .params = 1, .results = 1, .none_safe = ns },
        .VECTOR_CLAMP => .{ .params = 3, .results = 1, .none_safe = ns },
        .VECTOR_MIN, .VECTOR_MAX => .{ .params = -1, .results = 1 }, // variadic
        .VECTOR_LERP => .{ .params = 3, .results = 1, .none_safe = ns },
        .MATH_LERP => .{ .params = 3, .results = 1, .none_safe = ns },
        .MATH_ISNAN, .MATH_ISINF, .MATH_ISFINITE => .{ .params = 1, .results = 1, .none_safe = ns },
        .INTEGER_BAND, .INTEGER_BOR, .INTEGER_BXOR, .INTEGER_BTEST, .INTEGER_MIN, .INTEGER_MAX => .{ .params = -1, .results = 1 }, // variadic
        .INTEGER_EXTRACT => .{ .params = -1, .results = 1 }, // 2 or 3 parameters
        .INTEGER_BNOT, .INTEGER_BSWAP, .INTEGER_NEG, .INTEGER_COUNTLZ, .INTEGER_COUNTRZ, .INTEGER_TONUMBER, .INTEGER_CREATE => .{ .params = 1, .results = 1, .none_safe = ns },
        .INTEGER_CLAMP => .{ .params = 3, .results = 1, .none_safe = ns },
        .INTEGER_ADD, .INTEGER_SUB, .INTEGER_DIV, .INTEGER_REM, .INTEGER_UDIV, .INTEGER_UREM, .INTEGER_MOD, .INTEGER_MUL, .INTEGER_IDIV, .INTEGER_LT, .INTEGER_LE, .INTEGER_ULT, .INTEGER_ULE, .INTEGER_GT, .INTEGER_GE, .INTEGER_UGT, .INTEGER_UGE, .INTEGER_LSHIFT, .INTEGER_RSHIFT, .INTEGER_ARSHIFT, .INTEGER_LROTATE, .INTEGER_RROTATE => .{ .params = 2, .results = 1, .none_safe = ns },
    };
}
