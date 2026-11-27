//! OS library (upstream VM/src/loslib.cpp), translated to Zig.
//!
//! Portions derived from Luau (MIT, Copyright (c) 2019-2025 Roblox Corporation) and
//! Lua 5.x (MIT, Copyright (C) 1994-2008 Lua.org, PUC-Rio).
//!
//! Upstream Luau's os library is time-only (clock, date, difftime, time). Calendar
//! conversion lives in ostime.zig. Not opened by the p4pc restricted sandbox.

const std = @import("std");
const api = @import("api.zig");
const aux = @import("auxiliary.zig");
const ostime = @import("ostime.zig");
const State = api.State;

const LUA_STRFTIMEOPTIONS = "aAbBcdHIjmMpSUwWxXyYzZ%";

/// (time_t)d with x86-64 cvttsd2si semantics for out-of-range values.
fn d2time(d: f64) i64 {
    if (std.math.isNan(d) or d >= 9223372036854775808.0 or d < -9223372036854775808.0)
        return std.math.minInt(i64);
    return @intFromFloat(@trunc(d));
}

fn os_timegm(tm: *const ostime.Tm) ?i64 {
    // Julian day number calculation (int arithmetic as upstream)
    const day: i64 = tm.mday;
    const month: i64 = @as(i64, tm.mon) + 1;
    const year: i64 = @as(i64, tm.year) + 1900;

    // year adjustment, pretend that it starts in March
    var a: i64 = if (@rem(tm.mon, 12) < 2) 1 else 0;
    // also adjust for out-of-range month numbers in input
    a -= @divTrunc(tm.mon, 12);

    const y = year + 4800 - a;
    const m = month + (12 * a) - 3;

    const julianday = day + @divTrunc(153 * m + 2, 5) + (365 * y) + @divTrunc(y, 4) - @divTrunc(y, 100) + @divTrunc(y, 400) - 32045;

    const utcstartasjulianday: i64 = 2440588; // Jan 1st 1970 offset in Julian calendar
    const utcstartasjuliansecond: i64 = utcstartasjulianday * 86400; // same in seconds

    // fail the dates before UTC start
    if (julianday < utcstartasjulianday) return null;

    const daysecond = @as(i64, tm.hour) * 3600 + @as(i64, tm.min) * 60 + tm.sec;
    const julianseconds = julianday * 86400 + daysecond;

    if (julianseconds < utcstartasjuliansecond) return null;

    return julianseconds - utcstartasjuliansecond;
}

fn os_clock(L: *State) c_int {
    api.pushnumber(L, api.clock());
    return 1;
}

fn setfield(L: *State, key: [*:0]const u8, value: c_int) void {
    api.pushinteger(L, value);
    api.setfield(L, -2, key);
}

fn setboolfield(L: *State, key: [*:0]const u8, value: c_int) void {
    if (value < 0) return; // undefined? does not set field
    api.pushboolean(L, value != 0);
    api.setfield(L, -2, key);
}

fn getboolfield(L: *State, key: [*:0]const u8) c_int {
    _ = api.rawgetfield(L, -1, key);
    const res: c_int = if (api.isnil(L, -1)) -1 else @intFromBool(api.toboolean(L, -1));
    api.pop(L, 1);
    return res;
}

fn getfield(L: *State, key: [*:0]const u8, d: c_int) c_int {
    _ = api.rawgetfield(L, -1, key);
    var res: c_int = undefined;
    if (api.isnumber(L, -1)) {
        res = api.tointeger(L, -1);
    } else {
        if (d < 0) aux.errorf(L, "field '{s}' missing in date table", .{api.span(key)});
        res = d;
    }
    api.pop(L, 1);
    return res;
}

fn os_date(L: *State) c_int {
    const full = aux.optlstring(L, 1, "%c");
    // luaL_optstring: a C string
    var s = full[0 .. std.mem.indexOfScalar(u8, full, 0) orelse full.len];
    const t: i64 = if (api.isnoneornil(L, 2)) ostime.now() else d2time(aux.checknumber(L, 2));

    const utc = s.len > 0 and s[0] == '!';
    if (utc) s = s[1..]; // skip `!'
    // on Windows, localtime() fails with dates before epoch start so we disallow that
    const stm: ?ostime.Tm = if (utc) ostime.gmtime(t) else if (t < 0) null else ostime.localtime(t);
    const tm = stm orelse { // invalid date?
        api.pushnil(L);
        return 1;
    };

    if (std.mem.eql(u8, s, "*t")) {
        api.createtable(L, 0, 9); // 9 = number of fields
        setfield(L, "sec", tm.sec);
        setfield(L, "min", tm.min);
        setfield(L, "hour", tm.hour);
        setfield(L, "day", tm.mday);
        setfield(L, "month", tm.mon + 1);
        setfield(L, "year", tm.year +% 1900);
        setfield(L, "wday", tm.wday + 1);
        setfield(L, "yday", tm.yday + 1);
        setboolfield(L, "isdst", tm.isdst);
    } else {
        var b: aux.Strbuf = undefined;
        b.init(L);
        var i: usize = 0;
        while (i < s.len) : (i += 1) {
            if (s[i] != '%' or i + 1 == s.len) { // no conversion specifier?
                b.addchar(s[i]);
            } else if (std.mem.indexOfScalar(u8, LUA_STRFTIMEOPTIONS, s[i + 1]) == null) {
                aux.argerror(L, 1, "invalid conversion specifier");
            } else {
                var buff: [200]u8 = undefined; // should be big enough for any conversion result
                i += 1;
                const reslen = ostime.strftime1(&buff, s[i], &tm);
                b.addlstring(buff[0..reslen]);
            }
        }
        b.pushresult();
    }
    return 1;
}

fn os_time(L: *State) c_int {
    const t: ?i64 = if (api.isnoneornil(L, 1)) // called without args?
        ostime.now() // get current time
    else blk: {
        var ts = ostime.Tm{};
        aux.checktype(L, 1, api.TTABLE);
        api.settop(L, 1); // make sure table is at the top
        ts.sec = getfield(L, "sec", 0);
        ts.min = getfield(L, "min", 0);
        ts.hour = getfield(L, "hour", 12);
        ts.mday = getfield(L, "day", -1);
        ts.mon = getfield(L, "month", -1) -% 1;
        ts.year = getfield(L, "year", -1) -% 1900;
        ts.isdst = getboolfield(L, "isdst");

        // Note: upstream Lua uses mktime() here which assumes input is local time, but we prefer UTC for consistency
        break :blk os_timegm(&ts);
    };
    if (t) |v| {
        api.pushnumber(L, @floatFromInt(v));
    } else {
        api.pushnil(L);
    }
    return 1;
}

fn os_difftime(L: *State) c_int {
    const a = d2time(aux.checknumber(L, 1));
    const b = d2time(aux.optnumber(L, 2, 0));
    // difftime((time_t)a, (time_t)b) on 64-bit time_t
    api.pushnumber(L, @floatFromInt(@as(i128, a) - b));
    return 1;
}

const syslib = [_]aux.Entry{
    aux.entry("clock", os_clock),
    aux.entry("date", os_date),
    aux.entry("difftime", os_difftime),
    aux.entry("time", os_time),
};

pub fn open(L: *State) c_int {
    aux.register(L, "os", &syslib);
    return 1;
}
