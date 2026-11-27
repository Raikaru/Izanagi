//! Test-only access to the real policy's Persona-rating decision.
const std = @import("std");
pub const p4pc_test_policy_adapter = true;
const real = @import("policy.zig");

export fn p4pc_test_policy_set_ratings(data: [*c]const u8, size: usize) c_int {
    return real.p4_battle_policy_set_ratings(data, size);
}

export fn p4pc_test_policy_reset() void {
    real.policy = std.mem.zeroes(@TypeOf(real.policy));
}

export fn p4pc_test_choose_persona(actor: u32) void {
    real.choose_persona(actor);
}

export fn p4pc_test_policy_wheel() u16 {
    return real.policy.choice.wheel;
}

export fn p4pc_test_policy_menu_index() u16 {
    return real.policy.choice.menu_index;
}

export fn p4pc_test_rating(row: u32, element: u32, value: i8) void {
    real.ratings.value[row][element] = value;
}
