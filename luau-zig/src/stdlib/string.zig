//! String library (upstream VM/src/lstrlib.cpp), translated to Zig.
//!
//! Portions derived from Luau (MIT, Copyright (c) 2019-2025 Roblox Corporation) and
//! Lua 5.x (MIT, Copyright (C) 1994-2008 Lua.org, PUC-Rio).
//!
//! Upstream relies on Luau strings being NUL-terminated (reads at the end pointer yield
//! '\0'); `at` reproduces that for slices. Both variants of the LuauOptimizeStringSplit
//! and LuauOptimizeStringGsub fast flags (stdlib/flags.zig) are implemented.

const std = @import("std");
const api = @import("api.zig");
const aux = @import("auxiliary.zig");
const ctype = @import("ctype.zig");
const cprintf = @import("cprintf.zig");
const flags = @import("flags.zig");
const State = api.State;
const Strbuf = aux.Strbuf;

const INT_MAX: i64 = std.math.maxInt(c_int);

/// Byte at index i, or '\0' at/after the end (Luau strings carry a terminating NUL).
inline fn at(s: []const u8, i: usize) u8 {
    return if (i < s.len) s[i] else 0;
}

fn str_len(L: *State) c_int {
    const s = aux.checklstring(L, 1);
    api.pushinteger(L, @intCast(s.len));
    return 1;
}

/// relative string position: negative means back from end
fn posrelat(pos: c_int, len: usize) c_int {
    var p: i64 = pos;
    if (p < 0) p += @as(i64, @intCast(len)) + 1;
    return if (p >= 0) @intCast(p) else 0;
}

fn str_sub(L: *State) c_int {
    const s = aux.checklstring(L, 1);
    const l: c_int = @intCast(s.len);
    var start = posrelat(aux.checkinteger(L, 2), s.len);
    var end = posrelat(aux.optinteger(L, 3, -1), s.len);
    if (start < 1) start = 1;
    if (end > l) end = l;
    if (start <= end) {
        api.pushlstring(L, s[@intCast(start - 1)..@intCast(end)]);
    } else {
        api.pushlstring(L, "");
    }
    return 1;
}

fn str_reverse(L: *State) c_int {
    const s = aux.checklstring(L, 1);
    var b: Strbuf = undefined;
    const ptr = b.initsize(L, s.len);
    for (s, 0..) |c, i| ptr[s.len - 1 - i] = c;
    b.pushresultsize(s.len);
    return 1;
}

fn str_lower(L: *State) c_int {
    const s = aux.checklstring(L, 1);
    var b: Strbuf = undefined;
    const ptr = b.initsize(L, s.len);
    for (s, 0..) |c, i| ptr[i] = ctype.tolower(c);
    b.pushresultsize(s.len);
    return 1;
}

fn str_upper(L: *State) c_int {
    const s = aux.checklstring(L, 1);
    var b: Strbuf = undefined;
    const ptr = b.initsize(L, s.len);
    for (s, 0..) |c, i| ptr[i] = ctype.toupper(c);
    b.pushresultsize(s.len);
    return 1;
}

fn str_rep(L: *State) c_int {
    const s = aux.checklstring(L, 1);
    const n = aux.checkinteger(L, 2);

    if (n <= 0) {
        api.pushlstring(L, "");
        return 1;
    }

    const un: usize = @intCast(n);
    if (s.len > api.MAXSSIZE / un) // may overflow?
        aux.errorf(L, "resulting string too large", .{});

    const total = s.len * un;
    var b: Strbuf = undefined;
    const ptr = b.initsize(L, total);

    @memcpy(ptr[0..s.len], s);
    var filled = s.len;
    // use the increasing 'pattern' inside our target buffer to fill the next part
    var step = s.len;
    while (step < total - filled) {
        @memcpy(ptr[filled..][0..step], ptr[0..step]);
        filled += step;
        step <<= 1;
    }
    // fill tail
    const left = total - filled;
    @memcpy(ptr[filled..][0..left], ptr[0..left]);

    b.pushresultsize(total);
    return 1;
}

fn str_byte(L: *State) c_int {
    const s = aux.checklstring(L, 1);
    const l: c_int = @intCast(s.len);
    var posi = posrelat(aux.optinteger(L, 2, 1), s.len);
    var pose = posrelat(aux.optinteger(L, 3, posi), s.len);
    if (posi <= 0) posi = 1;
    if (pose > l) pose = l;
    if (posi > pose) return 0; // empty interval; return no values
    const n = pose - posi + 1;
    if (@as(i64, posi) + n <= pose) // overflow?
        aux.errorf(L, "string slice too long", .{});
    aux.checkstack(L, n, "string slice too long");
    var i: c_int = 0;
    while (i < n) : (i += 1)
        api.pushinteger(L, s[@intCast(posi + i - 1)]);
    return n;
}

fn str_char(L: *State) c_int {
    const n = api.gettop(L); // number of arguments
    var b: Strbuf = undefined;
    const ptr = b.initsize(L, @intCast(n));
    var i: c_int = 1;
    while (i <= n) : (i += 1) {
        const c = aux.checkinteger(L, i);
        aux.argcheck(L, (c & 0xff) == c, i, "invalid value");
        ptr[@intCast(i - 1)] = @intCast(c);
    }
    b.pushresultsize(@intCast(n));
    return 1;
}

// =======================================================================================
// PATTERN MATCHING

const CAP_UNFINISHED: isize = -1;
const CAP_POSITION: isize = -2;
const L_ESC = '%';
const SPECIALS = "^$*+?.([%-";

const MatchState = struct {
    matchdepth: c_int, // control for recursive depth (to avoid C stack overflow)
    src: []const u8, // source string (src_init .. src_end)
    pat: []const u8, // pattern (.. p_end)
    L: *State,
    level: c_int, // total number of captures (finished or unfinished)
    capture: [api.MAXCAPTURES]struct { init: usize, len: isize },

    inline fn pc(ms: *const MatchState, p: usize) u8 {
        return at(ms.pat, p);
    }
    inline fn sc(ms: *const MatchState, s: usize) u8 {
        return at(ms.src, s);
    }
};

fn check_capture(ms: *MatchState, l_: c_int) usize {
    const l = l_ - '1';
    if (l < 0 or l >= ms.level or ms.capture[@intCast(l)].len == CAP_UNFINISHED)
        aux.errorf(ms.L, "invalid capture index %{d}", .{l + 1});
    return @intCast(l);
}

fn capture_to_close(ms: *MatchState) usize {
    var level = ms.level - 1;
    while (level >= 0) : (level -= 1) {
        if (ms.capture[@intCast(level)].len == CAP_UNFINISHED) return @intCast(level);
    }
    aux.errorf(ms.L, "invalid pattern capture", .{});
}

fn classend(ms: *MatchState, p_: usize) usize {
    var p = p_;
    const c = ms.pc(p);
    p += 1;
    switch (c) {
        L_ESC => {
            if (p == ms.pat.len)
                aux.errorf(ms.L, "malformed pattern (ends with '%')", .{});
            return p + 1;
        },
        '[' => {
            if (ms.pc(p) == '^') p += 1;
            while (true) { // look for a `]'
                if (p == ms.pat.len)
                    aux.errorf(ms.L, "malformed pattern (missing ']')", .{});
                const cc = ms.pc(p);
                p += 1;
                if (cc == L_ESC and p < ms.pat.len) p += 1; // skip escapes (e.g. `%]')
                if (ms.pc(p) == ']') break;
            }
            return p + 1;
        },
        else => return p,
    }
}

fn match_class(c: u8, cl: u8) bool {
    const res = switch (ctype.tolower(cl)) {
        'a' => ctype.isalpha(c),
        'c' => ctype.iscntrl(c),
        'd' => ctype.isdigit(c),
        'g' => ctype.isgraph(c),
        'l' => ctype.islower(c),
        'p' => ctype.ispunct(c),
        's' => ctype.isspace(c),
        'u' => ctype.isupper(c),
        'w' => ctype.isalnum(c),
        'x' => ctype.isxdigit(c),
        'z' => c == 0, // deprecated option
        else => return cl == c,
    };
    return if (ctype.islower(cl)) res else !res;
}

/// p points at '[', ec at the closing ']'.
fn matchbracketclass(ms: *const MatchState, c: u8, p_: usize, ec: usize) bool {
    var p = p_;
    var sig = true;
    if (ms.pc(p + 1) == '^') {
        sig = false;
        p += 1; // skip the `^'
    }
    while (true) {
        p += 1;
        if (p >= ec) break;
        if (ms.pc(p) == L_ESC) {
            p += 1;
            if (match_class(c, ms.pc(p))) return sig;
        } else if (ms.pc(p + 1) == '-' and p + 2 < ec) {
            p += 2;
            if (ms.pc(p - 2) <= c and c <= ms.pc(p)) return sig;
        } else if (ms.pc(p) == c) {
            return sig;
        }
    }
    return !sig;
}

fn singlematch(ms: *const MatchState, s: usize, p: usize, ep: usize) bool {
    if (s >= ms.src.len) return false;
    const c = ms.src[s];
    return switch (ms.pc(p)) {
        '.' => true, // matches any char
        L_ESC => match_class(c, ms.pc(p + 1)),
        '[' => matchbracketclass(ms, c, p, ep - 1),
        else => ms.pc(p) == c,
    };
}

fn matchbalance(ms: *MatchState, s_: usize, p: usize) ?usize {
    if (p + 1 >= ms.pat.len)
        aux.errorf(ms.L, "malformed pattern (missing arguments to '%b')", .{});
    var s = s_;
    if (ms.sc(s) != ms.pc(p)) return null;
    const b = ms.pc(p);
    const e = ms.pc(p + 1);
    var cont: c_int = 1;
    while (true) {
        s += 1;
        if (s >= ms.src.len) break;
        const c = ms.src[s];
        if (c == e) {
            cont -= 1;
            if (cont == 0) return s + 1;
        } else if (c == b) {
            cont += 1;
        }
    }
    return null; // string ends out of balance
}

fn max_expand(ms: *MatchState, s: usize, p: usize, ep: usize) ?usize {
    var i: usize = 0; // counts maximum expand for item
    while (singlematch(ms, s + i, p, ep)) i += 1;
    // keeps trying to match with the maximum repetitions
    while (true) {
        if (match(ms, s + i, ep + 1)) |res| return res;
        if (i == 0) return null;
        i -= 1; // else didn't match; reduce 1 repetition to try again
    }
}

fn min_expand(ms: *MatchState, s_: usize, p: usize, ep: usize) ?usize {
    var s = s_;
    while (true) {
        if (match(ms, s, ep + 1)) |res| return res;
        if (singlematch(ms, s, p, ep)) {
            s += 1; // try with one more repetition
        } else {
            return null;
        }
    }
}

fn start_capture(ms: *MatchState, s: usize, p: usize, what: isize) ?usize {
    const level = ms.level;
    if (level >= api.MAXCAPTURES) aux.errorf(ms.L, "too many captures", .{});
    ms.capture[@intCast(level)] = .{ .init = s, .len = what };
    ms.level = level + 1;
    const res = match(ms, s, p);
    if (res == null) ms.level -= 1; // undo capture
    return res;
}

fn end_capture(ms: *MatchState, s: usize, p: usize) ?usize {
    const l = capture_to_close(ms);
    ms.capture[l].len = @intCast(s - ms.capture[l].init); // close capture
    const res = match(ms, s, p);
    if (res == null) ms.capture[l].len = CAP_UNFINISHED; // undo capture
    return res;
}

fn match_capture(ms: *MatchState, s: usize, l_: u8) ?usize {
    const l = check_capture(ms, l_);
    const len = ms.capture[l].len;
    if (len < 0) return null; // position capture: (size_t)len is huge, never fits
    const ulen: usize = @intCast(len);
    const init = ms.capture[l].init;
    if (ms.src.len - s >= ulen and std.mem.eql(u8, ms.src[init..][0..ulen], ms.src[s..][0..ulen]))
        return s + ulen;
    return null;
}

fn match(ms: *MatchState, s_: usize, p_: usize) ?usize {
    if (ms.matchdepth == 0) aux.errorf(ms.L, "pattern too complex", .{});
    ms.matchdepth -= 1;

    // this interrupt is not yieldable
    api.interruptnonyieldable(ms.L);

    var s = s_;
    var p = p_;
    const result: ?usize = loop: while (true) { // init: (tail calls)
        if (p == ms.pat.len) break :loop s; // end of pattern
        var dflt = false;
        switch (ms.pc(p)) {
            '(' => { // start capture
                if (ms.pc(p + 1) == ')') // position capture?
                    break :loop start_capture(ms, s, p + 2, CAP_POSITION)
                else
                    break :loop start_capture(ms, s, p + 1, CAP_UNFINISHED);
            },
            ')' => break :loop end_capture(ms, s, p + 1), // end capture
            '$' => {
                if (p + 1 != ms.pat.len) { // is the `$' the last char in pattern?
                    dflt = true; // no; go to default
                } else {
                    break :loop if (s == ms.src.len) s else null; // check end of string
                }
            },
            L_ESC => switch (ms.pc(p + 1)) { // escaped sequences not in the format class[*+?-]?
                'b' => { // balanced string?
                    if (matchbalance(ms, s, p + 2)) |ns| {
                        s = ns;
                        p += 4;
                        continue :loop;
                    }
                    break :loop null;
                },
                'f' => { // frontier?
                    p += 2;
                    if (ms.pc(p) != '[')
                        aux.errorf(ms.L, "missing '[' after '%f' in pattern", .{});
                    const ep = classend(ms, p); // points to what is next
                    const previous: u8 = if (s == 0) 0 else ms.src[s - 1];
                    if (!matchbracketclass(ms, previous, p, ep - 1) and matchbracketclass(ms, ms.sc(s), p, ep - 1)) {
                        p = ep;
                        continue :loop;
                    }
                    break :loop null; // match failed
                },
                '0'...'9' => { // capture results (%0-%9)?
                    if (match_capture(ms, s, ms.pc(p + 1))) |ns| {
                        s = ns;
                        p += 2;
                        continue :loop;
                    }
                    break :loop null;
                },
                else => dflt = true,
            },
            else => dflt = true,
        }
        std.debug.assert(dflt);
        // pattern class plus optional suffix
        const ep = classend(ms, p); // points to optional suffix
        if (!singlematch(ms, s, p, ep)) { // does not match at least once?
            const e = ms.pc(ep);
            if (e == '*' or e == '?' or e == '-') { // accept empty?
                p = ep + 1;
                continue :loop;
            }
            break :loop null; // '+' or no suffix: fail
        }
        // matched once
        switch (ms.pc(ep)) { // handle optional suffix
            '?' => { // optional
                if (match(ms, s + 1, ep + 1)) |res| break :loop res;
                p = ep + 1;
                continue :loop;
            },
            '+' => break :loop max_expand(ms, s + 1, p, ep), // 1 or more repetitions
            '*' => break :loop max_expand(ms, s, p, ep), // 0 or more repetitions
            '-' => break :loop min_expand(ms, s, p, ep), // 0 or more repetitions (minimum)
            else => { // no suffix
                s += 1;
                p = ep;
                continue :loop;
            },
        }
    } else unreachable;
    ms.matchdepth += 1;
    return result;
}

fn lmemfind(s1: []const u8, s2: []const u8) ?usize {
    if (s2.len == 0) return 0; // empty strings are everywhere
    if (s2.len > s1.len) return null; // avoids a negative `l1'
    return std.mem.indexOf(u8, s1, s2);
}

fn push_onecapture(ms: *MatchState, i: c_int, s: usize, e: usize) void {
    if (i >= ms.level) {
        if (i == 0) { // ms->level == 0, too
            api.pushlstring(ms.L, ms.src[s..e]); // add whole match
        } else {
            aux.errorf(ms.L, "invalid capture index", .{});
        }
    } else {
        const cap = ms.capture[@intCast(i)];
        if (cap.len == CAP_UNFINISHED) aux.errorf(ms.L, "unfinished capture", .{});
        if (cap.len == CAP_POSITION) {
            api.pushinteger(ms.L, @as(c_int, @intCast(cap.init)) + 1);
        } else {
            api.pushlstring(ms.L, ms.src[cap.init..][0..@intCast(cap.len)]);
        }
    }
}

fn push_captures(ms: *MatchState, s: ?usize, e: usize) c_int {
    const nlevels: c_int = if (ms.level == 0 and s != null) 1 else ms.level;
    aux.checkstack(ms.L, nlevels, "too many captures");
    var i: c_int = 0;
    while (i < nlevels) : (i += 1) push_onecapture(ms, i, s orelse 0, e);
    return nlevels; // number of strings pushed
}

/// check whether pattern has no special characters (scans past embedded NULs)
fn nospecials(p: []const u8) bool {
    return std.mem.indexOfAny(u8, p, SPECIALS) == null;
}

fn prepstate(L: *State, s: []const u8, p: []const u8) MatchState {
    return .{
        .matchdepth = api.MAXCCALLS,
        .src = s,
        .pat = p,
        .L = L,
        .level = 0,
        .capture = undefined,
    };
}

fn reprepstate(ms: *MatchState) void {
    ms.level = 0;
    std.debug.assert(ms.matchdepth == api.MAXCCALLS);
}

fn str_find_aux(L: *State, find: bool) c_int {
    const s = aux.checklstring(L, 1);
    var p = aux.checklstring(L, 2);
    var init: i64 = posrelat(aux.optinteger(L, 3, 1), s.len);
    if (init < 1) {
        init = 1;
    } else if (init > @as(i64, @intCast(s.len)) + 1) { // start after string's end?
        api.pushnil(L); // cannot find anything
        return 1;
    }
    const start: usize = @intCast(init - 1);
    // explicit request or no special characters?
    if (find and (api.toboolean(L, 4) or nospecials(p))) {
        // do a plain search
        if (lmemfind(s[start..], p)) |off| {
            const pos = start + off;
            api.pushinteger(L, @intCast(pos + 1));
            api.pushinteger(L, @intCast(pos + p.len));
            return 2;
        }
    } else {
        var s1 = start;
        const anchor = p.len > 0 and p[0] == '^';
        if (anchor) p = p[1..]; // skip anchor character
        var ms = prepstate(L, s, p);
        while (true) {
            reprepstate(&ms);
            if (match(&ms, s1, 0)) |res| {
                if (find) {
                    api.pushinteger(L, @intCast(s1 + 1)); // start
                    api.pushinteger(L, @intCast(res)); // end
                    return push_captures(&ms, null, 0) + 2;
                } else {
                    return push_captures(&ms, s1, res);
                }
            }
            const cont = s1 < s.len and !anchor;
            s1 += 1;
            if (!cont) break;
        }
    }
    api.pushnil(L); // not found
    return 1;
}

fn str_find(L: *State) c_int {
    return str_find_aux(L, true);
}

fn str_match(L: *State) c_int {
    return str_find_aux(L, false);
}

fn gmatch_aux(L: *State) c_int {
    const s = api.tolstring(L, api.upvalueindex(1)).?;
    const p = api.tolstring(L, api.upvalueindex(2)).?;
    var ms = prepstate(L, s, p);
    var src: usize = @intCast(api.tointeger(L, api.upvalueindex(3)));
    while (src <= s.len) : (src += 1) {
        reprepstate(&ms);
        if (match(&ms, src, 0)) |e| {
            var newstart: c_int = @intCast(e);
            if (e == src) newstart += 1; // empty match? go at least one position
            api.pushinteger(L, newstart);
            api.replace(L, api.upvalueindex(3));
            return push_captures(&ms, src, e);
        }
    }
    return 0; // not found
}

fn gmatch(L: *State) c_int {
    _ = aux.checklstring(L, 1);
    _ = aux.checklstring(L, 2);
    api.settop(L, 2);
    api.pushinteger(L, 0);
    api.pushcclosure(L, aux.wrap(gmatch_aux), null, 3);
    return 1;
}

fn add_s(ms: *MatchState, b: *Strbuf, s: usize, e: usize) void {
    const news = api.tolstring(ms.L, 3).?;
    _ = b.prepsize(news.len);
    var i: usize = 0;
    while (i < news.len) : (i += 1) {
        if (news[i] != L_ESC) {
            b.addchar(news[i]);
        } else {
            i += 1; // skip ESC
            const c = at(news, i);
            if (!ctype.isdigit(c)) {
                if (c != L_ESC)
                    aux.errorf(ms.L, "invalid use of '%' in replacement string", .{});
                b.addchar(c);
            } else if (c == '0') {
                b.addlstring(ms.src[s..e]);
            } else {
                push_onecapture(ms, c - '1', s, e);
                b.addvalue(); // add capture to accumulated result
            }
        }
    }
}

fn add_value(ms: *MatchState, b: *Strbuf, s: usize, e: usize, tr: c_int) void {
    const L = ms.L;
    switch (tr) {
        api.TFUNCTION => {
            api.pushvalue(L, 3);
            const n = push_captures(ms, s, e);
            api.call(L, n, 1);
        },
        api.TTABLE => {
            push_onecapture(ms, 0, s, e);
            _ = api.gettable(L, 3);
        },
        else => { // LUA_TNUMBER or LUA_TSTRING
            add_s(ms, b, s, e);
            return;
        },
    }
    if (!api.toboolean(L, -1)) { // nil or false?
        api.pop(L, 1);
        api.pushlstring(L, ms.src[s..e]); // keep original text
    } else if (!api.isstring(L, -1)) {
        aux.errorf(L, "invalid replacement value (a {s})", .{aux.typenameOf(L, -1)});
    }
    b.addvalue(); // add result to accumulator
}

fn str_gsub(L: *State) c_int {
    const srcfull = aux.checklstring(L, 1);
    var p = aux.checklstring(L, 2);
    const tr = api.@"type"(L, 3);
    const max_s = aux.optinteger(L, 4, @as(c_int, @intCast(srcfull.len)) + 1);
    const anchor = p.len > 0 and p[0] == '^';
    var n: c_int = 0;
    aux.argexpected(L, tr == api.TNUMBER or tr == api.TSTRING or tr == api.TFUNCTION or tr == api.TTABLE, 3, "string/function/table");
    var b: Strbuf = undefined;
    b.init(L);
    if (anchor) p = p[1..]; // skip anchor character
    var ms = prepstate(L, srcfull, p);
    var src: usize = 0;

    // a non-empty pattern without special characters can only match its own text, so we can search for it directly
    if (flags.LuauOptimizeStringGsub and !anchor and p.len > 0 and isliteralpattern(p)) {
        // replacement strings without escapes can be appended as is; this is determined on the first match
        var plainrepl: i8 = -1;
        var repl: []const u8 = "";
        while (n < max_s) {
            const off = std.mem.indexOf(u8, srcfull[src..], p) orelse break;
            const e = src + off;
            n += 1;
            if (e != src) b.addlstring(srcfull[src..e]);
            if (plainrepl < 0) {
                const r: ?[]const u8 = if (tr == api.TSTRING or tr == api.TNUMBER) api.tolstring(L, 3) else null;
                plainrepl = if (r != null and std.mem.indexOfScalar(u8, r.?, L_ESC) == null) 1 else 0;
                if (r) |rr| repl = rr;
            }
            if (plainrepl == 1) {
                if (repl.len != 0) b.addlstring(repl);
            } else {
                reprepstate(&ms);
                add_value(&ms, &b, e, e + p.len, tr);
            }
            src = e + p.len;
        }
    } else {
        while (n < max_s) {
            reprepstate(&ms);
            const e = match(&ms, src, 0);
            if (e) |ee| {
                n += 1;
                add_value(&ms, &b, src, ee, tr);
            }
            if (e != null and e.? > src) { // non empty match?
                src = e.?; // skip it
            } else if (src < srcfull.len) {
                b.addchar(srcfull[src]);
                src += 1;
            } else break;
            if (anchor) break;
        }
    }
    b.addlstring(srcfull[src..]);
    b.pushresult();
    api.pushinteger(L, n); // number of substitutions
    return 2;
}

// =======================================================================================
// string.format

const FLAGS = "-+ #0";

fn addquoted(L: *State, b: *Strbuf, arg: c_int) void {
    const s = aux.checklstring(L, arg);
    _ = b.prepsize(s.len + 2);
    b.addchar('"');
    for (s) |c| {
        switch (c) {
            '"', '\\', '\n' => {
                b.addchar('\\');
                b.addchar(c);
            },
            '\r' => b.addlstring("\\r"),
            0 => b.addlstring("\\000"),
            else => b.addchar(c),
        }
    }
    b.addchar('"');
}

/// Validates a format item like upstream scanformat; returns the index of the conversion
/// character and the spec text (between '%' and the conversion).
fn scanformat(L: *State, fmt: []const u8, start: usize) usize {
    var p = start;
    while (at(fmt, p) != 0 and std.mem.indexOfScalar(u8, FLAGS, fmt[p]) != null) p += 1; // skip flags
    if (p - start >= FLAGS.len + 1)
        aux.errorf(L, "invalid format (repeated flags)", .{});
    if (ctype.isdigit(at(fmt, p))) p += 1; // skip width
    if (ctype.isdigit(at(fmt, p))) p += 1; // (2 digits at most)
    if (at(fmt, p) == '.') {
        p += 1;
        if (ctype.isdigit(at(fmt, p))) p += 1; // skip precision
        if (ctype.isdigit(at(fmt, p))) p += 1; // (2 digits at most)
    }
    if (ctype.isdigit(at(fmt, p)))
        aux.errorf(L, "invalid format (width or precision too long)", .{});
    return p;
}

/// (long long)d with x86-64 cvttsd2si semantics for out-of-range values (upstream's
/// reference builds).
fn d2i64(d: f64) i64 {
    if (std.math.isNan(d) or d >= 9223372036854775808.0 or d < -9223372036854775808.0)
        return std.math.minInt(i64);
    return @intFromFloat(@trunc(d));
}

/// (unsigned long long)d for d >= 0 as compiled by clang/gcc on x86-64.
fn d2u64(d: f64) u64 {
    if (d < 9223372036854775808.0) return @bitCast(d2i64(d));
    const r = d2i64(d - 9223372036854775808.0);
    return @as(u64, @bitCast(r)) ^ 0x8000000000000000;
}

/// (int)d with cvttsd2si semantics.
fn d2int(d: f64) c_int {
    if (std.math.isNan(d) or d >= 2147483648.0 or d <= -2147483649.0)
        return std.math.minInt(c_int);
    return @intFromFloat(@trunc(d));
}

fn str_format(L: *State) c_int {
    const top = api.gettop(L);
    var arg: c_int = 1;
    const fmt = aux.checklstring(L, arg);
    var b: Strbuf = undefined;
    b.init(L);
    var i: usize = 0;
    while (i < fmt.len) {
        if (fmt[i] != L_ESC) {
            b.addchar(fmt[i]);
            i += 1;
            continue;
        }
        i += 1;
        if (at(fmt, i) == L_ESC) {
            b.addchar(fmt[i]); // %%
            i += 1;
        } else if (at(fmt, i) == '*') {
            i += 1;
            arg += 1;
            if (arg > top) aux.errorf(L, "missing argument #{d}", .{arg});
            b.addvalueany(arg);
        } else { // format item
            arg += 1;
            if (arg > top) aux.errorf(L, "missing argument #{d}", .{arg});
            const specstart = i;
            const convpos = scanformat(L, fmt, i);
            const spectext = fmt[specstart..convpos];
            const conv = at(fmt, convpos);
            i = convpos + 1;
            const spec = cprintf.Spec.parse(spectext);
            var out = cprintf.Out{};
            switch (conv) {
                'c' => {
                    const c = d2int(aux.checknumber(L, arg));
                    cprintf.formatChar(&out, spec, @truncate(@as(c_uint, @bitCast(c))));
                    b.addlstring(out.slice());
                    continue; // skip the 'luaL_addlstring' at the end
                },
                'd', 'i' => {
                    const value: i64 = if (api.isinteger64(L, arg)) aux.checkinteger64(L, arg) else d2i64(aux.checknumber(L, arg));
                    cprintf.formatSigned(&out, spec, value);
                },
                'o', 'u', 'x', 'X' => {
                    var v: u64 = undefined;
                    if (api.isinteger64(L, arg)) {
                        v = @bitCast(aux.checkinteger64(L, arg));
                    } else {
                        const argValue = aux.checknumber(L, arg);
                        v = if (argValue < 0) @bitCast(d2i64(argValue)) else d2u64(argValue);
                    }
                    cprintf.formatUnsigned(&out, spec, conv, v);
                },
                'e', 'E', 'f', 'g', 'G' => {
                    cprintf.formatFloat(&out, spec, conv, aux.outf64(aux.checknumber(L, arg)));
                },
                'q' => {
                    addquoted(L, &b, arg);
                    continue; // skip the 'luaL_addlstring' at the end
                },
                's' => {
                    const s = aux.checklstring(L, arg);
                    // no precision and string is too long to be formatted, or no format necessary to begin with
                    if (spectext.len == 0 or (std.mem.indexOfScalar(u8, spectext, '.') == null and s.len >= 100)) {
                        b.addlstring(s);
                        continue; // skip the `luaL_addlstring' at the end
                    }
                    cprintf.formatString(&out, spec, s);
                },
                '*' => {
                    // %* is parsed above, so if we got here we must have %...*
                    aux.errorf(L, "'%*' does not take a form", .{});
                },
                else => { // also treat cases `pnLlh'
                    // the C message is built with %c: a NUL conversion character ends it
                    if (conv == 0)
                        aux.errorf(L, "invalid option '%", .{})
                    else
                        aux.errorf(L, "invalid option '%{c}' to 'format'", .{conv});
                },
            }
            // upstream appends strlen(buff): the item text ends at its first NUL
            const item = out.slice();
            b.addlstring(item[0 .. std.mem.indexOfScalar(u8, item, 0) orelse item.len]);
        }
    }
    b.pushresult();
    return 1;
}

/// checks if a pattern only matches its own text: ')' is not in SPECIALS but is still an
/// error in a pattern, so it takes the regular path; ']' only matters inside a set
fn isliteralpattern(p: []const u8) bool {
    return std.mem.indexOfAny(u8, p, SPECIALS ++ ")") == null;
}

fn str_split(L: *State) c_int {
    const haystack = aux.checklstring(L, 1);
    const needle = aux.optlstring(L, 2, ",");

    if (flags.LuauOptimizeStringSplit) return str_split_optimized(L, haystack, needle);

    var begin: usize = 0;
    var spanStart: usize = 0;
    var numMatches: c_int = 0;

    api.createtable(L, 0, 0);

    if (needle.len == 0) begin += 1;

    // Don't iterate the last needleLen - 1 bytes of the string - they are
    // impossible to be splits and would let us memcmp past the end of the buffer.
    if (needle.len <= haystack.len) {
        const last = haystack.len - needle.len;
        var iter = begin;
        while (iter <= last) : (iter += 1) {
            if (std.mem.eql(u8, haystack[iter..][0..needle.len], needle)) {
                numMatches += 1;
                api.pushinteger(L, numMatches);
                api.pushlstring(L, haystack[spanStart..iter]);
                api.settable(L, -3);

                spanStart = iter + needle.len;
                if (needle.len > 0) iter += needle.len - 1;
            }
        }
    }

    if (needle.len > 0) {
        numMatches += 1;
        api.pushinteger(L, numMatches);
        api.pushlstring(L, haystack[spanStart..]);
        api.settable(L, -3);
    }

    return 1;
}

fn str_split_optimized(L: *State, haystack: []const u8, needle: []const u8) c_int {
    var spanStart: usize = 0;
    var numMatches: c_int = 0;

    if (needle.len == 0) {
        // empty separator splits the string into individual characters, so the result size is known up front
        api.createtable(L, @intCast(haystack.len), 0);
        for (0..haystack.len) |i| {
            api.pushlstring(L, haystack[i..][0..1]);
            numMatches += 1;
            api.rawseti(L, -2, numMatches);
        }
        return 1;
    } else if (needle.len == 1) {
        // every occurrence of a single character separator is a split, so we can cheaply count them up front
        const sep = needle[0];
        const count: c_int = @intCast(std.mem.count(u8, haystack, needle) + 1);
        api.createtable(L, count, 0);
        while (std.mem.indexOfScalarPos(u8, haystack, spanStart, sep)) |found| {
            api.pushlstring(L, haystack[spanStart..found]);
            numMatches += 1;
            api.rawseti(L, -2, numMatches);
            spanStart = found + 1;
        }
    } else {
        api.createtable(L, 0, 0);
        if (needle.len <= haystack.len) {
            const last = haystack.len - needle.len;
            var iter: usize = 0;
            while (iter <= last) {
                if (std.mem.eql(u8, haystack[iter..][0..needle.len], needle)) {
                    api.pushlstring(L, haystack[spanStart..iter]);
                    numMatches += 1;
                    api.rawseti(L, -2, numMatches);
                    spanStart = iter + needle.len;
                    iter = spanStart;
                } else {
                    iter += 1;
                }
            }
        }
    }

    api.pushlstring(L, haystack[spanStart..]);
    numMatches += 1;
    api.rawseti(L, -2, numMatches);
    return 1;
}

// =======================================================================================
// PACK/UNPACK

const PACKPADBYTE: u8 = 0x00; // value used for padding
const MAXINTSIZE = 16; // maximum size for the binary representation of an integer
const NB = 8; // number of bits in a character
const MC: u64 = (1 << NB) - 1; // mask for one character (NB 1's)
const SZINT = 8; // internal size of integers used for pack/unpack (long long)
const MAXALIGN = 8;
const nativelittle = @import("builtin").cpu.arch.endian() == .little;

const Header = struct {
    L: *State,
    islittle: bool,
    maxalign: c_int,
};

const KOption = enum {
    Kint, // signed integers
    Kuint, // unsigned integers
    Kfloat, // floating-point numbers
    Kchar, // fixed-length strings
    Kstring, // strings with prefixed length
    Kzstr, // zero-terminated strings
    Kpadding, // padding
    Kpaddalign, // padding for alignment
    Knop, // no-op (configuration or spaces)
};

/// Format string cursor; the format is a C string (ends at the first NUL).
const Fmt = struct {
    s: []const u8,
    i: usize = 0,

    fn cur(f: *const Fmt) u8 {
        return if (f.i < f.s.len) f.s[f.i] else 0;
    }
};

fn digit(c: u8) bool {
    return '0' <= c and c <= '9';
}

fn getnum(h: *Header, fmt: *Fmt, df: c_int) c_int {
    if (!digit(fmt.cur())) return df; // no number? return default value
    var a: c_int = 0;
    while (true) {
        a = a * 10 + (fmt.cur() - '0');
        fmt.i += 1;
        if (!(digit(fmt.cur()) and a <= @divTrunc(std.math.maxInt(c_int) - 9, 10))) break;
    }
    if (@as(i64, a) > @as(i64, api.MAXSSIZE) or digit(fmt.cur()))
        aux.errorf(h.L, "size specifier is too large", .{});
    return a;
}

/// Read an integer numeral and raises an error if it is larger than the maximum size for integers.
fn getnumlimit(h: *Header, fmt: *Fmt, df: c_int) c_int {
    const sz = getnum(h, fmt, df);
    if (sz > MAXINTSIZE or sz <= 0)
        aux.errorf(h.L, "integral size ({d}) out of limits [1,{d}]", .{ sz, MAXINTSIZE });
    return sz;
}

fn initheader(L: *State) Header {
    return .{ .L = L, .islittle = nativelittle, .maxalign = 1 };
}

/// A classified format option and the number of bytes it occupies.
const Option = struct { kind: KOption, size: c_int = 0 };

/// Read and classify next option.
fn getoption(h: *Header, fmt: *Fmt) Option {
    const opt = fmt.cur();
    fmt.i += 1;
    switch (opt) {
        'b' => return .{ .kind = .Kint, .size = 1 },
        'B' => return .{ .kind = .Kuint, .size = 1 },
        'h' => return .{ .kind = .Kint, .size = 2 },
        'H' => return .{ .kind = .Kuint, .size = 2 },
        'l' => return .{ .kind = .Kint, .size = 8 },
        'L' => return .{ .kind = .Kuint, .size = 8 },
        'j' => return .{ .kind = .Kint, .size = 4 },
        'J' => return .{ .kind = .Kuint, .size = 4 },
        'T' => return .{ .kind = .Kuint, .size = 4 },
        'f' => return .{ .kind = .Kfloat, .size = 4 },
        'd' => return .{ .kind = .Kfloat, .size = 8 },
        'n' => return .{ .kind = .Kfloat, .size = 8 },
        'i' => return .{ .kind = .Kint, .size = getnumlimit(h, fmt, 4) },
        'I' => return .{ .kind = .Kuint, .size = getnumlimit(h, fmt, 4) },
        's' => return .{ .kind = .Kstring, .size = getnumlimit(h, fmt, 4) },
        'c' => {
            const size = getnum(h, fmt, -1);
            if (size == -1)
                aux.errorf(h.L, "missing size for format option 'c'", .{});
            return .{ .kind = .Kchar, .size = size };
        },
        'z' => return .{ .kind = .Kzstr },
        'x' => return .{ .kind = .Kpadding, .size = 1 },
        'X' => return .{ .kind = .Kpaddalign },
        ' ' => {},
        '<' => h.islittle = true,
        '>' => h.islittle = false,
        '=' => h.islittle = nativelittle,
        '!' => h.maxalign = getnumlimit(h, fmt, MAXALIGN),
        else => {
            // the C message is built with %c: a NUL option character ends it
            if (opt == 0)
                aux.errorf(h.L, "invalid format option '", .{})
            else
                aux.errorf(h.L, "invalid format option '{c}'", .{opt});
        },
    }
    return .{ .kind = .Knop };
}

/// An option plus the padding bytes needed before it to satisfy its alignment.
const Details = struct { kind: KOption, size: c_int, ntoalign: c_int };

/// Read, classify, and fill other details about the next option.
fn getdetails(h: *Header, totalsize: usize, fmt: *Fmt) Details {
    const opt = getoption(h, fmt);
    var alignment = opt.size; // usually, alignment follows size
    if (opt.kind == .Kpaddalign) { // 'X' gets alignment from following option
        if (fmt.cur() == 0)
            aux.argerror(h.L, 1, "invalid next option for option 'X'");
        const next = getoption(h, fmt);
        alignment = next.size;
        if (next.kind == .Kchar or alignment == 0)
            aux.argerror(h.L, 1, "invalid next option for option 'X'");
    }
    var ntoalign: c_int = 0;
    if (alignment > 1 and opt.kind != .Kchar) { // need alignment?
        if (alignment > h.maxalign) // enforce maximum alignment
            alignment = h.maxalign;
        if ((alignment & (alignment - 1)) != 0) // is 'align' not a power of 2?
            aux.argerror(h.L, 1, "format asks for alignment not power of 2");
        const a: usize = @intCast(alignment);
        ntoalign = @intCast((a - (totalsize & (a - 1))) & (a - 1));
    }
    return .{ .kind = opt.kind, .size = opt.size, .ntoalign = ntoalign };
}

/// Pack integer 'n' with 'size' bytes and 'islittle' endianness.
fn packint(b: *Strbuf, n_: u64, islittle: bool, size: usize, neg: bool) void {
    var buff: [MAXINTSIZE]u8 = undefined;
    var n = n_;
    buff[if (islittle) 0 else size - 1] = @intCast(n & MC); // first byte
    var i: usize = 1;
    while (i < size) : (i += 1) {
        n >>= NB;
        buff[if (islittle) i else size - 1 - i] = @intCast(n & MC);
    }
    if (neg and size > SZINT) { // negative number need sign extension?
        i = SZINT;
        while (i < size) : (i += 1) // correct extra bytes
            buff[if (islittle) i else size - 1 - i] = @intCast(MC);
    }
    b.addlstring(buff[0..size]); // add result to buffer
}

/// Copy 'size' bytes from 'src' to 'dest', correcting endianness if given 'islittle' is
/// different from native endianness.
fn copywithendian(dest: []u8, src: []const u8, size: usize, islittle: bool) void {
    if (islittle == nativelittle) {
        @memcpy(dest[0..size], src[0..size]);
    } else {
        var i: usize = 0;
        while (i < size) : (i += 1) dest[size - 1 - i] = src[i];
    }
}

fn str_pack(L: *State) c_int {
    var b: Strbuf = undefined;
    var fmt = Fmt{ .s = cstringPrefix(aux.checklstring(L, 1)) };
    var h = initheader(L);
    var arg: c_int = 1; // current argument to pack
    var totalsize: usize = 0; // accumulate total size of result
    api.pushnil(L); // mark to separate arguments from string buffer
    b.init(L);
    while (fmt.cur() != 0) {
        const d = getdetails(&h, totalsize, &fmt);
        const size = d.size;
        const ntoalign: usize = @intCast(d.ntoalign);
        totalsize += ntoalign + @as(usize, @intCast(size));
        for (0..ntoalign) |_| b.addchar(PACKPADBYTE); // fill alignment
        arg += 1;
        const usize_: usize = @intCast(size);
        switch (d.kind) {
            .Kint => { // signed integers
                const n = d2i64(aux.checknumber(L, arg));
                if (size < SZINT) { // need overflow check?
                    const lim: i64 = @as(i64, 1) << @intCast(size * NB - 1);
                    aux.argcheck(L, -lim <= n and n < lim, arg, "integer overflow");
                }
                packint(&b, @bitCast(n), h.islittle, usize_, n < 0);
            },
            .Kuint => { // unsigned integers
                const n = d2i64(aux.checknumber(L, arg));
                if (size < SZINT) // need overflow check?
                    aux.argcheck(L, @as(u64, @bitCast(n)) < (@as(u64, 1) << @intCast(size * NB)), arg, "unsigned overflow");
                packint(&b, @bitCast(n), h.islittle, usize_, false);
            },
            .Kfloat => { // floating-point options
                var buff: [MAXINTSIZE]u8 = undefined;
                const n = aux.checknumber(L, arg); // get argument
                if (size == 4) {
                    const f: f32 = aux.outf32(@floatCast(n));
                    const bytes = std.mem.asBytes(&f);
                    copywithendian(&buff, bytes, 4, h.islittle);
                } else {
                    const canon = aux.outf64(n);
                    const bytes = std.mem.asBytes(&canon);
                    copywithendian(&buff, bytes, usize_, h.islittle);
                }
                b.addlstring(buff[0..usize_]);
            },
            .Kchar => { // fixed-size string
                const s = aux.checklstring(L, arg);
                aux.argcheck(L, s.len <= usize_, arg, "string longer than given size");
                b.addlstring(s); // add string
                var len = s.len;
                while (len < usize_) : (len += 1) b.addchar(PACKPADBYTE); // pad extra space
            },
            .Kstring => { // strings with length count
                const s = aux.checklstring(L, arg);
                aux.argcheck(L, size >= @sizeOf(usize) or s.len < (@as(usize, 1) << @intCast(size * NB)), arg, "string length does not fit in given size");
                packint(&b, s.len, h.islittle, usize_, false); // pack length
                b.addlstring(s);
                totalsize += s.len;
            },
            .Kzstr => { // zero-terminated string
                const s = aux.checklstring(L, arg);
                aux.argcheck(L, std.mem.indexOfScalar(u8, s, 0) == null, arg, "string contains zeros");
                b.addlstring(s);
                b.addchar(0); // add zero at the end
                totalsize += s.len + 1;
            },
            .Kpadding => {
                b.addchar(PACKPADBYTE);
                arg -= 1; // undo increment
            },
            .Kpaddalign, .Knop => arg -= 1, // undo increment
        }
    }
    b.pushresult();
    return 1;
}

fn str_packsize(L: *State) c_int {
    var h = initheader(L);
    var fmt = Fmt{ .s = cstringPrefix(aux.checklstring(L, 1)) };
    var totalsize: usize = 0; // accumulate total size of result
    while (fmt.cur() != 0) {
        const d = getdetails(&h, totalsize, &fmt);
        aux.argcheck(L, d.kind != .Kstring and d.kind != .Kzstr, 1, "variable-length format");
        const sz: i64 = @as(i64, d.size) + d.ntoalign; // total space used by option
        aux.argcheck(L, @as(i64, @intCast(totalsize)) <= @as(i64, api.MAXSSIZE) - sz, 1, "format result too large");
        totalsize += @intCast(sz);
    }
    api.pushinteger(L, @intCast(totalsize));
    return 1;
}

/// Unpack an integer with 'size' bytes and 'islittle' endianness.
fn unpackint(L: *State, str: []const u8, islittle: bool, size: usize, issigned: bool) i64 {
    var res: u64 = 0;
    const limit = if (size <= SZINT) size else SZINT;
    var i: usize = limit;
    while (i > 0) {
        i -= 1;
        res <<= NB;
        res |= str[if (islittle) i else size - 1 - i];
    }
    if (size < SZINT) { // real size smaller than int?
        if (issigned) { // needs sign extension?
            const mask: u64 = @as(u64, 1) << @intCast(size * NB - 1);
            res = ((res ^ mask) -% mask); // do sign extension
        }
    } else if (size > SZINT) { // must check unread bytes
        const mask: u8 = if (!issigned or @as(i64, @bitCast(res)) >= 0) 0 else @intCast(MC);
        i = limit;
        while (i < size) : (i += 1) {
            if (str[if (islittle) i else size - 1 - i] != mask)
                aux.errorf(L, "{d}-byte integer does not fit into Lua Integer", .{size});
        }
    }
    return @bitCast(res);
}

fn str_unpack(L: *State) c_int {
    var h = initheader(L);
    var fmt = Fmt{ .s = cstringPrefix(aux.checklstring(L, 1)) };
    const data = aux.checklstring(L, 2);
    const ld = data.len;
    var pos: i64 = @as(i64, posrelat(aux.optinteger(L, 3, 1), ld)) - 1;
    if (pos < 0) pos = 0;
    var n: c_int = 0; // number of results
    aux.argcheck(L, @as(usize, @intCast(pos)) <= ld, 3, "initial position out of string");
    while (fmt.cur() != 0) {
        const d = getdetails(&h, @intCast(pos), &fmt);
        const upos: usize = @intCast(pos);
        const usz: usize = @intCast(d.size);
        aux.argcheck(L, @as(usize, @intCast(d.ntoalign)) + usz <= ld - upos, 2, "data string too short");
        pos += d.ntoalign; // skip alignment
        const p: usize = @intCast(pos);
        // stack space for item + next position
        aux.checkstack(L, 2, "too many results");
        n += 1;
        switch (d.kind) {
            .Kint => {
                const res = unpackint(L, data[p..], h.islittle, usz, true);
                api.pushnumber(L, @floatFromInt(res));
            },
            .Kuint => {
                const res: u64 = @bitCast(unpackint(L, data[p..], h.islittle, usz, false));
                api.pushnumber(L, @floatFromInt(res));
            },
            .Kfloat => {
                var buff: [8]u8 = undefined;
                copywithendian(&buff, data[p..], usz, h.islittle);
                const num: f64 = if (usz == 4)
                    @as(f64, @as(f32, @bitCast(buff[0..4].*)))
                else
                    @bitCast(buff[0..8].*);
                api.pushnumber(L, num);
            },
            .Kchar => {
                api.pushlstring(L, data[p..][0..usz]);
            },
            .Kstring => {
                const len: usize = @bitCast(unpackint(L, data[p..], h.islittle, usz, false));
                aux.argcheck(L, len <= ld - p - usz, 2, "data string too short");
                api.pushlstring(L, data[p + usz ..][0..len]);
                pos += @as(i64, @intCast(len)); // skip string
            },
            .Kzstr => {
                // strlen(data + pos): the terminating NUL of the Luau string bounds it
                const len = std.mem.indexOfScalar(u8, data[p..], 0) orelse (ld - p);
                aux.argcheck(L, p + len < ld, 2, "unfinished string for format 'z'");
                api.pushlstring(L, data[p..][0..len]);
                pos += @as(i64, @intCast(len)) + 1; // skip string plus final '\0'
            },
            .Kpaddalign, .Kpadding, .Knop => n -= 1, // undo increment
        }
        pos += d.size;
    }
    api.pushinteger(L, @intCast(pos + 1)); // next position
    return n + 1;
}

/// C-string view of a format argument (ends at the first embedded NUL).
fn cstringPrefix(s: []const u8) []const u8 {
    return s[0 .. std.mem.indexOfScalar(u8, s, 0) orelse s.len];
}

// }======================================================

const strlib = [_]aux.Entry{
    aux.entry("byte", str_byte),
    aux.entry("char", str_char),
    aux.entry("find", str_find),
    aux.entry("format", str_format),
    aux.entry("gmatch", gmatch),
    aux.entry("gsub", str_gsub),
    aux.entry("len", str_len),
    aux.entry("lower", str_lower),
    aux.entry("match", str_match),
    aux.entry("rep", str_rep),
    aux.entry("reverse", str_reverse),
    aux.entry("sub", str_sub),
    aux.entry("upper", str_upper),
    aux.entry("split", str_split),
    aux.entry("pack", str_pack),
    aux.entry("packsize", str_packsize),
    aux.entry("unpack", str_unpack),
};

fn createmetatable(L: *State) void {
    api.createtable(L, 0, 1); // create metatable for strings
    api.pushlstring(L, ""); // dummy string
    api.pushvalue(L, -2);
    api.setmetatable(L, -2); // set string metatable
    api.pop(L, 1); // pop dummy string
    api.pushvalue(L, -2); // string library...
    api.setfield(L, -2, "__index"); // ...is the __index metamethod
    api.pop(L, 1); // pop metatable
}

pub fn open(L: *State) c_int {
    aux.register(L, "string", &strlib);
    createmetatable(L);
    return 1;
}
