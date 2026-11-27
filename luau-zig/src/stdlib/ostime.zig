//! Calendar time support for the `os` library: time(NULL), gmtime_r, localtime_r and a
//! C-locale strftime, implemented natively (no C runtime). Platform conventions follow the
//! C runtime each upstream build links against:
//! - POSIX (glibc/musl/libSystem): local time from the TZ environment variable (Linux:
//!   read from /proc/self/environ) or /etc/localtime, both as TZif files with POSIX TZ
//!   footer rules; unset/unreadable zones are UTC. TZif leap-second ("right/") data is
//!   not applied. %Z is the zone abbreviation, %z the struct's UTC offset, %c is
//!   "%a %b %e %H:%M:%S %Y".
//! - Windows (UCRT): local time from GetTimeZoneInformation via
//!   SystemTimeToTzSpecificLocalTime; times before 1970 or after year 3000 are rejected
//!   like _gmtime64_s/_localtime64_s; %Z/%z describe the local zone; %c is
//!   "%m/%d/%y %H:%M:%S".
//! Transient file buffers come from the page allocator and are released before control
//! returns to the VM (no Lua call can unwind while they are held).

const std = @import("std");
const builtin = @import("builtin");

const is_windows = builtin.os.tag == .windows;

pub const Tm = struct {
    sec: c_int = 0,
    min: c_int = 0,
    hour: c_int = 0,
    mday: c_int = 1,
    mon: c_int = 0, // 0-11
    year: c_int = 70, // years since 1900
    wday: c_int = 4,
    yday: c_int = 0, // 0-365
    isdst: c_int = 0,
    gmtoff: i64 = 0, // seconds east of UTC
    zone: [64]u8 = undefined,
    zonelen: usize = 0,

    pub fn zoneName(tm: *const Tm) []const u8 {
        return tm.zone[0..tm.zonelen];
    }
    fn setZone(tm: *Tm, name: []const u8) void {
        const n = @min(name.len, tm.zone.len);
        @memcpy(tm.zone[0..n], name[0..n]);
        tm.zonelen = n;
    }
};

fn io() std.Io {
    return std.Io.Threaded.global_single_threaded.io();
}

/// time(NULL)
pub fn now() i64 {
    const ts = std.Io.Clock.real.now(io());
    return @intCast(@divFloor(ts.nanoseconds, std.time.ns_per_s));
}

// =======================================================================================
// Civil calendar arithmetic (proleptic Gregorian)

fn isleap(y: i64) bool {
    return (@mod(y, 4) == 0 and @mod(y, 100) != 0) or @mod(y, 400) == 0;
}

fn daysFromCivil(y_: i64, m: i64, d: i64) i64 {
    const y = if (m <= 2) y_ - 1 else y_;
    const era = @divFloor(y, 400);
    const yoe = y - era * 400;
    const mp = if (m > 2) m - 3 else m + 9;
    const doy = @divFloor(153 * mp + 2, 5) + d - 1;
    const doe = yoe * 365 + @divFloor(yoe, 4) - @divFloor(yoe, 100) + doy;
    return era * 146097 + doe - 719468;
}

/// Breaks down t (seconds since the epoch, already shifted to the wanted zone).
/// Returns null when the year does not fit an int (EOVERFLOW).
fn breakdown(t: i64) ?Tm {
    const days = @divFloor(t, 86400);
    const secs = t - days * 86400;
    const z = days + 719468;
    const era = @divFloor(z, 146097);
    const doe = z - era * 146097;
    const yoe = @divFloor(doe - @divFloor(doe, 1460) + @divFloor(doe, 36524) - @divFloor(doe, 146096), 365);
    var y = yoe + era * 400;
    const doy = doe - (365 * yoe + @divFloor(yoe, 4) - @divFloor(yoe, 100));
    const mp = @divFloor(5 * doy + 2, 153);
    const d = doy - @divFloor(153 * mp + 2, 5) + 1;
    const m = if (mp < 10) mp + 3 else mp - 9;
    if (m <= 2) y += 1;

    const tmyear = y - 1900;
    if (tmyear < std.math.minInt(c_int) or tmyear > std.math.maxInt(c_int)) return null;

    var tm = Tm{};
    tm.sec = @intCast(@mod(secs, 60));
    tm.min = @intCast(@mod(@divFloor(secs, 60), 60));
    tm.hour = @intCast(@divFloor(secs, 3600));
    tm.mday = @intCast(d);
    tm.mon = @intCast(m - 1);
    tm.year = @intCast(tmyear);
    tm.wday = @intCast(@mod(days + 4, 7)); // 1970-01-01 was a Thursday
    tm.yday = @intCast(days - daysFromCivil(y, 1, 1));
    return tm;
}

// Windows _MAX__TIME64_T (3000-12-31 23:59:59 UTC)
const win_max_time: i64 = 32535215999;

/// gmtime_r
pub fn gmtime(t: i64) ?Tm {
    if (is_windows and (t < 0 or t > win_max_time)) return null;
    var tm = breakdown(t) orelse return null;
    if (!is_windows) tm.setZone("GMT");
    return tm;
}

/// localtime_r (callers already reject negative times, as upstream does on all platforms)
pub fn localtime(t: i64) ?Tm {
    if (is_windows) return windowsLocal(t);
    const z = posixZone(t);
    var tm = breakdown(t +| z.offset) orelse return null;
    tm.isdst = @intFromBool(z.isdst);
    tm.gmtoff = z.offset;
    tm.setZone(z.abbr[0..z.abbrlen]);
    return tm;
}

// =======================================================================================
// POSIX zones: TZif + POSIX TZ strings

const ZoneInfo = struct {
    offset: i64 = 0,
    isdst: bool = false,
    abbr: [64]u8 = undefined,
    abbrlen: usize = 0,

    fn utc() ZoneInfo {
        var z = ZoneInfo{};
        @memcpy(z.abbr[0..3], "UTC");
        z.abbrlen = 3;
        return z;
    }
    fn set(z: *ZoneInfo, name: []const u8) void {
        const n = @min(name.len, z.abbr.len);
        @memcpy(z.abbr[0..n], name[0..n]);
        z.abbrlen = n;
    }
};

const max_tzfile = 1 << 20;

fn readFile(path: []const u8) ?[]u8 {
    return std.Io.Dir.cwd().readFileAlloc(io(), path, std.heap.page_allocator, .limited(max_tzfile)) catch null;
}

/// TZ from the process environment (Linux /proc; other systems report none).
fn envTZ(buf: []u8) ?[]const u8 {
    if (builtin.os.tag != .linux) return null;
    const env = readFile("/proc/self/environ") orelse return null;
    defer std.heap.page_allocator.free(env);
    var it = std.mem.splitScalar(u8, env, 0);
    while (it.next()) |kv| {
        if (std.mem.startsWith(u8, kv, "TZ=")) {
            const v = kv[3..];
            if (v.len > buf.len) return null;
            @memcpy(buf[0..v.len], v);
            return buf[0..v.len];
        }
    }
    return null;
}

fn posixZone(t: i64) ZoneInfo {
    var tzbuf: [256]u8 = undefined;
    var pathbuf: [512]u8 = undefined;
    var path: []const u8 = "/etc/localtime";
    if (envTZ(&tzbuf)) |tz_| {
        var tz = tz_;
        if (tz.len == 0) return ZoneInfo.utc();
        const isfile = tz[0] == ':';
        if (isfile) tz = tz[1..];
        if (!isfile) {
            // a POSIX TZ rule string takes precedence when it parses as one
            if (Rule.parse(tz)) |rule| return rule.compute(t);
        }
        if (tz.len > 0 and tz[0] == '/') {
            path = tz;
        } else {
            path = std.fmt.bufPrint(&pathbuf, "/usr/share/zoneinfo/{s}", .{tz}) catch return ZoneInfo.utc();
        }
    }
    const data = readFile(path) orelse return ZoneInfo.utc();
    defer std.heap.page_allocator.free(data);
    return tzifCompute(data, t) orelse ZoneInfo.utc();
}

fn be32(b: []const u8) u32 {
    return std.mem.readInt(u32, b[0..4], .big);
}

fn tzifCompute(data: []const u8, t: i64) ?ZoneInfo {
    if (data.len < 44 or !std.mem.eql(u8, data[0..4], "TZif")) return null;
    const version = data[4];
    var hdr: usize = 0;
    var timesize: usize = 4;

    const Counts = struct { isut: usize, isstd: usize, leap: usize, time: usize, typ: usize, char: usize };
    const counts = struct {
        fn get(d: []const u8, at: usize) Counts {
            return .{
                .isut = be32(d[at + 20 ..]),
                .isstd = be32(d[at + 24 ..]),
                .leap = be32(d[at + 28 ..]),
                .time = be32(d[at + 32 ..]),
                .typ = be32(d[at + 36 ..]),
                .char = be32(d[at + 40 ..]),
            };
        }
    }.get;

    var c = counts(data, 0);
    if (version >= '2') {
        // skip the 32-bit block and use the 64-bit one
        const v1len = c.time * 4 + c.time + c.typ * 6 + c.char + c.leap * 8 + c.isstd + c.isut;
        hdr = 44 + v1len;
        if (data.len < hdr + 44 or !std.mem.eql(u8, data[hdr .. hdr + 4], "TZif")) return null;
        c = counts(data, hdr);
        timesize = 8;
    }
    const body = hdr + 44;
    const transat = body;
    const transidx = transat + c.time * timesize;
    const types = transidx + c.time;
    const chars = types + c.typ * 6;
    const after = chars + c.char + c.leap * (timesize + 4) + c.isstd + c.isut;
    if (after > data.len or c.typ == 0) return null;

    const footer: ?[]const u8 = blk: {
        if (timesize != 8 or after >= data.len or data[after] != '\n') break :blk null;
        const end = std.mem.indexOfScalarPos(u8, data, after + 1, '\n') orelse break :blk null;
        break :blk data[after + 1 .. end];
    };

    const transTime = struct {
        fn get(d: []const u8, base: usize, size: usize, i: usize) i64 {
            if (size == 8) return std.mem.readInt(i64, d[base + i * 8 ..][0..8], .big);
            return @as(i32, @bitCast(be32(d[base + i * 4 ..])));
        }
    }.get;

    var ti: usize = 0;
    if (c.time == 0 or t < transTime(data, transat, timesize, 0)) {
        // before the first transition: first standard-time type (glibc)
        while (ti < c.typ and data[types + ti * 6 + 4] != 0) ti += 1;
        if (ti == c.typ) ti = 0;
    } else if (t >= transTime(data, transat, timesize, c.time - 1)) {
        if (footer) |f| {
            if (f.len > 0) {
                if (Rule.parse(f)) |rule| return rule.compute(t);
            }
        }
        ti = data[transidx + c.time - 1];
    } else {
        var lo: usize = 0;
        var hi: usize = c.time - 1;
        while (hi - lo > 1) {
            const mid = (lo + hi) / 2;
            if (t < transTime(data, transat, timesize, mid)) hi = mid else lo = mid;
        }
        ti = data[transidx + lo];
    }
    if (ti >= c.typ) return null;
    const ty = data[types + ti * 6 ..][0..6];
    var z = ZoneInfo{};
    z.offset = @as(i32, @bitCast(be32(ty[0..4])));
    z.isdst = ty[4] != 0;
    const ai = chars + ty[5];
    if (ai < chars + c.char) {
        const end = std.mem.indexOfScalarPos(u8, data[0 .. chars + c.char], ai, 0) orelse (chars + c.char);
        z.set(data[ai..end]);
    }
    return z;
}

/// POSIX TZ rule string: std offset [dst [offset] [,start[/time],end[/time]]]
const Rule = struct {
    stdname: [64]u8 = undefined,
    stdlen: usize = 0,
    dstname: [64]u8 = undefined,
    dstlen: usize = 0,
    stdoff: i64 = 0, // seconds east of UTC
    dstoff: i64 = 0,
    hasdst: bool = false,
    start: Date = .{},
    end: Date = .{},

    const Date = struct {
        kind: enum { julian1, julian0, mwd } = .mwd,
        n: i64 = 0,
        m: i64 = 0,
        w: i64 = 0,
        d: i64 = 0,
        time: i64 = 7200,
    };

    const Parser = struct {
        s: []const u8,
        i: usize = 0,
        fn peek(p: *Parser) u8 {
            return if (p.i < p.s.len) p.s[p.i] else 0;
        }
        fn name(p: *Parser, out: []u8) ?usize {
            var n: usize = 0;
            if (p.peek() == '<') {
                p.i += 1;
                while (p.peek() != '>') {
                    if (p.peek() == 0 or n == out.len) return null;
                    out[n] = p.peek();
                    n += 1;
                    p.i += 1;
                }
                p.i += 1;
            } else {
                while (std.ascii.isAlphabetic(p.peek())) {
                    if (n == out.len) return null;
                    out[n] = p.peek();
                    n += 1;
                    p.i += 1;
                }
            }
            return if (n >= 3) n else null;
        }
        fn num(p: *Parser) ?i64 {
            const start = p.i;
            var v: i64 = 0;
            while (std.ascii.isDigit(p.peek()) and p.i - start < 4) {
                v = v * 10 + (p.peek() - '0');
                p.i += 1;
            }
            return if (p.i == start) null else v;
        }
        /// [+-]hh[:mm[:ss]] in seconds
        fn hms(p: *Parser) ?i64 {
            var sign: i64 = 1;
            if (p.peek() == '+' or p.peek() == '-') {
                if (p.peek() == '-') sign = -1;
                p.i += 1;
            }
            var v = (p.num() orelse return null) * 3600;
            if (p.peek() == ':') {
                p.i += 1;
                v += (p.num() orelse return null) * 60;
                if (p.peek() == ':') {
                    p.i += 1;
                    v += p.num() orelse return null;
                }
            }
            return sign * v;
        }
        fn date(p: *Parser) ?Date {
            var d = Date{};
            if (p.peek() == 'J') {
                p.i += 1;
                d.kind = .julian1;
                d.n = p.num() orelse return null;
                if (d.n < 1 or d.n > 365) return null;
            } else if (p.peek() == 'M') {
                p.i += 1;
                d.kind = .mwd;
                d.m = p.num() orelse return null;
                if (p.peek() != '.') return null;
                p.i += 1;
                d.w = p.num() orelse return null;
                if (p.peek() != '.') return null;
                p.i += 1;
                d.d = p.num() orelse return null;
                if (d.m < 1 or d.m > 12 or d.w < 1 or d.w > 5 or d.d > 6) return null;
            } else {
                d.kind = .julian0;
                d.n = p.num() orelse return null;
                if (d.n > 365) return null;
            }
            if (p.peek() == '/') {
                p.i += 1;
                d.time = p.hms() orelse return null;
            }
            return d;
        }
    };

    fn parse(s: []const u8) ?Rule {
        var r = Rule{};
        var p = Parser{ .s = s };
        r.stdlen = p.name(&r.stdname) orelse return null;
        r.stdoff = -(p.hms() orelse return null);
        if (p.peek() == 0) return r;
        r.dstlen = p.name(&r.dstname) orelse return null;
        r.hasdst = true;
        r.dstoff = r.stdoff + 3600;
        if (p.peek() != ',' and p.peek() != 0) r.dstoff = -(p.hms() orelse return null);
        if (p.peek() == 0) {
            // no rule: the conventional US default (glibc posixrules fallback)
            r.start = .{ .kind = .mwd, .m = 3, .w = 2, .d = 0 };
            r.end = .{ .kind = .mwd, .m = 11, .w = 1, .d = 0 };
            return r;
        }
        if (p.peek() != ',') return null;
        p.i += 1;
        r.start = p.date() orelse return null;
        if (p.peek() != ',') return null;
        p.i += 1;
        r.end = p.date() orelse return null;
        if (p.peek() != 0) return null;
        return r;
    }

    /// Day number (days since epoch) of the rule date in `year`.
    fn dayOf(d: Date, year: i64) i64 {
        const jan1 = daysFromCivil(year, 1, 1);
        switch (d.kind) {
            .julian1 => {
                var n = d.n - 1;
                if (isleap(year) and d.n >= 60) n += 1;
                return jan1 + n;
            },
            .julian0 => return jan1 + d.n,
            .mwd => {
                const first = daysFromCivil(year, d.m, 1);
                const wd1 = @mod(first + 4, 7);
                var day = @mod(d.d - wd1 + 7, 7) + (d.w - 1) * 7;
                const mdays: [12]i64 = .{ 31, if (isleap(year)) 29 else 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
                while (day >= mdays[@intCast(d.m - 1)]) day -= 7;
                return first + day;
            },
        }
    }

    fn compute(r: Rule, t: i64) ZoneInfo {
        var z = ZoneInfo{};
        var dst = false;
        if (r.hasdst) {
            const year = @as(i64, (breakdown(t +| r.stdoff) orelse Tm{}).year) + 1900;
            const start = dayOf(r.start, year) * 86400 + r.start.time - r.stdoff;
            const end = dayOf(r.end, year) * 86400 + r.end.time - r.dstoff;
            dst = if (start < end) (t >= start and t < end) else !(t >= end and t < start);
        }
        if (dst) {
            z.offset = r.dstoff;
            z.isdst = true;
            z.set(r.dstname[0..r.dstlen]);
        } else {
            z.offset = r.stdoff;
            z.set(r.stdname[0..r.stdlen]);
        }
        return z;
    }
};

// =======================================================================================
// Windows (kernel32)

const SYSTEMTIME = extern struct {
    wYear: u16,
    wMonth: u16,
    wDayOfWeek: u16,
    wDay: u16,
    wHour: u16,
    wMinute: u16,
    wSecond: u16,
    wMilliseconds: u16,
};

const TIME_ZONE_INFORMATION = extern struct {
    Bias: i32,
    StandardName: [32]u16,
    StandardDate: SYSTEMTIME,
    StandardBias: i32,
    DaylightName: [32]u16,
    DaylightDate: SYSTEMTIME,
    DaylightBias: i32,
};

const win = if (is_windows) struct {
    extern "kernel32" fn GetTimeZoneInformation(info: *TIME_ZONE_INFORMATION) callconv(.winapi) u32;
    extern "kernel32" fn SystemTimeToTzSpecificLocalTime(tz: ?*const TIME_ZONE_INFORMATION, utc: *const SYSTEMTIME, local: *SYSTEMTIME) callconv(.winapi) c_int;
} else struct {};

const TIME_ZONE_ID_DAYLIGHT = 2;

fn winZoneName(tm: *Tm, name: *const [32]u16) void {
    // _tzname is the ANSI conversion of the zone name; non-ASCII becomes '?'
    var n: usize = 0;
    for (name) |ch| {
        if (ch == 0) break;
        tm.zone[n] = if (ch < 0x80) @intCast(ch) else '?';
        n += 1;
    }
    tm.zonelen = n;
}

fn windowsLocal(t: i64) ?Tm {
    if (t < 0 or t > win_max_time) return null;
    const utc = breakdown(t) orelse return null;
    var info: TIME_ZONE_INFORMATION = undefined;
    _ = win.GetTimeZoneInformation(&info);
    const st = SYSTEMTIME{
        .wYear = @intCast(utc.year + 1900),
        .wMonth = @intCast(utc.mon + 1),
        .wDayOfWeek = @intCast(utc.wday),
        .wDay = @intCast(utc.mday),
        .wHour = @intCast(utc.hour),
        .wMinute = @intCast(utc.min),
        .wSecond = @intCast(utc.sec),
        .wMilliseconds = 0,
    };
    var lt: SYSTEMTIME = undefined;
    if (win.SystemTimeToTzSpecificLocalTime(null, &st, &lt) == 0) return null;
    const localsecs = daysFromCivil(lt.wYear, lt.wMonth, lt.wDay) * 86400 + @as(i64, lt.wHour) * 3600 + @as(i64, lt.wMinute) * 60 + lt.wSecond;
    var tm = breakdown(localsecs) orelse return null;
    tm.gmtoff = localsecs - t;
    const stdoff = -@as(i64, info.Bias + info.StandardBias) * 60;
    tm.isdst = @intFromBool(info.DaylightDate.wMonth != 0 and tm.gmtoff != stdoff);
    winZoneName(&tm, if (tm.isdst != 0) &info.DaylightName else &info.StandardName);
    return tm;
}

/// UCRT %Z/%z always describe the local zone (from _tzset), whatever struct is formatted.
fn windowsZoneFor(tm: *const Tm, out: *Tm) void {
    var info: TIME_ZONE_INFORMATION = undefined;
    _ = win.GetTimeZoneInformation(&info);
    const dst = tm.isdst != 0 and info.DaylightDate.wMonth != 0;
    winZoneName(out, if (dst) &info.DaylightName else &info.StandardName);
    out.gmtoff = -@as(i64, info.Bias + (if (dst) info.DaylightBias else info.StandardBias)) * 60;
}

// =======================================================================================
// strftime (C locale) for the conversions upstream accepts: "aAbBcdHIjmMpSUwWxXyYzZ%"

const wdays = [_][]const u8{ "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
const months = [_][]const u8{ "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

/// Truncating write cursor over the caller's output buffer.
const Out = struct {
    buf: []u8,
    n: usize = 0,

    fn put(out: *Out, s: []const u8) void {
        const k = @min(s.len, out.buf.len - out.n);
        @memcpy(out.buf[out.n..][0..k], s[0..k]);
        out.n += k;
    }

    fn putnum(out: *Out, v: i64, width: usize, pad: u8) void {
        var tmp: [32]u8 = undefined;
        const s = std.fmt.bufPrint(&tmp, "{d}", .{@abs(v)}) catch unreachable;
        if (v < 0) out.put("-");
        var i = s.len;
        while (i < width) : (i += 1) out.put(&[_]u8{pad});
        out.put(s);
    }

    fn conv(out: *Out, c: u8, tm: *const Tm) void {
        switch (c) {
            'a' => out.put(wdays[@intCast(@mod(tm.wday, 7))][0..3]),
            'A' => out.put(wdays[@intCast(@mod(tm.wday, 7))]),
            'b' => out.put(months[@intCast(@mod(tm.mon, 12))][0..3]),
            'B' => out.put(months[@intCast(@mod(tm.mon, 12))]),
            'c' => {
                if (is_windows) {
                    // UCRT C locale: "%m/%d/%y %H:%M:%S"
                    out.conv('x', tm);
                    out.put(" ");
                    out.conv('X', tm);
                } else {
                    out.conv('a', tm);
                    out.put(" ");
                    out.conv('b', tm);
                    out.put(" ");
                    out.putnum(tm.mday, 2, ' ');
                    out.put(" ");
                    out.conv('X', tm);
                    out.put(" ");
                    out.conv('Y', tm);
                }
            },
            'd' => out.putnum(tm.mday, 2, '0'),
            'H' => out.putnum(tm.hour, 2, '0'),
            'I' => out.putnum(if (@mod(tm.hour, 12) == 0) 12 else @mod(tm.hour, 12), 2, '0'),
            'j' => out.putnum(@as(i64, tm.yday) + 1, 3, '0'),
            'm' => out.putnum(@as(i64, tm.mon) + 1, 2, '0'),
            'M' => out.putnum(tm.min, 2, '0'),
            'p' => out.put(if (tm.hour < 12) "AM" else "PM"),
            'S' => out.putnum(tm.sec, 2, '0'),
            'U' => out.putnum(@divTrunc(@as(i64, tm.yday) + 7 - tm.wday, 7), 2, '0'),
            'w' => out.putnum(tm.wday, 1, '0'),
            'W' => out.putnum(@divTrunc(@as(i64, tm.yday) + 7 - @mod(@as(i64, tm.wday) + 6, 7), 7), 2, '0'),
            'x' => {
                out.conv('m', tm);
                out.put("/");
                out.conv('d', tm);
                out.put("/");
                out.conv('y', tm);
            },
            'X' => {
                out.conv('H', tm);
                out.put(":");
                out.conv('M', tm);
                out.put(":");
                out.conv('S', tm);
            },
            'y' => out.putnum(@mod(@as(i64, tm.year) + 1900, 100), 2, '0'),
            'Y' => out.putnum(@as(i64, tm.year) + 1900, 1, '0'),
            'z', 'Z' => {
                var zone = tm.*;
                if (is_windows) windowsZoneFor(tm, &zone);
                if (c == 'Z') {
                    out.put(zone.zoneName());
                } else {
                    const off = zone.gmtoff;
                    out.put(if (off < 0) "-" else "+");
                    const a: i64 = @intCast(@abs(off));
                    out.putnum(@divTrunc(a, 3600) * 100 + @mod(@divTrunc(a, 60), 60), 4, '0');
                }
            },
            '%' => out.put("%"),
            else => {},
        }
    }
};

/// Formats one conversion `c` into buf (truncating); returns the bytes written.
pub fn strftime1(buf: []u8, c: u8, tm: *const Tm) usize {
    var out: Out = .{ .buf = buf };
    out.conv(c, tm);
    return out.n;
}
