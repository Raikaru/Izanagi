test {
    _ = @import("vm/fastmath.zig");
}

const std = @import("std");
const vm = @import("vm.zig");
const o = @import("vm/object.zig");
const fast = @import("vm/fastmath.zig");
const State = @import("State.zig");

const Case = struct { id: u8, lib: [:0]const u8, name: [:0]const u8, argc: u8 };
const cases = [_]Case{
    .{ .id = 2, .lib = "math", .name = "abs", .argc = 1 },
    .{ .id = 3, .lib = "math", .name = "acos", .argc = 1 },
    .{ .id = 4, .lib = "math", .name = "asin", .argc = 1 },
    .{ .id = 5, .lib = "math", .name = "atan2", .argc = 2 },
    .{ .id = 6, .lib = "math", .name = "atan", .argc = 1 },
    .{ .id = 7, .lib = "math", .name = "ceil", .argc = 1 },
    .{ .id = 8, .lib = "math", .name = "cosh", .argc = 1 },
    .{ .id = 9, .lib = "math", .name = "cos", .argc = 1 },
    .{ .id = 10, .lib = "math", .name = "deg", .argc = 1 },
    .{ .id = 11, .lib = "math", .name = "exp", .argc = 1 },
    .{ .id = 12, .lib = "math", .name = "floor", .argc = 1 },
    .{ .id = 13, .lib = "math", .name = "fmod", .argc = 2 },
    .{ .id = 14, .lib = "math", .name = "frexp", .argc = 1 },
    .{ .id = 15, .lib = "math", .name = "ldexp", .argc = 2 },
    .{ .id = 16, .lib = "math", .name = "log10", .argc = 1 },
    .{ .id = 17, .lib = "math", .name = "log", .argc = 1 },
    .{ .id = 18, .lib = "math", .name = "max", .argc = 2 },
    .{ .id = 19, .lib = "math", .name = "min", .argc = 2 },
    .{ .id = 20, .lib = "math", .name = "modf", .argc = 1 },
    .{ .id = 21, .lib = "math", .name = "pow", .argc = 2 },
    .{ .id = 22, .lib = "math", .name = "rad", .argc = 1 },
    .{ .id = 23, .lib = "math", .name = "sinh", .argc = 1 },
    .{ .id = 24, .lib = "math", .name = "sin", .argc = 1 },
    .{ .id = 25, .lib = "math", .name = "sqrt", .argc = 1 },
    .{ .id = 26, .lib = "math", .name = "tanh", .argc = 1 },
    .{ .id = 27, .lib = "math", .name = "tan", .argc = 1 },
    .{ .id = 28, .lib = "bit32", .name = "arshift", .argc = 2 },
    .{ .id = 29, .lib = "bit32", .name = "band", .argc = 2 },
    .{ .id = 30, .lib = "bit32", .name = "bnot", .argc = 1 },
    .{ .id = 31, .lib = "bit32", .name = "bor", .argc = 2 },
    .{ .id = 32, .lib = "bit32", .name = "bxor", .argc = 2 },
    .{ .id = 33, .lib = "bit32", .name = "btest", .argc = 2 },
    .{ .id = 34, .lib = "bit32", .name = "extract", .argc = 3 },
    .{ .id = 35, .lib = "bit32", .name = "lrotate", .argc = 2 },
    .{ .id = 36, .lib = "bit32", .name = "lshift", .argc = 2 },
    .{ .id = 37, .lib = "bit32", .name = "replace", .argc = 3 },
    .{ .id = 38, .lib = "bit32", .name = "rrotate", .argc = 2 },
    .{ .id = 39, .lib = "bit32", .name = "rshift", .argc = 2 },
    .{ .id = 46, .lib = "math", .name = "clamp", .argc = 3 },
    .{ .id = 47, .lib = "math", .name = "sign", .argc = 1 },
    .{ .id = 48, .lib = "math", .name = "round", .argc = 1 },
    .{ .id = 55, .lib = "bit32", .name = "countlz", .argc = 1 },
    .{ .id = 56, .lib = "bit32", .name = "countrz", .argc = 1 },
    .{ .id = 59, .lib = "bit32", .name = "extract", .argc = 3 },
    .{ .id = 64, .lib = "bit32", .name = "byteswap", .argc = 1 },
    .{ .id = 89, .lib = "math", .name = "lerp", .argc = 3 },
    .{ .id = 91, .lib = "math", .name = "isnan", .argc = 1 },
    .{ .id = 92, .lib = "math", .name = "isinf", .argc = 1 },
    .{ .id = 93, .lib = "math", .name = "isfinite", .argc = 1 },
};

test "dense real-library versus fastcall bit parity" {
    try parityWitness(false);
}

pub fn parityWitness(comptime trace: bool) !void {
    var state = try State.init(std.testing.allocator, .{});
    defer state.deinit();
    const L = state.L;
    const edges = [_]u64{ 0, 0x8000000000000000, 1, 0x8000000000000001, 0x7ff0000000000000, 0xfff0000000000000, 0x7ff8000000001234, 0xfff8000000005678, 0x41e0000000000000, 0xc1e0000000000000, 0x41f0000000000000, 0x43e0000000000000 };
    var dense_seed: u32 = 0x2345abcd;
    for (cases) |item| {
        var seed: u64 = 0x8d26c5ab91d37e4f;
        for (0..10009) |i| {
            var x: f64 = undefined;
            if (i == 0) {
                x = 4;
            } else if (i <= 8) {
                dense_seed = dense_seed *% 1664525 +% 1013904223;
                const bitwise = (item.id >= 28 and item.id <= 39) or item.id == 55 or item.id == 56 or item.id == 59 or item.id == 64;
                const value: i32 = if (bitwise) @as(i32, @intCast(dense_seed % 2049)) - 1024 else @as(i32, @intCast(dense_seed % 19001)) - 9500;
                x = @floatFromInt(value);
                if (!bitwise) x /= 10000;
            } else {
                seed = seed *% 6364136223846793005 +% 1442695040888963407;
                x = @bitCast(if (i - 9 < edges.len) edges[i - 9] else seed);
            }
            var inputs = [_]o.TValue{ .{}, .{}, .{} };
            var results = [_]o.TValue{ .{}, .{} };
            o.setnvalue(&inputs[0], x);
            o.setnvalue(&inputs[1], if (item.id == 59) 33 else 2);
            o.setnvalue(&inputs[2], 3);
            const result_count: c_int = if (item.id == 14 or item.id == 20) 2 else 1;
            const fast_argc: c_int = if (item.id == 59) 2 else item.argc;
            const got = fast.run(L, item.id, &results, &inputs[0], inputs[1..].ptr, fast_argc, result_count);
            if (got == null) {
                std.debug.print("unexpected fallback: id={d} sample={d}\n", .{ item.id, i });
                return error.UnexpectedFallback;
            }
            vm.lua_settop(L, 0);
            _ = vm.lua_getfield(L, vm.LUA_GLOBALSINDEX, item.lib);
            _ = vm.lua_getfield(L, -1, item.name);
            vm.lua_pushnumber(L, x);
            if (item.argc >= 2) vm.lua_pushnumber(L, if (item.id == 59) 1 else 2);
            if (item.argc >= 3) vm.lua_pushnumber(L, if (item.id == 59) 2 else 3);
            const status = vm.lua_pcall(L, item.argc, result_count, 0);
            if (status != vm.LUA_OK) {
                std.debug.print("slow call failed: id={d} sample={d} status={d}\n", .{ item.id, i, status });
                return error.SlowCallFailed;
            }
            for (0..@intCast(result_count)) |j| {
                const index: c_int = @intCast(j + 2);
                if (trace) {
                    const bits: u64 = if (results[j].tt == o.T_BOOLEAN) @intFromBool(results[j].value.b != 0) else @bitCast(results[j].value.n);
                    std.debug.print("M {d} {d} {d} {x:0>16}\n", .{ item.id, i, j, bits });
                }
                if (results[j].tt == o.T_BOOLEAN) {
                    if ((results[j].value.b != 0) != (vm.lua_toboolean(L, index) != 0))
                        return error.BooleanMismatch;
                } else {
                    const a: u64 = @bitCast(results[j].value.n);
                    const b: u64 = @bitCast(vm.lua_tonumber(L, index));
                    if (a != b) {
                        std.debug.print("parity mismatch: id={d} sample={d} result={d} fast={x} slow={x}\n", .{ item.id, i, j, a, b });
                        return error.NumberMismatch;
                    }
                }
            }
        }
        std.debug.print("id={d} parity=10009\n", .{item.id});
    }
}
