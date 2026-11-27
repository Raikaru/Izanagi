//! Vector library (upstream VM/src/lveclib.cpp), translated to Zig.
//!
//! Portions derived from Luau (MIT, Copyright (c) 2019-2025 Roblox Corporation).
//!
//! LUA_VECTOR_SIZE == 3 with float components (LUA_VECTOR_DOUBLE == 0), as in the vendored
//! luaconf.h: component arithmetic is done in single precision exactly like the C++.

const std = @import("std");
const api = @import("api.zig");
const aux = @import("auxiliary.zig");
const State = api.State;

fn vector_create(L: *State) c_int {
    // checking argument count to avoid accepting 'nil' as a valid value
    const count = api.gettop(L);
    const x = aux.checknumber(L, 1);
    const y = aux.checknumber(L, 2);
    const z = if (count >= 3) aux.checknumber(L, 3) else 0.0;
    api.pushvector(L, @floatCast(x), @floatCast(y), @floatCast(z));
    return 1;
}

inline fn dot3(a: *const [3]f32, b: *const [3]f32) f32 {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline fn cross3(a: *const [3]f32, b: *const [3]f32) [3]f32 {
    return .{ a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] };
}

fn vector_magnitude(L: *State) c_int {
    const v = aux.checkvector(L, 1);
    const magnitude: f32 = @sqrt(dot3(v, v));
    api.pushnumber(L, magnitude);
    return 1;
}

fn vector_normalize(L: *State) c_int {
    const v = aux.checkvector(L, 1);
    const invSqrt: f32 = 1.0 / @sqrt(dot3(v, v));
    api.pushvector(L, v[0] * invSqrt, v[1] * invSqrt, v[2] * invSqrt);
    return 1;
}

fn vector_cross(L: *State) c_int {
    const a = aux.checkvector(L, 1);
    const b = aux.checkvector(L, 2);
    const c = cross3(a, b);
    api.pushvector(L, c[0], c[1], c[2]);
    return 1;
}

fn vector_dot(L: *State) c_int {
    const a = aux.checkvector(L, 1);
    const b = aux.checkvector(L, 2);
    api.pushnumber(L, dot3(a, b));
    return 1;
}

fn vector_angle(L: *State) c_int {
    const a = aux.checkvector(L, 1);
    const b = aux.checkvector(L, 2);
    const axis: ?*const [3]f32 = if (api.isnoneornil(L, 3)) null else aux.checkvector(L, 3);

    // cross(a, b)
    const cross = cross3(a, b);

    const sinA: f64 = @sqrt(@as(f64, cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]));
    const cosA: f64 = dot3(a, b);
    var angle = std.math.atan2(sinA, cosA);

    if (axis) |ax| {
        if (dot3(&cross, ax) < 0.0) angle = -angle;
    }

    api.pushnumber(L, angle);
    return 1;
}

fn vector_floor(L: *State) c_int {
    const v = aux.checkvector(L, 1);
    api.pushvector(L, @floor(v[0]), @floor(v[1]), @floor(v[2]));
    return 1;
}

fn vector_ceil(L: *State) c_int {
    const v = aux.checkvector(L, 1);
    api.pushvector(L, @ceil(v[0]), @ceil(v[1]), @ceil(v[2]));
    return 1;
}

fn vector_abs(L: *State) c_int {
    const v = aux.checkvector(L, 1);
    api.pushvector(L, @abs(v[0]), @abs(v[1]), @abs(v[2]));
    return 1;
}

inline fn signf(v: f32) f32 {
    return if (v > 0.0) 1.0 else if (v < 0.0) -1.0 else 0.0;
}

inline fn clampf(v: f32, min: f32, max: f32) f32 {
    const r = if (v < min) min else v;
    return if (r > max) max else r;
}

inline fn lerpf(a: f32, b: f32, t: f32) f32 {
    return if (t == 1.0) b else a + (b - a) * t;
}

fn vector_sign(L: *State) c_int {
    const v = aux.checkvector(L, 1);
    api.pushvector(L, signf(v[0]), signf(v[1]), signf(v[2]));
    return 1;
}

fn vector_clamp(L: *State) c_int {
    const v = aux.checkvector(L, 1);
    const min = aux.checkvector(L, 2);
    const max = aux.checkvector(L, 3);

    aux.argcheck(L, min[0] <= max[0], 3, "max.x must be greater than or equal to min.x");
    aux.argcheck(L, min[1] <= max[1], 3, "max.y must be greater than or equal to min.y");
    aux.argcheck(L, min[2] <= max[2], 3, "max.z must be greater than or equal to min.z");

    api.pushvector(L, clampf(v[0], min[0], max[0]), clampf(v[1], min[1], max[1]), clampf(v[2], min[2], max[2]));
    return 1;
}

fn vector_min(L: *State) c_int {
    const n = api.gettop(L);
    const v = aux.checkvector(L, 1);
    var result = v.*;
    var i: c_int = 2;
    while (i <= n) : (i += 1) {
        const b = aux.checkvector(L, i);
        for (0..3) |k| {
            if (b[k] < result[k]) result[k] = b[k];
        }
    }
    api.pushvector(L, result[0], result[1], result[2]);
    return 1;
}

fn vector_max(L: *State) c_int {
    const n = api.gettop(L);
    const v = aux.checkvector(L, 1);
    var result = v.*;
    var i: c_int = 2;
    while (i <= n) : (i += 1) {
        const b = aux.checkvector(L, i);
        for (0..3) |k| {
            if (b[k] > result[k]) result[k] = b[k];
        }
    }
    api.pushvector(L, result[0], result[1], result[2]);
    return 1;
}

fn vector_index(L: *State) c_int {
    const v = aux.checkvector(L, 1);
    const name = aux.checklstring(L, 2);

    // field access implementation mirrors the fast-path we have in the VM
    if (name.len == 1) {
        const ic: c_int = @as(c_int, name[0] | ' ') - 'x';
        if (ic >= 0 and ic < api.VECTOR_SIZE) {
            api.pushnumber(L, v[@intCast(ic)]);
            return 1;
        }
    }

    aux.errorf(L, "attempt to index vector with '{s}'", .{name});
}

fn vector_lerp(L: *State) c_int {
    const a = aux.checkvector(L, 1);
    const b = aux.checkvector(L, 2);
    const t: f32 = @floatCast(aux.checknumber(L, 3));
    api.pushvector(L, lerpf(a[0], b[0], t), lerpf(a[1], b[1], t), lerpf(a[2], b[2], t));
    return 1;
}

const vectorlib = [_]aux.Entry{
    aux.entry("create", vector_create),
    aux.entry("magnitude", vector_magnitude),
    aux.entry("normalize", vector_normalize),
    aux.entry("cross", vector_cross),
    aux.entry("dot", vector_dot),
    aux.entry("angle", vector_angle),
    aux.entry("floor", vector_floor),
    aux.entry("ceil", vector_ceil),
    aux.entry("abs", vector_abs),
    aux.entry("sign", vector_sign),
    aux.entry("clamp", vector_clamp),
    aux.entry("max", vector_max),
    aux.entry("min", vector_min),
    aux.entry("lerp", vector_lerp),
};

fn createmetatable(L: *State) void {
    api.createtable(L, 0, 1); // create metatable for vectors

    // push dummy vector
    api.pushvector(L, 0.0, 0.0, 0.0);

    api.pushvalue(L, -2);
    api.setmetatable(L, -2); // set vector metatable
    api.pop(L, 1); // pop dummy vector

    api.pushcfunction(L, aux.wrap(vector_index), null);
    api.setfield(L, -2, "__index");

    api.setreadonly(L, -1, true);
    api.pop(L, 1); // pop the metatable
}

pub fn open(L: *State) c_int {
    aux.register(L, "vector", &vectorlib);

    api.pushvector(L, 0.0, 0.0, 0.0);
    api.setfield(L, -2, "zero");
    api.pushvector(L, 1.0, 1.0, 1.0);
    api.setfield(L, -2, "one");

    createmetatable(L);
    return 1;
}
