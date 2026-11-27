//! Table library (upstream VM/src/ltablib.cpp), translated to Zig.
//!
//! Portions derived from Luau (MIT, Copyright (c) 2019-2025 Roblox Corporation) and
//! Lua 5.x (MIT, Copyright (C) 1994-2008 Lua.org, PUC-Rio).
//!
//! Upstream reaches into LuaTable for fast paths (direct array copies). Those paths are
//! observably identical to the generic raw-access paths used here; the places where the
//! table layout changes observable behaviour (table.move's sparse-move heuristic and
//! array growth, table.sort's "table modified during sorting" detection) use the VM's
//! `luau_tablesizes` / `luau_tableresizearray` helpers; table.sort swaps array slots via
//! `luau_tableswap` like upstream. Both LuauTableFindImprovement paths are implemented.

const std = @import("std");
const api = @import("api.zig");
const aux = @import("auxiliary.zig");
const flags = @import("flags.zig");
const State = api.State;

const INT_MAX: i64 = std.math.maxInt(c_int);

fn foreachi(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    aux.checktype(L, 2, api.TFUNCTION);
    const n = api.objlen(L, 1);
    var i: c_int = 1;
    while (i <= n) : (i += 1) {
        api.pushvalue(L, 2); // function
        api.pushinteger(L, i); // 1st argument
        _ = api.rawgeti(L, 1, i); // 2nd argument
        api.call(L, 2, 1);
        if (!api.isnil(L, -1)) return 1;
        api.pop(L, 1); // remove nil result
    }
    return 0;
}

fn foreach(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    aux.checktype(L, 2, api.TFUNCTION);
    api.pushnil(L); // first key
    while (api.next(L, 1)) {
        api.pushvalue(L, 2); // function
        api.pushvalue(L, -3); // key
        api.pushvalue(L, -3); // value
        api.call(L, 2, 1);
        if (!api.isnil(L, -1)) return 1;
        api.pop(L, 2); // remove value and result
    }
    return 0;
}

fn maxn(L: *State) c_int {
    var max: f64 = 0;
    aux.checktype(L, 1, api.TTABLE);
    // array part first (keys 1..sizearray), then the hash part, like upstream
    var iter: c_int = 0;
    while (true) {
        iter = api.rawiter(L, 1, iter);
        if (iter == -1) break;
        if (api.@"type"(L, -2) == api.TNUMBER) {
            const v = api.tonumber(L, -2);
            if (v > max) max = v;
        }
        api.pop(L, 2);
    }
    api.pushnumber(L, max);
    return 1;
}

fn getn(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    api.pushinteger(L, api.objlen(L, 1));
    return 1;
}

fn shouldsparsemove(L: *State, src: c_int, dst: c_int, n: c_int) bool {
    const s = api.tablesizes(L, src);
    const d = api.tablesizes(L, dst);
    const srcelems = s.array + s.node;
    const dstelems = d.array + d.node;
    const maxelems = @max(srcelems, dstelems);
    const minsparsemoveelems = 32;
    return n > minsparsemoveelems and @divTrunc(n, 2) > maxelems;
}

fn tovalidintkey(L: *State, idx: c_int, f: c_int, e: c_int) ?c_int {
    if (api.@"type"(L, idx) == api.TNUMBER) {
        const nkey = api.tonumber(L, idx);
        if (nkey >= @as(f64, @floatFromInt(f)) and nkey <= @as(f64, @floatFromInt(e))) {
            // in range [f, e], so the int conversion is exact for integral keys
            const r: c_int = @intFromFloat(@trunc(nkey));
            if (@as(f64, @floatFromInt(r)) == nkey) return r;
        }
    }
    return null;
}

fn moveelements(L: *State, srct: c_int, dstt: c_int, f: c_int, e: c_int, t: c_int, sparsemove: bool) void {
    if (api.getreadonly(L, dstt)) aux.readonlyerror(L);

    const n = e - f + 1; // number of elements to move

    if (sparsemove) {
        const srcta = api.absindex(L, srct);
        const dstta = api.absindex(L, dstt);
        const te = t + (n - 1);

        // temporary table to hold elements from source that are being moved
        api.newtable(L);

        // collect integer key elements that are being moved
        var iter: c_int = 0;
        while (true) {
            iter = api.rawiter(L, srcta, iter);
            if (iter == -1) break;
            if (tovalidintkey(L, -2, f, e)) |ikey| {
                api.rawseti(L, -3, ikey); // pops value
            } else {
                api.pop(L, 1); // pop value
            }
            api.pop(L, 1); // pop key
        }

        // clear the destination range
        iter = 0;
        while (true) {
            iter = api.rawiter(L, dstta, iter);
            if (iter == -1) break;
            if (tovalidintkey(L, -2, t, te)) |ikey| {
                api.pushnil(L);
                api.rawseti(L, dstta, ikey);
            }
            api.pop(L, 2);
        }

        // copy the elements from the temporary table to the destination table
        iter = 0;
        while (true) {
            iter = api.rawiter(L, -1, iter);
            if (iter == -1) break;
            const ikey = api.tointeger(L, -2);
            api.rawseti(L, dstta, ikey - f + t);
            api.pop(L, 1);
        }

        api.pop(L, 1);
    } else {
        const same = api.rawequal(L, srct, dstt);
        if (t > e or t <= f or !same) {
            var i: c_int = 0;
            while (i < n) : (i += 1) {
                _ = api.rawgeti(L, srct, f + i);
                api.rawseti(L, dstt, t + i);
            }
        } else {
            var i: c_int = n - 1;
            while (i >= 0) : (i -= 1) {
                _ = api.rawgeti(L, srct, f + i);
                api.rawseti(L, dstt, t + i);
            }
        }
    }
}

fn tinsert(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    const n = api.objlen(L, 1);
    var pos: c_int = undefined; // where to insert new element
    switch (api.gettop(L)) {
        2 => pos = n + 1, // insert new element at the end
        3 => {
            pos = aux.checkinteger(L, 2); // 2nd argument is the position
            // move up elements if necessary
            if (1 <= pos and pos <= n)
                moveelements(L, 1, 1, pos, n, pos + 1, false);
        },
        else => aux.errorf(L, "wrong number of arguments to 'insert'", .{}),
    }
    api.rawseti(L, 1, pos); // t[pos] = v
    return 0;
}

fn tremove(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    const n = api.objlen(L, 1);
    const pos = aux.optinteger(L, 2, n);

    if (!(1 <= pos and pos <= n)) // position is outside bounds?
        return 0; // nothing to remove
    _ = api.rawgeti(L, 1, pos); // result = t[pos]

    moveelements(L, 1, 1, pos + 1, n, pos, false);

    api.pushnil(L);
    api.rawseti(L, 1, n); // t[n] = nil
    return 1;
}

// Copy elements (1[f], ..., 1[e]) into (tt[t], tt[t+1], ...).
fn tmove(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    const f = aux.checkinteger(L, 2);
    const e = aux.checkinteger(L, 3);
    const t = aux.checkinteger(L, 4);
    const tt: c_int = if (!api.isnoneornil(L, 5)) 5 else 1; // destination table
    aux.checktype(L, tt, api.TTABLE);

    if (e >= f) { // otherwise, nothing to move
        aux.argcheck(L, f > 0 or @as(i64, e) < INT_MAX + f, 3, "too many elements to move");
        const n = e - f + 1; // number of elements to move
        aux.argcheck(L, @as(i64, t) <= INT_MAX - n + 1, 4, "destination wrap around");

        // also checked in moveelements, but this blocks resizes of r/o tables
        if (api.getreadonly(L, tt)) aux.readonlyerror(L);

        const sparsemove = shouldsparsemove(L, 1, tt, n);

        const dstarray = api.tablesizes(L, tt).array;
        if (t > 0 and (t - 1) <= dstarray and @as(i64, t) - 1 + n > dstarray) {
            // grow the destination table array
            api.tableresizearray(L, tt, t - 1 + n);
        }

        moveelements(L, 1, tt, f, e, t, sparsemove);
    }
    api.pushvalue(L, tt); // return destination table
    return 1;
}

fn addfield(L: *State, b: *aux.Strbuf, i: c_int) void {
    const tt = api.rawgeti(L, 1, i);
    if (tt != api.TSTRING and tt != api.TNUMBER)
        aux.errorf(L, "invalid value ({s}) at index {d} in table for 'concat'", .{ aux.typenameOf(L, -1), i });
    b.addvalue();
}

fn tconcat(L: *State) c_int {
    const sep = aux.optlstring(L, 2, "");
    aux.checktype(L, 1, api.TTABLE);
    var i = aux.optinteger(L, 3, 1);
    const last = if (api.isnoneornil(L, 4)) api.objlen(L, 1) else aux.checkinteger(L, 4);

    var b: aux.Strbuf = undefined;
    b.init(L);
    while (i < last) : (i += 1) {
        addfield(L, &b, i);
        if (sep.len != 0) b.addlstring(sep);
    }
    if (i == last) // add last value (if interval was not empty)
        addfield(L, &b, i);
    b.pushresult();
    return 1;
}

fn tpack(L: *State) c_int {
    const n = api.gettop(L); // number of elements to pack
    api.createtable(L, n, 1); // create result table
    var i: c_int = 1;
    while (i <= n) : (i += 1) {
        api.pushvalue(L, i);
        api.rawseti(L, -2, i);
    }
    // t.n = number of elements
    api.pushnumber(L, @floatFromInt(n));
    api.rawsetfield(L, -2, "n");
    return 1; // return table
}

fn tunpack(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    var i = aux.optinteger(L, 2, 1);
    const e = if (api.isnoneornil(L, 3)) api.objlen(L, 1) else aux.checkinteger(L, 3);
    if (i > e) return 0; // empty range
    var n: c_uint = @as(c_uint, @bitCast(e)) -% @as(c_uint, @bitCast(i)); // number of elements minus 1
    if (n >= @as(c_uint, @intCast(INT_MAX)) or !api.checkstack(L, @intCast(n + 1)))
        aux.errorf(L, "too many results to unpack", .{});
    n += 1;
    // push arg[i..e - 1] (to avoid overflows)
    while (i < e) : (i += 1) _ = api.rawgeti(L, 1, i);
    _ = api.rawgeti(L, 1, e); // push last element
    return @intCast(n);
}

// =======================================================================================
// table.sort: the upstream introsort over the table's array part, operating on 0-based
// positions [0, n) of the array (luaH_getn never exceeds sizearray in Luau).

const Sorter = struct {
    L: *State,
    hasfunc: bool,
    sizearray: c_int,

    fn less(s: *const Sorter, i: c_int, j: c_int) bool {
        const L = s.L;
        var res: bool = undefined;
        if (s.hasfunc) {
            api.pushvalue(L, 2);
            _ = api.rawgeti(L, 1, i + 1);
            _ = api.rawgeti(L, 1, j + 1);
            api.call(L, 2, 1);
            res = api.toboolean(L, -1);
            api.pop(L, 1);
        } else {
            _ = api.rawgeti(L, 1, i + 1);
            _ = api.rawgeti(L, 1, j + 1);
            res = api.lessthan(L, -2, -1);
            api.pop(L, 2);
        }
        // predicate call may resize the table, which is invalid
        if (api.tablesizes(L, 1).array != s.sizearray)
            aux.errorf(L, "table modified during sorting", .{});
        return res;
    }

    fn swap(s: *const Sorter, i: c_int, j: c_int) void {
        // direct array-slot swap like upstream sort_swap: no readonly re-check, so a
        // comparator that freezes the table does not stop the sort
        api.tableswap(s.L, 1, i, j);
    }

    fn siftheap(s: *const Sorter, l: c_int, u: c_int, root_: c_int) void {
        var root = root_;
        const count = u - l + 1;
        // process all elements with two children
        while (root * 2 + 2 < count) {
            const left = root * 2 + 1;
            const right = root * 2 + 2;
            var nxt = root;
            nxt = if (s.less(l + nxt, l + left)) left else nxt;
            nxt = if (s.less(l + nxt, l + right)) right else nxt;
            if (nxt == root) break;
            s.swap(l + root, l + nxt);
            root = nxt;
        }
        // process last element if it has just one child
        const lastleft = root * 2 + 1;
        if (lastleft == count - 1 and s.less(l + root, l + lastleft))
            s.swap(l + root, l + lastleft);
    }

    fn heap(s: *const Sorter, l: c_int, u: c_int) void {
        const count = u - l + 1;
        var i = @divTrunc(count, 2) - 1;
        while (i >= 0) : (i -= 1) s.siftheap(l, u, i);
        i = count - 1;
        while (i > 0) : (i -= 1) {
            s.swap(l, l + i);
            s.siftheap(l, l + i - 1, 0);
        }
    }

    fn rec(s: *const Sorter, l_: c_int, u_: c_int, limit_: c_int) void {
        const L = s.L;
        var l = l_;
        var u = u_;
        var limit = limit_;
        // sort range [l..u] (inclusive, 0-based)
        while (l < u) {
            // if the limit has been reached, quick sort is going over the permitted nlogn complexity, so we fall back to heap sort
            if (limit == 0) return s.heap(l, u);

            // sort elements a[l], a[(l+u)/2] and a[u]
            if (s.less(u, l)) s.swap(u, l);
            if (u - l == 1) break; // only 2 elements
            const m = l + ((u - l) >> 1); // midpoint
            if (s.less(m, l)) {
                s.swap(m, l);
            } else if (s.less(u, m)) {
                s.swap(m, u);
            }
            if (u - l == 2) break; // only 3 elements

            // here l, m, u are ordered; m will become the new pivot
            const p = u - 1;
            s.swap(m, u - 1); // pivot is now (and always) at u-1

            // a[l] <= P == a[u-1] <= a[u], only need to sort from l+1 to u-2
            var i = l;
            var j = u - 1;
            while (true) {
                // repeat ++i until a[i] >= P
                while (true) {
                    i += 1;
                    if (!s.less(i, p)) break;
                    if (i >= u) aux.errorf(L, "invalid order function for sorting", .{});
                }
                // repeat --j until a[j] <= P
                while (true) {
                    j -= 1;
                    if (!s.less(p, j)) break;
                    if (j <= l) aux.errorf(L, "invalid order function for sorting", .{});
                }
                if (j < i) break;
                s.swap(i, j);
            }

            // swap pivot a[p] with a[i], which is the new midpoint
            s.swap(p, i);

            // adjust limit to allow 1.5 log2N recursive steps
            limit = (limit >> 1) + (limit >> 2);

            // sort smaller half recursively; the larger half is sorted in the next loop iteration
            if (i - l < u - i) {
                s.rec(l, i - 1, limit);
                l = i + 1;
            } else {
                s.rec(i + 1, u, limit);
                u = i - 1;
            }
        }
    }
};

fn tsort(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    const n = api.objlen(L, 1);
    if (api.getreadonly(L, 1)) aux.readonlyerror(L);

    var hasfunc = false;
    if (!api.isnoneornil(L, 2)) { // is there a 2nd argument?
        aux.checktype(L, 2, api.TFUNCTION);
        hasfunc = true;
    }
    api.settop(L, 2); // make sure there are two arguments

    if (n > 0) {
        const s = Sorter{ .L = L, .hasfunc = hasfunc, .sizearray = api.tablesizes(L, 1).array };
        s.rec(0, n - 1, n);
    }
    return 0;
}

fn tcreate(L: *State) c_int {
    const size = aux.checkinteger(L, 1);
    if (size < 0) aux.argerror(L, 1, "size out of range");

    // decide on the fill value before pushing the result: with a single argument, index 2
    // would otherwise name the new table itself (false is a valid fill value, nil is not)
    const fill = !api.isnoneornil(L, 2);
    api.createtable(L, size, 0);
    if (fill) {
        var i: c_int = 1;
        while (i <= size) : (i += 1) {
            api.pushvalue(L, 2);
            api.rawseti(L, -2, i);
        }
    }
    return 1;
}

fn tfind(L: *State) c_int {
    var init: c_int = 1;

    if (flags.LuauTableFindImprovement) {
        const nargs = api.gettop(L);
        if (nargs < 1 or !api.istable(L, 1)) aux.checktype(L, 1, api.TTABLE);
        if (nargs < 2) aux.checkany(L, 2);
        if (nargs >= 3 and !api.isnil(L, 3)) init = aux.checkinteger(L, 3);
    } else {
        aux.checktype(L, 1, api.TTABLE);
        aux.checkany(L, 2);
        init = aux.optinteger(L, 3, 1);
    }

    if (init < 1) aux.argerror(L, 3, "index out of range");

    if (flags.LuauTableFindImprovement) {
        if (init > api.tablesizes(L, 1).array and api.tableisdummynode(L, 1)) {
            api.pushnil(L);
            return 1;
        }
        // Fast array search can only be performed when metamethod is absent/not used, as it can mutate table and relocate stack
        if (!api.haseqtm(L, 2)) {
            const index = api.tablerawfind(L, 1, init, 2);
            if (index != 0) api.pushinteger(L, index) else api.pushnil(L);
            return 1;
        }
    }
    var i = init;
    while (true) : (i +%= 1) {
        _ = api.rawgeti(L, 1, i);
        if (api.isnil(L, -1)) break;
        if (api.equal(L, 2, -1)) {
            api.pushinteger(L, i);
            return 1;
        }
        api.pop(L, 1);
    }
    api.pushnil(L);
    return 1;
}

fn tclear(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    api.cleartable(L, 1); // raises the readonly error for frozen tables
    return 0;
}

fn tfreeze(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    aux.argcheck(L, !api.getreadonly(L, 1), 1, "table is already frozen");
    aux.argcheck(L, !aux.getmetafield(L, 1, "__metatable"), 1, "table has a protected metatable");
    api.setreadonly(L, 1, true);
    api.pushvalue(L, 1);
    return 1;
}

fn tisfrozen(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    api.pushboolean(L, api.getreadonly(L, 1));
    return 1;
}

fn tclone(L: *State) c_int {
    aux.checktype(L, 1, api.TTABLE);
    aux.argcheck(L, !aux.getmetafield(L, 1, "__metatable"), 1, "table has a protected metatable");
    api.clonetable(L, 1);
    return 1;
}

const tab_funcs = [_]aux.Entry{
    aux.entry("concat", tconcat),
    aux.entry("foreach", foreach),
    aux.entry("foreachi", foreachi),
    aux.entry("getn", getn),
    aux.entry("maxn", maxn),
    aux.entry("insert", tinsert),
    aux.entry("remove", tremove),
    aux.entry("sort", tsort),
    aux.entry("pack", tpack),
    aux.entry("unpack", tunpack),
    aux.entry("move", tmove),
    aux.entry("create", tcreate),
    aux.entry("find", tfind),
    aux.entry("clear", tclear),
    aux.entry("freeze", tfreeze),
    aux.entry("isfrozen", tisfrozen),
    aux.entry("clone", tclone),
};

pub fn open(L: *State) c_int {
    aux.register(L, "table", &tab_funcs);

    // Lua 5.1 compat
    api.pushcfunction(L, aux.wrap(tunpack), "unpack");
    api.setglobal(L, "unpack");

    return 1;
}
