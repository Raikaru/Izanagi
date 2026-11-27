//! Reusable pure-Zig Luau compiler, VM and standard libraries.
//!
//! The compiler's Output belongs to its caller; release it with Output.deinit.
//! `State` is the Zig-facing entry point: it owns the VM's allocator context and runs
//! source under protected calls, returning Zig errors and typed result views.
//! The raw `vm` C API uses Lua's non-local error unwinding. Calls that can raise must be
//! made under a protected call and must not cross Zig cleanup scopes.
//! On x86_64 the package's build graph selects LLVM: the pinned nightly's
//! self-hosted backend fails encoding VM SIMD instructions. Consumers
//! compiling this module themselves must select `-fllvm` or set
//! `.use_llvm = true` on their executable, library or test compile step.
//! The build also selects LLVM on Linux for other architectures. Selecting it
//! is not evidence of successful execution or conformance.
//!
//! Portions derived from Luau and Lua, distributed under the MIT license:
//!
//! Copyright (c) 2019-2026 Roblox Corporation
//! Copyright (c) 1994-2019 Lua.org, PUC-Rio.
//!
//! Permission is hereby granted, free of charge, to any person obtaining a copy
//! of this software and associated documentation files (the "Software"), to deal
//! in the Software without restriction, including without limitation the rights
//! to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//! copies of the Software, and to permit persons to whom the Software is
//! furnished to do so, subject to the following conditions:
//!
//! The above copyright notice and this permission notice shall be included in all
//! copies or substantial portions of the Software.
//!
//! THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//! IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//! FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//! AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//! LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//! OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//! SOFTWARE.

pub const compiler = @import("compiler.zig");
pub const vm = @import("vm.zig");
pub const require = @import("require.zig");
pub const stdlib = @import("stdlib.zig");
pub const State = @import("State.zig");

// The C exports are also available in the separately requested vm-abi archive.
comptime {
    const builtin = @import("builtin");
    if (builtin.zig_backend == .stage2_x86_64)
        @compileError("Luau VM requires LLVM on x86_64 with the pinned Zig nightly; use -fllvm or .use_llvm = true");
    _ = vm;
    _ = stdlib;
}

fn testAllocate(_: ?*anyopaque, ptr: ?*anyopaque, old_size: usize, new_size: usize) callconv(.c) ?*anyopaque {
    const allocator = @import("std").heap.page_allocator;
    const alignment: @import("std").mem.Alignment = .@"16";
    const address = @returnAddress();
    if (new_size == 0) {
        if (ptr) |p| allocator.rawFree(@as([*]u8, @ptrCast(p))[0..old_size], alignment, address);
        return null;
    }
    const p = ptr orelse return allocator.rawAlloc(new_size, alignment, address);
    const old = @as([*]u8, @ptrCast(p))[0..old_size];
    if (allocator.rawRemap(old, alignment, new_size, address)) |resized| return resized;
    const fresh = allocator.rawAlloc(new_size, alignment, address) orelse return null;
    @memcpy(fresh[0..@min(old_size, new_size)], old[0..@min(old_size, new_size)]);
    allocator.rawFree(old, alignment, address);
    return fresh;
}

test "compiled arithmetic executes in the VM" {
    const std = @import("std");
    _ = require;
    var result = try compiler.compile(std.testing.allocator, "return 1 + 2", .{});
    defer result.deinit(std.testing.allocator);
    try std.testing.expect(result.ok());
    const L = vm.lua_newstate(&testAllocate, null) orelse return error.OutOfMemory;
    defer vm.lua_close(L);
    try std.testing.expectEqual(@as(c_int, 0), vm.luau_load(L, "@arithmetic", result.bytecode.ptr, result.bytecode.len, 0));
    try std.testing.expectEqual(vm.LUA_OK, vm.lua_pcall(L, 0, 1, 0));
    try std.testing.expectEqual(@as(f64, 3), vm.lua_tonumber(L, -1));
}

test {
    _ = State;
}
