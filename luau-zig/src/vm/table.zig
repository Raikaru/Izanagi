//! Tables (port of upstream ltable.cpp, MIT; see object.zig header).
//!
//! The hash functions, node layout, collision resolution (Brent's variation),
//! rehash policy and array-boundary hints match upstream so that `next`/`pairs`
//! iteration order and `#t` results are identical for the same operation sequence
//! (except for keys hashed by address: tables, functions, userdata, light userdata).

const std = @import("std");
const o = @import("object.zig");
const mem = @import("mem.zig");
const debug = @import("debug.zig");
const tm = @import("tm.zig");
const gc = @import("gc.zig");

const State = o.State;
const Table = o.Table;
const TValue = o.TValue;
const LuaNode = o.LuaNode;
const TString = o.TString;

const MAXBITS = 26;
const MAXSIZE: c_int = 1 << MAXBITS;

inline fn gnode(t: *const Table, i: usize) *LuaNode {
    return &t.node[i];
}
inline fn isDummy(t: *const Table) bool {
    return t.node == o.dummynode();
}
inline fn hashpow2(t: *const Table, n: u32) *LuaNode {
    return gnode(t, o.lmod(n, @intCast(t.sizenode())));
}

fn hashpointer(t: *const Table, p: ?*const anyopaque) *LuaNode {
    var h: u32 = @truncate(@intFromPtr(p));
    h ^= h >> 16;
    h *%= 0x85ebca6b;
    h ^= h >> 13;
    h *%= 0xc2b2ae35;
    h ^= h >> 16;
    return hashpow2(t, h);
}

fn mix64(lo: u32, hi: u32) u32 {
    var h1 = lo;
    var h2 = hi;
    const m: u32 = 0x5bd1e995;
    h1 ^= h2 >> 18;
    h1 *%= m;
    h2 ^= h1 >> 22;
    h2 *%= m;
    h1 ^= h2 >> 17;
    h1 *%= m;
    h2 ^= h1 >> 19;
    h2 *%= m;
    return h2;
}

fn hashnum(t: *const Table, n: f64) *LuaNode {
    const bits: u64 = @bitCast(n);
    return hashpow2(t, mix64(@truncate(bits), @as(u32, @truncate(bits >> 32)) & 0x7fffffff));
}

fn hashint(t: *const Table, n: i64) *LuaNode {
    const bits: u64 = @bitCast(n);
    return hashpow2(t, mix64(@truncate(bits), @truncate(bits >> 32)));
}

fn hashvec(t: *const Table, v: [o.LUA_VECTOR_SIZE]f32) *LuaNode {
    var i: [3]u32 = .{ @bitCast(v[0]), @bitCast(v[1]), @bitCast(v[2]) };
    for (&i) |*x| {
        if (x.* == 0x80000000) x.* = 0;
        x.* ^= x.* >> 17;
    }
    const h: u32 = (i[0] *% 73856093) ^ (i[1] *% 19349663) ^ (i[2] *% 83492791);
    return hashpow2(t, h);
}

inline fn hashstr(t: *const Table, s: *const TString) *LuaNode {
    return hashpow2(t, s.hash);
}

fn mainposition(t: *const Table, key: *const TValue) *LuaNode {
    return switch (o.ttype(key)) {
        o.T_NUMBER => hashnum(t, o.nvalue(key)),
        o.T_INTEGER => hashint(t, o.lvalue(key)),
        o.T_VECTOR => hashvec(t, o.vvalue(key)),
        o.T_STRING => hashstr(t, o.tsvalue(key)),
        o.T_BOOLEAN => hashpow2(t, @bitCast(o.bvalue(key))),
        o.T_LIGHTUSERDATA => hashpointer(t, o.pvalue(key)),
        else => hashpointer(t, o.gcvalue(key)),
    };
}

pub fn rawequalObj(t1: *const TValue, t2: *const TValue) bool {
    if (t1.tt != t2.tt) return false;
    return switch (o.ttype(t1)) {
        o.T_NIL => true,
        o.T_NUMBER => o.nvalue(t1) == o.nvalue(t2),
        o.T_INTEGER => o.lvalue(t1) == o.lvalue(t2),
        o.T_VECTOR => veceq(o.vvalue(t1), o.vvalue(t2)),
        o.T_BOOLEAN => o.bvalue(t1) == o.bvalue(t2),
        o.T_LIGHTUSERDATA => o.pvalue(t1) == o.pvalue(t2) and o.lightuserdatatag(t1) == o.lightuserdatatag(t2),
        else => o.gcvalue(t1) == o.gcvalue(t2),
    };
}

pub inline fn veceq(a: [o.LUA_VECTOR_SIZE]f32, b: [o.LUA_VECTOR_SIZE]f32) bool {
    return a[0] == b[0] and a[1] == b[1] and a[2] == b[2];
}

inline fn rawequalKey(k: *const o.TKey, v: *const TValue) bool {
    return rawequalObj(o.keyAsValue(k), v);
}

/// Generic chain search used for every key type except numbers handled by getnum.
fn chainsearch(t: *const Table, key: *const TValue) *TValue {
    var n = mainposition(t, key);
    while (true) {
        if (rawequalKey(&n.key, key)) return &n.val;
        if (n.key.next == 0) break;
        n = offsetNode(n, n.key.next);
    }
    return o.nilobject();
}

inline fn offsetNode(n: *LuaNode, d: c_int) *LuaNode {
    const p: [*]LuaNode = @ptrCast(n);
    return if (d >= 0) &p[@intCast(d)] else &(p - @as(usize, @intCast(-d)))[0];
}

inline fn nodeIndex(t: *const Table, n: *const LuaNode) c_int {
    return o.idiff(@as([*]const LuaNode, @ptrCast(n)), @as([*]const LuaNode, t.node));
}

fn arrayindex(key: f64) c_int {
    const i = o.num2int(key);
    return if (@as(f64, @floatFromInt(i)) == key) i else -1;
}

fn findindex(L: *State, t: *Table, key: *TValue) c_int {
    if (o.ttisnil(key)) return -1;
    const i: c_int = if (o.ttisnumber(key)) arrayindex(o.nvalue(key)) else -1;
    if (0 < i and i <= t.sizearray) return i - 1;
    var n = mainposition(t, key);
    while (true) {
        if (rawequalKey(&n.key, key) or (n.key.tt == o.T_DEADKEY and o.iscollectable(key) and n.key.value.gc == o.gcvalue(key))) {
            return nodeIndex(t, n) + t.sizearray;
        }
        if (n.key.next == 0) break;
        n = offsetNode(n, n.key.next);
    }
    debug.runerror(L, "invalid key to 'next'", .{});
}

/// `key` points to a stack slot; on success key/value are written to key[0]/key[1].
pub fn next(L: *State, t: *Table, key: o.StkId) bool {
    var i = findindex(L, t, &key[0]) + 1;
    while (i < t.sizearray) : (i += 1) {
        const e = &t.arr()[@intCast(i)];
        if (!o.ttisnil(e)) {
            o.setnvalue(&key[0], @floatFromInt(i + 1));
            key[1] = e.*;
            return true;
        }
    }
    i -= t.sizearray;
    while (i < t.sizenode()) : (i += 1) {
        const n = gnode(t, @intCast(i));
        if (!o.ttisnil(&n.val)) {
            o.getnodekey(&key[0], n);
            key[1] = n.val;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Rehash
// ---------------------------------------------------------------------------

inline fn maybesetaboundary(t: *Table, boundary: c_int) void {
    if (t.aboundary() <= 0) t.setAboundary(-boundary);
}
inline fn getaboundary(t: *const Table) c_int {
    return if (t.aboundary() < 0) -t.aboundary() else t.sizearray;
}

fn computesizes(nums: *[MAXBITS + 1]c_int, narray: *c_int) c_int {
    var a: c_int = 0;
    var na: c_int = 0;
    var n: c_int = 0;
    var i: usize = 0;
    var twotoi: c_int = 1;
    while (@divTrunc(twotoi, 2) < narray.*) : ({
        i += 1;
        twotoi *%= 2;
    }) {
        if (nums[i] > 0) {
            a += nums[i];
            if (a > @divTrunc(twotoi, 2)) {
                n = twotoi;
                na = a;
            }
        }
        if (a == narray.*) break;
    }
    narray.* = n;
    return na;
}

fn countint(key: f64, nums: *[MAXBITS + 1]c_int) c_int {
    const k = arrayindex(key);
    if (0 < k and k <= MAXSIZE) {
        nums[@intCast(o.ceillog2(k))] += 1;
        return 1;
    }
    return 0;
}

fn numusearray(t: *const Table, nums: *[MAXBITS + 1]c_int) c_int {
    var ause: c_int = 0;
    var i: c_int = 1;
    var lg: usize = 0;
    var ttlg: c_int = 1;
    while (lg <= MAXBITS) : ({
        lg += 1;
        ttlg *= 2;
    }) {
        var lc: c_int = 0;
        var lim = ttlg;
        if (lim > t.sizearray) {
            lim = t.sizearray;
            if (i > lim) break;
        }
        while (i <= lim) : (i += 1) {
            if (!o.ttisnil(&t.arr()[@intCast(i - 1)])) lc += 1;
        }
        nums[lg] += lc;
        ause += lc;
    }
    return ause;
}

fn numusehash(t: *const Table, nums: *[MAXBITS + 1]c_int, pnasize: *c_int) c_int {
    var totaluse: c_int = 0;
    var ause: c_int = 0;
    var i: usize = @intCast(t.sizenode());
    while (i > 0) {
        i -= 1;
        const n = gnode(t, i);
        if (!o.ttisnil(&n.val)) {
            if (n.key.tt == o.T_NUMBER) ause += countint(n.key.value.n, nums);
            totaluse += 1;
        }
    }
    pnasize.* += ause;
    return totaluse;
}

fn hashslotsused(t: *const Table) c_int {
    var total: c_int = 0;
    var i: usize = @intCast(t.sizenode());
    while (i > 0) {
        i -= 1;
        if (!o.ttisnil(&gnode(t, i).val)) total += 1;
    }
    return total;
}

fn initNodes(nodes: [*]LuaNode, size: usize) void {
    for (0..size) |i| nodes[i] = .{};
}

fn setnodevector(L: *State, t: *Table, size_in: c_int) void {
    var lsize: c_int = 0;
    var size = size_in;
    if (size == 0) {
        t.node = o.dummynode();
    } else {
        lsize = o.ceillog2(size);
        if (lsize > MAXBITS) debug.runerror(L, "table overflow", .{});
        size = @as(c_int, 1) << @intCast(lsize);
        t.node = mem.newArray(L, LuaNode, @intCast(size), t.gch.memcat);
        initNodes(t.node, @intCast(size));
    }
    t.lsizenode = @intCast(lsize);
    t.nodemask8 = @truncate(@as(c_uint, @bitCast((@as(c_int, 1) << @intCast(lsize)) - 1)));
    t.lastfree = size;
}

fn setarrayvector(L: *State, t: *Table, size: c_int) void {
    if (size > MAXSIZE) debug.runerror(L, "table overflow", .{});
    const old: usize = @intCast(t.sizearray);
    const arr = mem.reallocArray(L, TValue, t.array, old, @intCast(size), t.gch.memcat);
    var i = old;
    while (i < @as(usize, @intCast(size))) : (i += 1) arr[i] = .{};
    t.array = if (size == 0) null else arr;
    t.sizearray = size;
}

/// Port of upstream `resize` (the allocation-robust variant: no state change is made
/// until all new storage has been obtained).
fn resize(L: *State, t: *Table, nasize: c_int, nhsize_in: c_int) void {
    if (nasize > MAXSIZE or nhsize_in > MAXSIZE) debug.runerror(L, "table overflow", .{});
    var nhsize = nhsize_in;
    var lsizenode: c_int = 0;
    if (nhsize != 0) {
        lsizenode = o.ceillog2(nhsize);
        if (lsizenode > MAXBITS) debug.runerror(L, "table overflow", .{});
        nhsize = @as(c_int, 1) << @intCast(lsizenode);
    }
    const oldsizearray = t.sizearray;
    const oldsizenode = t.sizenode();
    const olddummy = isDummy(t);
    const oldarray = t.array;
    const oldnode = t.node;
    const memcat = t.gch.memcat;

    var newarray = t.array;
    if (nasize != oldsizearray) {
        if (nasize == 0) {
            newarray = null;
        } else {
            const na = mem.newArray(L, TValue, @intCast(nasize), memcat);
            const keep: usize = @intCast(@min(nasize, oldsizearray));
            if (keep > 0) @memcpy(na[0..keep], oldarray.?[0..keep]);
            var i = keep;
            while (i < @as(usize, @intCast(nasize))) : (i += 1) na[i] = .{};
            newarray = na;
        }
    }
    var newnode: [*]LuaNode = o.dummynode();
    if (nhsize != 0) {
        newnode = mem.tryNewArray(L, LuaNode, @intCast(nhsize), memcat) orelse {
            if (nasize != oldsizearray and newarray != null) mem.freeArray(L, TValue, newarray.?, @intCast(nasize), memcat);
            @import("do.zig").throw(L, o.LUA_ERRMEM);
        };
        initNodes(newnode, @intCast(nhsize));
    }
    t.array = newarray;
    t.sizearray = nasize;
    t.node = newnode;
    t.lsizenode = @intCast(lsizenode);
    t.nodemask8 = @truncate(@as(c_uint, @bitCast((@as(c_int, 1) << @intCast(lsizenode)) - 1)));
    t.lastfree = nhsize;

    var i: c_int = nasize;
    while (i < oldsizearray) : (i += 1) {
        const v = &oldarray.?[@intCast(i)];
        if (!o.ttisnil(v)) {
            var ok: TValue = .{};
            o.setnvalue(&ok, @floatFromInt(i + 1));
            reinsertkey(L, t, &ok, false).* = v.*;
        }
    }
    if (!olddummy) {
        var j: usize = @intCast(oldsizenode);
        while (j > 0) {
            j -= 1;
            const old = &oldnode[j];
            if (!o.ttisnil(&old.val)) {
                var ok: TValue = .{};
                o.getnodekey(&ok, old);
                reinsertkey(L, t, &ok, true).* = old.val;
            }
        }
    }
    if (nasize != oldsizearray and oldsizearray != 0) mem.freeArray(L, TValue, oldarray.?, @intCast(oldsizearray), memcat);
    if (!olddummy) mem.freeArray(L, LuaNode, oldnode, @intCast(oldsizenode), memcat);
}

fn adjustasize(t: *Table, size_in: c_int, extraintkey: c_int) c_int {
    var size = size_in;
    const tbound = !isDummy(t) or size < t.sizearray;
    while (size + 1 == extraintkey or (tbound and !o.ttisnil(getnum(t, size + 1)))) size += 1;
    return size;
}

pub fn resizearray(L: *State, t: *Table, nasize: c_int) void {
    if (nasize > MAXSIZE) debug.runerror(L, "table overflow", .{});
    const nsize: c_int = if (isDummy(t)) 0 else t.sizenode();
    const asize = adjustasize(t, nasize, -1);
    resize(L, t, asize, nsize);
}

pub fn resizehash(L: *State, t: *Table, nhsize: c_int) void {
    resize(L, t, t.sizearray, nhsize);
}

fn rehash(L: *State, t: *Table, comptime IsIntKey: bool, newintkey: c_int) void {
    if (!IsIntKey) {
        const nh = hashslotsused(t) + 1;
        resize(L, t, t.sizearray, nh);
        return;
    }
    var nums: [MAXBITS + 1]c_int = @splat(0);
    var nasize: c_int = 0;
    var totaluse: c_int = 0;
    if (t.sizearray != 0) {
        nasize = numusearray(t, &nums);
        totaluse = nasize;
    }
    if (!isDummy(t)) totaluse += numusehash(t, &nums, &nasize);
    if (0 < newintkey and newintkey <= MAXSIZE) {
        nums[@intCast(o.ceillog2(newintkey))] += 1;
        nasize += 1;
    }
    totaluse += 1;
    const na = computesizes(&nums, &nasize);
    var nh = totaluse - na;
    const nadjusted = adjustasize(t, nasize, newintkey);
    const aextra = nadjusted - nasize;
    if (aextra != 0) {
        nh -= aextra;
        nasize = nadjusted + aextra;
        nasize = adjustasize(t, nasize, newintkey);
    }
    resize(L, t, nasize, nh);
}

pub fn new(L: *State, narray: c_int, nhash: c_int) *Table {
    const t = mem.newGco(L, Table, @sizeOf(Table), o.T_TABLE);
    t.metatable = null;
    t.tmcache = 0xff;
    t.array = null;
    t.sizearray = 0;
    t.lastfree = 0;
    t.lsizenode = 0;
    t.readonly = 0;
    t.safeenv = 0;
    t.nodemask8 = 0;
    t.node = o.dummynode();
    if (narray > 0) setarrayvector(L, t, narray);
    if (nhash > 0) setnodevector(L, t, nhash);
    return t;
}

pub fn free(L: *State, t: *Table) void {
    const memcat = t.gch.memcat;
    if (!isDummy(t)) mem.freeArray(L, LuaNode, t.node, @intCast(t.sizenode()), memcat);
    if (t.array) |a| mem.freeArray(L, TValue, a, @intCast(t.sizearray), memcat);
    mem.freeGco(L, t, @sizeOf(Table), memcat);
}

fn getfreepos(t: *Table) ?*LuaNode {
    while (t.lastfree > 0) {
        t.lastfree -= 1;
        const n = gnode(t, @intCast(t.lastfree));
        if (n.key.tt == o.T_NIL) return n;
    }
    return null;
}

fn newcollidingkey(t: *Table, mp: *LuaNode, n: *LuaNode) *LuaNode {
    var mk: TValue = .{};
    o.getnodekey(&mk, mp);
    var othern = mainposition(t, &mk);
    if (othern != mp) {
        while (offsetNode(othern, othern.key.next) != mp) othern = offsetNode(othern, othern.key.next);
        othern.key.next = nodeIndex(t, n) - nodeIndex(t, othern);
        n.* = mp.*;
        if (mp.key.next != 0) {
            n.key.next += nodeIndex(t, mp) - nodeIndex(t, n);
            mp.key.next = 0;
        }
        o.setnilvalue(&mp.val);
        return mp;
    } else {
        if (mp.key.next != 0) {
            n.key.next = (nodeIndex(t, mp) + mp.key.next) - nodeIndex(t, n);
        }
        mp.key.next = nodeIndex(t, n) - nodeIndex(t, mp);
        return n;
    }
}

fn newkeyImpl(L: *State, t: *Table, key: *const TValue, comptime IsIntKey: bool, newintkey: c_int) *TValue {
    if (IsIntKey and newintkey == t.sizearray + 1) {
        rehash(L, t, IsIntKey, newintkey);
        if (@as(c_uint, @bitCast(newintkey -% 1)) < @as(c_uint, @bitCast(t.sizearray))) return &t.arr()[@intCast(newintkey - 1)];
        return newkeyImpl(L, t, key, IsIntKey, newintkey);
    }
    var mp = mainposition(t, key);
    if (!o.ttisnil(&mp.val) or isDummy(t)) {
        const n = getfreepos(t) orelse {
            rehash(L, t, IsIntKey, newintkey);
            if (IsIntKey and @as(c_uint, @bitCast(newintkey -% 1)) < @as(c_uint, @bitCast(t.sizearray))) return &t.arr()[@intCast(newintkey - 1)];
            return newkeyImpl(L, t, key, IsIntKey, newintkey);
        };
        mp = newcollidingkey(t, mp, n);
    }
    o.setnodekey(mp, key);
    gc.barriert(L, t, key);
    return &mp.val;
}

fn reinsertkey(L: *State, t: *Table, key: *const TValue, checkarraypart: bool) *TValue {
    if (checkarraypart and o.ttisnumber(key)) {
        const n = o.nvalue(key);
        const k = o.num2int(n);
        if (@as(f64, @floatFromInt(k)) == n and @as(c_uint, @bitCast(k -% 1)) < @as(c_uint, @bitCast(t.sizearray))) return &t.arr()[@intCast(k - 1)];
    }
    var mp = mainposition(t, key);
    if (!o.ttisnil(&mp.val) or isDummy(t)) mp = newcollidingkey(t, mp, getfreepos(t).?);
    o.setnodekey(mp, key);
    gc.barriert(L, t, key);
    return &mp.val;
}

pub fn getnum(t: *const Table, key: c_int) *TValue {
    if (@as(c_uint, @bitCast(key -% 1)) < @as(c_uint, @bitCast(t.sizearray))) return &t.arr()[@intCast(key - 1)];
    if (!isDummy(t)) {
        const nk: f64 = @floatFromInt(key);
        var n = hashnum(t, nk);
        while (true) {
            if (n.key.tt == o.T_NUMBER and n.key.value.n == nk) return &n.val;
            if (n.key.next == 0) break;
            n = offsetNode(n, n.key.next);
        }
    }
    return o.nilobject();
}

pub fn getstr(t: *const Table, key: *const TString) *TValue {
    var n = hashstr(t, key);
    while (true) {
        if (n.key.tt == o.T_STRING and @as(*TString, @ptrCast(n.key.value.gc)) == key) return &n.val;
        if (n.key.next == 0) break;
        n = offsetNode(n, n.key.next);
    }
    return o.nilobject();
}

pub fn getp(t: *const Table, key: ?*anyopaque, tag: c_int) *TValue {
    var n = hashpointer(t, key);
    while (true) {
        if (n.key.tt == o.T_LIGHTUSERDATA and n.key.value.p == key and n.key.extra[0] == tag) return &n.val;
        if (n.key.next == 0) break;
        n = offsetNode(n, n.key.next);
    }
    return o.nilobject();
}

pub fn get(t: *const Table, key: *const TValue) *TValue {
    switch (o.ttype(key)) {
        o.T_NIL => return o.nilobject(),
        o.T_LIGHTUSERDATA => return getp(t, o.pvalue(key), o.lightuserdatatag(key)),
        o.T_NUMBER => {
            const n = o.nvalue(key);
            const k = o.num2int(n);
            if (@as(f64, @floatFromInt(k)) == n) return getnum(t, k);
            return chainsearch(t, key);
        },
        o.T_STRING => return getstr(t, o.tsvalue(key)),
        else => return chainsearch(t, key),
    }
}

pub fn set(L: *State, t: *Table, key: *const TValue) *TValue {
    const p = get(t, key);
    t.tmcache = 0;
    if (p != o.nilobject()) return p;
    return newkey(L, t, key);
}

pub fn newkey(L: *State, t: *Table, key: *const TValue) *TValue {
    switch (o.ttype(key)) {
        o.T_NIL => debug.runerror(L, "table index is nil", .{}),
        o.T_NUMBER => {
            const n = o.nvalue(key);
            if (n != n) debug.runerror(L, "table index is NaN", .{});
            const k = o.num2int(n);
            if (@as(f64, @floatFromInt(k)) == n) return newkeyImpl(L, t, key, true, k);
            return newkeyImpl(L, t, key, false, -1);
        },
        o.T_VECTOR => {
            const v = o.vvalue(key);
            if (v[0] != v[0] or v[1] != v[1] or v[2] != v[2]) debug.runerror(L, "table index contains NaN", .{});
            return newkeyImpl(L, t, key, false, -1);
        },
        else => return newkeyImpl(L, t, key, false, -1),
    }
}

pub fn setnum(L: *State, t: *Table, key: c_int) *TValue {
    if (@as(c_uint, @bitCast(key -% 1)) < @as(c_uint, @bitCast(t.sizearray))) return &t.arr()[@intCast(key - 1)];
    const p = getnum(t, key);
    if (p != o.nilobject()) return p;
    var k: TValue = .{};
    o.setnvalue(&k, @floatFromInt(key));
    return newkeyImpl(L, t, &k, true, key);
}

pub fn setstr(L: *State, t: *Table, key: *TString) *TValue {
    const p = getstr(t, key);
    t.tmcache = 0;
    if (p != o.nilobject()) return p;
    var k: TValue = .{};
    o.setsvalue(&k, key);
    return newkeyImpl(L, t, &k, false, -1);
}

pub fn setp(L: *State, t: *Table, key: ?*anyopaque, tag: c_int) *TValue {
    const p = getp(t, key, tag);
    if (p != o.nilobject()) return p;
    var k: TValue = .{};
    o.setpvalue(&k, key, tag);
    return newkeyImpl(L, t, &k, false, -1);
}

fn updateaboundary(t: *Table, boundary: c_int) c_int {
    const a = t.arr();
    if (boundary < t.sizearray and o.ttisnil(&a[@intCast(boundary - 1)])) {
        if (boundary >= 2 and !o.ttisnil(&a[@intCast(boundary - 2)])) {
            maybesetaboundary(t, boundary - 1);
            return boundary - 1;
        }
    } else if (boundary + 1 < t.sizearray and !o.ttisnil(&a[@intCast(boundary)]) and o.ttisnil(&a[@intCast(boundary + 1)])) {
        maybesetaboundary(t, boundary + 1);
        return boundary + 1;
    }
    return 0;
}

pub fn getn(t: *Table) c_int {
    const boundary = getaboundary(t);
    if (boundary > 0) {
        const a = t.arr();
        if (!o.ttisnil(&a[@intCast(t.sizearray - 1)]) and isDummy(t)) return t.sizearray;
        if (boundary < t.sizearray and !o.ttisnil(&a[@intCast(boundary - 1)]) and o.ttisnil(&a[@intCast(boundary)])) return boundary;
        const found = updateaboundary(t, boundary);
        if (found > 0) return found;
    }
    const j = t.sizearray;
    if (j > 0 and o.ttisnil(&t.arr()[@intCast(j - 1)])) {
        const arr = t.arr();
        var base: usize = 0;
        var rest: usize = @intCast(j);
        while (true) {
            const half = rest >> 1;
            if (half == 0) break;
            if (!o.ttisnil(&arr[base + half])) base += half;
            rest -= half;
        }
        const b: c_int = @as(c_int, @intFromBool(!o.ttisnil(&arr[base]))) + @as(c_int, @intCast(base));
        maybesetaboundary(t, b);
        return b;
    }
    return j;
}

pub fn clone(L: *State, tt: *Table) *Table {
    const t = mem.newGco(L, Table, @sizeOf(Table), o.T_TABLE);
    t.metatable = tt.metatable;
    t.tmcache = tt.tmcache;
    t.array = null;
    t.sizearray = 0;
    t.lsizenode = 0;
    t.nodemask8 = 0;
    t.readonly = 0;
    t.safeenv = 0;
    t.node = o.dummynode();
    t.lastfree = 0;
    if (tt.sizearray != 0) {
        const n: usize = @intCast(tt.sizearray);
        const a = mem.newArray(L, TValue, n, t.gch.memcat);
        @memcpy(a[0..n], tt.arr()[0..n]);
        t.array = a;
        maybesetaboundary(t, getaboundary(tt));
        t.sizearray = tt.sizearray;
    }
    if (!isDummy(tt)) {
        const size: usize = @intCast(tt.sizenode());
        const nodes = mem.newArray(L, LuaNode, size, t.gch.memcat);
        @memcpy(nodes[0..size], tt.node[0..size]);
        t.node = nodes;
        t.lsizenode = tt.lsizenode;
        t.nodemask8 = tt.nodemask8;
        t.lastfree = tt.lastfree;
    }
    return t;
}

pub fn clear(tt: *Table) void {
    var i: usize = 0;
    while (i < @as(usize, @intCast(tt.sizearray))) : (i += 1) o.setnilvalue(&tt.arr()[i]);
    maybesetaboundary(tt, 0);
    if (!isDummy(tt)) {
        const size: usize = @intCast(tt.sizenode());
        tt.lastfree = @intCast(size);
        for (0..size) |j| tt.node[j] = .{};
    }
    tt.tmcache = 0xff;
}

pub fn setreadonly(t: *Table, ro: bool) void {
    t.readonly = @intFromBool(ro);
}

/// Used by `table.find` style searches (upstream luaH_rawfind semantics are provided by
/// the library through the public API; kept here for the VM's own users).
pub fn dummy(t: *const Table) bool {
    return isDummy(t);
}

pub const removeKeyDead = struct {
    /// GC helper: mark a node with a nil value as dead so the key can be collected.
    pub fn apply(n: *LuaNode) void {
        if (n.key.tt >= o.T_STRING) n.key.tt = o.T_DEADKEY;
    }
};

pub const internal = struct {
    pub const hashslots = hashslotsused;
    pub const nodeAt = gnode;
};

comptime {
    _ = tm;
}
