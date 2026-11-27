const std = @import("std");
const o = @import("object.zig");
const state = @import("state.zig");
const table = @import("table.zig");
const strings = @import("string.zig");
const gc = @import("gc.zig");
const fast = @import("fastmisc.zig");
extern "c" fn realloc(?*anyopaque, usize) ?*anyopaque;
extern "c" fn free(?*anyopaque) void;
fn alloc(_: ?*anyopaque, p: ?*anyopaque, _: usize, n: usize) callconv(.c) ?*anyopaque {
    if (n == 0) { free(p); return null; }
    return realloc(p, n);
}
fn number(v: f64) o.TValue { var x: o.TValue = .{}; o.setnvalue(&x, v); return x; }
fn string(L: *o.State, s: []const u8) o.TValue { var x: o.TValue = .{}; o.setsvalue(&x, strings.new(L, s)); return x; }
fn tab(t: *o.Table) o.TValue { var x: o.TValue = .{}; o.sethvalue(&x, t); return x; }
fn expectNum(v: *o.TValue, n: f64) !void { try std.testing.expect(o.ttisnumber(v)); try std.testing.expectEqual(n, o.nvalue(v)); }
fn check(id: u8) !void {
    const L = state.newstate(alloc, null) orelse return error.StateInit;
    defer state.close(L);
    const t = table.new(L, 3, 0);
    t.arr()[0] = number(17);
    t.arr()[1] = number(23);
    t.arr()[2] = number(31);
    var bmem: [o.Buffer.header + 16]u8 align(@alignOf(o.Buffer)) = @splat(0);
    const b: *o.Buffer = @ptrCast(&bmem);
    b.len = 8;
    var buf: o.TValue = .{};
    o.setbufvalue(&buf, b);
    var a = [_]o.TValue{ number(0), number(0) };
    var first: o.TValue = .{};
    var nparams: c_int = 1;
    var nresults: c_int = 1;
    const r = L.stack + 2;
    switch (id) {
        41 => { first = string(L, "abc"); a[0] = number(2); nparams = 2; },
        42 => { first = number(65); },
        43 => { first = string(L, "abc"); },
        45 => { first = string(L, "abc"); a[0] = number(1); a[1] = number(2); nparams = 3; },
        49 => { first = tab(t); a[0] = number(1); a[1] = number(42); nparams = 3; },
        50 => { first = tab(t); a[0] = number(1); nparams = 2; },
        51 => { first = number(7); a[0] = number(7); nparams = 2; },
        52 => { first = tab(t); a[0] = number(42); nparams = 2; nresults = 0; },
        53 => { first = tab(t); nresults = -1; },
        58 => { first = string(L, "abc"); },
        60 => { first = tab(t); },
        61 => { first = tab(t); a[0] = tab(table.new(L, 0, 0)); nparams = 2; },
        62 => { first = string(L, "17"); },
        63 => { first = number(17); },
        65...77, 131, 132 => {
            first = buf;
            const writing = switch (id) { 67, 70, 73, 75, 77, 132 => true, else => false };
            nparams = if (writing) 3 else 2;
            nresults = if (writing) 0 else 1;
            a[1] = number(42);
            if (id == 132) o.setlvalue(&a[1], 42);
        },
        133 => { first = number(3); a[0] = number(7); nparams = 2; },
        134 => { first = tab(t); nresults = 0; },
        else => unreachable,
    }
    const initial_top = L.top;
    const count = fast.run(L, id, r, &first, &a, nparams, nresults);
    try std.testing.expectEqual(@as(?c_int, if (id == 52 or id == 134 or id == 67 or id == 70 or id == 73 or id == 75 or id == 77 or id == 132) 0 else if (id == 53) 3 else 1), count);
    try std.testing.expectEqual(initial_top, L.top);
    switch (id) {
        41 => try expectNum(&r[0], 98),
        42 => try std.testing.expectEqualStrings("A", o.tsvalue(&r[0]).slice()),
        43, 58 => try expectNum(&r[0], 3),
        45 => try std.testing.expectEqualStrings("ab", o.tsvalue(&r[0]).slice()),
        49 => { try std.testing.expectEqual(t, o.hvalue(&r[0])); try expectNum(table.getnum(t, 1), 42); },
        50 => try expectNum(&r[0], 17),
        51 => try std.testing.expect(o.ttisboolean(&r[0]) and o.bvalue(&r[0]) == 1),
        52 => try expectNum(table.getnum(t, 4), 42),
        53 => { try expectNum(&r[0], 17); try expectNum(&r[1], 23); try expectNum(&r[2], 31); },
        60 => try std.testing.expect(o.ttisnil(&r[0])),
        61 => try std.testing.expectEqual(o.hvalue(&a[0]), t.metatable.?),
        62 => try expectNum(&r[0], 17),
        63 => try std.testing.expectEqualStrings("17", o.tsvalue(&r[0]).slice()),
        65, 66, 68, 69, 71, 72, 74, 76 => try expectNum(&r[0], 0),
        67, 70, 73 => try std.testing.expectEqual(@as(u8, 42), b.data()[0]),
        75 => try std.testing.expectEqual(@as(u8, 0x28), b.data()[2]),
        77 => try std.testing.expectEqual(@as(u8, 0x45), b.data()[6]),
        131 => { try std.testing.expect(o.ttisinteger(&r[0])); try std.testing.expectEqual(@as(i64, 0), o.lvalue(&r[0])); },
        132 => try std.testing.expectEqual(@as(u8, 42), b.data()[0]),
        133 => { const created = o.hvalue(&r[0]); try std.testing.expectEqual(@as(c_int, 3), created.sizearray); try expectNum(&created.arr()[2], 7); },
        134 => try std.testing.expect(o.ttisnil(table.getnum(t, 1))),
        else => unreachable,
    }
    const before = L.global.totalbytes;
    const top = L.top;
    const saved = r[0];
    // Every family handler rejects this arity/result combination, including char.
    const buffer_before = bmem;
    const entry_before = table.getnum(t, 1).*;
    const mt_before = t.metatable;
    try std.testing.expectEqual(@as(?c_int, null), fast.run(L, id, r, &first, &a, 0, 2));
    try std.testing.expectEqual(before, L.global.totalbytes);
    try std.testing.expectEqual(top, L.top);
    try std.testing.expectEqualSlices(u8, std.mem.asBytes(&saved), std.mem.asBytes(&r[0]));
    try std.testing.expectEqualSlices(u8, &buffer_before, &bmem);
    try std.testing.expectEqualSlices(u8, std.mem.asBytes(&entry_before), std.mem.asBytes(table.getnum(t, 1)));
    try std.testing.expectEqual(mt_before, t.metatable);
}
test "builtin 41 fast and guard" { try check(41); }
test "builtin 42 fast and guard" { try check(42); }
test "builtin 43 fast and guard" { try check(43); }
test "builtin 45 fast and guard" { try check(45); }
test "builtin 49 fast and guard" { try check(49); }
test "builtin 50 fast and guard" { try check(50); }
test "builtin 51 fast and guard" { try check(51); }
test "builtin 52 fast and guard" { try check(52); }
test "builtin 53 fast and guard" { try check(53); }
test "builtin 58 fast and guard" { try check(58); }
test "builtin 60 fast and guard" { try check(60); }
test "builtin 61 fast and guard" { try check(61); }
test "builtin 62 fast and guard" { try check(62); }
test "builtin 63 fast and guard" { try check(63); }
test "builtin 65 fast and guard" { try check(65); }
test "builtin 66 fast and guard" { try check(66); }
test "builtin 67 fast and guard" { try check(67); }
test "builtin 68 fast and guard" { try check(68); }
test "builtin 69 fast and guard" { try check(69); }
test "builtin 70 fast and guard" { try check(70); }
test "builtin 71 fast and guard" { try check(71); }
test "builtin 72 fast and guard" { try check(72); }
test "builtin 73 fast and guard" { try check(73); }
test "builtin 74 fast and guard" { try check(74); }
test "builtin 75 fast and guard" { try check(75); }
test "builtin 76 fast and guard" { try check(76); }
test "builtin 77 fast and guard" { try check(77); }
test "builtin 131 fast and guard" { try check(131); }
test "builtin 132 fast and guard" { try check(132); }
test "builtin 133 fast and guard" { try check(133); }
test "builtin 134 fast and guard" { try check(134); }

test "metatable protection readonly and barriers" {
    const L = state.newstate(alloc, null) orelse return error.StateInit;
    defer state.close(L);
    const t = table.new(L, 2, 0);
    const mt = table.new(L, 0, 1);
    table.setstr(L, mt, L.global.tmname[@intFromEnum(o.TMS.METATABLE)]).* = number(99);
    t.metatable = mt;
    var v = tab(t);
    var args = [_]o.TValue{ tab(mt), number(7) };
    const r = L.stack + 2;
    try std.testing.expectEqual(@as(?c_int, 1), fast.run(L, 60, r, &v, &args, 1, 1));
    try expectNum(&r[0], 99);
    try std.testing.expectEqual(@as(?c_int, null), fast.run(L, 61, r, &v, &args, 2, 1));
    try std.testing.expectEqual(mt, t.metatable.?);
    t.readonly = 1;
    args[0] = number(1);
    try std.testing.expectEqual(@as(?c_int, null), fast.run(L, 49, r, &v, &args, 3, 1));
    try std.testing.expectEqual(@as(?c_int, null), fast.run(L, 52, r, &v, &args, 2, 0));
    try std.testing.expectEqual(@as(?c_int, null), fast.run(L, 134, r, &v, &args, 1, 0));
    try std.testing.expect(o.ttisnil(table.getnum(t, 1)));
    t.readonly = 0;
    const child = table.new(L, 0, 0);
    args[1] = tab(child);
    t.gch.marked = 4;
    child.gch.marked = 1;
    L.global.gcstate = gc.GCSpropagate;
    try std.testing.expectEqual(@as(?c_int, 1), fast.run(L, 49, r, &v, &args, 3, 1));
    try std.testing.expect(gc.isgray(&t.gch));
    try std.testing.expectEqual(&t.gch, L.global.grayagain.?);
    t.gch.marked = 4;
    L.global.grayagain = null;
    args[0] = tab(child);
    try std.testing.expectEqual(@as(?c_int, 0), fast.run(L, 52, r, &v, &args, 2, 0));
    try std.testing.expect(gc.isgray(&t.gch));
    try std.testing.expectEqual(&t.gch, L.global.grayagain.?);
    L.global.gcstate = gc.GCSpause;
}

test "string allocation interning and index fallbacks" {
    const L = state.newstate(alloc, null) orelse return error.StateInit;
    defer state.close(L);
    const r = L.stack + 2;
    var arg = number(65);
    var a = [_]o.TValue{ number(1), number(2) };
    try std.testing.expectEqual(@as(?c_int, 1), fast.run(L, 42, r, &arg, &a, 1, 1));
    try std.testing.expectEqual(strings.new(L, "A"), o.tsvalue(&r[0]));
    arg = string(L, "abc");
    try std.testing.expectEqual(@as(?c_int, 1), fast.run(L, 45, r, &arg, &a, 3, 1));
    try std.testing.expectEqual(strings.new(L, "ab"), o.tsvalue(&r[0]));
    for ([_]f64{ -1, 0, 4 }) |bad| {
        a[0] = number(bad);
        try std.testing.expectEqual(@as(?c_int, null), fast.run(L, 41, r, &arg, &a, 2, 1));
        try std.testing.expectEqual(@as(?c_int, null), fast.run(L, 45, r, &arg, &a, 3, 1));
    }
    a[0] = number(1); a[1] = number(4);
    try std.testing.expectEqual(@as(?c_int, null), fast.run(L, 41, r, &arg, &a, 3, 4));
    try std.testing.expectEqual(@as(?c_int, null), fast.run(L, 45, r, &arg, &a, 3, 1));
    arg = number(17);
    try std.testing.expectEqual(@as(?c_int, 1), fast.run(L, 63, r, &arg, &a, 1, 1));
    try std.testing.expectEqual(strings.new(L, "17"), o.tsvalue(&r[0]));
    for ([_]f64{ 2, 1, 37 }) |base| {
        arg = string(L, "11"); a[0] = number(base);
        try std.testing.expectEqual(@as(?c_int, null), fast.run(L, 62, r, &arg, &a, 2, 1));
    }
}

test "unpack multret limits and clear metatable" {
    const L = state.newstate(alloc, null) orelse return error.StateInit;
    defer state.close(L);
    const t = table.new(L, 3, 0);
    const mt = table.new(L, 0, 0);
    t.metatable = mt;
    t.arr()[0] = number(7);
    var arg = tab(t);
    var a = [_]o.TValue{ number(1), number(100000) };
    const r = L.stack + 2;
    try std.testing.expectEqual(@as(?c_int, null), fast.run(L, 53, r, &arg, &a, 3, -1));
    a[1] = number(1);
    try std.testing.expectEqual(@as(?c_int, 1), fast.run(L, 53, r, &arg, &a, 3, -1));
    try expectNum(&r[0], 7);
    try std.testing.expectEqual(@as(?c_int, 0), fast.run(L, 134, r, &arg, &a, 1, 0));
    try std.testing.expectEqual(mt, t.metatable.?);
    t.readonly = 1;
    var size = number(2);
    a[0] = tab(t);
    try std.testing.expectEqual(@as(?c_int, 1), fast.run(L, 133, r, &size, &a, 2, 1));
    const created = o.hvalue(&r[0]);
    try std.testing.expectEqual(t, o.hvalue(&created.arr()[0]));
    try std.testing.expectEqual(mt, o.hvalue(&created.arr()[1]).metatable.?);
}

test "buffer widths and boundary offsets" {
    const L = state.newstate(alloc, null) orelse return error.StateInit;
    defer state.close(L);
    var storage: [o.Buffer.header + 16]u8 align(@alignOf(o.Buffer)) = @splat(0);
    const b: *o.Buffer = @ptrCast(&storage);
    b.len = 8;
    var arg: o.TValue = .{};
    o.setbufvalue(&arg, b);
    const r = L.stack + 2;
    const ids = [_]u8{ 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 131, 132 };
    for (ids) |id| {
        const width: usize = switch (id) { 65...67 => 1, 68...70 => 2, 71...75 => 4, else => 8 };
        const writing = switch (id) { 67, 70, 73, 75, 77, 132 => true, else => false };
        var a = [_]o.TValue{ number(0), number(1) };
        if (id == 132) o.setlvalue(&a[1], 1);
        for ([_]usize{ 0, 8 - width, 9 - width }) |offset| {
            a[0] = number(@floatFromInt(offset));
            const before = storage;
            const top = L.top;
            const count = fast.run(L, id, r, &arg, &a, if (writing) 3 else 2, if (writing) 0 else 1);
            if (offset == 9 - width) {
                try std.testing.expectEqual(@as(?c_int, null), count);
                try std.testing.expectEqualSlices(u8, &before, &storage);
            } else try std.testing.expectEqual(@as(?c_int, if (writing) 0 else 1), count);
            try std.testing.expectEqual(top, L.top);
        }
    }
}
