//! Native battle tool: no alternate damage, AI, RNG or outcome implementation.
//! Private session state, C ABI, trace/result wire formats and lifecycle.
const std = @import("std");
const builtin = @import("builtin");
pub const panic = @import("panic.zig").panic;

const c = @import("c_bindings");

// ---- libc entry points whose header forms vary by C library (variadic, noreturn, stderr object) ----
extern fn fprintf(stream: ?*c.FILE, format: [*c]const u8, ...) c_int;
extern fn snprintf(buffer: [*c]u8, count: usize, format: [*c]const u8, ...) c_int;
extern fn exit(code: c_int) noreturn;
extern fn _putenv_s(name: [*c]const u8, value: [*c]const u8) c_int;
extern fn setenv(name: [*c]const u8, value: [*c]const u8, overwrite: c_int) c_int;
extern fn __acrt_iob_func(index: c_uint) ?*c.FILE;
extern var __stderrp: ?*c.FILE;
extern var stderr: ?*c.FILE;

fn cStderr() ?*c.FILE {
    if (builtin.os.tag == .windows) return __acrt_iob_func(2);
    if (comptime builtin.os.tag.isDarwin()) return __stderrp;
    return stderr;
}

// p4pc/crt.h p4pc_exit (static inline there).
extern fn p4pc_notify_normal_exit() void;
fn p4pc_exit(code: c_int) noreturn {
    if (code == 0) p4pc_notify_normal_exit();
    exit(code);
}

// ---- native game functions ----
extern fn kwlnTaskGetTaskByName(name: ?*const anyopaque) ?*anyopaque;
extern fn kwlnTaskDestroyWithHierarchy(task: ?*anyopaque) u8;
extern fn datSaveRestoreImage(version: i32, image: ?*anyopaque, size: i32) void;
extern fn datSaveVerifyChecksum(version: i32, image: ?*anyopaque, size: i32) i32;
extern fn datInitDefaultGameState() void;
extern fn datCalcMaxHpById(id: i16) u16;
extern fn datCalcMaxSpById(id: i16) u16;
extern fn datSetUnitHp(id: i16, hp: i16) void;
extern fn datSetUnitSp(id: i16, sp: i16) void;
extern fn datSetUnitAiTactic(id: i16, tactic: i16) void;
extern fn datGetPartyId(slot: i32) i16;
extern fn datUnitGetRecord(id: i16) ?*anyopaque;
extern fn datCalcGetHp(unit: ?*anyopaque) u16;
extern fn datCalcGetSp(unit: ?*anyopaque) u16;
extern fn datCalcGetBadStatus(unit: ?*anyopaque) u32;
extern fn datCalcIsDead(unit: ?*anyopaque, hp_delta: i32) u32;
extern fn RpRandomSeed(seed: i32, unused: ?*anyopaque) void;
extern fn RpRandomSeedMT(seed: i32) void;
extern fn effMiscRandInit(state: ?*anyopaque, seed: u32) void;
extern var p4pc_rng_draws: u32;
extern fn calendarSyncFromDate() void;
extern fn calendarSetMode(mode: i32) void;
extern fn datGetDateIndex() i16;
extern fn datGetPendingDateIndex() i16;
extern fn datGetPendingTimeSlot() u8;
extern fn FldStepIndex() i32;
extern fn sequenceRequest(sequence: i32, parameters: u32, size: i32, delay: i32) void;
extern fn datSaveBuildImage(version: i32, size: ?*i32, compact: i32) ?*anyopaque;

// Guest conversion follows guest.h, including the slow path for null and sentinels.
extern var p4_guest_base: [*c]u8;
extern fn p4_g2h_slow(a: u32) ?*anyopaque;
extern fn p4_h2g_slow(p: ?*const anyopaque) u32;
inline fn g2h(a: u32) *anyopaque {
    if (@sizeOf(usize) == 4) return @ptrFromInt(a);
    if (a -% 1 < 0x03ffffff) return @ptrFromInt(@intFromPtr(p4_guest_base) +% a);
    return p4_g2h_slow(a).?;
}
inline fn h2g(p: ?*const anyopaque) u32 {
    if (@sizeOf(usize) == 4) return @truncate(@intFromPtr(p));
    const d = @intFromPtr(p) -% @intFromPtr(p4_guest_base);
    if (d -% 1 < 0x03ffffff) return @truncate(d);
    return p4_h2g_slow(p);
}
inline fn g8p(a: u32) *volatile u8 {
    return @ptrCast(g2h(a));
}
inline fn g16p(a: u32) *align(1) volatile u16 {
    return @ptrCast(g2h(a));
}
inline fn g32p(a: u32) *align(1) volatile u32 {
    return @ptrCast(g2h(a));
}
inline fn g8(a: u32) u8 {
    return g8p(a).*;
}
inline fn g16(a: u32) u16 {
    return g16p(a).*;
}
inline fn g32(a: u32) u32 {
    return g32p(a).*;
}
inline fn gbytes(a: u32) [*]u8 {
    return @ptrCast(g2h(a));
}
const P4_GP_VALUE: u32 = 0x007690F0;
/// P4_GPDATA(uint32_t, off): gp plus the sign-extended 16-bit offset (non-volatile object).
inline fn gpp(off: u16) *align(1) u32 {
    const address = P4_GP_VALUE +% @as(u32, @bitCast(@as(i32, @as(i16, @bitCast(off)))));
    return @ptrCast(g2h(address));
}
inline fn gp(off: u16) u32 {
    return gpp(off).*;
}

const MAX_ENTRY: u32 = 8 << 20;
const RNG_WORDS = 662;

export var p4_battle_sim_active: c_int = 0;
export var p4_battle_sim_renderless: c_int = 0;

const Session = struct {
    flags: u32 = 0,
    encounter: u32 = 0,
    seed: u32 = 0,
    mt_seed: u32 = 0,
    effect_seed: u32 = 0,
    limit: u32 = 0,
    boot_limit: u32 = 0,
    policy: u32 = 0,
    latency: u32 = 0,
    action_limit: u32 = 0,
    tape_count: u32 = 0,
    save_size: u32 = 0,
    rng_size: u32 = 0,
    updates: u32 = 0,
    boot_updates: u32 = 0,
    tape_index: u32 = 0,
    launched: u32 = 0,
    seen_battle: u32 = 0,
    entry_draws: u32 = 0,
    entry_rng_hash: u64 = 0,
    last_hash: u64 = 0,
    entry: [*c]u8 = null,
    save: [*c]const u8 = null,
    rng: [*c]const u8 = null,
    tape: [*c]const u8 = null,
    trace: ?*c.FILE = null,
    result: [*c]const u8 = null,
    defeat_cause: [*c]const u8 = null,
    initial_party: [2048]u8 = @splat(0),
    first_command_party: [2048]u8 = @splat(0),
    first_command_update: u32 = 0,
    first_command_seen: c_int = 0,
    observation_error: [*c]const u8 = null,
    observer_token: u32 = 0,
    admissions: [2][3]u64 = .{ .{ 0, 0, 0 }, .{ 0, 0, 0 } },
    commands: [3]u64 = .{ 0, 0, 0 },
    free_commands: u64 = 0,
    action_count: u64 = 0,
    hp_cost: u64 = 0,
    sp_cost: u64 = 0,
    item_cost: u64 = 0,
    item_counts: [1536]u64 = @splat(0),
    hp_damage: u64 = 0,
    hp_recovery: u64 = 0,
    sp_drain: u64 = 0,
    sp_recovery: u64 = 0,
    hp_cost_adjustment: i64 = 0,
    sp_cost_adjustment: i64 = 0,
    hp_target_adjustment: i64 = 0,
    sp_target_adjustment: i64 = 0,
    protagonist_lethal: c.P4BattleHit = std.mem.zeroes(c.P4BattleHit),
    protagonist_lethal_update: u32 = 0,
    has_protagonist_lethal: c_int = 0,
    preparing: c_int = 0,
    prepare_decoded: c_int = 0,
    prepare_state: c_int = 0,
    prepare_rng_valid: c_int = 0,
    prepare_complete: c_int = 0,
    preparation: c.P4BattlePreparation = std.mem.zeroes(c.P4BattlePreparation),
    entry_size: usize = 0,
    prepared_image: [*c]const u8 = null,
    input_error: [*c]const u8 = null,
    context_before: [5]u32 = .{ 0, 0, 0, 0, 0 },
    context_after: [5]u32 = .{ 0, 0, 0, 0, 0 },
    prepared_size: u32 = 0,
    prepare_rng_entry: u64 = 0,
    prepare_rng_exit: u64 = 0,
    request_hash: [65]u8 = @splat(0),
    source_hash: [65]u8 = @splat(0),
    prepared_hash: [65]u8 = @splat(0),
    prompt_ready: u32 = 0,
    skill_offers_declined: u32 = 0,
    walk: ?*c.P4Walk = null, // policy 4: the ENTRY's battle:/battlerule: plan on the walker's battle driver
    opening: bool = false,
    opening_done: bool = false,
    walk_frame: u32 = 0,
    plan_hash: [65]u8 = @splat(0),
    mod_cohort: c_int = 0, // a declared mod set: explicit mod-witness simulation only
    mod_identity: [65]u8 = @splat(0),
    // The cohort's declared equipment of added items (P4PC_BATTLE_SIM_EQUIPMENT), applied to the active item
    // table's expanded-equipment side state once the ENTRY image is restored.
    mod_equipment: [30]ModEquipment = @splat(.{}),
    mod_equipment_count: u32 = 0,
};
const ModEquipment = struct { actor: u16 = 0, slot: u16 = 0, item: u16 = 0 };
var session: Session = .{};

fn le32(p: [*c]const u8) u32 {
    return @as(u32, p[0]) | (@as(u32, p[1]) << 8) | (@as(u32, p[2]) << 16) | (@as(u32, p[3]) << 24);
}
fn hash_bytes(seed: u64, pointer: ?*const anyopaque, size: usize) u64 {
    const p: [*]const u8 = @ptrCast(pointer.?);
    var h = seed;
    for (0..size) |i| h = (h ^ p[i]) *% 1099511628211;
    return h;
}
fn environment(key: [*c]const u8, value: [*c]const u8) c_int {
    if (builtin.os.tag == .windows) return _putenv_s(key, value);
    return setenv(key, value, 1);
}
fn guest_range(address: u32, size: u32) bool {
    return address >= 0x00100000 and address <= 0x02000000 and size <= 0x02000000 - address;
}
fn fail(message: [*c]const u8) c_int {
    session.input_error = message;
    _ = fprintf(cStderr(), "p4pc: battle-sim: %s\n", message);
    return -1;
}

// Policy 4: a walker battle plan (include/p4pc/walk_script.h) drives the party through the same
// battle driver as gate walks. Only battle:/battlerule:/battleturn: lines, printable ASCII; the worker appends
// the waits that keep the driver running until the battle task ends. Inputs come from ENTRY only.
const WALK_POLICY: u32 = 4;
// Policy 6: attentive with Persona ratings (policy.zig); +32 is its ratings payload's byte count, as for a walk plan.
const AFFINITY_POLICY: u32 = 6;
const LAST_POLICY: u32 = 9; // 7: technical, 8: strategic, 9: expert
fn text_policy(p: u32) bool {
    return p == WALK_POLICY or p == AFFINITY_POLICY;
}
const WALK_PLAN_MAX: u32 = 16384;
const RATINGS_PAYLOAD_MAX: u32 = 12 + 512 * 10;

fn sim_walk_read32(user: ?*anyopaque, address: u32) callconv(.c) u32 {
    _ = user;
    return if (address >= 0x00100000 and address <= 0x01FFFFFC) g32(address) else 0;
}
fn sim_walk_note(user: ?*anyopaque, message: [*c]const u8) callconv(.c) void {
    _ = user;
    _ = fprintf(cStderr(), "p4pc: battle-sim walk: %s\n", message);
}

fn opening_turn(_: ?*anyopaque, _: c_uint, _: [*c]u8, _: usize) callconv(.c) c_int {
    if (session.opening) session.opening_done = true;
    return 0;
}
fn parse_walk_plan(text: [*c]const u8, size: u32) !void {
    const script: [*c]u8 = @ptrCast(c.malloc(@as(usize, size) + 96));
    if (script == null) return error.OutOfMemory;
    defer c.free(script);
    for (0..size) |i| {
        if (text[i] != '\n' and (text[i] < 0x20 or text[i] > 0x7e))
            return error.NotPrintableAscii;
    }
    var start: u32 = 0;
    while (start < size) {
        var stop = start;
        while (stop < size and text[stop] != '\n') stop += 1;
        const len: usize = stop - start;
        const line = text + start;
        if (len != 0 and !(len > 7 and std.mem.eql(u8, line[0..7], "battle:")) and
            !(len > 11 and std.mem.eql(u8, line[0..11], "battlerule:")) and
            !(len > 11 and std.mem.eql(u8, line[0..11], "battleturn:")))
            return error.InvalidLine;
        start = stop + 1;
    }
    if (size != 0) @memcpy(script[0..size], text[0..size]);
    _ = snprintf(script + size, 96, "\ntimeout:%u\nwaittask:battle\nwaittask:battle:off\n", @as(c_uint, session.limit));
    var reason: [160]u8 = undefined;
    session.walk = c.p4walk_parse(script, &reason, reason.len);
    if (session.walk == null) return error.InvalidPlan;
}

fn walk_plan(text: [*c]const u8, size: u32) c_int {
    parse_walk_plan(text, size) catch |err| return fail(switch (err) {
        error.OutOfMemory => "cannot allocate walk battle plan",
        error.NotPrintableAscii => "walk battle plan is not printable ASCII",
        error.InvalidLine => "walk battle plan line is not battle:, battlerule: or battleturn:",
        error.InvalidPlan => "invalid walk battle plan",
    });
    c.p4_content_digest(text, size, &session.plan_hash);
    return 0;
}

const walk_pad = [_]u16{
    c.P4PC_PAD_UP,    c.P4PC_PAD_DOWN,   c.P4PC_PAD_LEFT, c.P4PC_PAD_RIGHT, c.P4PC_PAD_CROSS, c.P4PC_PAD_CIRCLE, c.P4PC_PAD_TRIANGLE, c.P4PC_PAD_SQUARE,
    c.P4PC_PAD_START, c.P4PC_PAD_SELECT, c.P4PC_PAD_L1,   c.P4PC_PAD_R1,    c.P4PC_PAD_L2,    c.P4PC_PAD_R2,     c.P4PC_PAD_L3,       c.P4PC_PAD_R3,
};
comptime {
    std.debug.assert(walk_pad.len == c.P4W_BUTTON_COUNT);
}
const walk_host: c.P4WalkHost = blk: {
    var host = std.mem.zeroes(c.P4WalkHost);
    host.read32 = &sim_walk_read32;
    host.note = &sim_walk_note;
    host.battle_turn = &opening_turn;
    break :blk host;
};
fn walk_plan_update() void {
    session.walk_frame +%= 1;
    const mask = c.p4walk_frame(session.walk, &walk_host, session.walk_frame);
    if (c.p4walk_status(session.walk) == c.P4WALK_FAILED) finish("policy_error", "walk_battle_plan_failed", 4);
    if (session.opening_done) {
        c.p4walk_free(session.walk);
        session.walk = null;
        session.opening = false;
        return;
    }
    var bits: c_uint = 0;
    for (walk_pad, 0..) |pad, b| {
        if (mask & (@as(u32, 1) << @intCast(b)) != 0) bits |= pad;
    }
    c.p4pc_pad_inject(bits, 1);
}

fn load_entry(path: [*c]const u8) !usize {
    const f = c.fopen(path, "rb");
    if (f == null) return error.OpenFailed;
    defer _ = c.fclose(f);
    if (c.fseek(f, 0, c.SEEK_END) != 0) return error.SizeFailed;
    const length: c_long = c.ftell(f);
    if (length < 64 or length > MAX_ENTRY or c.fseek(f, 0, c.SEEK_SET) != 0)
        return error.InvalidSize;
    const size: usize = @intCast(length);
    session.entry = @ptrCast(c.malloc(size));
    if (session.entry == null) return error.OutOfMemory;
    if (c.fread(session.entry, 1, size, f) != size or c.ferror(f) != 0)
        return error.ReadFailed;
    return size;
}

fn read_entry(path: [*c]const u8) c_int {
    const size = load_entry(path) catch |err| return fail(switch (err) {
        error.OpenFailed => "cannot read immutable BattleEntry",
        error.SizeFailed => "cannot size BattleEntry",
        error.InvalidSize => "BattleEntry size outside contract",
        error.OutOfMemory => "cannot allocate BattleEntry",
        error.ReadFailed => "cannot read complete BattleEntry",
    });
    session.entry_size = size;
    if (session.preparing != 0) {
        var reason: [*c]const u8 = null;
        if (c.p4_battle_prepare_decode(session.entry, session.entry_size, &session.preparation, &reason) == 0) return fail(reason);
        session.prepare_decoded = 1;
        session.save = session.preparation.source;
        session.save_size = session.preparation.source_bytes;
        session.flags = 1;
        session.seed = le32(session.entry + 48);
        session.mt_seed = le32(session.entry + 52);
        session.effect_seed = le32(session.entry + 56);
        session.boot_limit = le32(session.entry + 60);
        session.limit = le32(session.entry + 64);
        return 0;
    }
    const h = session.entry;
    if (!std.mem.eql(u8, h[0..8], "P4BSIM01") or le32(h + 8) != 64) return fail("unsupported BattleEntry header");
    session.flags = le32(h + 12);
    session.encounter = le32(h + 16);
    session.seed = le32(h + 20);
    session.mt_seed = le32(h + 24);
    session.limit = le32(h + 28);
    session.tape_count = le32(h + 32);
    session.save_size = le32(h + 36);
    session.rng_size = le32(h + 40);
    session.boot_limit = le32(h + 44);
    session.policy = le32(h + 48);
    session.latency = le32(h + 52);
    session.action_limit = le32(h + 56);
    session.effect_seed = le32(h + 60);
    // Policies 4 and 6 carry a payload (walk plan, Persona ratings): +32 is its byte count, it follows the RNG block.
    const payload: u64 = if (text_policy(session.policy) or session.flags & 16 != 0) @as(u64, session.tape_count) else @as(u64, session.tape_count) * 12;
    const expected: u64 = 64 + @as(u64, session.save_size) + session.rng_size + payload;
    if (session.flags & ~@as(u32, 63) != 0 or session.encounter > 65535 or session.limit == 0 or session.boot_limit == 0 or
        session.action_limit == 0 or session.policy > LAST_POLICY or expected != @as(u64, size) or
        session.rng_size != (if (session.flags & 2 != 0) @as(u32, RNG_WORDS * 4) else 0)) return fail("invalid BattleEntry counts/flags/limits");
    if (session.policy != 0 and !text_policy(session.policy) and session.flags & 16 == 0 and session.tape_count != 0)
        return fail("named party policy cannot compete with a controller tape");
    if (session.policy == WALK_POLICY and (session.tape_count == 0 or session.tape_count > WALK_PLAN_MAX))
        return fail("invalid walk battle plan size");
    if (session.policy == AFFINITY_POLICY and (session.tape_count == 0 or session.tape_count > RATINGS_PAYLOAD_MAX))
        return fail("invalid Persona ratings size");
    if (session.flags & 16 != 0 and ((session.policy != 8 and session.policy != 9) or session.tape_count == 0 or session.tape_count > WALK_PLAN_MAX))
        return fail("invalid scripted opening payload");
    if (session.flags & 32 != 0 and session.policy != 8 and session.policy != 9)
        return fail("decision trace requires strategic or expert policy");

    session.save = session.entry + 64;
    session.rng = session.save + session.save_size;
    session.tape = session.rng + session.rng_size;
    if (c.p4_battle_save_valid(session.save, session.save_size) == 0) return fail("invalid native save version, section layout or checksum");
    if (session.rng_size != 0) {
        const front = le32(session.rng + 124);
        const back = le32(session.rng + 128);
        if (front >= 31 or back >= 31 or (front + 31 - back) % 31 != 3 or le32(session.rng + 2628) >= 624)
            return fail("invalid captured RNG cursor");
    }
    if (session.flags & 16 != 0) {
        session.opening = true;
        return walk_plan(session.tape, session.tape_count);
    }
    if (session.policy == WALK_POLICY) return walk_plan(session.tape, session.tape_count);
    if (session.policy == AFFINITY_POLICY) {
        if (c.p4_battle_policy_set_ratings(session.tape, session.tape_count) == 0) return fail("invalid Persona ratings payload");
        c.p4_content_digest(session.tape, session.tape_count, &session.plan_hash);
        return 0;
    }
    var end: u32 = 0;
    for (0..session.tape_count) |i| {
        const r = session.tape + i * 12;
        const at = le32(r);
        const bits = le32(r + 4);
        const hold = le32(r + 8);
        if (at < end or at >= session.limit or bits > 65535 or hold == 0 or hold > session.limit - at)
            return fail("invalid/overlapping controller tape record");
        end = at + hold;
    }
    return 0;
}

// Context declared by the scenario, not hidden encounter-table data.
export fn p4_battle_sim_is_boss() c_int {
    return @intFromBool(session.flags & 8 != 0);
}
export fn p4_battle_sim_opening_active() c_int {
    return @intFromBool(session.opening);
}
export fn p4_battle_sim_decision_trace() ?*anyopaque {
    if (session.flags & 32 == 0) return null;
    return if (session.trace) |trace| @ptrCast(trace) else null;
}


export fn p4_battle_sim_mod_cohort() c_int {
    return session.mod_cohort;
}

export fn p4_battle_sim_cli(argc: c_int, argv: [*c][*c]u8, elf: [*c][*c]const u8) c_int {
    var entry: [*c]const u8 = null;
    var result: [*c]const u8 = null;
    var trace: [*c]const u8 = null;
    var prepared: [*c]const u8 = null;
    var requested: c_int = 0;
    var live: c_int = 0;
    var i: c_int = 1;
    while (i < argc) : (i += 1) {
        const arg = argv[@intCast(i)];
        if (c.strcmp(arg, "--battle-sim") == 0) requested |= 1;
        if (c.strcmp(arg, "--battle-prepare") == 0) requested |= 2;
        if (c.strcmp(arg, "--battle-live") == 0) live += 1;
    }
    if (live > 1 or (live != 0 and requested != 1)) return fail("live mode requires one battle simulation");
    if (requested == 0) return 0;
    const mod_set = c.getenv("P4PC_BATTLE_SIM_MODS");
    if (mod_set != null) {
        // A declared mod set runs only as an explicit mod-witness battle
        // simulation (never preparation, live mode or a retail reference):
        // the coordinator names the set by its content identity.
        var valid = c.strlen(mod_set) == 64;
        var k: usize = 0;
        while (valid and k < 64) : (k += 1)
            valid = (mod_set[k] >= '0' and mod_set[k] <= '9') or (mod_set[k] >= 'a' and mod_set[k] <= 'f');
        if (!valid) return fail("P4PC_BATTLE_SIM_MODS must be a lowercase SHA-256 mod-set identity");
        if (requested != 1 or live != 0) return fail("a declared mod set runs only battle simulation");
        const witness = c.getenv("P4PC_MOD_WITNESS");
        const directory = c.getenv("P4PC_MOD_DIR");
        if (witness == null or c.strcmp(witness, "1") != 0 or directory == null or directory[0] == 0)
            return fail("a declared mod set requires P4PC_MOD_WITNESS=1 and P4PC_MOD_DIR");
        session.mod_cohort = 1;
        @memcpy(session.mod_identity[0..], mod_set[0..session.mod_identity.len]);
    } else if (c.getenv("P4PC_MOD_WITNESS") != null) return fail("mod witness is incompatible with simulator fidelity");
    const equipment = c.getenv("P4PC_BATTLE_SIM_EQUIPMENT");
    if (equipment != null) {
        // "actor:slot:item" entries, comma-separated: added items only (the retail fields belong to preparation).
        if (session.mod_cohort == 0) return fail("declared mod equipment requires a declared mod set");
        var p: [*c]const u8 = equipment;
        while (p.* != 0) {
            var end: [*c]u8 = undefined;
            const actor = c.strtoul(p, &end, 10);
            if (end == p or end.* != ':') return fail("invalid P4PC_BATTLE_SIM_EQUIPMENT");
            p = end + 1;
            const slot = c.strtoul(p, &end, 10);
            if (end == p or end.* != ':') return fail("invalid P4PC_BATTLE_SIM_EQUIPMENT");
            p = end + 1;
            const item = c.strtoul(p, &end, 10);
            if (end == p or (end.* != 0 and end.* != ',') or actor == 0 or actor > 10 or slot > 2 or
                item < c.P4_ITEM_RETAIL_COUNT or item > 0xFFFF or session.mod_equipment_count == session.mod_equipment.len)
                return fail("invalid P4PC_BATTLE_SIM_EQUIPMENT");
            for (session.mod_equipment[0..session.mod_equipment_count]) |declared| {
                if (declared.actor == actor and declared.slot == slot) return fail("duplicate P4PC_BATTLE_SIM_EQUIPMENT slot");
            }
            session.mod_equipment[session.mod_equipment_count] = .{ .actor = @intCast(actor), .slot = @intCast(slot), .item = @intCast(item) };
            session.mod_equipment_count += 1;
            const more = end.* != 0;
            p = if (more) end + 1 else end;
            if (more and p.* == 0) return fail("invalid P4PC_BATTLE_SIM_EQUIPMENT");
        }
        if (session.mod_equipment_count == 0) return fail("invalid P4PC_BATTLE_SIM_EQUIPMENT");
    }
    if (requested == 3) return fail("battle simulation and preparation are mutually exclusive");
    session.preparing = @intFromBool(requested == 2);
    const preparing = session.preparing != 0;
    elf.* = null;
    i = 1;
    while (i < argc) : (i += 1) {
        const arg = argv[@intCast(i)];
        var value: *[*c]const u8 = undefined;
        if ((!preparing and c.strcmp(arg, "--battle-sim") == 0) or
            (preparing and c.strcmp(arg, "--battle-prepare") == 0)) value = &entry
        else if ((!preparing and c.strcmp(arg, "--battle-result") == 0) or
            (preparing and c.strcmp(arg, "--battle-prepare-result") == 0)) value = &result
        else if (!preparing and c.strcmp(arg, "--battle-trace") == 0) value = &trace
        else if (preparing and c.strcmp(arg, "--battle-prepared-image") == 0) value = &prepared
        else if (live != 0 and c.strcmp(arg, "--battle-live") == 0) continue
        else if (arg[0] != '-' and elf.* == null) {
            elf.* = arg;
            continue;
        } else return fail("unknown or duplicate simulator argument");
        if (value.* != null or i + 1 == argc) return fail("missing or duplicate simulator argument");
        i += 1;
        value.* = argv[@intCast(i)];
    }
    if (preparing) {
        if (entry == null or result == null or prepared == null or elf.* == null or c.strcmp(entry, result) == 0 or
            c.strcmp(entry, prepared) == 0 or c.strcmp(result, prepared) == 0)
            return fail("preparation requires distinct request/image/result and an explicit ELF");
    } else if (entry == null or result == null or trace == null or elf.* == null or c.strcmp(entry, result) == 0 or
        c.strcmp(entry, trace) == 0 or c.strcmp(result, trace) == 0)
        return fail("simulator requires distinct entry/result/trace and an explicit ELF");
    if (live != 0) return c.p4_battle_live_start(entry, result, trace);
    const incompatible = [_][*:0]const u8{
        "P4PC_WALK_SCRIPT",       "P4PC_START_BATTLE",   "P4PC_BATTLE_DRIVER", "P4PC_AUTOPRESS",       "P4PC_AUTOPRESS_LOAD",
        "P4PC_AUTOPRESS_NEWGAME", "P4PC_CAPTURE_FRAMES", "P4PC_CAPTURE_STATE", "P4PC_START_MENU",      "P4PC_MENU_SCRIPT",
        "P4PC_MOD_WITNESS",       "P4PC_TRAP_CONTINUE",  "P4PC_OVERRIDE_DIR",  "P4PC_LOCKSTEP_REPLAY", "P4PC_LOCKSTEP_LIVE",
    };
    for (incompatible) |name| {
        if (session.mod_cohort != 0 and c.strcmp(name, "P4PC_MOD_WITNESS") == 0) continue;
        if (c.getenv(name) != null) return fail("competing gameplay/automation environment in simulator");
    }
    if (c.getenv("P4PC_SAVE_DIR") == null or c.getenv("P4PC_CONFIG") == null) return fail("simulator requires a private save/config profile");
    var check = c.fopen(result, "rb");
    if (check != null) {
        _ = c.fclose(check);
        return fail("result exists; refusing overwrite");
    }
    if (prepared != null) {
        check = c.fopen(prepared, "rb");
        if (check != null) {
            _ = c.fclose(check);
            return fail("prepared image exists; refusing overwrite");
        }
    }
    session.result = result;
    session.prepared_image = prepared;
    if (read_entry(entry) != 0) {
        if (session.preparing != 0) prepare_finish("preparation_error", session.input_error, 1);
        return -1;
    }
    const keys = [_][*:0]const u8{ "P4PC_INPUT", "P4PC_MODS", "P4PC_PACKS", "P4PC_VIRTUAL_CLOCK", "P4PC_TURBO", "P4PC_HIDDEN_WINDOW", "SDL_AUDIODRIVER" };
    const values = [_][*:0]const u8{ "script", if (session.mod_cohort != 0) "1" else "0", "0", "1", "1", "1", "dummy" };
    for (keys, values) |key, value| {
        if (environment(key, value) != 0) return fail("cannot isolate simulator environment");
    }
    if (session.preparing == 0) {
        session.trace = c.fopen(trace, "wbx");
        if (session.trace == null) return fail("cannot create exclusive trace");
    }
    c.p4_mod_fidelity_check_start();
    p4_battle_sim_renderless = @intFromBool(session.flags & 1 != 0);
    p4_battle_sim_active = 1;
    return 1;
}

export fn p4_battle_sim_update_count() u32 {
    return session.updates;
}
fn additive_block() u32 {
    const offset = gp(0xb618);
    if (offset > 0x10000) return 0;
    return 0x008872e0 + offset;
}
fn rng_snapshot(out: *[RNG_WORDS]u32) bool {
    const block = additive_block();
    if (block == 0 or !guest_range(block, 16)) return false;
    const base = g32(block);
    const front = g32(block + 4);
    const back = g32(block + 8);
    const end = g32(block + 12);
    if (!guest_range(base, 124) or end != base +% 124 or front < base or front >= end or back < base or back >= end or
        ((front - base) & 3) != 0 or ((back - base) & 3) != 0) return false;
    const words = std.mem.asBytes(out);
    @memcpy(words[0..124], gbytes(base)[0..124]);
    out[31] = (front - base) / 4;
    out[32] = (back - base) / 4;
    @memcpy(words[33 * 4 ..][0..2496], gbytes(0x00885a90)[0..2496]);
    out[657] = gp(0xaa7c);
    @memcpy(words[658 * 4 ..][0..16], gbytes(0x00922de0)[0..16]);
    return true;
}
fn restore_rng() bool {
    var state: [RNG_WORDS]u32 = undefined;
    if (!rng_snapshot(&state)) return false;
    if (session.flags & 2 == 0) {
        RpRandomSeed(@bitCast(session.seed), null);
        RpRandomSeedMT(@bitCast(session.mt_seed));
        effMiscRandInit(null, session.effect_seed);
    } else {
        const block = additive_block();
        const base = g32(block);
        @memcpy(gbytes(base)[0..124], session.rng[0..124]);
        g32p(block + 4).* = base +% le32(session.rng + 124) *% 4;
        g32p(block + 8).* = base +% le32(session.rng + 128) *% 4;
        @memcpy(gbytes(0x00885a90)[0..2496], (session.rng + 132)[0..2496]);
        gpp(0xaa7c).* = le32(session.rng + 2628);
        @memcpy(gbytes(0x00922de0)[0..16], (session.rng + 2632)[0..16]);
    }
    return rng_snapshot(&state);
}
fn rng_hash(hash: *u64) bool {
    var state: [RNG_WORDS]u32 = undefined;
    if (!rng_snapshot(&state)) return false;
    hash.* = hash_bytes(14695981039346656037, &state, @sizeOf(@TypeOf(state)));
    return true;
}
fn context_json(file: ?*c.FILE, context: *const [5]u32) void {
    _ = fprintf(file, "{\"date\":%u,\"time_slot\":%u,\"field\":%u,\"room\":%u,\"step\":%u}", @as(c_uint, context[0]), @as(c_uint, context[1]), @as(c_uint, context[2]), @as(c_uint, context[3]), @as(c_uint, context[4]));
}
fn prepare_finish(outcome: [*c]const u8, cause: [*c]const u8, code: c_int) noreturn {
    const file = c.fopen(session.result, "wbx");
    if (file == null) {
        _ = fprintf(cStderr(), "p4pc: cannot create exclusive preparation receipt\n");
        exit(5);
    }
    _ = fprintf(file, "{\"schema\":1,\"operation\":\"battle_prepare\",\"outcome\":\"%s\",\"terminal_cause\":\"%s\",\"bootstrap_updates\":%u,\"field_resume_updates\":%u,", outcome, cause, @as(c_uint, session.boot_updates), @as(c_uint, session.updates));
    _ = c.fputs("\"request_sha256\":", file);
    if (session.entry_size != 0) {
        c.p4_content_digest(session.entry, session.entry_size, &session.request_hash);
        _ = fprintf(file, "\"%s\",", &session.request_hash);
    } else _ = c.fputs("null,", file);
    _ = c.fputs("\"definition_sha256\":", file);
    if (session.prepare_decoded != 0) {
        _ = c.fputc('"', file);
        for (0..32) |i| _ = fprintf(file, "%02x", @as(c_uint, session.preparation.header[128 + i]));
        c.p4_content_digest(session.preparation.source, session.preparation.source_bytes, &session.source_hash);
        const classification: [*:0]const u8 = if (le32(session.preparation.header + 36) == 1) "source_attested" else "synthetic_lab";
        const resource_mode: [*:0]const u8 = if (le32(session.preparation.header + 40) != 0) "declared_fill_living_hp_sp" else "native_inherit_or_explicit_actors";
        const clear: [*:0]const u8 = if (le32(session.preparation.header + 44) != 0) "true" else "false";
        _ = fprintf(file, "\",\"source_sha256\":\"%s\",\"classification\":\"%s\",\"resource_mode\":\"%s\",\"clear_temporary_statuses\":%s,", &session.source_hash, classification, resource_mode, clear);
    } else _ = c.fputs("null,\"source_sha256\":null,\"classification\":null,", file);
    _ = c.fputs("\"context_before\":", file);
    if (session.prepare_state != 0) context_json(file, &session.context_before) else _ = c.fputs("null", file);
    _ = c.fputs(",\"context_after\":", file);
    if (session.prepare_state >= 2) context_json(file, &session.context_after) else _ = c.fputs("null", file);
    _ = c.fputs(",\"preparation_rng\":", file);
    if (session.prepare_rng_valid != 0) {
        const have_exit = session.prepare_complete != 0 or rng_hash(&session.prepare_rng_exit);
        _ = fprintf(file, "{\"policy\":\"seed_at_native_apply\",\"additive_seed\":%u,\"mt_seed\":%u,\"effect_seed\":%u,\"bank_hash_algorithm\":\"fnv1a64-662-native-words\",\"entry\":\"%016llx\",\"exit\":", @as(c_uint, session.seed), @as(c_uint, session.mt_seed), @as(c_uint, session.effect_seed), @as(c_ulonglong, session.prepare_rng_entry));
        if (have_exit) _ = fprintf(file, "\"%016llx\"", @as(c_ulonglong, session.prepare_rng_exit)) else _ = c.fputs("null", file);
        _ = fprintf(file, ",\"additive_draws\":%u}", @as(c_uint, p4pc_rng_draws -% session.entry_draws));
    } else _ = c.fputs("null", file);
    if (session.prepare_complete != 0) {
        _ = fprintf(file, ",\"effective_image_sha256\":\"%s\",\"effective_image_bytes\":%u,\"effective_image_native_valid\":true,\"effective_image_kind\":\"native_full\",", &session.prepared_hash, @as(c_uint, session.prepared_size));
        c.p4_battle_prepare_report(file, &session.preparation);
    }
    _ = c.fputs("}\n", file);
    const write_error = c.ferror(file);
    const close_error = c.fclose(file);
    if (write_error != 0 or close_error != 0) {
        _ = fprintf(cStderr(), "p4pc: cannot finish preparation receipt\n");
        exit(5);
    }
    p4pc_exit(code);
}
fn party_json(out: [*c]u8, cap: usize) usize {
    var used: usize = 0;
    var seen: c_uint = 0;
    var count: c_int = 0;
    const ids = [4]i32{ 1, @as(i32, datGetPartyId(0)) & 65535, @as(i32, datGetPartyId(1)) & 65535, @as(i32, datGetPartyId(2)) & 65535 };
    out[0] = 0;
    for (ids) |id| {
        if (id < 1 or id > 10 or (seen & (@as(c_uint, 1) << @intCast(id))) != 0) continue;
        seen |= @as(c_uint, 1) << @intCast(id);
        const native: i16 = @intCast(id);
        const unit = datUnitGetRecord(native);
        const separator: [*:0]const u8 = if (count != 0) "," else "";
        const n = snprintf(out + used, cap - used, "%s{\"id\":%d,\"hp\":%u,\"sp\":%u,\"max_hp\":%u,\"max_sp\":%u,\"status\":%u}", separator, @as(c_int, id), @as(c_uint, datCalcGetHp(unit)), @as(c_uint, datCalcGetSp(unit)), @as(c_uint, datCalcMaxHpById(native)), @as(c_uint, datCalcMaxSpById(native)), @as(c_uint, datCalcGetBadStatus(unit)));
        if (n < 0 or @as(usize, @intCast(n)) >= cap - used) return 0;
        used += @intCast(n);
        count += 1;
    }
    return used;
}
export fn p4_battle_sim_command_ready(action: u32) void {
    if (c.p4_battle_live_active != 0) {
        c.p4_battle_live_command_ready(action);
        return;
    }
    if (session.trace == null or session.first_command_seen != 0 or session.observation_error != null) return;
    const unit = g32(action +% 0x30);
    if (g8(unit +% 0xa2) != 0 or g16(unit +% 0xa4) != 1) return;
    if (party_json(&session.first_command_party, session.first_command_party.len) == 0) {
        session.observation_error = "first_protagonist_command_resources";
        return;
    }
    // The declared added-item equipment must still be in place once the battle asks for its first command.
    for (session.mod_equipment[0..session.mod_equipment_count]) |declared| {
        var item: u32 = 0;
        if (c.p4_game_tables_items_equipment_get(c.p4_game_tables_items_active(), declared.actor, declared.slot, &item) != c.P4_TABLE_OK or
            item != declared.item)
        {
            session.observation_error = "mod_equipment_lost_before_battle";
            return;
        }
    }
    session.first_command_seen = 1;
    session.first_command_update = session.updates +% 1;
    _ = fprintf(session.trace, "{\"kind\":\"first_protagonist_command\",\"boundary\":\"native_input_ready\",\"update\":%u,\"party\":[%s]}\n", @as(c_uint, session.first_command_update), &session.first_command_party);
}
fn actor_json(file: ?*c.FILE, actor: *allowzero const c.P4BattleActor) void {
    _ = fprintf(file, "{\"present\":%u,\"action_uid\":\"%016llx\",\"unit_uid\":%u,\"id\":%u,\"side\":%u}", @as(c_uint, actor.present), @as(c_ulonglong, actor.action_uid), @as(c_uint, actor.unit_uid), @as(c_uint, actor.native_id), @as(c_uint, actor.side));
}
fn admission_event(event_ptr: [*c]const c.P4BattleAdmission, context: ?*anyopaque) callconv(.c) void {
    _ = context;
    const event = &event_ptr[0];
    const side: usize = if (event.actor.side < 2) event.actor.side else 2;
    session.admissions[@intFromBool(event.queue_kind == c.P4_BATTLE_QUEUE_FOLLOWUP)][side] +%= 1;
    _ = fprintf(session.trace, "{\"kind\":\"admission\",\"update\":%u,\"queue_kind\":%u,\"native_action_count\":%u,\"requested_state\":%u,\"actor\":", @as(c_uint, session.updates +% 1), @as(c_uint, event.queue_kind), @as(c_uint, event.native_action_count), @as(c_uint, event.next_state));
    actor_json(session.trace, &event.actor);
    _ = c.fputs("}\n", session.trace);
}
fn command_event(event_ptr: [*c]const c.P4BattleCommand, context: ?*anyopaque) callconv(.c) void {
    _ = context;
    const event = &event_ptr[0];
    const side: usize = if (event.actor.side < 2) event.actor.side else 2;
    if (event.free_action != 0) session.free_commands +%= 1 else {
        session.commands[side] +%= 1;
        session.action_count +%= 1;
    }
    _ = fprintf(session.trace, "{\"kind\":\"command_commit\",\"update\":%u,\"action_kind\":%u,\"skill_id\":%u,\"item_id\":%u,\"target_count\":%u,\"native_state\":%u,\"free_action\":%u,\"actor\":", @as(c_uint, session.updates +% 1), @as(c_uint, event.action_kind), @as(c_uint, event.skill_id), @as(c_uint, event.item_id), @as(c_uint, event.target_count), @as(c_uint, event.native_state), @as(c_uint, event.free_action));
    actor_json(session.trace, &event.actor);
    _ = c.fputs("}\n", session.trace);
}
fn cost_event(event_ptr: [*c]const c.P4BattleCost, context: ?*anyopaque) callconv(.c) void {
    _ = context;
    const event = &event_ptr[0];
    if (event.actor.side == 0) {
        if (event.kind == c.P4_BATTLE_COST_HP) {
            session.hp_cost +%= event.requested_cost;
            session.hp_cost_adjustment +%= @as(i64, event.hp_after) - @as(i64, event.hp_before) + @as(i64, event.requested_cost);
        } else if (event.kind == c.P4_BATTLE_COST_SP) {
            session.sp_cost +%= event.requested_cost;
            session.sp_cost_adjustment +%= @as(i64, event.sp_after) - @as(i64, event.sp_before) + @as(i64, event.requested_cost);
        } else if (event.kind == c.P4_BATTLE_COST_ITEM and event.item_count_before > event.item_count_after) {
            const used: u32 = event.item_count_before - event.item_count_after;
            session.item_cost +%= used;
            if (event.item_id < 1536) session.item_counts[event.item_id] +%= used;
        }
    }
    _ = fprintf(session.trace, "{\"kind\":\"cost_commit\",\"update\":%u,\"cost_kind\":%u,\"requested_cost\":%u,\"skill_id\":%u,\"item_id\":%u,\"hp_before\":%u,\"hp_after\":%u,\"sp_before\":%u,\"sp_after\":%u,\"item_count_before\":%u,\"item_count_after\":%u,\"actor\":", @as(c_uint, session.updates +% 1), @as(c_uint, event.kind), @as(c_uint, event.requested_cost), @as(c_uint, event.skill_id), @as(c_uint, event.item_id), @as(c_uint, event.hp_before), @as(c_uint, event.hp_after), @as(c_uint, event.sp_before), @as(c_uint, event.sp_after), @as(c_uint, event.item_count_before), @as(c_uint, event.item_count_after));
    actor_json(session.trace, &event.actor);
    _ = c.fputs("}\n", session.trace);
}
fn hit_event(event_ptr: [*c]const c.P4BattleHit, context: ?*anyopaque) callconv(.c) void {
    _ = context;
    const event = &event_ptr[0];
    if (event.target.side == 0) {
        if (event.target.native_id == 1) {
            if (event.hp_after == 0 and event.hp_before != 0) {
                session.protagonist_lethal = event.*;
                session.protagonist_lethal_update = session.updates +% 1;
                session.has_protagonist_lethal = 1;
            } else if (event.hp_after != 0) session.has_protagonist_lethal = 0;
        }
        const requested_hp: i64 = if (event.resources_applied != 0) event.requested_hp_delta else 0;
        const requested_sp: i64 = if (event.resources_applied != 0) event.requested_sp_delta else 0;
        if (requested_hp < 0) session.hp_damage +%= @as(u64, @bitCast(-requested_hp)) else session.hp_recovery +%= @as(u64, @bitCast(requested_hp));
        if (requested_sp < 0) session.sp_drain +%= @as(u64, @bitCast(-requested_sp)) else session.sp_recovery +%= @as(u64, @bitCast(requested_sp));
        session.hp_target_adjustment +%= @as(i64, event.hp_after) - @as(i64, event.hp_before) - requested_hp;
        session.sp_target_adjustment +%= @as(i64, event.sp_after) - @as(i64, event.sp_before) - requested_sp;
    }
    _ = fprintf(session.trace, "{\"kind\":\"target_commit\",\"update\":%u,\"skill_id\":%u,\"action_kind\":%u,\"hp_delta_requested\":%d,\"sp_delta_requested\":%d,\"money_delta_requested\":%d,\"hp_before\":%u,\"hp_after\":%u,\"sp_before\":%u,\"sp_after\":%u,\"status_before\":%u,\"status_after\":%u,\"added_status\":%u,\"removed_status\":%u,\"buff_changes\":%u,\"target_flags\":%u,\"result_flags\":%u,\"effect\":%u,\"resources_applied\":%u,\"statuses_applied\":%u,\"source\":", @as(c_uint, session.updates +% 1), @as(c_uint, event.skill_id), @as(c_uint, event.action_kind), @as(c_int, event.requested_hp_delta), @as(c_int, event.requested_sp_delta), @as(c_int, event.requested_money_delta), @as(c_uint, event.hp_before), @as(c_uint, event.hp_after), @as(c_uint, event.sp_before), @as(c_uint, event.sp_after), @as(c_uint, event.status_before), @as(c_uint, event.status_after), @as(c_uint, event.added_status), @as(c_uint, event.removed_status), @as(c_uint, event.buff_changes), @as(c_uint, event.target_flags), @as(c_uint, event.result_flags), @as(c_uint, event.effect), @as(c_uint, event.resources_applied), @as(c_uint, event.statuses_applied));
    actor_json(session.trace, &event.source);
    _ = c.fputs(",\"target\":", session.trace);
    actor_json(session.trace, &event.target);
    _ = c.fputs("}\n", session.trace);
}
fn observe(kind: [*:0]const u8, full_rng: bool) u64 {
    var rng: [RNG_WORDS]u32 = undefined;
    if (!rng_snapshot(&rng)) return 0;
    const hash = hash_bytes(14695981039346656037, &rng, @sizeOf(@TypeOf(rng)));
    var state = hash;
    const context = gp(0xb3ac);
    var phase: u32 = 0;
    var flags: u32 = 0;
    var terminal: u32 = 0;
    if (context != 0 and !guest_range(context, 0xe88)) return 0;
    if (context != 0) {
        phase = g32(context + 0x2fc);
        flags = g32(context + 0xc);
        terminal = g16(context + 0x1c);
        state = hash_bytes(state, g2h(context + 0xc), 0x18);
        for (0..2) |group| {
            var unit = g32(context + 0x178 + @as(u32, @intCast(group)) * 8);
            var n: u32 = 0;
            while (unit != 0 and n < 12) : (n += 1) {
                if (!guest_range(unit, 0xa70)) return 0;
                const record = g32(unit + 0xa64);
                state = hash_bytes(state, g2h(unit + 0x98), 0x14);
                if (record != 0 and !guest_range(record, 0x30)) return 0;
                if (record != 0) state = hash_bytes(state, g2h(record), 0x30);
                unit = g32(unit + 0xa6c);
                if (n == 11 and unit != 0) return 0;
            }
        }
    }
    var party: [2048]u8 = undefined;
    if (party_json(&party, party.len) == 0) return 0;
    state = hash_bytes(state, &party, std.mem.indexOfScalar(u8, &party, 0).?);
    _ = fprintf(session.trace, "{\"kind\":\"%s\",\"update\":%u,\"phase\":%u,\"flags\":%u,\"terminal\":%u,\"rng_hash\":\"%016llx\",\"state_hash\":\"%016llx\",\"rng_draws\":%u,\"party\":[%s]", kind, @as(c_uint, session.updates), @as(c_uint, phase), @as(c_uint, flags), @as(c_uint, terminal), @as(c_ulonglong, hash), @as(c_ulonglong, state), @as(c_uint, p4pc_rng_draws -% session.entry_draws), &party);
    if (full_rng) {
        _ = c.fputs(",\"rng_words\":[", session.trace);
        for (&rng, 0..) |word, i| {
            const separator: [*:0]const u8 = if (i != 0) "," else "";
            _ = fprintf(session.trace, "%s%u", separator, @as(c_uint, word));
        }
        _ = c.fputc(']', session.trace);
    }
    _ = c.fputs("}\n", session.trace);
    if (c.ferror(session.trace) != 0) return 0;
    if (session.entry_rng_hash == 0) session.entry_rng_hash = hash;
    session.last_hash = state;
    return hash;
}
fn json_text(f: ?*c.FILE, text: [*c]const u8) void {
    _ = c.fputc('"', f);
    var p = text;
    while (p[0] != 0) : (p += 1) {
        const ch = p[0];
        if (ch == '"' or ch == '\\') _ = fprintf(f, "\\%c", @as(c_int, ch))
        else if (ch < 0x20 or ch >= 0x7f) _ = fprintf(f, "\\u%04x", @as(c_uint, ch))
        else _ = c.fputc(ch, f);
    }
    _ = c.fputc('"', f);
}
const policy_names = [_][*:0]const u8{
    "controller-tape-v1",   "greedy-weakness-v1",          "cautious-healer-v1",  "typical-player-v1",
    "walk-battle-rules-v1", "attentive-player-v1",         "attentive-player-affinity-v1",
    "technical-player-v1",  "strategic-player-candidate-v1", "expert-player-v1",
};
const policy_knowledge = [_][*:0]const u8{
    "controller_tape",                                                       "observed_damage_weakness_only", "observed_damage_weakness_only",
    "observed_damage_weakness_only",                                         "walker_battle_rules_native_unit_records",
    "observed_damage_weakness_public_charge_guard",
    "observed_damage_weakness_public_charge_guard_declared_persona_ratings",
    "observed_damage_weakness_public_charge_guard_ailment_landings",
    "observed_damage_weakness_public_charge_guard_ailment_landings_buff_state_fixed_heal_amounts_owned_persona_skills",
    "visible_observation_observed_hits_scenario_boss_context",
};
fn finish(outcome_in: [*c]const u8, cause_in: [*c]const u8, code_in: c_int) noreturn {
    var outcome = outcome_in;
    var cause = cause_in;
    var code = code_in;
    const hash: u64 = if (session.launched != 0) observe("terminal", true) else 0;
    var party: [2048]u8 = @splat(0);
    if (session.launched != 0) _ = party_json(&party, party.len);
    // Every result names the table catalog it ran on, retail included; an
    // unavailable identity fails the run rather than defaulting to retail.
    var catalog: [32]u8 = undefined;
    const expanded = c.p4_game_table_catalog_identity(&catalog);
    if (expanded < 0) {
        outcome = "engine_error";
        cause = "table_catalog_identity_unavailable";
        code = 4;
    }
    // A declared mod set names the packages actually active; none fails the run.
    const mod_count: usize = if (session.mod_cohort != 0) c.p4_mods_count() else 0;
    if (session.mod_cohort != 0 and mod_count == 0) {
        outcome = "engine_error";
        cause = "mod_set_unavailable";
        code = 4;
    }
    var path: [2048]u8 = undefined;
    const n = snprintf(&path, path.len, "%s.tmp", session.result);
    if (n < 0 or @as(usize, @intCast(n)) >= path.len) {
        _ = fail("result path too long");
        exit(4);
    }
    const f = c.fopen(&path, "wbx");
    if (f == null) {
        _ = fail("cannot create exclusive result staging");
        exit(4);
    }
    const ordinary: u64 = session.admissions[0][0] +% session.admissions[0][1] +% session.admissions[0][2];
    const followup: u64 = session.admissions[1][0] +% session.admissions[1][1] +% session.admissions[1][2];
    const actions: u64 = session.action_count;
    _ = fprintf(f, "{\"schema\":1,\"entry_kind\":\"harness-script6\",\"trace_capability\":\"committed-events-and-scoped-state-rng\",\"outcome\":\"%s\",\"terminal_cause\":\"%s\",\"encounter_id\":%u,\"updates\":%u,\"counter_capability\":\"native-queue-ready-cost-target-v1\",\"observer_attached\":%u,", outcome, cause, @as(c_uint, session.encounter), @as(c_uint, session.updates), @as(c_uint, @intFromBool(session.observer_token != 0)));
    _ = c.fputs("\"table_catalog\":", f);
    if (expanded < 0) _ = c.fputs("null,", f) else {
        _ = fprintf(f, "{\"expanded\":%d,\"identity\":\"", @as(c_int, expanded));
        for (catalog) |byte| _ = fprintf(f, "%02x", @as(c_uint, byte));
        _ = c.fputs("\"},", f);
    }
    _ = c.fputs("\"mods\":", f);
    if (session.mod_cohort == 0) _ = c.fputs("null,", f) else {
        _ = fprintf(f, "{\"identity\":\"%s\",\"witness\":true,\"active\":[", &session.mod_identity);
        for (0..mod_count) |i| {
            const info = c.p4_mods_info(i);
            _ = c.fputs(if (i != 0) ",{\"id\":" else "{\"id\":", f);
            json_text(f, if (info != null) &info.*.id else "");
            _ = c.fputs(",\"version\":", f);
            json_text(f, if (info != null) &info.*.version else "");
            _ = c.fputc('}', f);
        }
        _ = c.fputs("]", f);
        if (session.mod_equipment_count != 0) {
            _ = c.fputs(",\"equipment\":[", f);
            for (session.mod_equipment[0..session.mod_equipment_count], 0..) |declared, i| {
                const separator: [*:0]const u8 = if (i != 0) "," else "";
                _ = fprintf(f, "%s{\"actor\":%u,\"slot\":%u,\"item\":%u}", separator, @as(c_uint, declared.actor), @as(c_uint, declared.slot), @as(c_uint, declared.item));
            }
            _ = c.fputc(']', f);
        }
        _ = c.fputs("},", f);
    }
    const prompt_capability: [*:0]const u8 = if (session.policy != 0) "periodic_cross_native_skill_offer_decline_v1" else "controller_tape";
    _ = fprintf(f, "\"result_prompt_capability\":\"%s\",\"result_prompts\":{\"skill_offers_declined\":%u},", prompt_capability, @as(c_uint, session.skill_offers_declined));
    const party_control: [*:0]const u8 = if (session.policy != 0) "native_direct_commands" else "source_tactics";
    _ = fprintf(f, "\"party_policy\":{\"id\":%u,\"name\":\"%s\",\"latency_updates\":%u,\"party_control\":\"%s\",\"knowledge\":\"%s\"", @as(c_uint, session.policy), policy_names[session.policy], @as(c_uint, session.latency), party_control, policy_knowledge[session.policy]);
    // Each plan is its own cohort; its delay= option, not latency_updates, times the walker's input.
    if (session.policy == WALK_POLICY) _ = fprintf(f, ",\"plan_sha256\":\"%s\"", &session.plan_hash);
    if (session.policy == AFFINITY_POLICY) _ = fprintf(f, ",\"ratings_sha256\":\"%s\"", &session.plan_hash);
    _ = c.fputs("},", f);
    _ = c.fputs("\"entry_observations\":{\"first_protagonist_command\":{\"boundary\":\"native_input_ready\",\"observed\":", f);
    if (session.first_command_seen != 0)
        _ = fprintf(f, "1,\"update\":%u,\"party\":[%s]}},", @as(c_uint, session.first_command_update), &session.first_command_party)
    else
        _ = c.fputs("0,\"update\":null,\"party\":null}},", f);
    _ = c.fputs("\"last_observed_protagonist_lethal_packet\":", f);
    if (session.has_protagonist_lethal != 0 and c.strcmp(outcome, "defeat") == 0 and
        datCalcGetHp(datUnitGetRecord(1)) == 0)
    {
        const lethal = &session.protagonist_lethal;
        _ = fprintf(f, "{\"update\":%u,\"skill_id\":%u,\"hp_delta_requested\":%d,\"added_status\":%u,\"result_flags\":%u,\"source\":", @as(c_uint, session.protagonist_lethal_update), @as(c_uint, lethal.skill_id), @as(c_int, lethal.requested_hp_delta), @as(c_uint, lethal.added_status), @as(c_uint, lethal.result_flags));
        actor_json(f, &lethal.source);
        _ = c.fputs("},", f);
    } else _ = c.fputs("null,", f);
    _ = fprintf(f, "\"turns\":{\"actor_turns\":%llu,\"followup_admissions\":%llu,\"admitted_opportunities\":%llu,\"actions\":%llu,\"rounds\":null,\"party_actions\":%llu,\"enemy_actions\":%llu,\"other_actions\":%llu,\"free_commands\":%llu,\"actor_turns_definition\":\"ordinary_queue_admissions_including_skips\",\"actions_definition\":\"ready_to_execution_attempts_excluding_persona_analyze\"},", @as(c_ulonglong, ordinary), @as(c_ulonglong, followup), @as(c_ulonglong, ordinary +% followup), @as(c_ulonglong, actions), @as(c_ulonglong, session.commands[0]), @as(c_ulonglong, session.commands[1]), @as(c_ulonglong, session.commands[2]), @as(c_ulonglong, session.free_commands));
    _ = fprintf(f, "\"resources\":{\"initial_party_boundary\":\"after_restore_before_constructor\",\"initial_party\":[%s],\"final_party\":[%s],\"hp_spent\":%llu,\"sp_spent\":%llu,\"items_consumed\":%llu,\"spend_definition\":\"native_party_skill_costs_and_actual_item_decrements\",\"hp_damage_requested\":%llu,\"hp_recovery_requested\":%llu,\"sp_drain_requested\":%llu,\"sp_recovery_requested\":%llu,\"cost_hp_adjustment\":%lld,\"cost_sp_adjustment\":%lld,\"target_hp_adjustment\":%lld,\"target_sp_adjustment\":%lld,\"recovery_scope\":\"committed_target_packets_not_direct_postbattle_gifts\",\"items_by_id\":[", &session.initial_party, &party, @as(c_ulonglong, session.hp_cost), @as(c_ulonglong, session.sp_cost), @as(c_ulonglong, session.item_cost), @as(c_ulonglong, session.hp_damage), @as(c_ulonglong, session.hp_recovery), @as(c_ulonglong, session.sp_drain), @as(c_ulonglong, session.sp_recovery), @as(c_longlong, session.hp_cost_adjustment), @as(c_longlong, session.sp_cost_adjustment), @as(c_longlong, session.hp_target_adjustment), @as(c_longlong, session.sp_target_adjustment));
    var first_item = true;
    for (&session.item_counts, 0..) |used, id| {
        if (used == 0) continue;
        const separator: [*:0]const u8 = if (first_item) "" else ",";
        _ = fprintf(f, "%s{\"id\":%u,\"used\":%llu}", separator, @as(c_uint, @intCast(id)), @as(c_ulonglong, used));
        first_item = false;
    }
    const mode: [*:0]const u8 = if (session.flags & 2 != 0) "captured" else "seeded";
    _ = fprintf(f, "]},\"rng\":{\"mode\":\"%s\",\"additive_seed\":%u,\"mt_seed\":%u,\"effect_seed\":%u,\"entry_hash\":\"%016llx\",\"final_hash\":\"%016llx\",\"draws\":%u},\"state_hash\":\"%016llx\"}\n", mode, @as(c_uint, session.seed), @as(c_uint, session.mt_seed), @as(c_uint, session.effect_seed), @as(c_ulonglong, session.entry_rng_hash), @as(c_ulonglong, hash), @as(c_uint, p4pc_rng_draws -% session.entry_draws), @as(c_ulonglong, session.last_hash));
    var failed = c.ferror(f) != 0;
    if (c.fclose(f) != 0) failed = true;
    if (session.trace != null and c.fclose(session.trace) != 0) failed = true;
    if (failed or c.rename(&path, session.result) != 0) {
        _ = fail("cannot publish complete result");
        exit(4);
    }
    _ = fprintf(cStderr(), "p4pc: battle-sim: %s after %u native updates (%s)\n", outcome, @as(c_uint, session.updates), cause);
    p4pc_exit(code);
}
fn boot_ready() bool {
    const sequence = gp(0xb1a8);
    if (!guest_range(sequence, 0x3c)) return false;
    const work = g32(sequence + 0x38);
    if (!guest_range(work, 8) or g32(work + 4) != 27 or gp(0xb414) == 0) return false;
    const logo = kwlnTaskGetTaskByName(g2h(0x005e57a8)) orelse return false;
    const logo_work = std.mem.readInt(u32, @as([*]const u8, @ptrCast(logo))[0x38..][0..4], builtin.cpu.arch.endian());
    return guest_range(logo_work, 4) and g32(logo_work) >= 4;
}

fn restore_source() [*c]const u8 {
    const image = c.p4_gport_alloc(session.save_size, 16);
    if (image == 0) return "entry_guest_allocation";
    if (session.save_size != 0) @memcpy(gbytes(image)[0..session.save_size], session.save[0..session.save_size]);
    if (datSaveVerifyChecksum(0, g2h(image +% 4), @as(i32, @bitCast(session.save_size)) -% 4) == 0) return "native_save_checksum";
    // A declared mod set sees this restore as the real load it stands for. The
    // native reset (0x001113b0) reports "reset"; the exact image is then staged
    // through the save transport like a card read without metadata (no sidecar:
    // an entry carries no mod blocks), and the native restore (0x0010e710)
    // proves, commits and announces it.
    datInitDefaultGameState();
    if (session.mod_cohort != 0) {
        var reason: [512]u8 = undefined;
        if (c.p4_mod_save_transport_loaded_untagged(session.save, session.save_size, "battle-sim-entry", &reason, reason.len) != 0) {
            _ = fprintf(cStderr(), "p4pc: battle-sim: mod-save load staging: %s\n", &reason);
            return "mod_save_load_staging";
        }
    }
    datSaveRestoreImage(0, g2h(image +% 4), @as(i32, @bitCast(session.save_size)) -% 4);
    return null;
}
fn current_context(context: *[5]u32) bool {
    const field = gp(0x9db0);
    if (!guest_range(field, 8)) return false;
    context[0] = @as(u16, @bitCast(datGetDateIndex()));
    context[1] = g8(0x797b7c);
    context[2] = g32(field);
    context[3] = g32(field + 4);
    context[4] = @bitCast(FldStepIndex());
    return true;
}
fn field_interactive() bool {
    // FldRootCreate00155e10 publishes this genuine task. FldRootUpdate
    // state13 follows load, both startup scripts, fade and trigger setup.
    // Scene-load flag2/3 alone is earlier and is not sufficient.
    const root = g32(0x7d2510);
    if (!guest_range(root, 0x3c)) return false;
    const work = g32(root + 0x38);
    return guest_range(work, 0xf4) and g32(work) == 13 and g32(work + 8) == 0;
}
fn prepare_before_update() void {
    if (session.prepare_state == 0) {
        if (session.boot_updates >= session.boot_limit) prepare_finish("preparation_error", "bootstrap_update_limit", 4);
        if (!boot_ready()) return;
        const reason = restore_source();
        if (reason != null) prepare_finish("preparation_error", reason, 4);
        session.context_before[0] = @as(u16, @bitCast(datGetDateIndex()));
        session.context_before[1] = g8(0x797b7c);
        session.context_before[2] = g32(0x79b698);
        session.context_before[3] = g32(0x79b69c);
        session.context_before[4] = g32(0x79b6a0);
        session.prepare_state = 1;
        const assertions = le32(session.preparation.header + 88);
        for (0..5) |i| {
            if ((assertions & (@as(u32, 1) << @intCast(i))) != 0 and session.context_before[i] != le32(session.preparation.header + 92 + i * 4))
                prepare_finish("preparation_error", "source_context_assertion_mismatch", 1);
        }
        const logo = kwlnTaskGetTaskByName(g2h(0x005e57a8));
        _ = kwlnTaskDestroyWithHierarchy(logo);
        // The actual clndSceneUpdate00123e80 post-LOAD branch, without
        // fabricating its owner task or claiming an active-VM checkpoint.
        gpp(0xb1f0).* = 1;
        calendarSyncFromDate();
        calendarSetMode(1);
        if (datGetPendingDateIndex() == 9 and datGetPendingTimeSlot() == 5)
            prepare_finish("preparation_error", "source_requires_newgame_not_battle_ready_field", 1);
        const packet = c.p4_gport_alloc(28, 16);
        if (packet == 0) prepare_finish("preparation_error", "field_resume_packet_allocation", 4);
        @memset(gbytes(packet)[0..28], 0);
        if (session.context_before[4] != 0) {
            g32p(packet).* = session.context_before[4];
            g32p(packet + 4).* = 0xff;
            sequenceRequest(0xb, packet, 8, 2);
        } else {
            if (session.context_before[2] > std.math.maxInt(i16) or session.context_before[3] > std.math.maxInt(i16))
                prepare_finish("preparation_error", "source_field_ids_outside_native_resume_packet", 1);
            g16p(packet).* = @truncate(session.context_before[2]);
            g16p(packet + 2).* = @truncate(session.context_before[3]);
            g16p(packet + 4).* = 0xff;
            sequenceRequest(9, packet, 28, 0);
        }
        return;
    }
    if (gp(0xb3ac) != 0) prepare_finish("preparation_error", "source_requires_live_battle_entry", 1);
    if (!field_interactive()) {
        if (session.updates >= session.limit) prepare_finish("preparation_error", "field_resume_update_limit", 4);
        return;
    }
    if (!current_context(&session.context_after)) prepare_finish("preparation_error", "native_field_context_unavailable", 4);
    session.prepare_state = 2;
    if (!std.mem.eql(u32, &session.context_before, &session.context_after))
        prepare_finish("preparation_error", "native_field_resume_changed_source_context", 1);
    // Growth has an explicit independent seed boundary after real field
    // readiness; asynchronous load duration does not select stat rolls.
    if (!restore_rng() or !rng_hash(&session.prepare_rng_entry))
        prepare_finish("preparation_error", "preparation_rng_seed_or_snapshot", 4);
    session.prepare_rng_valid = 1;
    session.entry_draws = p4pc_rng_draws;
    const reason = c.p4_battle_prepare_apply(&session.preparation);
    if (reason != null) prepare_finish("preparation_error", reason, 1);
    if (!current_context(&session.context_after) or
        !std.mem.eql(u32, &session.context_before, &session.context_after))
        prepare_finish("preparation_error", "native_preparation_changed_source_context", 4);
    const size_slot = c.p4_gport_alloc(4, 4);
    if (size_slot == 0) prepare_finish("preparation_error", "native_save_size_allocation", 4);
    const image = datSaveBuildImage(0, @ptrCast(@alignCast(g2h(size_slot))), 0);
    session.prepared_size = g32(size_slot);
    if (image == null or !guest_range(h2g(image), session.prepared_size) or
        c.p4_battle_save_valid(@ptrCast(image), session.prepared_size) == 0)
        prepare_finish("preparation_error", "native_save_build_export_invalid", 4);
    c.p4_content_digest(image, session.prepared_size, &session.prepared_hash);
    if (!rng_hash(&session.prepare_rng_exit)) prepare_finish("preparation_error", "native_preparation_exit_rng_snapshot", 4);
    const output = c.fopen(session.prepared_image, "wbx");
    if (output == null) prepare_finish("preparation_error", "cannot_create_exclusive_prepared_image", 5);
    const written = c.fwrite(image, 1, session.prepared_size, output) == session.prepared_size and c.ferror(output) == 0;
    const close_error = c.fclose(output);
    if (!written or close_error != 0) prepare_finish("preparation_error", "cannot_finish_native_prepared_image", 5);
    session.prepare_complete = 1;
    prepare_finish("prepared", "native_save_build_image_complete", 0);
}
// Retail addresses of the post-battle skill-offer prompt's owners.
const MESSAGE_WINDOW_CURRENT: u32 = 0x00882f60; // NmMesWindowRecord returned by nmMesWindowCurrent
const MESSAGE_WINDOW_SLOTS: u32 = 0x00881808; // ItfMesWindowSlot[], 0x20 stride, window at +0
// The skill-replace task (0011f5a0) is created as "shdPersonaSkillSelectSeq";
// kwlnTaskCreate keeps 23 characters but sums all 24, so kwlnTaskGetTaskByName
// finds it under neither name. Walk the three scheduler lists (sdkTask.c) and
// match the stored 0x18-byte name instead.
fn skill_select_task() u32 {
    const stored: *const [0x18]u8 = "shdPersonaSkillSelectSe" ++ "\x00";
    const heads = [3]u16{ 0xb9ec, 0xba04, 0xb9f8 };
    for (heads) |head| {
        var node = gp(head);
        var n: u32 = 0;
        while (node != 0) : (n += 1) {
            if (n == 4096 or !guest_range(node, 0x40)) {
                session.observation_error = "result_task_list_invalid";
                return 0;
            }
            if (std.mem.eql(u8, gbytes(node)[0..0x18], stored)) return node;
            node = g32(node + 0x3c); // listNext
        }
    }
    return 0;
}
// Battle result phase 16 waits for the result task, which waits for every
// persona level-up. With eight skills, a level-up asks which skill to forget;
// the named policies' periodic Cross would re-select forever (the Yes/No
// defaults to No) or, racing the list, forget a known skill. While that task
// reads input (states 0-3 and 6) the session owns input and declines the
// offer through the real UI. Returns true when it owns input this update.
fn result_skill_prompt() bool {
    const battle = gp(0xb3ac);
    if (battle == 0 or !guest_range(battle, 0x300) or g32(battle + 0x2fc) != 16) return false;
    const task = skill_select_task();
    if (task == 0) return false;
    const work: u32 = if (guest_range(task, 0x3c)) g32(task + 0x38) else 0;
    const parent: u32 = if (work != 0 and guest_range(work, 0x10)) g32(work + 0xc) else 0;
    const panel: u32 = if (parent != 0 and guest_range(parent, 0x3c)) g32(parent + 0x38) else 0;
    if (panel == 0 or !guest_range(panel, 0x538)) {
        session.observation_error = "result_skill_prompt_layout";
        return true;
    }
    const state: i32 = @bitCast(g32(work + 8));
    if (state < 0 or (state > 3 and state != 6)) return false;
    if (session.updates < session.prompt_ready) return true;
    var prompt = std.mem.zeroes(c.P4BattleSkillPrompt);
    prompt.state = state;
    prompt.list_active = @intFromBool((g32(panel + 0x534) & 0x800) != 0);
    prompt.list_cursor = @as(i8, @bitCast(g8(panel + 0x88))); // ShdSkillState at +0x84
    prompt.pending = g16(panel + 0x8c + 0x224); // ShdSkillPool.pendingCount
    prompt.replacement_pending = @intFromBool(g16(work + 6) == g16(panel + 0x8c + 0x66)); // pending[0].id
    const handle: i32 = @bitCast(g32(MESSAGE_WINDOW_CURRENT));
    const window_state: i8 = @bitCast(g8(MESSAGE_WINDOW_CURRENT + 4)); // 1 text, 2 choice/close
    if ((window_state == 1 or window_state == 2) and handle >= 0 and handle < 0x100) {
        const window = g32(MESSAGE_WINDOW_SLOTS + @as(u32, @bitCast(handle)) * 0x20);
        if (window != 0 and guest_range(window, 0x50)) {
            prompt.row_waiting = @intFromBool(@as(i8, @bitCast(g8(window + 0x24))) == 1); // ItfMesRow.state
            prompt.choice_active = @intFromBool(@as(i16, @bitCast(g16(window + 0x48))) == 2 and @as(i16, @bitCast(g16(window + 0x4e))) == 2);
            prompt.choice_cursor = @as(i16, @bitCast(g16(window + 0x4a)));
        }
    }
    const bits = c.p4_battle_skill_prompt_buttons(&prompt);
    if (bits == 0) return true;
    if (state == 3 and prompt.choice_active != 0 and prompt.replacement_pending != 0 and bits == c.P4PC_PAD_CROSS)
        session.skill_offers_declined +%= 1;
    c.p4pc_pad_inject(bits, 1);
    session.prompt_ready = session.updates +% 4; // release so the next press is a new edge
    return true;
}
export fn p4_battle_sim_before_update() void {
    if (c.p4_battle_live_active != 0) {
        c.p4_battle_live_before_update();
        return;
    }
    if (p4_battle_sim_active == 0) return;
    if (session.preparing != 0) {
        prepare_before_update();
        return;
    }
    if (session.launched == 0) {
        if (session.boot_updates >= session.boot_limit) finish("engine_error", "bootstrap_update_limit", 4);
        if (!boot_ready()) return;
        const reason = restore_source();
        if (reason != null) finish("engine_error", reason, 4);
        if (session.mod_equipment_count != 0) {
            // After the ENTRY image's load (which resets the side state) and before the party and encounter are
            // built: the item service checks each row's kind/slot and the unit's compatibility bit.
            const items = c.p4_game_tables_items_active();
            if (items == null) finish("engine_error", "mod_equipment_without_item_table", 4);
            for (session.mod_equipment[0..session.mod_equipment_count]) |declared| {
                if (c.p4_game_tables_items_equipment_set(items, declared.actor, declared.slot, declared.item) != c.P4_TABLE_OK)
                    finish("engine_error", "mod_equipment_refused", 4);
            }
        }
        if (session.policy != 0) {
            for (0..3) |i| {
                const id = datGetPartyId(@intCast(i));
                if (id >= 2 and id <= 10) datSetUnitAiTactic(id, 4);
            }
        }
        if (session.flags & 4 != 0) {
            const ids = [4]i32{ 1, datGetPartyId(0), datGetPartyId(1), datGetPartyId(2) };
            for (ids) |id32| {
                if (id32 >= 1 and id32 <= 10) {
                    const id: i16 = @intCast(id32);
                    const unit = datUnitGetRecord(id);
                    if (datCalcGetHp(unit) != 0) {
                        datSetUnitHp(id, @bitCast(datCalcMaxHpById(id)));
                        datSetUnitSp(id, @bitCast(datCalcMaxSpById(id)));
                    }
                }
            }
        }
        if (party_json(&session.initial_party, session.initial_party.len) == 0) finish("engine_error", "entry_party_observation", 4);
        const logo = kwlnTaskGetTaskByName(g2h(0x005e57a8));
        _ = kwlnTaskDestroyWithHierarchy(logo);
        if (!restore_rng()) finish("engine_error", "entry_rng_state", 4);
        session.entry_draws = p4pc_rng_draws;
        session.launched = 1;
        var observer = std.mem.zeroes(c.P4BattleObserver);
        observer.admission = &admission_event;
        observer.cost = &cost_event;
        observer.hit = &hit_event;
        observer.command = &command_event;
        session.observer_token = c.p4_battle_observer_add(&observer);
        if (session.observer_token == 0) finish("engine_error", "native_observer_registration", 4);
        // The walk policy sets Direct Commands like the named policies but owns input through the walker.
        if (c.p4_battle_policy_begin(if (session.policy == WALK_POLICY) 0 else session.policy, session.latency) == 0)
            finish("policy_error", c.p4_battle_policy_error(), 4);
        c.p4pc_pad_autopress_set_clock(&p4_battle_sim_update_count);
        if (observe("entry", true) == 0) finish("engine_error", "entry_observation", 4);
        if (c.p4_battle_encounter_start(session.encounter) == 0) finish("engine_error", "invalid_encounter", 4);
    }
    if (session.updates >= session.limit) finish("update_limit", "completed_native_update_limit", 3);
    if (session.policy != 0 and result_skill_prompt()) return;
    if (session.walk != null) {
        walk_plan_update();
        if (session.walk != null) return;
    }
    c.p4_battle_policy_before_update();
    if (session.policy == 0 and session.tape_index < session.tape_count) {
        const record = session.tape + session.tape_index * 12;
        if (le32(record) == session.updates) {
            c.p4pc_pad_inject(le32(record + 4), le32(record + 8));
            session.tape_index += 1;
        }
    }
}
export fn p4_battle_sim_after_update(completed: c_int) void {
    if (c.p4_battle_live_active != 0) {
        c.p4_battle_live_after_update(completed);
        return;
    }
    if (p4_battle_sim_active == 0 or completed == 0) return;
    if (session.preparing != 0) {
        if (session.prepare_state != 0) session.updates +%= 1 else session.boot_updates +%= 1;
        return;
    }
    if (session.launched == 0) {
        session.boot_updates +%= 1;
        return;
    }
    session.updates +%= 1;
    const battle = gp(0xb3ac);
    if (battle != 0) {
        session.seen_battle = 1;
        if (observe("update", false) == 0) finish("engine_error", "invalid_runtime_observation", 4);
        if (session.observation_error != null) finish("engine_error", session.observation_error, 4);
        if (c.p4_battle_policy_error() != null) finish("policy_error", c.p4_battle_policy_error(), 4);
        const terminal: c_uint = g16(battle +% 0x1c);
        if (terminal != 2) session.defeat_cause = null // A real native rescue may resume play.
        else if (session.defeat_cause == null) {
            session.defeat_cause = if ((g32(battle +% 0x10) & 0x10000000) != 0)
                "native_forced_defeat_flag"
            else if (datCalcIsDead(datUnitGetRecord(1), 0) != 0)
                "protagonist_terminal_ko"
            else
                "native_party_terminal_defeat";
        }
        // Final phase17 after native result/chain/revival decisions. Wait for
        // the actual exit owner to drain record/sound work and clear ACTIVE.
        // Never call mutating btlCheckBattleEnd to speculate on an outcome.
        if (g32(battle +% 0x2fc) == 17 and (g32(battle +% 0xc) & 1) == 0) {
            const end: c_uint = g16(battle +% 0x1c);
            if (end == 1) finish("win", "native_battle_exit_victory", 0);
            if (end == 2) finish("defeat", if (session.defeat_cause != null) session.defeat_cause else "native_battle_exit_defeat", 0);
            if (end == 3 and g16(battle +% 0xc5e) != 0) finish("escape", "native_committed_party_group_escape", 0);
            if (end == 3) finish("scripted_conclusion", "native_battle_exit_no_party", 0);
            finish("engine_error", "native_exit_without_terminal_state", 4);
        }
    } else if (session.seen_battle != 0) finish("engine_error", "battle_teardown_before_terminal_capture", 4);
    if (session.action_count >= session.action_limit)
        finish("action_limit", "completed_native_update_action_bound", 3);
}
