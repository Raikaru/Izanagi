//! 64-bit integer library (upstream VM/src/lintlib.cpp), translated to Zig.
//!
//! Portions derived from Luau (MIT, Copyright (c) 2019-2025 Roblox Corporation).
//!
//! Operates on the VM's LUA_TINTEGER values through lua_pushinteger64 /
//! luaL_checkinteger64. Upstream opens it from luaL_openlibs only when the
//! LuauIntegerLibrary fast flag is set.

const std = @import("std");
const api = @import("api.zig");
const aux = @import("auxiliary.zig");
const cstrto = @import("cstrto.zig");
const State = api.State;

const LLONG_MIN = std.math.minInt(i64);
const LLONG_MAX = std.math.maxInt(i64);

inline fn mask64(w: i64) u64 {
    return @as(u64, 0xFFFFFFFFFFFFFFFF) >> @intCast(64 - w);
}

inline fn chk(L: *State, n: c_int) i64 {
    return aux.checkinteger64(L, n);
}
inline fn chku(L: *State, n: c_int) u64 {
    return @bitCast(aux.checkinteger64(L, n));
}

fn int64_create(L: *State) c_int {
    const x = aux.checknumber(L, 1);
    if (x >= -9223372036854775808.0 and x < 9223372036854775808.0) {
        const l: i64 = @intFromFloat(@trunc(x));
        if (@as(f64, @floatFromInt(l)) == x) {
            api.pushinteger64(L, l);
            return 1;
        }
    }
    api.pushnil(L);
    return 1;
}

fn int64_fromstring(L: *State) c_int {
    const s = aux.checklstring(L, 1);
    const base = aux.optinteger(L, 2, 10);
    aux.argcheck(L, 2 <= base and base <= 36, 2, "base out of range");
    if (cstrto.str2l(s, @intCast(base))) |result| {
        api.pushinteger64(L, result);
    } else {
        api.pushnil(L);
    }
    return 1;
}

fn int64_tonumber(L: *State) c_int {
    api.pushnumber(L, @floatFromInt(chk(L, 1)));
    return 1;
}

fn int64_neg(L: *State) c_int {
    api.pushinteger64(L, @bitCast(~chku(L, 1) +% 1));
    return 1;
}

fn int64_add(L: *State) c_int {
    const x = chku(L, 1);
    const y = chku(L, 2);
    api.pushinteger64(L, @bitCast(x +% y));
    return 1;
}

fn int64_sub(L: *State) c_int {
    const x = chku(L, 1);
    const y = chku(L, 2);
    api.pushinteger64(L, @bitCast(x -% y));
    return 1;
}

fn int64_mul(L: *State) c_int {
    const x = chku(L, 1);
    const y = chku(L, 2);
    api.pushinteger64(L, @bitCast(x *% y));
    return 1;
}

fn checkdiv(L: *State, a: i64, b: i64) void {
    if (b == 0) aux.errorf(L, "division by zero", .{});
    if (a == LLONG_MIN and b == -1) aux.errorf(L, "integer overflow", .{});
}

fn int64_div(L: *State) c_int {
    const a = chk(L, 1);
    const b = chk(L, 2);
    checkdiv(L, a, b);
    api.pushinteger64(L, @divTrunc(a, b));
    return 1;
}

fn int64_idiv(L: *State) c_int {
    const a = chk(L, 1);
    const b = chk(L, 2);
    checkdiv(L, a, b);
    // Floored division rounds toward -inf
    api.pushinteger64(L, @divFloor(a, b));
    return 1;
}

fn int64_rem(L: *State) c_int {
    const a = chk(L, 1);
    const b = chk(L, 2);
    if (b == 0) aux.errorf(L, "division by zero", .{});
    if (a == LLONG_MIN and b == -1) {
        api.pushinteger64(L, 0);
        return 1;
    }
    api.pushinteger64(L, @rem(a, b));
    return 1;
}

fn int64_mod(L: *State) c_int {
    const a = chk(L, 1);
    const b = chk(L, 2);
    if (b == 0) aux.errorf(L, "division by zero", .{});
    var remainder: i64 = 0;
    if (a != LLONG_MIN or b != -1) {
        remainder = @rem(a, b);
        if (remainder != 0 and ((a < 0) != (b < 0))) remainder +%= b;
    }
    api.pushinteger64(L, remainder);
    return 1;
}

fn int64_udiv(L: *State) c_int {
    const a = chku(L, 1);
    const b = chku(L, 2);
    if (b == 0) aux.errorf(L, "division by zero", .{});
    api.pushinteger64(L, @bitCast(a / b));
    return 1;
}

fn int64_urem(L: *State) c_int {
    const a = chku(L, 1);
    const b = chku(L, 2);
    if (b == 0) aux.errorf(L, "division by zero", .{});
    api.pushinteger64(L, @bitCast(a % b));
    return 1;
}

fn int64_min(L: *State) c_int {
    var tmin = chk(L, 1);
    const n = api.gettop(L);
    var i: c_int = 2;
    while (i <= n) : (i += 1) {
        const x = chk(L, i);
        if (x < tmin) tmin = x;
    }
    api.pushinteger64(L, tmin);
    return 1;
}

fn int64_max(L: *State) c_int {
    var tmax = chk(L, 1);
    const n = api.gettop(L);
    var i: c_int = 2;
    while (i <= n) : (i += 1) {
        const x = chk(L, i);
        if (x > tmax) tmax = x;
    }
    api.pushinteger64(L, tmax);
    return 1;
}

fn bitfold(L: *State, comptime op: enum { band, bor, bxor }) u64 {
    var tres: u64 = if (op == .band) std.math.maxInt(u64) else 0;
    const n = api.gettop(L);
    var i: c_int = 1;
    while (i <= n) : (i += 1) {
        const x = chku(L, i);
        switch (op) {
            .band => tres &= x,
            .bor => tres |= x,
            .bxor => tres ^= x,
        }
    }
    return tres;
}

fn int64_band(L: *State) c_int {
    api.pushinteger64(L, @bitCast(bitfold(L, .band)));
    return 1;
}

fn int64_bor(L: *State) c_int {
    api.pushinteger64(L, @bitCast(bitfold(L, .bor)));
    return 1;
}

fn int64_bnot(L: *State) c_int {
    api.pushinteger64(L, @bitCast(~chku(L, 1)));
    return 1;
}

fn int64_bxor(L: *State) c_int {
    api.pushinteger64(L, @bitCast(bitfold(L, .bxor)));
    return 1;
}

fn int64_btest(L: *State) c_int {
    api.pushboolean(L, bitfold(L, .band) != 0);
    return 1;
}

fn cmp(comptime T: type, comptime op: std.math.CompareOperator) fn (*State) c_int {
    return struct {
        fn f(L: *State) c_int {
            const a: T = @bitCast(aux.checkinteger64(L, 1));
            const b: T = @bitCast(aux.checkinteger64(L, 2));
            api.pushboolean(L, std.math.compare(a, op, b));
            return 1;
        }
    }.f;
}

fn int64_lshift(L: *State) c_int {
    const n = chku(L, 1);
    const i = chk(L, 2);
    if (i >= -63 and i <= 63) {
        api.pushinteger64(L, @bitCast(if (i < 0) n >> @intCast(-i) else n << @intCast(i)));
    } else {
        api.pushinteger64(L, 0);
    }
    return 1;
}

fn int64_rshift(L: *State) c_int {
    const n = chku(L, 1);
    const i = chk(L, 2);
    if (i >= -63 and i <= 63) {
        api.pushinteger64(L, @bitCast(if (i < 0) n << @intCast(-i) else n >> @intCast(i)));
    } else {
        api.pushinteger64(L, 0);
    }
    return 1;
}

fn int64_arshift(L: *State) c_int {
    const n = chk(L, 1);
    const i = chk(L, 2);
    if (i >= -63 and i <= 63) {
        api.pushinteger64(L, if (i < 0) @bitCast(@as(u64, @bitCast(n)) << @intCast(-i)) else n >> @intCast(i));
    } else if (i < -63) {
        api.pushinteger64(L, 0);
    } else {
        api.pushinteger64(L, if (n < 0) -1 else 0);
    }
    return 1;
}

fn int64_lrotate(L: *State) c_int {
    const n = chku(L, 1);
    const s: u6 = @intCast(chku(L, 2) % 64);
    api.pushinteger64(L, @bitCast(std.math.rotl(u64, n, s)));
    return 1;
}

fn int64_rrotate(L: *State) c_int {
    const n = chku(L, 1);
    const s: u6 = @intCast(chku(L, 2) % 64);
    api.pushinteger64(L, @bitCast(std.math.rotr(u64, n, s)));
    return 1;
}

fn int64_extract(L: *State) c_int {
    const n = chk(L, 1);
    const f = chk(L, 2);
    const w = aux.optinteger64(L, 3, 1);
    aux.argcheck(L, 0 <= f, 2, "field cannot be negative");
    aux.argcheck(L, 0 < w, 3, "width must be positive");
    if (w > 64 - f) aux.errorf(L, "trying to access non-existent bits", .{});
    api.pushinteger64(L, @bitCast((@as(u64, @bitCast(n)) >> @intCast(f)) & mask64(w)));
    return 1;
}

fn int64_replace(L: *State) c_int {
    const n = chk(L, 1);
    const r = chk(L, 2);
    const f = chk(L, 3);
    const w = aux.optinteger64(L, 4, 1);
    aux.argcheck(L, 0 <= f, 3, "field cannot be negative");
    aux.argcheck(L, 0 < w, 4, "width must be positive");
    if (w > 64 - f) aux.errorf(L, "trying to access non-existent bits", .{});
    const fs: u6 = @intCast(f);
    const baseMask = mask64(w);
    const replacement = (@as(u64, @bitCast(r)) & baseMask) << fs;
    const mask = @as(u64, 0xFFFFFFFFFFFFFFFF) ^ (baseMask << fs);
    api.pushinteger64(L, @bitCast((@as(u64, @bitCast(n)) & mask) | replacement));
    return 1;
}

fn int64_clamp(L: *State) c_int {
    const a = chk(L, 1);
    const mi = chk(L, 2);
    const mx = chk(L, 3);
    aux.argcheck(L, mi <= mx, 3, "max must be greater than or equal to min");
    api.pushinteger64(L, if (a < mi) mi else if (a > mx) mx else a);
    return 1;
}

fn int64_countrz(L: *State) c_int {
    api.pushinteger64(L, @ctz(chku(L, 1)));
    return 1;
}

fn int64_countlz(L: *State) c_int {
    api.pushinteger64(L, @clz(chku(L, 1)));
    return 1;
}

fn int64_bswap(L: *State) c_int {
    api.pushinteger64(L, @bitCast(@byteSwap(chku(L, 1))));
    return 1;
}

const int64lib = [_]aux.Entry{
    aux.entry("create", int64_create),
    aux.entry("tonumber", int64_tonumber),
    aux.entry("neg", int64_neg),
    aux.entry("add", int64_add),
    aux.entry("sub", int64_sub),
    aux.entry("mul", int64_mul),
    aux.entry("div", int64_div),
    aux.entry("min", int64_min),
    aux.entry("max", int64_max),
    aux.entry("rem", int64_rem),
    aux.entry("idiv", int64_idiv),
    aux.entry("udiv", int64_udiv),
    aux.entry("urem", int64_urem),
    aux.entry("mod", int64_mod),
    aux.entry("clamp", int64_clamp),
    aux.entry("band", int64_band),
    aux.entry("bor", int64_bor),
    aux.entry("bnot", int64_bnot),
    aux.entry("bxor", int64_bxor),
    aux.entry("lt", cmp(i64, .lt)),
    aux.entry("le", cmp(i64, .lte)),
    aux.entry("ult", cmp(u64, .lt)),
    aux.entry("ule", cmp(u64, .lte)),
    aux.entry("gt", cmp(i64, .gt)),
    aux.entry("ge", cmp(i64, .gte)),
    aux.entry("ugt", cmp(u64, .gt)),
    aux.entry("uge", cmp(u64, .gte)),
    aux.entry("lshift", int64_lshift),
    aux.entry("rshift", int64_rshift),
    aux.entry("arshift", int64_arshift),
    aux.entry("lrotate", int64_lrotate),
    aux.entry("rrotate", int64_rrotate),
    aux.entry("extract", int64_extract),
    aux.entry("replace", int64_replace),
    aux.entry("btest", int64_btest),
    aux.entry("countrz", int64_countrz),
    aux.entry("countlz", int64_countlz),
    aux.entry("bswap", int64_bswap),
    aux.entry("fromstring", int64_fromstring),
};

pub fn open(L: *State) c_int {
    aux.register(L, "integer", &int64lib);
    api.pushinteger64(L, LLONG_MAX);
    api.setfield(L, -2, "maxsigned");
    api.pushinteger64(L, LLONG_MIN);
    api.setfield(L, -2, "minsigned");
    return 1;
}
