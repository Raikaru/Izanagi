//! Bitwise library (upstream VM/src/lbitlib.cpp), translated to Zig.
//!
//! Portions derived from Luau (MIT, Copyright (c) 2019-2025 Roblox Corporation) and
//! Lua 5.x (MIT, Copyright (C) 1994-2008 Lua.org, PUC-Rio).
//!
//! Where the C code relies on unchecked int overflow / oversized shifts for extreme
//! arguments, this follows the x86 behaviour of the upstream reference builds (two's
//! complement wrap, shift counts taken modulo 32).

const api = @import("api.zig");
const aux = @import("auxiliary.zig");
const State = api.State;

const ALLONES: u32 = ~@as(u32, 0);
const NBITS: c_int = 32;

/// x86 shl/shr semantics for a (possibly out-of-range) int shift count.
inline fn shl(x: u32, n: c_int) u32 {
    return x << @truncate(@as(u32, @bitCast(n)));
}
inline fn shr(x: u32, n: c_int) u32 {
    return x >> @truncate(@as(u32, @bitCast(n)));
}

/// builds a number with 'n' ones (1 <= n <= NBITS)
inline fn mask(n: c_int) u32 {
    return ~shl(ALLONES << 1, n -% 1);
}

fn andaux(L: *State) u32 {
    const n = api.gettop(L);
    var r: u32 = ~@as(u32, 0);
    var i: c_int = 1;
    while (i <= n) : (i += 1) r &= aux.checkunsigned(L, i);
    return r;
}

fn b_and(L: *State) c_int {
    api.pushunsigned(L, andaux(L));
    return 1;
}

fn b_test(L: *State) c_int {
    api.pushboolean(L, andaux(L) != 0);
    return 1;
}

fn b_or(L: *State) c_int {
    const n = api.gettop(L);
    var r: u32 = 0;
    var i: c_int = 1;
    while (i <= n) : (i += 1) r |= aux.checkunsigned(L, i);
    api.pushunsigned(L, r);
    return 1;
}

fn b_xor(L: *State) c_int {
    const n = api.gettop(L);
    var r: u32 = 0;
    var i: c_int = 1;
    while (i <= n) : (i += 1) r ^= aux.checkunsigned(L, i);
    api.pushunsigned(L, r);
    return 1;
}

fn b_not(L: *State) c_int {
    api.pushunsigned(L, ~aux.checkunsigned(L, 1));
    return 1;
}

fn b_shift(L: *State, r_: u32, i_: c_int) c_int {
    var r = r_;
    var i = i_;
    if (i < 0) { // shift right?
        i = 0 -% i;
        if (i >= NBITS) r = 0 else r = shr(r, i);
    } else { // shift left
        if (i >= NBITS) r = 0 else r = shl(r, i);
    }
    api.pushunsigned(L, r);
    return 1;
}

fn b_lshift(L: *State) c_int {
    const r = aux.checkunsigned(L, 1);
    return b_shift(L, r, aux.checkinteger(L, 2));
}

fn b_rshift(L: *State) c_int {
    const r = aux.checkunsigned(L, 1);
    return b_shift(L, r, 0 -% aux.checkinteger(L, 2));
}

fn b_arshift(L: *State) c_int {
    var r = aux.checkunsigned(L, 1);
    const i = aux.checkinteger(L, 2);
    if (i < 0 or (r & (@as(u32, 1) << 31)) == 0)
        return b_shift(L, r, 0 -% i);
    // arithmetic shift for 'negative' number
    if (i >= NBITS) {
        r = ALLONES;
    } else {
        r = shr(r, i) | ~shr(ALLONES, i); // add signal bit
    }
    api.pushunsigned(L, r);
    return 1;
}

fn b_rot(L: *State, i_: c_int) c_int {
    var r = aux.checkunsigned(L, 1);
    const i = i_ & (NBITS - 1); // i = i % NBITS
    if (i != 0) // avoid undefined shift of NBITS when i == 0
        r = shl(r, i) | shr(r, NBITS - i);
    api.pushunsigned(L, r);
    return 1;
}

fn b_lrot(L: *State) c_int {
    return b_rot(L, aux.checkinteger(L, 2));
}

fn b_rrot(L: *State) c_int {
    return b_rot(L, 0 -% aux.checkinteger(L, 2));
}

/// A bit field: its starting bit and width.
const Field = struct { pos: c_int, width: c_int };

/// get field and width arguments for field-manipulation functions, checking whether they
/// are valid.
fn fieldargs(L: *State, farg: c_int) Field {
    const f = aux.checkinteger(L, farg);
    const w = aux.optinteger(L, farg + 1, 1);
    aux.argcheck(L, 0 <= f, farg, "field cannot be negative");
    aux.argcheck(L, 0 < w, farg + 1, "width must be positive");
    if (f +% w > NBITS)
        aux.errorf(L, "trying to access non-existent bits", .{});
    return .{ .pos = f, .width = w };
}

fn b_extract(L: *State) c_int {
    var r = aux.checkunsigned(L, 1);
    const fld = fieldargs(L, 2);
    r = shr(r, fld.pos) & mask(fld.width);
    api.pushunsigned(L, r);
    return 1;
}

fn b_replace(L: *State) c_int {
    var r = aux.checkunsigned(L, 1);
    var v = aux.checkunsigned(L, 2);
    const fld = fieldargs(L, 3);
    const m = mask(fld.width);
    v &= m; // erase bits outside given width
    r = (r & ~shl(m, fld.pos)) | shl(v, fld.pos);
    api.pushunsigned(L, r);
    return 1;
}

fn b_countlz(L: *State) c_int {
    const v = aux.checkunsigned(L, 1);
    api.pushunsigned(L, @clz(v));
    return 1;
}

fn b_countrz(L: *State) c_int {
    const v = aux.checkunsigned(L, 1);
    api.pushunsigned(L, @ctz(v));
    return 1;
}

fn b_swap(L: *State) c_int {
    const n = aux.checkunsigned(L, 1);
    api.pushunsigned(L, @byteSwap(n));
    return 1;
}

const bitlib = [_]aux.Entry{
    aux.entry("arshift", b_arshift),
    aux.entry("band", b_and),
    aux.entry("bnot", b_not),
    aux.entry("bor", b_or),
    aux.entry("bxor", b_xor),
    aux.entry("btest", b_test),
    aux.entry("extract", b_extract),
    aux.entry("lrotate", b_lrot),
    aux.entry("lshift", b_lshift),
    aux.entry("replace", b_replace),
    aux.entry("rrotate", b_rrot),
    aux.entry("rshift", b_rshift),
    aux.entry("countlz", b_countlz),
    aux.entry("countrz", b_countrz),
    aux.entry("byteswap", b_swap),
};

pub fn open(L: *State) c_int {
    aux.register(L, "bit32", &bitlib);
    return 1;
}
