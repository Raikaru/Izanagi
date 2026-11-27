//! Interned strings (port of upstream lstring.cpp, MIT; see object.zig header).

const std = @import("std");
const o = @import("object.zig");
const mem = @import("mem.zig");
const gc = @import("gc.zig");

const State = o.State;
const TString = o.TString;

pub const FIXEDBIT: u8 = 1 << 3;

pub inline fn hash(str: []const u8) u32 {
    return @import("string_hash.zig").hash(str);
}

inline fn sizestring(len: usize) usize {
    return @sizeOf(TString) + len + 1;
}

pub fn resize(L: *State, newsize: c_int) void {
    const g = L.global;
    const nsz: usize = @intCast(newsize);
    const newhash = mem.newArray(L, ?*TString, nsz, 0);
    for (0..nsz) |i| newhash[i] = null;
    const tb = &g.strt;
    var i: usize = 0;
    while (i < @as(usize, @intCast(tb.size))) : (i += 1) {
        var p = tb.hash[i];
        while (p) |s| {
            const next = s.hnext;
            const h1 = o.lmod(s.hash, @intCast(newsize));
            s.hnext = newhash[h1];
            newhash[h1] = s;
            p = next;
        }
    }
    if (tb.size > 0) mem.freeArray(L, ?*TString, tb.hash, @intCast(tb.size), 0);
    tb.size = newsize;
    tb.hash = newhash;
}

fn link(L: *State, ts: *TString, h: u32) void {
    const tb = &L.global.strt;
    const bucket = o.lmod(h, @intCast(tb.size));
    ts.hnext = tb.hash[bucket];
    tb.hash[bucket] = ts;
    tb.nuse += 1;
    if (tb.nuse > @as(u32, @intCast(tb.size)) and tb.size <= std.math.maxInt(c_int) / 2)
        resize(L, tb.size * 2);
}

fn newlstr(L: *State, str: []const u8, h: u32) *TString {
    if (str.len > o.MAXSSIZE) mem.toobig(L);
    const ts = mem.newGco(L, TString, sizestring(str.len), o.T_STRING);
    ts.atom = o.ATOM_UNDEF;
    ts.pad = 0;
    ts.hash = h;
    ts.len = @intCast(str.len);
    ts.hnext = null;
    const d = ts.data();
    if (str.len > 0) @memcpy(d[0..str.len], str);
    d[str.len] = 0;
    link(L, ts, h);
    return ts;
}

pub fn newlstr_(L: *State, str: []const u8) *TString {
    const h = hash(str);
    const g = L.global;
    var el = g.strt.hash[o.lmod(h, @intCast(g.strt.size))];
    while (el) |s| : (el = s.hnext) {
        if (s.len == str.len and std.mem.eql(u8, s.slice(), str)) {
            if (gc.isdead(g, &s.gch)) gc.changewhite(&s.gch);
            return s;
        }
    }
    return newlstr(L, str, h);
}

pub inline fn new(L: *State, str: []const u8) *TString {
    return newlstr_(L, str);
}

pub fn newz(L: *State, s: [*:0]const u8) *TString {
    return newlstr_(L, std.mem.span(s));
}

/// Starts a string buffer of `size` bytes that is not yet interned.
pub fn bufstart(L: *State, size: usize) *TString {
    if (size > o.MAXSSIZE) mem.toobig(L);
    const ts = mem.newGco(L, TString, sizestring(size), o.T_STRING);
    ts.atom = o.ATOM_UNDEF;
    ts.pad = 0;
    ts.hash = 0;
    ts.len = @intCast(size);
    ts.hnext = null;
    return ts;
}

/// Interns a buffer created by `bufstart`; returns an existing equal string if any
/// (the buffer then becomes garbage).
pub fn buffinish(L: *State, ts: *TString) *TString {
    const h = hash(ts.slice());
    const g = L.global;
    var el = g.strt.hash[o.lmod(h, @intCast(g.strt.size))];
    while (el) |s| : (el = s.hnext) {
        if (s.len == ts.len and std.mem.eql(u8, s.slice(), ts.slice())) {
            if (gc.isdead(g, &s.gch)) gc.changewhite(&s.gch);
            return s;
        }
    }
    ts.hash = h;
    ts.data()[ts.len] = 0;
    ts.atom = o.ATOM_UNDEF;
    link(L, ts, h);
    return ts;
}

fn unlink(L: *State, ts: *TString) bool {
    const g = L.global;
    if (g.strt.size == 0) return false;
    var p: *?*TString = &g.strt.hash[o.lmod(ts.hash, @intCast(g.strt.size))];
    while (p.*) |curr| {
        if (curr == ts) {
            p.* = curr.hnext;
            return true;
        }
        p = &curr.hnext;
    }
    return false;
}

pub fn free(L: *State, ts: *TString) void {
    if (unlink(L, ts)) L.global.strt.nuse -= 1;
    mem.freeGco(L, ts, sizestring(ts.len), ts.gch.memcat);
}

pub fn fix(ts: *TString) void {
    ts.gch.marked |= FIXEDBIT;
}

pub fn updateatom(L: *State, ts: *TString) void {
    if (ts.atom == o.ATOM_UNDEF) {
        ts.atom = if (L.global.cb.useratom) |cb| cb(L, ts.data(), ts.len) else -1;
    }
}
