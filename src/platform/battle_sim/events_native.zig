const std = @import("std");
pub const panic = @import("panic.zig").panic;
const c = @import("c_bindings");

extern var p4_guest_base: [*]u8;
extern fn p4_g2h_slow(u32) callconv(.c) ?*anyopaque;
extern fn p4_battle_publish_admission([*c]const c.P4BattleAdmission) callconv(.c) void;
extern fn p4_battle_publish_cost([*c]const c.P4BattleCost) callconv(.c) void;
extern fn p4_battle_publish_hit([*c]const c.P4BattleHit) callconv(.c) void;
extern fn p4_battle_publish_command([*c]const c.P4BattleCommand) callconv(.c) void;
extern fn p4_battle_publish_request([*c]const c.P4BattleRequest) callconv(.c) void;

fn guest(comptime T: type, address: u32) T {
    const host: usize = if (@sizeOf(usize) == 8)
        (if (address -% 1 < 0x04000000 - 1) @intFromPtr(p4_guest_base) + address else @intFromPtr(p4_g2h_slow(address)))
    else address;
    const p: *align(1) volatile const T = @ptrFromInt(host);
    return p.*;
}
fn g8(a: u32) u8 { return guest(u8, a); }
fn g16(a: u32) u16 { return guest(u16, a); }
fn g32(a: u32) u32 { return guest(u32, a); }

fn actorSnapshot(action: u32) c.P4BattleActor {
    var actor = std.mem.zeroes(c.P4BattleActor);
    if (action == 0) return actor;
    const unit = g32(action +% 0x30);
    actor.action_uid = @as(u64, g32(action)) | (@as(u64, g32(action +% 4)) << 32);
    actor.unit_uid = g32(unit +% 0xa8);
    actor.native_id = g16(unit +% 0xa4);
    actor.side = g8(unit +% 0xa2);
    actor.present = 1;
    return actor;
}

export fn p4_battle_emit_admission(action: u32, queue_kind: u32, native_action_count: u32, requested_state: u32) void {
    const event = c.P4BattleAdmission{ .actor = actorSnapshot(action), .native_action_count = native_action_count, .next_state = @truncate(requested_state), .queue_kind = @truncate(queue_kind), .reserved = 0 };
    p4_battle_publish_admission(&event);
}

export fn p4_battle_emit_cost(action: u32, kind: u32, requested_cost: u32, hp_before: u32, sp_before: u32, item_id: u32, item_count_before: u32, item_count_after: u32) void {
    const unit = g32(action +% 0x30);
    const record = g32(unit +% 0xa64);
    const event = c.P4BattleCost{
        .actor = actorSnapshot(action), .requested_cost = requested_cost,
        .skill_id = g16(action +% 0x6e), .item_id = @truncate(item_id),
        .hp_before = @truncate(hp_before), .hp_after = g16(record +% 8),
        .sp_before = @truncate(sp_before), .sp_after = g16(record +% 10),
        .item_count_before = @truncate(item_count_before), .item_count_after = @truncate(item_count_after),
        .kind = @truncate(kind), .reserved = @splat(0),
    };
    p4_battle_publish_cost(&event);
}

export fn p4_battle_hit_begin(event: [*c]c.P4BattleHit, work: u32) void {
    const source = g32(work);
    const target = g32(work +% 4);
    const record = g32(g32(target +% 0x30) +% 0xa64);
    var hit = std.mem.zeroes(c.P4BattleHit);
    hit.source = actorSnapshot(source);
    hit.target = actorSnapshot(target);
    hit.requested_hp_delta = @bitCast(g32(work +% 8));
    hit.requested_sp_delta = @bitCast(g32(work +% 12));
    hit.requested_money_delta = @bitCast(g32(work +% 28));
    hit.added_status = g32(work +% 16);
    hit.removed_status = g32(work +% 20);
    hit.buff_changes = g32(work +% 24);
    hit.status_before = g32(record +% 12);
    hit.skill_id = if (source != 0) g16(source +% 0x6e) else 0;
    hit.action_kind = if (source != 0) g16(source +% 0x6c) else 0;
    hit.hp_before = g16(record +% 8);
    hit.sp_before = g16(record +% 10);
    hit.target_flags = g16(work +% 42);
    hit.result_flags = g16(work +% 38);
    hit.effect = g16(work +% 40);
    event.* = hit;
}

export fn p4_battle_hit_end(event: [*c]c.P4BattleHit, target_action: u32) void {
    const record = g32(g32(target_action +% 0x30) +% 0xa64);
    event.*.hp_after = g16(record +% 8);
    event.*.sp_after = g16(record +% 10);
    event.*.status_after = g32(record +% 12);
    p4_battle_publish_hit(event);
}

export fn p4_battle_emit_command(action: u32, action_kind: u32) void {
    const event = c.P4BattleCommand{
        .actor = actorSnapshot(action), .action_kind = @truncate(action_kind), .skill_id = g16(action +% 0x6e),
        .item_id = if (action_kind == 3) g16(action +% 0x70) else 0,
        .target_count = g16(action +% 0x6a), .native_state = g16(action +% 0x0c),
        .free_action = @intFromBool(action_kind == 5 or action_kind == 10), .reserved = 0,
    };
    p4_battle_publish_command(&event);
}

export fn p4_battle_emit_request(stage: u32, encounter_id: u32, guest_object: u32, value: u32) void {
    const event = c.P4BattleRequest{ .stage = stage, .encounter_id = encounter_id, .guest_object = guest_object, .value = value };
    p4_battle_publish_request(&event);
}
