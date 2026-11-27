//! Test-only script-visible raw arithmetic NaN measurement.
const std = @import("std");
const State = @import("State.zig");

test "raw arithmetic nan visibility" {
    var state = try State.init(std.testing.allocator, .{});
    defer state.deinit();
    const result = try state.run("=raw-nan-probe",
        \\local zero = 0
        \\local inf = 1 / zero
        \\local function make(lo, hi)
        \\    local raw = buffer.create(8)
        \\    buffer.writeu32(raw, 0, lo)
        \\    buffer.writeu32(raw, 4, hi)
        \\    return buffer.readf64(raw, 0)
        \\end
        \\local values = { zero / zero, inf - inf, zero * inf, math.fmod(1, zero),
        \\    make(0, 0x7ff80000), make(0, 0xfff80000), make(1, 0x7ff00000), make(1, 0xfff00000),
        \\    make(0x1234, 0x7ff80000), make(0xabcd, 0xfff80000) }
        \\local names = { 'zero-div-zero', 'inf-sub-inf', 'zero-mul-inf', 'fmod-zero',
        \\    'quiet-pos', 'quiet-neg', 'signaling-pos', 'signaling-neg', 'payload-pos', 'payload-neg' }
        \\local lines = {}
        \\for i, value in values do
        \\    local bytes = buffer.create(8)
        \\    buffer.writef64(bytes, 0, value)
        \\    local packed = string.pack('<d', value)
        \\    local a, b = '', ''
        \\    for j = 0, 7 do
        \\        a ..= string.format('%02x', buffer.readu8(bytes, j))
        \\        b ..= string.format('%02x', string.byte(packed, j + 1))
        \\    end
        \\    local f32 = buffer.create(4)
        \\    buffer.writef32(f32, 0, value)
        \\    local c = ''
        \\    for j = 0, 3 do c ..= string.format('%02x', buffer.readu8(f32, j)) end
        \\    lines[i] = names[i] .. ' buffer=' .. a .. ' buffer32=' .. c .. ' pack=' .. b .. ' pack32=' .. string.format('%02x', string.byte(string.pack('>f', value), 1)) .. ' tostring=' .. tostring(value) .. ' format=' .. string.format('%g/%E/%+f', value, value, value)
        \\end
        \\return table.concat(lines, '\n')
    , .{ .optimization_level = 0 });
    try std.testing.expectEqual(@as(usize, 1), result.len);
    std.debug.print("RAW_NAN_BEGIN\n{s}\nRAW_NAN_END\n", .{result.get(0).string});
}
