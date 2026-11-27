//! Math library (upstream VM/src/lmathlib.cpp), translated to Zig.
//!
//! Portions derived from Luau (MIT, Copyright (c) 2019-2025 Roblox Corporation) and
//! Lua 5.x (MIT, Copyright (C) 1994-2008 Lua.org, PUC-Rio).
//!
//! Numerics: transcendental functions use Zig-native implementations (std.math /
//! compiler-rt, musl-derived) rather than the host C runtime. Upstream's own results
//! already differ by last ulps between glibc and UCRT builds; ours may differ from either
//! in the last ulp. Domains, infinities, NaNs and signed zeros follow C99 Annex F like
//! both upstream runtimes (C modf/frexp edge cases are handled explicitly).
//! `math.pow` shares the VM's `luai_numpow` so it always agrees with `^`.
//! The PCG32 state upstream keeps in global_State::rngstate lives in a buffer shared as an
//! upvalue by math.random and math.randomseed (one per luaopen_math, i.e. per VM).
//! Fast flag FixMathNoisePrecision (stdlib/flags.zig) selects the wrapped noise inputs.

const std = @import("std");
const api = @import("api.zig");
const aux = @import("auxiliary.zig");
const flags = @import("flags.zig");
const ostime = @import("ostime.zig");
const luau_math = @import("../luau_math.zig");
const State = api.State;

const LUAU_PI: f64 = 3.14159265358979323846;
const RADIANS_PER_DEGREE: f64 = LUAU_PI / 180.0;
const LUAU_E: f64 = 2.71828182845904523536;
const LUAU_PHI: f64 = 1.61803398874989484820;
const LUAU_SQRT2: f64 = 1.41421356237309504880;
const LUAU_TAU: f64 = 6.28318530717958647692;

const PCG32_INC: u64 = 105;

fn pcg32_random(state: *u64) u32 {
    const oldstate = state.*;
    state.* = oldstate *% 6364136223846793005 +% (PCG32_INC | 1);
    const xorshifted: u32 = @truncate(((oldstate >> 18) ^ oldstate) >> 27);
    const rot: u32 = @intCast(oldstate >> 59);
    return std.math.rotr(u32, xorshifted, rot);
}

fn pcg32_seed(state: *u64, seed: u64) void {
    state.* = 0;
    _ = pcg32_random(state);
    state.* +%= seed;
    _ = pcg32_random(state);
}

/// The RNG state buffer bound as upvalue 1 of random/randomseed.
fn rngstate(L: *State) *align(1) u64 {
    const b = api.tobuffer(L, api.upvalueindex(1)).?;
    return @ptrCast(b.ptr);
}

fn math_abs(L: *State) c_int {
    api.pushnumber(L, @abs(aux.checknumber(L, 1)));
    return 1;
}

fn math_sin(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    api.pushnumber(L, luau_math.domainError(24, x, 0) orelse luau_math.transcendentalResult(24, x, 0, @sin(x)));
    return 1;
}

fn math_sinh(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    api.pushnumber(L, luau_math.transcendentalResult(23, x, 0, luau_math.sinh(x)));
    return 1;
}

fn math_cos(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    api.pushnumber(L, luau_math.domainError(9, x, 0) orelse luau_math.transcendentalResult(9, x, 0, @cos(x)));
    return 1;
}

fn math_cosh(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    api.pushnumber(L, luau_math.transcendentalResult(8, x, 0, std.math.cosh(x)));
    return 1;
}

fn math_tan(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    api.pushnumber(L, luau_math.domainError(27, x, 0) orelse luau_math.transcendentalResult(27, x, 0, @tan(x)));
    return 1;
}

fn math_tanh(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    api.pushnumber(L, luau_math.transcendentalResult(26, x, 0, luau_math.tanh(x)));
    return 1;
}

fn math_asin(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    api.pushnumber(L, luau_math.domainError(4, x, 0) orelse luau_math.transcendentalResult(4, x, 0, std.math.asin(x)));
    return 1;
}

fn math_acos(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    api.pushnumber(L, luau_math.domainError(3, x, 0) orelse luau_math.transcendentalResult(3, x, 0, std.math.acos(x)));
    return 1;
}

fn math_atan(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    api.pushnumber(L, luau_math.transcendentalResult(6, x, 0, std.math.atan(x)));
    return 1;
}

fn math_atan2(L: *State) c_int {
    const y = aux.checknumber(L, 1);
    const x = aux.checknumber(L, 2);
    api.pushnumber(L, luau_math.transcendentalResult(5, y, x, std.math.atan2(y, x)));
    return 1;
}

fn math_ceil(L: *State) c_int {
    api.pushnumber(L, @ceil(aux.checknumber(L, 1)));
    return 1;
}

fn math_floor(L: *State) c_int {
    api.pushnumber(L, @floor(aux.checknumber(L, 1)));
    return 1;
}

fn math_fmod(L: *State) c_int {
    const a = aux.checknumber(L, 1);
    const b = aux.checknumber(L, 2);
    api.pushnumber(L, @rem(a, b));
    return 1;
}

fn math_modf(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    const ip = @trunc(x);
    // C modf: the fractional part of +-inf is +-0 and carries the sign of x
    const fp = if (std.math.isInf(x)) std.math.copysign(@as(f64, 0.0), x) else std.math.copysign(x - ip, x);
    api.pushnumber(L, ip);
    api.pushnumber(L, fp);
    return 2;
}

fn math_sqrt(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    api.pushnumber(L, luau_math.domainError(25, x, 0) orelse luau_math.transcendentalResult(25, x, 0, @sqrt(x)));
    return 1;
}

fn math_pow(L: *State) c_int {
    const a = aux.checknumber(L, 1);
    const b = aux.checknumber(L, 2);
    api.pushnumber(L, luau_math.domainError(21, a, b) orelse luau_math.transcendentalResult(21, a, b, api.vm.luai_numpow(a, b)));
    return 1;
}

fn math_log(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    const has_base = !api.isnoneornil(L, 2);
    const base = if (has_base) aux.checknumber(L, 2) else 0;
    const id: u8 = if (base == 10) 16 else 17;
    if (luau_math.domainError(id, x, base)) |result| {
        api.pushnumber(L, result);
        return 1;
    }
    const res = if (!has_base) @log(x) else if (base == 2) @log2(x) else if (base == 10)
        luau_math.log10(x)
    else
        @log(x) / @log(base);
    api.pushnumber(L, luau_math.transcendentalResult(id, x, base, res));
    return 1;
}

fn math_log10(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    api.pushnumber(L, luau_math.domainError(16, x, 0) orelse luau_math.transcendentalResult(16, x, 0, luau_math.log10(x)));
    return 1;
}

fn math_exp(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    api.pushnumber(L, luau_math.transcendentalResult(11, x, 0, luau_math.exp(x)));
    return 1;
}

fn math_deg(L: *State) c_int {
    api.pushnumber(L, aux.checknumber(L, 1) / RADIANS_PER_DEGREE);
    return 1;
}

fn math_rad(L: *State) c_int {
    api.pushnumber(L, aux.checknumber(L, 1) * RADIANS_PER_DEGREE);
    return 1;
}

fn math_frexp(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    if (x == 0 or std.math.isNan(x) or std.math.isInf(x)) {
        // C frexp quiets signaling NaNs while preserving their sign and payload.
        api.pushnumber(L, luau_math.quietNan(x));
        api.pushinteger(L, 0);
        return 2;
    }
    const r = std.math.frexp(x);
    api.pushnumber(L, r.significand);
    api.pushinteger(L, r.exponent);
    return 2;
}

fn math_ldexp(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    const e = aux.checkinteger(L, 2);
    // std.math.ldexp can overflow zero to infinity; C ldexp preserves its sign.
    api.pushnumber(L, if (std.math.isNan(x)) luau_math.quietNan(x) else if (x == 0) x else std.math.ldexp(x, e));
    return 1;
}

fn math_min(L: *State) c_int {
    const n = api.gettop(L); // number of arguments
    var dmin = aux.checknumber(L, 1);
    var i: c_int = 2;
    while (i <= n) : (i += 1) {
        const d = aux.checknumber(L, i);
        if (d < dmin) dmin = d;
    }
    api.pushnumber(L, dmin);
    return 1;
}

fn math_max(L: *State) c_int {
    const n = api.gettop(L); // number of arguments
    var dmax = aux.checknumber(L, 1);
    var i: c_int = 2;
    while (i <= n) : (i += 1) {
        const d = aux.checknumber(L, i);
        if (d > dmax) dmax = d;
    }
    api.pushnumber(L, dmax);
    return 1;
}

fn math_random(L: *State) c_int {
    // the state is read and written back around each draw; argument errors unwind before
    // any draw, so no cleanup is needed on the error path
    const state = rngstate(L);
    var st = state.*;
    switch (api.gettop(L)) { // check number of arguments
        0 => { // no arguments
            // Using ldexp instead of division for speed & clarity.
            // See http://mumble.net/~campbell/tmp/random_real.c for details on generating doubles from integer ranges.
            const rl = pcg32_random(&st);
            const rh = pcg32_random(&st);
            state.* = st;
            const rd = std.math.ldexp(@as(f64, @floatFromInt(@as(u64, rl) | (@as(u64, rh) << 32))), -64);
            api.pushnumber(L, rd); // number between 0 and 1
        },
        1 => { // only upper limit
            const u = aux.checkinteger(L, 1);
            aux.argcheck(L, 1 <= u, 1, "interval is empty");
            const x: u64 = @as(u64, @as(u32, @bitCast(u))) * pcg32_random(&st);
            state.* = st;
            const r: c_int = @bitCast(@as(u32, 1) +% @as(u32, @truncate(x >> 32)));
            api.pushinteger(L, r); // int between 1 and `u'
        },
        2 => { // lower and upper limits
            const l = aux.checkinteger(L, 1);
            const u = aux.checkinteger(L, 2);
            aux.argcheck(L, l <= u, 2, "interval is empty");
            const ul: u32 = @as(u32, @bitCast(u)) -% @as(u32, @bitCast(l));
            aux.argcheck(L, ul < std.math.maxInt(u32), 2, "interval is too large"); // -INT_MIN..INT_MAX interval can result in integer overflow
            const x: u64 = @as(u64, ul + 1) * pcg32_random(&st);
            state.* = st;
            const r: c_int = @bitCast(@as(u32, @bitCast(l)) +% @as(u32, @truncate(x >> 32)));
            api.pushinteger(L, r); // int between `l' and `u'
        },
        else => aux.errorf(L, "wrong number of arguments", .{}),
    }
    return 1;
}

fn math_randomseed(L: *State) c_int {
    const seed = aux.checkinteger(L, 1);
    var st: u64 = undefined;
    pcg32_seed(&st, @bitCast(@as(i64, seed)));
    rngstate(L).* = st;
    return 0;
}

const kPerlinHash = [257]u8{
    151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,  23,
    190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203, 117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,
    125, 136, 171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,
    41,  55,  46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130,
    116, 188, 159, 86,  164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,   202, 38,  147, 118, 126, 255, 82,  85,  212, 207,
    206, 59,  227, 47,  16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,
    172, 9,   129, 22,  39,  253, 19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228, 251, 34,  242, 193, 238, 210, 144,
    12,  191, 179, 162, 241, 81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157, 184, 84,  204, 176, 115, 121, 50,  45,
    127, 4,   150, 254, 138, 236, 205, 93,  222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156, 180, 151,
};

const kPerlinGrad = [16][3]f32{
    .{ 1, 1, 0 },
    .{ -1, 1, 0 },
    .{ 1, -1, 0 },
    .{ -1, -1, 0 },
    .{ 1, 0, 1 },
    .{ -1, 0, 1 },
    .{ 1, 0, -1 },
    .{ -1, 0, -1 },
    .{ 0, 1, 1 },
    .{ 0, -1, 1 },
    .{ 0, 1, -1 },
    .{ 0, -1, -1 },
    .{ 1, 1, 0 },
    .{ 0, -1, 1 },
    .{ -1, 1, 0 },
    .{ 0, -1, -1 },
};

inline fn perlin_fade(t: f32) f32 {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

inline fn perlin_lerp(t: f32, a: f32, b: f32) f32 {
    return a + t * (b - a);
}

inline fn perlin_grad(hash: u8, x: f32, y: f32, z: f32) f32 {
    const g = kPerlinGrad[hash & 15];
    return g[0] * x + g[1] * y + g[2] * z;
}

/// int(f) with x86 cvttss2si semantics (out-of-range and NaN give INT_MIN).
fn f2int(f: f32) i32 {
    if (std.math.isNan(f) or f >= 2147483648.0 or f < -2147483648.0) return std.math.minInt(i32);
    return @intFromFloat(f);
}

fn perlin(x: f32, y: f32, z: f32) f32 {
    const xflr = @floor(x);
    const yflr = @floor(y);
    const zflr = @floor(z);

    const xi: usize = @intCast(f2int(xflr) & 255);
    const yi: usize = @intCast(f2int(yflr) & 255);
    const zi: usize = @intCast(f2int(zflr) & 255);

    const xf = x - xflr;
    const yf = y - yflr;
    const zf = z - zflr;

    const u = perlin_fade(xf);
    const v = perlin_fade(yf);
    const w = perlin_fade(zf);

    const p = &kPerlinHash;

    const a = (p[xi] + yi) & 255;
    const aa = (p[a] + zi) & 255;
    const ab = (p[a + 1] + zi) & 255;

    const b = (p[xi + 1] + yi) & 255;
    const ba = (p[b] + zi) & 255;
    const bb = (p[b + 1] + zi) & 255;

    const la = perlin_lerp(u, perlin_grad(p[aa], xf, yf, zf), perlin_grad(p[ba], xf - 1, yf, zf));
    const lb = perlin_lerp(u, perlin_grad(p[ab], xf, yf - 1, zf), perlin_grad(p[bb], xf - 1, yf - 1, zf));
    const la1 = perlin_lerp(u, perlin_grad(p[aa + 1], xf, yf, zf - 1), perlin_grad(p[ba + 1], xf - 1, yf, zf - 1));
    const lb1 = perlin_lerp(u, perlin_grad(p[ab + 1], xf, yf - 1, zf - 1), perlin_grad(p[bb + 1], xf - 1, yf - 1, zf - 1));

    return perlin_lerp(w, perlin_lerp(v, la, lb), perlin_lerp(v, la1, lb1));
}

fn math_noise(L: *State) c_int {
    const nx = api.tonumberx(L, 1);
    const ny = api.tonumberx(L, 2);
    const nz = api.tonumberx(L, 3);

    aux.argexpected(L, nx != null, 1, "number");
    aux.argexpected(L, ny != null or api.isnoneornil(L, 2), 2, "number");
    aux.argexpected(L, nz != null or api.isnoneornil(L, 3), 3, "number");

    // lua_tonumberx yields 0 for non-numbers
    var x = nx.?;
    var y = ny orelse 0;
    var z = nz orelse 0;

    if (flags.FixMathNoisePrecision) {
        // Noise repeats exactly every 256 units in all dimensions, so wrap large inputs to
        // keep the precision perlin() needs in float (upstream FixMathNoisePrecision).
        x = @rem(x, 256.0);
        y = @rem(y, 256.0);
        z = @rem(z, 256.0);
    }

    const r: f64 = perlin(@floatCast(x), @floatCast(y), @floatCast(z));
    api.pushnumber(L, r);
    return 1;
}

fn math_clamp(L: *State) c_int {
    const v = aux.checknumber(L, 1);
    const min = aux.checknumber(L, 2);
    const max = aux.checknumber(L, 3);

    aux.argcheck(L, min <= max, 3, "max must be greater than or equal to min");

    var r = if (v < min) min else v;
    r = if (r > max) max else r;

    api.pushnumber(L, r);
    return 1;
}

fn math_sign(L: *State) c_int {
    const v = aux.checknumber(L, 1);
    api.pushnumber(L, if (v > 0.0) 1.0 else if (v < 0.0) -1.0 else 0.0);
    return 1;
}

fn math_round(L: *State) c_int {
    api.pushnumber(L, @round(aux.checknumber(L, 1)));
    return 1;
}

fn math_map(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    const inmin = aux.checknumber(L, 2);
    const inmax = aux.checknumber(L, 3);
    const outmin = aux.checknumber(L, 4);
    const outmax = aux.checknumber(L, 5);

    const result = outmin + (x - inmin) * (outmax - outmin) / (inmax - inmin);
    api.pushnumber(L, result);
    return 1;
}

fn math_lerp(L: *State) c_int {
    const a = aux.checknumber(L, 1);
    const b = aux.checknumber(L, 2);
    const t = aux.checknumber(L, 3);

    const r = if (t == 1.0) b else a + (b - a) * t;
    api.pushnumber(L, r);
    return 1;
}

fn math_isnan(L: *State) c_int {
    api.pushboolean(L, std.math.isNan(aux.checknumber(L, 1)));
    return 1;
}

fn math_isinf(L: *State) c_int {
    api.pushboolean(L, std.math.isInf(aux.checknumber(L, 1)));
    return 1;
}

fn math_isfinite(L: *State) c_int {
    api.pushboolean(L, std.math.isFinite(aux.checknumber(L, 1)));
    return 1;
}

const mathlib = [_]aux.Entry{
    aux.entry("abs", math_abs),
    aux.entry("acos", math_acos),
    aux.entry("asin", math_asin),
    aux.entry("atan2", math_atan2),
    aux.entry("atan", math_atan),
    aux.entry("ceil", math_ceil),
    aux.entry("cosh", math_cosh),
    aux.entry("cos", math_cos),
    aux.entry("deg", math_deg),
    aux.entry("exp", math_exp),
    aux.entry("floor", math_floor),
    aux.entry("fmod", math_fmod),
    aux.entry("frexp", math_frexp),
    aux.entry("ldexp", math_ldexp),
    aux.entry("log10", math_log10),
    aux.entry("log", math_log),
    aux.entry("max", math_max),
    aux.entry("min", math_min),
    aux.entry("modf", math_modf),
    aux.entry("pow", math_pow),
    aux.entry("rad", math_rad),
    aux.entry("random", math_random), // closure over the RNG state, see open()
    aux.entry("randomseed", math_randomseed), // closure over the RNG state, see open()
    aux.entry("sinh", math_sinh),
    aux.entry("sin", math_sin),
    aux.entry("sqrt", math_sqrt),
    aux.entry("tanh", math_tanh),
    aux.entry("tan", math_tan),
    aux.entry("noise", math_noise),
    aux.entry("clamp", math_clamp),
    aux.entry("sign", math_sign),
    aux.entry("round", math_round),
    aux.entry("map", math_map),
    aux.entry("lerp", math_lerp),
    aux.entry("isnan", math_isnan),
    aux.entry("isinf", math_isinf),
    aux.entry("isfinite", math_isfinite),
};

pub fn open(L: *State) c_int {
    // upstream: lua_encodepointer(L) ^ time(NULL) ^ clock(); clock() (process CPU time)
    // is approximated by the VM's high-resolution lua_clock()
    var seed: u64 = api.encodepointer(L, @intFromPtr(L));
    seed ^= @bitCast(ostime.now());
    seed ^= @bitCast(@as(i64, @intFromFloat(api.clock() * 1000.0)));

    aux.openlib(L, "math", @intCast(mathlib.len));

    // RNG state shared by random/randomseed (upstream: global_State::rngstate)
    const state = api.newbuffer(L, @sizeOf(u64));
    var st: u64 = 0;
    pcg32_seed(&st, seed);
    @memcpy(state, std.mem.asBytes(&st));

    // same registration order as luaL_register(L, "math", mathlib)
    for (mathlib) |f| {
        if (std.mem.startsWith(u8, api.span(f.name), "random")) {
            api.pushvalue(L, -1);
            api.pushcclosure(L, f.func, f.name, 1);
        } else {
            api.pushcfunction(L, f.func, f.name);
        }
        api.setfield(L, -3, f.name);
    }
    api.pop(L, 1); // RNG state buffer

    api.pushnumber(L, LUAU_PI);
    api.setfield(L, -2, "pi");
    api.pushnumber(L, std.math.inf(f64));
    api.setfield(L, -2, "huge");
    api.pushnumber(L, std.math.nan(f64));
    api.setfield(L, -2, "nan");
    api.pushnumber(L, LUAU_E);
    api.setfield(L, -2, "e");
    api.pushnumber(L, LUAU_PHI);
    api.setfield(L, -2, "phi");
    api.pushnumber(L, LUAU_SQRT2);
    api.setfield(L, -2, "sqrt2");
    api.pushnumber(L, LUAU_TAU);
    api.setfield(L, -2, "tau");

    return 1;
}
