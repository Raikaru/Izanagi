//! Memory management: every VM allocation goes through the host `lua_Alloc` (or the
//! buffer cage allocator when LuauBufferCage is enabled), so the host allocator
//! observes and can cap the bytes used by the VM. Allocation failure raises
//! LUA_ERRMEM, and the `onallocate`/`onfree` callbacks are invoked like upstream
//! lmem.cpp.
//!
//! Difference from upstream: upstream suballocates small blocks from 16KB pages
//! (size classes), so the host allocator sees page-sized requests; here each block
//! is requested individually. `totalbytes`/`lua_totalbytes` count block sizes in
//! both implementations, but object layouts differ, so byte counts are not equal to
//! upstream's.

const std = @import("std");
const o = @import("object.zig");
const ldo = @import("do.zig");
const flags = @import("flags.zig");

const State = o.State;

inline fn notifyFree(L: *State, block: ?*anyopaque) void {
    if (block) |b| {
        if (L.global.cb.onfree) |cb| cb(L, b);
    }
}

inline fn notifyAlloc(L: *State, block: ?*anyopaque, osize: usize, nsize: usize, memcat: u8, tt: c_int, tag: c_int) void {
    if (L.global.cb.onallocate) |cb| cb(L, block, osize, nsize, memcat, tt, tag);
}

/// Raw reallocation. Returns null on failure (nsize > 0), never throws.
pub fn tryRealloc(L: *State, block: ?*anyopaque, osize: usize, nsize: usize, memcat: u8) ?*anyopaque {
    const g = L.global;
    notifyFree(L, block);
    const result = g.frealloc(g.ud, block, osize, nsize);
    if (result == null and nsize > 0) return null;
    g.totalbytes = g.totalbytes - osize + nsize;
    g.memcatbytes[memcat] = g.memcatbytes[memcat] -% osize +% nsize;
    if (nsize > 0) notifyAlloc(L, result, osize, nsize, memcat, o.T_ALL, 0);
    return result;
}

pub fn realloc(L: *State, block: ?*anyopaque, osize: usize, nsize: usize, memcat: u8) ?*anyopaque {
    const r = tryRealloc(L, block, osize, nsize, memcat);
    if (r == null and nsize > 0) ldo.throw(L, o.LUA_ERRMEM);
    return r;
}

pub fn alloc(L: *State, size: usize, memcat: u8) *anyopaque {
    return realloc(L, null, 0, size, memcat).?;
}

pub fn free(L: *State, block: ?*anyopaque, size: usize, memcat: u8) void {
    if (block == null) return;
    _ = tryRealloc(L, block, size, 0, memcat);
}

pub fn newArray(L: *State, comptime T: type, n: usize, memcat: u8) [*]T {
    if (n == 0) return @ptrFromInt(@alignOf(T));
    if (n > std.math.maxInt(usize) / @sizeOf(T)) toobig(L);
    return @ptrCast(@alignCast(alloc(L, n * @sizeOf(T), memcat)));
}

pub fn tryNewArray(L: *State, comptime T: type, n: usize, memcat: u8) ?[*]T {
    if (n == 0) return @ptrFromInt(@alignOf(T));
    if (n > std.math.maxInt(usize) / @sizeOf(T)) return null;
    const p = tryRealloc(L, null, 0, n * @sizeOf(T), memcat) orelse return null;
    return @ptrCast(@alignCast(p));
}

pub fn freeArray(L: *State, comptime T: type, p: [*]T, n: usize, memcat: u8) void {
    if (n == 0) return;
    free(L, @ptrCast(p), n * @sizeOf(T), memcat);
}

pub fn reallocArray(L: *State, comptime T: type, p: ?[*]T, oldn: usize, newn: usize, memcat: u8) [*]T {
    if (newn > std.math.maxInt(usize) / @sizeOf(T)) toobig(L);
    const old: ?*anyopaque = if (oldn == 0) null else @ptrCast(p.?);
    if (newn == 0) {
        free(L, old, oldn * @sizeOf(T), memcat);
        return @ptrFromInt(@alignOf(T));
    }
    const r = realloc(L, old, oldn * @sizeOf(T), newn * @sizeOf(T), memcat).?;
    return @ptrCast(@alignCast(r));
}

pub fn toobig(L: *State) noreturn {
    @import("debug.zig").runerror(L, "memory allocation error: block too big", .{});
}

fn linkGco(L: *State, block: *anyopaque, tt: u8) void {
    const g = L.global;
    const h: *o.GCObject = @ptrCast(@alignCast(block));
    h.* = .{ .tt = tt, .marked = g.currentwhite & 3, .memcat = L.activememcat, .pad = 0, .next = g.allgc };
    g.allgc = h;
}

/// Allocates a collectable object of `size` bytes, white, linked into the object list.
pub fn newGcoTagged(L: *State, comptime T: type, size: usize, tt: u8, tag: c_int) *T {
    const g = L.global;
    const memcat = L.activememcat;
    const block = g.frealloc(g.ud, null, 0, size) orelse ldo.throw(L, o.LUA_ERRMEM);
    g.totalbytes += size;
    g.memcatbytes[memcat] +%= size;
    notifyAlloc(L, block, 0, size, memcat, tt, tag);
    linkGco(L, block, tt);
    return @ptrCast(@alignCast(block));
}

pub fn newGco(L: *State, comptime T: type, size: usize, tt: u8) *T {
    return newGcoTagged(L, T, size, tt, 0);
}

/// Buffers use the cage allocator when LuauBufferCage is on and a cage is set.
pub fn newGcoBuffer(L: *State, comptime T: type, size: usize) *T {
    const g = L.global;
    if (flags.LuauBufferCage) {
        if (g.cagealloc) |cage| {
            const memcat = L.activememcat;
            const block = cage(g.cageud, null, 0, size, o.T_BUFFER) orelse ldo.throw(L, o.LUA_ERRMEM);
            g.totalbytes += size;
            g.memcatbytes[memcat] +%= size;
            notifyAlloc(L, block, 0, size, memcat, o.T_BUFFER, 0);
            linkGco(L, block, o.T_BUFFER);
            const h: *o.GCObject = @ptrCast(@alignCast(block));
            h.pad = 1; // allocated from the cage
            return @ptrCast(@alignCast(block));
        }
    }
    return newGco(L, T, size, o.T_BUFFER);
}

pub fn freeGco(L: *State, obj: *anyopaque, size: usize, memcat: u8) void {
    const g = L.global;
    notifyFree(L, obj);
    const h: *o.GCObject = @ptrCast(@alignCast(obj));
    if (h.tt == o.T_BUFFER and h.pad == 1) {
        _ = g.cagealloc.?(g.cageud, obj, size, 0, o.T_BUFFER);
    } else {
        _ = g.frealloc(g.ud, obj, size, 0);
    }
    g.totalbytes -= size;
    g.memcatbytes[memcat] -%= size;
}
