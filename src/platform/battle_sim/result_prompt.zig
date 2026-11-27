pub const panic = @import("panic.zig").panic;
const c = @import("c_bindings");
const cross: c_uint = 0x4000;
const left: c_uint = 0x0080;
const down: c_uint = 0x0040;
const up: c_uint = 0x0010;

export fn p4_battle_skill_prompt_buttons(prompt: [*c]const c.P4BattleSkillPrompt) c_uint {
    const p = prompt.*;
    return switch (p.state) {
        0, 1, 6 => if (p.row_waiting != 0) cross else 0,
        2 => if (p.list_active == 0 or p.list_cursor < 0 or p.pending <= 0) 0 else if (p.list_cursor == 8) cross else left,
        3 => if (p.choice_active != 0) blk: {
            const wanted: i32 = if (p.replacement_pending != 0) 0 else 1;
            break :blk if (p.choice_cursor == wanted) cross else if (wanted != 0) down else up;
        } else if (p.row_waiting != 0) cross else 0,
        else => 0,
    };
}
