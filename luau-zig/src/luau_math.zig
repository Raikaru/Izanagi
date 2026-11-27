//! Pure Zig math shared by the runtime library (stdlib/math.zig) and the
//! compiler's builtin constant folding (compiler/builtin_folding.zig), so that
//! folded constants at -O2 equal the values the runtime computes at -O0/-O1.
//! Has no dependencies besides std.

const std = @import("std");

// Double-double evaluation avoids the compiler-rt exp(1) rounding difference
// exposed by the upstream conformance suite. This is not a proof of correct
// rounding, particularly for halfway and subnormal results.

const DD = struct { hi: f64, lo: f64 };

inline fn twoSum(a: f64, b: f64) DD {
    const s = a + b;
    const bb = s - a;
    return .{ .hi = s, .lo = (a - (s - bb)) + (b - bb) };
}
inline fn quickTwoSum(a: f64, b: f64) DD {
    const s = a + b;
    return .{ .hi = s, .lo = b - (s - a) };
}
inline fn twoProd(a: f64, b: f64) DD {
    const p = a * b;
    return .{ .hi = p, .lo = @mulAdd(f64, a, b, -p) };
}
fn ddAdd(x: DD, y: DD) DD {
    var s = twoSum(x.hi, y.hi);
    const t = twoSum(x.lo, y.lo);
    s.lo += t.hi;
    s = quickTwoSum(s.hi, s.lo);
    s.lo += t.lo;
    return quickTwoSum(s.hi, s.lo);
}
fn ddMul(x: DD, y: DD) DD {
    var p = twoProd(x.hi, y.hi);
    p.lo += x.hi * y.lo + x.lo * y.hi;
    return quickTwoSum(p.hi, p.lo);
}
fn ddMulD(x: DD, d: f64) DD {
    var p = twoProd(x.hi, d);
    p.lo += x.lo * d;
    return quickTwoSum(p.hi, p.lo);
}
fn ddDivD(x: DD, d: f64) DD {
    const q1 = x.hi / d;
    const p = twoProd(q1, d);
    const r = ddAdd(x, .{ .hi = -p.hi, .lo = -p.lo });
    const q2 = r.hi / d;
    return quickTwoSum(q1, q2);
}
fn ddDiv(x: DD, y: DD) DD {
    const q = x.hi / y.hi;
    const remainder = ddAdd(x, ddMulD(y, -q));
    return quickTwoSum(q, (remainder.hi + remainder.lo) / y.hi);
}

/// Match C math's quieting of signaling NaNs while retaining sign and payload.
pub fn quietNan(x: f64) f64 {
    if (!std.math.isNan(x)) return x;
    return @bitCast(@as(u64, @bitCast(x)) | 0x0008000000000000);
}
/// Deterministic domain-error NaNs, matching the x86-64 glibc reference.
/// IDs not listed here have no domain error of their own.
pub const domain_nan_bits = [_]struct { id: u8, bits: u64 }{
    .{ .id = 3, .bits = 0x7ff8000000000000 }, // acos
    .{ .id = 4, .bits = 0x7ff8000000000000 }, // asin
    .{ .id = 9, .bits = 0xfff8000000000000 }, // cos(infinity)
    .{ .id = 16, .bits = 0x7ff8000000000000 }, // log10
    .{ .id = 17, .bits = 0xfff8000000000000 }, // log
    .{ .id = 21, .bits = 0xfff8000000000000 }, // pow
    .{ .id = 24, .bits = 0xfff8000000000000 }, // sin(infinity)
    .{ .id = 25, .bits = 0xfff8000000000000 }, // sqrt
    .{ .id = 27, .bits = 0xfff8000000000000 }, // tan(infinity)
};

/// Return the fixed domain NaN before evaluating architecture-specific math.
pub fn domainError(id: u8, x: f64, y: f64) ?f64 {
    const invalid = switch (id) {
        3, 4 => @abs(x) > 1,
        9, 24, 27 => std.math.isInf(x),
        16, 17, 25 => x < 0,
        21 => x < 0 and std.math.isFinite(x) and std.math.isFinite(y) and y != @trunc(y),
        else => false,
    };
    if (!invalid or std.math.isNan(x) or std.math.isNan(y)) return null;
    inline for (domain_nan_bits) |entry| {
        if (id == entry.id) return @bitCast(entry.bits);
    }
    unreachable;
}

/// Preserve input NaN payloads deterministically; canonicalize domain errors.
pub fn transcendentalResult(id: u8, x: f64, y: f64, result: f64) f64 {
    const xb: u64 = @bitCast(x);
    const yb: u64 = @bitCast(y);
    if (std.math.isNan(x) and (xb & 0x0008000000000000) == 0) return quietNan(x);
    if (std.math.isNan(y) and (yb & 0x0008000000000000) == 0) return quietNan(y);
    if (!std.math.isNan(result)) return result;
    if (std.math.isNan(x)) return quietNan(x);
    if (std.math.isNan(y)) return quietNan(y);
    inline for (domain_nan_bits) |entry| {
        if (id == entry.id) return @bitCast(entry.bits);
    }
    return result;
}

/// Scale subnormal inputs before the compiler-rt logarithm loses precision.
pub fn log10(x: f64) f64 {
    if (x > 0 and x < 0x1p-1022)
        return @log10(x * 0x1p54) - 0x1.041704c068eefp+4;
    return @log10(x);
}

const LN2_DD = DD{ .hi = 0x1.62e42fefa39efp-1, .lo = 0x1.abc9e3b39803fp-56 };

/// e^x, used by math.exp and by its constant folding.
pub fn exp(x: f64) f64 {
    if (std.math.isNan(x)) return x;
    if (x > 7.09782712893383973096e+02) return std.math.inf(f64);
    if (x < -7.45133219101941108420e+02) return 0.0;
    if (@abs(x) < 0x1p-54) return 1.0 + x;

    const result = expFinite(x);
    return std.math.ldexp(result.value.hi + result.value.lo, result.exponent);
}

fn expFinite(x: f64) struct { value: DD, exponent: i32 } {
    // x = k*ln2 + r, |r| <= ln2/2, r in double-double
    const k = @round(x / LN2_DD.hi);
    const kl = ddMulD(LN2_DD, k);
    var r = ddAdd(.{ .hi = x, .lo = 0 }, .{ .hi = -kl.hi, .lo = -kl.lo });

    // exp(r) = exp(r/256)^256; Taylor series to degree 12 for |r/256| < 0.0014
    r = .{ .hi = r.hi * 0x1p-8, .lo = r.lo * 0x1p-8 };
    var term = r;
    var sum = ddAdd(.{ .hi = 1.0, .lo = 0 }, term);
    var n: f64 = 2;
    while (n <= 12) : (n += 1) {
        term = ddDivD(ddMul(term, r), n);
        sum = ddAdd(sum, term);
    }
    var i: usize = 0;
    while (i < 8) : (i += 1) sum = ddMul(sum, sum);
    return .{ .value = sum, .exponent = @intFromFloat(k) };
}

fn logPositive(x: f64) DD {
    const parts = std.math.frexp(x);
    var m = parts.significand;
    var exponent = parts.exponent;
    if (m < 0x1.6a09e667f3bcdp-1) {
        m *= 2;
        exponent -= 1;
    }
    const z = ddDiv(.{ .hi = m - 1, .lo = 0 }, twoSum(m, 1));
    const z2 = ddMul(z, z);
    var term = z;
    var sum = z;
    var n: f64 = 3;
    while (n <= 45) : (n += 2) {
        term = ddMul(term, z2);
        sum = ddAdd(sum, ddDivD(term, n));
    }
    return ddAdd(ddMulD(sum, 2), ddMulD(LN2_DD, @floatFromInt(exponent)));
}

/// Range-reduced high-precision power for the common positive fractional range.
pub fn pow(x: f64, y: f64) f64 {
    if (!(x >= 0x1p-10 and x <= 2 and @abs(y) <= 1))
        return std.math.pow(f64, x, y);
    const product = ddMulD(logPositive(x), y);
    const raw = expFinite(product.hi);
    const corrected = ddMul(raw.value, .{ .hi = 1, .lo = product.lo });
    return std.math.ldexp(corrected.hi + corrected.lo, raw.exponent);
}

/// Small-argument sinh in double-double to avoid excess cancellation error.
pub fn sinh(x: f64) f64 {
    const magnitude = @abs(x);
    if (magnitude < 0x1p-28) return x;
    if (!(magnitude < 1)) return std.math.sinh(x);
    const raw = expFinite(magnitude);
    const e = DD{ .hi = std.math.ldexp(raw.value.hi, raw.exponent),
                  .lo = std.math.ldexp(raw.value.lo, raw.exponent) };
    const inverse = ddDiv(.{ .hi = 1, .lo = 0 }, e);
    const result = ddAdd(e, .{ .hi = -inverse.hi, .lo = -inverse.lo });
    return std.math.copysign((result.hi + result.lo) * 0.5, x);
}

/// tanh evaluated in double-double to avoid cancellation and extra ULPs.
pub fn tanh(x: f64) f64 {
    if (std.math.isNan(x)) return quietNan(x);
    const magnitude = @abs(x);
    if (magnitude < 0x1p-28) return x;
    if (magnitude >= 20) return std.math.copysign(@as(f64, 1), x);
    const raw = expFinite(2 * magnitude);
    const e = DD{ .hi = std.math.ldexp(raw.value.hi, raw.exponent),
                  .lo = std.math.ldexp(raw.value.lo, raw.exponent) };
    const numerator = ddAdd(e, .{ .hi = -1, .lo = 0 });
    const denominator = ddAdd(e, .{ .hi = 1, .lo = 0 });
    const result = ddDiv(numerator, denominator);
    return std.math.copysign(result.hi + result.lo, x);
}

test "subnormal log and tanh reference regressions" {
    const log_input: f64 = @bitCast(@as(u64, 0x000ccedf985dedea));
    const log_bits: u64 = @bitCast(log10(log_input));
    try std.testing.expect(log_bits >= 0xc0733bfd186b2c11 and log_bits <= 0xc0733bfd186b2c13);
    const tanh_input: f64 = @bitCast(@as(u64, 0xbfdb1ff565f67a7d));
    const tanh_bits: u64 = @bitCast(tanh(tanh_input));
    try std.testing.expect(tanh_bits >= 0xbfd99c07fe1290e0 and tanh_bits <= 0xbfd99c07fe1290e2);
    const signaling: f64 = @bitCast(@as(u64, 0x7ff6e227ed2e5b73));
    try std.testing.expectEqual(@as(u64, 0x7ffee227ed2e5b73), @as(u64, @bitCast(quietNan(signaling))));
}

test "domain NaNs use the pinned per-operation bits" {
    const positive: u64 = 0x7ff8000000000000;
    const negative: u64 = 0xfff8000000000000;
    const cases = [_]struct { id: u8, x: f64, y: f64, expected: u64 }{
        .{ .id = 3, .x = 2, .y = 0, .expected = positive },
        .{ .id = 4, .x = -2, .y = 0, .expected = positive },
        .{ .id = 16, .x = -1, .y = 0, .expected = positive },
        .{ .id = 17, .x = -1, .y = 2, .expected = negative },
        .{ .id = 21, .x = -1, .y = 0.5, .expected = negative },
        .{ .id = 25, .x = -1, .y = 0, .expected = negative },
        .{ .id = 9, .x = std.math.inf(f64), .y = 0, .expected = negative },
        .{ .id = 24, .x = std.math.inf(f64), .y = 0, .expected = negative },
        .{ .id = 27, .x = std.math.inf(f64), .y = 0, .expected = negative },
    };
    for (cases) |case| {
        const value = domainError(case.id, case.x, case.y) orelse return error.MissingDomainNaN;
        try std.testing.expectEqual(case.expected, @as(u64, @bitCast(value)));
    }
    try std.testing.expect(domainError(25, -0.0, 0) == null);
}
