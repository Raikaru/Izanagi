//! Ruling LZ-NaN: every NaN a script can see as bytes or text is the canonical quiet NaN, whatever
//! sign and payload the VM carried. Raw NaNs are built bit-exactly with buffer.readf64, so the result
//! does not depend on which NaN the host CPU's arithmetic happens to produce. Finite values,
//! infinities and signed zeros must pass through the same boundaries unchanged.
const std = @import("std");
const State = @import("State.zig");

const script =
    \\local function make(lo, hi)
    \\    local b = buffer.create(8)
    \\    buffer.writeu32(b, 0, lo)
    \\    buffer.writeu32(b, 4, hi)
    \\    return buffer.readf64(b, 0)
    \\end
    \\-- +/- quiet, +/- signaling, payloads, all-ones
    \\local nans = {
    \\    make(0, 0x7ff80000), make(0, 0xfff80000), make(1, 0x7ff00000), make(1, 0xfff00000),
    \\    make(0x1234, 0x7ff80000), make(0xabcd, 0xfff80000), make(0xffffffff, 0x7fffffff), make(0xffffffff, 0xffffffff),
    \\}
    \\local zero = 0
    \\local arith = { zero / zero, (1 / zero) - (1 / zero), zero * (1 / zero), math.fmod(1, zero), -(zero / zero) }
    \\for _, v in arith do table.insert(nans, v) end
    \\
    \\local function hex(s)
    \\    local r = ''
    \\    for i = 1, #s do r ..= string.format('%02x', string.byte(s, i)) end
    \\    return r
    \\end
    \\local function check(cond, what, i)
    \\    if not cond then error(what .. ' (case ' .. i .. ')', 0) end
    \\end
    \\
    \\for i, v in nans do
    \\    check(v ~= v, 'not a NaN', i)
    \\    local b = buffer.create(16)
    \\    buffer.writef64(b, 0, v)
    \\    check(buffer.readu32(b, 0) == 0 and buffer.readu32(b, 4) == 0x7ff80000, 'writef64 bytes', i)
    \\    buffer.writef32(b, 8, v)
    \\    check(buffer.readu32(b, 8) == 0x7fc00000, 'writef32 bytes', i)
    \\    check(hex(string.pack('<d', v)) == '000000000000f87f', 'pack <d', i)
    \\    check(hex(string.pack('>d', v)) == '7ff8000000000000', 'pack >d', i)
    \\    check(hex(string.pack('<n', v)) == '000000000000f87f', 'pack <n', i)
    \\    check(hex(string.pack('>n', v)) == '7ff8000000000000', 'pack >n', i)
    \\    check(hex(string.pack('<f', v)) == '0000c07f', 'pack <f', i)
    \\    check(hex(string.pack('>f', v)) == '7fc00000', 'pack >f', i)
    \\    check(tostring(v) == 'nan', 'tostring', i)
    \\    check(('' .. v) == 'nan', 'concat', i)
    \\    check(string.format('%s', v) == 'nan', 'format %s', i)
    \\    check(string.format('%f', v) == 'nan' and string.format('%e', v) == 'nan' and string.format('%g', v) == 'nan', 'format lower', i)
    \\    check(string.format('%E', v) == 'NAN' and string.format('%G', v) == 'NAN' and string.format('%.3E', v) == 'NAN', 'format upper', i)
    \\    check(string.format('%10.3f', v) == '       nan' and string.format('%-10g|', v) == 'nan       |', 'format width', i)
    \\    check(string.format('%.0f %#g %010e', v, v, v) == string.format('%.0f %#g %010e', nans[1], nans[1], nans[1]), 'format flags match canonical', i)
    \\    check(not string.find(string.format('%+f% g%+E', v, v, v), '-', 1, true), 'format sign flags have no minus', i)
    \\end
    \\
    \\-- finite numbers, infinities and signed zeros are untouched
    \\local keep = { 0, 1, -1, 1.5, -2.25e300, 5e-324, 1 / zero, -1 / zero, make(0, 0x80000000), make(0, 0) }
    \\for i, v in keep do
    \\    local b = buffer.create(8)
    \\    buffer.writef64(b, 0, v)
    \\    check(buffer.readf64(b, 0) == v, 'writef64 value', i)
    \\    check(string.unpack('<d', string.pack('<d', v)) == v, 'pack d value', i)
    \\    check(string.unpack('>n', string.pack('>n', v)) == v, 'pack n value', i)
    \\end
    \\local negzero = make(0, 0x80000000)
    \\local b = buffer.create(16)
    \\buffer.writef64(b, 0, negzero)
    \\check(buffer.readu32(b, 4) == 0x80000000, 'negative zero sign kept', 0)
    \\buffer.writef64(b, 8, -1 / zero)
    \\check(buffer.readu32(b, 12) == 0xfff00000, 'negative infinity kept', 0)
    \\check(hex(string.pack('<f', 1.5)) == '0000c03f' and hex(string.pack('>f', -0.0)) == '80000000', 'f32 finite and negative zero', 0)
    \\check(string.format('%g %g %g', 1 / zero, -1 / zero, negzero) == 'inf -inf -0', 'text finite forms', 0)
    \\
    \\-- VM values keep whatever the arithmetic produced: the raw bits survive reads and copies
    \\local raw = make(0xdead, 0xfff80000)
    \\local copy = raw
    \\buffer.writef64(b, 0, copy)
    \\check(buffer.readu32(b, 4) == 0x7ff80000, 'copy writes canonical', 0)
    \\return 'ok'
;

test "script-visible NaN output is canonical" {
    inline for (.{ 0, 1, 2 }) |level| {
        var state = try State.init(std.testing.allocator, .{});
        defer state.deinit();
        const result = try state.run("=nan-boundary", script, .{ .optimization_level = level });
        try std.testing.expectEqual(@as(usize, 1), result.len);
        try std.testing.expectEqualStrings("ok", result.get(0).string);
    }
}
