//! The strategic candidate's pure decision rules (include/p4pc/battle_strategy.h).
//! Exported through the native C ABI.
pub const panic = @import("panic.zig").panic;

/// include/p4pc/battle_strategy.h constants.
const P4_STRATEGY_SETUP: i64 = 1200;
const P4_STRATEGY_SWITCH_MARGIN: i64 = 300;
const P4_STRATEGY_BUFF_RAISE: u32 = 0x155;
const P4_STRATEGY_BUFF_LOWER: u32 = 0x2AA;
const P4_STRATEGY_PARTY: c_uint = 0;
const P4_STRATEGY_ENEMY: c_uint = 1;
const P4_STRATEGY_COMMIT_DAMAGE: c_uint = 1;
const P4_STRATEGY_COMMIT_SETUP: c_uint = 2;

/// struct P4StrategyEnemy { uint32_t tried, weak; uint8_t living; }
pub const P4StrategyEnemy = extern struct {
    tried: u32,
    weak: u32,
    living: u8,
};
/// struct P4StrategyTempo { uint8_t setup_pending; }
pub const P4StrategyTempo = extern struct {
    setup_pending: u8,
};

comptime {
    if (@sizeOf(P4StrategyEnemy) != 12 or @offsetOf(P4StrategyEnemy, "living") != 8) @compileError("P4StrategyEnemy layout");
    if (@sizeOf(P4StrategyTempo) != 1) @compileError("P4StrategyTempo layout");
}

export fn p4_strategy_setup_direction(mask: u32, side: c_uint) c_int {
    const helps: u32 = if (side == P4_STRATEGY_PARTY) P4_STRATEGY_BUFF_RAISE else P4_STRATEGY_BUFF_LOWER;
    return @intFromBool(side <= P4_STRATEGY_ENEMY and mask != 0 and (mask & ~helps) == 0);
}

export fn p4_strategy_durable(enemies: [*c]const P4StrategyEnemy, count: c_uint) c_int {
    var living: c_uint = 0;
    var i: c_uint = 0;
    while (i < count) : (i += 1) {
        if (enemies[i].living == 0) continue;
        if (enemies[i].tried == 0 or enemies[i].weak != 0) return 0;
        living +%= 1;
    }
    return @intFromBool(living != 0);
}

export fn p4_strategy_setup_allowed(tempo: *const P4StrategyTempo) c_int {
    return @intFromBool(tempo.setup_pending == 0);
}

export fn p4_strategy_tempo_commit(tempo: *P4StrategyTempo, kind: c_uint) void {
    if (kind == P4_STRATEGY_COMMIT_SETUP) {
        tempo.setup_pending = 1;
    } else if (kind == P4_STRATEGY_COMMIT_DAMAGE) {
        tempo.setup_pending = 0;
    }
}

export fn p4_strategy_new_encounter(previous: [*c]const u32, previous_count: c_uint, current: [*c]const u32, current_count: c_uint) c_int {
    if (previous_count == 0 or current_count == 0) return 0;
    var i: c_uint = 0;
    while (i < current_count) : (i += 1) {
        var j: c_uint = 0;
        while (j < previous_count) : (j += 1) {
            if (current[i] == previous[j]) return 0;
        }
    }
    return 1;
}

export fn p4_strategy_setup_value(mask: u32, side: c_uint, durable: c_int, tempo: *const P4StrategyTempo, inherited: i64) i64 {
    if (durable == 0 or p4_strategy_setup_direction(mask, side) == 0 or p4_strategy_setup_allowed(tempo) == 0) return inherited;
    return if (P4_STRATEGY_SETUP > inherited) P4_STRATEGY_SETUP else inherited;
}

export fn p4_strategy_heal_value(inherited: i64, fixed: c_int, power: u32, hp: u32, max_hp: u32) i64 {
    if (fixed == 0 or hp >= max_hp) return inherited;
    const missing: u32 = max_hp - hp;
    // C int64_t subtraction; wrapping mirrors the compiled two's-complement result (unreachable from policy scores).
    return if (missing > power) inherited -% @as(i64, missing - power) else inherited;
}

export fn p4_strategy_switch_priceable(cost_kind: c_uint, percent: c_int, cost_stage: c_int, active_master: c_int, candidate_master: c_int) c_int {
    // datCalcSkillCost prices only kinds 1 (HP) and 2 (SP).
    if (cost_kind != 1 and cost_kind != 2) return 1;
    return @intFromBool(cost_kind == 2 and percent == 0 and cost_stage == 0 and (active_master == 0) == (candidate_master == 0));
}

export fn p4_strategy_switch(current: i64, candidate: i64, switched: c_int) c_int {
    if (switched != 0 or candidate == std.math.minInt(i64)) return 0;
    // current is a real score or INT64_MIN (no legal command yet); neither overflows.
    return @intFromBool(current < std.math.maxInt(i64) - P4_STRATEGY_SWITCH_MARGIN and candidate > current + P4_STRATEGY_SWITCH_MARGIN);
}


const std = @import("std");
