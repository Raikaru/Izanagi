//! Keep Zig safety failures fatal and visible through the host's existing stderr.
const std = @import("std");
const builtin = @import("builtin");

extern fn fprintf(stream: ?*anyopaque, format: [*:0]const u8, ...) c_int;
extern fn abort() noreturn;
extern fn __acrt_iob_func(index: c_uint) ?*anyopaque;
extern var __stderrp: ?*anyopaque;
extern var stderr: ?*anyopaque;

fn fail(message: []const u8, address: ?usize) noreturn {
    const stream = if (builtin.os.tag == .windows) __acrt_iob_func(2) else if (builtin.os.tag.isDarwin()) __stderrp else stderr;
    const length: c_int = @intCast(@min(message.len, std.math.maxInt(c_int)));
    _ = fprintf(stream, "p4pc: Zig panic: %.*s (return address 0x%zx)\n", length, message.ptr, address orelse 0);
    abort();
}

pub const panic = std.debug.FullPanic(fail);
