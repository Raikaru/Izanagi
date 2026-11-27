//! Builtin FASTCALL dispatch. Family implementations follow upstream lbuiltins.cpp
//! (Luau, MIT; see third_party/luau/LICENSE.txt).
const o = @import("object.zig");
const math = @import("fastmath.zig");
const misc = @import("fastmisc.zig");
const vector = @import("fastvector.zig");

/// Per-operation domain NaN bits match x86-64 glibc and are shared with
/// normal library calls and compiler folding. Raw arithmetic NaNs are outside
/// this table. See luau_math.zig for the fixed ID-to-bits mapping.
/// acos/asin/log10: 0x7ff8000000000000; cos/log/pow/sin/sqrt/tan:
/// 0xfff8000000000000. Input NaNs are quieted with sign and payload preserved.
pub const domain_nan_bits = @import("../luau_math.zig").domain_nan_bits;

/// Return null when the upstream fast function declines and the VM must execute
/// the original fallback CALL; otherwise return the number of produced values.
pub fn run(L: *o.State, id: u8, result: o.StkId, arg0: *o.TValue, args: ?o.StkId, nparams: c_int, nresults: c_int) ?c_int {
    return switch (id) {
        2...39, 46...48, 55...56, 59, 64, 89, 91...93 => math.run(L, id, result, arg0, args, nparams, nresults),
        41...43, 45, 49...53, 58, 60...63, 65...77, 131...134 => misc.run(L, id, result, arg0, args, nparams, nresults),
        1, 40, 44, 54, 57, 78...88, 90, 94...130 => vector.run(L, id, result, arg0, args, nparams, nresults),
        else => null, // upstream LBF_NONE or invalid bytecode
    };
}
