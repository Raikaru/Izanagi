//! C variadic argument access and a printf-compatible formatter, written in Zig.
//!
//! Zig's builtin C-varargs support is disabled for aarch64 (non-Apple) and
//! x86_64-windows in the pinned toolchain, so this module reads `va_list` values
//! itself according to each platform ABI and provides naked assembly entry thunks
//! for C-variadic exports on those targets. The formatter follows C99 printf
//! semantics (flags, width, precision, length modifiers, exact decimal rounding for
//! %f/%e/%g) with the 512-byte truncation used by upstream luaO_pushvfstring.

const std = @import("std");
const builtin = @import("builtin");

const arch = builtin.cpu.arch;
const os = builtin.os.tag;
const is_apple = os.isDarwin();

pub const VaListSysV64 = extern struct {
    gp_offset: c_uint,
    fp_offset: c_uint,
    overflow_arg_area: [*]u8,
    reg_save_area: [*]u8,
};

pub const VaListAapcs64 = extern struct {
    stack: [*]u8,
    gr_top: [*]u8,
    vr_top: [*]u8,
    gr_offs: c_int,
    vr_offs: c_int,
};

const Kind = enum { mem32, mem64, sysv64, aapcs64 };
const kind: Kind = switch (arch) {
    .x86 => .mem32,
    .x86_64 => if (os == .windows or os == .uefi) .mem64 else .sysv64,
    .aarch64 => if (is_apple or os == .windows) .mem64 else .aapcs64,
    else => @compileError("luau-zig VM: C varargs are not implemented for this architecture"),
};

/// The C-ABI type of a `va_list` function parameter on this target.
pub const RawVaList = switch (kind) {
    .mem32, .mem64 => [*]u8,
    .sysv64 => *VaListSysV64,
    .aapcs64 => *VaListAapcs64,
};

pub const VaReader = struct {
    ap: RawVaList,

    fn slotInt(self: *VaReader) u64 {
        switch (kind) {
            .mem32 => {
                const v = std.mem.readInt(u32, self.ap[0..4], .little);
                self.ap += 4;
                return v;
            },
            .mem64 => {
                const v = std.mem.readInt(u64, self.ap[0..8], .little);
                self.ap += 8;
                return v;
            },
            .sysv64 => {
                const a = self.ap;
                if (a.gp_offset < 48) {
                    const v = std.mem.readInt(u64, (a.reg_save_area + a.gp_offset)[0..8], .little);
                    a.gp_offset += 8;
                    return v;
                }
                const v = std.mem.readInt(u64, a.overflow_arg_area[0..8], .little);
                a.overflow_arg_area += 8;
                return v;
            },
            .aapcs64 => {
                const a = self.ap;
                if (a.gr_offs < 0) {
                    const off = a.gr_offs;
                    a.gr_offs += 8;
                    if (a.gr_offs <= 0) {
                        const p = a.gr_top - @as(usize, @intCast(-off));
                        return std.mem.readInt(u64, p[0..8], .little);
                    }
                }
                const v = std.mem.readInt(u64, a.stack[0..8], .little);
                a.stack += 8;
                return v;
            },
        }
    }

    /// Reads an integer argument of C type T (after default promotions).
    pub fn int(self: *VaReader, comptime T: type) T {
        const bits = @typeInfo(T).int.bits;
        if (kind == .mem32 and bits == 64) {
            const lo = self.slotInt();
            const hi = self.slotInt();
            return @bitCast(lo | (hi << 32));
        }
        const raw = self.slotInt();
        if (bits == 64) return @bitCast(raw);
        if (bits == 32) return @bitCast(@as(u32, @truncate(raw)));
        @compileError("unsupported vararg integer width");
    }

    pub fn ptr(self: *VaReader) usize {
        return @intCast(self.slotInt() & std.math.maxInt(usize));
    }

    pub fn double(self: *VaReader) f64 {
        switch (kind) {
            .mem32 => {
                const lo = self.slotInt();
                const hi = self.slotInt();
                return @bitCast(lo | (hi << 32));
            },
            .mem64 => return @bitCast(self.slotInt()),
            .sysv64 => {
                const a = self.ap;
                if (a.fp_offset < 176) {
                    const v = std.mem.readInt(u64, (a.reg_save_area + a.fp_offset)[0..8], .little);
                    a.fp_offset += 16;
                    return @bitCast(v);
                }
                const v = std.mem.readInt(u64, a.overflow_arg_area[0..8], .little);
                a.overflow_arg_area += 8;
                return @bitCast(v);
            },
            .aapcs64 => {
                const a = self.ap;
                if (a.vr_offs < 0) {
                    const off = a.vr_offs;
                    a.vr_offs += 16;
                    if (a.vr_offs <= 0) {
                        const p = a.vr_top - @as(usize, @intCast(-off));
                        return @bitCast(std.mem.readInt(u64, p[0..8], .little));
                    }
                }
                const v = std.mem.readInt(u64, a.stack[0..8], .little);
                a.stack += 8;
                return @bitCast(v);
            },
        }
    }
};

// ---------------------------------------------------------------------------
// Variadic export thunks: `name(a0, a1, ...)` forwards to impl(a0, a1, va_list)
// ---------------------------------------------------------------------------

pub fn VariadicThunk2(comptime A0: type, comptime R: type, comptime impl: fn (A0, [*:0]const u8, RawVaList) callconv(.c) R) type {
    return struct {
        fn implTrampoline(a0: A0, a1: [*:0]const u8, ap: RawVaList) callconv(.c) R {
            return impl(a0, a1, ap);
        }

        pub const entry = switch (kind) {
            .mem64 => if (arch == .x86_64) entryWin64 else entryGeneric,
            .aapcs64 => entryAapcs64,
            .sysv64 => entrySysV,
            .mem32 => entryGeneric,
        };

        /// SysV x86_64: builds the register save area and a va_list (gp_offset=16
        /// for the two fixed arguments, fp_offset=48) without @cVaStart.
        noinline fn entrySysV(_: A0, _: [*:0]const u8) callconv(.naked) R {
            asm volatile (
                \\pushq %%rbp
                \\movq %%rsp, %%rbp
                \\subq $208, %%rsp
                \\movq %%rdi, 0(%%rsp)
                \\movq %%rsi, 8(%%rsp)
                \\movq %%rdx, 16(%%rsp)
                \\movq %%rcx, 24(%%rsp)
                \\movq %%r8, 32(%%rsp)
                \\movq %%r9, 40(%%rsp)
                \\movq %%xmm0, 48(%%rsp)
                \\movq %%xmm1, 64(%%rsp)
                \\movq %%xmm2, 80(%%rsp)
                \\movq %%xmm3, 96(%%rsp)
                \\movq %%xmm4, 112(%%rsp)
                \\movq %%xmm5, 128(%%rsp)
                \\movq %%xmm6, 144(%%rsp)
                \\movq %%xmm7, 160(%%rsp)
                \\movl $16, 176(%%rsp)
                \\movl $48, 180(%%rsp)
                \\leaq 16(%%rbp), %%rax
                \\movq %%rax, 184(%%rsp)
                \\movq %%rsp, 192(%%rsp)
                \\leaq 176(%%rsp), %%rdx
                \\callq %[f:P]
                \\movq %%rbp, %%rsp
                \\popq %%rbp
                \\retq
                :
                : [f] "X" (&implTrampoline),
            );
        }

        noinline fn entryWin64(_: A0, _: [*:0]const u8) callconv(.naked) R {
            asm volatile (
                \\movq %%r8, 24(%%rsp)
                \\movq %%r9, 32(%%rsp)
                \\leaq 24(%%rsp), %%r8
                \\jmp %[f:P]
                :
                : [f] "X" (&implTrampoline),
            );
        }

        noinline fn entryAapcs64(_: A0, _: [*:0]const u8) callconv(.naked) R {
            asm volatile (
                \\stp x29, x30, [sp, #-16]!
                \\mov x29, sp
                \\sub sp, sp, #224
                \\stp x0, x1, [sp, #0]
                \\stp x2, x3, [sp, #16]
                \\stp x4, x5, [sp, #32]
                \\stp x6, x7, [sp, #48]
                \\stp q0, q1, [sp, #64]
                \\stp q2, q3, [sp, #96]
                \\stp q4, q5, [sp, #128]
                \\stp q6, q7, [sp, #160]
                \\add x9, x29, #16
                \\str x9, [sp, #192]
                \\add x9, sp, #64
                \\str x9, [sp, #200]
                \\add x9, sp, #192
                \\str x9, [sp, #208]
                \\mov w9, #-48
                \\str w9, [sp, #216]
                \\mov w9, #-128
                \\str w9, [sp, #220]
                \\add x2, sp, #192
                \\bl %[f]
                \\mov sp, x29
                \\ldp x29, x30, [sp], #16
                \\ret
                :
                : [f] "X" (&implTrampoline),
            );
        }

        fn entryGeneric(a0: A0, a1: [*:0]const u8, ...) callconv(.c) R {
            var ap = @cVaStart();
            defer @cVaEnd(&ap);
            const raw: RawVaList = switch (kind) {
                .sysv64 => @ptrCast(&ap),
                else => @ptrCast(ap),
            };
            return impl(a0, a1, raw);
        }
    };
}

// ---------------------------------------------------------------------------
// Formatter
// ---------------------------------------------------------------------------

pub const Out = struct {
    buf: []u8,
    len: usize = 0,

    pub fn put(self: *Out, c: u8) void {
        if (self.len < self.buf.len) {
            self.buf[self.len] = c;
            self.len += 1;
        }
    }
    pub fn puts(self: *Out, s: []const u8) void {
        for (s) |c| self.put(c);
    }
    pub fn pad(self: *Out, c: u8, n: usize) void {
        var i: usize = 0;
        while (i < n) : (i += 1) self.put(c);
    }
    pub fn slice(self: *const Out) []u8 {
        return self.buf[0..self.len];
    }
};

const Spec = struct {
    left: bool = false,
    plus: bool = false,
    space: bool = false,
    alt: bool = false,
    zero: bool = false,
    width: usize = 0,
    prec: ?usize = null,
};

fn emitPadded(out: *Out, spec: Spec, prefix: []const u8, body: []const u8, numeric: bool) void {
    const total = prefix.len + body.len;
    const padn = if (spec.width > total) spec.width - total else 0;
    if (spec.left) {
        out.puts(prefix);
        out.puts(body);
        out.pad(' ', padn);
    } else if (spec.zero and numeric) {
        out.puts(prefix);
        out.pad('0', padn);
        out.puts(body);
    } else {
        out.pad(' ', padn);
        out.puts(prefix);
        out.puts(body);
    }
}

fn fmtInt(out: *Out, spec: Spec, value: u64, negative: bool, base: u8, upper: bool, signed: bool) void {
    var digits: [72]u8 = undefined;
    var n: usize = 0;
    var v = value;
    const chars = if (upper) "0123456789ABCDEF" else "0123456789abcdef";
    while (v != 0) : (v /= base) {
        digits[digits.len - 1 - n] = chars[@intCast(v % base)];
        n += 1;
    }
    // precision: minimum number of digits (0 with value 0 prints nothing)
    const minDigits: usize = spec.prec orelse 1;
    var body_buf: [140]u8 = undefined;
    var bl: usize = 0;
    var alt_octal_zero = false;
    if (spec.alt and base == 8 and (n >= minDigits)) alt_octal_zero = true;
    const zeros = if (minDigits > n) minDigits - n else 0;
    var z: usize = 0;
    while (z < zeros and bl < body_buf.len) : (z += 1) {
        body_buf[bl] = '0';
        bl += 1;
    }
    if (alt_octal_zero and (n == 0 or digits[digits.len - n] != '0') and zeros == 0) {
        body_buf[bl] = '0';
        bl += 1;
    }
    @memcpy(body_buf[bl .. bl + n], digits[digits.len - n ..]);
    bl += n;
    var prefix_buf: [3]u8 = undefined;
    var pl: usize = 0;
    if (signed) {
        if (negative) {
            prefix_buf[0] = '-';
            pl = 1;
        } else if (spec.plus) {
            prefix_buf[0] = '+';
            pl = 1;
        } else if (spec.space) {
            prefix_buf[0] = ' ';
            pl = 1;
        }
    }
    if (spec.alt and base == 16 and value != 0) {
        prefix_buf[pl] = '0';
        prefix_buf[pl + 1] = if (upper) 'X' else 'x';
        pl += 2;
    }
    var s = spec;
    if (spec.prec != null) s.zero = false;
    emitPadded(out, s, prefix_buf[0..pl], body_buf[0..bl], true);
}

/// Exact decimal expansion of a positive finite double: value = 0.d1d2...dn * 10^point.
const Digits = struct {
    d: [800]u8 = undefined,
    n: usize = 0,
    point: i32 = 0,

    fn init(x: f64) Digits {
        var r: Digits = .{};
        if (x == 0) return r;
        const bits: u64 = @bitCast(x);
        const bexp: i32 = @intCast((bits >> 52) & 0x7ff);
        var m: u64 = bits & ((@as(u64, 1) << 52) - 1);
        var e: i32 = undefined;
        if (bexp == 0) {
            e = -1074;
        } else {
            m |= @as(u64, 1) << 52;
            e = bexp - 1075;
        }
        // big integer in base 1e9, little endian limbs
        var limbs: [130]u32 = undefined;
        var nl: usize = 0;
        var mm = m;
        while (mm != 0) : (mm /= 1_000_000_000) {
            limbs[nl] = @intCast(mm % 1_000_000_000);
            nl += 1;
        }
        var dexp: i32 = 0; // value = big * 10^dexp
        if (e > 0) {
            var k: i32 = e;
            while (k > 0) {
                const sh: u6 = @intCast(@min(k, 29));
                mulSmall(&limbs, &nl, @as(u64, 1) << sh);
                k -= sh;
            }
        } else if (e < 0) {
            var k: i32 = -e;
            dexp = e;
            while (k > 0) {
                const c = @min(k, 13);
                var p5: u64 = 1;
                var j: i32 = 0;
                while (j < c) : (j += 1) p5 *= 5;
                mulSmall(&limbs, &nl, p5);
                k -= c;
            }
        }
        // to decimal digits
        var tmp: [1200]u8 = undefined;
        var tl: usize = 0;
        var li: usize = nl;
        while (li > 0) {
            li -= 1;
            var chunk: [9]u8 = undefined;
            var v = limbs[li];
            var q: usize = 9;
            while (q > 0) {
                q -= 1;
                chunk[q] = '0' + @as(u8, @intCast(v % 10));
                v /= 10;
            }
            @memcpy(tmp[tl .. tl + 9], &chunk);
            tl += 9;
        }
        var start: usize = 0;
        while (start < tl and tmp[start] == '0') start += 1;
        var end = tl;
        const intlen: i32 = @intCast(tl - start);
        while (end > start and tmp[end - 1] == '0') end -= 1;
        const nd = @min(end - start, r.d.len);
        @memcpy(r.d[0..nd], tmp[start .. start + nd]);
        r.n = nd;
        r.point = intlen + dexp;
        return r;
    }

    fn mulSmall(limbs: *[130]u32, nl: *usize, mul: u64) void {
        var carry: u64 = 0;
        for (0..nl.*) |i| {
            const v = @as(u64, limbs[i]) * mul + carry;
            limbs[i] = @intCast(v % 1_000_000_000);
            carry = v / 1_000_000_000;
        }
        while (carry != 0) {
            limbs[nl.*] = @intCast(carry % 1_000_000_000);
            carry /= 1_000_000_000;
            nl.* += 1;
        }
    }

    fn digit(self: *const Digits, i: usize) u8 {
        return if (i < self.n) self.d[i] else '0';
    }

    /// Rounds to `keep` significant digits (round-half-even on the exact value).
    fn round(self: *Digits, keep_in: i64) void {
        if (self.n == 0) return;
        if (keep_in < 0) {
            self.n = 0;
            return;
        }
        const keep: usize = @intCast(keep_in);
        if (keep >= self.n) return;
        const next = self.d[keep];
        var up = false;
        if (next > '5') {
            up = true;
        } else if (next == '5') {
            var rest_nonzero = false;
            for (self.d[keep + 1 .. self.n]) |c| {
                if (c != '0') rest_nonzero = true;
            }
            if (rest_nonzero) {
                up = true;
            } else {
                const prev: u8 = if (keep > 0) self.d[keep - 1] else '0';
                up = (prev - '0') % 2 == 1;
            }
        }
        self.n = keep;
        if (up) {
            var i = keep;
            while (i > 0) {
                i -= 1;
                if (self.d[i] == '9') {
                    self.d[i] = '0';
                } else {
                    self.d[i] += 1;
                    break;
                }
            } else {
                // carry out of the most significant digit
                @memmove(self.d[1 .. keep + 1], self.d[0..keep]);
                self.d[0] = '1';
                self.n = keep + 1;
                self.point += 1;
            }
            if (keep == 0) {
                self.d[0] = '1';
                self.n = 1;
                self.point += 1;
            }
        }
        while (self.n > 0 and self.d[self.n - 1] == '0') self.n -= 1;
        if (self.n == 0) self.point = 0;
    }
};

fn fmtFloat(out: *Out, spec: Spec, value: f64, conv: u8) void {
    const upper = conv == 'F' or conv == 'E' or conv == 'G' or conv == 'A';
    const neg = std.math.signbit(value);
    var prefix: []const u8 = "";
    if (neg) prefix = "-" else if (spec.plus) prefix = "+" else if (spec.space) prefix = " ";
    if (std.math.isNan(value) or std.math.isInf(value)) {
        const s: []const u8 = if (std.math.isNan(value)) (if (upper) "NAN" else "nan") else (if (upper) "INF" else "inf");
        var sp = spec;
        sp.zero = false;
        emitPadded(out, sp, prefix, s, false);
        return;
    }
    const x = @abs(value);
    var body: [1400]u8 = undefined;
    var bl: usize = 0;
    const lower = std.ascii.toLower(conv);
    if (lower == 'a') {
        bl = hexFloat(&body, x, spec.prec, upper);
    } else {
        var prec: usize = spec.prec orelse 6;
        var mode = lower;
        var strip = false;
        var d = Digits.init(x);
        if (lower == 'g') {
            if (prec == 0) prec = 1;
            var t = d;
            t.round(@intCast(prec));
            const xexp: i64 = if (t.n == 0) 0 else t.point - 1;
            if (@as(i64, @intCast(prec)) > xexp and xexp >= -4) {
                mode = 'f';
                prec = @intCast(@as(i64, @intCast(prec)) - 1 - xexp);
            } else {
                mode = 'e';
                prec -= 1;
            }
            strip = !spec.alt;
        }
        if (mode == 'f') {
            d.round(@as(i64, d.point) + @as(i64, @intCast(prec)));
            // integer part
            if (d.n == 0 or d.point <= 0) {
                body[bl] = '0';
                bl += 1;
            } else {
                var i: usize = 0;
                while (i < @as(usize, @intCast(d.point)) and bl < body.len - 64) : (i += 1) {
                    body[bl] = d.digit(i);
                    bl += 1;
                }
            }
            if (prec > 0 or spec.alt) {
                body[bl] = '.';
                bl += 1;
            }
            var j: usize = 0;
            while (j < prec and bl < body.len) : (j += 1) {
                const pos: i64 = @as(i64, d.point) + @as(i64, @intCast(j));
                body[bl] = if (d.n == 0 or pos < 0) '0' else d.digit(@intCast(pos));
                bl += 1;
            }
        } else {
            d.round(@intCast(prec + 1));
            const e10: i64 = if (d.n == 0) 0 else d.point - 1;
            body[bl] = d.digit(0);
            bl += 1;
            if (prec > 0 or spec.alt) {
                body[bl] = '.';
                bl += 1;
            }
            var j: usize = 0;
            while (j < prec and bl < body.len - 8) : (j += 1) {
                body[bl] = d.digit(j + 1);
                bl += 1;
            }
            if (strip) bl = stripZeros(&body, bl);
            body[bl] = if (upper) 'E' else 'e';
            body[bl + 1] = if (e10 < 0) '-' else '+';
            bl += 2;
            const ae: u64 = @intCast(if (e10 < 0) -e10 else e10);
            if (ae < 10) {
                body[bl] = '0';
                bl += 1;
            }
            var eb: [8]u8 = undefined;
            const es = std.fmt.bufPrint(&eb, "{d}", .{ae}) catch unreachable;
            @memcpy(body[bl .. bl + es.len], es);
            bl += es.len;
            strip = false;
        }
        if (strip) bl = stripZeros(&body, bl);
    }
    emitPadded(out, spec, prefix, body[0..bl], true);
}

fn stripZeros(body: []u8, len_in: usize) usize {
    var len = len_in;
    if (std.mem.indexOfScalar(u8, body[0..len], '.') == null) return len;
    while (len > 0 and body[len - 1] == '0') len -= 1;
    if (len > 0 and body[len - 1] == '.') len -= 1;
    return len;
}

fn hexFloat(body: []u8, x: f64, prec: ?usize, upper: bool) usize {
    const bits: u64 = @bitCast(x);
    const bexp: i32 = @intCast((bits >> 52) & 0x7ff);
    var mant: u64 = bits & ((@as(u64, 1) << 52) - 1);
    var lead: u8 = '1';
    var e: i32 = bexp - 1023;
    if (bexp == 0) {
        lead = '0';
        e = if (mant == 0) 0 else -1022;
    }
    const chars = if (upper) "0123456789ABCDEF" else "0123456789abcdef";
    var nib: usize = 13;
    if (prec) |p| {
        if (p < 13) {
            const drop: u6 = @intCast((13 - p) * 4);
            const half = @as(u64, 1) << (drop - 1);
            const rem = mant & ((@as(u64, 1) << drop) - 1);
            mant >>= drop;
            if (rem > half or (rem == half and (mant & 1) == 1)) mant += 1;
            if (mant >> @intCast(p * 4) != 0 and p < 13) {
                lead += 1;
                mant &= (@as(u64, 1) << @intCast(p * 4)) - 1;
            }
            nib = p;
        }
    } else {
        while (nib > 0 and (mant & 0xf) == 0) {
            mant >>= 4;
            nib -= 1;
        }
    }
    var bl: usize = 0;
    body[0] = '0';
    body[1] = if (upper) 'X' else 'x';
    body[2] = lead;
    bl = 3;
    if (nib > 0) {
        body[bl] = '.';
        bl += 1;
        var i: usize = nib;
        while (i > 0) {
            i -= 1;
            body[bl] = chars[@intCast((mant >> @intCast(i * 4)) & 0xf)];
            bl += 1;
        }
    }
    const s = std.fmt.bufPrint(body[bl..], "{c}{s}{d}", .{ @as(u8, if (upper) 'P' else 'p'), if (e < 0) "-" else "+", @abs(e) }) catch unreachable;
    return bl + s.len;
}

const LenMod = enum { none, hh, h, l, ll, z, j, t, L };

fn readSigned(ap: *VaReader, lm: LenMod) i64 {
    return switch (lm) {
        .hh => @as(i8, @truncate(ap.int(c_int))),
        .h => @as(i16, @truncate(ap.int(c_int))),
        .none => ap.int(c_int),
        .l => ap.int(c_long),
        .ll, .j, .L => ap.int(c_longlong),
        .z, .t => @as(isize, @bitCast(ap.ptr())),
    };
}

fn readUnsigned(ap: *VaReader, lm: LenMod) u64 {
    return switch (lm) {
        .hh => @as(u8, @truncate(ap.int(c_uint))),
        .h => @as(u16, @truncate(ap.int(c_uint))),
        .none => ap.int(c_uint),
        .l => ap.int(c_ulong),
        .ll, .j, .L => ap.int(c_ulonglong),
        .z, .t => ap.ptr(),
    };
}

/// vsnprintf-style formatting into `out` (output beyond the buffer is dropped).
pub fn vformat(out: *Out, fmt: [*:0]const u8, ap: *VaReader) void {
    var i: usize = 0;
    while (fmt[i] != 0) {
        const c = fmt[i];
        if (c != '%') {
            out.put(c);
            i += 1;
            continue;
        }
        i += 1;
        var spec: Spec = .{};
        while (true) : (i += 1) {
            switch (fmt[i]) {
                '-' => spec.left = true,
                '+' => spec.plus = true,
                ' ' => spec.space = true,
                '#' => spec.alt = true,
                '0' => spec.zero = true,
                else => break,
            }
        }
        if (fmt[i] == '*') {
            const w = ap.int(c_int);
            if (w < 0) {
                spec.left = true;
                spec.width = @intCast(-@as(i64, w));
            } else spec.width = @intCast(w);
            i += 1;
        } else {
            while (fmt[i] >= '0' and fmt[i] <= '9') : (i += 1) spec.width = spec.width *| 10 +| (fmt[i] - '0');
        }
        if (fmt[i] == '.') {
            i += 1;
            if (fmt[i] == '*') {
                const p = ap.int(c_int);
                spec.prec = if (p < 0) null else @intCast(p);
                i += 1;
            } else {
                var p: usize = 0;
                while (fmt[i] >= '0' and fmt[i] <= '9') : (i += 1) p = p *| 10 +| (fmt[i] - '0');
                spec.prec = p;
            }
        }
        var lm: LenMod = .none;
        switch (fmt[i]) {
            'h' => {
                i += 1;
                lm = .h;
                if (fmt[i] == 'h') {
                    i += 1;
                    lm = .hh;
                }
            },
            'l' => {
                i += 1;
                lm = .l;
                if (fmt[i] == 'l') {
                    i += 1;
                    lm = .ll;
                }
            },
            'z' => {
                i += 1;
                lm = .z;
            },
            'j' => {
                i += 1;
                lm = .j;
            },
            't' => {
                i += 1;
                lm = .t;
            },
            'L' => {
                i += 1;
                lm = .L;
            },
            'I' => {
                // MSVC I64 / I32 / I
                if (fmt[i + 1] == '6' and fmt[i + 2] == '4') {
                    i += 3;
                    lm = .ll;
                } else if (fmt[i + 1] == '3' and fmt[i + 2] == '2') {
                    i += 3;
                } else {
                    i += 1;
                    lm = .z;
                }
            },
            else => {},
        }
        const conv = fmt[i];
        if (conv == 0) break;
        i += 1;
        switch (conv) {
            '%' => out.put('%'),
            'd', 'i' => {
                const v = readSigned(ap, lm);
                const mag: u64 = if (v < 0) @as(u64, @bitCast(-%v)) else @intCast(v);
                fmtInt(out, spec, mag, v < 0, 10, false, true);
            },
            'u' => fmtInt(out, spec, readUnsigned(ap, lm), false, 10, false, false),
            'x' => fmtInt(out, spec, readUnsigned(ap, lm), false, 16, false, false),
            'X' => fmtInt(out, spec, readUnsigned(ap, lm), false, 16, true, false),
            'o' => fmtInt(out, spec, readUnsigned(ap, lm), false, 8, false, false),
            'c' => {
                const ch: u8 = @truncate(@as(c_uint, @bitCast(ap.int(c_int))));
                var sp = spec;
                sp.zero = false;
                emitPadded(out, sp, "", &[_]u8{ch}, false);
            },
            's' => {
                const p = ap.ptr();
                var sp = spec;
                sp.zero = false;
                if (p == 0) {
                    emitPadded(out, sp, "", "(null)", false);
                } else {
                    const s: [*:0]const u8 = @ptrFromInt(p);
                    var n: usize = 0;
                    const limit = spec.prec orelse std.math.maxInt(usize);
                    while (n < limit and s[n] != 0) n += 1;
                    emitPadded(out, sp, "", s[0..n], false);
                }
            },
            'p' => {
                const p = ap.ptr();
                var sp = spec;
                sp.zero = false;
                if (p == 0) {
                    emitPadded(out, sp, "", "(nil)", false);
                } else {
                    sp.alt = true;
                    sp.prec = null;
                    fmtInt(out, sp, p, false, 16, false, false);
                }
            },
            'f', 'F', 'e', 'E', 'g', 'G', 'a', 'A' => fmtFloat(out, spec, ap.double(), conv),
            'n' => {
                _ = ap.ptr();
            },
            else => {
                out.put('%');
                out.put(conv);
            },
        }
    }
}
