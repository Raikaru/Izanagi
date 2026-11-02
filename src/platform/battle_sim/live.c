/* Natural walk worker: observation only; the game owns its VM, input and RNG. */
#define _POSIX_C_SOURCE 200809L
#include "p4pc/battle_live.h"
#include "p4pc/battle_sim.h"
#include "p4pc/battle_events.h"
#include "p4pc/content_digest.h"
#include "p4pc/crt.h"
#include "p4pc/gmem.h"
#include "p4pc/mod_fidelity.h"
#include "p4pc/platform.h"
#include "p4pc/walk_script.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_START 0x00100000u
#define RAM_BYTES 0x01f00000u
#define GP(off) P4_GPDATA(uint32_t, off)

typedef struct LiveUnit {
    uint32_t address, record, uid, status;
    uint16_t id, record_id, hp, sp;
    uint8_t genus;
} LiveUnit;

int p4_battle_live_active;
static struct {
    P4BattleLiveEntry entry;
    FILE *trace;
    const char *result;
    char ram_path[2048], ram_sha[65];
    uint8_t *ram;
    uint32_t observer, updates, fight_updates, requests, matching;
    uint32_t pending_flip, pending_frame, pending_draws, request_flip, request_frame;
    uint32_t request_draws, task_setup, battle, actions, field_task, field_actor, field_group, script_group, unbound_requests;
    uint64_t admissions[2], costs_hp, costs_sp, items;
    uint64_t item_counts[1536];
    P4BattleHit lethal;
    uint32_t lethal_update;
    int lethal_seen, terminal_lethal, first_command;
    uint32_t terminal, terminal_update, terminal_flip, terminal_frame, terminal_draws;
    LiveUnit terminal_units[2][12];
    unsigned terminal_counts[2];
    int pending, selected, task_begun, bound, seen, terminal_seen, published;
    int qualification_terminal_present;
    const char *failure, *battle_cause, *battle_outcome;
} live;

/* Discovery is bounded and records only supported native request paths.
 * A constructor without an exact pending request is never guessed. */
static struct {
    uint16_t occurrences[3][65536];
    uint32_t records, ordinal, mode, id, occurrence, group, field_task, actor, setup;
    uint32_t battle, fights, commands, fight_updates, actions;
    int pending, identified, task_begun, active, first_command;
} discovery;

static void discovery_command_ready(uint32_t action);

extern uint32_t p4pc_rng_draws;
extern int p4pc_pad_walk_status(void);
extern int16_t datGetDateIndex(void);
extern int32_t FldStepIndex(void);
extern int16_t datGetPartyId(int32_t slot);
extern void *datUnitGetRecord(int16_t id);
extern uint16_t datCalcGetHp(void *unit);
extern uint16_t datCalcGetSp(void *unit);
extern uint16_t datCalcMaxHpById(int16_t id);
extern uint16_t datCalcMaxSpById(int16_t id);
extern uint32_t datCalcIsDead(void *unit, int32_t hp_delta);

static int range(uint32_t p, uint32_t bytes)
{
    return p >= RAM_START && p <= 0x02000000u && bytes <= 0x02000000u - p;
}
static int setup_range(uint32_t p)
{
#ifdef P4_GUEST64
    return p >= RAM_START && p <= P4_GUEST_SIZE - 0x3cu;
#else
    /* The original constructor receives a native stack address on 32-bit. */
    return p >= RAM_START && p <= UINT32_MAX - 0x3cu;
#endif
}
static int exact_env(const char *key, const char *value)
{
    const char *actual = getenv(key);
    return actual && !strcmp(actual, value);
}
static int nonempty_env(const char *key)
{
    const char *value = getenv(key);
    return value && *value;
}
static int bad(const char *reason)
{
    fprintf(stderr, "p4pc: battle-live: %s\n", reason);
    return -1;
}
static int valid_environment(const P4BattleLiveEntry *entry)
{
    static const char *const forbidden[] = {
        "P4PC_MOD_WITNESS", "P4PC_START_BATTLE", "P4PC_BATTLE_DRIVER", "P4PC_AUTOPRESS",
        "P4PC_AUTOPRESS_NEWGAME", "P4PC_CAPTURE_FRAMES", "P4PC_CAPTURE_STATE",
        "P4PC_START_MENU", "P4PC_MENU_SCRIPT", "P4PC_TRAP_CONTINUE", "P4PC_OVERRIDE_DIR"
    };
    for (size_t i = 0; i < sizeof forbidden / sizeof *forbidden; i++) if (getenv(forbidden[i])) return 0;
    if (!nonempty_env("P4PC_SAVE_DIR") || !nonempty_env("P4PC_CONFIG") || !nonempty_env("P4PC_LOCKSTEP_DIR") ||
        !exact_env("P4PC_INPUT", "script") || !exact_env("P4PC_MODS", "0") ||
        !exact_env("P4PC_PACKS", "0") || !exact_env("P4PC_VIRTUAL_CLOCK", "1") ||
        !exact_env("P4PC_TURBO", "2")) return 0;
    const int replay = getenv("P4PC_LOCKSTEP_REPLAY") != NULL;
    if (entry->mode == 3 && replay) return 0;
    if (replay) {
        if (!nonempty_env("P4PC_LOCKSTEP_REPLAY")) return 0;
        if (getenv("P4PC_WALK_SCRIPT") || getenv("P4PC_AUTOPRESS_LOAD")) return 0;
        if (entry->completion == 2 && !nonempty_env("P4PC_LOCKSTEP_STOP")) return 0;
    } else {
        const char *walk = getenv("P4PC_WALK_SCRIPT");
        if (!walk || !*walk || !exact_env("P4PC_AUTOPRESS_LOAD", "0") ||
            !exact_env("P4PC_AUTOPRESS_FRAMES", "1")) return 0;
        if (!!getenv("P4PC_WALK_GAME") == !!(entry->flags & 2u)) return 0;
    }
    return 1;
}
static void snapshot(void)
{
    memcpy(live.ram, P4_G2H(RAM_START), RAM_BYTES);
    live.pending_flip = p4pc_flip_count();
    live.pending_frame = P4_G32(0x00764290u);
    live.pending_draws = p4pc_rng_draws;
    live.pending = 1;
}
static uint32_t rng_word(uint32_t address, int captured)
{
    if (!captured) return P4_G32(address);
    const uint8_t *p = live.ram + address - RAM_START;
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static int trace_rng(const char *kind, int captured)
{
    const uint32_t active = rng_word(0x00764708u, captured);
    if (active > 0x10000u || !range(0x008872e0u + active, 16)) return 0;
    const uint32_t slot = 0x008872e0u + active;
    const uint32_t base = rng_word(slot, captured), front = rng_word(slot + 4, captured);
    const uint32_t back = rng_word(slot + 8, captured), end = rng_word(slot + 12, captured);
    if (!range(base, 124) || end != base + 124 || front < base || front >= end ||
        back < base || back >= end || ((front - base) & 3) || ((back - base) & 3)) return 0;
    fprintf(live.trace, "{\"kind\":\"%s\",\"flip\":%u,\"game_frame\":%u,\"additive_draws\":%u,"
            "\"active_slot_offset\":%u,\"front_byte_offset\":%u,\"back_byte_offset\":%u,\"additive\":[",
            kind, captured ? live.pending_flip : p4pc_flip_count(),
            captured ? live.pending_frame : P4_G32(0x00764290u),
            captured ? live.pending_draws : p4pc_rng_draws, active, front - base, back - base);
    for (unsigned i = 0; i < 31; i++) fprintf(live.trace, "%s%u", i ? "," : "", rng_word(base + 4 * i, captured));
    fputs("],\"mt\":[", live.trace);
    for (unsigned i = 0; i < 624; i++) fprintf(live.trace, "%s%u", i ? "," : "", rng_word(0x00885a90u + 4 * i, captured));
    fprintf(live.trace, "],\"mt_index\":%u,\"effect\":[", rng_word(0x00763b6cu, captured));
    for (unsigned i = 0; i < 4; i++) fprintf(live.trace, "%s%u", i ? "," : "", rng_word(0x00922de0u + 4 * i, captured));
    fputs("]}\n", live.trace);
    return !ferror(live.trace);
}
static void publish_snapshot(uint32_t mode, uint32_t encounter, uint32_t object, uint32_t value)
{
    if (!live.pending) { live.failure = "request_snapshot_missing"; return; }
    p4_content_digest(live.ram, RAM_BYTES, live.ram_sha);
    FILE *file = fopen(live.ram_path, "wbx");
    if (!file) { live.failure = "request_ram_create"; return; }
    int written = fwrite(live.ram, 1, RAM_BYTES, file) == RAM_BYTES && !ferror(file);
    if (fclose(file)) written = 0;
    if (!written) { live.failure = "request_ram_write"; return; }
    live.request_flip = live.pending_flip;
    live.request_frame = live.pending_frame;
    live.request_draws = live.pending_draws;
    live.selected = 1;
    fprintf(live.trace, "{\"kind\":\"natural_request\",\"mode\":%u,\"encounter_id\":%u,\"raw_object\":%u,\"raw_value\":%u,\"request_ordinal\":%u,\"matching_occurrence\":%u,\"flip\":%u,\"game_frame\":%u,\"additive_draws_at_request\":%u,\"ram_start\":%u,\"ram_bytes\":%u,\"ram_sha256\":\"%s\",\"rng_mode\":\"inherit-native\"}\n",
            mode, encounter, object, value, live.requests, live.matching, live.request_flip,
            live.request_frame, live.request_draws, RAM_START, RAM_BYTES, live.ram_sha);
    if (!trace_rng("request_rng", 1)) { live.failure = "request_rng_snapshot"; return; }
    if (ferror(live.trace)) live.failure = "request_trace_write";
}
static void request(const P4BattleRequest *event, void *unused)
{
    (void)unused;
    if (live.failure || live.terminal_seen) return;
    if (live.selected && !live.bound &&
        (event->stage == P4_BATTLE_SCRIPT_REQUEST_BEGIN || event->stage == P4_BATTLE_FIELD_REQUEST_BEGIN ||
         event->stage == P4_BATTLE_COLLISION_READY)) {
        live.failure = "intervening_request_before_selected_task";
        return;
    }
    if (event->stage == P4_BATTLE_SCRIPT_REQUEST_BEGIN) {
        live.requests++;
        live.unbound_requests++;
        if (!live.selected && live.entry.mode == 1) snapshot();
    } else if (event->stage == P4_BATTLE_SCRIPT_REQUEST_IDENTIFIED) {
        if (!live.selected && live.entry.mode == 1 && event->encounter_id == live.entry.encounter &&
            ++live.matching == live.entry.occurrence)
            publish_snapshot(1, event->encounter_id, 0, 0);
        live.pending = 0;
    } else if (event->stage == P4_BATTLE_FIELD_REQUEST_BEGIN) {
        live.requests++;
        live.unbound_requests++;
        if (!live.selected && live.entry.mode == 2 && (event->encounter_id & 0xffffu) == live.entry.encounter &&
            ++live.matching == live.entry.occurrence) {
            snapshot();
            live.field_task = event->guest_object;
            publish_snapshot(2, event->encounter_id, event->guest_object, event->value);
            live.pending = 0;
        }
    } else if (event->stage == P4_BATTLE_COLLISION_READY) {
        live.requests++;
        live.unbound_requests++;
        if (!live.selected && live.entry.mode == 4 && event->encounter_id == live.entry.encounter &&
            ++live.matching == live.entry.occurrence) {
            if (!range(event->guest_object, 0x3c)) { live.failure = "invalid_collision_task"; return; }
            const uint32_t work = P4_G32(event->guest_object + 0x38);
            if (!range(work, 0x110) || event->value != work + 0x50 || P4_G32(work + 0x10c)) {
                live.failure = "collision_setup_mismatch"; return;
            }
            live.field_task = event->guest_object;
            live.field_actor = P4_G32(work + 0x44);
            live.field_group = P4_G32(event->value + 0x14);
            if (!range(live.field_actor, 0x4c) || !range(live.field_group, 10) ||
                P4_G32(live.field_actor + 0x48) != live.field_group ||
                P4_G16(live.field_group + 8) != live.entry.encounter) {
                live.failure = "collision_group_mismatch"; return;
            }
            snapshot();
            publish_snapshot(4, event->encounter_id, event->guest_object, event->value);
            if (!live.failure) {
                fprintf(live.trace, "{\"kind\":\"collision_preconstructor\",\"task\":%u,\"setup\":%u,\"actor\":%u,\"group\":%u,\"encounter_id\":%u,\"flip\":%u,\"game_frame\":%u}\n",
                        live.field_task, event->value, live.field_actor, live.field_group, event->encounter_id,
                        live.request_flip, live.request_frame);
                if (ferror(live.trace)) live.failure = "collision_trace_write";
            }
            live.pending = 0;
        }
    } else if (event->stage == P4_BATTLE_SCRIPT_GROUP_CREATED && live.selected && live.entry.mode == 1 && !live.bound) {
        if (event->encounter_id != live.entry.encounter || live.script_group || !range(event->guest_object, 10) ||
            P4_G16(event->guest_object + 8) != live.entry.encounter) {
            live.failure = "script_group_identity_invalid"; return;
        }
        live.script_group = event->guest_object;
        fprintf(live.trace, "{\"kind\":\"script_group_created\",\"group\":%u,\"encounter_id\":%u}\n",
                live.script_group, event->encounter_id);
        if (ferror(live.trace)) live.failure = "script_group_trace_write";
    } else if (event->stage == P4_BATTLE_FIELD_GROUP_CREATED && live.selected && live.entry.mode == 2 && !live.bound) {
        if (event->guest_object != live.field_task || (event->encounter_id & 0xffffu) != live.entry.encounter ||
            live.field_group || !range(live.field_task, 0x3c) || !range(event->value, 10)) {
            live.failure = "field_group_identity_invalid"; return;
        }
        const uint32_t work = P4_G32(live.field_task + 0x38);
        if (!range(work, 0x48)) { live.failure = "invalid_field_work"; return; }
        live.field_actor = P4_G32(work + 0x44);
        if (!range(live.field_actor, 0x4c) || P4_G32(live.field_actor + 0x48) != event->value ||
            P4_G16(event->value + 8) != live.entry.encounter) {
            live.failure = "field_group_identity_invalid"; return;
        }
        live.field_group = event->value;
        fprintf(live.trace, "{\"kind\":\"field_group_created\",\"actor\":%u,\"group\":%u,\"encounter_id\":%u}\n",
                live.field_actor, live.field_group, live.entry.encounter);
        if (ferror(live.trace)) live.failure = "field_group_trace_write";
    } else if (event->stage == P4_BATTLE_TASK_BEGIN) {
        if (!live.selected) {
            if (live.unbound_requests) live.unbound_requests--;
            return;
        }
        if (live.bound) return;
        if (live.task_begun || live.unbound_requests != 1) { live.failure = "ambiguous_task_construction"; return; }
        live.unbound_requests--;
        live.task_begun = 1;
        live.task_setup = event->guest_object;
        if (!live.task_setup || !setup_range(live.task_setup)) { live.failure = "invalid_task_setup"; return; }
        uint32_t expected_group = live.script_group;
        if (live.entry.mode == 2 || live.entry.mode == 4) {
            if (!range(live.field_task, 0x3c)) { live.failure = "invalid_field_task"; return; }
            const uint32_t work = P4_G32(live.field_task + 0x38);
            if (!range(work, 0x8c) || live.task_setup != work + 0x50) {
                live.failure = "field_task_setup_mismatch"; return;
            }
            if (P4_G32(work + 0x44) != live.field_actor || !range(live.field_actor, 0x4c) ||
                P4_G32(live.field_actor + 0x48) != live.field_group) {
                live.failure = "field_actor_or_group_changed"; return;
            }
            expected_group = live.field_group;
        }
        const uint32_t enemies = P4_G32(live.task_setup + 0x14);
        if (!expected_group || enemies != expected_group || !range(enemies, 10) ||
            P4_G16(enemies + 8) != live.entry.encounter) {
            live.failure = "encounter_group_mismatch"; return;
        }
        if (!trace_rng("task_begin_rng", 0)) { live.failure = "task_rng_snapshot"; return; }
        fprintf(live.trace, "{\"kind\":\"task_begin\",\"flip\":%u,\"game_frame\":%u,\"setup\":%u,\"setup_hex\":",
                p4pc_flip_count(), P4_G32(0x00764290u), live.task_setup);
        if (!live.task_setup) fputs("null", live.trace);
        else {
            const uint8_t *p = P4_G2H(live.task_setup);
            fputc('"', live.trace);
            for (unsigned i = 0; i < 0x3c; i++) fprintf(live.trace, "%02x", p[i]);
            fputc('"', live.trace);
        }
        fputs("}\n", live.trace);
        if (ferror(live.trace)) live.failure = "task_trace_write";
    } else if (event->stage == P4_BATTLE_TASK_CREATED && live.selected && !live.bound) {
        if (!live.task_begun || event->guest_object != live.task_setup || !event->value || !range(GP(0xb3ac), 0xe88)) {
            live.failure = "unbound_native_task"; return;
        }
        live.battle = GP(0xb3ac);
        live.bound = 1;
        fprintf(live.trace, "{\"kind\":\"task_created\",\"flip\":%u,\"game_frame\":%u,\"task\":%u,\"battle_context\":%u}\n",
                p4pc_flip_count(), P4_G32(0x00764290u), event->value, live.battle);
        if (!trace_rng("task_created_rng", 0)) { live.failure = "task_rng_snapshot"; return; }
        if (ferror(live.trace)) live.failure = "task_trace_write";
    }
}
static void admission(const P4BattleAdmission *event, void *unused)
{
    (void)unused;
    if (!live.bound || live.terminal_seen) return;
    live.admissions[event->queue_kind == P4_BATTLE_QUEUE_FOLLOWUP]++;
    fprintf(live.trace, "{\"kind\":\"admission\",\"update\":%u,\"queue_kind\":%u,\"id\":%u,\"side\":%u,\"action_uid\":\"%016" PRIx64 "\"}\n",
            live.updates + 1, event->queue_kind, event->actor.native_id, event->actor.side, event->actor.action_uid);
    if (ferror(live.trace)) live.failure = "admission_trace_write";
}
static void command(const P4BattleCommand *event, void *unused)
{
    (void)unused;
    if (!live.bound || live.terminal_seen) return;
    if (!event->free_action) live.actions++;
    fprintf(live.trace, "{\"kind\":\"command_commit\",\"update\":%u,\"id\":%u,\"side\":%u,\"action_kind\":%u,\"skill_id\":%u,\"item_id\":%u,\"targets\":%u,\"free_action\":%u}\n",
            live.updates + 1, event->actor.native_id, event->actor.side, event->action_kind,
            event->skill_id, event->item_id, event->target_count, event->free_action);
    if (ferror(live.trace)) live.failure = "command_trace_write";
}
static void cost(const P4BattleCost *event, void *unused)
{
    (void)unused;
    if (!live.bound || live.terminal_seen || event->actor.side != 0) return;
    if (event->kind == P4_BATTLE_COST_HP) live.costs_hp += event->requested_cost;
    else if (event->kind == P4_BATTLE_COST_SP) live.costs_sp += event->requested_cost;
    else if (event->kind == P4_BATTLE_COST_ITEM && event->item_count_before > event->item_count_after) {
        const uint32_t used = event->item_count_before - event->item_count_after;
        live.items += used;
        if (event->item_id < 1536) live.item_counts[event->item_id] += used;
    }
    fprintf(live.trace, "{\"kind\":\"cost_commit\",\"update\":%u,\"id\":%u,\"cost_kind\":%u,\"requested\":%u,\"item_id\":%u,\"hp_before\":%u,\"hp_after\":%u,\"sp_before\":%u,\"sp_after\":%u,\"item_before\":%u,\"item_after\":%u}\n",
            live.updates + 1, event->actor.native_id, event->kind, event->requested_cost, event->item_id,
            event->hp_before, event->hp_after, event->sp_before, event->sp_after,
            event->item_count_before, event->item_count_after);
    if (ferror(live.trace)) live.failure = "cost_trace_write";
}
static void hit(const P4BattleHit *event, void *unused)
{
    (void)unused;
    if (!live.bound || live.terminal_seen) return;
    if (event->target.side == 0 && event->target.native_id == 1) {
        if (event->hp_before && !event->hp_after) {
            live.lethal = *event;
            live.lethal_update = live.updates + 1;
            live.lethal_seen = 1;
        } else if (event->hp_after) live.lethal_seen = 0;
    }
    fprintf(live.trace, "{\"kind\":\"target_commit\",\"update\":%u,\"source_id\":%u,\"source_side\":%u,\"target_id\":%u,\"target_side\":%u,\"skill_id\":%u,\"hp_delta\":%" PRId32 ",\"sp_delta\":%" PRId32 ",\"hp_before\":%u,\"hp_after\":%u,\"sp_before\":%u,\"sp_after\":%u,\"result_flags\":%u,\"added_status\":%u,\"resources_applied\":%u}\n",
            live.updates + 1, event->source.native_id, event->source.side, event->target.native_id,
            event->target.side, event->skill_id, event->requested_hp_delta, event->requested_sp_delta,
            event->hp_before, event->hp_after, event->sp_before, event->sp_after,
            event->result_flags, event->added_status, event->resources_applied);
    if (ferror(live.trace)) live.failure = "target_trace_write";
}

void p4_battle_live_command_ready(uint32_t action)
{
    if (live.entry.mode == 3) { discovery_command_ready(action); return; }
    if (!live.bound || live.terminal_seen || live.first_command || !range(action, 0x34)) return;
    const uint32_t unit = P4_G32(action + 0x30);
    if (!range(unit, 0xa6) || P4_G8(unit + 0xa2) != 0 || P4_G16(unit + 0xa4) != 1) return;
    live.first_command = 1;
    fprintf(live.trace, "{\"kind\":\"first_protagonist_command\",\"update\":%u,\"flip\":%u,\"game_frame\":%u,\"party\":[",
            live.updates + 1, p4pc_flip_count(), P4_G32(0x00764290u));
    int32_t ids[4] = {1, datGetPartyId(0) & 65535, datGetPartyId(1) & 65535, datGetPartyId(2) & 65535};
    unsigned seen = 0, count = 0;
    for (unsigned i = 0; i < 4; i++) {
        const int32_t id = ids[i];
        if (id < 1 || id > 10 || (seen & (1u << id))) continue;
        seen |= 1u << id;
        void *record = datUnitGetRecord((int16_t)id);
        fprintf(live.trace, "%s{\"id\":%d,\"hp\":%u,\"sp\":%u,\"max_hp\":%u,\"max_sp\":%u}",
                count++ ? "," : "", id, datCalcGetHp(record), datCalcGetSp(record),
                datCalcMaxHpById(id), datCalcMaxSpById(id));
    }
    fputs("]}\n", live.trace);
    if (ferror(live.trace)) live.failure = "first_command_trace_write";
}
/* Copy only linked units. An empty list or a missing record is not a death. */
static int collect_units(uint32_t battle, LiveUnit units[2][12], unsigned counts[2])
{
    for (unsigned group = 0; group < 2; group++) {
        uint32_t unit = P4_G32(battle + 0x178 + group * 8);
        counts[group] = 0;
        while (unit && counts[group] < 12) {
            if (!range(unit, 0xa70)) return 0;
            LiveUnit *out = &units[group][counts[group]++];
            *out = (LiveUnit){0};
            out->address = unit;
            out->genus = P4_G8(unit + 0xa2);
            out->id = P4_G16(unit + 0xa4);
            out->uid = P4_G32(unit + 0xa8);
            out->record = P4_G32(unit + 0xa64);
            if (out->record) {
                if (!range(out->record, 0x10)) return 0;
                out->record_id = P4_G16(out->record + 2);
                out->hp = P4_G16(out->record + 8);
                out->sp = P4_G16(out->record + 10);
                out->status = P4_G32(out->record + 12);
            }
            unit = P4_G32(unit + 0xa6c);
        }
        if (unit) return 0;
    }
    return 1;
}
static void write_units(FILE *file, const LiveUnit units[2][12], const unsigned counts[2])
{
    fputc('[', file);
    for (unsigned group = 0; group < 2; group++) {
        if (group) fputc(',', file);
        fputc('[', file);
        for (unsigned i = 0; i < counts[group]; i++) {
            const LiveUnit *u = &units[group][i];
            fprintf(file, "%s{\"guest_unit\":%u,\"uid\":%u,\"genus\":%u,\"id\":%u,\"guest_record\":",
                    i ? "," : "", u->address, u->uid, u->genus, u->id);
            if (u->record)
                fprintf(file, "%u,\"record_id\":%u,\"hp\":%u,\"sp\":%u,\"status\":%u}",
                        u->record, u->record_id, u->hp, u->sp, u->status);
            else fputs("null,\"record_id\":null,\"hp\":null,\"sp\":null,\"status\":null}", file);
        }
        fputc(']', file);
    }
    fputc(']', file);
}

static int endpoint(uint32_t context[4])
{
    const uint32_t field = GP(0x9db0);
    if (!range(field, 8)) return 0;
    context[0] = P4_G32(field);
    context[1] = P4_G32(field + 4);
    context[2] = (uint16_t)datGetDateIndex();
    context[3] = (uint32_t)FldStepIndex();
    const uint32_t expected[] = {live.entry.field, live.entry.room, live.entry.date, live.entry.step};
    for (unsigned i = 0; i < 4; i++) if ((live.entry.endpoint_mask & (1u << i)) && context[i] != expected[i]) return 0;
    return 1;
}

/* Discovery emits bounded evidence, not a selected or synthesized encounter. */
static void discovery_check_write(void)
{
    long position = ftell(live.trace);
    if (ferror(live.trace) || position < 0) live.failure = "discovery_trace_write";
    else if (position > 8 * 1024 * 1024 || ++discovery.records > 8192)
        live.failure = "discovery_trace_limit";
}
static int discovery_can_write(void)
{
    long position = ftell(live.trace);
    if (position < 0 || ferror(live.trace)) { live.failure = "discovery_trace_write"; return 0; }
    /* Reserve more than the maximum line containing 24 bounded unit records. */
    if (discovery.records >= 8192 || position > 8 * 1024 * 1024 - 8192) {
        live.failure = "discovery_trace_limit"; return 0;
    }
    return 1;
}
static void discovery_event(const char *kind, uint32_t object, uint32_t value)
{
    if (!discovery_can_write()) return;
    const uint32_t field = GP(0x9db0);
    fprintf(live.trace, "{\"kind\":\"%s\",\"ordinal\":%u,\"mode\":%u,\"encounter_id\":",
            kind, discovery.ordinal, discovery.mode);
    if (discovery.identified) fprintf(live.trace, "%u", discovery.id); else fputs("null", live.trace);
    fprintf(live.trace, ",\"occurrence\":%u,\"object\":%u,\"value\":%u,\"flip\":%u,\"game_frame\":%u,\"field\":",
            discovery.occurrence, object, value, p4pc_flip_count(), P4_G32(0x00764290u));
    if (range(field, 8))
        fprintf(live.trace, "%u,\"room\":%u", P4_G32(field), P4_G32(field + 4));
    else fputs("null,\"room\":null", live.trace);
    fprintf(live.trace, ",\"date\":%u,\"step\":%u}\n", (uint16_t)datGetDateIndex(), (uint32_t)FldStepIndex());
    discovery_check_write();
}
static void discovery_identify(uint32_t id)
{
    discovery.id = id & 0xffffu;
    discovery.identified = 1;
    discovery.occurrence = ++discovery.occurrences[discovery.mode == 4 ? 2 : discovery.mode - 1][discovery.id];
}
static void discovery_request(const P4BattleRequest *event, void *unused)
{
    (void)unused;
    if (live.failure) return;
    if (event->stage == P4_BATTLE_SCRIPT_REQUEST_BEGIN || event->stage == P4_BATTLE_FIELD_REQUEST_BEGIN) {
        if (discovery.pending || discovery.active) { live.failure = "discovery_ambiguous_request"; return; }
        discovery.ordinal++;
        discovery.mode = event->stage == P4_BATTLE_SCRIPT_REQUEST_BEGIN ? 1 : 2;
        discovery.pending = 1;
        discovery.identified = discovery.task_begun = 0;
        discovery.id = discovery.occurrence = discovery.group = discovery.actor = discovery.setup = 0;
        discovery.field_task = event->guest_object;
        if (discovery.mode == 2) discovery_identify(event->encounter_id);
        discovery_event("request_begin", event->guest_object, event->value);
    } else if (event->stage == P4_BATTLE_COLLISION_READY) {
        if (discovery.pending || discovery.active || !range(event->guest_object, 0x3c)) {
            live.failure = "discovery_ambiguous_collision"; return;
        }
        const uint32_t work = P4_G32(event->guest_object + 0x38);
        if (!range(work, 0x110) || event->value != work + 0x50 || P4_G32(work + 0x10c)) {
            live.failure = "discovery_collision_setup_mismatch"; return;
        }
        discovery.ordinal++;
        discovery.mode = 4;
        discovery.pending = 1;
        discovery.identified = discovery.task_begun = 0;
        discovery.field_task = event->guest_object;
        discovery.actor = P4_G32(work + 0x44);
        discovery.group = P4_G32(event->value + 0x14);
        if (!range(discovery.actor, 0x4c) || !range(discovery.group, 10) ||
            P4_G32(discovery.actor + 0x48) != discovery.group ||
            P4_G16(discovery.group + 8) != event->encounter_id) {
            live.failure = "discovery_collision_group_mismatch"; return;
        }
        discovery_identify(event->encounter_id);
        discovery_event("collision_preconstructor", event->guest_object, event->value);
        if (!live.failure) discovery_event("group_created", discovery.group, discovery.actor);
    } else if (event->stage == P4_BATTLE_SCRIPT_REQUEST_IDENTIFIED) {
        if (!discovery.pending || discovery.mode != 1 || discovery.identified) {
            live.failure = "discovery_unpaired_identification"; return;
        }
        discovery_identify(event->encounter_id);
        discovery_event("request_identified", 0, 0);
    } else if (event->stage == P4_BATTLE_SCRIPT_GROUP_CREATED) {
        if (!discovery.pending || discovery.mode != 1 || !discovery.identified || discovery.group ||
            event->encounter_id != discovery.id || !range(event->guest_object, 10) ||
            P4_G16(event->guest_object + 8) != discovery.id) {
            live.failure = "discovery_script_group_mismatch"; return;
        }
        discovery.group = event->guest_object;
        discovery_event("group_created", discovery.group, 0);
    } else if (event->stage == P4_BATTLE_FIELD_GROUP_CREATED) {
        if (!discovery.pending || discovery.mode != 2 || discovery.group ||
            event->guest_object != discovery.field_task || (event->encounter_id & 0xffffu) != discovery.id ||
            !range(discovery.field_task, 0x3c) || !range(event->value, 10)) {
            live.failure = "discovery_field_group_mismatch"; return;
        }
        const uint32_t work = P4_G32(discovery.field_task + 0x38);
        if (!range(work, 0x48)) { live.failure = "discovery_invalid_field_work"; return; }
        discovery.actor = P4_G32(work + 0x44);
        if (!range(discovery.actor, 0x4c) || P4_G32(discovery.actor + 0x48) != event->value ||
            P4_G16(event->value + 8) != discovery.id) {
            live.failure = "discovery_field_actor_mismatch"; return;
        }
        discovery.group = event->value;
        discovery_event("group_created", discovery.group, discovery.actor);
    } else if (event->stage == P4_BATTLE_TASK_BEGIN) {
        if (!discovery.pending || !discovery.identified || !discovery.group || discovery.task_begun ||
            !event->guest_object || !setup_range(event->guest_object)) {
            live.failure = "discovery_unsupported_or_ambiguous_task"; return;
        }
        discovery.setup = event->guest_object;
        if (P4_G32(discovery.setup + 0x14) != discovery.group) {
            live.failure = "discovery_task_group_mismatch"; return;
        }
        if (discovery.mode == 2 || discovery.mode == 4) {
            if (!range(discovery.field_task, 0x3c)) { live.failure = "discovery_invalid_field_task"; return; }
            const uint32_t work = P4_G32(discovery.field_task + 0x38);
            if (!range(work, 0x8c) || discovery.setup != work + 0x50 ||
                P4_G32(work + 0x44) != discovery.actor || !range(discovery.actor, 0x4c) ||
                P4_G32(discovery.actor + 0x48) != discovery.group) {
                live.failure = "discovery_field_task_mismatch"; return;
            }
        }
        discovery.task_begun = 1;
        discovery_event("task_begin", discovery.setup, discovery.group);
    } else if (event->stage == P4_BATTLE_TASK_CREATED) {
        if (!discovery.pending || !discovery.task_begun || event->guest_object != discovery.setup ||
            !event->value || !range(GP(0xb3ac), 0xe88)) {
            live.failure = "discovery_unbound_task"; return;
        }
        discovery.pending = 0;
        discovery.active = 1;
        discovery.first_command = 0;
        discovery.fight_updates = discovery.actions = 0;
        discovery.battle = GP(0xb3ac);
        discovery.fights++;
        discovery_event("task_created", event->value, discovery.battle);
    }
}
static void discovery_command(const P4BattleCommand *event, void *unused)
{
    (void)unused;
    if (discovery.active && !event->free_action) discovery.actions++;
}
static void discovery_command_ready(uint32_t action)
{
    if (live.failure || !discovery.active || discovery.first_command || !range(action, 0x34)) return;
    const uint32_t unit = P4_G32(action + 0x30);
    if (!range(unit, 0xa6) || P4_G8(unit + 0xa2) != 0 || P4_G16(unit + 0xa4) != 1) return;
    LiveUnit units[2][12];
    unsigned counts[2];
    if (!collect_units(discovery.battle, units, counts)) { live.failure = "discovery_invalid_unit_list"; return; }
    if (!discovery_can_write()) return;
    discovery.first_command = 1;
    discovery.commands++;
    fprintf(live.trace, "{\"kind\":\"first_protagonist_command\",\"ordinal\":%u,\"mode\":%u,"
            "\"encounter_id\":%u,\"occurrence\":%u,\"flip\":%u,\"game_frame\":%u,\"linked_units\":",
            discovery.ordinal, discovery.mode, discovery.id, discovery.occurrence,
            p4pc_flip_count(), P4_G32(0x00764290u));
    write_units(live.trace, units, counts);
    fputs("}\n", live.trace);
    discovery_check_write();
}
static int discovery_publish(const char *outcome, const char *cause, const uint32_t *context)
{
    if (live.published) return 0;
    char temp[2048];
    const int length = snprintf(temp, sizeof temp, "%s.tmp", live.result);
    if (length < 0 || (size_t)length >= sizeof temp) return -1;
    FILE *file = fopen(temp, "wbx");
    if (!file) return -1;
    fprintf(file, "{\"schema\":1,\"entry_kind\":\"live-discovery\",\"outcome\":\"%s\","
            "\"cause\":\"%s\",\"requests\":%u,\"bound_fights\":%u,\"first_commands\":%u,"
            "\"trace_records\":%u,\"route_updates\":%u,\"absolute_flip_limit\":%u,"
            "\"final_flip\":%u,\"final_game_frame\":%u,\"endpoint\":",
            outcome, cause, discovery.ordinal, discovery.fights, discovery.commands,
            discovery.records, live.updates, live.entry.occurrence,
            p4pc_flip_count(), P4_G32(0x00764290u));
    if (context) fprintf(file, "{\"field\":%u,\"room\":%u,\"date\":%u,\"step\":%u}",
                         context[0], context[1], context[2], context[3]);
    else fputs("null", file);
    fputs("}\n", file);
    int failed = ferror(file);
    if (fclose(file)) failed = 1;
    if (live.trace && fclose(live.trace)) failed = 1;
    live.trace = NULL;
    if (failed || rename(temp, live.result)) return -1;
    live.published = 1;
    return 0;
}
static int publish(const char *outcome, const char *cause, const uint32_t *context)
{
    if (live.entry.mode == 3) return discovery_publish(outcome, cause, context);
    if (live.published) return 0;
    char temp[2048];
    const int length = snprintf(temp, sizeof temp, "%s.tmp", live.result);
    if (length < 0 || (size_t)length >= sizeof temp) return -1;
    FILE *file = fopen(temp, "wbx");
    if (!file) return -1;
    fprintf(file, "{\"schema\":1,\"entry_kind\":\"live-natural\",\"outcome\":\"%s\",\"terminal_cause\":\"%s\",\"rng_mode\":\"inherit-native\",\"selected\":%s,\"task_bound\":%s,\"route_updates\":%u,\"fight_updates\":%u,\"encounter_id\":%u,\"request_mode\":%u,\"matching_occurrence\":%u,\"request_ram_sha256\":",
            outcome, cause, live.selected ? "true" : "false", live.bound ? "true" : "false",
            live.updates, live.fight_updates, live.entry.encounter, live.entry.mode, live.entry.occurrence);
    if (live.selected) fprintf(file, "\"%s\"", live.ram_sha); else fputs("null", file);
    fprintf(file, ",\"fight_terminal\":");
    if (live.terminal_seen)
        fprintf(file, "{\"native_result\":%u,\"outcome\":\"%s\",\"cause\":\"%s\",\"update\":%u,\"flip\":%u,\"game_frame\":%u,\"additive_draws\":%u}",
                live.terminal, live.battle_outcome, live.battle_cause, live.terminal_update,
                live.terminal_flip, live.terminal_frame, live.terminal_draws);
    else fputs("null", file);
    fputs(",\"terminal_linked_units\":", file);
    if (live.terminal_seen) write_units(file, live.terminal_units, live.terminal_counts);
    else fputs("null", file);
    fprintf(file, ",\"last_protagonist_lethal\":");
    if (live.terminal_lethal)
        fprintf(file, "{\"update\":%u,\"skill_id\":%u,\"source_id\":%u,\"hp_delta\":%" PRId32 ",\"result_flags\":%u,\"added_status\":%u}",
                live.lethal_update, live.lethal.skill_id, live.lethal.source.native_id,
                live.lethal.requested_hp_delta, live.lethal.result_flags, live.lethal.added_status);
    else fputs("null", file);
    fprintf(file, ",\"first_protagonist_command_observed\":%s,\"turns\":{\"ordinary_admissions\":%" PRIu64 ",\"followup_admissions\":%" PRIu64 ",\"actions\":%u},\"resources\":{\"hp_spent\":%" PRIu64 ",\"sp_spent\":%" PRIu64 ",\"items_consumed\":%" PRIu64 ",\"items_by_id\":[",
            live.first_command ? "true" : "false", live.admissions[0], live.admissions[1], live.actions,
            live.costs_hp, live.costs_sp, live.items);
    int first_item = 1;
    for (unsigned id = 0; id < 1536; id++) if (live.item_counts[id]) {
        fprintf(file, "%s{\"id\":%u,\"used\":%" PRIu64 "}", first_item ? "" : ",", id, live.item_counts[id]);
        first_item = 0;
    }
    fputs("]},\"route_endpoint\":", file);
    if (context) fprintf(file, "{\"field\":%u,\"room\":%u,\"date\":%u,\"step\":%u}", context[0], context[1], context[2], context[3]);
    else fputs("null", file);
    fprintf(file, ",\"final_flip\":%u,\"final_game_frame\":%u,\"final_additive_draws\":%u}\n",
            p4pc_flip_count(), P4_G32(0x00764290u), p4pc_rng_draws);
    int failed = ferror(file);
    if (fclose(file)) failed = 1;
    if (live.trace && fclose(live.trace)) failed = 1;
    live.trace = NULL;
    if (failed || rename(temp, live.result)) return -1;
    live.published = 1;
    return 0;
}
static void finish(const char *outcome, const char *cause, int code, const uint32_t *context)
{
    if (publish(outcome, cause, context)) { bad("result_publication"); exit(4); }
    p4pc_exit(code);
}
void p4_battle_live_before_present(void)
{
    if (live.entry.mode != 3) return;
    if (live.failure) finish("engine_error", live.failure, 4, NULL);
    if (p4pc_flip_count() >= live.entry.occurrence)
        finish("update_limit", "absolute_flip_limit", 3, NULL);
}
void p4_battle_live_after_present(void)
{
    if (live.qualification_terminal_present && live.terminal_seen)
        finish(live.battle_outcome, live.battle_cause, 0, NULL);
}
int p4_battle_live_start(const char *entry, const char *result, const char *trace)
{
    uint8_t bytes[64];
    if (!entry || !result || !trace) return bad("missing_paths");
    FILE *file = fopen(entry, "rb");
    if (!file) return bad("entry_open");
    int valid = fread(bytes, 1, sizeof bytes, file) == sizeof bytes && fgetc(file) == EOF && !ferror(file);
    if (fclose(file)) valid = 0;
    if (!valid || !p4_battle_live_decode(bytes, sizeof bytes, &live.entry)) return bad("invalid_entry");
    if (!valid_environment(&live.entry)) return bad("incompatible_environment");
    if (getenv("P4PC_LOCKSTEP_BATTLE_ANCHOR")) {
        if ((live.entry.mode != 2 && live.entry.mode != 4) || live.entry.occurrence != 1 || live.entry.completion != 1 ||
            !exact_env("P4PC_LOCKSTEP_STATE_ALIGNED", "1")) return bad("invalid_battle_qualification");
        live.qualification_terminal_present = 1;
    }
    FILE *existing = fopen(result, "rb");
    if (existing) { fclose(existing); return bad("result_exists"); }
    if (live.entry.mode != 3) {
        if (snprintf(live.ram_path, sizeof live.ram_path, "%s.request-ram", trace) >= (int)sizeof live.ram_path)
            return bad("trace_path_too_long");
        existing = fopen(live.ram_path, "rb");
        if (existing) { fclose(existing); return bad("request_ram_exists"); }
    }
    p4_mod_fidelity_check_start();
    if (live.entry.mode != 3) {
        live.ram = malloc(RAM_BYTES);
        if (!live.ram) return bad("request_ram_allocation");
    }
    live.trace = fopen(trace, "wbx");
    if (!live.trace) { free(live.ram); live.ram = NULL; return bad("trace_create"); }
    live.result = result;
    const P4BattleObserver observer = live.entry.mode == 3 ?
        (P4BattleObserver){NULL, NULL, NULL, NULL, discovery_command, discovery_request} :
        (P4BattleObserver){NULL, admission, cost, hit, command, request};
    live.observer = p4_battle_observer_add(&observer);
    if (!live.observer) { fclose(live.trace); live.trace = NULL; free(live.ram); live.ram = NULL; return bad("observer_registration"); }
    p4_battle_sim_renderless = !!(live.entry.flags & 1u);
    p4_battle_live_active = 1;
    p4_battle_sim_active = 1;
    return 1;
}
void p4_battle_live_before_update(void)
{
    if (live.qualification_terminal_present && live.terminal_seen)
        finish("engine_error", "terminal_frame_was_not_presented", 4, NULL);
    if (live.failure) finish("engine_error", live.failure, 4, NULL);
    if (live.updates >= live.entry.route_limit) finish("update_limit", "route_update_limit", 3, NULL);
    if (live.entry.mode == 3) {
        if (p4pc_flip_count() >= live.entry.occurrence)
            finish("update_limit", "absolute_flip_limit", 3, NULL);
        if (discovery.active && discovery.fight_updates >= live.entry.fight_limit)
            finish("update_limit", "fight_update_limit", 3, NULL);
        if (discovery.active && discovery.actions >= live.entry.action_limit)
            finish("action_limit", "native_action_limit", 3, NULL);
        return;
    }
    if (live.bound && !live.terminal_seen && live.fight_updates >= live.entry.fight_limit)
        finish("update_limit", "fight_update_limit", 3, NULL);
    if (live.bound && !live.terminal_seen && live.actions >= live.entry.action_limit)
        finish("action_limit", "native_action_limit", 3, NULL);
}
void p4_battle_live_after_update(int completed)
{
    if (!completed) return;
    live.updates++;
    if (live.failure) finish("engine_error", live.failure, 4, NULL);
    if (live.entry.mode == 3) {
        if (p4pc_flip_count() > live.entry.occurrence)
            finish("update_limit", "absolute_flip_limit", 3, NULL);
        const uint32_t battle = GP(0xb3ac);
        if (discovery.active) {
            discovery.fight_updates++;
            if (battle != discovery.battle || !range(battle, 0xe88))
                finish("engine_error", "discovery_battle_context_lost", 4, NULL);
            const uint32_t phase = P4_G32(battle + 0x2fc), flags = P4_G32(battle + 0xc);
            if (phase == 17 && !(flags & 1u)) {
                discovery_event("fight_terminal", P4_G16(battle + 0x1c), flags);
                if (live.failure) finish("engine_error", live.failure, 4, NULL);
                discovery.active = 0;
            }
        } else if (range(battle, 0xe88) && (P4_G32(battle + 0xc) & 1u)) {
            finish("engine_error", "discovery_unsupported_unbound_battle", 4, NULL);
        }
        if (p4pc_pad_walk_status() == P4WALK_DONE) {
            uint32_t context[4];
            if (discovery.pending || discovery.active || !discovery.fights || !discovery.commands)
                finish("engine_error", "discovery_incomplete_or_no_witness", 4, NULL);
            if (!endpoint(context)) finish("engine_error", "walk_endpoint_mismatch", 4, NULL);
            finish("route_complete", "native_walk_done_and_endpoint", 0, context);
        }
        if (p4pc_flip_count() >= live.entry.occurrence)
            finish("update_limit", "absolute_flip_limit", 3, NULL);
        return;
    }
    if (live.bound && !live.terminal_seen) {
        live.fight_updates++;
        const uint32_t battle = GP(0xb3ac);
        if (battle != live.battle || !range(battle, 0xe88)) finish("engine_error", "battle_context_lost_before_terminal", 4, NULL);
        live.seen = 1;
        const uint32_t phase = P4_G32(battle + 0x2fc), flags = P4_G32(battle + 0xc);
        const uint32_t terminal = P4_G16(battle + 0x1c);
        LiveUnit units[2][12];
        unsigned counts[2];
        if (!collect_units(battle, units, counts)) finish("engine_error", "invalid_linked_unit_list", 4, NULL);
        fprintf(live.trace, "{\"kind\":\"battle_update\",\"update\":%u,\"phase\":%u,\"flags\":%u,\"native_result\":%u,\"linked_units\":",
                live.updates, phase, flags, terminal);
        write_units(live.trace, units, counts);
        fputs("}\n", live.trace);
        if (ferror(live.trace)) finish("engine_error", "unit_trace_write", 4, NULL);
        if (phase == 17 && !(flags & 1u)) {
            if (terminal < 1 || terminal > 3) finish("engine_error", "native_exit_without_terminal_state", 4, NULL);
            live.terminal = terminal;
            live.terminal_seen = 1;
            for (unsigned group = 0; group < 2; group++)
                memcpy(live.terminal_units[group], units[group], counts[group] * sizeof units[group][0]);
            memcpy(live.terminal_counts, counts, sizeof counts);
            live.battle_outcome = terminal == 1 ? "win" : terminal == 2 ? "defeat" :
                P4_G16(battle + 0xc5e) ? "escape" : "scripted_conclusion";
            live.battle_cause = terminal == 1 ? "native_battle_exit_victory" : terminal == 2 ?
                ((P4_G32(battle + 0x10) & 0x10000000u) ? "native_forced_defeat_flag" :
                 datCalcIsDead(datUnitGetRecord(1), 0) ? "protagonist_terminal_ko" : "native_party_terminal_defeat") :
                P4_G16(battle + 0xc5e) ? "native_committed_party_group_escape" : "native_battle_exit_no_party";
            live.terminal_lethal = terminal == 2 && live.lethal_seen && !datCalcGetHp(datUnitGetRecord(1));
            live.terminal_update = live.updates;
            live.terminal_flip = p4pc_flip_count();
            live.terminal_frame = P4_G32(0x00764290u);
            live.terminal_draws = p4pc_rng_draws;
            if (!trace_rng("fight_terminal_rng", 0)) finish("engine_error", "terminal_rng_snapshot", 4, NULL);
            fprintf(live.trace, "{\"kind\":\"fight_terminal\",\"native_result\":%u,\"update\":%u,\"flip\":%u,\"game_frame\":%u,\"additive_draws\":%u}\n",
                    terminal, live.updates, live.terminal_flip, live.terminal_frame, live.terminal_draws);
            if (ferror(live.trace)) finish("engine_error", "terminal_trace_write", 4, NULL);
            if (live.entry.completion == 1 && !live.qualification_terminal_present)
                finish(live.battle_outcome, live.battle_cause, 0, NULL);
        }
    }
    if (live.entry.completion == 2 && p4pc_pad_walk_status() == P4WALK_DONE) {
        uint32_t context[4];
        if (!live.terminal_seen) finish("engine_error", "walk_done_without_selected_terminal", 4, NULL);
        if (!endpoint(context)) finish("engine_error", "walk_endpoint_mismatch", 4, NULL);
        if (!trace_rng("route_final_rng", 0)) finish("engine_error", "route_rng_snapshot", 4, NULL);
        finish("route_complete", "native_walk_done_and_endpoint", 0, context);
    }
}
static int prefix(const char *cause)
{
    if (!p4_battle_live_active) return 0;
    if (live.failure) return publish("engine_error", live.failure, NULL) ? -1 : 1;
    return publish("observation_prefix", cause, NULL);
}
int p4_battle_live_lockstep_stop(void) { return prefix("lockstep_stop"); }
int p4_battle_live_lockstep_eof(void) { return prefix("lockstep_replay_eof"); }
int p4_battle_live_lockstep_boundary(void) { return prefix("lockstep_boundary_stop"); }
