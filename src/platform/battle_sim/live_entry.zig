const std = @import("std");
pub const panic = @import("panic.zig").panic;
const Entry = extern struct {
    flags: u32, mode: u32, encounter: u32, occurrence: u32,
    route_limit: u32, fight_limit: u32, action_limit: u32, completion: u32,
    endpoint_mask: u32, field: u32, room: u32, date: u32, step: u32,
};
fn word(p: [*]const u8) u32 { return std.mem.readInt(u32, p[0..4], .little); }
export fn p4_battle_live_decode(bytes: ?[*]const u8, length: usize, out: ?*Entry) c_int {
    const p = bytes orelse return 0;
    const dst = out orelse return 0;
    if (length != 64 or !std.mem.eql(u8, p[0..8], "P4BLIVE1") or word(p + 8) != 64) return 0;
    const e = Entry{
        .flags = word(p + 12), .mode = word(p + 16),
        .encounter = word(p + 20), .occurrence = word(p + 24),
        .route_limit = word(p + 28), .fight_limit = word(p + 32),
        .action_limit = word(p + 36), .completion = word(p + 40),
        .endpoint_mask = word(p + 44), .field = word(p + 48),
        .room = word(p + 52), .date = word(p + 56), .step = word(p + 60),
    };
    if (e.flags & ~@as(u32, 3) != 0 or
        (e.mode != 1 and e.mode != 2 and e.mode != 3 and e.mode != 4) or
        e.encounter > 65535 or e.route_limit == 0 or e.fight_limit == 0 or
        e.action_limit == 0 or e.occurrence == 0 or
        (e.mode == 3 and (e.encounter != 0 or e.completion != 2)) or
        (e.completion != 1 and e.completion != 2) or e.endpoint_mask & ~@as(u32, 15) != 0 or
        (e.completion == 1 and e.endpoint_mask != 0) or
        (e.completion == 2 and e.endpoint_mask == 0) or
        (e.endpoint_mask & 1 == 0 and e.field != 0) or
        (e.endpoint_mask & 2 == 0 and e.room != 0) or
        (e.endpoint_mask & 4 == 0 and e.date != 0) or
        (e.endpoint_mask & 8 == 0 and e.step != 0)) return 0;
    dst.* = e;
    return 1;
}
