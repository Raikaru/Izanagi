//! Observation-only expert decision scoring. Mirrors expert.c; no guest reads.
const std = @import("std");
const c = @import("c_bindings");
pub const panic = @import("panic.zig").panic;
const Observation = c.struct_bsim_observation;
const Candidate = c.struct_bsim_candidate_observation;
const Choice = c.struct_bsim_choice;

fn damage(e: u8) bool { return e == 1 or e == 4 or e == 8 or e == 10 or e == 12 or e == 13 or e == 14; }
fn healing(e: u8) bool { return e == 2 or e == 5 or e == 7 or e == 9 or e == 11 or e == 15; }
fn percent(n: u32, d: u32) u32 { return if (d == 0) 0 else if (n >= d) 100 else n * 100 / d; }
fn bit(index: usize) u16 { return @as(u16, 1) << @as(u4, @intCast(index)); }
fn elementBit(element: i16) u32 { return @as(u32, 1) << @as(u5, @intCast(element)); }

fn targetScore(o: *const Observation, cand: *const Candidate, index: usize, expert: bool) i64 {
    const unit = &o.units[index];
    const ailments: u32 = if (cand.status_mode == 1) cand.status_mask & 0x1e else 0;
    const power: u32 = if (cand.power > 0) @intCast(cand.power) else 0;
    if (unit.side == 1) {
        var technical: i64 = 0;
        if (ailments != 0 and unit.status & ailments == 0) {
            if (unit.vulnerable & ailments != 0) technical += 2600 else if (unit.probed2 & ailments == 0) { technical += 500; }
        }
        const paired = [_]u32{ 0, 2, 4, 8, 16 };
        if (damage(cand.hp_effect) and cand.element >= 1 and cand.element <= 4 and
            unit.status & paired[@intCast(cand.element)] != 0 and unit.weak & elementBit(cand.element) == 0) technical += 1500;
        if (damage(cand.hp_effect)) {
            var score: i64 = 200 + @as(i64, power);
            if (cand.element >= 0 and cand.element < 32) {
                const mask = elementBit(cand.element);
                if (unit.weak & mask != 0) score += 3000 else if (unit.bad & mask != 0) return -score else if (unit.tried & mask == 0) { score += 160; }
            }
            if (unit.unit_uid == o.focus_uid) score += 400;
            for (0..o.healer_count) |i| if (unit.unit_uid == o.healer_uid[i]) { score += 700; };
            return score + technical;
        }
        if (cand.buff_mask != 0 and cand.held_targets & bit(index) == 0) {
            const tempo = c.struct_P4StrategyTempo{ .setup_pending = o.setup_pending };
            return c.p4_strategy_setup_value(cand.buff_mask, c.P4_STRATEGY_ENEMY, o.durable, &tempo, 240) + technical;
        }
        return technical;
    }
    if (unit.side != 0 or unit.native_id < 1 or unit.native_id > 10) return 0;
    if (damage(cand.hp_effect)) return -20000;
    const hp_pct = percent(unit.hp, unit.max_hp);
    const sp_pct = percent(unit.sp, unit.max_sp);
    const removes = cand.status_mode == 2;
    if (unit.hp == 0 and removes and cand.status_mask & 0x80000 != 0) return 30000;
    const threshold: u32 = if (unit.native_id == 1) 70 else 50;
    if (unit.hp != 0 and healing(cand.hp_effect) and hp_pct <= threshold) {
        if (cand.hp_effect == 5 and hp_pct > 25 and power * 4 <= unit.max_hp) return 300;
        const inherited: i64 = 24000 + @as(i64, 100 - hp_pct) * 20 + @as(i64, if (unit.native_id == 1) 500 else 0);
        const amount = if (expert and cand.hp_effect == 11)
            @max(@as(u32, 1), @as(u32, unit.max_hp) * power / 100)
        else
            power;
        return c.p4_strategy_heal_value(inherited, @intFromBool(cand.hp_effect == 5 or (expert and cand.hp_effect == 11)), amount, unit.hp, unit.max_hp);
    }
    if (removes and unit.status & cand.status_mask != 0) return 14000;
    if (unit.hp != 0 and healing(cand.sp_effect) and sp_pct < 35) return 9000 + @as(i64, 100 - sp_pct) * 10;
    if (unit.hp != 0 and cand.buff_mask != 0 and cand.held_targets & bit(index) == 0) {
        const tempo = c.struct_P4StrategyTempo{ .setup_pending = o.setup_pending };
        return c.p4_strategy_setup_value(cand.buff_mask, c.P4_STRATEGY_PARTY, o.durable, &tempo, 260);
    }
    return 0;
}

fn personaAttackNotWeaker(o: *const Observation, cand: *const Candidate) bool {
    if (cand.persona_slot == 255 or !damage(cand.hp_effect)) return true;
    var lead: ?*const c.struct_bsim_persona_observation = null;
    var candidate: ?*const c.struct_bsim_persona_observation = null;
    for (0..o.persona_count) |i| {
        if (o.personas[i].slot == o.lead_slot) lead = &o.personas[i];
        if (o.personas[i].slot == cand.persona_slot) candidate = &o.personas[i];
    }
    const a = lead orelse return false;
    const b = candidate orelse return false;
    if (cand.element == 0) return b.stats[0] >= a.stats[0];
    if (cand.element > 0) return b.stats[1] >= a.stats[1];
    return b.stats[0] >= a.stats[0] and b.stats[1] >= a.stats[1];
}

fn better(o: *const Observation, score: i64, ci: usize, ti: usize, best: *const Choice) bool {
    if (score != best.score) return score > best.score;
    const cand = &o.candidates[ci];
    const prior = &o.candidates[best.candidate];
    return cand.skill < prior.skill or (cand.skill == prior.skill and
        (cand.item < prior.item or (cand.item == prior.item and o.units[ti].action_uid < o.units[best.target].action_uid)));
}

export fn bsim_choose(o_arg: [*c]const Observation, expert: c_int, out: [*c]Choice) c_int {
    if (o_arg == null or out == null) return 0;
    const o: *const Observation = @ptrFromInt(@intFromPtr(o_arg));
    if (o.actor_index >= o.unit_count) return 0;
    var best = Choice{ .score = std.math.minInt(i64), .candidate = 0, .target = 0 };
    var persona = best;
    const actor = &o.units[o.actor_index];
    var charged: u32 = 0;
    for (0..o.unit_count) |i| {
        const unit = &o.units[i];
        if (unit.side == 1 and unit.status & 0x80000 == 0) charged |= unit.charged;
    }
    for (0..o.candidate_count) |ci| {
        const cand = &o.candidates[ci];
        if (expert != 0 and !personaAttackNotWeaker(o, cand)) continue;
        if (expert != 0 and o.is_boss == 0 and percent(actor.sp, actor.max_sp) < 30 and !damage(cand.hp_effect) and cand.buff_mask & c.P4_STRATEGY_BUFF_RAISE != 0) continue;
        const selected = if (cand.persona_slot == 255) &best else &persona;
        if (cand.wheel == 2) {
            const score: i64 = if (charged != 0) (if (expert != 0) @as(i64, 4500000) else 20000) else if (percent(actor.hp, actor.max_hp) < 20) 450 else 10;
            if (score > selected.score) selected.* = .{ .score = score, .candidate = @intCast(ci), .target = o.actor_index };
            continue;
        }
        var penalty: i64 = @as(i64, cand.cost) * (if (cand.cost_kind == 1) @as(i64, 4) else 5);
        if (cand.cost_kind == 2 and percent(actor.sp, actor.max_sp) < 30) penalty += @as(i64, cand.cost) * 60;
        for (0..o.unit_count) |ti| {
            if (cand.targets & bit(ti) == 0 or (cand.group != 0 and ti != cand.first_target)) continue;
            var score: i64 = -penalty;
            var priority: i64 = 0;
            for (0..o.unit_count) |j| {
                if (cand.targets & bit(j) == 0 or (cand.group == 0 and j != ti)) continue;
                const unit = &o.units[j];
                score += targetScore(o, cand, j, expert != 0);
                if (expert == 0) continue;
                const hp = percent(unit.hp, unit.max_hp);
                if (unit.side == 0 and ((unit.hp == 0 and cand.status_mode == 2 and cand.status_mask & 0x80000 != 0) or
                    (unit.hp != 0 and healing(cand.hp_effect) and hp <= (if (unit.native_id == 1) @as(u32, 70) else 50)))) priority = @max(priority, 5000000);
                // Removing a party debuff is a tier; dispelling an enemy buff is not (EB-9a).
                var negative = false;
                for (unit.buffs) |v| negative = negative or v < 0;
                if (unit.side == 0 and negative and cand.buff_mask & 0x2000 != 0) priority = @max(priority, 4000000);
                if (unit.side == 0 and !damage(cand.hp_effect) and cand.buff_mask & unit.expired_buffs & 0x45 != 0) score += 100;
                if (unit.side != 0 and damage(cand.hp_effect) and cand.element >= 0 and cand.element < 32 and
                    unit.weak & elementBit(cand.element) != 0) priority = @max(priority, 3500000);
            }
            score += priority;
            const other = &o.candidates[selected.candidate];
            const improves = if (selected == &persona and score == selected.score and cand.persona_slot != other.persona_slot)
                cand.persona_slot < other.persona_slot
            else better(o, score, ci, ti, selected);
            if (selected.score == std.math.minInt(i64) or improves)
                selected.* = .{ .score = score, .candidate = @intCast(ci), .target = @intCast(ti) };
        }
    }
    if (o.switched == 0 and c.p4_strategy_switch(best.score, persona.score, 0) != 0) best = persona;
    if (best.score == std.math.minInt(i64)) return 0;
    out[0] = best;
    return 1;
}
