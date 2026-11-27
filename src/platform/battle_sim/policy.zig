//! Tool party policies, exported through the native C ABI.
//! Native menus validate/commit every choice and native target collectors
//! determine legality. No damage preview, affinity-table read, speculative
//! state mutation, rollback, host RNG or Lua is used here.
//! 1 greedy, 2 cautious, 3 typical, 5 attentive: a careful player who guards
//! while an enemy is visibly charged (public DatUnit +0x14 & 0xC0000), keeps the
//! protagonist above 70% and allies above 50%, revives, explores elements to find
//! weaknesses, remembers elements a target was seen absorbing or nulling, and
//! focuses the last damaged enemy and any enemy seen healing its side.
//! 6 attentive with Persona ratings: attentive, and at the protagonist's command it
//! first changes to the stocked Persona whose rating (a scenario-pinned table, one
//! value per retail element 0..7) is best for a weakness it has seen on a living
//! enemy, else best overall, when that beats the current Persona's.
//! 7 technical: attentive, plus Technical windows. It probes an enemy with each
//! Technical-set ailment at most twice, remembers an ailment it saw land, inflicts
//! a known one on a target not already suffering it, and prefers the element
//! paired with an ailment the target shows (Fire/Rage, Ice/Fear, Elec/Silence,
//! Wind/Panic) unless that element is a known weakness.
//! 8 strategic candidate: technical (all of 7's observed-ailment and Technical
//! reasoning), plus the rules of include/p4pc/battle_strategy.h: a
//! direction-pure buff/debuff (raise on a member, lower on an enemy) not yet held
//! is worth more in a durable fight, never two without a party damage decision
//! between; a fixed heal loses the missing HP it leaves uncovered; and the
//! protagonist changes, once per command, to a stocked Persona whose own listed,
//! exactly priced skills score clearly better for this decision.

const std = @import("std");
const item_bindings = @import("c_bindings");
pub const panic = @import("panic.zig").panic;

// The test adapter imports this implementation without exporting the unrelated
// game entry points, whose external dependencies are unavailable to the test.
comptime {
    if (!@hasDecl(@import("root"), "p4pc_test_policy_adapter")) {
        @export(&p4_battle_policy_begin, .{ .name = "p4_battle_policy_begin" });
        @export(&p4_battle_policy_error, .{ .name = "p4_battle_policy_error" });
        @export(&p4_battle_policy_set_ratings, .{ .name = "p4_battle_policy_set_ratings" });
        @export(&p4_battle_policy_input, .{ .name = "p4_battle_policy_input" });
        @export(&p4_battle_policy_target, .{ .name = "p4_battle_policy_target" });
        @export(&p4_battle_policy_before_update, .{ .name = "p4_battle_policy_before_update" });
    }
}

// ---- Guest memory (include/p4pc/guest.h, include/p4pc/gmem.h) ----

const guest64 = @sizeOf(usize) == 8;
const P4_GUEST_SIZE: u32 = 0x04000000;
const P4_GP_VALUE: u32 = 0x007690F0;

extern var p4_guest_base: [*c]u8;
extern fn p4_g2h_slow(a: u32) ?*anyopaque;
extern fn p4_h2g_slow(p: ?*const anyopaque) u32;
extern fn p4_gport_alloc(size: u32, @"align": u32) u32;

/// P4_G2H: p4_g2h on a 64-bit host, identity on i386.
inline fn g2h(a: u32) ?*anyopaque {
    if (guest64) {
        if (a -% 1 < P4_GUEST_SIZE - 1) return @ptrFromInt(@intFromPtr(p4_guest_base) +% a);
        return p4_g2h_slow(a);
    }
    return @ptrFromInt(a);
}
/// P4_H2G: p4_h2g on a 64-bit host, identity on i386.
inline fn h2g(p: ?*const anyopaque) u32 {
    if (guest64) {
        const d: usize = @intFromPtr(p) -% @intFromPtr(p4_guest_base);
        if (d -% 1 < P4_GUEST_SIZE - 1) return @truncate(d);
        return p4_h2g_slow(p);
    }
    return @truncate(@intFromPtr(p));
}
inline fn hostAddr(a: u32) usize {
    return if (g2h(a)) |p| @intFromPtr(p) else 0;
}
/// P4_G8/P4_G16/P4_G32: volatile guest lvalues (x86 tolerates any alignment, as the C casts do).
inline fn g8(a: u32) u8 {
    return @as(*allowzero align(1) volatile u8, @ptrFromInt(hostAddr(a))).*;
}
inline fn g16(a: u32) u16 {
    return @as(*allowzero align(1) volatile u16, @ptrFromInt(hostAddr(a))).*;
}
inline fn g32(a: u32) u32 {
    return @as(*allowzero align(1) volatile u32, @ptrFromInt(hostAddr(a))).*;
}
inline fn g16Store(a: u32, value: u16) void {
    @as(*allowzero align(1) volatile u16, @ptrFromInt(hostAddr(a))).* = value;
}
/// GP(off) = P4_GPDATA(uint32_t, off): the (non-volatile) gp-relative word.
inline fn gp(comptime off: u16) u32 {
    const addr: u32 = P4_GP_VALUE +% @as(u32, @bitCast(@as(i32, @as(i16, @bitCast(off)))));
    return @as(*allowzero align(1) const u32, @ptrFromInt(hostAddr(addr))).*;
}

// ---- Battle events (include/p4pc/battle_events.h) ----

const P4BattleActor = extern struct {
    action_uid: u64,
    unit_uid: u32,
    native_id: u16,
    side: u8,
    present: u8,
};
const P4BattleAdmission = extern struct {
    actor: P4BattleActor,
    native_action_count: u32,
    next_state: u16,
    queue_kind: u8,
    reserved: u8,
};
const P4BattleCost = opaque {};
const P4BattleRequest = opaque {};
const P4BattleHit = extern struct {
    source: P4BattleActor,
    target: P4BattleActor,
    requested_hp_delta: i32,
    requested_sp_delta: i32,
    requested_money_delta: i32,
    added_status: u32,
    removed_status: u32,
    buff_changes: u32,
    status_before: u32,
    status_after: u32,
    skill_id: u16,
    action_kind: u16,
    hp_before: u16,
    hp_after: u16,
    sp_before: u16,
    sp_after: u16,
    target_flags: u16,
    result_flags: u16,
    effect: u16,
    resources_applied: u8,
    statuses_applied: u8,
};
const P4BattleCommand = extern struct {
    actor: P4BattleActor,
    action_kind: u16,
    skill_id: u16,
    item_id: u16,
    target_count: u16,
    native_state: u16,
    free_action: u8,
    reserved: u8,
};
const P4BattleObserver = extern struct {
    context: ?*anyopaque,
    admission: ?*const fn (*const P4BattleAdmission, ?*anyopaque) callconv(.c) void,
    cost: ?*const fn (*const P4BattleCost, ?*anyopaque) callconv(.c) void,
    hit: ?*const fn (*const P4BattleHit, ?*anyopaque) callconv(.c) void,
    command: ?*const fn (*const P4BattleCommand, ?*anyopaque) callconv(.c) void,
    request: ?*const fn (*const P4BattleRequest, ?*anyopaque) callconv(.c) void,
};
comptime {
    if (@sizeOf(P4BattleActor) != 16) @compileError("P4BattleActor layout");
    if (@sizeOf(P4BattleAdmission) != 24) @compileError("P4BattleAdmission layout");
    if (@offsetOf(P4BattleHit, "skill_id") != 64 or @offsetOf(P4BattleHit, "statuses_applied") != 83 or
        @sizeOf(P4BattleHit) != std.mem.alignForward(usize, 84, @alignOf(P4BattleActor)))
        @compileError("P4BattleHit layout");
    if (@offsetOf(P4BattleCommand, "reserved") != 27 or @sizeOf(P4BattleCommand) != std.mem.alignForward(usize, 28, @alignOf(P4BattleActor)))
        @compileError("P4BattleCommand layout");
}
extern fn p4_battle_observer_add(observer: *const P4BattleObserver) u32;

// ---- Strategy rules (include/p4pc/battle_strategy.h, exported by strategy.zig) ----

const P4_STRATEGY_COMMIT_OTHER: c_uint = 0;
const P4_STRATEGY_COMMIT_DAMAGE: c_uint = 1;
const P4_STRATEGY_COMMIT_SETUP: c_uint = 2;
const P4StrategyEnemy = extern struct {
    tried: u32,
    weak: u32,
    living: u8,
};
const P4StrategyTempo = extern struct {
    setup_pending: u8,
};
extern fn p4_strategy_setup_direction(mask: u32, side: c_uint) c_int;
extern fn p4_strategy_durable(enemies: [*c]const P4StrategyEnemy, count: c_uint) c_int;
extern fn p4_strategy_tempo_commit(tempo: *P4StrategyTempo, kind: c_uint) void;
extern fn p4_strategy_new_encounter(previous: [*c]const u32, previous_count: c_uint, current: [*c]const u32, current_count: c_uint) c_int;

// ---- Other port services ----

extern fn p4_game_table_extent_rows(slot: u32, stride: u32, base: *u32, rows: *u32) c_int;
extern fn p4_battle_sim_update_count() u32;
extern fn p4_battle_sim_command_ready(action: u32) void;
extern fn p4_battle_sim_opening_active() c_int;
extern fn p4_battle_sim_decision_trace() ?*anyopaque;

// ---- Native game functions ----

extern fn btlTargetCheckCommandAvailability(action: ?*anyopaque, kind: i64) i32;
extern fn datCalcThresholdTrigger(unit: ?*anyopaque, skill: i32) i32;
extern fn datCalcSkillCost(unit: ?*anyopaque, skill: i32) u32;
extern fn datCalcSkillElement(unit: ?*anyopaque, skill: u16) i32;
extern fn datCalcDefaultSkillId(unit: ?*anyopaque) i32;
extern fn datCalcUnderlyingSkillId(item: i64) u16;
extern fn datCalcMaxHpById(id: i16) u16;
extern fn datCalcMaxSpById(id: i16) u16;
extern fn datCalcHasBuffMask(unit: ?*anyopaque, mask: i32) i32;
extern fn btlSkillClassifyTargets(action: ?*anyopaque, selection: ?*anyopaque, skill: u16, reverse: u32) u16;
extern fn btlEffectCollectTargetUnits(action: ?*anyopaque, selection: ?*anyopaque, sides: u32, filters: u32, status: u32) void;
extern fn btlFormationEntrySetSelected(selection: i32, action: ?*anyopaque) void;
extern fn p4pc_pad_inject(bits: c_uint, frames: c_uint) void;
extern fn datPersonaRosterSlot(slot: i16) ?*anyopaque;
extern fn datPersonaGetLeadSlot() i32;
extern fn datPersonaCountValidMembers() u16;
extern fn datPersonaGetSkillsByPcId(pc_id: u16) [*c]u16;

// ---- Policy state ----

const WHEEL_ATTACK: u16 = 3;
const WHEEL_SKILL: u16 = 4;
const WHEEL_PERSONA: u16 = 5;
const WHEEL_ITEM: u16 = 6;
const WHEEL_GUARD: u16 = 2;
inline fn attentive(mode: u32) bool {
    return mode == 5 or mode == 6 or mode == 7 or mode == 8 or mode == 9;
}
inline fn technical(mode: u32) bool {
    return mode == 7 or mode == 8 or mode == 9;
}
inline fn strategic(mode: u32) bool {
    return mode == 8 or mode == 9;
}
const TECHNICAL_AILMENTS: u32 = 0x1E; // Rage 0x02, Fear 0x04, Silence 0x08, Panic 0x10
const RATINGS_MAX: u32 = 512;
const INT_MIN: c_int = std.math.minInt(c_int);
const INT64_MIN: i64 = std.math.minInt(i64);
const INT64_MAX: i64 = std.math.maxInt(i64);

const Choice = struct {
    score: i64 = 0,
    target: u32 = 0,
    target_uid: u64 = 0,
    skill: u16 = 0,
    item: u16 = 0,
    wheel: u16 = 0,
    menu_index: u16 = 0,
};
const Knowledge = struct {
    unit_uid: u32,
    tried: u32,
    weak: u32,
    unchanged: u32, // attentive: shown no damage once
    bad: u32, // attentive: absorbed or nulled (public outcome)
    vulnerable: u32, // technical: ailments seen landing
    probed: u32, // technical: attempted once
    probed2: u32, // technical: attempted twice
    charge_current: u32,
    charge_stale: u32,
    charge_before: u32,
    charge_action_uid: u64,
    charge_turn: u8,
    native_id: u16,
    valid: u8,
};
const Policy = struct {
    mode: u32,
    latency: u32,
    scratch: u32,
    skill_base: u32,
    skill_rows: u32,
    observer: u32,
    actor: u32,
    decision_update: u32,
    actor_uid: u64,
    pending: c_int,
    @"error": ?[*:0]const u8,
    choice: Choice,
    knowledge: [64]Knowledge,
    next_knowledge: c_uint,
    /// Attentive (5) only, from observed hits: the enemy the party last damaged,
    /// and enemies seen healing an ally.
    focus_uid: u32,
    healer_uid: [8]u32,
    healers: c_uint,
    /// Strategic (8): the protagonist already changed Persona for this command; this decision's durable-fight fact;
    /// the encounter's setup tempo and the enemy units of the last decision (a new set restarts both).
    switched: c_int,
    durable: c_int,
    tempo: P4StrategyTempo,
    field_uid: [12]u32,
    buff_uid: [11]u32,
    buff_cast: [11]u32,
    field_count: c_uint,
};
pub var policy: Policy = std.mem.zeroes(Policy);
var expert_observation: item_bindings.struct_bsim_observation = undefined;
/// Mode 6's Persona ratings (p4_battle_policy_set_ratings): ascending Persona ids, one signed rating per retail element.
const Ratings = struct {
    id: [RATINGS_MAX]u16,
    value: [RATINGS_MAX][8]i8,
    count: c_uint,
};
pub var ratings: Ratings = std.mem.zeroes(Ratings);

fn action_uid(action: u32) u64 {
    return @as(u64, g32(action)) | (@as(u64, g32(action +% 4)) << 32);
}
fn record_of(action: u32) u32 {
    return g32(g32(action +% 0x30) +% 0xa64);
}
fn is_healing(effect: c_uint) bool {
    return effect == 2 or effect == 5 or effect == 7 or effect == 9 or effect == 11 or effect == 15;
}
fn is_damage(effect: c_uint) bool {
    return effect == 1 or effect == 4 or effect == 8 or effect == 10 or effect == 12 or effect == 13 or effect == 14;
}
fn percent(current: c_uint, maximum: c_uint) c_uint {
    if (maximum == 0) return 0;
    return if (current >= maximum) 100 else current *% 100 / maximum;
}
fn knowledge_slot(uid: u32, id: u16, create: bool) c_uint {
    var i: c_uint = 0;
    while (i < 64) : (i += 1) {
        const known = &policy.knowledge[i];
        if (known.valid != 0 and known.unit_uid == uid and known.native_id == id) return i;
    }
    if (!create) return 64;
    const slot: c_uint = policy.next_knowledge % 64;
    policy.next_knowledge +%= 1;
    const k = &policy.knowledge[slot];
    k.unit_uid = uid;
    k.native_id = id;
    k.tried = 0;
    k.weak = 0;
    k.unchanged = 0;
    k.bad = 0;
    k.vulnerable = 0;
    k.probed = 0;
    k.probed2 = 0;
    k.valid = 1;
    k.charge_current = 0;
    k.charge_stale = 0;
    k.charge_before = 0;
    k.charge_action_uid = 0;
    k.charge_turn = 0;
    return slot;
}
fn observe_hit(event: *const P4BattleHit, context: ?*anyopaque) callconv(.c) void {
    _ = context;
    // These are deliberately the only outcome fields fed into policy memory.
    // Enemy exact HP/SP, hidden stats/affinities, input and RNG stay excluded.
    if (event.skill_id >= policy.skill_rows) return;
    if (policy.mode == 9 and event.source.present != 0 and event.target.present != 0 and event.source.side == 0 and event.target.side == 0 and
        event.target.native_id >= 1 and event.target.native_id <= 10 and event.buff_changes & 0x45 != 0)
    {
        const id = event.target.native_id;
        if (policy.buff_uid[id] != event.target.unit_uid) {
            policy.buff_uid[id] = event.target.unit_uid;
            policy.buff_cast[id] = 0;
        }
        policy.buff_cast[id] |= event.buff_changes & 0x45;
    }
    const row: u32 = policy.skill_base +% @as(u32, event.skill_id) *% 0x28;
    if (event.source.side == 1 and event.target.side == 1) {
        if (!attentive(policy.mode) or !is_healing(g8(row +% 0x11))) return;
        var i: c_uint = 0;
        while (i < policy.healers) : (i += 1) {
            if (policy.healer_uid[i] == event.source.unit_uid) return;
        }
        if (policy.healers < 8) {
            policy.healer_uid[policy.healers] = event.source.unit_uid;
            policy.healers += 1;
        }
        return;
    }
    if (event.source.side != 0 or event.target.side != 1) return;
    if (technical(policy.mode) and g8(row +% 0x18) == 1 and (g32(row +% 0x1c) & TECHNICAL_AILMENTS) != 0) {
        // The ailment icon a hit leaves is public. Attempts are counted where the
        // policy commits to them (choose), since an immune target may show no hit.
        const slot = knowledge_slot(event.target.unit_uid, event.target.native_id, true);
        policy.knowledge[slot].vulnerable |= event.added_status & g32(row +% 0x1c) & TECHNICAL_AILMENTS;
    }
    if (!is_damage(g8(row +% 0x11))) return; // KO channels are not damage knowledge.
    policy.focus_uid = event.target.unit_uid;
    const battle = gp(0xb3ac);
    if (battle == 0) return;
    var action = g32(battle +% 0x174);
    var n: c_uint = 0;
    while (action != 0 and n < 12) : ({
        n += 1;
        action = g32(action +% 0x450);
    }) {
        if (action_uid(action) != event.source.action_uid) continue;
        const element = datCalcSkillElement(g2h(record_of(action)), event.skill_id);
        if (element < 0 or element >= 32) return;
        const slot = knowledge_slot(event.target.unit_uid, event.target.native_id, true);
        const k = &policy.knowledge[slot];
        const bit = @as(u32, 1) << @as(u5, @intCast(element));
        k.tried |= bit; // Attempted is not an affinity inference.
        if ((event.target_flags & 4) != 0) k.weak |= bit; // Native WEAK, not CRITICAL.
        // What the screen shows: the target healed (absorb), or twice took no
        // damage (null; once could be a miss).
        if (event.hp_after > event.hp_before) {
            k.bad |= bit;
        } else if (event.hp_after == event.hp_before) {
            if ((k.unchanged & bit) != 0) k.bad |= bit;
            k.unchanged |= bit;
        }
        return;
    }
}
// Public charge bits become stale only after their actor's action reaches END.
fn observe_charges() void {
    const battle = gp(0xb3ac);
    var action: u32 = if (battle != 0) g32(battle +% 0x174) else 0;
    var n: u32 = 0;
    while (action != 0 and n < 12) : (n += 1) {
        defer action = g32(action +% 0x450);
        const unit = g32(action +% 0x30);
        if (unit == 0 or g8(unit +% 0xa2) != 1) continue;
        const record = g32(unit +% 0xa64);
        if (record == 0) continue;
        const slot = knowledge_slot(g32(unit +% 0xa8), g16(unit +% 0xa4), false);
        if (slot == 64) continue;
        const k = &policy.knowledge[slot];
        const current = g32(record +% 0x14) & 0xc0000;
        if (k.charge_action_uid != action_uid(action)) {
            k.charge_action_uid = action_uid(action);
            k.charge_stale = 0;
            k.charge_before = 0;
            k.charge_turn = 0;
        }
        k.charge_stale &= current;
        k.charge_before &= current;
        k.charge_current = current;
        if (k.charge_turn != 0 and g16(action +% 0x0c) == 0x21) {
            k.charge_stale |= k.charge_before;
            k.charge_turn = 0;
        }
    }
}

fn observe_admission(event: *const P4BattleAdmission, context: ?*anyopaque) callconv(.c) void {
    _ = context;
    if (policy.mode == 9 and event.actor.present != 0 and event.actor.side == 1) {
        const slot = knowledge_slot(event.actor.unit_uid, event.actor.native_id, true);
        observe_charges();
        const k = &policy.knowledge[slot];
        if (k.charge_action_uid == event.actor.action_uid) {
            k.charge_before = k.charge_current;
            k.charge_turn = 1;
        }
    }
    if (policy.pending != 0 and event.actor.action_uid == policy.actor_uid) policy.pending = 0;
}
fn observe_command(event: *const P4BattleCommand, context: ?*anyopaque) callconv(.c) void {
    _ = context;
    if (policy.pending != 0 and event.actor.action_uid == policy.actor_uid) policy.pending = 0;
}

fn p4_battle_policy_begin(mode: u32, latency: u32) callconv(.c) c_int {
    policy.mode = mode;
    policy.latency = latency;
    if (mode == 0) return 1;
    if (mode > 3 and !attentive(mode)) {
        policy.@"error" = "unsupported_native_party_policy";
        return 0;
    }
    if (mode == 6 and ratings.count == 0) {
        policy.@"error" = "native_policy_persona_ratings_missing";
        return 0;
    }
    if (p4_game_table_extent_rows(0x007644a8, 0x28, &policy.skill_base, &policy.skill_rows) == 0) {
        policy.@"error" = "native_policy_skill_catalog_unavailable";
        return 0;
    }
    policy.scratch = p4_gport_alloc(0x40, 16); // Full native BtlTargetSelection, not obsolete0x38.
    if (policy.scratch == 0) {
        policy.@"error" = "native_policy_target_scratch";
        return 0;
    }
    const observer: P4BattleObserver = .{
        .context = null,
        .admission = &observe_admission,
        .cost = null,
        .hit = &observe_hit,
        .command = &observe_command,
        .request = null,
    };
    policy.observer = p4_battle_observer_add(&observer);
    if (policy.observer == 0) {
        policy.@"error" = "native_policy_observer_registration";
        return 0;
    }
    return 1;
}
fn p4_battle_policy_error() callconv(.c) ?[*:0]const u8 {
    return policy.@"error";
}

/// Technical (7): the ailment and Technical-element share of a skill on an enemy.
/// The target's ailment word is what its status icons show.
fn technical_utility(target: u32, unit: u32, id: u16, row: u32, element: i32) i64 {
    if (!technical(policy.mode)) return 0;
    const paired = [5]u32{ 0, 0x02, 0x04, 0x08, 0x10 }; // Fire Rage, Ice Fear, Elec Silence, Wind Panic
    const status = g32(record_of(target) +% 0xC);
    const slot = knowledge_slot(g32(unit +% 0xa8), id, false);
    var score: i64 = 0;
    const mask: u32 = if (g8(row +% 0x18) == 1) g32(row +% 0x1c) & TECHNICAL_AILMENTS else 0;
    if (mask != 0 and (status & mask) == 0) {
        if (slot < 64 and (policy.knowledge[slot].vulnerable & mask) != 0) {
            score += 2600;
        } else if (slot == 64 or (policy.knowledge[slot].probed2 & mask) == 0) {
            score += 500;
        }
    }
    if (is_damage(g8(row +% 0x11)) and element >= 1 and element <= 4 and (status & paired[@intCast(element)]) != 0 and
        (slot == 64 or (policy.knowledge[slot].weak & (@as(u32, 1) << @as(u5, @intCast(element)))) == 0))
        score += 1500;
    return score;
}

fn target_utility(target: u32, row: u32, element: i32) i64 {
    const unit = g32(target +% 0x30);
    const side = g8(unit +% 0xa2);
    const id = g16(unit +% 0xa4);
    const hp_effect: c_uint = g8(row +% 0x11);
    const sp_effect: c_uint = g8(row +% 0x14);
    const power: i32 = @as(i16, @bitCast(g16(row +% 0x12)));
    const power_u: u32 = if (power > 0) @intCast(power) else 0;
    if (side == 1) {
        if (is_damage(hp_effect)) {
            var score: i64 = 200 + @as(i64, power_u);
            const slot = knowledge_slot(g32(unit +% 0xa8), id, false);
            if (element >= 0 and element < 32) {
                const bit = @as(u32, 1) << @as(u5, @intCast(element));
                if (slot < 64 and (policy.knowledge[slot].weak & bit) != 0) {
                    score += 3000;
                }
                // A target seen absorbing or nulling the element is worth nothing:
                // it cancels its own share of a spread instead of vetoing it.
                else if (attentive(policy.mode) and slot < 64 and (policy.knowledge[slot].bad & bit) != 0) {
                    return -score;
                } else if (slot == 64 or (policy.knowledge[slot].tried & bit) == 0) {
                    score += if (policy.mode == 1 or attentive(policy.mode)) 160 else 60;
                }
            }
            if (attentive(policy.mode)) {
                const uid = g32(unit +% 0xa8);
                if (uid == policy.focus_uid) score += 400;
                var i: c_uint = 0;
                while (i < policy.healers) : (i += 1) {
                    if (policy.healer_uid[i] == uid) score += 700;
                }
            }
            return score + technical_utility(target, unit, id, row, element);
        }
        // Current buff levels/icons are public. This reads no base defense.
        const mask = g32(row +% 0x20);
        const tech = technical_utility(target, unit, id, row, element);
        if (mask != 0 and datCalcHasBuffMask(g2h(record_of(target)), @bitCast(mask)) == 0) {
            return @as(i64, if (policy.mode == 1) 80 else 240) + tech;
        }
        return tech;
    }
    if (side != 0 or id < 1 or id > 10) return 0;
    if (is_damage(hp_effect)) return -20000; // Do not voluntarily damage an ally.
    const record = record_of(target);
    const hp: c_uint = g16(record +% 8);
    const sp: c_uint = g16(record +% 10);
    const hp_percent = percent(hp, datCalcMaxHpById(@bitCast(id)));
    const sp_percent = percent(sp, datCalcMaxSpById(@bitCast(id)));
    const status = g32(record +% 12);
    const mask = g32(row +% 0x1c);
    const removes_status = g8(row +% 0x18) == 2;
    if (hp == 0 and removes_status and (mask & 0x80000) != 0) return if (policy.mode == 1) 9000 else 30000;
    const heal_threshold: c_uint = if (policy.mode == 1) 20 else if (policy.mode == 2) (if (id == 1) 60 else 45) else if (attentive(policy.mode)) (if (id == 1) 70 else 50) else (if (id == 1) 45 else 35);
    if (hp != 0 and is_healing(hp_effect) and hp_percent <= heal_threshold) {
        // A small fixed heal (a quarter of the ally's HP or less) is not worth
        // a turn unless the ally is critical.
        if (attentive(policy.mode) and hp_effect == 5 and hp_percent > 25 and
            power_u *% 4 <= datCalcMaxHpById(@bitCast(id))) return 300;
        // C: int + unsigned arithmetic, then widened to int64_t.
        const base: c_uint = if (policy.mode == 1) 7000 else 24000;
        const bonus: c_uint = if (id == 1) 500 else 0;
        return base +% (100 -% hp_percent) *% 20 +% bonus;
    }
    if (removes_status and (status & mask) != 0) return if (policy.mode == 1) 100 else 14000;
    if (hp != 0 and is_healing(sp_effect) and sp_percent < @as(c_uint, if (policy.mode == 2 or attentive(policy.mode)) 35 else 20))
        return 9000 +% (100 -% sp_percent) *% 10;
    const buff = g32(row +% 0x20);
    if (hp != 0 and buff != 0 and datCalcHasBuffMask(g2h(record), @bitCast(buff)) == 0) {
        return if (policy.mode == 1) 60 else 260;
    }
    return 0;
}
fn consider(actor: u32, skill: u16, item: u16, wheel: u16, menu_index: u16) void {
    if (skill >= policy.skill_rows) {
        policy.@"error" = "native_policy_skill_outside_loaded_catalog";
        return;
    }
    const row: u32 = policy.skill_base +% @as(u32, skill) *% 0x28;
    const record = record_of(actor);
    const classification: u16 = g8(row +% 8);
    const group = classification == 1 or classification == 2;
    if (group) {
        btlEffectCollectTargetUnits(g2h(actor), g2h(policy.scratch), g8(row +% 9), g8(row +% 10), 0);
    } else {
        _ = btlSkillClassifyTargets(g2h(actor), g2h(policy.scratch), skill, 0);
    }
    const count: c_uint = g16(policy.scratch +% 0x38);
    if (count > 12) {
        policy.@"error" = "native_policy_target_count_outside_engine_capacity";
        return;
    }
    if (count == 0) return;
    const element = datCalcSkillElement(g2h(record), skill);
    const cost: u32 = if (wheel == WHEEL_SKILL) datCalcSkillCost(g2h(record), skill) else 0;
    const cost_kind: c_uint = g8(row +% 3);
    const unit = g32(actor +% 0x30);
    const actor_id = g16(unit +% 0xa4);
    const sp_percent = percent(g16(record +% 10), datCalcMaxSpById(@bitCast(actor_id)));
    var penalty: i64 = if (cost_kind == 1)
        @as(i64, cost) * @as(i64, if (policy.mode == 1) 1 else 4)
    else
        @as(i64, cost) * @as(i64, if (policy.mode == 1) 2 else 5);
    if ((policy.mode == 3 or attentive(policy.mode)) and cost_kind == 2 and sp_percent < 30) penalty += @as(i64, cost) * 60;
    const choices: c_uint = if (group) 1 else count;
    var i: c_uint = 0;
    while (i < choices) : (i += 1) {
        const target = g32(policy.scratch +% i *% 4);
        var score: i64 = -penalty;
        if (group) {
            var j: c_uint = 0;
            while (j < count) : (j += 1) score += target_utility(g32(policy.scratch +% j *% 4), row, element);
        } else score += target_utility(target, row, element);
        const uid = action_uid(target);
        const c = &policy.choice;
        if (score > c.score or (score == c.score and
            (skill < c.skill or (skill == c.skill and
            (item < c.item or (item == c.item and uid < c.target_uid))))))
        {
            policy.choice = .{ .score = score, .target = target, .target_uid = uid, .skill = skill, .item = item, .wheel = wheel, .menu_index = menu_index };
        }
    }
}
/// Any living enemy showing the public charge state (Power/Mind Charge).
fn enemy_charged() bool {
    const battle = gp(0xb3ac);
    if (battle == 0) return false;
    var action = g32(battle +% 0x174);
    var n: c_uint = 0;
    while (action != 0 and n < 12) : ({
        n += 1;
        action = g32(action +% 0x450);
    }) {
        const unit = g32(action +% 0x30);
        if (unit == 0 or g8(unit +% 0xa2) != 1) continue;
        const record = g32(unit +% 0xa64);
        if (record != 0 and (g32(record +% 12) & 0x80000) == 0 and (g32(record +% 0x14) & 0xC0000) != 0) return true;
    }
    return false;
}
pub fn p4_battle_policy_set_ratings(data: [*c]const u8, size: usize) callconv(.c) c_int {
    ratings.count = 0;
    if (data == null or size < 12 or !std.mem.eql(u8, data[0..8], "P4AFFRT1")) return 0;
    const count: u32 = @as(u32, data[8]) | (@as(u32, data[9]) << 8) | (@as(u32, data[10]) << 16) | (@as(u32, data[11]) << 24);
    if (count == 0 or count > RATINGS_MAX or size != 12 + @as(usize, count) * 10) return 0;
    var i: u32 = 0;
    while (i < count) : (i += 1) {
        const row = data + 12 + i * 10;
        ratings.id[i] = @as(u16, row[0]) | (@as(u16, row[1]) << 8);
        if (i != 0 and ratings.id[i] <= ratings.id[i - 1]) return 0;
        for (0..8) |element| ratings.value[i][element] = @bitCast(row[2 + element]);
    }
    ratings.count = count;
    return 1;
}
/// A stocked Persona's value: its best (signed) rating among the elements in `weak` (seen WEAK on a living enemy),
/// else its best rating overall; INT_MIN when the table does not name it, so an unlisted Persona never wins.
fn persona_value(id: u16, weak: u32) c_int {
    var low: c_uint = 0;
    var high: c_uint = ratings.count;
    while (low < high) {
        const mid = (low + high) / 2;
        if (ratings.id[mid] < id) low = mid + 1 else high = mid;
    }
    if (low == ratings.count or ratings.id[low] != id) return INT_MIN;
    var best: c_int = INT_MIN;
    var element: u5 = 0;
    while (element < 8) : (element += 1) {
        if ((weak == 0 or (weak & (@as(u32, 1) << element)) != 0) and ratings.value[low][element] > best) best = ratings.value[low][element];
    }
    return best;
}
fn roster_id(slot: i16) u16 {
    const record = datPersonaRosterSlot(slot);
    return if (record != null) g16(h2g(record) +% 2) else 0;
}
/// Mode 6: the protagonist first changes Persona, through the native wheel and swap list, when a stocked Persona's
/// value is strictly higher than the current one's; ties keep the current Persona, then the lowest row. Weakness
/// knowledge covers the retail elements 0..7 only (an expanded leaf is ignored here).
pub fn choose_persona(actor: u32) void {
    const unit = g32(actor +% 0x30);
    if (unit == 0 or g16(unit +% 0xa4) != 1 or btlTargetCheckCommandAvailability(g2h(actor), 5) == 0) return;
    var weak: u32 = 0;
    const battle = gp(0xb3ac);
    var action: u32 = if (battle != 0) g32(battle +% 0x174) else 0;
    var n: c_uint = 0;
    while (action != 0 and n < 12) : ({
        n += 1;
        action = g32(action +% 0x450);
    }) {
        const enemy = g32(action +% 0x30);
        if (enemy == 0 or g8(enemy +% 0xa2) != 1) continue;
        const record = g32(enemy +% 0xa64);
        if (record == 0 or (g32(record +% 12) & 0x80000) != 0) continue;
        const slot = knowledge_slot(g32(enemy +% 0xa8), g16(enemy +% 0xa4), false);
        if (slot < 64) weak |= policy.knowledge[slot].weak & 0xff;
    }
    const lead = datPersonaGetLeadSlot();
    const count: c_uint = datPersonaCountValidMembers();
    var best_value = persona_value(roster_id(@truncate(lead)), weak);
    var best_row: c_int = -1;
    var row: c_uint = 0;
    while (row < count) : (row += 1) {
        if (@as(i32, @bitCast(row)) == lead) continue;
        const id = roster_id(@truncate(@as(i32, @bitCast(row))));
        const value = if (id != 0) persona_value(id, weak) else INT_MIN;
        if (value > best_value) {
            best_value = value;
            best_row = @bitCast(row);
        }
    }
    if (best_row >= 0) policy.choice = .{ .score = INT64_MAX, .wheel = WHEEL_PERSONA, .menu_index = @truncate(@as(u32, @bitCast(best_row))) };
}
/// Strategic (8): a committed non-Persona decision ends the protagonist's command and moves the setup tempo.
fn strategic_commit(actor: u32) void {
    if (policy.choice.wheel == WHEEL_PERSONA) return;
    const unit = g32(actor +% 0x30);
    if (unit != 0 and g16(unit +% 0xa4) == 1) policy.switched = 0;
    var kind: c_uint = P4_STRATEGY_COMMIT_OTHER;
    if (policy.choice.wheel == WHEEL_ATTACK) {
        kind = P4_STRATEGY_COMMIT_DAMAGE;
    } else if ((policy.choice.wheel == WHEEL_SKILL or policy.choice.wheel == WHEEL_ITEM) and policy.choice.target != 0 and
        policy.choice.skill < policy.skill_rows)
    {
        const row: u32 = policy.skill_base +% @as(u32, policy.choice.skill) *% 0x28;
        const target = g32(policy.choice.target +% 0x30);
        if (is_damage(g8(row +% 0x11))) {
            kind = P4_STRATEGY_COMMIT_DAMAGE;
        } else if (target != 0 and p4_strategy_setup_direction(g32(row +% 0x20), g8(target +% 0xa2)) != 0) {
            kind = P4_STRATEGY_COMMIT_SETUP;
        }
    }
    p4_strategy_tempo_commit(&policy.tempo, kind);
}

fn snapshot_choice(panel: u32, actor: u32, expert: c_int) void {
    var known: [64]item_bindings.struct_bsim_knowledge = undefined;
    for (&known, policy.knowledge) |*out, source| {
        out.* = .{ .unit_uid = source.unit_uid, .tried = source.tried, .weak = source.weak,
            .bad = source.bad, .vulnerable = source.vulnerable, .probed = source.probed,
            .probed2 = source.probed2, .native_id = source.native_id, .valid = source.valid };
    }
    if (item_bindings.bsim_observe(&expert_observation, panel, actor, policy.scratch,
        policy.skill_base, policy.skill_rows, &known, 64) == 0)
    {
        policy.@"error" = "native_expert_observation_failed";
        return;
    }
    if (expert != 0) {
        observe_charges();
        for (0..expert_observation.unit_count) |i| {
            const unit = &expert_observation.units[i];
            if (unit.side != 1 or unit.charged == 0) continue;
            const slot = knowledge_slot(unit.unit_uid, unit.native_id, false);
            if (slot < 64) {
                const k = &policy.knowledge[slot];
                unit.charged = @intFromBool(k.charge_current & ~k.charge_stale != 0);
            }
        }
    }
    var field: [12]u32 = undefined;
    var enemies: [12]P4StrategyEnemy = undefined;
    var count: c_uint = 0;
    for (0..expert_observation.unit_count) |i| {
        const unit = &expert_observation.units[i];
        if (unit.side != 1) continue;
        field[count] = unit.unit_uid;
        enemies[count] = .{ .tried = unit.tried, .weak = unit.weak,
            .living = @intFromBool(unit.status & 0x80000 == 0) };
        count += 1;
    }
    if (p4_strategy_new_encounter(&policy.field_uid, policy.field_count, &field, count) != 0) {
        policy.tempo = .{ .setup_pending = 0 };
        policy.switched = 0;
    }
    if (count != 0) {
        @memcpy(policy.field_uid[0..count], field[0..count]);
        policy.field_count = count;
    }
    policy.durable = p4_strategy_durable(&enemies, count);
    expert_observation.focus_uid = policy.focus_uid;
    for (0..expert_observation.unit_count) |i| {
        const unit = &expert_observation.units[i];
        const id = unit.native_id;
        if (unit.side != 0 or id < 1 or id > 10 or policy.buff_uid[id] != unit.unit_uid) continue;
        const active: u32 = (if (unit.buffs[0] > 0) @as(u32, 1) else 0) |
            (if (unit.buffs[1] > 0) @as(u32, 4) else 0) |
            (if (unit.buffs[3] > 0) @as(u32, 0x40) else 0);
        unit.expired_buffs = policy.buff_cast[id] & ~active;
    }
    expert_observation.healer_count = @intCast(policy.healers);
    expert_observation.healer_uid = policy.healer_uid;
    expert_observation.durable = @intCast(policy.durable);
    expert_observation.setup_pending = policy.tempo.setup_pending;
    expert_observation.switched = @intCast(policy.switched);
    var selected: item_bindings.struct_bsim_choice = undefined;
    if (item_bindings.bsim_choose(&expert_observation, expert, &selected) == 0) {
        policy.@"error" = "native_policy_has_no_legal_command";
        return;
    }
    if (p4_battle_sim_decision_trace()) |trace|
        item_bindings.bsim_trace_decision(trace, &expert_observation, &selected, policy.mode, p4_battle_sim_update_count());
    const candidate = expert_observation.candidates[selected.candidate];
    if (candidate.persona_slot != 255) {
        policy.switched = 1;
        policy.choice = .{ .score = INT64_MAX, .wheel = WHEEL_PERSONA, .menu_index = candidate.persona_slot };
        return;
    }
    const uid = expert_observation.units[selected.target].action_uid;
    var target: u32 = 0;
    if (candidate.wheel != WHEEL_GUARD) {
        const battle = gp(0xb3ac);
        target = if (battle != 0) g32(battle +% 0x174) else 0;
        var n: u32 = 0;
        while (target != 0 and action_uid(target) != uid and n < 12) : (n += 1) target = g32(target +% 0x450);
        if (target == 0 or action_uid(target) != uid) {
            policy.@"error" = "native_expert_target_invalidated";
            return;
        }
    }
    policy.choice = .{ .score = selected.score, .target = target, .target_uid = uid,
        .skill = candidate.skill, .item = candidate.item, .wheel = candidate.wheel, .menu_index = candidate.menu_index };
}

fn finish_choice(actor: u32) void {
    if (policy.choice.score == INT64_MIN) {
        policy.@"error" = "native_policy_has_no_legal_command";
    } else if (technical(policy.mode) and policy.choice.target != 0 and policy.choice.skill < policy.skill_rows) {
        const row: u32 = policy.skill_base +% @as(u32, policy.choice.skill) *% 0x28;
        const mask: u32 = if (g8(row +% 0x18) == 1) g32(row +% 0x1c) & TECHNICAL_AILMENTS else 0;
        if (mask != 0) {
            const unit = g32(policy.choice.target +% 0x30);
            const slot = knowledge_slot(g32(unit +% 0xa8), g16(unit +% 0xa4), true);
            if ((policy.knowledge[slot].probed & mask) != 0) policy.knowledge[slot].probed2 |= mask;
            policy.knowledge[slot].probed |= mask;
        }
    }
    if (strategic(policy.mode) and policy.@"error" == null and policy.choice.score != INT64_MIN) strategic_commit(actor);
}
fn choose(panel: u32, actor: u32) void {
    policy.choice = .{ .score = INT64_MIN };
    if (policy.mode == 8 or policy.mode == 9) {
        snapshot_choice(panel, actor, @intFromBool(policy.mode == 9));
        finish_choice(actor);
        return;
    }
    if (policy.mode == 6) {
        choose_persona(actor);
        if (policy.choice.wheel == WHEEL_PERSONA) return;
    }
    const record = record_of(actor);
    if (btlTargetCheckCommandAvailability(g2h(actor), 1) != 0)
        consider(actor, @truncate(@as(u32, @bitCast(datCalcDefaultSkillId(g2h(record))))), 0, WHEEL_ATTACK, 0);
    if (btlTargetCheckCommandAvailability(g2h(actor), 2) != 0) {
        const count: c_uint = g16(panel +% 0x1a4);
        if (count > 8) {
            policy.@"error" = "native_policy_skill_menu_capacity";
            return;
        }
        var i: c_uint = 0;
        while (i < count) : (i += 1) {
            const skill = g16(panel +% 0x194 +% i * 2);
            if (datCalcThresholdTrigger(g2h(record), skill) == 0) consider(actor, skill, 0, WHEEL_SKILL, @truncate(i));
        }
    }
    if (btlTargetCheckCommandAvailability(g2h(actor), 3) != 0) {
        const count: c_uint = g16(panel +% 0x5a6);
        const items = item_bindings.p4_battle_policy_item_rows(@ptrCast(g2h(panel)), count);
        if (items == null) {
            policy.@"error" = "native_policy_item_menu_capacity";
            return;
        }
        var i: c_uint = 0;
        while (i < count) : (i += 1) {
            const item: u16 = @bitCast(items[i].id);
            const skill = datCalcUnderlyingSkillId(item);
            consider(actor, skill, item, WHEEL_ITEM, @truncate(i));
        }
    }
    if (btlTargetCheckCommandAvailability(g2h(actor), 7) != 0) {
        const unit = g32(actor +% 0x30);
        const id = g16(unit +% 0xa4);
        const hp = percent(g16(record +% 8), datCalcMaxHpById(@bitCast(id)));
        var score: i64 = if (policy.mode != 1 and hp < 20) 450 else 10;
        // A visible charge is the boss's warning: below reviving and healing a
        // low ally, above any attack.
        if (attentive(policy.mode) and enemy_charged()) score = 20000;
        if (score > policy.choice.score) policy.choice = .{ .score = score, .wheel = WHEEL_GUARD };
    }
    finish_choice(actor);
}
fn select_row(selection: u32, index: c_uint) void {
    const count: c_uint = g16(selection +% 8);
    const page: c_uint = g16(selection +% 6);
    if (index >= count) {
        policy.@"error" = "native_policy_menu_choice_invalidated";
        return;
    }
    var first: c_uint = if (index >= page and page != 0) index - page + 1 else 0;
    const maximum_first: c_uint = if (count > page) count - page else 0;
    if (first > maximum_first) first = maximum_first;
    g16Store(selection +% 2, @truncate(first));
    g16Store(selection +% 4, @truncate(index));
}
fn p4_battle_policy_input(selection: u32) callconv(.c) c_int {
    const battle = gp(0xb3ac);
    if (battle == 0) return -1;
    const task = g32(battle +% 0xdd4);
    if (task == 0) return -1;
    const panel = g32(task +% 0x38) +% 0x9c;
    const actor = g32(panel +% 0x178);
    if (actor == 0) return -1;
    if (p4_battle_sim_opening_active() != 0) {
        if (selection == panel +% 0x40) p4_battle_sim_command_ready(actor);
        return -1;
    }
    if (selection == panel +% 0x40) {
        p4_battle_sim_command_ready(actor);
        if (policy.mode == 0) return -1;
        if (policy.@"error" != null) return 0;
        const uid = action_uid(actor);
        if (policy.pending == 0 or policy.actor_uid != uid) {
            policy.actor = actor;
            policy.actor_uid = uid;
            policy.decision_update = p4_battle_sim_update_count();
            policy.pending = 1;
            choose(panel, actor);
        }
        if (policy.@"error" != null or p4_battle_sim_update_count() -% policy.decision_update < policy.latency) return 0;
        select_row(selection, policy.choice.wheel);
        return if (policy.@"error" != null) 0 else 2;
    }
    if (policy.mode == 0) return -1;
    if (selection != panel +% 0x74 and selection != panel +% 0xa8 and selection != panel +% 0x144) return -1;
    if (policy.pending == 0 or policy.actor != actor) {
        policy.@"error" = "native_policy_unowned_submenu";
        return 0;
    }
    if ((selection == panel +% 0x74 and policy.choice.wheel == WHEEL_SKILL) or
        (selection == panel +% 0xa8 and policy.choice.wheel == WHEEL_ITEM))
    {
        select_row(selection, policy.choice.menu_index);
        return if (policy.@"error" != null) 0 else 2;
    }
    if (selection == panel +% 0x144 and policy.choice.wheel == WHEEL_PERSONA) {
        // The native swap handler commits the change (unk_6c 5) and reopens this actor's command panel; the next
        // decision is a new one, against the changed Persona's skills.
        select_row(selection, policy.choice.menu_index);
        if (policy.@"error" != null) return 0;
        policy.pending = 0;
        return 2;
    }
    policy.@"error" = "native_policy_unexpected_submenu";
    return 0;
}
fn p4_battle_policy_target(actor: u32) callconv(.c) void {
    if (policy.mode == 0 or policy.pending == 0 or policy.@"error" != null or actor != policy.actor or policy.choice.target == 0) return;
    const selection = actor +% 0x98;
    const count: c_uint = g16(selection +% 0x38);
    if (count > 12) {
        policy.@"error" = "native_policy_target_capacity";
        return;
    }
    var i: c_uint = 0;
    while (i < count) : (i += 1) {
        if (g32(selection +% i * 4) != policy.choice.target) continue;
        btlFormationEntrySetSelected(@bitCast(selection), g2h(policy.choice.target));
        return;
    }
    policy.@"error" = "native_policy_target_invalidated";
}
fn p4_battle_policy_before_update() callconv(.c) void {
    if (policy.mode == 9) observe_charges();
    // Native menu handlers consume policy input codes; normal packets still
    // own tutorials, Help, target confirmation, All-Out and result prompts.
    if (policy.mode != 0 and p4_battle_sim_update_count() % 24 == 0) p4pc_pad_inject(0x4000, 1);
}
