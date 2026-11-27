//! Natural walk worker: observation only; the game owns its VM, input and RNG.
//! C ABI, trace/result wire formats, fences and exit codes.
const std = @import("std");
const builtin = @import("builtin");
pub const panic = @import("panic.zig").panic;

/// C: `#if defined(__linux__) && !defined(__ANDROID__)` -> P4_BATTLE_FORK.
const battle_fork_enabled = builtin.os.tag == .linux and !builtin.abi.isAndroid();
/// C: p4pc/guest.h defines P4_GUEST64 when `__SIZEOF_POINTER__ == 8`.
const guest64 = @sizeOf(usize) == 8;

/// POSIX declarations (dirent, errno, fcntl, signal, sys/stat, sys/types, unistd under
/// _POSIX_C_SOURCE 200809L), translated from bindings/live.h by the build; empty off-target.
const posix = if (battle_fork_enabled) @import("c_bindings") else struct {};

// ---- libc (stdio.h, stdlib.h, string.h) -------------------------------------------------------
const FILE = opaque {};
extern "c" fn fopen(path: [*c]const u8, mode: [*c]const u8) ?*FILE;
extern "c" fn fdopen(fd: c_int, mode: [*c]const u8) ?*FILE;
extern "c" fn fclose(file: ?*FILE) c_int;
extern "c" fn fread(data: ?*anyopaque, size: usize, count: usize, file: ?*FILE) usize;
extern "c" fn fwrite(data: ?*const anyopaque, size: usize, count: usize, file: ?*FILE) usize;
extern "c" fn fgetc(file: ?*FILE) c_int;
extern "c" fn fgets(buffer: [*c]u8, size: c_int, file: ?*FILE) [*c]u8;
extern "c" fn ferror(file: ?*FILE) c_int;
extern "c" fn fflush(file: ?*FILE) c_int;
extern "c" fn ftell(file: ?*FILE) c_long;
extern "c" fn fprintf(file: ?*FILE, format: [*:0]const u8, ...) c_int;
extern "c" fn fputs(text: [*c]const u8, file: ?*FILE) c_int;
extern "c" fn fputc(ch: c_int, file: ?*FILE) c_int;
extern "c" fn snprintf(buffer: [*c]u8, size: usize, format: [*:0]const u8, ...) c_int;
extern "c" fn rename(old: [*c]const u8, new: [*c]const u8) c_int;
extern "c" fn getenv(key: [*c]const u8) [*c]const u8;
extern "c" fn malloc(size: usize) ?*anyopaque;
extern "c" fn free(ptr: ?*anyopaque) void;
extern "c" fn exit(code: c_int) noreturn;
extern "c" fn strcmp(a: [*c]const u8, b: [*c]const u8) c_int;
extern "c" fn strncmp(a: [*c]const u8, b: [*c]const u8, n: usize) c_int;
extern "c" fn strlen(text: [*c]const u8) usize;
extern "c" fn strcpy(dst: [*c]u8, src: [*c]const u8) [*c]u8;
extern "c" fn strchr(text: [*c]const u8, ch: c_int) [*c]u8;
extern "c" fn strspn(text: [*c]const u8, accept: [*c]const u8) usize;
extern "c" fn strcspn(text: [*c]const u8, reject: [*c]const u8) usize;
extern "c" fn _exit(code: c_int) noreturn;
// glibc's `signal` (BSD semantics); the C file's _POSIX_C_SOURCE spelling resolves to __sysv_signal. Both install
// SIG_IGN identically, which is the only disposition set here. Disposition sentinels such as address 1 are not
// callable/aligned function addresses (in particular on AArch64); retain the function-pointer C ABI with alignment 1.
extern "c" fn signal(sig: c_int, handler: ?*align(1) const fn (c_int) callconv(.c) void) ?*align(1) const fn (c_int) callconv(.c) void;
const EOF: c_int = -1;

// `stderr`/`stdout` are macros in C; these are the objects they name on each supported libc.
extern "c" fn __acrt_iob_func(index: c_uint) ?*FILE; // Windows UCRT: stdin/stdout/stderr = __acrt_iob_func(0/1/2)
extern "c" var __stderrp: ?*FILE; // Darwin
extern "c" var stderr: ?*FILE; // glibc, musl, bionic
extern "c" var stdout: ?*FILE;
fn stderrFile() ?*FILE {
    return switch (builtin.os.tag) {
        .windows => __acrt_iob_func(2),
        .macos, .ios, .tvos, .watchos, .visionos => __stderrp,
        else => stderr,
    };
}

// ---- guest memory (p4pc/guest.h, p4pc/gmem.h) -------------------------------------------------
const P4_GUEST_SIZE: u32 = 0x04000000;
extern var p4_guest_base: [*c]u8;
extern fn p4_g2h_slow(a: u32) [*c]u8;
/// P4_G2H: identity on a 32-bit host; base + guest (slow path for windows/sentinels) on 64-bit.
inline fn g2h(a: u32) [*c]u8 {
    if (comptime guest64) {
        if (a -% 1 < P4_GUEST_SIZE - 1) return p4_guest_base + a;
        return p4_g2h_slow(a);
    }
    return @ptrFromInt(a);
}
fn g8(a: u32) u8 {
    const p: [*c]const volatile u8 = g2h(a);
    return p.*;
}
fn g16(a: u32) u16 {
    const p: [*c]align(1) const volatile u16 = @ptrCast(g2h(a));
    return p.*;
}
fn g32(a: u32) u32 {
    const p: [*c]align(1) const volatile u32 = @ptrCast(g2h(a));
    return p.*;
}
/// GP(off) = P4_GPDATA(uint32_t, off): gp 0x007690F0 plus the sign-extended 16-bit offset.
fn gp(off: u16) u32 {
    return g32(0x007690F0 +% @as(u32, @bitCast(@as(i32, @as(i16, @bitCast(off))))));
}

// ---- native engine and platform -----------------------------------------------------------------
extern var p4pc_rng_draws: u32;
extern var p4_battle_sim_active: c_int;
extern var p4_battle_sim_renderless: c_int;
extern fn p4pc_pad_walk_status() c_int;
extern fn datGetDateIndex() i16;
extern fn FldStepIndex() i32;
extern fn datGetPartyId(slot: i32) i16;
extern fn datUnitGetRecord(id: i16) ?*anyopaque;
extern fn datCalcGetHp(unit: ?*anyopaque) u16;
extern fn datCalcGetSp(unit: ?*anyopaque) u16;
extern fn datCalcMaxHpById(id: i16) u16;
extern fn datCalcMaxSpById(id: i16) u16;
extern fn datCalcIsDead(unit: ?*anyopaque, hp_delta: i32) u32;
extern fn p4pc_flip_count() u32;
extern fn p4pc_notify_normal_exit() void;
extern fn p4_content_digest(data: ?*const anyopaque, size: usize, hex: [*c]u8) void;
extern fn p4_mod_fidelity_check_start() void;
extern fn p4_battle_live_decode(bytes: [*c]const u8, length: usize, out: *Entry) c_int;
extern fn p4_battle_observer_add(observer: *const Observer) u32;
extern fn p4pc_pad_set_battle_turn_hook(hook: ?*const fn (c_uint, [*c]u8, usize) callconv(.c) c_int) void;
extern fn p4pc_lockstep_flush() c_int;
extern fn p4pc_lockstep_fork_reopen(dir: [*c]const u8) c_int;
extern fn vfs_fork_private() c_int;

const P4WALK_DONE: c_int = 1;

/// p4pc_exit (p4pc/crt.h, static inline): code 0 notifies the normal-exit observer first.
fn p4pcExit(code: c_int) noreturn {
    if (code == 0) p4pc_notify_normal_exit();
    exit(code);
}

// ---- p4pc/battle_live.h, p4pc/battle_events.h ---------------------------------------------------
const Entry = extern struct {
    flags: u32, mode: u32, encounter: u32, occurrence: u32,
    route_limit: u32, fight_limit: u32, action_limit: u32, completion: u32,
    endpoint_mask: u32, field: u32, room: u32, date: u32, step: u32,
};
const Actor = extern struct {
    action_uid: u64,
    unit_uid: u32,
    native_id: u16,
    side: u8,
    present: u8,
};
const Admission = extern struct {
    actor: Actor,
    native_action_count: u32,
    next_state: u16,
    queue_kind: u8,
    reserved: u8,
};
const Cost = extern struct {
    actor: Actor,
    requested_cost: u32,
    skill_id: u16,
    item_id: u16,
    hp_before: u16, hp_after: u16,
    sp_before: u16, sp_after: u16,
    item_count_before: u16, item_count_after: u16,
    kind: u8,
    reserved: [3]u8,
};
const Hit = extern struct {
    source: Actor, target: Actor,
    requested_hp_delta: i32, requested_sp_delta: i32,
    requested_money_delta: i32,
    added_status: u32, removed_status: u32,
    buff_changes: u32,
    status_before: u32, status_after: u32,
    skill_id: u16, action_kind: u16,
    hp_before: u16, hp_after: u16,
    sp_before: u16, sp_after: u16,
    target_flags: u16, result_flags: u16,
    effect: u16,
    resources_applied: u8,
    statuses_applied: u8,
};
const Command = extern struct {
    actor: Actor,
    action_kind: u16, skill_id: u16, item_id: u16, target_count: u16, native_state: u16,
    free_action: u8,
    reserved: u8,
};
const Request = extern struct {
    stage: u32,
    encounter_id: u32,
    guest_object: u32,
    value: u32,
};
const Observer = extern struct {
    context: ?*anyopaque,
    admission: ?*const fn (*const Admission, ?*anyopaque) callconv(.c) void,
    cost: ?*const fn (*const Cost, ?*anyopaque) callconv(.c) void,
    hit: ?*const fn (*const Hit, ?*anyopaque) callconv(.c) void,
    command: ?*const fn (*const Command, ?*anyopaque) callconv(.c) void,
    request: ?*const fn (*const Request, ?*anyopaque) callconv(.c) void,
};
const P4_BATTLE_QUEUE_FOLLOWUP: u8 = 2;
const P4_BATTLE_COST_HP: u8 = 1;
const P4_BATTLE_COST_SP: u8 = 2;
const P4_BATTLE_COST_ITEM: u8 = 3;
const P4_BATTLE_SCRIPT_REQUEST_BEGIN: u32 = 1;
const P4_BATTLE_SCRIPT_REQUEST_IDENTIFIED: u32 = 2;
const P4_BATTLE_FIELD_REQUEST_BEGIN: u32 = 3;
const P4_BATTLE_TASK_BEGIN: u32 = 4;
const P4_BATTLE_TASK_CREATED: u32 = 5;
const P4_BATTLE_SCRIPT_GROUP_CREATED: u32 = 6;
const P4_BATTLE_FIELD_GROUP_CREATED: u32 = 7;
const P4_BATTLE_COLLISION_READY: u32 = 8;

// ---- module state -------------------------------------------------------------------------------
const RAM_START: u32 = 0x00100000;
const RAM_BYTES: u32 = 0x01f00000;
const GAME_FRAME: u32 = 0x00764290;

const LiveUnit = struct {
    address: u32 = 0, record: u32 = 0, uid: u32 = 0, status: u32 = 0,
    id: u16 = 0, record_id: u16 = 0, hp: u16 = 0, sp: u16 = 0,
    genus: u8 = 0,
};
const Units = [2][12]LiveUnit;

export var p4_battle_live_active: c_int = 0;
var live: struct {
    entry: Entry = std.mem.zeroes(Entry),
    trace: ?*FILE = null,
    result: [*c]const u8 = null,
    ram_path: [2048]u8 = @splat(0),
    ram_sha: [65]u8 = @splat(0),
    ram: ?[*]u8 = null,
    observer: u32 = 0, updates: u32 = 0, fight_updates: u32 = 0, requests: u32 = 0, matching: u32 = 0,
    pending_flip: u32 = 0, pending_frame: u32 = 0, pending_draws: u32 = 0, request_flip: u32 = 0, request_frame: u32 = 0,
    request_draws: u32 = 0, task_setup: u32 = 0, battle: u32 = 0, actions: u32 = 0, field_task: u32 = 0,
    field_actor: u32 = 0, field_group: u32 = 0, script_group: u32 = 0, unbound_requests: u32 = 0,
    admissions: [2]u64 = .{ 0, 0 }, costs_hp: u64 = 0, costs_sp: u64 = 0, items: u64 = 0,
    item_counts: [1536]u64 = @splat(0),
    lethal: Hit = std.mem.zeroes(Hit),
    lethal_update: u32 = 0,
    lethal_seen: bool = false, terminal_lethal: bool = false, first_command: bool = false,
    ready_actor: u32 = 0,
    terminal: u32 = 0, terminal_update: u32 = 0, terminal_flip: u32 = 0, terminal_frame: u32 = 0, terminal_draws: u32 = 0,
    terminal_units: Units = @splat(@splat(.{})),
    terminal_counts: [2]c_uint = .{ 0, 0 },
    pending: bool = false, selected: bool = false, task_begun: bool = false, bound: bool = false,
    seen: bool = false, terminal_seen: bool = false, published: bool = false,
    qualification: bool = false,
    update_flip: u32 = 0,
    terminal_presentation: [*c]const u8 = null,
    failure: [*c]const u8 = null, battle_cause: [*c]const u8 = null, battle_outcome: [*c]const u8 = null,
} = .{};

// Discovery is bounded and records only supported native request paths.
// A constructor without an exact pending request is never guessed.
var discovery: struct {
    occurrences: [3][65536]u16 = @splat(@splat(0)),
    records: u32 = 0, ordinal: u32 = 0, mode: u32 = 0, id: u32 = 0, occurrence: u32 = 0,
    group: u32 = 0, field_task: u32 = 0, actor: u32 = 0, setup: u32 = 0,
    battle: u32 = 0, fights: u32 = 0, commands: u32 = 0, fight_updates: u32 = 0, actions: u32 = 0,
    pending: bool = false, identified: bool = false, task_begun: bool = false, active: bool = false, first_command: bool = false,
} = .{};

// ---- helpers ------------------------------------------------------------------------------------
fn comma(index: anytype) [*:0]const u8 {
    return if (index != 0) "," else "";
}
fn jsonBool(value: bool) [*:0]const u8 {
    return if (value) "true" else "false";
}
fn cs(text: [*c]const u8) [*c]const u8 {
    return text;
}

fn range(p: u32, bytes: u32) bool {
    return p >= RAM_START and p <= 0x02000000 and bytes <= 0x02000000 - p;
}
fn setupRange(p: u32) bool {
    if (comptime guest64) return p >= RAM_START and p <= P4_GUEST_SIZE - 0x3c;
    // The original constructor receives a native stack address on 32-bit.
    return p >= RAM_START and p <= std.math.maxInt(u32) - 0x3c;
}
fn exactEnv(key: [*:0]const u8, value: [*:0]const u8) bool {
    const actual = getenv(key);
    return actual != null and strcmp(actual, value) == 0;
}
fn nonemptyEnv(key: [*:0]const u8) bool {
    const value = getenv(key);
    return value != null and value[0] != 0;
}
fn bad(reason: [*:0]const u8) c_int {
    _ = fprintf(stderrFile(), "p4pc: battle-live: %s\n", reason);
    return -1;
}
fn validEnvironment(entry: *const Entry) bool {
    const forbidden = [_][*:0]const u8{
        "P4PC_MOD_WITNESS",       "P4PC_START_BATTLE",   "P4PC_BATTLE_DRIVER", "P4PC_AUTOPRESS",
        "P4PC_AUTOPRESS_NEWGAME", "P4PC_CAPTURE_FRAMES", "P4PC_CAPTURE_STATE",
        "P4PC_START_MENU",        "P4PC_MENU_SCRIPT",    "P4PC_TRAP_CONTINUE", "P4PC_OVERRIDE_DIR",
    };
    for (forbidden) |key| if (getenv(key) != null) return false;
    if (!nonemptyEnv("P4PC_SAVE_DIR") or !nonemptyEnv("P4PC_CONFIG") or !nonemptyEnv("P4PC_LOCKSTEP_DIR") or
        !exactEnv("P4PC_INPUT", "script") or !exactEnv("P4PC_MODS", "0") or
        !exactEnv("P4PC_PACKS", "0") or !exactEnv("P4PC_VIRTUAL_CLOCK", "1") or
        !exactEnv("P4PC_TURBO", "2")) return false;
    const replay = getenv("P4PC_LOCKSTEP_REPLAY") != null;
    if (entry.mode == 3 and replay) return false;
    if (replay) {
        if (!nonemptyEnv("P4PC_LOCKSTEP_REPLAY")) return false;
        if (getenv("P4PC_WALK_SCRIPT") != null or getenv("P4PC_AUTOPRESS_LOAD") != null) return false;
        if (entry.completion == 2 and !nonemptyEnv("P4PC_LOCKSTEP_STOP")) return false;
    } else {
        const walk = getenv("P4PC_WALK_SCRIPT");
        if (walk == null or walk[0] == 0 or !exactEnv("P4PC_AUTOPRESS_LOAD", "0") or
            !exactEnv("P4PC_AUTOPRESS_FRAMES", "1")) return false;
        if ((getenv("P4PC_WALK_GAME") != null) == ((entry.flags & 2) != 0)) return false;
    }
    return true;
}
fn snapshot() void {
    const ram = live.ram.?;
    @memcpy(ram[0..RAM_BYTES], g2h(RAM_START)[0..RAM_BYTES]);
    live.pending_flip = p4pc_flip_count();
    live.pending_frame = g32(GAME_FRAME);
    live.pending_draws = p4pc_rng_draws;
    live.pending = true;
}
fn rngWord(address: u32, captured: bool) u32 {
    if (!captured) return g32(address);
    const p = live.ram.? + (address -% RAM_START);
    return std.mem.readInt(u32, p[0..4], .little);
}
fn traceRng(kind: [*:0]const u8, captured: bool) bool {
    const active = rngWord(0x00764708, captured);
    if (active > 0x10000 or !range(0x008872e0 + active, 16)) return false;
    const slot = 0x008872e0 + active;
    const base = rngWord(slot, captured);
    const front = rngWord(slot + 4, captured);
    const back = rngWord(slot + 8, captured);
    const end = rngWord(slot + 12, captured);
    if (!range(base, 124) or end != base +% 124 or front < base or front >= end or
        back < base or back >= end or ((front - base) & 3) != 0 or ((back - base) & 3) != 0) return false;
    _ = fprintf(live.trace, "{\"kind\":\"%s\",\"flip\":%u,\"game_frame\":%u,\"additive_draws\":%u," ++
        "\"active_slot_offset\":%u,\"front_byte_offset\":%u,\"back_byte_offset\":%u,\"additive\":[",
        kind, if (captured) live.pending_flip else p4pc_flip_count(),
        if (captured) live.pending_frame else g32(GAME_FRAME),
        if (captured) live.pending_draws else p4pc_rng_draws, active, front - base, back - base);
    var i: u32 = 0;
    while (i < 31) : (i += 1) _ = fprintf(live.trace, "%s%u", comma(i), rngWord(base +% 4 * i, captured));
    _ = fputs("],\"mt\":[", live.trace);
    i = 0;
    while (i < 624) : (i += 1) _ = fprintf(live.trace, "%s%u", comma(i), rngWord(0x00885a90 + 4 * i, captured));
    _ = fprintf(live.trace, "],\"mt_index\":%u,\"effect\":[", rngWord(0x00763b6c, captured));
    i = 0;
    while (i < 4) : (i += 1) _ = fprintf(live.trace, "%s%u", comma(i), rngWord(0x00922de0 + 4 * i, captured));
    _ = fputs("]}\n", live.trace);
    return ferror(live.trace) == 0;
}
fn publishSnapshot(mode: u32, encounter: u32, object: u32, value: u32) void {
    if (!live.pending) {
        live.failure = "request_snapshot_missing";
        return;
    }
    p4_content_digest(live.ram, RAM_BYTES, &live.ram_sha);
    const file = fopen(&live.ram_path, "wbx") orelse {
        live.failure = "request_ram_create";
        return;
    };
    var written = fwrite(live.ram, 1, RAM_BYTES, file) == RAM_BYTES and ferror(file) == 0;
    if (fclose(file) != 0) written = false;
    if (!written) {
        live.failure = "request_ram_write";
        return;
    }
    live.request_flip = live.pending_flip;
    live.request_frame = live.pending_frame;
    live.request_draws = live.pending_draws;
    live.selected = true;
    _ = fprintf(live.trace, "{\"kind\":\"natural_request\",\"mode\":%u,\"encounter_id\":%u,\"raw_object\":%u,\"raw_value\":%u,\"request_ordinal\":%u,\"matching_occurrence\":%u,\"flip\":%u,\"game_frame\":%u,\"additive_draws_at_request\":%u,\"ram_start\":%u,\"ram_bytes\":%u,\"ram_sha256\":\"%s\",\"rng_mode\":\"inherit-native\"}\n",
        mode, encounter, object, value, live.requests, live.matching, live.request_flip,
        live.request_frame, live.request_draws, RAM_START, RAM_BYTES, cs(&live.ram_sha));
    if (!traceRng("request_rng", true)) {
        live.failure = "request_rng_snapshot";
        return;
    }
    if (ferror(live.trace) != 0) live.failure = "request_trace_write";
}
/// C: `++live.matching == live.entry.occurrence`, evaluated only where the C condition reaches it.
fn nextMatching() bool {
    live.matching +%= 1;
    return live.matching == live.entry.occurrence;
}
fn onRequest(event: *const Request, unused: ?*anyopaque) callconv(.c) void {
    _ = unused;
    if (live.failure != null or live.terminal_seen) return;
    if (live.selected and !live.bound and
        (event.stage == P4_BATTLE_SCRIPT_REQUEST_BEGIN or event.stage == P4_BATTLE_FIELD_REQUEST_BEGIN or
            event.stage == P4_BATTLE_COLLISION_READY))
    {
        live.failure = "intervening_request_before_selected_task";
        return;
    }
    if (event.stage == P4_BATTLE_SCRIPT_REQUEST_BEGIN) {
        live.requests +%= 1;
        live.unbound_requests +%= 1;
        if (!live.selected and live.entry.mode == 1) snapshot();
    } else if (event.stage == P4_BATTLE_SCRIPT_REQUEST_IDENTIFIED) {
        if (!live.selected and live.entry.mode == 1 and event.encounter_id == live.entry.encounter and nextMatching())
            publishSnapshot(1, event.encounter_id, 0, 0);
        live.pending = false;
    } else if (event.stage == P4_BATTLE_FIELD_REQUEST_BEGIN) {
        live.requests +%= 1;
        live.unbound_requests +%= 1;
        if (!live.selected and live.entry.mode == 2 and (event.encounter_id & 0xffff) == live.entry.encounter and nextMatching()) {
            snapshot();
            live.field_task = event.guest_object;
            publishSnapshot(2, event.encounter_id, event.guest_object, event.value);
            live.pending = false;
        }
    } else if (event.stage == P4_BATTLE_COLLISION_READY) {
        live.requests +%= 1;
        live.unbound_requests +%= 1;
        if (!live.selected and live.entry.mode == 4 and event.encounter_id == live.entry.encounter and nextMatching()) {
            if (!range(event.guest_object, 0x3c)) {
                live.failure = "invalid_collision_task";
                return;
            }
            const work = g32(event.guest_object +% 0x38);
            if (!range(work, 0x110) or event.value != work +% 0x50 or g32(work +% 0x10c) != 0) {
                live.failure = "collision_setup_mismatch";
                return;
            }
            live.field_task = event.guest_object;
            live.field_actor = g32(work +% 0x44);
            live.field_group = g32(event.value +% 0x14);
            if (!range(live.field_actor, 0x4c) or !range(live.field_group, 10) or
                g32(live.field_actor +% 0x48) != live.field_group or
                g16(live.field_group +% 8) != live.entry.encounter)
            {
                live.failure = "collision_group_mismatch";
                return;
            }
            snapshot();
            publishSnapshot(4, event.encounter_id, event.guest_object, event.value);
            if (live.failure == null) {
                _ = fprintf(live.trace, "{\"kind\":\"collision_preconstructor\",\"task\":%u,\"setup\":%u,\"actor\":%u,\"group\":%u,\"encounter_id\":%u,\"flip\":%u,\"game_frame\":%u}\n",
                    live.field_task, event.value, live.field_actor, live.field_group, event.encounter_id,
                    live.request_flip, live.request_frame);
                if (ferror(live.trace) != 0) live.failure = "collision_trace_write";
            }
            live.pending = false;
        }
    } else if (event.stage == P4_BATTLE_SCRIPT_GROUP_CREATED and live.selected and live.entry.mode == 1 and !live.bound) {
        if (event.encounter_id != live.entry.encounter or live.script_group != 0 or !range(event.guest_object, 10) or
            g16(event.guest_object +% 8) != live.entry.encounter)
        {
            live.failure = "script_group_identity_invalid";
            return;
        }
        live.script_group = event.guest_object;
        _ = fprintf(live.trace, "{\"kind\":\"script_group_created\",\"group\":%u,\"encounter_id\":%u}\n",
            live.script_group, event.encounter_id);
        if (ferror(live.trace) != 0) live.failure = "script_group_trace_write";
    } else if (event.stage == P4_BATTLE_FIELD_GROUP_CREATED and live.selected and live.entry.mode == 2 and !live.bound) {
        if (event.guest_object != live.field_task or (event.encounter_id & 0xffff) != live.entry.encounter or
            live.field_group != 0 or !range(live.field_task, 0x3c) or !range(event.value, 10))
        {
            live.failure = "field_group_identity_invalid";
            return;
        }
        const work = g32(live.field_task +% 0x38);
        if (!range(work, 0x48)) {
            live.failure = "invalid_field_work";
            return;
        }
        live.field_actor = g32(work +% 0x44);
        if (!range(live.field_actor, 0x4c) or g32(live.field_actor +% 0x48) != event.value or
            g16(event.value +% 8) != live.entry.encounter)
        {
            live.failure = "field_group_identity_invalid";
            return;
        }
        live.field_group = event.value;
        _ = fprintf(live.trace, "{\"kind\":\"field_group_created\",\"actor\":%u,\"group\":%u,\"encounter_id\":%u}\n",
            live.field_actor, live.field_group, live.entry.encounter);
        if (ferror(live.trace) != 0) live.failure = "field_group_trace_write";
    } else if (event.stage == P4_BATTLE_TASK_BEGIN) {
        if (!live.selected) {
            if (live.unbound_requests != 0) live.unbound_requests -= 1;
            return;
        }
        if (live.bound) return;
        if (live.task_begun or live.unbound_requests != 1) {
            live.failure = "ambiguous_task_construction";
            return;
        }
        live.unbound_requests -= 1;
        live.task_begun = true;
        live.task_setup = event.guest_object;
        if (live.task_setup == 0 or !setupRange(live.task_setup)) {
            live.failure = "invalid_task_setup";
            return;
        }
        var expected_group = live.script_group;
        if (live.entry.mode == 2 or live.entry.mode == 4) {
            if (!range(live.field_task, 0x3c)) {
                live.failure = "invalid_field_task";
                return;
            }
            const work = g32(live.field_task +% 0x38);
            if (!range(work, 0x8c) or live.task_setup != work +% 0x50) {
                live.failure = "field_task_setup_mismatch";
                return;
            }
            if (g32(work +% 0x44) != live.field_actor or !range(live.field_actor, 0x4c) or
                g32(live.field_actor +% 0x48) != live.field_group)
            {
                live.failure = "field_actor_or_group_changed";
                return;
            }
            expected_group = live.field_group;
        }
        const enemies = g32(live.task_setup +% 0x14);
        if (expected_group == 0 or enemies != expected_group or !range(enemies, 10) or
            g16(enemies +% 8) != live.entry.encounter)
        {
            live.failure = "encounter_group_mismatch";
            return;
        }
        if (!traceRng("task_begin_rng", false)) {
            live.failure = "task_rng_snapshot";
            return;
        }
        _ = fprintf(live.trace, "{\"kind\":\"task_begin\",\"flip\":%u,\"game_frame\":%u,\"setup\":%u,\"setup_hex\":",
            p4pc_flip_count(), g32(GAME_FRAME), live.task_setup);
        if (live.task_setup == 0) {
            _ = fputs("null", live.trace);
        } else {
            const p = g2h(live.task_setup);
            _ = fputc('"', live.trace);
            var i: usize = 0;
            while (i < 0x3c) : (i += 1) _ = fprintf(live.trace, "%02x", @as(c_uint, p[i]));
            _ = fputc('"', live.trace);
        }
        _ = fputs("}\n", live.trace);
        if (ferror(live.trace) != 0) live.failure = "task_trace_write";
    } else if (event.stage == P4_BATTLE_TASK_CREATED and live.selected and !live.bound) {
        if (!live.task_begun or event.guest_object != live.task_setup or event.value == 0 or !range(gp(0xb3ac), 0xe88)) {
            live.failure = "unbound_native_task";
            return;
        }
        live.battle = gp(0xb3ac);
        live.bound = true;
        _ = fprintf(live.trace, "{\"kind\":\"task_created\",\"flip\":%u,\"game_frame\":%u,\"task\":%u,\"battle_context\":%u}\n",
            p4pc_flip_count(), g32(GAME_FRAME), event.value, live.battle);
        if (!traceRng("task_created_rng", false)) {
            live.failure = "task_rng_snapshot";
            return;
        }
        if (ferror(live.trace) != 0) live.failure = "task_trace_write";
    }
}
fn onAdmission(event: *const Admission, unused: ?*anyopaque) callconv(.c) void {
    _ = unused;
    if (!live.bound or live.terminal_seen) return;
    live.admissions[@intFromBool(event.queue_kind == P4_BATTLE_QUEUE_FOLLOWUP)] +%= 1;
    _ = fprintf(live.trace, "{\"kind\":\"admission\",\"update\":%u,\"queue_kind\":%u,\"id\":%u,\"side\":%u,\"action_uid\":\"%016llx\"}\n",
        live.updates +% 1, @as(c_uint, event.queue_kind), @as(c_uint, event.actor.native_id),
        @as(c_uint, event.actor.side), @as(c_ulonglong, event.actor.action_uid));
    if (ferror(live.trace) != 0) live.failure = "admission_trace_write";
}
fn onCommand(event: *const Command, unused: ?*anyopaque) callconv(.c) void {
    _ = unused;
    if (!live.bound or live.terminal_seen) return;
    if (event.free_action == 0) live.actions +%= 1;
    _ = fprintf(live.trace, "{\"kind\":\"command_commit\",\"update\":%u,\"id\":%u,\"side\":%u,\"action_kind\":%u,\"skill_id\":%u,\"item_id\":%u,\"targets\":%u,\"free_action\":%u}\n",
        live.updates +% 1, @as(c_uint, event.actor.native_id), @as(c_uint, event.actor.side), @as(c_uint, event.action_kind),
        @as(c_uint, event.skill_id), @as(c_uint, event.item_id), @as(c_uint, event.target_count), @as(c_uint, event.free_action));
    if (ferror(live.trace) != 0) live.failure = "command_trace_write";
}
fn onCost(event: *const Cost, unused: ?*anyopaque) callconv(.c) void {
    _ = unused;
    if (!live.bound or live.terminal_seen or event.actor.side != 0) return;
    if (event.kind == P4_BATTLE_COST_HP) {
        live.costs_hp +%= event.requested_cost;
    } else if (event.kind == P4_BATTLE_COST_SP) {
        live.costs_sp +%= event.requested_cost;
    } else if (event.kind == P4_BATTLE_COST_ITEM and event.item_count_before > event.item_count_after) {
        const used: u32 = event.item_count_before - event.item_count_after;
        live.items +%= used;
        if (event.item_id < 1536) live.item_counts[event.item_id] +%= used;
    }
    _ = fprintf(live.trace, "{\"kind\":\"cost_commit\",\"update\":%u,\"id\":%u,\"cost_kind\":%u,\"requested\":%u,\"item_id\":%u,\"hp_before\":%u,\"hp_after\":%u,\"sp_before\":%u,\"sp_after\":%u,\"item_before\":%u,\"item_after\":%u}\n",
        live.updates +% 1, @as(c_uint, event.actor.native_id), @as(c_uint, event.kind), event.requested_cost, @as(c_uint, event.item_id),
        @as(c_uint, event.hp_before), @as(c_uint, event.hp_after), @as(c_uint, event.sp_before), @as(c_uint, event.sp_after),
        @as(c_uint, event.item_count_before), @as(c_uint, event.item_count_after));
    if (ferror(live.trace) != 0) live.failure = "cost_trace_write";
}
fn onHit(event: *const Hit, unused: ?*anyopaque) callconv(.c) void {
    _ = unused;
    if (!live.bound or live.terminal_seen) return;
    if (event.target.side == 0 and event.target.native_id == 1) {
        if (event.hp_before != 0 and event.hp_after == 0) {
            live.lethal = event.*;
            live.lethal_update = live.updates +% 1;
            live.lethal_seen = true;
        } else if (event.hp_after != 0) live.lethal_seen = false;
    }
    _ = fprintf(live.trace, "{\"kind\":\"target_commit\",\"update\":%u,\"source_id\":%u,\"source_side\":%u,\"target_id\":%u,\"target_side\":%u,\"skill_id\":%u,\"hp_delta\":%d,\"sp_delta\":%d,\"hp_before\":%u,\"hp_after\":%u,\"sp_before\":%u,\"sp_after\":%u,\"result_flags\":%u,\"added_status\":%u,\"resources_applied\":%u}\n",
        live.updates +% 1, @as(c_uint, event.source.native_id), @as(c_uint, event.source.side), @as(c_uint, event.target.native_id),
        @as(c_uint, event.target.side), @as(c_uint, event.skill_id), @as(c_int, event.requested_hp_delta), @as(c_int, event.requested_sp_delta),
        @as(c_uint, event.hp_before), @as(c_uint, event.hp_after), @as(c_uint, event.sp_before), @as(c_uint, event.sp_after),
        @as(c_uint, event.result_flags), event.added_status, @as(c_uint, event.resources_applied));
    if (ferror(live.trace) != 0) live.failure = "target_trace_write";
}

export fn p4_battle_live_command_ready(action: u32) void {
    if (live.entry.mode == 3) {
        discoveryCommandReady(action);
        return;
    }
    if (range(action, 0x34)) { // the party member whose command is ready, for a search checkpoint
        const ready = g32(action +% 0x30);
        if (range(ready, 0xa6) and g8(ready +% 0xa2) == 0) live.ready_actor = g16(ready +% 0xa4);
    }
    if (!live.bound or live.terminal_seen or live.first_command or !range(action, 0x34)) return;
    const unit = g32(action +% 0x30);
    if (!range(unit, 0xa6) or g8(unit +% 0xa2) != 0 or g16(unit +% 0xa4) != 1) return;
    live.first_command = true;
    _ = fprintf(live.trace, "{\"kind\":\"first_protagonist_command\",\"update\":%u,\"flip\":%u,\"game_frame\":%u,\"party\":[",
        live.updates +% 1, p4pc_flip_count(), g32(GAME_FRAME));
    const p0 = @as(i32, datGetPartyId(0)) & 65535;
    const p1 = @as(i32, datGetPartyId(1)) & 65535;
    const p2 = @as(i32, datGetPartyId(2)) & 65535;
    const ids = [4]i32{ 1, p0, p1, p2 };
    var seen: u32 = 0;
    var count: u32 = 0;
    for (ids) |id| {
        if (id < 1 or id > 10 or (seen & (@as(u32, 1) << @intCast(id))) != 0) continue;
        seen |= @as(u32, 1) << @intCast(id);
        const record = datUnitGetRecord(@intCast(id));
        const hp = datCalcGetHp(record);
        const sp = datCalcGetSp(record);
        const max_hp = datCalcMaxHpById(@intCast(id));
        const max_sp = datCalcMaxSpById(@intCast(id));
        _ = fprintf(live.trace, "%s{\"id\":%d,\"hp\":%u,\"sp\":%u,\"max_hp\":%u,\"max_sp\":%u}",
            comma(count), @as(c_int, id), @as(c_uint, hp), @as(c_uint, sp), @as(c_uint, max_hp), @as(c_uint, max_sp));
        count += 1;
    }
    _ = fputs("]}\n", live.trace);
    if (ferror(live.trace) != 0) live.failure = "first_command_trace_write";
}
/// Copy only linked units. An empty list or a missing record is not a death.
fn collectUnits(battle: u32, units: *Units, counts: *[2]c_uint) bool {
    for (0..2) |group| {
        var unit = g32(battle +% 0x178 +% @as(u32, @intCast(group)) * 8);
        counts[group] = 0;
        while (unit != 0 and counts[group] < 12) {
            if (!range(unit, 0xa70)) return false;
            const out = &units[group][counts[group]];
            counts[group] += 1;
            out.* = .{};
            out.address = unit;
            out.genus = g8(unit +% 0xa2);
            out.id = g16(unit +% 0xa4);
            out.uid = g32(unit +% 0xa8);
            out.record = g32(unit +% 0xa64);
            if (out.record != 0) {
                if (!range(out.record, 0x10)) return false;
                out.record_id = g16(out.record +% 2);
                out.hp = g16(out.record +% 8);
                out.sp = g16(out.record +% 10);
                out.status = g32(out.record +% 12);
            }
            unit = g32(unit +% 0xa6c);
        }
        if (unit != 0) return false;
    }
    return true;
}
fn writeUnits(file: ?*FILE, units: *const Units, counts: *const [2]c_uint) void {
    _ = fputc('[', file);
    for (0..2) |group| {
        if (group != 0) _ = fputc(',', file);
        _ = fputc('[', file);
        for (0..counts[group]) |i| {
            const u = &units[group][i];
            _ = fprintf(file, "%s{\"guest_unit\":%u,\"uid\":%u,\"genus\":%u,\"id\":%u,\"guest_record\":",
                comma(i), u.address, u.uid, @as(c_uint, u.genus), @as(c_uint, u.id));
            if (u.record != 0) {
                _ = fprintf(file, "%u,\"record_id\":%u,\"hp\":%u,\"sp\":%u,\"status\":%u}",
                    u.record, @as(c_uint, u.record_id), @as(c_uint, u.hp), @as(c_uint, u.sp), u.status);
            } else _ = fputs("null,\"record_id\":null,\"hp\":null,\"sp\":null,\"status\":null}", file);
        }
        _ = fputc(']', file);
    }
    _ = fputc(']', file);
}

fn endpoint(context: *[4]u32) bool {
    const field = gp(0x9db0);
    if (!range(field, 8)) return false;
    context[0] = g32(field);
    context[1] = g32(field +% 4);
    context[2] = @as(u16, @bitCast(datGetDateIndex()));
    context[3] = @bitCast(FldStepIndex());
    const expected = [4]u32{ live.entry.field, live.entry.room, live.entry.date, live.entry.step };
    for (0..4) |i| if ((live.entry.endpoint_mask & (@as(u32, 1) << @intCast(i))) != 0 and context[i] != expected[i]) return false;
    return true;
}

// Discovery emits bounded evidence, not a selected or synthesized encounter.
fn discoveryCheckWrite() void {
    const position = ftell(live.trace);
    if (ferror(live.trace) != 0 or position < 0) {
        live.failure = "discovery_trace_write";
    } else if (position > 8 * 1024 * 1024 or blk: {
        discovery.records +%= 1;
        break :blk discovery.records > 8192;
    }) live.failure = "discovery_trace_limit";
}
fn discoveryCanWrite() bool {
    const position = ftell(live.trace);
    if (position < 0 or ferror(live.trace) != 0) {
        live.failure = "discovery_trace_write";
        return false;
    }
    // Reserve more than the maximum line containing 24 bounded unit records.
    if (discovery.records >= 8192 or position > 8 * 1024 * 1024 - 8192) {
        live.failure = "discovery_trace_limit";
        return false;
    }
    return true;
}
fn discoveryEvent(kind: [*:0]const u8, object: u32, value: u32) void {
    if (!discoveryCanWrite()) return;
    const field = gp(0x9db0);
    _ = fprintf(live.trace, "{\"kind\":\"%s\",\"ordinal\":%u,\"mode\":%u,\"encounter_id\":",
        kind, discovery.ordinal, discovery.mode);
    if (discovery.identified) _ = fprintf(live.trace, "%u", discovery.id) else _ = fputs("null", live.trace);
    _ = fprintf(live.trace, ",\"occurrence\":%u,\"object\":%u,\"value\":%u,\"flip\":%u,\"game_frame\":%u,\"field\":",
        discovery.occurrence, object, value, p4pc_flip_count(), g32(GAME_FRAME));
    if (range(field, 8)) {
        _ = fprintf(live.trace, "%u,\"room\":%u", g32(field), g32(field +% 4));
    } else _ = fputs("null,\"room\":null", live.trace);
    const date: c_uint = @as(u16, @bitCast(datGetDateIndex()));
    const step: u32 = @bitCast(FldStepIndex());
    _ = fprintf(live.trace, ",\"date\":%u,\"step\":%u}\n", date, step);
    discoveryCheckWrite();
}
fn discoveryIdentify(id: u32) void {
    discovery.id = id & 0xffff;
    discovery.identified = true;
    const row: usize = if (discovery.mode == 4) 2 else discovery.mode - 1;
    const slot = &discovery.occurrences[row][discovery.id];
    slot.* +%= 1;
    discovery.occurrence = slot.*;
}
fn discoveryRequest(event: *const Request, unused: ?*anyopaque) callconv(.c) void {
    _ = unused;
    if (live.failure != null) return;
    if (event.stage == P4_BATTLE_SCRIPT_REQUEST_BEGIN or event.stage == P4_BATTLE_FIELD_REQUEST_BEGIN) {
        if (discovery.pending or discovery.active) {
            live.failure = "discovery_ambiguous_request";
            return;
        }
        discovery.ordinal +%= 1;
        discovery.mode = if (event.stage == P4_BATTLE_SCRIPT_REQUEST_BEGIN) 1 else 2;
        discovery.pending = true;
        discovery.identified = false;
        discovery.task_begun = false;
        discovery.id = 0;
        discovery.occurrence = 0;
        discovery.group = 0;
        discovery.actor = 0;
        discovery.setup = 0;
        discovery.field_task = event.guest_object;
        if (discovery.mode == 2) discoveryIdentify(event.encounter_id);
        discoveryEvent("request_begin", event.guest_object, event.value);
    } else if (event.stage == P4_BATTLE_COLLISION_READY) {
        if (discovery.pending or discovery.active or !range(event.guest_object, 0x3c)) {
            live.failure = "discovery_ambiguous_collision";
            return;
        }
        const work = g32(event.guest_object +% 0x38);
        if (!range(work, 0x110) or event.value != work +% 0x50 or g32(work +% 0x10c) != 0) {
            live.failure = "discovery_collision_setup_mismatch";
            return;
        }
        discovery.ordinal +%= 1;
        discovery.mode = 4;
        discovery.pending = true;
        discovery.identified = false;
        discovery.task_begun = false;
        discovery.field_task = event.guest_object;
        discovery.actor = g32(work +% 0x44);
        discovery.group = g32(event.value +% 0x14);
        if (!range(discovery.actor, 0x4c) or !range(discovery.group, 10) or
            g32(discovery.actor +% 0x48) != discovery.group or
            g16(discovery.group +% 8) != event.encounter_id)
        {
            live.failure = "discovery_collision_group_mismatch";
            return;
        }
        discoveryIdentify(event.encounter_id);
        discoveryEvent("collision_preconstructor", event.guest_object, event.value);
        if (live.failure == null) discoveryEvent("group_created", discovery.group, discovery.actor);
    } else if (event.stage == P4_BATTLE_SCRIPT_REQUEST_IDENTIFIED) {
        if (!discovery.pending or discovery.mode != 1 or discovery.identified) {
            live.failure = "discovery_unpaired_identification";
            return;
        }
        discoveryIdentify(event.encounter_id);
        discoveryEvent("request_identified", 0, 0);
    } else if (event.stage == P4_BATTLE_SCRIPT_GROUP_CREATED) {
        if (!discovery.pending or discovery.mode != 1 or !discovery.identified or discovery.group != 0 or
            event.encounter_id != discovery.id or !range(event.guest_object, 10) or
            g16(event.guest_object +% 8) != discovery.id)
        {
            live.failure = "discovery_script_group_mismatch";
            return;
        }
        discovery.group = event.guest_object;
        discoveryEvent("group_created", discovery.group, 0);
    } else if (event.stage == P4_BATTLE_FIELD_GROUP_CREATED) {
        if (!discovery.pending or discovery.mode != 2 or discovery.group != 0 or
            event.guest_object != discovery.field_task or (event.encounter_id & 0xffff) != discovery.id or
            !range(discovery.field_task, 0x3c) or !range(event.value, 10))
        {
            live.failure = "discovery_field_group_mismatch";
            return;
        }
        const work = g32(discovery.field_task +% 0x38);
        if (!range(work, 0x48)) {
            live.failure = "discovery_invalid_field_work";
            return;
        }
        discovery.actor = g32(work +% 0x44);
        if (!range(discovery.actor, 0x4c) or g32(discovery.actor +% 0x48) != event.value or
            g16(event.value +% 8) != discovery.id)
        {
            live.failure = "discovery_field_actor_mismatch";
            return;
        }
        discovery.group = event.value;
        discoveryEvent("group_created", discovery.group, discovery.actor);
    } else if (event.stage == P4_BATTLE_TASK_BEGIN) {
        if (!discovery.pending or !discovery.identified or discovery.group == 0 or discovery.task_begun or
            event.guest_object == 0 or !setupRange(event.guest_object))
        {
            live.failure = "discovery_unsupported_or_ambiguous_task";
            return;
        }
        discovery.setup = event.guest_object;
        if (g32(discovery.setup +% 0x14) != discovery.group) {
            live.failure = "discovery_task_group_mismatch";
            return;
        }
        if (discovery.mode == 2 or discovery.mode == 4) {
            if (!range(discovery.field_task, 0x3c)) {
                live.failure = "discovery_invalid_field_task";
                return;
            }
            const work = g32(discovery.field_task +% 0x38);
            if (!range(work, 0x8c) or discovery.setup != work +% 0x50 or
                g32(work +% 0x44) != discovery.actor or !range(discovery.actor, 0x4c) or
                g32(discovery.actor +% 0x48) != discovery.group)
            {
                live.failure = "discovery_field_task_mismatch";
                return;
            }
        }
        discovery.task_begun = true;
        discoveryEvent("task_begin", discovery.setup, discovery.group);
    } else if (event.stage == P4_BATTLE_TASK_CREATED) {
        if (!discovery.pending or !discovery.task_begun or event.guest_object != discovery.setup or
            event.value == 0 or !range(gp(0xb3ac), 0xe88))
        {
            live.failure = "discovery_unbound_task";
            return;
        }
        discovery.pending = false;
        discovery.active = true;
        discovery.first_command = false;
        discovery.fight_updates = 0;
        discovery.actions = 0;
        discovery.battle = gp(0xb3ac);
        discovery.fights +%= 1;
        discoveryEvent("task_created", event.value, discovery.battle);
    }
}
fn discoveryCommand(event: *const Command, unused: ?*anyopaque) callconv(.c) void {
    _ = unused;
    if (discovery.active and event.free_action == 0) discovery.actions +%= 1;
}
fn discoveryCommandReady(action: u32) void {
    if (live.failure != null or !discovery.active or discovery.first_command or !range(action, 0x34)) return;
    const unit = g32(action +% 0x30);
    if (!range(unit, 0xa6) or g8(unit +% 0xa2) != 0 or g16(unit +% 0xa4) != 1) return;
    var units: Units = undefined;
    var counts: [2]c_uint = undefined;
    if (!collectUnits(discovery.battle, &units, &counts)) {
        live.failure = "discovery_invalid_unit_list";
        return;
    }
    if (!discoveryCanWrite()) return;
    discovery.first_command = true;
    discovery.commands +%= 1;
    _ = fprintf(live.trace, "{\"kind\":\"first_protagonist_command\",\"ordinal\":%u,\"mode\":%u," ++
        "\"encounter_id\":%u,\"occurrence\":%u,\"flip\":%u,\"game_frame\":%u,\"linked_units\":",
        discovery.ordinal, discovery.mode, discovery.id, discovery.occurrence,
        p4pc_flip_count(), g32(GAME_FRAME));
    writeUnits(live.trace, &units, &counts);
    _ = fputs("}\n", live.trace);
    discoveryCheckWrite();
}
/// Shared tail of both publishers: close result and trace, then rename the temporary into place.
fn publishClose(file: *FILE, temp: [*c]const u8) c_int {
    var failed = ferror(file) != 0;
    if (fclose(file) != 0) failed = true;
    if (live.trace != null and fclose(live.trace) != 0) failed = true;
    live.trace = null;
    if (failed or rename(temp, live.result) != 0) return -1;
    live.published = true;
    return 0;
}
fn discoveryPublish(outcome: [*c]const u8, cause: [*c]const u8, context: ?*const [4]u32) c_int {
    if (live.published) return 0;
    var temp: [2048]u8 = undefined;
    const length = snprintf(&temp, temp.len, "%s.tmp", live.result);
    if (length < 0 or @as(usize, @intCast(length)) >= temp.len) return -1;
    const file = fopen(&temp, "wbx") orelse return -1;
    _ = fprintf(file, "{\"schema\":1,\"entry_kind\":\"live-discovery\",\"outcome\":\"%s\"," ++
        "\"cause\":\"%s\",\"requests\":%u,\"bound_fights\":%u,\"first_commands\":%u," ++
        "\"trace_records\":%u,\"route_updates\":%u,\"absolute_flip_limit\":%u," ++
        "\"final_flip\":%u,\"final_game_frame\":%u,\"endpoint\":",
        outcome, cause, discovery.ordinal, discovery.fights, discovery.commands,
        discovery.records, live.updates, live.entry.occurrence,
        p4pc_flip_count(), g32(GAME_FRAME));
    if (context) |ctx| {
        _ = fprintf(file, "{\"field\":%u,\"room\":%u,\"date\":%u,\"step\":%u}", ctx[0], ctx[1], ctx[2], ctx[3]);
    } else _ = fputs("null", file);
    _ = fputs("}\n", file);
    return publishClose(file, &temp);
}
fn publish(outcome: [*c]const u8, cause: [*c]const u8, context: ?*const [4]u32) c_int {
    if (live.entry.mode == 3) return discoveryPublish(outcome, cause, context);
    if (live.published) return 0;
    var temp: [2048]u8 = undefined;
    const length = snprintf(&temp, temp.len, "%s.tmp", live.result);
    if (length < 0 or @as(usize, @intCast(length)) >= temp.len) return -1;
    const file = fopen(&temp, "wbx") orelse return -1;
    _ = fprintf(file, "{\"schema\":1,\"entry_kind\":\"live-natural\",\"outcome\":\"%s\",\"terminal_cause\":\"%s\",\"rng_mode\":\"inherit-native\",\"selected\":%s,\"task_bound\":%s,\"route_updates\":%u,\"fight_updates\":%u,\"encounter_id\":%u,\"request_mode\":%u,\"matching_occurrence\":%u,\"request_ram_sha256\":",
        outcome, cause, jsonBool(live.selected), jsonBool(live.bound),
        live.updates, live.fight_updates, live.entry.encounter, live.entry.mode, live.entry.occurrence);
    if (live.selected) _ = fprintf(file, "\"%s\"", cs(&live.ram_sha)) else _ = fputs("null", file);
    _ = fprintf(file, ",\"fight_terminal\":");
    if (live.terminal_seen) {
        _ = fprintf(file, "{\"native_result\":%u,\"outcome\":\"%s\",\"cause\":\"%s\",\"update\":%u,\"flip\":%u,\"game_frame\":%u,\"additive_draws\":%u,\"presentation\":",
            live.terminal, live.battle_outcome, live.battle_cause, live.terminal_update,
            live.terminal_flip, live.terminal_frame, live.terminal_draws);
        if (live.terminal_presentation != null) {
            _ = fprintf(file, "\"%s\"}", live.terminal_presentation);
        } else _ = fputs("null}", file);
    } else _ = fputs("null", file);
    _ = fputs(",\"terminal_linked_units\":", file);
    if (live.terminal_seen) writeUnits(file, &live.terminal_units, &live.terminal_counts) else _ = fputs("null", file);
    _ = fprintf(file, ",\"last_protagonist_lethal\":");
    if (live.terminal_lethal) {
        _ = fprintf(file, "{\"update\":%u,\"skill_id\":%u,\"source_id\":%u,\"hp_delta\":%d,\"result_flags\":%u,\"added_status\":%u}",
            live.lethal_update, @as(c_uint, live.lethal.skill_id), @as(c_uint, live.lethal.source.native_id),
            @as(c_int, live.lethal.requested_hp_delta), @as(c_uint, live.lethal.result_flags), live.lethal.added_status);
    } else _ = fputs("null", file);
    _ = fprintf(file, ",\"first_protagonist_command_observed\":%s,\"turns\":{\"ordinary_admissions\":%llu,\"followup_admissions\":%llu,\"actions\":%u},\"resources\":{\"hp_spent\":%llu,\"sp_spent\":%llu,\"items_consumed\":%llu,\"items_by_id\":[",
        jsonBool(live.first_command), @as(c_ulonglong, live.admissions[0]), @as(c_ulonglong, live.admissions[1]), live.actions,
        @as(c_ulonglong, live.costs_hp), @as(c_ulonglong, live.costs_sp), @as(c_ulonglong, live.items));
    var first_item = true;
    for (&live.item_counts, 0..) |used, id| {
        if (used == 0) continue;
        const sep: [*:0]const u8 = if (first_item) "" else ",";
        _ = fprintf(file, "%s{\"id\":%u,\"used\":%llu}", sep, @as(c_uint, @intCast(id)), @as(c_ulonglong, used));
        first_item = false;
    }
    _ = fputs("]},\"route_endpoint\":", file);
    if (context) |ctx| {
        _ = fprintf(file, "{\"field\":%u,\"room\":%u,\"date\":%u,\"step\":%u}", ctx[0], ctx[1], ctx[2], ctx[3]);
    } else _ = fputs("null", file);
    _ = fprintf(file, ",\"final_flip\":%u,\"final_game_frame\":%u,\"final_additive_draws\":%u}\n",
        p4pc_flip_count(), g32(GAME_FRAME), p4pc_rng_draws);
    return publishClose(file, &temp);
}
fn finish(outcome: [*c]const u8, cause: [*c]const u8, code: c_int, context: ?*const [4]u32) noreturn {
    if (publish(outcome, cause, context) != 0) {
        _ = bad("result_publication");
        exit(4);
    }
    p4pcExit(code);
}
export fn p4_battle_live_before_present() void {
    if (live.entry.mode != 3) return;
    if (live.failure != null) finish("engine_error", live.failure, 4, null);
    if (p4pc_flip_count() >= live.entry.occurrence)
        finish("update_limit", "absolute_flip_limit", 3, null);
}
export fn p4_battle_live_start(entry: [*c]const u8, result: [*c]const u8, trace: [*c]const u8) c_int {
    var bytes: [64]u8 = undefined;
    if (entry == null or result == null or trace == null) return bad("missing_paths");
    const file = fopen(entry, "rb") orelse return bad("entry_open");
    var valid = fread(&bytes, 1, bytes.len, file) == bytes.len and fgetc(file) == EOF and ferror(file) == 0;
    if (fclose(file) != 0) valid = false;
    if (!valid or p4_battle_live_decode(&bytes, bytes.len, &live.entry) == 0) return bad("invalid_entry");
    if (!validEnvironment(&live.entry)) return bad("incompatible_environment");
    if (getenv("P4PC_LOCKSTEP_BATTLE_ANCHOR") != null) {
        if ((live.entry.mode != 2 and live.entry.mode != 4) or live.entry.occurrence != 1 or live.entry.completion != 1 or
            !exactEnv("P4PC_LOCKSTEP_STATE_ALIGNED", "1")) return bad("invalid_battle_qualification");
        live.qualification = true;
    }
    if (getenv("P4PC_BATTLE_FORK") != null and !battleForkStart(getenv("P4PC_BATTLE_FORK")))
        return bad("invalid_battle_fork");
    if (fopen(result, "rb")) |existing| {
        _ = fclose(existing);
        return bad("result_exists");
    }
    if (live.entry.mode != 3) {
        if (snprintf(&live.ram_path, live.ram_path.len, "%s.request-ram", trace) >= @as(c_int, live.ram_path.len))
            return bad("trace_path_too_long");
        if (fopen(&live.ram_path, "rb")) |existing| {
            _ = fclose(existing);
            return bad("request_ram_exists");
        }
    }
    p4_mod_fidelity_check_start();
    if (live.entry.mode != 3) {
        live.ram = @ptrCast(malloc(RAM_BYTES) orelse return bad("request_ram_allocation"));
    }
    live.trace = fopen(trace, "wbx");
    if (live.trace == null) {
        free(live.ram);
        live.ram = null;
        return bad("trace_create");
    }
    live.result = result;
    const observer: Observer = if (live.entry.mode == 3)
        .{ .context = null, .admission = null, .cost = null, .hit = null, .command = &discoveryCommand, .request = &discoveryRequest }
    else
        .{ .context = null, .admission = &onAdmission, .cost = &onCost, .hit = &onHit, .command = &onCommand, .request = &onRequest };
    live.observer = p4_battle_observer_add(&observer);
    if (live.observer == 0) {
        _ = fclose(live.trace);
        live.trace = null;
        free(live.ram);
        live.ram = null;
        return bad("observer_registration");
    }
    p4_battle_sim_renderless = @intFromBool((live.entry.flags & 1) != 0);
    p4_battle_live_active = 1;
    p4_battle_sim_active = 1;
    return 1;
}
export fn p4_battle_live_before_update() void {
    live.update_flip = p4pc_flip_count();
    if (live.failure != null) finish("engine_error", live.failure, 4, null);
    if (live.updates >= live.entry.route_limit) finish("update_limit", "route_update_limit", 3, null);
    if (live.entry.mode == 3) {
        if (p4pc_flip_count() >= live.entry.occurrence)
            finish("update_limit", "absolute_flip_limit", 3, null);
        if (discovery.active and discovery.fight_updates >= live.entry.fight_limit)
            finish("update_limit", "fight_update_limit", 3, null);
        if (discovery.active and discovery.actions >= live.entry.action_limit)
            finish("action_limit", "native_action_limit", 3, null);
        return;
    }
    if (live.bound and !live.terminal_seen and live.fight_updates >= live.entry.fight_limit)
        finish("update_limit", "fight_update_limit", 3, null);
    if (live.bound and !live.terminal_seen and live.actions >= live.entry.action_limit)
        finish("action_limit", "native_action_limit", 3, null);
}
export fn p4_battle_live_after_update(completed: c_int) void {
    if (completed == 0) return;
    live.updates +%= 1;
    if (live.failure != null) finish("engine_error", live.failure, 4, null);
    if (live.entry.mode == 3) {
        if (p4pc_flip_count() > live.entry.occurrence)
            finish("update_limit", "absolute_flip_limit", 3, null);
        const battle = gp(0xb3ac);
        if (discovery.active) {
            discovery.fight_updates +%= 1;
            if (battle != discovery.battle or !range(battle, 0xe88))
                finish("engine_error", "discovery_battle_context_lost", 4, null);
            const phase = g32(battle +% 0x2fc);
            const flags = g32(battle +% 0xc);
            if (phase == 17 and (flags & 1) == 0) {
                discoveryEvent("fight_terminal", g16(battle +% 0x1c), flags);
                if (live.failure != null) finish("engine_error", live.failure, 4, null);
                discovery.active = false;
            }
        } else if (range(battle, 0xe88) and (g32(battle +% 0xc) & 1) != 0) {
            finish("engine_error", "discovery_unsupported_unbound_battle", 4, null);
        }
        if (p4pc_pad_walk_status() == P4WALK_DONE) {
            var context: [4]u32 = undefined;
            if (discovery.pending or discovery.active or discovery.fights == 0 or discovery.commands == 0)
                finish("engine_error", "discovery_incomplete_or_no_witness", 4, null);
            if (!endpoint(&context)) finish("engine_error", "walk_endpoint_mismatch", 4, null);
            finish("route_complete", "native_walk_done_and_endpoint", 0, &context);
        }
        if (p4pc_flip_count() >= live.entry.occurrence)
            finish("update_limit", "absolute_flip_limit", 3, null);
        return;
    }
    if (live.bound and !live.terminal_seen) {
        live.fight_updates +%= 1;
        const battle = gp(0xb3ac);
        if (battle != live.battle or !range(battle, 0xe88)) finish("engine_error", "battle_context_lost_before_terminal", 4, null);
        live.seen = true;
        const phase = g32(battle +% 0x2fc);
        const flags = g32(battle +% 0xc);
        const terminal: u32 = g16(battle +% 0x1c);
        var units: Units = undefined;
        var counts: [2]c_uint = undefined;
        if (!collectUnits(battle, &units, &counts)) finish("engine_error", "invalid_linked_unit_list", 4, null);
        _ = fprintf(live.trace, "{\"kind\":\"battle_update\",\"update\":%u,\"phase\":%u,\"flags\":%u,\"native_result\":%u,\"linked_units\":",
            live.updates, phase, flags, terminal);
        writeUnits(live.trace, &units, &counts);
        _ = fputs("}\n", live.trace);
        if (ferror(live.trace) != 0) finish("engine_error", "unit_trace_write", 4, null);
        if (phase == 17 and (flags & 1) == 0) {
            if (terminal < 1 or terminal > 3) finish("engine_error", "native_exit_without_terminal_state", 4, null);
            live.terminal = terminal;
            live.terminal_seen = true;
            for (0..2) |group|
                @memcpy(live.terminal_units[group][0..counts[group]], units[group][0..counts[group]]);
            live.terminal_counts = counts;
            live.battle_outcome = if (terminal == 1) "win" else if (terminal == 2) "defeat" else if (g16(battle +% 0xc5e) != 0) "escape" else "scripted_conclusion";
            live.battle_cause = if (terminal == 1)
                "native_battle_exit_victory"
            else if (terminal == 2)
                (if ((g32(battle +% 0x10) & 0x10000000) != 0)
                    "native_forced_defeat_flag"
                else if (datCalcIsDead(datUnitGetRecord(1), 0) != 0)
                    "protagonist_terminal_ko"
                else
                    "native_party_terminal_defeat")
            else if (g16(battle +% 0xc5e) != 0)
                "native_committed_party_group_escape"
            else
                "native_battle_exit_no_party";
            live.terminal_lethal = terminal == 2 and live.lethal_seen and datCalcGetHp(datUnitGetRecord(1)) == 0;
            live.terminal_update = live.updates;
            live.terminal_flip = p4pc_flip_count();
            live.terminal_frame = g32(GAME_FRAME);
            live.terminal_draws = p4pc_rng_draws;
            if (!traceRng("fight_terminal_rng", false)) finish("engine_error", "terminal_rng_snapshot", 4, null);
            _ = fprintf(live.trace, "{\"kind\":\"fight_terminal\",\"native_result\":%u,\"update\":%u,\"flip\":%u,\"game_frame\":%u,\"additive_draws\":%u}\n",
                terminal, live.updates, live.terminal_flip, live.terminal_frame, live.terminal_draws);
            if (ferror(live.trace) != 0) finish("engine_error", "terminal_trace_write", 4, null);
            // Presentation happens inside the update, before this terminal observation.
            if (live.qualification)
                live.terminal_presentation = if (live.terminal_flip > live.update_flip) "presented" else "unpresented";
            if (live.entry.completion == 1)
                finish(live.battle_outcome, live.battle_cause, 0, null);
        }
    }
    if (live.entry.completion == 2 and p4pc_pad_walk_status() == P4WALK_DONE) {
        var context: [4]u32 = undefined;
        if (!live.terminal_seen) finish("engine_error", "walk_done_without_selected_terminal", 4, null);
        if (!endpoint(&context)) finish("engine_error", "walk_endpoint_mismatch", 4, null);
        if (!traceRng("route_final_rng", false)) finish("engine_error", "route_rng_snapshot", 4, null);
        finish("route_complete", "native_walk_done_and_endpoint", 0, &context);
    }
}
fn prefix(cause: [*:0]const u8) c_int {
    if (p4_battle_live_active == 0) return 0;
    if (live.failure != null) return if (publish("engine_error", live.failure, null) != 0) -1 else 1;
    return publish("observation_prefix", cause, null);
}
export fn p4_battle_live_lockstep_stop() c_int {
    return prefix("lockstep_stop");
}
export fn p4_battle_live_lockstep_eof() c_int {
    return prefix("lockstep_replay_eof");
}
export fn p4_battle_live_lockstep_boundary() c_int {
    return prefix("lockstep_boundary_stop");
}

// P4PC_BATTLE_FORK=DIR (Linux): a search checkpoint at each fresh party command of the selected fight once the walk's
// battleturn: lines are spent. The process stops inside the walker's command arming, before that pad read is recorded,
// and serves DIR/NODE/control: "fork NAME LINE" forks a child that continues as node NAME with LINE as the command;
// "exit" ends the checkpoint. A child writes its own trace, result, lockstep log and stdout/stderr under DIR/NAME and
// reads the disc through a private descriptor, so each node's files continue its parent's exactly: a path's segments
// concatenate to a fresh run of the same tape. No guest state, RNG, input schedule or clock is changed.
var battle_fork: struct {
    dir: [1024]u8 = @splat(0),
    node: [64]u8 = @splat(0),
    result: [1200]u8 = @splat(0),
} = .{};

fn forkPath(out: [*c]u8, size: usize, node: [*c]const u8, leaf: [*c]const u8) bool {
    const slash: [*c]const u8 = if (leaf != null) "/" else "";
    const tail: [*c]const u8 = if (leaf != null) leaf else "";
    const length = snprintf(out, size, "%s/%s%s%s", cs(&battle_fork.dir), node, slash, tail);
    return length > 0 and @as(usize, @intCast(length)) < size;
}
fn forkThreads() c_int {
    const tasks = posix.opendir("/proc/self/task") orelse return -1;
    var count: c_int = 0;
    while (true) {
        const e = posix.readdir(tasks);
        if (e == null) break;
        if (e.*.d_name[0] != '.') count += 1;
    }
    _ = posix.closedir(tasks);
    return count;
}
fn battleForkStart(dir: [*c]const u8) bool {
    return if (comptime battle_fork_enabled) battleForkStartLinux(dir) else false;
}
fn battleForkStartLinux(dir: [*c]const u8) bool {
    var path: [1200]u8 = undefined;
    if (dir[0] == 0 or strlen(dir) >= battle_fork.dir.len - 80 or live.entry.mode == 3 or live.entry.completion != 1 or
        live.qualification or getenv("P4PC_LOCKSTEP_REPLAY") != null) return false;
    _ = strcpy(&battle_fork.dir, dir);
    _ = strcpy(&battle_fork.node, "root");
    // The driver may create DIR/root first (the trace and lockstep log live there); it must be a directory.
    if (!forkPath(&path, path.len, "root", null) or (posix.mkdir(&path, 0o755) != 0 and std.c._errno().* != posix.EEXIST)) return false;
    // Opening it succeeds only for a directory (ENOTDIR otherwise). No stat structure is declared: translate-c leaves
    // `struct stat` opaque for some targets (the i386 host-unit build), which makes a by-value variable a compile error.
    const root = posix.opendir(&path) orelse return false;
    _ = posix.closedir(root);
    // signal(SIGCHLD, SIG_IGN); SIG_IGN is (__sighandler_t)1 on Linux. Checkpoints never wait for their children.
    _ = signal(posix.SIGCHLD, @ptrFromInt(1));
    p4pc_pad_set_battle_turn_hook(&forkTurn);
    return true;
}
fn forkBecome(name: [*c]const u8) bool {
    var path: [1200]u8 = undefined;
    var stream: c_int = 1;
    while (stream <= 2) : (stream += 1) {
        if (!forkPath(&path, path.len, name, if (stream == 1) "stdout.log" else "stderr.log")) return false;
        const fd = posix.open(&path, posix.O_WRONLY | posix.O_CREAT | posix.O_EXCL | posix.O_CLOEXEC, @as(c_uint, 0o644));
        if (fd < 0 or posix.dup2(fd, stream) < 0) return false;
        _ = posix.close(fd);
    }
    if (!forkPath(&path, path.len, name, "trace.jsonl")) return false;
    _ = fclose(live.trace); // flushed before fork: closes only this process's descriptor
    live.trace = fopen(&path, "wbx");
    if (live.trace == null) return false;
    if (!forkPath(&battle_fork.result, battle_fork.result.len, name, "result.json")) return false;
    live.result = &battle_fork.result;
    if (!forkPath(&path, path.len, name, "lockstep") or p4pc_lockstep_fork_reopen(&path) != 0 or vfs_fork_private() != 0) return false;
    _ = snprintf(&battle_fork.node, battle_fork.node.len, "%s", name);
    return true;
}
fn forkCheckpointRecord(n: c_uint) void {
    var path: [1200]u8 = undefined;
    var temp: [1210]u8 = undefined;
    var units: Units = undefined;
    var counts: [2]c_uint = .{ 0, 0 };
    if (!forkPath(&path, path.len, &battle_fork.node, "checkpoint.json")) return;
    _ = snprintf(&temp, temp.len, "%s.tmp", cs(&path));
    const file = fopen(&temp, "wbx") orelse return;
    _ = fprintf(file, "{\"schema\":1,\"node\":\"%s\",\"turn\":%u,\"pid\":%ld,\"update\":%u,\"fight_updates\":%u,\"actions\":%u," ++
        "\"flip\":%u,\"game_frame\":%u,\"additive_draws\":%u,\"threads\":1,\"actor\":%u,\"linked_units\":",
        cs(&battle_fork.node), n, @as(c_long, posix.getpid()), live.updates, live.fight_updates, live.actions,
        p4pc_flip_count(), g32(GAME_FRAME), p4pc_rng_draws, live.ready_actor);
    const linked = collectUnits(gp(0xb3ac), &units, &counts);
    if (linked) writeUnits(file, &units, &counts) else _ = fputs("null", file);
    // Per linked unit, in the same order, the DatUnit record words +0x00 (flags; 0x200 guarding, datCalcIsGuarding) and
    // +0x10..+0x20 (+0x14 charge state, +0x1C..+0x23 signed buff nibbles) a search ranks by; null for an absent record.
    _ = fputs(",\"record_words\":", file);
    if (!linked) {
        _ = fputs("null", file);
    } else {
        _ = fputc('[', file);
        for (0..2) |group| {
            _ = fputs(if (group != 0) ",[" else "[", file);
            for (0..counts[group]) |i| {
                const record = units[group][i].record;
                if (i != 0) _ = fputc(',', file);
                if (record == 0 or !range(record, 0x24)) {
                    _ = fputs("null", file);
                    continue;
                }
                _ = fprintf(file, "[%u,%u,%u,%u,%u,%u]", g32(record), g32(record +% 0x10), g32(record +% 0x14),
                    g32(record +% 0x18), g32(record +% 0x1c), g32(record +% 0x20));
            }
            _ = fputc(']', file);
        }
        _ = fputc(']', file);
    }
    _ = fputs(",\"descriptors\":[", file);
    const fds = posix.opendir("/proc/self/fd");
    var first = true;
    while (fds != null) {
        const e = posix.readdir(fds);
        if (e == null) break;
        var link: [64]u8 = undefined;
        var target: [1024]u8 = undefined;
        if (e.*.d_name[0] == '.') continue;
        _ = snprintf(&link, link.len, "/proc/self/fd/%s", cs(&e.*.d_name));
        const length = posix.readlink(&link, &target, target.len - 1);
        const end: usize = if (length > 0) @intCast(length) else 0;
        target[end] = 0;
        var c: usize = 0;
        while (target[c] != 0) : (c += 1) {
            if (target[c] == '"' or target[c] == '\\' or target[c] < 0x20) target[c] = '?';
        }
        const sep: [*:0]const u8 = if (first) "" else ",";
        _ = fprintf(file, "%s{\"fd\":%s,\"target\":\"%s\"}", sep, cs(&e.*.d_name), cs(&target));
        first = false;
    }
    if (fds != null) _ = posix.closedir(fds);
    _ = fputs("]}\n", file);
    if (fclose(file) == 0) _ = rename(&temp, &path);
}
/// Serves the control FIFO until a fork returns in the child. The checkpoint itself never runs another game update.
fn forkTurn(n: c_uint, line: [*c]u8, size: usize) callconv(.c) c_int {
    var control: [1200]u8 = undefined;
    var command: [1024]u8 = undefined;
    if (p4_battle_live_active == 0 or !live.bound or live.terminal_seen or live.failure != null) return 0;
    if (fflush(live.trace) != 0 or p4pc_lockstep_flush() != 0 or fflush(stdout) != 0 or fflush(stderr) != 0) {
        live.failure = "battle_fork_flush";
        return 0;
    }
    if (forkThreads() != 1) {
        live.failure = "battle_fork_requires_single_thread";
        return 0;
    }
    if (!forkPath(&control, control.len, &battle_fork.node, "control") or posix.mkfifo(&control, 0o600) != 0) {
        live.failure = "battle_fork_control";
        return 0;
    }
    forkCheckpointRecord(n);
    // Hold a read end for the entire checkpoint lifetime, including across forks.
    const control_fd = posix.open(&control, posix.O_RDWR);
    const in_file = if (control_fd >= 0) fdopen(control_fd, "r") orelse _exit(4) else _exit(4);
    while (fgets(&command, command.len, in_file) != null) {
        command[strcspn(&command, "\r\n")] = 0;
        if (strcmp(&command, "exit") == 0) _exit(0);
        if (strncmp(&command, "fork ", 5) != 0) continue;
        const name: [*c]u8 = @as([*c]u8, &command) + 5;
        var turn = strchr(name, ' ');
        if (turn == null or turn == name or @intFromPtr(turn) - @intFromPtr(name) >= battle_fork.node.len) continue;
        turn.* = 0;
        turn += 1;
        if (strspn(name, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.") != strlen(name) or
            strlen(turn) >= size) continue;
        var node: [1200]u8 = undefined;
        if (!forkPath(&node, node.len, name, null) or posix.mkdir(&node, 0o755) != 0) continue;
        const child = posix.fork();
        if (child == 0) {
            _ = fclose(in_file);
            if (!forkBecome(name)) _exit(4);
            _ = strcpy(line, turn);
            return 1;
        }
        var marker: [1200]u8 = undefined;
        const note = if (forkPath(&marker, marker.len, name, if (child < 0) "fork_failed" else "pid")) fopen(&marker, "wb") else null;
        if (note) |f| {
            _ = fprintf(f, "%ld\n", @as(c_long, child));
            _ = fclose(f);
        }
    }
    _exit(4);
}
