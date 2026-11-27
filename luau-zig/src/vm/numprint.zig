//! Number <-> string conversion matching upstream Luau exactly.
//!
//! `num2str` is a port of upstream lnumprint.cpp (Schubfach shortest round-trip
//! formatting, MIT; see object.zig header); `str2d` reproduces luaO_str2d on top of
//! the C `strtod` grammar (decimal, hexadecimal, inf/nan, surrounding whitespace).

const std = @import("std");

pub const MAXNUM2STR = 48;

const kPow5Table = [16]u64{
    0x8000000000000000,
    0xa000000000000000,
    0xc800000000000000,
    0xfa00000000000000,
    0x9c40000000000000,
    0xc350000000000000,
    0xf424000000000000,
    0x9896800000000000,
    0xbebc200000000000,
    0xee6b280000000000,
    0x9502f90000000000,
    0xba43b74000000000,
    0xe8d4a51000000000,
    0x9184e72a00000000,
    0xb5e620f480000000,
    0xe35fa931a0000000,
};
const kPow10Table = [39][3]u64{
    .{ 0xff77b1fcbebcdc4f, 0x25e8e89c13bb0f7b, 0x333443443333443b },
    .{ 0x8dd01fad907ffc3b, 0xae3da7d97f6792e4, 0xbbb3ab3cb3ba3cbc },
    .{ 0x9d71ac8fada6c9b5, 0x6f773fc3603db4aa, 0x4ba4bc4bb4bb4bcc },
    .{ 0xaecc49914078536d, 0x58fae9f773886e19, 0x3ba3bc33b43b43bb },
    .{ 0xc21094364dfb5636, 0x985915fc12f542e5, 0x33b43b43a33b33cb },
    .{ 0xd77485cb25823ac7, 0x7d633293366b828c, 0x34b44c444343443c },
    .{ 0xef340a98172aace4, 0x86fb897116c87c35, 0x333343333343334b },
    .{ 0x84c8d4dfd2c63f3b, 0x29ecd9f40041e074, 0xccaccbbcbcbb4bbc },
    .{ 0x936b9fcebb25c995, 0xcab10dd900beec35, 0x3ab3ab3ab3bb3bbb },
    .{ 0xa3ab66580d5fdaf5, 0xc13e60d0d2e0ebbb, 0x4cc3dc4db4db4dbb },
    .{ 0xb5b5ada8aaff80b8, 0x0d819992132456bb, 0x33b33a34c33b34ab },
    .{ 0xc9bcff6034c13052, 0xfc89b393dd02f0b6, 0x33c33b44b43c34bc },
    .{ 0xdff9772470297ebd, 0x59787e2b93bc56f8, 0x43b444444443434c },
    .{ 0xf8a95fcf88747d94, 0x75a44c6397ce912b, 0x443334343443343b },
    .{ 0x8a08f0f8bf0f156b, 0x1b8e9ecb641b5900, 0xbbabab3aa3ab4ccc },
    .{ 0x993fe2c6d07b7fab, 0xe546a8038efe402a, 0x4cb4bc4db4db4bcc },
    .{ 0xaa242499697392d2, 0xdde50bd1d5d0b9ea, 0x3ba3ba3bb33b33bc },
    .{ 0xbce5086492111aea, 0x88f4bb1ca6bcf585, 0x44b44c44c44c43cb },
    .{ 0xd1b71758e219652b, 0xd3c36113404ea4a9, 0x44c44c44c444443b },
    .{ 0xe8d4a51000000000, 0x0000000000000000, 0x444444444444444c },
    .{ 0x813f3978f8940984, 0x4000000000000000, 0xcccccccccccccccc },
    .{ 0x8f7e32ce7bea5c6f, 0xe4820023a2000000, 0xbba3bc4cc4cc4ccc },
    .{ 0x9f4f2726179a2245, 0x01d762422c946591, 0x4aa3bb3aa3ba3bab },
    .{ 0xb0de65388cc8ada8, 0x3b25a55f43294bcc, 0x3ca33b33b44b43bc },
    .{ 0xc45d1df942711d9a, 0x3ba5d0bd324f8395, 0x44c44c34c44b44cb },
    .{ 0xda01ee641a708de9, 0xe80e6f4820cc9496, 0x33b33b343333333c },
    .{ 0xf209787bb47d6b84, 0xc0678c5dbd23a49b, 0x443444444443443b },
    .{ 0x865b86925b9bc5c2, 0x0b8a2392ba45a9b3, 0xdbccbcccb4cb3bbb },
    .{ 0x952ab45cfa97a0b2, 0xdd945a747bf26184, 0x3bc4bb4ab3ca3cbc },
    .{ 0xa59bc234db398c25, 0x43fab9837e699096, 0x3bb3ac3ab3bb33ac },
    .{ 0xb7dcbf5354e9bece, 0x0c11ed6d538aeb30, 0x33b43b43b34c34dc },
    .{ 0xcc20ce9bd35c78a5, 0x31ec038df7b441f5, 0x34c44c43c44b44cb },
    .{ 0xe2a0b5dc971f303a, 0x2e44ae64840fd61e, 0x333333333333333c },
    .{ 0xfb9b7cd9a4a7443c, 0x169840ef017da3b2, 0x433344443333344c },
    .{ 0x8bab8eefb6409c1a, 0x1ad089b6c2f7548f, 0xdcbdcc3cc4cc4bcb },
    .{ 0x9b10a4e5e9913128, 0xca7cf2b4191c8327, 0x3ab3cb3bc3bb4bbb },
    .{ 0xac2820d9623bf429, 0x546345fa9fbdcd45, 0x3bb3cc43c43c43cb },
    .{ 0xbf21e44003acdd2c, 0xe0470a63e6bd56c4, 0x44b34a43b44c44bc },
    .{ 0xd433179d9c8cb841, 0x5fa60692a46151ec, 0x43a33a33a333333c },
};

const kDigitTable = "00010203040506070809101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899";

const kPow10TableMin = -292;

inline fn mul128(x: u64, y: u64, hi: *u64) u64 {
    const r: u128 = @as(u128, x) * y;
    hi.* = @truncate(r >> 64);
    return @truncate(r);
}

inline fn mul192hi(xhi: u64, xlo: u64, y: u64, hi: *u64) u64 {
    var z2: u64 = undefined;
    var z1 = mul128(xhi, y, &z2);
    var z1c: u64 = undefined;
    _ = mul128(xlo, y, &z1c);
    z1 +%= z1c;
    z2 +%= @intFromBool(z1 < z1c);
    hi.* = z2;
    return z1;
}

inline fn roundodd(ghi: u64, glo: u64, cp: u64) u64 {
    var xhi: u64 = undefined;
    _ = mul128(glo, cp, &xhi);
    var yhi: u64 = undefined;
    const ylo = mul128(ghi, cp, &yhi);
    const z = ylo +% xhi;
    return (yhi +% @intFromBool(z < xhi)) | @intFromBool(z > 1);
}

const Decimal = struct { s: u64, k: i32 };

fn schubfach(exponent: i32, fraction: u64) Decimal {
    var c = fraction;
    var q: i32 = exponent - 1023 - 51;
    if (exponent != 0) {
        c |= (@as(u64, 1) << 52);
        q -= 1;
    }
    if (q <= 0 and -q < 53) {
        const sh: u6 = @intCast(-q);
        if ((c & ((@as(u64, 1) << sh) - 1)) == 0) return .{ .s = c >> sh, .k = 0 };
    }
    const irr: u64 = @intFromBool(c == (@as(u64, 1) << 52) and q != -1074);
    const out: u64 = c & 1;
    const cbl: u64 = 4 * c - 2 + irr;
    const cb: u64 = 4 * c;
    const cbr: u64 = 4 * c + 2;
    const Q = 20;
    const C: i32 = 315652;
    const A: i32 = -131008;
    const C2: i32 = 3483294;
    const k: i32 = (q * C + (if (irr != 0) A else 0)) >> Q;
    const h: i32 = q + ((-k * C2) >> Q) + 1;
    const gtoff: usize = @intCast(-k - kPow10TableMin);
    const gt = kPow10Table[gtoff >> 4];
    var ghi: u64 = undefined;
    var glo = mul192hi(gt[0], gt[1], kPow5Table[gtoff & 15], &ghi);
    const gterr: u64 = (gt[2] >> @intCast((gtoff & 15) * 4)) & 15;
    const gtscale: u6 = @intCast(gterr >> 3);
    ghi <<= gtscale;
    ghi +%= (glo >> 63) & gtscale;
    glo <<= gtscale;
    glo -%= (gterr & 7) -% 4;
    const hs: u6 = @intCast(h);
    const vbl = roundodd(ghi, glo, cbl << hs);
    const vb = roundodd(ghi, glo, cb << hs);
    const vbr = roundodd(ghi, glo, cbr << hs);
    const s = vb / 4;
    if (s >= 10) {
        const sp = s / 10;
        const upin = vbl + out <= 40 * sp;
        const wpin = vbr >= 40 * sp + 40 + out;
        if (upin != wpin) return .{ .s = sp + @intFromBool(wpin), .k = k + 1 };
    }
    const uin = vbl + out <= 4 * s;
    const win = 4 * s + 4 + out <= vbr;
    const rup = vb >= 4 * s + 2 + 1 - (s & 1);
    return .{ .s = s + @intFromBool(if (uin != win) win else rup), .k = k };
}

fn printunsignedrev(buf: []u8, end_in: usize, num_in: u64) usize {
    var end = end_in;
    var num = num_in;
    while (num >= 10000) {
        const tail: usize = @intCast(num % 10000);
        @memcpy(buf[end - 4 .. end - 2], kDigitTable[(tail / 100) * 2 ..][0..2]);
        @memcpy(buf[end - 2 .. end], kDigitTable[(tail % 100) * 2 ..][0..2]);
        num /= 10000;
        end -= 4;
    }
    var rest: usize = @intCast(num);
    while (rest >= 10) {
        @memcpy(buf[end - 2 .. end], kDigitTable[(rest % 100) * 2 ..][0..2]);
        rest /= 100;
        end -= 2;
    }
    if (rest != 0) {
        buf[end - 1] = '0' + @as(u8, @intCast(rest));
        end -= 1;
    }
    return end;
}

fn trimzero(buf: []u8, end_in: usize) usize {
    var end = end_in;
    while (buf[end - 1] == '0') end -= 1;
    return end;
}

fn printexp(buf: []u8, pos_in: usize, num: i32) usize {
    var pos = pos_in;
    buf[pos] = 'e';
    buf[pos + 1] = if (num < 0) '-' else '+';
    pos += 2;
    var v: usize = @intCast(if (num < 0) -num else num);
    if (v >= 100) {
        buf[pos] = '0' + @as(u8, @intCast(v / 100));
        pos += 1;
        v %= 100;
    }
    @memcpy(buf[pos .. pos + 2], kDigitTable[v * 2 ..][0..2]);
    return pos + 2;
}

/// luai_num2str: formats `n` exactly like upstream (`tostring(n)`).
pub fn num2str(out: *[MAXNUM2STR]u8, n: f64) []u8 {
    const bits: u64 = @bitCast(n);
    const sign: usize = @intCast(bits >> 63);
    const exponent: i32 = @intCast((bits >> 52) & 2047);
    const fraction: u64 = bits & ((@as(u64, 1) << 52) - 1);
    if (exponent == 0x7ff) {
        if (fraction == 0) {
            const s: []const u8 = if (sign != 0) "-inf" else "inf";
            @memcpy(out[0..s.len], s);
            return out[0..s.len];
        }
        @memcpy(out[0..3], "nan");
        return out[0..3];
    }
    out[0] = '-';
    const buf = out[sign..];
    if (exponent == 0 and fraction == 0) {
        buf[0] = '0';
        return out[0 .. sign + 1];
    }
    const d = schubfach(exponent, fraction);
    var decbuf: [40]u8 = undefined;
    const decend: usize = 20;
    const decstart = printunsignedrev(&decbuf, decend, d.s);
    const dec = decbuf[decstart..decend];
    const declen: i32 = @intCast(dec.len);
    const dot: i32 = declen + d.k;
    var end: usize = 0;
    if (dot >= -5 and dot <= 21) {
        if (dot <= 0) {
            buf[0] = '0';
            buf[1] = '.';
            const z: usize = @intCast(-dot);
            @memset(buf[2 .. 2 + z], '0');
            @memcpy(buf[2 + z .. 2 + z + dec.len], dec);
            end = trimzero(buf, 2 + z + dec.len);
        } else if (dot == declen) {
            @memcpy(buf[0..dec.len], dec);
            end = dec.len;
        } else if (dot < declen) {
            const ud: usize = @intCast(dot);
            @memcpy(buf[0..ud], dec[0..ud]);
            buf[ud] = '.';
            @memcpy(buf[ud + 1 .. dec.len + 1], dec[ud..]);
            end = trimzero(buf, dec.len + 1);
        } else {
            const ud: usize = @intCast(dot);
            @memcpy(buf[0..dec.len], dec);
            @memset(buf[dec.len..ud], '0');
            end = ud;
        }
    } else {
        buf[0] = dec[0];
        buf[1] = '.';
        @memcpy(buf[2 .. 2 + dec.len - 1], dec[1..]);
        var e = trimzero(buf, dec.len + 1);
        if (buf[e - 1] == '.') e -= 1;
        end = printexp(buf, e, dot - 1);
    }
    return out[0 .. sign + end];
}

/// luai_int2str
pub fn int2str(out: *[32]u8, l: i64) []u8 {
    var val: u64 = if (l < 0) ~@as(u64, @bitCast(l)) +% 1 else @bitCast(l);
    var tmp: [24]u8 = undefined;
    var pos: usize = tmp.len;
    while (true) {
        pos -= 1;
        tmp[pos] = '0' + @as(u8, @intCast(val % 10));
        val /= 10;
        if (val == 0) break;
    }
    var w: usize = 0;
    if (l < 0) {
        out[0] = '-';
        w = 1;
    }
    const digits = tmp[pos..];
    @memcpy(out[w .. w + digits.len], digits);
    return out[0 .. w + digits.len];
}

inline fn isspace(c: u8) bool {
    return c == ' ' or (c >= '\t' and c <= '\r');
}

fn ieq(s: []const u8, i: usize, word: []const u8) bool {
    if (s.len - i < word.len) return false;
    for (word, 0..) |c, j| {
        if (std.ascii.toLower(s[i + j]) != c) return false;
    }
    return true;
}

/// A converted number and the offset just past the text it was parsed from.
const Parsed = struct { value: f64, end: usize };

/// Parses the longest prefix accepted by C `strtod`; null when nothing was converted.
pub fn strtod(s: []const u8) ?Parsed {
    var i: usize = 0;
    while (i < s.len and isspace(s[i])) i += 1;
    var neg = false;
    if (i < s.len and (s[i] == '+' or s[i] == '-')) {
        neg = s[i] == '-';
        i += 1;
    }
    // inf / infinity / nan
    if (ieq(s, i, "inf")) {
        const e = if (ieq(s, i, "infinity")) i + 8 else i + 3;
        return .{ .value = if (neg) -std.math.inf(f64) else std.math.inf(f64), .end = e };
    }
    if (ieq(s, i, "nan")) {
        var e = i + 3;
        if (e < s.len and s[e] == '(') {
            var j = e + 1;
            while (j < s.len and (std.ascii.isAlphanumeric(s[j]) or s[j] == '_')) j += 1;
            if (j < s.len and s[j] == ')') e = j + 1;
        }
        return .{ .value = std.math.nan(f64), .end = e };
    }
    var buf: [512]u8 = undefined;
    // hexadecimal
    if (i + 1 < s.len and s[i] == '0' and (s[i + 1] == 'x' or s[i + 1] == 'X')) {
        var j = i + 2;
        var nd: usize = 0;
        while (j < s.len and std.ascii.isHex(s[j])) : (j += 1) nd += 1;
        if (j < s.len and s[j] == '.') {
            j += 1;
            while (j < s.len and std.ascii.isHex(s[j])) : (j += 1) nd += 1;
        }
        if (nd == 0) {
            // only "0" is converted; endptr points at 'x'
            return .{ .value = if (neg) -0.0 else 0.0, .end = i + 1 };
        }
        var e = j;
        if (e < s.len and (s[e] == 'p' or s[e] == 'P')) {
            var k = e + 1;
            if (k < s.len and (s[k] == '+' or s[k] == '-')) k += 1;
            if (k < s.len and std.ascii.isDigit(s[k])) {
                while (k < s.len and std.ascii.isDigit(s[k])) k += 1;
                e = k;
            }
        }
        const body = s[i..e];
        const hasexp = e != j;
        const v = if (body.len + 3 <= buf.len) blk: {
            @memcpy(buf[0..body.len], body);
            var len = body.len;
            if (!hasexp) {
                @memcpy(buf[len .. len + 2], "p0");
                len += 2;
            }
            break :blk parseHexFallback(buf[0..len]);
        } else parseHexFallback(body);
        return .{ .value = if (neg) -v else v, .end = e };
    }
    // decimal
    var j = i;
    var nd: usize = 0;
    while (j < s.len and std.ascii.isDigit(s[j])) : (j += 1) nd += 1;
    if (j < s.len and s[j] == '.') {
        j += 1;
        while (j < s.len and std.ascii.isDigit(s[j])) : (j += 1) nd += 1;
    }
    if (nd == 0) return null;
    var e = j;
    if (e < s.len and (s[e] == 'e' or s[e] == 'E')) {
        var k = e + 1;
        if (k < s.len and (s[k] == '+' or s[k] == '-')) k += 1;
        if (k < s.len and std.ascii.isDigit(s[k])) {
            while (k < s.len and std.ascii.isDigit(s[k])) k += 1;
            e = k;
        }
    }
    const v = std.fmt.parseFloat(f64, s[i..e]) catch 0.0;
    return .{ .value = if (neg) -v else v, .end = e };
}

fn parseHexFallback(body: []const u8) f64 {
    return std.fmt.parseFloat(f64, body) catch 0.0;
}

/// C `strtoul(s, &end, 16)` subset used by luaO_str2d.
fn strtoul16(s: []const u8) ?Parsed {
    var i: usize = 0;
    while (i < s.len and isspace(s[i])) i += 1;
    var neg = false;
    if (i < s.len and (s[i] == '+' or s[i] == '-')) {
        neg = s[i] == '-';
        i += 1;
    }
    if (i + 1 < s.len and s[i] == '0' and (s[i + 1] == 'x' or s[i + 1] == 'X') and i + 2 < s.len and std.ascii.isHex(s[i + 2])) i += 2;
    var v: u64 = 0;
    var overflow = false;
    const st = i;
    while (i < s.len and std.ascii.isHex(s[i])) : (i += 1) {
        const d = std.fmt.charToDigit(s[i], 16) catch unreachable;
        const m = @mulWithOverflow(v, 16);
        const a = @addWithOverflow(m[0], d);
        if (m[1] != 0 or a[1] != 0) overflow = true;
        v = a[0];
    }
    if (i == st) return null;
    const ulong_max: u64 = std.math.maxInt(c_ulong);
    if (overflow or v > ulong_max) v = ulong_max;
    if (neg) v = (~v +% 1) & ulong_max;
    return .{ .value = @floatFromInt(v), .end = i };
}

/// luaO_str2d: converts a whole string (C-string semantics: stops at NUL).
pub fn str2d(str: []const u8) ?f64 {
    const s = std.mem.sliceTo(str, 0);
    var r = strtod(s) orelse return null;
    if (r.end < s.len and (s[r.end] == 'x' or s[r.end] == 'X')) {
        if (strtoul16(s)) |h| r = h;
    }
    var end = r.end;
    while (end < s.len and isspace(s[end])) end += 1;
    return if (end == s.len) r.value else null;
}
