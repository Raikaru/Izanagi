const std = @import("std");
const builtin = @import("builtin");
pub const panic = @import("panic.zig").panic;
const c = @import("c_bindings");
extern fn __acrt_iob_func(c_uint) ?*c.FILE;
extern var __stderrp: ?*c.FILE;
extern var stderr: ?*c.FILE;
fn cStderr() ?*c.FILE {
    if (builtin.os.tag == .windows) return __acrt_iob_func(2);
    if (comptime builtin.os.tag.isDarwin()) return __stderrp;
    return stderr;
}
extern var p4_guest_base: [*]u8;
extern fn p4_g2h_slow(u32) ?*anyopaque;
extern fn p4_h2g_slow(?*const anyopaque) u32;
extern fn p4_gport_alloc(u32,u32) u32;
extern fn p4_game_table_extent_rows(u32,u32,*u32,*u32) c_int;
extern fn p4_game_enemies_native_limit() u32;
extern fn datUnitCreatePc(u16) ?*anyopaque;
extern fn datUnitCreateEncounterGroup(u16) ?*anyopaque;
extern fn datGetPartyId(i32) i16;
extern fn hFadeTaskStart(i16) void;
extern fn sequenceRequest(i32,u32,i32,i32) void;
fn g2h(a: u32) [*]u8 {
    if (@sizeOf(usize) == 4) return @ptrFromInt(a);
    if (a -% 1 < 0x3ffffff) return p4_guest_base + a;
    return @ptrCast(p4_g2h_slow(a).?);
}
fn h2g(p: ?*const anyopaque) u32 {
    if (@sizeOf(usize) == 4) return @truncate(@intFromPtr(p));
    const d = @intFromPtr(p) -% @intFromPtr(p4_guest_base);
    if (d -% 1 < 0x3ffffff) return @truncate(d);
    return p4_h2g_slow(p);
}
fn word(p: [*]const u8) u32 { return std.mem.readInt(u32, p[0..4], .little); }
fn half(p: [*]const u8) u16 { return std.mem.readInt(u16, p[0..2], .little); }
export fn p4_battle_save_valid(image: ?[*]const u8, size: u32) c_int {
    const p = image orelse return 0;
    if (size < 56 or size > 0x2a000 or word(p) != 0) return 0;
    var summary: u8 = 0;
    for (p[4..54]) |b| summary +%= b;
    if (p[54] != summary or p[55] != 0) return 0;
    var offset: u32 = 56;
    var checksum: u8 = 0;
    var row: usize = 0;
    while (row < c.p4_battle_save_sections.len) : (row += 1) {
        if (size - offset < 8) return 0;
        const tag = word(p + offset);
        const len = word(p + offset + 4);
        // Compact saves omit the three field sections together.
        if (c.p4_battle_save_sections[row].tag == 0x25 and tag == 0x28) row += 3;
        const section = c.p4_battle_save_sections[row];
        if (tag != section.tag or len != section.bytes or len > size - offset - 8) return 0;
        for (p[offset..][0 .. len + 8]) |b| checksum +%= b;
        offset += len + 8;
    }
    return @intFromBool(size - offset >= 13 and word(p + offset) == 0x2000 and
        word(p + offset + 4) == 1 and p[offset + 8] == checksum and
        word(p + offset + 9) == 0xffffffff);
}
export fn p4_battle_encounter_start(id: u32) c_int {
    var base: u32 = undefined;
    var count: u32 = undefined;
    var enemy_base: u32 = undefined;
    var enemy_count: u32 = undefined;
    if (p4_game_table_extent_rows(0x764504, 0x18, &base, &count) == 0 or id >= count or
        p4_game_table_extent_rows(0x7644b4, 0x3c, &enemy_base, &enemy_count) == 0) return 0;
    const limit = p4_game_enemies_native_limit();
    const record = g2h(base +% (id *% 0x18));
    var populated = false;
    for (0..5) |i| {
        const enemy = half(record + 8 + i * 2);
        if (enemy >= limit or enemy >= enemy_count) return 0;
        if (enemy != 0) populated = true;
    }
    if (!populated) return 0;
    var party = [_]u32{ 1, 0, 0, 0 };
    var members: usize = 1;
    var seen: u32 = 2;
    for (0..4) |slot| {
        if (members >= 4) break;
        const pc: u32 = @as(u16, @bitCast(datGetPartyId(@intCast(slot))));
        if (pc == 0) continue;
        if (pc < 2 or pc > 10 or seen & (@as(u32, 1) << @intCast(pc)) != 0) return 0;
        seen |= @as(u32, 1) << @intCast(pc);
        party[members] = pc;
        members += 1;
    }
    const address = p4_gport_alloc(0x3c, 16);
    if (address == 0) return 0;
    const packet = g2h(address);
    hFadeTaskStart(1);
    for (0..members) |slot|
        std.mem.writeInt(u32, packet[4 + slot * 4 ..][0..4], h2g(datUnitCreatePc(@intCast(party[slot]))), .little);
    const group = h2g(datUnitCreateEncounterGroup(@truncate(id)));
    std.mem.writeInt(u32, packet[0x14..0x18], group, .little);
    var field = half(record + 0x12);
    var variant = half(record + 0x14);
    if (field == 0 and variant == 0) {
        field = 0xf0;
        variant = 1;
    }
    std.mem.writeInt(u16, packet[0x20..0x22], field, .little);
    std.mem.writeInt(u16, packet[0x22..0x24], variant, .little);
    _ = c.fprintf(cStderr(), "p4pc: encounter adapter: encounter %u, %u party, enemy group %08x, formation %u:%u\n",
        id, @as(c_uint, @intCast(members)), group, @as(c_uint, field), @as(c_uint, variant));
    sequenceRequest(6, address, 0x3c, 4);
    return 1;
}
