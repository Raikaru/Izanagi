//! Exact emulation of the C `snprintf` conversions used by Luau's `string.format`
//! (%c %d %i %o %u %x %X %e %E %f %g %G %s with flags "-+ #0", width and precision of at
//! most two digits each), following glibc output conventions (the pinned upstream
//! reference platform): correctly rounded decimal conversion of the exact binary value
//! with ties-to-even, "inf"/"nan" spellings with sign ("-nan" for negative NaNs), and the
//! 512-byte MAX_ITEM truncation of the upstream item buffer.

const std = @import("std");

pub const MAX_ITEM = 512;

pub const Spec = struct {
    minus: bool = false,
    plus: bool = false,
    space: bool = false,
    alt: bool = false,
    zero: bool = false,
    width: usize = 0,
    precision: ?usize = null,

    /// Parses the text between '%' and the conversion character (already validated by
    /// upstream's scanformat: flags, up to two width digits, optional '.' and up to two
    /// precision digits).
    pub fn parse(form: []const u8) Spec {
        var s = Spec{};
        var i: usize = 0;
        while (i < form.len) : (i += 1) {
            switch (form[i]) {
                '-' => s.minus = true,
                '+' => s.plus = true,
                ' ' => s.space = true,
                '#' => s.alt = true,
                '0' => s.zero = true,
                else => break,
            }
        }
        while (i < form.len and form[i] >= '0' and form[i] <= '9') : (i += 1)
            s.width = s.width * 10 + (form[i] - '0');
        if (i < form.len and form[i] == '.') {
            i += 1;
            var p: usize = 0;
            while (i < form.len and form[i] >= '0' and form[i] <= '9') : (i += 1)
                p = p * 10 + (form[i] - '0');
            s.precision = p;
        }
        return s;
    }
};

/// Output accumulator with snprintf truncation semantics (keeps at most MAX_ITEM - 1 bytes).
pub const Out = struct {
    buf: [MAX_ITEM]u8 = undefined,
    len: usize = 0,

    fn put(o: *Out, c: u8) void {
        if (o.len < MAX_ITEM - 1) {
            o.buf[o.len] = c;
            o.len += 1;
        }
    }
    fn puts(o: *Out, s: []const u8) void {
        for (s) |c| o.put(c);
    }
    fn pad(o: *Out, c: u8, n: usize) void {
        var i: usize = 0;
        while (i < n) : (i += 1) o.put(c);
    }
    pub fn slice(o: *const Out) []const u8 {
        return o.buf[0..o.len];
    }
};

/// Writes prefix (sign / 0x) + zero padding + body with width handling.
fn emit(o: *Out, spec: Spec, prefix: []const u8, body: []const u8, zero_ok: bool) void {
    const total = prefix.len + body.len;
    const padn = if (spec.width > total) spec.width - total else 0;
    if (spec.minus) {
        o.puts(prefix);
        o.puts(body);
        o.pad(' ', padn);
    } else if (spec.zero and zero_ok) {
        o.puts(prefix);
        o.pad('0', padn);
        o.puts(body);
    } else {
        o.pad(' ', padn);
        o.puts(prefix);
        o.puts(body);
    }
}

// =======================================================================================
// Integers

fn utoa(buf: []u8, v_: u64, base: u64, upper: bool) []const u8 {
    const digits = if (upper) "0123456789ABCDEF" else "0123456789abcdef";
    var v = v_;
    var i = buf.len;
    while (v != 0) {
        i -= 1;
        buf[i] = digits[@intCast(v % base)];
        v /= base;
    }
    return buf[i..];
}

/// %d / %i with a long long argument.
pub fn formatSigned(o: *Out, spec: Spec, value: i64) void {
    const neg = value < 0;
    const mag: u64 = if (neg) 0 -% @as(u64, @bitCast(value)) else @intCast(value);
    var tmp: [32]u8 = undefined;
    var digits = utoa(&tmp, mag, 10, false);
    var body: [160]u8 = undefined;
    var n: usize = 0;
    const prec = spec.precision orelse 1;
    if (digits.len < prec) {
        @memset(body[0 .. prec - digits.len], '0');
        n = prec - digits.len;
    }
    @memcpy(body[n..][0..digits.len], digits);
    n += digits.len;
    digits = body[0..n];
    const prefix: []const u8 = if (neg) "-" else if (spec.plus) "+" else if (spec.space) " " else "";
    emit(o, spec, prefix, digits, spec.precision == null);
}

/// %o %u %x %X with an unsigned long long argument.
pub fn formatUnsigned(o: *Out, spec: Spec, conv: u8, value: u64) void {
    const base: u64 = switch (conv) {
        'o' => 8,
        'x', 'X' => 16,
        else => 10,
    };
    var tmp: [32]u8 = undefined;
    const digits = utoa(&tmp, value, base, conv == 'X');
    var body: [160]u8 = undefined;
    var n: usize = 0;
    var prec = spec.precision orelse 1;
    if (conv == 'o' and spec.alt and digits.len + 1 > prec) {
        // '#' with 'o' increases the precision to force a leading zero
        if (digits.len == 0 or prec <= digits.len) prec = digits.len + 1;
    }
    if (digits.len < prec) {
        @memset(body[0 .. prec - digits.len], '0');
        n = prec - digits.len;
    }
    @memcpy(body[n..][0..digits.len], digits);
    n += digits.len;
    const prefix: []const u8 = if (spec.alt and value != 0 and conv == 'x') "0x" else if (spec.alt and value != 0 and conv == 'X') "0X" else "";
    emit(o, spec, prefix, body[0..n], spec.precision == null);
}

/// %c
pub fn formatChar(o: *Out, spec: Spec, c: u8) void {
    emit(o, spec, "", &[_]u8{c}, false);
}

/// %s with a C string (stops at the first NUL), precision limits the bytes written.
pub fn formatString(o: *Out, spec: Spec, s_: []const u8) void {
    var s = s_[0 .. std.mem.indexOfScalar(u8, s_, 0) orelse s_.len];
    if (spec.precision) |p| {
        if (s.len > p) s = s[0..p];
    }
    emit(o, spec, "", s, false);
}

// =======================================================================================
// Floating point: exact decimal expansion of binary64 values

const Digits = struct {
    buf: [1100]u8 = undefined,
    /// significant digits (no leading zeros unless the value is zero)
    sig: []const u8 = &.{},
    /// decimal exponent of sig[0]
    x: i32 = 0,
};

/// Exact decimal digits of a finite, non-negative double.
fn exactDigits(d: *Digits, v: f64) void {
    const bits: u64 = @bitCast(v);
    const bexp: i32 = @intCast((bits >> 52) & 0x7ff);
    const frac: u64 = bits & ((@as(u64, 1) << 52) - 1);
    var m: u64 = undefined;
    var e: i32 = undefined;
    if (bexp == 0) {
        m = frac;
        e = -1074;
    } else {
        m = frac | (@as(u64, 1) << 52);
        e = bexp - 1075;
    }
    if (m == 0) {
        d.buf[0] = '0';
        d.sig = d.buf[0..1];
        d.x = 0;
        return;
    }

    if (e >= 0) {
        // integer value m * 2^e: big integer to decimal by repeated division by 1e9
        var limbs: [36]u32 = @splat(0);
        const sh: u32 = @intCast(e);
        const word = sh / 32;
        const bit: u6 = @intCast(sh % 32);
        const wide: u128 = @as(u128, m) << bit;
        limbs[word] = @truncate(wide);
        limbs[word + 1] = @truncate(wide >> 32);
        limbs[word + 2] = @truncate(wide >> 64);
        var nl: usize = word + 3;
        while (nl > 0 and limbs[nl - 1] == 0) nl -= 1;

        var chunks: [40]u32 = undefined;
        var nc: usize = 0;
        while (nl > 0) {
            var rem: u64 = 0;
            var i = nl;
            while (i > 0) {
                i -= 1;
                const cur = (rem << 32) | limbs[i];
                limbs[i] = @intCast(cur / 1_000_000_000);
                rem = cur % 1_000_000_000;
            }
            chunks[nc] = @intCast(rem);
            nc += 1;
            while (nl > 0 and limbs[nl - 1] == 0) nl -= 1;
        }
        var n: usize = 0;
        var first = true;
        var ci = nc;
        while (ci > 0) {
            ci -= 1;
            var tmp: [9]u8 = undefined;
            var c = chunks[ci];
            var k: usize = 9;
            while (k > 0) {
                k -= 1;
                tmp[k] = '0' + @as(u8, @intCast(c % 10));
                c /= 10;
            }
            var start: usize = 0;
            if (first) {
                while (start < 8 and tmp[start] == '0') start += 1;
                first = false;
            }
            @memcpy(d.buf[n..][0 .. 9 - start], tmp[start..]);
            n += 9 - start;
        }
        d.sig = d.buf[0..n];
        d.x = @intCast(n - 1);
        return;
    }

    const k: u32 = @intCast(-e); // 1..1074 fractional bits
    var n: usize = 0;
    // integer part (fits in 53 bits)
    const ip: u64 = if (k >= 64) 0 else m >> @intCast(k);
    if (ip != 0) {
        var tmp: [24]u8 = undefined;
        const s = utoa(&tmp, ip, 10, false);
        @memcpy(d.buf[0..s.len], s);
        n = s.len;
    }
    const intdigits = n;

    // fractional part: m mod 2^k as a k-bit big integer, digits by multiplying by 10
    var limbs: [36]u32 = @splat(0);
    const fm: u64 = if (k >= 64) m else m & ((@as(u64, 1) << @intCast(k)) - 1);
    limbs[0] = @truncate(fm);
    limbs[1] = @truncate(fm >> 32);
    const nlimbs: usize = (k + 4) / 32 + 1;
    const kw = k / 32;
    const kb: u5 = @intCast(k % 32);
    var leading: usize = 0;
    var seen_nonzero = intdigits != 0;
    while (true) {
        var any = false;
        for (limbs[0..nlimbs]) |l| {
            if (l != 0) {
                any = true;
                break;
            }
        }
        if (!any) break;
        var carry: u64 = 0;
        for (limbs[0..nlimbs]) |*l| {
            const cur = @as(u64, l.*) * 10 + carry;
            l.* = @truncate(cur);
            carry = cur >> 32;
        }
        // digit = value >> k (at most 4 bits), then clear those bits
        const lo: u64 = limbs[kw];
        const hi: u64 = if (kw + 1 < limbs.len) limbs[kw + 1] else 0;
        const window = (hi << 32) | lo;
        const digit: u8 = @intCast((window >> @as(u6, kb)) & 0xf);
        limbs[kw] &= (@as(u32, 1) << kb) -% 1;
        if (kb == 0) limbs[kw] = 0;
        if (kw + 1 < limbs.len) limbs[kw + 1] = 0;
        if (!seen_nonzero) {
            if (digit == 0) {
                leading += 1;
                continue;
            }
            seen_nonzero = true;
        }
        d.buf[n] = '0' + digit;
        n += 1;
    }
    d.sig = d.buf[0..n];
    d.x = if (intdigits != 0) @as(i32, @intCast(intdigits)) - 1 else -@as(i32, @intCast(leading)) - 1;
}

/// Rounds the significant digits to `keep` digits (ties-to-even on the exact value).
/// Returns the digits (possibly one longer than `keep` on carry, or "0"/"1" when keep <= 0).
fn roundDigits(out: []u8, sig: []const u8, keep: i64) []const u8 {
    if (keep <= 0) {
        if (keep < 0) {
            out[0] = '0';
            return out[0..1];
        }
        var up = sig[0] > '5';
        if (sig[0] == '5') {
            for (sig[1..]) |c| {
                if (c != '0') {
                    up = true;
                    break;
                }
            }
        }
        out[0] = if (up) '1' else '0';
        return out[0..1];
    }
    const kp: usize = @intCast(keep);
    if (kp >= sig.len) {
        @memcpy(out[0..sig.len], sig);
        @memset(out[sig.len..kp], '0');
        return out[0..kp];
    }
    @memcpy(out[1 .. kp + 1], sig[0..kp]);
    const cut = sig[kp];
    var up = cut > '5';
    if (cut == '5') {
        var rest_nonzero = false;
        for (sig[kp + 1 ..]) |c| {
            if (c != '0') {
                rest_nonzero = true;
                break;
            }
        }
        up = rest_nonzero or ((sig[kp - 1] - '0') & 1) == 1;
    }
    if (up) {
        var i = kp;
        while (true) {
            if (out[i] == '9') {
                out[i] = '0';
                i -= 1;
                if (i == 0) {
                    out[0] = '1';
                    return out[0 .. kp + 1];
                }
            } else {
                out[i] += 1;
                break;
            }
        }
    }
    return out[1 .. kp + 1];
}

fn fixedBody(dst: []u8, d: *const Digits, prec: usize, alt: bool) []const u8 {
    var tmp: [1200]u8 = undefined;
    const keep: i64 = @as(i64, d.x) + 1 + @as(i64, @intCast(prec));
    var digits = roundDigits(&tmp, d.sig, keep);
    // digits = round(value * 10^prec); left-pad so there is at least one integer digit
    var padded: [1200]u8 = undefined;
    if (digits.len < prec + 1) {
        const z = prec + 1 - digits.len;
        @memset(padded[0..z], '0');
        @memcpy(padded[z..][0..digits.len], digits);
        digits = padded[0 .. z + digits.len];
    }
    const intlen = digits.len - prec;
    var n: usize = 0;
    @memcpy(dst[0..intlen], digits[0..intlen]);
    n = intlen;
    if (prec > 0 or alt) {
        dst[n] = '.';
        n += 1;
    }
    @memcpy(dst[n..][0..prec], digits[intlen..]);
    n += prec;
    return dst[0..n];
}

fn expBody(dst: []u8, d: *const Digits, prec: usize, alt: bool, upper: bool) []const u8 {
    var tmp: [1200]u8 = undefined;
    var digits = roundDigits(&tmp, d.sig, @intCast(prec + 1));
    var x = d.x;
    if (digits.len > prec + 1) {
        digits = digits[0 .. prec + 1];
        x += 1;
    }
    if (d.sig.len == 1 and d.sig[0] == '0') x = 0;
    var n: usize = 0;
    dst[0] = digits[0];
    n = 1;
    if (prec > 0 or alt) {
        dst[n] = '.';
        n += 1;
    }
    @memcpy(dst[n..][0..prec], digits[1..]);
    n += prec;
    dst[n] = if (upper) 'E' else 'e';
    n += 1;
    dst[n] = if (x < 0) '-' else '+';
    n += 1;
    const ax: u32 = @abs(x);
    if (ax < 10) {
        dst[n] = '0';
        n += 1;
    }
    var etmp: [8]u8 = undefined;
    const es = utoa(&etmp, ax, 10, false);
    const ev: []const u8 = if (es.len == 0) "0" else es;
    @memcpy(dst[n..][0..ev.len], ev);
    n += ev.len;
    return dst[0..n];
}

/// %e %E %f %g %G with a double argument.
pub fn formatFloat(o: *Out, spec: Spec, conv: u8, v: f64) void {
    const neg = std.math.signbit(v);
    const prefix: []const u8 = if (neg) "-" else if (spec.plus) "+" else if (spec.space) " " else "";
    const upper = conv == 'E' or conv == 'G';

    if (std.math.isNan(v) or std.math.isInf(v)) {
        const word: []const u8 = if (std.math.isNan(v))
            (if (upper) "NAN" else "nan")
        else
            (if (upper) "INF" else "inf");
        emit(o, spec, prefix, word, false);
        return;
    }

    var d = Digits{};
    exactDigits(&d, @abs(v));

    var body: [1300]u8 = undefined;
    var out: []const u8 = undefined;
    switch (conv) {
        'f' => out = fixedBody(&body, &d, spec.precision orelse 6, spec.alt),
        'e', 'E' => out = expBody(&body, &d, spec.precision orelse 6, spec.alt, upper),
        else => {
            // %g: P significant digits, style chosen from the exponent after rounding
            var P = spec.precision orelse 6;
            if (P == 0) P = 1;
            var tmp: [1200]u8 = undefined;
            const r = roundDigits(&tmp, d.sig, @intCast(P));
            var X: i64 = d.x;
            if (r.len > P) X += 1;
            if (d.sig.len == 1 and d.sig[0] == '0') X = 0;
            if (@as(i64, @intCast(P)) > X and X >= -4) {
                out = fixedBody(&body, &d, @intCast(@as(i64, @intCast(P)) - 1 - X), spec.alt);
            } else {
                out = expBody(&body, &d, P - 1, spec.alt, upper);
            }
            if (!spec.alt) out = stripZeros(&body, out);
        },
    }
    emit(o, spec, prefix, out, true);
}

/// Removes trailing zeros of the fraction (and a trailing '.') keeping any exponent suffix.
fn stripZeros(buf: []u8, s: []const u8) []const u8 {
    const epos = std.mem.indexOfAny(u8, s, "eE") orelse s.len;
    const mant = s[0..epos];
    if (std.mem.indexOfScalar(u8, mant, '.') == null) return s;
    var end = mant.len;
    while (end > 0 and mant[end - 1] == '0') end -= 1;
    if (end > 0 and mant[end - 1] == '.') end -= 1;
    if (end == epos) return s;
    const exp = s[epos..];
    std.mem.copyForwards(u8, buf[end..][0..exp.len], exp);
    return buf[0 .. end + exp.len];
}
