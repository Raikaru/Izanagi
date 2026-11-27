//! Luau VM string hash (upstream lstring.cpp, MIT; see object.zig header).
const std = @import("std");

pub fn hash(str: []const u8) u32 {
    var a: u32 = 0;
    var b: u32 = 0;
    var h: u32 = @truncate(str.len);
    var s = str;
    while (s.len >= 32) {
        a +%= std.mem.readInt(u32, s[0..4], .little);
        b +%= std.mem.readInt(u32, s[4..8], .little);
        h +%= std.mem.readInt(u32, s[8..12], .little);
        // mix(14, 11, 25)
        a ^= h;
        a -%= std.math.rotr(u32, h, 14);
        b ^= a;
        b -%= std.math.rotr(u32, a, 11);
        h ^= b;
        h -%= std.math.rotr(u32, b, 25);
        s = s[12..];
    }
    var i: usize = s.len;
    while (i > 0) : (i -= 1) {
        h ^= (h << 5) +% (h >> 2) +% @as(u32, s[i - 1]);
    }
    return h;
}
