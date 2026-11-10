/* Native battle tool: no alternate damage, AI, RNG or outcome implementation. */
#define _POSIX_C_SOURCE 200809L
#include "p4pc/battle_sim.h"
#include "p4pc/battle_live.h"
#include "p4pc/crt.h"
#include "p4pc/gmem.h"
#include "p4pc/platform.h"
#include "p4pc/mod_fidelity.h"
#include "p4pc/battle_events.h"
#include "p4pc/battle_prepare.h"
#include "p4pc/content_digest.h"
#include "p4pc/game_items.h"
#include "p4pc/game_items_native.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/mods.h"
#include "p4pc/walk_script.h"
#include "../memcard/mod_save_transport.h"
#include "../pad/pad_sdl.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTRY (8u << 20)
#define RNG_WORDS 662u
#define GP(off) P4_GPDATA(uint32_t, off)
int p4_battle_sim_active;
int p4_battle_sim_renderless;
static struct {
    uint32_t flags, encounter, seed, mt_seed, effect_seed, limit, boot_limit;
    uint32_t policy, latency, action_limit, tape_count, save_size, rng_size;
    uint32_t updates, boot_updates, tape_index, launched, seen_battle;
    uint32_t entry_draws;
    uint64_t entry_rng_hash, last_hash;
    uint8_t *entry, *save, *rng, *tape;
    FILE *trace;
    const char *result;
    const char *defeat_cause;
    char initial_party[2048];
    char first_command_party[2048];
    uint32_t first_command_update;
    int first_command_seen;
    const char *observation_error;
    uint32_t observer_token;
    uint64_t admissions[2][3], commands[3], free_commands, action_count;
    uint64_t hp_cost, sp_cost, item_cost, item_counts[1536];
    uint64_t hp_damage, hp_recovery, sp_drain, sp_recovery;
    int64_t hp_cost_adjustment, sp_cost_adjustment;
    int64_t hp_target_adjustment, sp_target_adjustment;
    P4BattleHit protagonist_lethal;
    uint32_t protagonist_lethal_update;
    int has_protagonist_lethal;
    int preparing, prepare_decoded, prepare_state, prepare_rng_valid, prepare_complete;
    P4BattlePreparation preparation;
    size_t entry_size;
    const char *prepared_image, *input_error;
    uint32_t context_before[5], context_after[5], prepared_size;
    uint64_t prepare_rng_entry, prepare_rng_exit;
    char request_hash[65], source_hash[65], prepared_hash[65];
    uint32_t prompt_ready, skill_offers_declined;
    P4Walk *walk;                  /* policy 4: the ENTRY's battle:/battlerule: plan on the walker's battle driver */
    uint32_t walk_frame;
    char plan_hash[65];
    int mod_cohort;                /* a declared mod set: explicit mod-witness simulation only */
    char mod_identity[65];
    /* The cohort's declared equipment of added items (P4PC_BATTLE_SIM_EQUIPMENT), applied to the active item
     * table's expanded-equipment side state once the ENTRY image is restored. */
    struct { uint16_t actor, slot, item; } mod_equipment[30];
    unsigned mod_equipment_count;
} session;

extern void *kwlnTaskGetTaskByName(const void *name);
extern uint8_t kwlnTaskDestroyWithHierarchy(void *task);
extern void datSaveRestoreImage(int32_t version, void *image, int32_t size);
extern int32_t datSaveVerifyChecksum(int32_t version, void *image, int32_t size);
extern void datInitDefaultGameState(void);
extern uint16_t datCalcMaxHpById(int16_t id);
extern uint16_t datCalcMaxSpById(int16_t id);
extern void datSetUnitHp(int16_t id, int16_t hp);
extern void datSetUnitSp(int16_t id, int16_t sp);
extern void datSetUnitAiTactic(int16_t id, int16_t tactic);
extern int16_t datGetPartyId(int32_t slot);
extern void *datUnitGetRecord(int16_t id);
extern uint16_t datCalcGetHp(void *unit);
extern uint16_t datCalcGetSp(void *unit);
extern uint32_t datCalcGetBadStatus(void *unit);
extern uint32_t datCalcIsDead(void *unit, int32_t hp_delta);
extern void RpRandomSeed(int32_t seed, void *unused);
extern void RpRandomSeedMT(int32_t seed);
extern void effMiscRandInit(void *state, uint32_t seed);
extern uint32_t p4pc_rng_draws;

static uint32_t le32(const void *pointer)
{
    const uint8_t *p = pointer;
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static uint64_t hash_bytes(uint64_t h, const void *pointer, size_t size)
{
    const uint8_t *p = pointer;
    for (size_t i = 0; i < size; i++) h = (h ^ p[i]) * UINT64_C(1099511628211);
    return h;
}
static int environment(const char *key, const char *value)
{
#ifdef _WIN32
    return _putenv_s(key, value);
#else
    return setenv(key, value, 1);
#endif
}
static int guest_range(uint32_t address, uint32_t size)
{
    return address >= 0x00100000u && address <= 0x02000000u && size <= 0x02000000u - address;
}
static int error(const char *message)
{
    session.input_error = message;
    fprintf(stderr, "p4pc: battle-sim: %s\n", message);
    return -1;
}
static void finish(const char *outcome, const char *cause, int code);

/* Policy 4: a walker battle plan (include/p4pc/walk_script.h) drives the party through the same
 * battle driver as gate walks. Only battle:/battlerule:/battleturn: lines, printable ASCII; the worker appends
 * the waits that keep the driver running until the battle task ends. Inputs come from ENTRY only. */
#define WALK_POLICY 4u
#define LAST_POLICY 5u /* policy 5: attentive player (policy.c) */
#define WALK_PLAN_MAX 16384u
static uint32_t sim_walk_read32(void *user, uint32_t address)
{
    (void)user;
    return address >= 0x00100000u && address <= 0x01FFFFFCu ? *(volatile uint32_t *)P4_G2H(address) : 0;
}
static void sim_walk_note(void *user, const char *message)
{
    (void)user;
    fprintf(stderr, "p4pc: battle-sim walk: %s\n", message);
}
static int walk_plan(const char *text, uint32_t size)
{
    char *script = malloc((size_t)size + 96);
    if (!script) return error("cannot allocate walk battle plan");
    for (uint32_t i = 0; i < size; i++)
        if (text[i] != '\n' && (text[i] < 0x20 || text[i] > 0x7e)) { free(script); return error("walk battle plan is not printable ASCII"); }
    for (uint32_t start = 0; start < size;) {
        uint32_t stop = start;
        while (stop < size && text[stop] != '\n') stop++;
        const size_t len = stop - start;
        if (len && !(len > 7 && !memcmp(text + start, "battle:", 7)) && !(len > 11 && !memcmp(text + start, "battlerule:", 11)) &&
            !(len > 11 && !memcmp(text + start, "battleturn:", 11))) {
            free(script);
            return error("walk battle plan line is not battle:, battlerule: or battleturn:");
        }
        start = stop + 1;
    }
    memcpy(script, text, size);
    snprintf(script + size, 96, "\ntimeout:%u\nwaittask:battle\nwaittask:battle:off\n", session.limit);
    char reason[160];
    session.walk = p4walk_parse(script, reason, sizeof reason);
    free(script);
    if (!session.walk) return error("invalid walk battle plan");
    p4_content_digest(text, size, session.plan_hash);
    return 0;
}
static void walk_plan_update(void)
{
    static const uint16_t pad[P4W_BUTTON_COUNT] = {
        P4PC_PAD_UP, P4PC_PAD_DOWN, P4PC_PAD_LEFT, P4PC_PAD_RIGHT, P4PC_PAD_CROSS, P4PC_PAD_CIRCLE, P4PC_PAD_TRIANGLE, P4PC_PAD_SQUARE,
        P4PC_PAD_START, P4PC_PAD_SELECT, P4PC_PAD_L1, P4PC_PAD_R1, P4PC_PAD_L2, P4PC_PAD_R2, P4PC_PAD_L3, P4PC_PAD_R3,
    };
    static const P4WalkHost host = {sim_walk_read32, sim_walk_note, NULL, NULL, NULL};
    const uint32_t mask = p4walk_frame(session.walk, &host, ++session.walk_frame);
    if (p4walk_status(session.walk) == P4WALK_FAILED) finish("policy_error", "walk_battle_plan_failed", 4);
    unsigned bits = 0;
    for (int b = 0; b < P4W_BUTTON_COUNT; b++) if (mask & (1u << b)) bits |= pad[b];
    p4pc_pad_inject(bits, 1);
}
static int read_entry(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return error("cannot read immutable BattleEntry");
    if (fseek(f, 0, SEEK_END)) { fclose(f); return error("cannot size BattleEntry"); }
    long length = ftell(f);
    if (length < 64 || length > MAX_ENTRY || fseek(f, 0, SEEK_SET)) { fclose(f); return error("BattleEntry size outside contract"); }
    session.entry = malloc((size_t)length);
    if (!session.entry) { fclose(f); return error("cannot allocate BattleEntry"); }
    int ok = fread(session.entry, 1, (size_t)length, f) == (size_t)length && !ferror(f);
    fclose(f);
    if (!ok) return error("cannot read complete BattleEntry");
    session.entry_size = (size_t)length;
    if (session.preparing) {
        const char *reason = NULL;
        if (!p4_battle_prepare_decode(session.entry, session.entry_size, &session.preparation, &reason)) return error(reason);
        session.prepare_decoded = 1;
        session.save = (uint8_t *)session.preparation.source;
        session.save_size = session.preparation.source_bytes;
        session.flags = 1;
        session.seed = le32(session.entry + 48);
        session.mt_seed = le32(session.entry + 52);
        session.effect_seed = le32(session.entry + 56);
        session.boot_limit = le32(session.entry + 60);
        session.limit = le32(session.entry + 64);
        return 0;
    }
    const uint8_t *h = session.entry;
    if (memcmp(h, "P4BSIM01", 8) || le32(h + 8) != 64) return error("unsupported BattleEntry header");
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
    /* Policy 4 carries a walker battle plan: +32 is its byte count, the text follows the RNG block. */
    const uint64_t payload = session.policy == WALK_POLICY ? (uint64_t)session.tape_count : (uint64_t)session.tape_count * 12;
    const uint64_t expected = 64u + (uint64_t)session.save_size + session.rng_size + payload;
    if (session.flags & ~7u || session.encounter > 65535u || !session.limit || !session.boot_limit ||
        !session.action_limit || session.policy > LAST_POLICY || expected != (uint64_t)length ||
        session.rng_size != ((session.flags & 2) ? RNG_WORDS * 4u : 0u)) return error("invalid BattleEntry counts/flags/limits");
    if (session.policy && session.policy != WALK_POLICY && session.tape_count)
        return error("named party policy cannot compete with a controller tape");
    if (session.policy == WALK_POLICY && (!session.tape_count || session.tape_count > WALK_PLAN_MAX))
        return error("invalid walk battle plan size");
    session.save = session.entry + 64;
    session.rng = session.save + session.save_size;
    session.tape = session.rng + session.rng_size;
    if (!p4_battle_save_valid(session.save, session.save_size)) return error("invalid native save version, section layout or checksum");
    if (session.rng_size) {
        uint32_t front = le32(session.rng + 124), back = le32(session.rng + 128);
        if (front >= 31 || back >= 31 || (front + 31 - back) % 31 != 3 || le32(session.rng + 2628) >= 624)
            return error("invalid captured RNG cursor");
    }
    if (session.policy == WALK_POLICY) return walk_plan((const char *)session.tape, session.tape_count);
    uint32_t end = 0;
    for (uint32_t i = 0; i < session.tape_count; i++) {
        const uint8_t *r = session.tape + i * 12;
        uint32_t at = le32(r), bits = le32(r + 4), hold = le32(r + 8);
        if (at < end || at >= session.limit || bits > 65535u || !hold || hold > session.limit - at)
            return error("invalid/overlapping controller tape record");
        end = at + hold;
    }
    return 0;
}

int p4_battle_sim_mod_cohort(void)
{
    return session.mod_cohort;
}
static void prepare_finish(const char *outcome, const char *cause, int code);
int p4_battle_sim_cli(int argc, char **argv, const char **elf)
{
    const char *entry = NULL, *result = NULL, *trace = NULL, *prepared = NULL;
    int requested = 0, live = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--battle-sim")) requested |= 1;
        if (!strcmp(argv[i], "--battle-prepare")) requested |= 2;
        if (!strcmp(argv[i], "--battle-live")) live++;
    }
    if (live > 1 || (live && requested != 1)) return error("live mode requires one battle simulation");
    if (!requested) return 0;
    const char *mod_set = getenv("P4PC_BATTLE_SIM_MODS");
    if (mod_set) {
        /* A declared mod set runs only as an explicit mod-witness battle
         * simulation (never preparation, live mode or a retail reference):
         * the coordinator names the set by its content identity. */
        int valid = strlen(mod_set) == 64;
        for (size_t i = 0; valid && i < 64; i++)
            valid = (mod_set[i] >= '0' && mod_set[i] <= '9') || (mod_set[i] >= 'a' && mod_set[i] <= 'f');
        if (!valid) return error("P4PC_BATTLE_SIM_MODS must be a lowercase SHA-256 mod-set identity");
        if (requested != 1 || live) return error("a declared mod set runs only battle simulation");
        const char *witness = getenv("P4PC_MOD_WITNESS"), *directory = getenv("P4PC_MOD_DIR");
        if (!witness || strcmp(witness, "1") || !directory || !*directory)
            return error("a declared mod set requires P4PC_MOD_WITNESS=1 and P4PC_MOD_DIR");
        session.mod_cohort = 1;
        memcpy(session.mod_identity, mod_set, sizeof session.mod_identity);
    } else if (getenv("P4PC_MOD_WITNESS")) return error("mod witness is incompatible with simulator fidelity");
    const char *equipment = getenv("P4PC_BATTLE_SIM_EQUIPMENT");
    if (equipment) {
        /* "actor:slot:item" entries, comma-separated: added items only (the retail fields belong to preparation). */
        if (!session.mod_cohort) return error("declared mod equipment requires a declared mod set");
        for (const char *p = equipment; *p;) {
            char *end;
            const unsigned long actor = strtoul(p, &end, 10);
            if (end == p || *end != ':') return error("invalid P4PC_BATTLE_SIM_EQUIPMENT");
            p = end + 1;
            const unsigned long slot = strtoul(p, &end, 10);
            if (end == p || *end != ':') return error("invalid P4PC_BATTLE_SIM_EQUIPMENT");
            p = end + 1;
            const unsigned long item = strtoul(p, &end, 10);
            if (end == p || (*end && *end != ',') || !actor || actor > 10 || slot > 2 || item < P4_ITEM_RETAIL_COUNT ||
                item > 0xFFFF || session.mod_equipment_count == sizeof session.mod_equipment / sizeof *session.mod_equipment)
                return error("invalid P4PC_BATTLE_SIM_EQUIPMENT");
            for (unsigned i = 0; i < session.mod_equipment_count; i++)
                if (session.mod_equipment[i].actor == actor && session.mod_equipment[i].slot == slot)
                    return error("duplicate P4PC_BATTLE_SIM_EQUIPMENT slot");
            session.mod_equipment[session.mod_equipment_count].actor = (uint16_t)actor;
            session.mod_equipment[session.mod_equipment_count].slot = (uint16_t)slot;
            session.mod_equipment[session.mod_equipment_count++].item = (uint16_t)item;
            p = *end ? end + 1 : end;
            if (*end && !*p) return error("invalid P4PC_BATTLE_SIM_EQUIPMENT");
        }
        if (!session.mod_equipment_count) return error("invalid P4PC_BATTLE_SIM_EQUIPMENT");
    }
    if (requested == 3) return error("battle simulation and preparation are mutually exclusive");
    session.preparing = requested == 2;
    *elf = NULL;
    for (int i = 1; i < argc; i++) {
        const char **value = NULL;
        if ((!session.preparing && !strcmp(argv[i], "--battle-sim")) ||
            (session.preparing && !strcmp(argv[i], "--battle-prepare"))) value = &entry;
        else if ((!session.preparing && !strcmp(argv[i], "--battle-result")) ||
                 (session.preparing && !strcmp(argv[i], "--battle-prepare-result"))) value = &result;
        else if (!session.preparing && !strcmp(argv[i], "--battle-trace")) value = &trace;
        else if (session.preparing && !strcmp(argv[i], "--battle-prepared-image")) value = &prepared;
        else if (live && !strcmp(argv[i], "--battle-live")) continue;
        else if (argv[i][0] != '-' && !*elf) { *elf = argv[i]; continue; }
        else return error("unknown or duplicate simulator argument");
        if (*value || i + 1 == argc) return error("missing or duplicate simulator argument");
        *value = argv[++i];
    }
    if (session.preparing) {
        if (!entry || !result || !prepared || !*elf || !strcmp(entry, result) || !strcmp(entry, prepared) || !strcmp(result, prepared))
            return error("preparation requires distinct request/image/result and an explicit ELF");
    } else if (!entry || !result || !trace || !*elf || !strcmp(entry, result) || !strcmp(entry, trace) || !strcmp(result, trace))
        return error("simulator requires distinct entry/result/trace and an explicit ELF");
    if (live) return p4_battle_live_start(entry, result, trace);
    static const char *const incompatible[] = {
        "P4PC_WALK_SCRIPT", "P4PC_START_BATTLE", "P4PC_BATTLE_DRIVER", "P4PC_AUTOPRESS", "P4PC_AUTOPRESS_LOAD",
        "P4PC_AUTOPRESS_NEWGAME", "P4PC_CAPTURE_FRAMES", "P4PC_CAPTURE_STATE", "P4PC_START_MENU", "P4PC_MENU_SCRIPT",
        "P4PC_MOD_WITNESS",
        "P4PC_TRAP_CONTINUE", "P4PC_OVERRIDE_DIR", "P4PC_LOCKSTEP_REPLAY", "P4PC_LOCKSTEP_LIVE"
    };
    for (size_t i = 0; i < sizeof incompatible / sizeof *incompatible; i++) {
        if (session.mod_cohort && !strcmp(incompatible[i], "P4PC_MOD_WITNESS")) continue;
        if (getenv(incompatible[i])) return error("competing gameplay/automation environment in simulator");
    }
    if (!getenv("P4PC_SAVE_DIR") || !getenv("P4PC_CONFIG")) return error("simulator requires a private save/config profile");
    FILE *check = fopen(result, "rb");
    if (check) { fclose(check); return error("result exists; refusing overwrite"); }
    if (prepared) {
        check = fopen(prepared, "rb");
        if (check) { fclose(check); return error("prepared image exists; refusing overwrite"); }
    }
    session.result = result;
    session.prepared_image = prepared;
    if (read_entry(entry)) {
        if (session.preparing) prepare_finish("preparation_error", session.input_error, 1);
        return -1;
    }
    const char *const keys[] = { "P4PC_INPUT", "P4PC_MODS", "P4PC_PACKS", "P4PC_VIRTUAL_CLOCK", "P4PC_TURBO", "P4PC_HIDDEN_WINDOW", "SDL_AUDIODRIVER" };
    const char *const values[] = { "script", session.mod_cohort ? "1" : "0", "0", "1", "1", "1", "dummy" };
    for (size_t i = 0; i < sizeof keys / sizeof *keys; i++) if (environment(keys[i], values[i])) return error("cannot isolate simulator environment");
    if (!session.preparing) {
        session.trace = fopen(trace, "wbx");
        if (!session.trace) return error("cannot create exclusive trace");
    }
    p4_mod_fidelity_check_start();
    p4_battle_sim_renderless = !!(session.flags & 1);
    p4_battle_sim_active = 1;
    return 1;
}

uint32_t p4_battle_sim_update_count(void) { return session.updates; }
static uint32_t additive_block(void)
{
    const uint32_t offset = GP(0xb618);
    if (offset > 0x10000u) return 0;
    return 0x008872e0u + offset;
}
static int rng_snapshot(uint32_t out[RNG_WORDS])
{
    const uint32_t block = additive_block();
    if (!block || !guest_range(block, 16)) return 0;
    const uint32_t base = P4_G32(block), front = P4_G32(block + 4), back = P4_G32(block + 8), end = P4_G32(block + 12);
    if (!guest_range(base, 124) || end != base + 124 || front < base || front >= end || back < base || back >= end ||
        ((front - base) & 3) || ((back - base) & 3)) return 0;
    memcpy(out, P4_G2H(base), 124);
    out[31] = (front - base) / 4;
    out[32] = (back - base) / 4;
    memcpy(out + 33, P4_G2H(0x00885a90u), 2496);
    out[657] = GP(0xaa7c);
    memcpy(out + 658, P4_G2H(0x00922de0u), 16);
    return 1;
}
static int restore_rng(void)
{
    uint32_t state[RNG_WORDS];
    if (!rng_snapshot(state)) return 0;
    if (!(session.flags & 2)) {
        RpRandomSeed((int32_t)session.seed, NULL);
        RpRandomSeedMT((int32_t)session.mt_seed);
        effMiscRandInit(NULL, session.effect_seed);
    } else {
        const uint32_t block = additive_block(), base = P4_G32(block);
        memcpy(P4_G2H(base), session.rng, 124);
        P4_G32(block + 4) = base + le32(session.rng + 124) * 4;
        P4_G32(block + 8) = base + le32(session.rng + 128) * 4;
        memcpy(P4_G2H(0x00885a90u), session.rng + 132, 2496);
        GP(0xaa7c) = le32(session.rng + 2628);
        memcpy(P4_G2H(0x00922de0u), session.rng + 2632, 16);
    }
    return rng_snapshot(state);
}
static int rng_hash(uint64_t *hash)
{
    uint32_t state[RNG_WORDS];
    if (!rng_snapshot(state)) return 0;
    *hash = hash_bytes(UINT64_C(14695981039346656037), state, sizeof state);
    return 1;
}
static void context_json(FILE *file, const uint32_t context[5])
{
    fprintf(file, "{\"date\":%u,\"time_slot\":%u,\"field\":%u,\"room\":%u,\"step\":%u}",
            context[0], context[1], context[2], context[3], context[4]);
}
static void prepare_finish(const char *outcome, const char *cause, int code)
{
    FILE *file = fopen(session.result, "wbx");
    if (!file) { fprintf(stderr, "p4pc: cannot create exclusive preparation receipt\n"); exit(5); }
    fprintf(file, "{\"schema\":1,\"operation\":\"battle_prepare\",\"outcome\":\"%s\",\"terminal_cause\":\"%s\",\"bootstrap_updates\":%u,\"field_resume_updates\":%u,",
            outcome, cause, session.boot_updates, session.updates);
    fputs("\"request_sha256\":", file);
    if (session.entry_size) {
        p4_content_digest(session.entry, session.entry_size, session.request_hash);
        fprintf(file, "\"%s\",", session.request_hash);
    } else fputs("null,", file);
    fputs("\"definition_sha256\":", file);
    if (session.prepare_decoded) {
        fputc('"', file);
        for (unsigned i = 0; i < 32; i++) fprintf(file, "%02x", session.preparation.header[128 + i]);
        p4_content_digest(session.preparation.source, session.preparation.source_bytes, session.source_hash);
        fprintf(file, "\",\"source_sha256\":\"%s\",\"classification\":\"%s\",\"resource_mode\":\"%s\",\"clear_temporary_statuses\":%s,",
                session.source_hash, le32(session.preparation.header + 36) == 1 ? "source_attested" : "synthetic_lab",
                le32(session.preparation.header + 40) ? "declared_fill_living_hp_sp" : "native_inherit_or_explicit_actors",
                le32(session.preparation.header + 44) ? "true" : "false");
    } else fputs("null,\"source_sha256\":null,\"classification\":null,", file);
    fputs("\"context_before\":", file);
    if (session.prepare_state) context_json(file, session.context_before); else fputs("null", file);
    fputs(",\"context_after\":", file);
    if (session.prepare_state >= 2) context_json(file, session.context_after); else fputs("null", file);
    fputs(",\"preparation_rng\":", file);
    if (session.prepare_rng_valid) {
        const int have_exit = session.prepare_complete || rng_hash(&session.prepare_rng_exit);
        fprintf(file, "{\"policy\":\"seed_at_native_apply\",\"additive_seed\":%u,\"mt_seed\":%u,\"effect_seed\":%u,\"bank_hash_algorithm\":\"fnv1a64-662-native-words\",\"entry\":\"%016" PRIx64 "\",\"exit\":",
                session.seed, session.mt_seed, session.effect_seed, session.prepare_rng_entry);
        if (have_exit) fprintf(file, "\"%016" PRIx64 "\"", session.prepare_rng_exit); else fputs("null", file);
        fprintf(file, ",\"additive_draws\":%u}", p4pc_rng_draws - session.entry_draws);
    } else fputs("null", file);
    if (session.prepare_complete) {
        fprintf(file, ",\"effective_image_sha256\":\"%s\",\"effective_image_bytes\":%u,\"effective_image_native_valid\":true,\"effective_image_kind\":\"native_full\",",
                session.prepared_hash, session.prepared_size);
        p4_battle_prepare_report(file, &session.preparation);
    }
    fputs("}\n", file);
    const int write_error = ferror(file);
    const int close_error = fclose(file);
    if (write_error || close_error) { fprintf(stderr, "p4pc: cannot finish preparation receipt\n"); exit(5); }
    p4pc_exit(code);
}
static size_t party_json(char *out, size_t cap)
{
    size_t used = 0;
    unsigned seen = 0;
    int count = 0;
    int32_t ids[4] = { 1, datGetPartyId(0) & 65535, datGetPartyId(1) & 65535, datGetPartyId(2) & 65535 };
    out[0] = 0;
    for (unsigned i = 0; i < 4; i++) {
        int32_t id = ids[i];
        if (id < 1 || id > 10 || (seen & (1u << id))) continue;
        seen |= 1u << id;
        void *unit = datUnitGetRecord((int16_t)id);
        int n = snprintf(out + used, cap - used, "%s{\"id\":%d,\"hp\":%u,\"sp\":%u,\"max_hp\":%u,\"max_sp\":%u,\"status\":%u}",
                         count ? "," : "", id, datCalcGetHp(unit), datCalcGetSp(unit), datCalcMaxHpById(id), datCalcMaxSpById(id), datCalcGetBadStatus(unit));
        if (n < 0 || (size_t)n >= cap - used) return 0;
        used += (size_t)n;
        count++;
    }
    return used;
}
void p4_battle_sim_command_ready(uint32_t action)
{
    if (p4_battle_live_active) { p4_battle_live_command_ready(action); return; }
    if (!session.trace || session.first_command_seen || session.observation_error) return;
    const uint32_t unit = P4_G32(action + 0x30);
    if (P4_G8(unit + 0xa2) != 0 || P4_G16(unit + 0xa4) != 1) return;
    if (!party_json(session.first_command_party, sizeof session.first_command_party)) {
        session.observation_error = "first_protagonist_command_resources";
        return;
    }
    /* The declared added-item equipment must still be in place once the battle asks for its first command. */
    for (unsigned i = 0; i < session.mod_equipment_count; i++) {
        uint32_t item = 0;
        if (p4_game_tables_items_equipment_get(p4_game_tables_items_active(), session.mod_equipment[i].actor,
                                               session.mod_equipment[i].slot, &item) != P4_TABLE_OK ||
            item != session.mod_equipment[i].item) {
            session.observation_error = "mod_equipment_lost_before_battle";
            return;
        }
    }
    session.first_command_seen = 1;
    session.first_command_update = session.updates + 1;
    fprintf(session.trace, "{\"kind\":\"first_protagonist_command\",\"boundary\":\"native_input_ready\",\"update\":%u,\"party\":[%s]}\n",
            session.first_command_update, session.first_command_party);
}
static void actor_json(FILE *file, const P4BattleActor *actor)
{
    fprintf(file, "{\"present\":%u,\"action_uid\":\"%016" PRIx64 "\",\"unit_uid\":%u,\"id\":%u,\"side\":%u}",
            actor->present, actor->action_uid, actor->unit_uid, actor->native_id, actor->side);
}
static void admission_event(const P4BattleAdmission *event, void *context)
{
    (void)context;
    const unsigned side = event->actor.side < 2 ? event->actor.side : 2;
    session.admissions[event->queue_kind == P4_BATTLE_QUEUE_FOLLOWUP][side]++;
    fprintf(session.trace, "{\"kind\":\"admission\",\"update\":%u,\"queue_kind\":%u,\"native_action_count\":%u,\"requested_state\":%u,\"actor\":",
            session.updates + 1, event->queue_kind, event->native_action_count, event->next_state);
    actor_json(session.trace, &event->actor);
    fputs("}\n", session.trace);
}
static void command_event(const P4BattleCommand *event, void *context)
{
    (void)context;
    const unsigned side = event->actor.side < 2 ? event->actor.side : 2;
    if (event->free_action) session.free_commands++;
    else {
        session.commands[side]++;
        session.action_count++;
    }
    fprintf(session.trace, "{\"kind\":\"command_commit\",\"update\":%u,\"action_kind\":%u,\"skill_id\":%u,\"item_id\":%u,\"target_count\":%u,\"native_state\":%u,\"free_action\":%u,\"actor\":",
            session.updates + 1, event->action_kind, event->skill_id, event->item_id,
            event->target_count, event->native_state, event->free_action);
    actor_json(session.trace, &event->actor);
    fputs("}\n", session.trace);
}
static void cost_event(const P4BattleCost *event, void *context)
{
    (void)context;
    if (event->actor.side == 0) {
        if (event->kind == P4_BATTLE_COST_HP) {
            session.hp_cost += event->requested_cost;
            session.hp_cost_adjustment += (int64_t)event->hp_after - event->hp_before + event->requested_cost;
        } else if (event->kind == P4_BATTLE_COST_SP) {
            session.sp_cost += event->requested_cost;
            session.sp_cost_adjustment += (int64_t)event->sp_after - event->sp_before + event->requested_cost;
        } else if (event->kind == P4_BATTLE_COST_ITEM && event->item_count_before > event->item_count_after) {
            const uint32_t used = event->item_count_before - event->item_count_after;
            session.item_cost += used;
            if (event->item_id < 1536) session.item_counts[event->item_id] += used;
        }
    }
    fprintf(session.trace, "{\"kind\":\"cost_commit\",\"update\":%u,\"cost_kind\":%u,\"requested_cost\":%u,\"skill_id\":%u,\"item_id\":%u,\"hp_before\":%u,\"hp_after\":%u,\"sp_before\":%u,\"sp_after\":%u,\"item_count_before\":%u,\"item_count_after\":%u,\"actor\":",
            session.updates + 1, event->kind, event->requested_cost, event->skill_id, event->item_id,
            event->hp_before, event->hp_after, event->sp_before, event->sp_after,
            event->item_count_before, event->item_count_after);
    actor_json(session.trace, &event->actor);
    fputs("}\n", session.trace);
}
static void hit_event(const P4BattleHit *event, void *context)
{
    (void)context;
    if (event->target.side == 0) {
        if (event->target.native_id == 1) {
            if (!event->hp_after && event->hp_before) {
                session.protagonist_lethal = *event;
                session.protagonist_lethal_update = session.updates + 1;
                session.has_protagonist_lethal = 1;
            } else if (event->hp_after) session.has_protagonist_lethal = 0;
        }
        int64_t requested_hp = event->resources_applied ? event->requested_hp_delta : 0;
        int64_t requested_sp = event->resources_applied ? event->requested_sp_delta : 0;
        if (requested_hp < 0) session.hp_damage += (uint64_t)-requested_hp;
        else session.hp_recovery += (uint64_t)requested_hp;
        if (requested_sp < 0) session.sp_drain += (uint64_t)-requested_sp;
        else session.sp_recovery += (uint64_t)requested_sp;
        session.hp_target_adjustment += (int64_t)event->hp_after - event->hp_before - requested_hp;
        session.sp_target_adjustment += (int64_t)event->sp_after - event->sp_before - requested_sp;
    }
    fprintf(session.trace, "{\"kind\":\"target_commit\",\"update\":%u,\"skill_id\":%u,\"action_kind\":%u,\"hp_delta_requested\":%" PRId32 ",\"sp_delta_requested\":%" PRId32 ",\"money_delta_requested\":%" PRId32 ",\"hp_before\":%u,\"hp_after\":%u,\"sp_before\":%u,\"sp_after\":%u,\"status_before\":%u,\"status_after\":%u,\"added_status\":%u,\"removed_status\":%u,\"buff_changes\":%u,\"target_flags\":%u,\"result_flags\":%u,\"effect\":%u,\"resources_applied\":%u,\"statuses_applied\":%u,\"source\":",
            session.updates + 1, event->skill_id, event->action_kind, event->requested_hp_delta,
            event->requested_sp_delta, event->requested_money_delta, event->hp_before, event->hp_after,
            event->sp_before, event->sp_after, event->status_before, event->status_after,
            event->added_status, event->removed_status, event->buff_changes, event->target_flags,
            event->result_flags, event->effect, event->resources_applied, event->statuses_applied);
    actor_json(session.trace, &event->source);
    fputs(",\"target\":", session.trace);
    actor_json(session.trace, &event->target);
    fputs("}\n", session.trace);
}
static uint64_t observe(const char *kind, int full_rng)
{
    uint32_t rng[RNG_WORDS];
    if (!rng_snapshot(rng)) return 0;
    uint64_t rng_hash = hash_bytes(UINT64_C(14695981039346656037), rng, sizeof rng);
    uint64_t state = rng_hash;
    const uint32_t context = GP(0xb3ac);
    uint32_t phase = 0, flags = 0, terminal = 0;
    if (context && !guest_range(context, 0xe88)) return 0;
    if (context) {
        phase = P4_G32(context + 0x2fc);
        flags = P4_G32(context + 0xc);
        terminal = P4_G16(context + 0x1c);
        state = hash_bytes(state, P4_G2H(context + 0xc), 0x18);
        for (unsigned group = 0; group < 2; group++) {
            uint32_t unit = P4_G32(context + 0x178 + group * 8);
            for (unsigned n = 0; unit && n < 12; n++) {
                if (!guest_range(unit, 0xa70)) return 0;
                const uint32_t record = P4_G32(unit + 0xa64);
                state = hash_bytes(state, P4_G2H(unit + 0x98), 0x14);
                if (record && !guest_range(record, 0x30)) return 0;
                if (record) state = hash_bytes(state, P4_G2H(record), 0x30);
                unit = P4_G32(unit + 0xa6c);
                if (n == 11 && unit) return 0;
            }
        }
    }
    char party[2048];
    if (!party_json(party, sizeof party)) return 0;
    state = hash_bytes(state, party, strlen(party));
    fprintf(session.trace, "{\"kind\":\"%s\",\"update\":%u,\"phase\":%u,\"flags\":%u,\"terminal\":%u,\"rng_hash\":\"%016" PRIx64 "\",\"state_hash\":\"%016" PRIx64 "\",\"rng_draws\":%u,\"party\":[%s]",
            kind, session.updates, phase, flags, terminal, rng_hash, state, p4pc_rng_draws - session.entry_draws, party);
    if (full_rng) {
        fputs(",\"rng_words\":[", session.trace);
        for (unsigned i = 0; i < RNG_WORDS; i++) fprintf(session.trace, "%s%u", i ? "," : "", rng[i]);
        fputc(']', session.trace);
    }
    fputs("}\n", session.trace);
    if (ferror(session.trace)) return 0;
    if (!session.entry_rng_hash) session.entry_rng_hash = rng_hash;
    session.last_hash = state;
    return rng_hash;
}
static void json_text(FILE *f, const char *text)
{
    fputc('"', f);
    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        if (*p == '"' || *p == '\\') fprintf(f, "\\%c", *p);
        else if (*p < 0x20 || *p >= 0x7f) fprintf(f, "\\u%04x", *p);
        else fputc(*p, f);
    }
    fputc('"', f);
}
static void finish(const char *outcome, const char *cause, int code)
{
    uint64_t rng_hash = session.launched ? observe("terminal", 1) : 0;
    char party[2048] = "";
    if (session.launched) party_json(party, sizeof party);
    /* Every result names the table catalog it ran on, retail included; an
     * unavailable identity fails the run rather than defaulting to retail. */
    uint8_t catalog[32];
    const int expanded = p4_game_table_catalog_identity(catalog);
    if (expanded < 0) { outcome = "engine_error"; cause = "table_catalog_identity_unavailable"; code = 4; }
    /* A declared mod set names the packages actually active; none fails the run. */
    const size_t mod_count = session.mod_cohort ? p4_mods_count() : 0;
    if (session.mod_cohort && !mod_count) { outcome = "engine_error"; cause = "mod_set_unavailable"; code = 4; }
    char path[2048];
    int n = snprintf(path, sizeof path, "%s.tmp", session.result);
    if (n < 0 || (size_t)n >= sizeof path) { error("result path too long"); exit(4); }
    FILE *f = fopen(path, "wbx");
    if (!f) { error("cannot create exclusive result staging"); exit(4); }
    const uint64_t ordinary = session.admissions[0][0] + session.admissions[0][1] + session.admissions[0][2];
    const uint64_t followup = session.admissions[1][0] + session.admissions[1][1] + session.admissions[1][2];
    const uint64_t actions = session.action_count;
    fprintf(f, "{\"schema\":1,\"entry_kind\":\"harness-script6\",\"trace_capability\":\"committed-events-and-scoped-state-rng\",\"outcome\":\"%s\",\"terminal_cause\":\"%s\",\"encounter_id\":%u,\"updates\":%u,\"counter_capability\":\"native-queue-ready-cost-target-v1\",\"observer_attached\":%u,",
            outcome, cause, session.encounter, session.updates, session.observer_token != 0);
    fputs("\"table_catalog\":", f);
    if (expanded < 0) fputs("null,", f);
    else {
        fprintf(f, "{\"expanded\":%d,\"identity\":\"", expanded);
        for (unsigned i = 0; i < sizeof catalog; i++) fprintf(f, "%02x", catalog[i]);
        fputs("\"},", f);
    }
    fputs("\"mods\":", f);
    if (!session.mod_cohort) fputs("null,", f);
    else {
        fprintf(f, "{\"identity\":\"%s\",\"witness\":true,\"active\":[", session.mod_identity);
        for (size_t i = 0; i < mod_count; i++) {
            const P4PackageInfo *info = p4_mods_info(i);
            fputs(i ? ",{\"id\":" : "{\"id\":", f);
            json_text(f, info ? info->id : "");
            fputs(",\"version\":", f);
            json_text(f, info ? info->version : "");
            fputc('}', f);
        }
        fputs("]", f);
        if (session.mod_equipment_count) {
            fputs(",\"equipment\":[", f);
            for (unsigned i = 0; i < session.mod_equipment_count; i++)
                fprintf(f, "%s{\"actor\":%u,\"slot\":%u,\"item\":%u}", i ? "," : "", session.mod_equipment[i].actor,
                        session.mod_equipment[i].slot, session.mod_equipment[i].item);
            fputc(']', f);
        }
        fputs("},", f);
    }
    fprintf(f, "\"result_prompt_capability\":\"%s\",\"result_prompts\":{\"skill_offers_declined\":%u},",
            session.policy ? "periodic_cross_native_skill_offer_decline_v1" : "controller_tape", session.skill_offers_declined);
    static const char *policy_names[] = {"controller-tape-v1", "greedy-weakness-v1", "cautious-healer-v1", "typical-player-v1",
                                         "walk-battle-rules-v1", "attentive-player-v1"};
    static const char *policy_knowledge[] = {"controller_tape", "observed_damage_weakness_only", "observed_damage_weakness_only",
                                             "observed_damage_weakness_only", "walker_battle_rules_native_unit_records",
                                             "observed_damage_weakness_public_charge_guard"};
    fprintf(f, "\"party_policy\":{\"id\":%u,\"name\":\"%s\",\"latency_updates\":%u,\"party_control\":\"%s\",\"knowledge\":\"%s\"",
            session.policy, policy_names[session.policy], session.latency,
            session.policy ? "native_direct_commands" : "source_tactics", policy_knowledge[session.policy]);
    /* Each plan is its own cohort; its delay= option, not latency_updates, times the walker's input. */
    if (session.policy == WALK_POLICY) fprintf(f, ",\"plan_sha256\":\"%s\"", session.plan_hash);
    fputs("},", f);
    fputs("\"entry_observations\":{\"first_protagonist_command\":{\"boundary\":\"native_input_ready\",\"observed\":", f);
    if (session.first_command_seen)
        fprintf(f, "1,\"update\":%u,\"party\":[%s]}},", session.first_command_update, session.first_command_party);
    else fputs("0,\"update\":null,\"party\":null}},", f);
    fputs("\"last_observed_protagonist_lethal_packet\":", f);
    if (session.has_protagonist_lethal && !strcmp(outcome, "defeat") &&
        !datCalcGetHp(datUnitGetRecord(1))) {
        const P4BattleHit *lethal = &session.protagonist_lethal;
        fprintf(f, "{\"update\":%u,\"skill_id\":%u,\"hp_delta_requested\":%" PRId32 ",\"added_status\":%u,\"result_flags\":%u,\"source\":",
                session.protagonist_lethal_update, lethal->skill_id,
                lethal->requested_hp_delta, lethal->added_status, lethal->result_flags);
        actor_json(f, &lethal->source);
        fputs("},", f);
    } else fputs("null,", f);
    fprintf(f, "\"turns\":{\"actor_turns\":%" PRIu64 ",\"followup_admissions\":%" PRIu64 ",\"admitted_opportunities\":%" PRIu64 ",\"actions\":%" PRIu64 ",\"rounds\":null,\"party_actions\":%" PRIu64 ",\"enemy_actions\":%" PRIu64 ",\"other_actions\":%" PRIu64 ",\"free_commands\":%" PRIu64 ",\"actor_turns_definition\":\"ordinary_queue_admissions_including_skips\",\"actions_definition\":\"ready_to_execution_attempts_excluding_persona_analyze\"},",
            ordinary, followup, ordinary + followup, actions, session.commands[0],
            session.commands[1], session.commands[2], session.free_commands);
    fprintf(f, "\"resources\":{\"initial_party_boundary\":\"after_restore_before_constructor\",\"initial_party\":[%s],\"final_party\":[%s],\"hp_spent\":%" PRIu64 ",\"sp_spent\":%" PRIu64 ",\"items_consumed\":%" PRIu64 ",\"spend_definition\":\"native_party_skill_costs_and_actual_item_decrements\",\"hp_damage_requested\":%" PRIu64 ",\"hp_recovery_requested\":%" PRIu64 ",\"sp_drain_requested\":%" PRIu64 ",\"sp_recovery_requested\":%" PRIu64 ",\"cost_hp_adjustment\":%" PRId64 ",\"cost_sp_adjustment\":%" PRId64 ",\"target_hp_adjustment\":%" PRId64 ",\"target_sp_adjustment\":%" PRId64 ",\"recovery_scope\":\"committed_target_packets_not_direct_postbattle_gifts\",\"items_by_id\":[",
            session.initial_party, party, session.hp_cost, session.sp_cost, session.item_cost,
            session.hp_damage, session.hp_recovery, session.sp_drain, session.sp_recovery,
            session.hp_cost_adjustment, session.sp_cost_adjustment,
            session.hp_target_adjustment, session.sp_target_adjustment);
    int first_item = 1;
    for (unsigned id = 0; id < 1536; id++) if (session.item_counts[id]) {
        fprintf(f, "%s{\"id\":%u,\"used\":%" PRIu64 "}", first_item ? "" : ",", id, session.item_counts[id]);
        first_item = 0;
    }
    fprintf(f, "]},\"rng\":{\"mode\":\"%s\",\"additive_seed\":%u,\"mt_seed\":%u,\"effect_seed\":%u,\"entry_hash\":\"%016" PRIx64 "\",\"final_hash\":\"%016" PRIx64 "\",\"draws\":%u},\"state_hash\":\"%016" PRIx64 "\"}\n",
            session.flags & 2 ? "captured" : "seeded", session.seed, session.mt_seed, session.effect_seed,
            session.entry_rng_hash, rng_hash, p4pc_rng_draws - session.entry_draws, session.last_hash);
    int failed = ferror(f);
    if (fclose(f)) failed = 1;
    if (session.trace && fclose(session.trace)) failed = 1;
    if (failed || rename(path, session.result)) { error("cannot publish complete result"); exit(4); }
    fprintf(stderr, "p4pc: battle-sim: %s after %u native updates (%s)\n", outcome, session.updates, cause);
    p4pc_exit(code);
}
static int boot_ready(void)
{
    const uint32_t sequence = GP(0xb1a8);
    if (!guest_range(sequence, 0x3c)) return 0;
    const uint32_t work = P4_G32(sequence + 0x38);
    if (!guest_range(work, 8) || P4_G32(work + 4) != 27 || !GP(0xb414)) return 0;
    void *logo = kwlnTaskGetTaskByName(P4_G2H(0x005e57a8u));
    if (!logo) return 0;
    uint32_t logo_work;
    memcpy(&logo_work, (uint8_t *)logo + 0x38, 4);
    return guest_range(logo_work, 4) && P4_G32(logo_work) >= 4;
}
extern void calendarSyncFromDate(void);
extern void calendarSetMode(int32_t mode);
extern int16_t datGetDateIndex(void);
extern int16_t datGetPendingDateIndex(void);
extern uint8_t datGetPendingTimeSlot(void);
extern int32_t FldStepIndex(void);
extern void sequenceRequest(int32_t sequence, uint32_t parameters, int32_t size, int32_t delay);
extern void *datSaveBuildImage(int32_t version, int32_t *size, int32_t compact);

static const char *restore_source(void)
{
    const uint32_t image = p4_gport_alloc(session.save_size, 16);
    if (!image) return "entry_guest_allocation";
    memcpy(P4_G2H(image), session.save, session.save_size);
    if (!datSaveVerifyChecksum(0, P4_G2H(image + 4), (int32_t)session.save_size - 4)) return "native_save_checksum";
    /* A declared mod set sees this restore as the real load it stands for. The
     * native reset (0x001113b0) reports "reset"; the exact image is then staged
     * through the save transport like a card read without metadata (no sidecar:
     * an entry carries no mod blocks), and the native restore (0x0010e710)
     * proves, commits and announces it. */
    datInitDefaultGameState();
    if (session.mod_cohort) {
        char reason[512];
        if (p4_mod_save_transport_loaded_untagged(session.save, session.save_size, "battle-sim-entry", reason, sizeof reason)) {
            fprintf(stderr, "p4pc: battle-sim: mod-save load staging: %s\n", reason);
            return "mod_save_load_staging";
        }
    }
    datSaveRestoreImage(0, P4_G2H(image + 4), (int32_t)session.save_size - 4);
    return NULL;
}
static int current_context(uint32_t context[5])
{
    const uint32_t field = GP(0x9db0);
    if (!guest_range(field, 8)) return 0;
    context[0] = (uint16_t)datGetDateIndex();
    context[1] = P4_G8(0x797b7c);
    context[2] = P4_G32(field);
    context[3] = P4_G32(field + 4);
    context[4] = (uint32_t)FldStepIndex();
    return 1;
}
static int field_interactive(void)
{
    /* FldRootCreate00155e10 publishes this genuine task. FldRootUpdate
     * state13 follows load, both startup scripts, fade and trigger setup.
     * Scene-load flag2/3 alone is earlier and is not sufficient. */
    const uint32_t root = P4_G32(0x7d2510);
    if (!guest_range(root, 0x3c)) return 0;
    const uint32_t work = P4_G32(root + 0x38);
    return guest_range(work, 0xf4) && P4_G32(work) == 13 && !P4_G32(work + 8);
}
static void prepare_before_update(void)
{
    if (!session.prepare_state) {
        if (session.boot_updates >= session.boot_limit) prepare_finish("preparation_error", "bootstrap_update_limit", 4);
        if (!boot_ready()) return;
        const char *reason = restore_source();
        if (reason) prepare_finish("preparation_error", reason, 4);
        session.context_before[0] = (uint16_t)datGetDateIndex();
        session.context_before[1] = P4_G8(0x797b7c);
        session.context_before[2] = P4_G32(0x79b698);
        session.context_before[3] = P4_G32(0x79b69c);
        session.context_before[4] = P4_G32(0x79b6a0);
        session.prepare_state = 1;
        const uint32_t assertions = le32(session.preparation.header + 88);
        for (unsigned i = 0; i < 5; i++)
            if ((assertions & (1u << i)) && session.context_before[i] != le32(session.preparation.header + 92 + i * 4))
                prepare_finish("preparation_error", "source_context_assertion_mismatch", 1);
        void *logo = kwlnTaskGetTaskByName(P4_G2H(0x005e57a8u));
        kwlnTaskDestroyWithHierarchy(logo);
        /* The actual clndSceneUpdate00123e80 post-LOAD branch, without
         * fabricating its owner task or claiming an active-VM checkpoint. */
        GP(0xb1f0) = 1;
        calendarSyncFromDate();
        calendarSetMode(1);
        if (datGetPendingDateIndex() == 9 && datGetPendingTimeSlot() == 5)
            prepare_finish("preparation_error", "source_requires_newgame_not_battle_ready_field", 1);
        const uint32_t packet = p4_gport_alloc(28, 16);
        if (!packet) prepare_finish("preparation_error", "field_resume_packet_allocation", 4);
        memset(P4_G2H(packet), 0, 28);
        if (session.context_before[4]) {
            P4_G32(packet) = session.context_before[4];
            P4_G32(packet + 4) = 0xff;
            sequenceRequest(0xb, packet, 8, 2);
        } else {
            if (session.context_before[2] > INT16_MAX || session.context_before[3] > INT16_MAX)
                prepare_finish("preparation_error", "source_field_ids_outside_native_resume_packet", 1);
            P4_G16(packet) = (uint16_t)session.context_before[2];
            P4_G16(packet + 2) = (uint16_t)session.context_before[3];
            P4_G16(packet + 4) = 0xff;
            sequenceRequest(9, packet, 28, 0);
        }
        return;
    }
    if (GP(0xb3ac)) prepare_finish("preparation_error", "source_requires_live_battle_entry", 1);
    if (!field_interactive()) {
        if (session.updates >= session.limit) prepare_finish("preparation_error", "field_resume_update_limit", 4);
        return;
    }
    if (!current_context(session.context_after)) prepare_finish("preparation_error", "native_field_context_unavailable", 4);
    session.prepare_state = 2;
    if (memcmp(session.context_before, session.context_after, sizeof session.context_before))
        prepare_finish("preparation_error", "native_field_resume_changed_source_context", 1);
    /* Growth has an explicit independent seed boundary after real field
     * readiness; asynchronous load duration does not select stat rolls. */
    if (!restore_rng() || !rng_hash(&session.prepare_rng_entry))
        prepare_finish("preparation_error", "preparation_rng_seed_or_snapshot", 4);
    session.prepare_rng_valid = 1;
    session.entry_draws = p4pc_rng_draws;
    const char *reason = p4_battle_prepare_apply(&session.preparation);
    if (reason) prepare_finish("preparation_error", reason, 1);
    if (!current_context(session.context_after) ||
        memcmp(session.context_before, session.context_after, sizeof session.context_before))
        prepare_finish("preparation_error", "native_preparation_changed_source_context", 4);
    const uint32_t size_slot = p4_gport_alloc(4, 4);
    if (!size_slot) prepare_finish("preparation_error", "native_save_size_allocation", 4);
    void *image = datSaveBuildImage(0, P4_G2H(size_slot), 0);
    session.prepared_size = P4_G32(size_slot);
    if (!image || !guest_range(P4_H2G(image), session.prepared_size) ||
        !p4_battle_save_valid(image, session.prepared_size))
        prepare_finish("preparation_error", "native_save_build_export_invalid", 4);
    p4_content_digest(image, session.prepared_size, session.prepared_hash);
    if (!rng_hash(&session.prepare_rng_exit)) prepare_finish("preparation_error", "native_preparation_exit_rng_snapshot", 4);
    FILE *output = fopen(session.prepared_image, "wbx");
    if (!output) prepare_finish("preparation_error", "cannot_create_exclusive_prepared_image", 5);
    const int written = fwrite(image, 1, session.prepared_size, output) == session.prepared_size && !ferror(output);
    const int close_error = fclose(output);
    if (!written || close_error) prepare_finish("preparation_error", "cannot_finish_native_prepared_image", 5);
    session.prepare_complete = 1;
    prepare_finish("prepared", "native_save_build_image_complete", 0);
}
/* Retail addresses of the post-battle skill-offer prompt's owners. */
#define MESSAGE_WINDOW_CURRENT 0x00882f60u /* NmMesWindowRecord returned by nmMesWindowCurrent */
#define MESSAGE_WINDOW_SLOTS 0x00881808u   /* ItfMesWindowSlot[], 0x20 stride, window at +0 */
/* The skill-replace task (0011f5a0) is created as "shdPersonaSkillSelectSeq";
 * kwlnTaskCreate keeps 23 characters but sums all 24, so kwlnTaskGetTaskByName
 * finds it under neither name. Walk the three scheduler lists (sdkTask.c) and
 * match the stored 0x18-byte name instead. */
static uint32_t skill_select_task(void)
{
    static const char stored[0x18] = "shdPersonaSkillSelectSe";
    static const uint16_t heads[3] = {0xb9ec, 0xba04, 0xb9f8};
    for (unsigned list = 0; list < 3; list++) {
        uint32_t node = GP(heads[list]);
        for (unsigned n = 0; node; n++) {
            if (n == 4096 || !guest_range(node, 0x40)) {
                session.observation_error = "result_task_list_invalid";
                return 0;
            }
            if (!memcmp(P4_G2H(node), stored, sizeof stored)) return node;
            node = P4_G32(node + 0x3c); /* listNext */
        }
    }
    return 0;
}
/* Battle result phase 16 waits for the result task, which waits for every
 * persona level-up. With eight skills, a level-up asks which skill to forget;
 * the named policies' periodic Cross would re-select forever (the Yes/No
 * defaults to No) or, racing the list, forget a known skill. While that task
 * reads input (states 0-3 and 6) the session owns input and declines the
 * offer through the real UI. Returns 1 when it owns input this update. */
static int result_skill_prompt(void)
{
    const uint32_t battle = GP(0xb3ac);
    if (!battle || !guest_range(battle, 0x300) || P4_G32(battle + 0x2fc) != 16) return 0;
    const uint32_t task = skill_select_task();
    if (!task) return 0;
    const uint32_t work = guest_range(task, 0x3c) ? P4_G32(task + 0x38) : 0;
    const uint32_t parent = work && guest_range(work, 0x10) ? P4_G32(work + 0xc) : 0;
    const uint32_t panel = parent && guest_range(parent, 0x3c) ? P4_G32(parent + 0x38) : 0;
    if (!panel || !guest_range(panel, 0x538)) {
        session.observation_error = "result_skill_prompt_layout";
        return 1;
    }
    const int32_t state = (int32_t)P4_G32(work + 8);
    if (state < 0 || (state > 3 && state != 6)) return 0;
    if (session.updates < session.prompt_ready) return 1;
    P4BattleSkillPrompt prompt = {0};
    prompt.state = state;
    prompt.list_active = (P4_G32(panel + 0x534) & 0x800) != 0;
    prompt.list_cursor = (int8_t)P4_G8(panel + 0x88);           /* ShdSkillState at +0x84 */
    prompt.pending = P4_G16(panel + 0x8c + 0x224);               /* ShdSkillPool.pendingCount */
    prompt.replacement_pending = P4_G16(work + 6) == P4_G16(panel + 0x8c + 0x66); /* pending[0].id */
    const int32_t handle = (int32_t)P4_G32(MESSAGE_WINDOW_CURRENT);
    const int8_t window_state = (int8_t)P4_G8(MESSAGE_WINDOW_CURRENT + 4); /* 1 text, 2 choice/close */
    if ((window_state == 1 || window_state == 2) && handle >= 0 && handle < 0x100) {
        const uint32_t window = P4_G32(MESSAGE_WINDOW_SLOTS + (uint32_t)handle * 0x20);
        if (window && guest_range(window, 0x50)) {
            prompt.row_waiting = (int8_t)P4_G8(window + 0x24) == 1;  /* ItfMesRow.state */
            prompt.choice_active = (int16_t)P4_G16(window + 0x48) == 2 && (int16_t)P4_G16(window + 0x4e) == 2;
            prompt.choice_cursor = (int16_t)P4_G16(window + 0x4a);
        }
    }
    const unsigned bits = p4_battle_skill_prompt_buttons(&prompt);
    if (!bits) return 1;
    if (state == 3 && prompt.choice_active && prompt.replacement_pending && bits == P4PC_PAD_CROSS)
        session.skill_offers_declined++;
    p4pc_pad_inject(bits, 1);
    session.prompt_ready = session.updates + 4; /* release so the next press is a new edge */
    return 1;
}
void p4_battle_sim_before_update(void)
{
    if (p4_battle_live_active) { p4_battle_live_before_update(); return; }
    if (!p4_battle_sim_active) return;
    if (session.preparing) { prepare_before_update(); return; }
    if (!session.launched) {
        if (session.boot_updates >= session.boot_limit) finish("engine_error", "bootstrap_update_limit", 4);
        if (!boot_ready()) return;
        const char *reason = restore_source();
        if (reason) finish("engine_error", reason, 4);
        if (session.mod_equipment_count) {
            /* After the ENTRY image's load (which resets the side state) and before the party and encounter are
             * built: the item service checks each row's kind/slot and the unit's compatibility bit. */
            P4GameItems *items = p4_game_tables_items_active();
            if (!items) finish("engine_error", "mod_equipment_without_item_table", 4);
            for (unsigned i = 0; i < session.mod_equipment_count; i++)
                if (p4_game_tables_items_equipment_set(items, session.mod_equipment[i].actor, session.mod_equipment[i].slot,
                                                       session.mod_equipment[i].item) != P4_TABLE_OK)
                    finish("engine_error", "mod_equipment_refused", 4);
        }
        if (session.policy) {
            for (unsigned i = 0; i < 3; i++) {
                const int16_t id = datGetPartyId((int32_t)i);
                if (id >= 2 && id <= 10) datSetUnitAiTactic(id, 4);
            }
        }
        if (session.flags & 4) {
            int32_t ids[4] = { 1, datGetPartyId(0), datGetPartyId(1), datGetPartyId(2) };
            for (unsigned i = 0; i < 4; i++) if (ids[i] >= 1 && ids[i] <= 10) {
                void *unit = datUnitGetRecord((int16_t)ids[i]);
                if (datCalcGetHp(unit)) {
                    datSetUnitHp(ids[i], (int32_t)datCalcMaxHpById(ids[i]));
                    datSetUnitSp(ids[i], (int32_t)datCalcMaxSpById(ids[i]));
                }
            }
        }
        if (!party_json(session.initial_party, sizeof session.initial_party)) finish("engine_error", "entry_party_observation", 4);
        void *logo = kwlnTaskGetTaskByName(P4_G2H(0x005e57a8u));
        kwlnTaskDestroyWithHierarchy(logo);
        if (!restore_rng()) finish("engine_error", "entry_rng_state", 4);
        session.entry_draws = p4pc_rng_draws;
        session.launched = 1;
        const P4BattleObserver observer = {NULL, admission_event, cost_event, hit_event, command_event, NULL};
        session.observer_token = p4_battle_observer_add(&observer);
        if (!session.observer_token) finish("engine_error", "native_observer_registration", 4);
        /* The walk policy sets Direct Commands like the named policies but owns input through the walker. */
        if (!p4_battle_policy_begin(session.policy == WALK_POLICY ? 0 : session.policy, session.latency))
            finish("policy_error", p4_battle_policy_error(), 4);
        p4pc_pad_autopress_set_clock(p4_battle_sim_update_count);
        if (!observe("entry", 1)) finish("engine_error", "entry_observation", 4);
        if (!p4_battle_encounter_start(session.encounter)) finish("engine_error", "invalid_encounter", 4);
    }
    if (session.updates >= session.limit) finish("update_limit", "completed_native_update_limit", 3);
    if (session.policy && result_skill_prompt()) return;
    p4_battle_policy_before_update();
    if (session.walk) { walk_plan_update(); return; }
    if (session.tape_index < session.tape_count) {
        const uint8_t *record = session.tape + session.tape_index * 12;
        if (le32(record) == session.updates) {
            p4pc_pad_inject(le32(record + 4), le32(record + 8));
            session.tape_index++;
        }
    }
}
void p4_battle_sim_after_update(int completed)
{
    if (p4_battle_live_active) { p4_battle_live_after_update(completed); return; }
    if (!p4_battle_sim_active || !completed) return;
    if (session.preparing) {
        if (session.prepare_state) session.updates++; else session.boot_updates++;
        return;
    }
    if (!session.launched) { session.boot_updates++; return; }
    session.updates++;
    const uint32_t battle = GP(0xb3ac);
    if (battle) {
        session.seen_battle = 1;
        if (!observe("update", 0)) finish("engine_error", "invalid_runtime_observation", 4);
        if (session.observation_error) finish("engine_error", session.observation_error, 4);
        if (p4_battle_policy_error()) finish("policy_error", p4_battle_policy_error(), 4);
        const unsigned terminal = P4_G16(battle + 0x1c);
        if (terminal != 2) session.defeat_cause = NULL; /* A real native rescue may resume play. */
        else if (!session.defeat_cause) {
            session.defeat_cause = (P4_G32(battle + 0x10) & 0x10000000u) ? "native_forced_defeat_flag" :
                datCalcIsDead(datUnitGetRecord(1), 0) ? "protagonist_terminal_ko" : "native_party_terminal_defeat";
        }
        /* Final phase17 after native result/chain/revival decisions. Wait for
         * the actual exit owner to drain record/sound work and clear ACTIVE.
         * Never call mutating btlCheckBattleEnd to speculate on an outcome. */
        if (P4_G32(battle + 0x2fc) == 17 && !(P4_G32(battle + 0xc) & 1u)) {
            unsigned end = P4_G16(battle + 0x1c);
            if (end == 1) finish("win", "native_battle_exit_victory", 0);
            if (end == 2) finish("defeat", session.defeat_cause ? session.defeat_cause : "native_battle_exit_defeat", 0);
            if (end == 3 && P4_G16(battle + 0xc5e)) finish("escape", "native_committed_party_group_escape", 0);
            if (end == 3) finish("scripted_conclusion", "native_battle_exit_no_party", 0);
            finish("engine_error", "native_exit_without_terminal_state", 4);
        }
    } else if (session.seen_battle) finish("engine_error", "battle_teardown_before_terminal_capture", 4);
    if (session.action_count >= session.action_limit)
        finish("action_limit", "completed_native_update_action_bound", 3);
}
