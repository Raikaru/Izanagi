//! C-locale strtoull / strtoll (as used by lbaselib tonumber and luaO_str2l) on byte
//! slices. Results carry the C `endptr` as an offset; end == 0 means no conversion.

const std = @import("std");
const ctype = @import("ctype.zig");

pub const Result = struct { value: u64, end: usize };
pub const SResult = struct { value: i64, end: usize };

fn digitValue(c: u8) u32 {
    return switch (c) {
        '0'...'9' => c - '0',
        'a'...'z' => c - 'a' + 10,
        'A'...'Z' => c - 'A' + 10,
        else => 99,
    };
}

const Scan = struct { neg: bool, start: usize, end: usize, mag: u64, overflow: bool };

fn scan(s: []const u8, base: u32) ?Scan {
    var i: usize = 0;
    while (i < s.len and ctype.isspace(s[i])) i += 1;
    var neg = false;
    if (i < s.len and (s[i] == '+' or s[i] == '-')) {
        neg = s[i] == '-';
        i += 1;
    }
    if (base == 16 and i + 2 < s.len and s[i] == '0' and (s[i + 1] == 'x' or s[i + 1] == 'X') and digitValue(s[i + 2]) < 16)
        i += 2;
    const start = i;
    var acc: u64 = 0;
    var overflow = false;
    while (i < s.len) : (i += 1) {
        const d = digitValue(s[i]);
        if (d >= base) break;
        const m = @mulWithOverflow(acc, @as(u64, base));
        const a = @addWithOverflow(m[0], @as(u64, d));
        if (m[1] != 0 or a[1] != 0) overflow = true;
        acc = a[0];
    }
    if (i == start) return null;
    return .{ .neg = neg, .start = start, .end = i, .mag = acc, .overflow = overflow };
}

/// strtoull(s, &end, base), 2 <= base <= 36: negation wraps, overflow gives ULLONG_MAX.
pub fn strtoull(s: []const u8, base: u32) Result {
    const r = scan(s, base) orelse return .{ .value = 0, .end = 0 };
    if (r.overflow) return .{ .value = std.math.maxInt(u64), .end = r.end };
    return .{ .value = if (r.neg) 0 -% r.mag else r.mag, .end = r.end };
}

/// strtoll(s, &end, base): out-of-range values clamp to LLONG_MAX / LLONG_MIN.
pub fn strtoll(s: []const u8, base: u32) SResult {
    const r = scan(s, base) orelse return .{ .value = 0, .end = 0 };
    const limit: u64 = if (r.neg) @as(u64, 1) << 63 else std.math.maxInt(i64);
    if (r.overflow or r.mag > limit)
        return .{ .value = if (r.neg) std.math.minInt(i64) else std.math.maxInt(i64), .end = r.end };
    return .{ .value = if (r.neg) @bitCast(0 -% r.mag) else @intCast(r.mag), .end = r.end };
}

/// luaO_str2l: whole-string integer conversion (C string: ends at the first NUL).
pub fn str2l(full: []const u8, base: u32) ?i64 {
    const s = full[0 .. std.mem.indexOfScalar(u8, full, 0) orelse full.len];
    var result: i64 = undefined;
    var end: usize = undefined;
    if (base == 10) {
        const r = strtoll(s, 10);
        if (r.end == 0) return null; // conversion failed
        result = r.value;
        end = r.end;
        if (end < s.len and (s[end] == 'x' or s[end] == 'X')) { // maybe an hexadecimal constant?
            const h = strtoull(s, 16);
            result = @bitCast(h.value);
            end = h.end;
        }
    } else {
        // unsigned parse in other bases
        const r = strtoull(s, base);
        if (r.end == 0) return null;
        result = @bitCast(r.value);
        end = r.end;
    }
    while (end < s.len and ctype.isspace(s[end])) end += 1;
    if (end != s.len) return null; // invalid trailing characters?
    return result;
}
