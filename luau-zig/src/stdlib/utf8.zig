//! UTF-8 library (upstream VM/src/lutf8lib.cpp), translated to Zig.
//!
//! Portions derived from Luau (MIT, Copyright (c) 2019-2025 Roblox Corporation) and
//! Lua 5.x (MIT, Copyright (C) 1994-2008 Lua.org, PUC-Rio).

const std = @import("std");
const api = @import("api.zig");
const aux = @import("auxiliary.zig");
const State = api.State;

const MAXUNICODE: u32 = 0x10FFFF;
const INT_MAX: i64 = std.math.maxInt(c_int);

/// Byte at index i, or the terminating '\0' at/after the end of a Luau string.
inline fn at(s: []const u8, i: usize) u8 {
    return if (i < s.len) s[i] else 0;
}

inline fn iscont(s: []const u8, i: usize) bool {
    return (at(s, i) & 0xC0) == 0x80;
}

/// translate a relative string position: negative means back from end
fn u_posrelat(pos: c_int, len: usize) c_int {
    if (pos >= 0) return pos;
    if (@as(u64, @intCast(-@as(i64, pos))) > len) return 0;
    return @intCast(@as(i64, @intCast(len)) + pos + 1);
}

/// Decode one UTF-8 sequence starting at s[i]; returns the index after it, or null if the
/// byte sequence is invalid.
fn utf8_decode(s: []const u8, i: usize, val: ?*u32) ?usize {
    const limits = [_]u32{ 0xFF, 0x7F, 0x7FF, 0xFFFF };
    var c: u32 = at(s, i);
    var res: u32 = 0; // final result
    var count: usize = 0; // to count number of continuation bytes
    if (c < 0x80) { // ascii?
        res = c;
    } else {
        while ((c & 0x40) != 0) { // still have continuation bytes?
            count += 1;
            const cc: u32 = at(s, i + count); // read next byte
            if ((cc & 0xC0) != 0x80) // not a continuation byte?
                return null; // invalid byte sequence
            res = (res << 6) | (cc & 0x3F); // add lower 6 bits from cont. byte
            c <<= 1; // to test next bit
        }
        if (count > 3) return null;
        res |= ((c & 0x7F) << @intCast(count * 5)); // add first byte
        if (res > MAXUNICODE or res <= limits[count])
            return null; // invalid byte sequence
        if (res -% 0xD800 < 0x800)
            return null; // surrogate
    }
    if (val) |v| v.* = res;
    return i + count + 1; // +1 to include first byte
}

/// utf8len(s [, i [, j]]) --> number of characters that start in the range [i,j], or nil +
/// current position if 's' is not well formed in that interval
fn utflen(L: *State) c_int {
    var n: c_int = 0;
    const s = aux.checklstring(L, 1);
    const len: c_int = @intCast(s.len);
    var posi = u_posrelat(aux.optinteger(L, 2, 1), s.len);
    var posj = u_posrelat(aux.optinteger(L, 3, -1), s.len);
    aux.argcheck(L, 1 <= posi and blk: {
        posi -= 1;
        break :blk posi <= len;
    }, 2, "initial position out of string");
    posj -= 1;
    aux.argcheck(L, posj < len, 3, "final position out of string");
    while (posi <= posj) {
        const s1 = utf8_decode(s, @intCast(posi), null) orelse {
            // conversion error?
            api.pushnil(L); // return nil ...
            api.pushinteger(L, posi + 1); // ... and current position
            return 2;
        };
        posi = @intCast(s1);
        n += 1;
    }
    api.pushinteger(L, n);
    return 1;
}

/// codepoint(s, [i, [j]])  -> returns codepoints for all characters that start in the range [i,j]
fn codepoint(L: *State) c_int {
    const s = aux.checklstring(L, 1);
    const posi = u_posrelat(aux.optinteger(L, 2, 1), s.len);
    const pose = u_posrelat(aux.optinteger(L, 3, posi), s.len);
    aux.argcheck(L, posi >= 1, 2, "out of range");
    aux.argcheck(L, pose <= @as(c_int, @intCast(s.len)), 3, "out of range");
    if (posi > pose) return 0; // empty interval; return no values
    if (@as(i64, pose) - posi >= INT_MAX) // (int -> int) overflow?
        aux.errorf(L, "string slice too long", .{});
    const n = pose - posi + 1;
    aux.checkstack(L, n, "string slice too long");
    var count: c_int = 0;
    const se: usize = @intCast(pose);
    var i: usize = @intCast(posi - 1);
    while (i < se) {
        var code: u32 = 0;
        i = utf8_decode(s, i, &code) orelse aux.errorf(L, "invalid UTF-8 code", .{});
        api.pushinteger(L, @intCast(code));
        count += 1;
    }
    return count;
}

const UTF8BUFFSZ = 8;

/// from Lua 5.3 lobject.c: encodes x backwards at the end of buff, returns byte count
fn luaO_utf8esc(buff: *[UTF8BUFFSZ]u8, x_: u32) usize {
    var x = x_;
    var n: usize = 1; // number of bytes put in buffer (backwards)
    std.debug.assert(x <= 0x10FFFF);
    if (x < 0x80) { // ascii?
        buff[UTF8BUFFSZ - 1] = @intCast(x);
    } else { // need continuation bytes
        var mfb: u32 = 0x3f; // maximum that fits in first byte
        while (true) { // add continuation bytes
            buff[UTF8BUFFSZ - n] = @intCast(0x80 | (x & 0x3f));
            n += 1;
            x >>= 6; // remove added bits
            mfb >>= 1; // now there is one less bit available in first byte
            if (!(x > mfb)) break; // still needs continuation byte?
        }
        buff[UTF8BUFFSZ - n] = @truncate((~mfb << 1) | x); // add first byte
    }
    return n;
}

/// lighter replacement for pushutfchar; doesn't push any string onto the stack
fn buffutfchar(L: *State, arg: c_int, buff: *[UTF8BUFFSZ]u8) []const u8 {
    const code = aux.checkinteger(L, arg);
    aux.argcheck(L, 0 <= code and code <= MAXUNICODE, arg, "value out of range");
    const l = luaO_utf8esc(buff, @intCast(code));
    return buff[UTF8BUFFSZ - l ..];
}

/// utfchar(n1, n2, ...)  -> char(n1)..char(n2)...
fn utfchar(L: *State) c_int {
    var buff: [UTF8BUFFSZ]u8 = undefined;
    const n = api.gettop(L); // number of arguments
    if (n == 1) { // optimize common case of single char
        api.pushlstring(L, buffutfchar(L, 1, &buff));
    } else {
        var b: aux.Strbuf = undefined;
        b.init(L);
        var i: c_int = 1;
        while (i <= n) : (i += 1) b.addlstring(buffutfchar(L, i, &buff));
        b.pushresult();
    }
    return 1;
}

/// offset(s, n, [i])  -> index where n-th character counting from position 'i' starts;
/// 0 means character at 'i'.
fn byteoffset(L: *State) c_int {
    const s = aux.checklstring(L, 1);
    const len: c_int = @intCast(s.len);
    var n = aux.checkinteger(L, 2);
    var posi: c_int = if (n >= 0) 1 else len + 1;
    posi = u_posrelat(aux.optinteger(L, 3, posi), s.len);
    aux.argcheck(L, 1 <= posi and blk: {
        posi -= 1;
        break :blk posi <= len;
    }, 3, "position out of range");
    if (n == 0) {
        // find beginning of current byte sequence
        while (posi > 0 and iscont(s, @intCast(posi))) posi -= 1;
    } else {
        if (iscont(s, @intCast(posi)))
            aux.errorf(L, "initial position is a continuation byte", .{});
        if (n < 0) {
            while (n < 0 and posi > 0) { // move back
                while (true) { // find beginning of previous character
                    posi -= 1;
                    if (!(posi > 0 and iscont(s, @intCast(posi)))) break;
                }
                n += 1;
            }
        } else {
            n -= 1; // do not move for 1st character
            while (n > 0 and posi < len) {
                while (true) { // find beginning of next character
                    posi += 1;
                    if (!iscont(s, @intCast(posi))) break; // (cannot pass final '\0')
                }
                n -= 1;
            }
        }
    }
    if (n == 0) { // did it find given character?
        api.pushinteger(L, posi + 1);
    } else { // no such character
        api.pushnil(L);
    }
    return 1;
}

fn iter_aux(L: *State) c_int {
    const s = aux.checklstring(L, 1);
    const len: c_int = @intCast(s.len);
    var n: c_int = api.tointeger(L, 2) -% 1;
    if (n < 0) { // first iteration?
        n = 0; // start from here
    } else if (n < len) {
        n += 1; // skip current byte
        while (iscont(s, @intCast(n))) n += 1; // and its continuations
    }
    if (n >= len) return 0; // no more codepoints
    var code: u32 = 0;
    const nxt = utf8_decode(s, @intCast(n), &code);
    if (nxt == null or iscont(s, nxt.?))
        aux.errorf(L, "invalid UTF-8 code", .{});
    api.pushinteger(L, n + 1);
    api.pushinteger(L, @intCast(code));
    return 2;
}

fn iter_codes(L: *State) c_int {
    _ = aux.checklstring(L, 1);
    api.pushcfunction(L, aux.wrap(iter_aux), null);
    api.pushvalue(L, 1);
    api.pushinteger(L, 0);
    return 3;
}

/// pattern to match a single UTF-8 character
const UTF8PATT = "[\x00-\x7F\xC2-\xF4][\x80-\xBF]*";

const funcs = [_]aux.Entry{
    aux.entry("offset", byteoffset),
    aux.entry("codepoint", codepoint),
    aux.entry("char", utfchar),
    aux.entry("len", utflen),
    aux.entry("codes", iter_codes),
};

pub fn open(L: *State) c_int {
    aux.register(L, "utf8", &funcs);
    api.pushlstring(L, UTF8PATT);
    api.setfield(L, -2, "charpattern");
    return 1;
}
