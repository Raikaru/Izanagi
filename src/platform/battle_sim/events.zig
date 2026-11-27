const std = @import("std");
pub const panic = @import("panic.zig").panic;
const c = @import("c_bindings");

export var p4_battle_observer_interest: u32 = 0;
const Subscriber = struct { token: u32 = 0, observer: c.P4BattleObserver = std.mem.zeroes(c.P4BattleObserver) };
var subscribers: [8]Subscriber = @splat(Subscriber{});
var extent: usize = 0;
var delivering: c_uint = 0;
var last_token: u32 = 0;

fn refreshInterest() void {
    var interest: u32 = 0;
    while (extent != 0 and subscribers[extent - 1].token == 0) extent -= 1;
    for (subscribers[0..extent]) |s| {
        if (s.token == 0) continue;
        if (s.observer.admission != null) interest |= c.P4_BATTLE_OBSERVE_ADMISSION;
        if (s.observer.cost != null) interest |= c.P4_BATTLE_OBSERVE_COST;
        if (s.observer.hit != null) interest |= c.P4_BATTLE_OBSERVE_HIT;
        if (s.observer.command != null) interest |= c.P4_BATTLE_OBSERVE_COMMAND;
        if (s.observer.request != null) interest |= c.P4_BATTLE_OBSERVE_REQUEST;
    }
    p4_battle_observer_interest = interest;
}

export fn p4_battle_observer_add(observer: [*c]const c.P4BattleObserver) u32 {
    if (observer == null or delivering != 0 or last_token == std.math.maxInt(u32)) return 0;
    const o = observer.*;
    if (o.admission == null and o.cost == null and o.hit == null and o.command == null and o.request == null) return 0;
    var slot: usize = 0;
    while (slot < subscribers.len and subscribers[slot].token != 0) : (slot += 1) {}
    if (slot == subscribers.len) return 0;
    subscribers[slot].observer = o;
    last_token += 1;
    subscribers[slot].token = last_token;
    if (extent <= slot) extent = slot + 1;
    refreshInterest();
    return subscribers[slot].token;
}

export fn p4_battle_observer_remove(token: u32) c_int {
    if (token == 0 or delivering != 0) return 0;
    for (subscribers[0..extent]) |*s| {
        if (s.token != token) continue;
        s.* = .{};
        refreshInterest();
        return 1;
    }
    return 0;
}

fn publish(comptime field: []const u8, event: anytype) void {
    delivering +%= 1;
    var i: usize = 0;
    while (i < extent) : (i += 1) {
        const s = &subscribers[i];
        if (s.token != 0) {
            if (@field(s.observer, field)) |callback| callback(event, s.observer.context);
        }
    }
    delivering -%= 1;
}

export fn p4_battle_publish_admission(event: [*c]const c.P4BattleAdmission) void { publish("admission", event); }
export fn p4_battle_publish_cost(event: [*c]const c.P4BattleCost) void { publish("cost", event); }
export fn p4_battle_publish_hit(event: [*c]const c.P4BattleHit) void { publish("hit", event); }
export fn p4_battle_publish_command(event: [*c]const c.P4BattleCommand) void { publish("command", event); }
export fn p4_battle_publish_request(event: [*c]const c.P4BattleRequest) void { publish("request", event); }
