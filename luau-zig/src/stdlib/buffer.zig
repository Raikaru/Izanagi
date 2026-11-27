//! Buffer library (upstream VM/src/lbuflib.cpp), translated to Zig.
//!
//! Portions derived from Luau (MIT, Copyright (c) 2019-2025 Roblox Corporation).
//!
//! readinteger / writeinteger (64-bit integer type) are registered under the
//! LuauIntegerLibrary flag (stdlib/flags.zig). Data is stored little-endian on
//! every target, matching upstream's big-endian byte swapping.

const std = @import("std");
const api = @import("api.zig");
const aux = @import("auxiliary.zig");
const flags = @import("flags.zig");
const State = api.State;

/// Unsigned integer of the given width (element widths used by the buffer library).
fn UInt(comptime bits: comptime_int) type {
    return switch (bits) {
        8 => u8,
        16 => u16,
        32 => u32,
        64 => u64,
        else => @compileError("unsupported buffer element width"),
    };
}

// while C API returns 'size_t' for binary compatibility in case of future extensions,
// in the current implementation, length and offset are limited to 31 bits
// because offset is limited to an integer, a single 64bit comparison can be used and will not overflow
inline fn isoutofbounds(offset: c_int, len: usize, accesssize: u64) bool {
    return @as(u64, @as(c_uint, @bitCast(offset))) + accesssize > @as(u64, len);
}

fn buffer_create(L: *State) c_int {
    const size = aux.checkinteger(L, 1);
    aux.argcheck(L, size >= 0, 1, "size");
    _ = api.newbuffer(L, @intCast(size));
    return 1;
}

fn buffer_fromstring(L: *State) c_int {
    const val = aux.checklstring(L, 1);
    const data = api.newbuffer(L, val.len);
    @memcpy(data, val);
    return 1;
}

fn buffer_tostring(L: *State) c_int {
    const data = aux.checkbuffer(L, 1);
    api.pushlstring(L, data);
    return 1;
}

fn readinteger(comptime T: type) fn (*State) c_int {
    return struct {
        fn f(L: *State) c_int {
            const buf = aux.checkbuffer(L, 1);
            const offset = aux.checkinteger(L, 2);
            if (isoutofbounds(offset, buf.len, @sizeOf(T)))
                aux.errorf(L, "buffer access out of bounds", .{});
            const off: usize = @intCast(offset);
            const val = std.mem.readInt(T, buf[off..][0..@sizeOf(T)], .little);
            api.pushnumber(L, @floatFromInt(val));
            return 1;
        }
    }.f;
}

fn writeinteger(comptime T: type) fn (*State) c_int {
    return struct {
        fn f(L: *State) c_int {
            const buf = aux.checkbuffer(L, 1);
            const offset = aux.checkinteger(L, 2);
            const value = aux.checkunsigned(L, 3);
            if (isoutofbounds(offset, buf.len, @sizeOf(T)))
                aux.errorf(L, "buffer access out of bounds", .{});
            const off: usize = @intCast(offset);
            // T(value): truncation of the unsigned argument to the element width
            const U = UInt(@bitSizeOf(T));
            const val: T = @bitCast(@as(U, @truncate(value)));
            std.mem.writeInt(T, buf[off..][0..@sizeOf(T)], val, .little);
            return 0;
        }
    }.f;
}

fn readfp(comptime T: type) fn (*State) c_int {
    return struct {
        fn f(L: *State) c_int {
            const buf = aux.checkbuffer(L, 1);
            const offset = aux.checkinteger(L, 2);
            if (isoutofbounds(offset, buf.len, @sizeOf(T)))
                aux.errorf(L, "buffer access out of bounds", .{});
            const off: usize = @intCast(offset);
            const U = UInt(@bitSizeOf(T));
            const bits = std.mem.readInt(U, buf[off..][0..@sizeOf(T)], .little);
            const val: T = @bitCast(bits);
            api.pushnumber(L, @as(f64, val));
            return 1;
        }
    }.f;
}

fn writefp(comptime T: type) fn (*State) c_int {
    return struct {
        fn f(L: *State) c_int {
            const buf = aux.checkbuffer(L, 1);
            const offset = aux.checkinteger(L, 2);
            const value = aux.checknumber(L, 3);
            if (isoutofbounds(offset, buf.len, @sizeOf(T)))
                aux.errorf(L, "buffer access out of bounds", .{});
            const off: usize = @intCast(offset);
            const val: T = if (T == f64) aux.outf64(value) else aux.outf32(@floatCast(value));
            const U = UInt(@bitSizeOf(T));
            std.mem.writeInt(U, buf[off..][0..@sizeOf(T)], @bitCast(val), .little);
            return 0;
        }
    }.f;
}

fn buffer_readstring(L: *State) c_int {
    const buf = aux.checkbuffer(L, 1);
    const offset = aux.checkinteger(L, 2);
    const size = aux.checkinteger(L, 3);
    aux.argcheck(L, size >= 0, 3, "size");
    if (isoutofbounds(offset, buf.len, @as(c_uint, @intCast(size))))
        aux.errorf(L, "buffer access out of bounds", .{});
    const off: usize = @intCast(offset);
    api.pushlstring(L, buf[off..][0..@intCast(size)]);
    return 1;
}

fn buffer_writestring(L: *State) c_int {
    const buf = aux.checkbuffer(L, 1);
    const offset = aux.checkinteger(L, 2);
    const val = aux.checklstring(L, 3);
    const count = aux.optinteger(L, 4, @intCast(val.len));
    aux.argcheck(L, count >= 0, 4, "count");
    if (@as(usize, @intCast(count)) > val.len)
        aux.errorf(L, "string length overflow", .{});
    // string size can't exceed INT_MAX at this point
    if (isoutofbounds(offset, buf.len, @as(c_uint, @intCast(count))))
        aux.errorf(L, "buffer access out of bounds", .{});
    const off: usize = @intCast(offset);
    const n: usize = @intCast(count);
    @memcpy(buf[off..][0..n], val[0..n]);
    return 0;
}

fn buffer_len(L: *State) c_int {
    const buf = aux.checkbuffer(L, 1);
    api.pushnumber(L, @floatFromInt(@as(c_uint, @intCast(buf.len))));
    return 1;
}

fn buffer_copy(L: *State) c_int {
    const tbuf = aux.checkbuffer(L, 1);
    const toffset = aux.checkinteger(L, 2);
    const sbuf = aux.checkbuffer(L, 3);
    const soffset = aux.optinteger(L, 4, 0);
    const size = aux.optinteger(L, 5, @as(c_int, @intCast(sbuf.len)) -% soffset);

    if (size < 0)
        aux.errorf(L, "buffer access out of bounds", .{});
    if (isoutofbounds(soffset, sbuf.len, @as(c_uint, @intCast(size))))
        aux.errorf(L, "buffer access out of bounds", .{});
    if (isoutofbounds(toffset, tbuf.len, @as(c_uint, @intCast(size))))
        aux.errorf(L, "buffer access out of bounds", .{});

    const n: usize = @intCast(size);
    const src = sbuf[@intCast(soffset)..][0..n];
    const dst = tbuf[@intCast(toffset)..][0..n];
    // memmove: source and target may be the same buffer with overlapping ranges
    if (@intFromPtr(dst.ptr) <= @intFromPtr(src.ptr)) {
        std.mem.copyForwards(u8, dst, src);
    } else {
        std.mem.copyBackwards(u8, dst, src);
    }
    return 0;
}

fn buffer_fill(L: *State) c_int {
    const buf = aux.checkbuffer(L, 1);
    const offset = aux.checkinteger(L, 2);
    const value = aux.checkunsigned(L, 3);
    const size = aux.optinteger(L, 4, @as(c_int, @intCast(buf.len)) -% offset);

    if (size < 0)
        aux.errorf(L, "buffer access out of bounds", .{});
    if (isoutofbounds(offset, buf.len, @as(c_uint, @intCast(size))))
        aux.errorf(L, "buffer access out of bounds", .{});

    const off: usize = @intCast(offset);
    @memset(buf[off..][0..@intCast(size)], @truncate(value));
    return 0;
}

/// (int64_t)d with x86 cvttsd2si semantics for out-of-range values.
fn d2i64(d: f64) i64 {
    if (std.math.isNan(d) or d >= 9223372036854775808.0 or d < -9223372036854775808.0)
        return std.math.minInt(i64);
    return @intFromFloat(@trunc(d));
}

fn buffer_readbits(L: *State) c_int {
    const buf = aux.checkbuffer(L, 1);
    const bitoffset = d2i64(aux.checknumber(L, 2));
    const bitcount = aux.checkinteger(L, 3);

    if (bitoffset < 0)
        aux.errorf(L, "buffer access out of bounds", .{});
    if (@as(c_uint, @bitCast(bitcount)) > 32)
        aux.errorf(L, "bit count is out of range of [0; 32]", .{});
    if (@as(u64, @bitCast(bitoffset +% bitcount)) > @as(u64, buf.len) * 8)
        aux.errorf(L, "buffer access out of bounds", .{});

    const startbyte: usize = @intCast(@divTrunc(bitoffset, 8));
    const endbyte: usize = @intCast(@divTrunc(bitoffset + bitcount + 7, 8));

    var bytes: [8]u8 = @splat(0);
    @memcpy(bytes[0 .. endbyte - startbyte], buf[startbyte..endbyte]);
    const data = std.mem.readInt(u64, &bytes, .little);

    const subbyteoffset: u6 = @intCast(bitoffset & 0x7);
    const mask: u64 = (@as(u64, 1) << @intCast(bitcount)) - 1;

    api.pushunsigned(L, @truncate((data >> subbyteoffset) & mask));
    return 1;
}

fn buffer_writebits(L: *State) c_int {
    const buf = aux.checkbuffer(L, 1);
    const bitoffset = d2i64(aux.checknumber(L, 2));
    const bitcount = aux.checkinteger(L, 3);
    const value = aux.checkunsigned(L, 4);

    if (bitoffset < 0)
        aux.errorf(L, "buffer access out of bounds", .{});
    if (@as(c_uint, @bitCast(bitcount)) > 32)
        aux.errorf(L, "bit count is out of range of [0; 32]", .{});
    if (@as(u64, @bitCast(bitoffset +% bitcount)) > @as(u64, buf.len) * 8)
        aux.errorf(L, "buffer access out of bounds", .{});

    const startbyte: usize = @intCast(@divTrunc(bitoffset, 8));
    const endbyte: usize = @intCast(@divTrunc(bitoffset + bitcount + 7, 8));

    var bytes: [8]u8 = @splat(0);
    @memcpy(bytes[0 .. endbyte - startbyte], buf[startbyte..endbyte]);
    var data = std.mem.readInt(u64, &bytes, .little);

    const subbyteoffset: u6 = @intCast(bitoffset & 0x7);
    const mask: u64 = ((@as(u64, 1) << @intCast(bitcount)) - 1) << subbyteoffset;

    data = (data & ~mask) | ((@as(u64, value) << subbyteoffset) & mask);

    std.mem.writeInt(u64, &bytes, data, .little);
    @memcpy(buf[startbyte..endbyte], bytes[0 .. endbyte - startbyte]);
    return 0;
}

const bufferlib_NOINTEGER = [_]aux.Entry{
    aux.entry("create", buffer_create),
    aux.entry("fromstring", buffer_fromstring),
    aux.entry("tostring", buffer_tostring),
    aux.entry("readi8", readinteger(i8)),
    aux.entry("readu8", readinteger(u8)),
    aux.entry("readi16", readinteger(i16)),
    aux.entry("readu16", readinteger(u16)),
    aux.entry("readi32", readinteger(i32)),
    aux.entry("readu32", readinteger(u32)),
    aux.entry("readf32", readfp(f32)),
    aux.entry("readf64", readfp(f64)),
    aux.entry("writei8", writeinteger(i8)),
    aux.entry("writeu8", writeinteger(u8)),
    aux.entry("writei16", writeinteger(i16)),
    aux.entry("writeu16", writeinteger(u16)),
    aux.entry("writei32", writeinteger(i32)),
    aux.entry("writeu32", writeinteger(u32)),
    aux.entry("writef32", writefp(f32)),
    aux.entry("writef64", writefp(f64)),
    aux.entry("readstring", buffer_readstring),
    aux.entry("writestring", buffer_writestring),
    aux.entry("len", buffer_len),
    aux.entry("copy", buffer_copy),
    aux.entry("fill", buffer_fill),
    aux.entry("readbits", buffer_readbits),
    aux.entry("writebits", buffer_writebits),
};

fn buffer_readlong(L: *State) c_int {
    const buf = aux.checkbuffer(L, 1);
    const offset = aux.checkinteger(L, 2);
    if (isoutofbounds(offset, buf.len, @sizeOf(i64)))
        aux.errorf(L, "buffer access out of bounds", .{});
    const off: usize = @intCast(offset);
    api.pushinteger64(L, std.mem.readInt(i64, buf[off..][0..8], .little));
    return 1;
}

fn buffer_writelong(L: *State) c_int {
    const buf = aux.checkbuffer(L, 1);
    const offset = aux.checkinteger(L, 2);
    const value = aux.checkinteger64(L, 3);
    if (isoutofbounds(offset, buf.len, @sizeOf(i64)))
        aux.errorf(L, "buffer access out of bounds", .{});
    const off: usize = @intCast(offset);
    std.mem.writeInt(i64, buf[off..][0..8], value, .little);
    return 0;
}

/// LuauIntegerLibrary: same table plus readinteger/writeinteger for the 64-bit integer type.
const bufferlib = bufferlib_NOINTEGER ++ [_]aux.Entry{
    aux.entry("readinteger", buffer_readlong),
    aux.entry("writeinteger", buffer_writelong),
};

pub fn open(L: *State) c_int {
    if (flags.LuauIntegerLibrary) {
        aux.register(L, "buffer", &bufferlib);
    } else {
        aux.register(L, "buffer", &bufferlib_NOINTEGER);
    }
    return 1;
}
