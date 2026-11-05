/* Game services for mods (include/p4pc/mod_services.h, API 1.4).
 *
 * Host adapter. It reads guest scene state, owns native observer hooks and a
 * one-slot request state machine that the scheduler frame boundary drives
 * (P4_MOD_SAFE_SERVICES in p4_mod_event_frame), and calls the game only
 * through services_native.c at that boundary. Hooks are created disabled
 * when the first mod VM initializes and enabled only while a service needs
 * them, so a run without service use executes none of this per frame. */
#include "p4pc/mod_services.h"
#include "p4pc/mod_authorize.h"
#include "p4pc/mod_entry.h"
#include "p4pc/mod_events.h"
#include "p4pc/mod_hooks.h"
#include "p4pc/mod_status.h"
#include "p4pc/game_enemies_native.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/gmem.h"
#include "p4pc/os.h"
#include "p4pc/settings_native.h"
#include "../files/vfs.h"
#include "../../render/p4gl.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Guest anchors (gp 0x007690F0). */
#define SEQUENCE_TASK P4_GPADDR(0xb1a8)  /* sequence manager task (sequence.c) */
#define CAMP_WORK P4_GPADDR(0xb1ec)      /* cmpMain work; cleared on destroy */
#define RESET_PENDING P4_GPADDR(0x85b8)  /* kwlnRunFrame's reset branch flag */
#define ENCOUNTER_TABLE P4_GPADDR(0xb414)
#define ENEMY_TABLE P4_GPADDR(0xb3c4)
#define FIELD_ROOT 0x007D2510u           /* field root task handle (k_field.c) */
#define STEP_TASK 0x00764334u            /* dungeon step task (K_SeqFldStepStart) */
#define EVENT_FLAG 0x00764604u           /* message window/event holds input */
#define BATTLE_CONTEXT 0x0076449Cu       /* btlTaskCreate context */
#define SCENE_OBJECT 0x00762EA0u
#define TASK_WORK 0x38u
#define TASK_NEXT 0x3cu

enum { SEQ_NONE = -1, SEQ_BATTLE = 6, SEQ_FIELD = 9, SEQ_EVENT = 0xA, SEQ_FLOOR = 0xB };

static int range(uint32_t address, uint32_t size)
{
    return address >= 0x00100000u && address < 0x02000000u && size <= 0x02000000u - address;
}
static uint32_t word(uint32_t address)
{
    uint32_t value = 0;
    if (range(address, 4)) memcpy(&value, P4_G2H(address), 4);
    return value;
}
static uint16_t half(uint32_t address)
{
    uint16_t value = 0;
    if (range(address, 2)) memcpy(&value, P4_G2H(address), 2);
    return value;
}

/* ---- scene state */

static int task_named(const char *name)
{
    static const uint32_t lists[3] = {0x00764adcu, 0x00764af4u, 0x00764ae8u};
    size_t length = strlen(name);
    for (int l = 0; l < 3; ++l) {
        uint32_t node = word(lists[l]);
        for (int i = 0; node && i < 256; ++i) {
            if (range(node, 0x40) && !memcmp(P4_G2H(node), name, length + 1)) return 1;
            node = word(node + TASK_NEXT);
        }
    }
    return 0;
}

static int sequence_state(int *pending)
{
    uint32_t task = word(SEQUENCE_TASK), work = task ? word(task + TASK_WORK) : 0;
    *pending = work ? (word(work) & 1u) != 0 : 0;
    return work ? (int32_t)word(work + 4) : SEQ_NONE;
}

/* events.c's ready predicate: root state 12..14, no scene transition bits,
 * scene ids equal the root's requested ids. Returns the root state or -1. */
static int field_root_state(uint32_t *ids)
{
    uint32_t root = word(FIELD_ROOT), work = root ? word(root + TASK_WORK) : 0;
    uint32_t scene = word(SCENE_OBJECT);
    if (!range(work, 0x24)) return -1;
    *ids = word(work + 0x18);
    if (!range(scene, 0x14) || (word(scene + 0xc) & 6u) || (word(scene) & 0xffffu) != (*ids & 0xffffu) ||
        (word(scene + 4) & 0xffffu) != (*ids >> 16))
        return -1;
    return (int)word(work);
}

/* Origins recorded from the sequence start callbacks (see prepare()). */
static struct {
    int valid;
    uint16_t field, room, entry, spawn, direction;
} origin;

typedef struct Request {
    size_t owner;
    int kind, state, battle_seen, outcome;
    uint32_t field, room, entry, direction, encounter, waited;
    uint16_t back_field, back_room, back_entry, back_direction;
    const char *detail;
    char reason[128];
} Request;
enum { KIND_WARP = 1, KIND_ENCOUNTER };
enum { REQ_IDLE, REQ_QUEUED, REQ_TRANSITION, REQ_BATTLE, REQ_RETURN, REQ_DONE, REQ_REFUSED };
static Request request;

static int request_busy(void)
{
    return request.state == REQ_QUEUED || request.state == REQ_TRANSITION ||
           request.state == REQ_BATTLE || request.state == REQ_RETURN;
}

static void scene_state(P4ModSceneState *s, int include_request)
{
    memset(s, 0, sizeof *s);
    int pending = 0;
    s->sequence = sequence_state(&pending);
    uint32_t ids = 0;
    int root = field_root_state(&ids);
    s->field = root >= 0 ? ids & 0xffffu : 0;
    s->room = root >= 0 ? ids >> 16 : 0;
    s->entry = origin.valid && root >= 0 && origin.field == s->field && origin.room == s->room ? origin.entry : 0xffffffffu;
    uint32_t step = word(STEP_TASK), step_work = step ? word(step + TASK_WORK) : 0;
    s->step = s->sequence == SEQ_FLOOR && step_work ? word(step_work + 4) : 0xffffffffu;
    s->control = task_named("rotate pc");
    s->event = word(EVENT_FLAG) != 0;
    s->menu = word(CAMP_WORK) != 0 || p4_settings_native_active();
    int battle = word(BATTLE_CONTEXT) != 0;
    s->pending = pending || (include_request && request_busy());
    switch (s->sequence) {
    case SEQ_NONE: s->context = P4_MOD_SCENE_NONE; break;
    case SEQ_BATTLE: s->context = P4_MOD_SCENE_BATTLE; break;
    case SEQ_FIELD: s->context = battle ? P4_MOD_SCENE_BATTLE : P4_MOD_SCENE_FIELD; break;
    case SEQ_EVENT: s->context = P4_MOD_SCENE_EVENT; break;
    case SEQ_FLOOR: s->context = battle ? P4_MOD_SCENE_BATTLE : P4_MOD_SCENE_DUNGEON; break;
    default: s->context = s->sequence >= 0x1b ? P4_MOD_SCENE_TITLE : P4_MOD_SCENE_OTHER; break;
    }
    if (s->context != P4_MOD_SCENE_FIELD && s->context != P4_MOD_SCENE_DUNGEON) s->reason = "not in a field";
    else if (root != 13) s->reason = "the field is loading or running a script";
    else if (s->pending) s->reason = "a scene change is already pending";
    else if (s->event) s->reason = "an event or message holds the input";
    else if (s->menu) s->reason = "a menu is open";
    else if (!s->control) s->reason = "the player is not in control";
    else s->ready = 1;
}

static int activate_scene(void);
void p4_mod_scene_state(P4ModSceneState *out)
{
    activate_scene(); /* the first scene query starts observing field entries */
    scene_state(out, 1);
}

/* ---- native seams */

typedef struct Seam {
    uint32_t address;
    const char *signature;
    int phase;
    P4ModHook *hook;
} Seam;
enum { SEAM_FIELD_START, SEAM_FIELD_POS, SEAM_STEP_START, SEAM_FRAME_BEFORE, SEAM_FRAME_AFTER,
       SEAM_BATTLE_END, SEAM_DRAW, SEAM_COUNT };
static Seam seams[SEAM_COUNT] = {
    {0x00170220u, "vp", P4_MOD_BEFORE, NULL},   /* K_SeqFieldRecordStart: 0x1C record */
    {0x00170350u, "vp", P4_MOD_BEFORE, NULL},   /* K_SeqFieldPosStart: no entry record */
    {0x001704a0u, "vp", P4_MOD_BEFORE, NULL},   /* K_SeqFldStepStart: generated floor */
    {0x00101270u, "i", P4_MOD_BEFORE, NULL},    /* kwlnRunFrame */
    {0x00101270u, "i", P4_MOD_AFTER, NULL},
    {0x00192cd0u, "i", P4_MOD_BEFORE, NULL},    /* btlRelease: outcome */
    {0x00102550u, "ip", P4_MOD_BEFORE, NULL},   /* mainDrawFrameUpdate: before the OT flush */
};
static int prepared;
static uint32_t updates, reset_branch;
static int clock_on;
static uint64_t wall_start;

/* Each service checks only the seams it uses; a missing seam refuses that
 * service with a reason and never disables the others. */
static int seams_ready(int first, int last)
{
    for (int i = first; i <= last; ++i)
        if (!seams[i].hook) return 0;
    return 1;
}

static void seam_enable(int seam, int enabled)
{
    if (seams[seam].hook) p4_mod_hook_enable(seams[seam].hook, enabled);
}

static void want_safe_point(void) { p4_mod_safe_point |= P4_MOD_SAFE_SERVICES; }

/* ---- camera lease */

static struct {
    size_t owner;
    uint32_t generation, camera, root, applied_frame;
    char name[48];
    int active, input, pose_set, applied, sign;
    float up, pose[6];
    uint8_t saved_model[64], saved_window[16];
    const char *why;
} cam;
static uint32_t next_generation = 1;
int p4_mod_camera_override;
int p4_mod_input_fence;

static void camera_end_lease(const char *why)
{
    if (!cam.active) return;
    cam.active = 0;
    cam.why = why;
    p4_mod_camera_override = cam.applied; /* an open view is still restored at END */
    if (cam.input) p4_mod_input_fence = 2; /* drain: neutral until every button is released */
    cam.input = 0;
}

/* ---- HUD groups: render-only suppression of their draws at the OT flush */

enum { GROUP_BUCKETS, GROUP_CALLBACKS };
static const struct {
    const char *name;
    int kind;
    uint32_t low, high;
} groups[] = {
    /* src/render/p4gl_im2d.c: retail layers 0x4a..0x53 are the banner and map. */
    {"field_banner", GROUP_BUCKETS, 0x00794c60u, 0x00794c60u},
    {"field_map", GROUP_BUCKETS, 0x00794c90u, 0x00794e10u},
    /* Node callbacks the renderer anchors as edge HUD. */
    {"calendar", GROUP_CALLBACKS, 0x00122a40u, 0x00122a40u},
    {"party", GROUP_CALLBACKS, 0x002167f0u, 0x002167f0u},
    {"battle_commands", GROUP_CALLBACKS, 0x0020b1a0u, 0x0020b5b0u},
};
enum { GROUP_COUNT = sizeof groups / sizeof groups[0], HIDE_OWNERS = 8 };
static size_t hide_owner[GROUP_COUNT][HIDE_OWNERS];
static unsigned hide_count[GROUP_COUNT];
static uint32_t hud_scene_root, hud_scene_battle;
static int hud_scene_sequence;
static int hud_rows;

static int group_index(const char *name)
{
    for (int i = 0; i < (int)GROUP_COUNT; ++i)
        if (!strcmp(groups[i].name, name)) return i;
    return -1;
}
static int any_hidden(void)
{
    for (int i = 0; i < (int)GROUP_COUNT; ++i)
        if (hide_count[i]) return 1;
    return 0;
}
static void draw_seam_update(void) { seam_enable(SEAM_DRAW, hud_rows); }

/* The renderer drops the draws of hidden buckets and node callbacks while
 * their nodes still run (src/render/p4gl_im2d.c p4gl_hud_hidden_set). It
 * keeps pointers to these arrays, so they are static. */
static uint32_t hidden_queues[16], hidden_callbacks[GROUP_COUNT][2];
static void publish_hidden(void)
{
    unsigned queueCount = 0, callbackCount = 0;
    for (int i = 0; i < (int)GROUP_COUNT; ++i) {
        if (!hide_count[i]) continue;
        if (groups[i].kind == GROUP_BUCKETS) {
            for (uint32_t bucket = groups[i].low; bucket <= groups[i].high && queueCount < 16; bucket += 0x30u)
                hidden_queues[queueCount++] = bucket;
        } else {
            hidden_callbacks[callbackCount][0] = groups[i].low;
            hidden_callbacks[callbackCount++][1] = groups[i].high;
        }
    }
    p4gl_hud_hidden_set(hidden_queues, queueCount, (const uint32_t (*)[2])hidden_callbacks, callbackCount);
}
static void release_hides(size_t owner, int everyone)
{
    for (int i = 0; i < (int)GROUP_COUNT; ++i) {
        unsigned kept = 0;
        for (unsigned j = 0; j < hide_count[i]; ++j)
            if (!everyone && hide_owner[i][j] != owner) hide_owner[i][kept++] = hide_owner[i][j];
        hide_count[i] = kept;
    }
    publish_hidden();
}

/* ---- seam callbacks (no Lua, no game calls) */

static int seam_callback(P4ModCall *call, void *user)
{
    Seam *seam = user;
    switch ((int)(seam - seams)) {
    case SEAM_FIELD_START: {
        uint32_t record = (uint32_t)call->args[0].u;
        origin.valid = range(record, 0x1c);
        if (origin.valid) {
            origin.field = half(record);
            origin.room = half(record + 2);
            origin.entry = half(record + 4);
            origin.spawn = half(record + 6);
            origin.direction = half(record + 8);
        }
        break;
    }
    case SEAM_FIELD_POS: case SEAM_STEP_START:
        origin.valid = 0;
        break;
    case SEAM_FRAME_BEFORE:
        reset_branch = word(RESET_PENDING);
        break;
    case SEAM_FRAME_AFTER:
        if (call->result->i == 1 && !reset_branch) ++updates;
        break;
    case SEAM_BATTLE_END: {
        uint32_t battle = word(BATTLE_CONTEXT);
        request.outcome = battle ? (int16_t)half(battle + 0x1c) : -1;
        break;
    }
    case SEAM_DRAW:
        p4_mod_status_draw_hud();
        break;
    }
    return 0;
}

static int pump(uint32_t frame);

static void prepare(void)
{
    if (prepared) return;
    prepared = 1;
    for (int i = 0; i < SEAM_COUNT; ++i) {
        P4ModFunction *function = p4_mod_function_find(seams[i].address, seams[i].signature);
        seams[i].hook = function ? p4_mod_hook_add(P4_MOD_NATIVE_OWNER, function, seams[i].phase, seam_callback, &seams[i]) : NULL;
        if (!seams[i].hook) {
            fprintf(stderr, "p4pc: mods: service seam unavailable: %08x %s\n", seams[i].address, seams[i].signature);
            continue;
        }
        p4_mod_hook_enable(seams[i].hook, 0);
    }
    p4_mod_events_set_services(pump);
}

/* Origins must be observed before a launch: the field-start observers run
 * from the first scene/encounter use for the rest of the process. */
static int activate_scene(void)
{
    if (!seams_ready(SEAM_FIELD_START, SEAM_STEP_START)) return -1;
    seam_enable(SEAM_FIELD_START, 1);
    seam_enable(SEAM_FIELD_POS, 1);
    seam_enable(SEAM_STEP_START, 1);
    return 0;
}

/* ---- requests */

static void refuse(const char *why)
{
    request.state = REQ_REFUSED;
    snprintf(request.reason, sizeof request.reason, "%s", why);
    request.detail = request.reason;
    fprintf(stderr, "p4pc: mods: service request refused: %s\n", why);
}

static int field_resource(uint32_t field, uint32_t room)
{
    char path[48];
    int directory = 0;
    long long size = 0;
    snprintf(path, sizeof path, "field/pack/f%03u_%03u.p00", (unsigned)field, (unsigned)room);
    return vfs_stat(path, &directory, &size) == 0 && !directory && size > 0;
}

int p4_mod_scene_available(uint32_t field, uint32_t room)
{
    return field && field <= 999 && room <= 999 && field_resource(field, room);
}

int p4_mod_scene_warp(size_t owner, uint32_t field, uint32_t room, uint32_t entry, uint32_t direction, const char **reason)
{
    P4ModSceneState s;
    if (activate_scene()) { *reason = "the scene service seams are unavailable"; return -1; }
    if (request_busy()) { *reason = "another scene request is in flight"; return -1; }
    scene_state(&s, 0);
    if (!s.ready || s.context != P4_MOD_SCENE_FIELD) {
        *reason = s.ready ? "warps start from a fixed field" : s.reason;
        return -1;
    }
    if (!field || field > 999 || room > 999 || entry > 0xffffu || direction > 0xffffu) {
        *reason = "field, room, entry or direction is out of range";
        return -1;
    }
    if (!field_resource(field, room)) { *reason = "the disc has no resources for that field and room"; return -1; }
    memset(&request, 0, sizeof request);
    request = (Request){.owner = owner, .kind = KIND_WARP, .state = REQ_QUEUED, .outcome = -1,
                        .field = field, .room = room, .entry = entry, .direction = direction, .detail = "queued"};
    want_safe_point();
    return 0;
}

uint32_t p4_mod_encounter_count(void)
{
    uint32_t base, count;
    return p4_game_table_extent_rows(ENCOUNTER_TABLE, 0x18, &base, &count) ? count : 0;
}

int p4_mod_encounter_info(uint32_t id, uint16_t enemies[5], uint16_t *field, uint16_t *variant)
{
    uint32_t base, count, enemy_base, enemy_count;
    if (!p4_game_table_extent_rows(ENCOUNTER_TABLE, 0x18, &base, &count) || id >= count ||
        !p4_game_table_extent_rows(ENEMY_TABLE, 0x3c, &enemy_base, &enemy_count)) return -1;
    const uint32_t limit = p4_game_enemies_native_limit();
    int populated = 0;
    for (unsigned i = 0; i < 5; ++i) {
        enemies[i] = half(base + id * 0x18u + 8u + i * 2u);
        if (enemies[i] >= limit || enemies[i] >= enemy_count) return -1;
        populated |= enemies[i] != 0;
    }
    *field = half(base + id * 0x18u + 0x12u);
    *variant = half(base + id * 0x18u + 0x14u);
    return populated ? 0 : -1;
}

int p4_mod_encounter_launch(size_t owner, uint32_t id, const char **reason)
{
    P4ModSceneState s;
    uint16_t enemies[5], field, variant;
    if (activate_scene() || !seams_ready(SEAM_BATTLE_END, SEAM_BATTLE_END)) {
        *reason = "the scene service seams are unavailable";
        return -1;
    }
    if (request_busy()) { *reason = "another scene request is in flight"; return -1; }
    scene_state(&s, 0);
    if (!s.ready || s.context != P4_MOD_SCENE_FIELD) {
        *reason = s.ready ? "launch from a fixed field; generated floors cannot be returned to" : s.reason;
        return -1;
    }
    if (!origin.valid || origin.field != s.field || origin.room != s.room) {
        *reason = "this field was entered before the service started; re-enter it first";
        return -1;
    }
    if (p4_mod_encounter_info(id, enemies, &field, &variant)) { *reason = "not a valid loaded encounter"; return -1; }
    memset(&request, 0, sizeof request);
    request = (Request){.owner = owner, .kind = KIND_ENCOUNTER, .state = REQ_QUEUED, .outcome = -1,
                        .encounter = id, .back_field = origin.field, .back_room = origin.room,
                        .back_entry = origin.entry, .back_direction = origin.direction, .detail = "queued"};
    want_safe_point();
    return 0;
}

const char *p4_mod_services_status(size_t owner, const char **detail, int *outcome)
{
    static const char *const names[] = {"idle", "requested", "requested", "battle", "returning", "done", "refused"};
    if (request.owner != owner || request.state == REQ_IDLE) { *detail = ""; *outcome = -1; return "idle"; }
    *detail = request.detail ? request.detail : "";
    *outcome = request.outcome;
    return names[request.state];
}

static int arrived(uint16_t field, uint16_t room)
{
    uint32_t ids = 0;
    int state = field_root_state(&ids);
    return state >= 12 && state <= 14 && (ids & 0xffffu) == field && (ids >> 16) == room;
}

static void run_request(void)
{
    P4ModSceneState s;
    int pending = 0;
    switch (request.state) {
    case REQ_QUEUED:
        scene_state(&s, 0);
        if (!s.ready) { refuse(s.reason); return; }
        if (!p4_mod_services_native_resources_ready()) {
            if (++request.waited > 600) refuse("scene resources stayed busy");
            return;
        }
        request.waited = 0;
        if (request.kind == KIND_WARP) {
            if (!p4_mod_services_native_field(request.field, request.room, request.entry, request.direction)) {
                refuse("no port record for the field request");
                return;
            }
            request.state = REQ_TRANSITION;
            request.detail = "loading";
            return;
        }
        if (p4_mod_authorize_mask & P4_MOD_AUTH_ENCOUNTER_BIT) {
            P4ModEvent context = {"encounter", 2, {{"encounter", request.encounter}, {"source", 1}}};
            char why[128] = "";
            if (!p4_mod_authorize(P4_MOD_AUTH_ENCOUNTER, &context, why, sizeof why)) {
                refuse(why[0] ? why : "an encounter authorizer refused the launch");
                return;
            }
        }
        seam_enable(SEAM_BATTLE_END, 1);
        if (!p4_mod_services_native_encounter(request.encounter)) {
            seam_enable(SEAM_BATTLE_END, 0);
            refuse("the encounter adapter refused the record or the party");
            return;
        }
        request.state = REQ_BATTLE;
        request.detail = "battle";
        return;
    case REQ_TRANSITION:
        if (arrived((uint16_t)request.field, (uint16_t)request.room)) {
            request.state = REQ_DONE;
            request.detail = "arrived";
        } else if (++request.waited > 5400) {
            request.state = REQ_DONE;
            request.detail = "the field did not report ready";
        }
        return;
    case REQ_BATTLE: {
        int sequence = sequence_state(&pending);
        if (p4_mod_services_native_battle_running()) { request.battle_seen = 1; return; }
        if (!request.battle_seen) {
            if (++request.waited > 600) {
                seam_enable(SEAM_BATTLE_END, 0);
                request.state = REQ_DONE;
                request.detail = "the battle did not start";
            }
            return;
        }
        if (pending || sequence != SEQ_NONE) {
            if (sequence == SEQ_BATTLE) return; /* the battle sequence has not finished its poll yet */
            seam_enable(SEAM_BATTLE_END, 0);
            request.state = REQ_DONE;
            request.detail = "the game chose the scene after the battle";
            return;
        }
        seam_enable(SEAM_BATTLE_END, 0);
        if (request.outcome == 2) p4_mod_services_native_revive_party();
        if (!p4_mod_services_native_field(request.back_field, request.back_room, request.back_entry, request.back_direction)) {
            request.state = REQ_DONE;
            request.detail = "no port record for the return";
            return;
        }
        request.state = REQ_RETURN;
        request.waited = 0;
        request.detail = "returning";
        return;
    }
    case REQ_RETURN:
        if (arrived(request.back_field, request.back_room)) {
            request.state = REQ_DONE;
            request.detail = "returned";
        } else if (++request.waited > 5400) {
            request.state = REQ_DONE;
            request.detail = "the origin field did not report ready";
        }
        return;
    }
}

static void check_leases(void)
{
    if (cam.active) {
        P4ModSceneState s;
        scene_state(&s, 1);
        if (word(FIELD_ROOT) != cam.root || s.context != P4_MOD_SCENE_FIELD) camera_end_lease("the scene changed");
        else if (s.event) camera_end_lease("an event took over the scene");
        else if (s.menu) camera_end_lease("a menu opened");
    }
    if (any_hidden()) {
        int pending = 0;
        if (word(FIELD_ROOT) != hud_scene_root || word(BATTLE_CONTEXT) != hud_scene_battle ||
            sequence_state(&pending) != hud_scene_sequence)
            release_hides(0, 1);
    }
}

static int pump(uint32_t frame)
{
    (void)frame;
    if (request_busy()) run_request();
    check_leases();
    return request_busy() || cam.active || any_hidden();
}

/* ---- clock */

int p4_mod_clock_updates(uint32_t *out)
{
    if (!seams_ready(SEAM_FRAME_BEFORE, SEAM_FRAME_AFTER)) return -1;
    if (!clock_on) {
        clock_on = 1;
        seam_enable(SEAM_FRAME_BEFORE, 1);
        seam_enable(SEAM_FRAME_AFTER, 1);
    }
    *out = updates;
    return 0;
}
uint64_t p4_mod_clock_wall_ns(void)
{
    uint64_t now = p4_os_now_ns();
    if (!wall_start) wall_start = now ? now : 1;
    return now - wall_start;
}

/* ---- camera */

uint32_t p4_mod_camera_acquire(size_t owner, const char *name, const char **reason, const char **owner_name)
{
    P4ModSceneState s;
    *owner_name = NULL;
    if (cam.active) {
        *reason = cam.owner == owner ? "this mod already holds the camera" : "another mod holds the camera";
        *owner_name = cam.name;
        return 0;
    }
    scene_state(&s, 1);
    if (!s.ready || s.context != P4_MOD_SCENE_FIELD) {
        *reason = s.ready ? "the free camera supports fixed fields" : s.reason;
        return 0;
    }
    memset(&cam, 0, sizeof cam);
    cam.owner = owner;
    cam.generation = next_generation++;
    snprintf(cam.name, sizeof cam.name, "%s", name);
    cam.root = word(FIELD_ROOT);
    cam.active = 1;
    p4_mod_camera_override = 1;
    want_safe_point();
    return cam.generation;
}

static int lease_ok(size_t owner, uint32_t generation)
{
    return cam.active && cam.owner == owner && cam.generation == generation;
}

int p4_mod_camera_set(size_t owner, uint32_t generation, const float pose[6])
{
    if (!lease_ok(owner, generation)) return -1;
    for (int i = 0; i < 6; ++i)
        if (!isfinite(pose[i])) return -1;
    if (fabsf(pose[0]) > 1e6f || fabsf(pose[1]) > 1e6f || fabsf(pose[2]) > 1e6f ||
        pose[4] < -89.0f || pose[4] > 89.0f || pose[5] < 10.0f || pose[5] > 120.0f) return -1;
    memcpy(cam.pose, pose, sizeof cam.pose);
    cam.pose_set = 1;
    return 0;
}

int p4_mod_camera_get(size_t owner, uint32_t generation, float pose[6])
{
    if (!lease_ok(owner, generation) || !cam.pose_set) return -1;
    memcpy(pose, cam.pose, sizeof cam.pose);
    return 0;
}

int p4_mod_camera_active(size_t owner, uint32_t generation, const char **why)
{
    if (lease_ok(owner, generation)) { *why = NULL; return 1; }
    *why = cam.owner == owner && cam.generation == generation && cam.why ? cam.why : "released";
    return 0;
}

void p4_mod_camera_release(size_t owner, uint32_t generation)
{
    if (lease_ok(owner, generation)) camera_end_lease("released by its owner");
}

const char *p4_mod_camera_owner(void) { return cam.active ? cam.name : NULL; }

int p4_mod_camera_input(size_t owner, uint32_t generation, int exclusive)
{
    if (!lease_ok(owner, generation)) return -1;
    cam.input = exclusive != 0;
    if (cam.input) p4_mod_input_fence = 1;
    else if (p4_mod_input_fence == 1) p4_mod_input_fence = 2;
    return 0;
}

static float f32_at(const uint8_t *p) { float v; memcpy(&v, p, 4); return v; }
static void f32_put(uint8_t *p, float v) { memcpy(p, &v, 4); }

/* RwCamera: +4 frame, +20 projection, +96 frameBuffer, +104 viewWindow,
 * +112 recipViewWindow. RwFrame: +16 modelling matrix (rows right, up, at,
 * pos; 16 bytes each). See src/render/p4gl_camera.c. */
static void derive_pose(const uint8_t *model, const uint8_t *camera)
{
    float at[3] = {f32_at(model + 32), f32_at(model + 36), f32_at(model + 40)};
    float up[3] = {f32_at(model + 16), f32_at(model + 20), f32_at(model + 24)};
    float right[3] = {f32_at(model), f32_at(model + 4), f32_at(model + 8)};
    float cross[3] = {up[1] * at[2] - up[2] * at[1], up[2] * at[0] - up[0] * at[2], up[0] * at[1] - up[1] * at[0]};
    cam.sign = cross[0] * right[0] + cross[1] * right[1] + cross[2] * right[2] >= 0.0f ? 1 : -1;
    cam.up = up[1] >= 0.0f ? 1.0f : -1.0f;
    cam.pose[0] = f32_at(model + 48);
    cam.pose[1] = f32_at(model + 52);
    cam.pose[2] = f32_at(model + 56);
    float pitch = at[1] * cam.up;
    if (pitch > 1.0f) pitch = 1.0f;
    if (pitch < -1.0f) pitch = -1.0f;
    cam.pose[3] = atan2f(at[0], at[2]) * (180.0f / 3.14159265f);
    cam.pose[4] = asinf(pitch) * (180.0f / 3.14159265f);
    if (cam.pose[4] > 89.0f) cam.pose[4] = 89.0f;
    if (cam.pose[4] < -89.0f) cam.pose[4] = -89.0f;
    cam.pose[5] = 2.0f * atanf(f32_at(camera + 108)) * (180.0f / 3.14159265f);
    if (cam.pose[5] < 10.0f) cam.pose[5] = 10.0f;
    if (cam.pose[5] > 120.0f) cam.pose[5] = 120.0f;
    cam.pose_set = 1;
}

static void build_model(uint8_t *model)
{
    const float radians = 3.14159265f / 180.0f;
    float yaw = cam.pose[3] * radians, pitch = cam.pose[4] * radians;
    float at[3] = {cosf(pitch) * sinf(yaw), sinf(pitch) * cam.up, cosf(pitch) * cosf(yaw)};
    float world[3] = {0.0f, cam.up, 0.0f};
    float d = world[1] * at[1];
    float up[3] = {-at[0] * d, world[1] - at[1] * d, -at[2] * d};
    float n = sqrtf(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);
    for (int i = 0; i < 3; ++i) up[i] /= n;
    float right[3] = {up[1] * at[2] - up[2] * at[1], up[2] * at[0] - up[0] * at[2], up[0] * at[1] - up[1] * at[0]};
    for (int i = 0; i < 3; ++i) {
        f32_put(model + i * 4, right[i] * (float)cam.sign);
        f32_put(model + 16 + i * 4, up[i]);
        f32_put(model + 32 + i * 4, at[i]);
        f32_put(model + 48 + i * 4, cam.pose[i]);
    }
}

extern void *func_003e9680(void *frame); /* RwFrameUpdateObjects: mark the frame dirty */

void p4_mod_camera_view(void *camera, int begin)
{
    uint8_t *c = camera;
    uint32_t address = P4_H2G(camera);
    if (begin) {
        uint32_t projection, buffer, frame;
        if (!cam.active || cam.applied) return;
        memcpy(&projection, c + 20, 4);
        memcpy(&buffer, c + 96, 4);
        memcpy(&frame, c + 4, 4);
        if (projection != 1 || !frame || !range(frame, 0x50)) return;
        if (buffer && range(buffer, 0x40) && (P4_G8(buffer + 0x20) & 7u) == 5u) return; /* RwRaster.cType: camera texture */
        if (!cam.camera) cam.camera = address;
        if (address != cam.camera) return;
        uint8_t *model = P4_G2H(frame + 16);
        if (!cam.pose_set) derive_pose(model, c);
        memcpy(cam.saved_model, model, 64);
        memcpy(cam.saved_window, c + 104, 16);
        build_model(model);
        float scale = tanf(cam.pose[5] * (3.14159265f / 360.0f)) / f32_at(c + 108);
        if (isfinite(scale) && scale > 0.0f) {
            float x = f32_at(c + 104) * scale, y = f32_at(c + 108) * scale;
            f32_put(c + 104, x);
            f32_put(c + 108, y);
            f32_put(c + 112, 1.0f / x);
            f32_put(c + 116, 1.0f / y);
        }
        func_003e9680(P4_G2H(frame));
        cam.applied = 1;
        return;
    }
    if (!cam.applied || address != cam.camera) return;
    uint32_t frame;
    memcpy(&frame, c + 4, 4);
    memcpy(P4_G2H(frame + 16), cam.saved_model, 64);
    memcpy(c + 104, cam.saved_window, 16);
    func_003e9680(P4_G2H(frame));
    cam.applied = 0;
    if (!cam.active) p4_mod_camera_override = 0;
}

void p4_mod_input_fence_apply(uint8_t packet[32])
{
    if (p4_mod_input_fence == 2 && packet[2] == 0xff && packet[3] == 0xff) {
        p4_mod_input_fence = 0; /* every button released: the game sees no stale edge */
        return;
    }
    packet[2] = packet[3] = 0xff;
    packet[4] = packet[5] = packet[6] = packet[7] = 0x80;
    memset(packet + 8, 0, 12);
}

/* ---- HUD */

int p4_mod_hud_group_valid(const char *group) { return group_index(group) >= 0; }

int p4_mod_hud_hidden(const char *group)
{
    int i = group_index(group);
    return i >= 0 && hide_count[i] != 0;
}

int p4_mod_hud_hide(size_t owner, const char *group, int hidden, const char **reason)
{
    int i = group_index(group);
    if (i < 0) { *reason = "unknown HUD group"; return -1; }
    unsigned j = 0;
    while (j < hide_count[i] && hide_owner[i][j] != owner) ++j;
    if (!hidden) {
        if (j < hide_count[i]) hide_owner[i][j] = hide_owner[i][--hide_count[i]];
        publish_hidden();
        return 0;
    }
    if (j < hide_count[i]) return 0;
    if (hide_count[i] == HIDE_OWNERS) { *reason = "too many mods hide this group"; return -1; }
    P4ModSceneState s;
    scene_state(&s, 1);
    if (s.context != P4_MOD_SCENE_FIELD && s.context != P4_MOD_SCENE_DUNGEON && s.context != P4_MOD_SCENE_BATTLE) {
        *reason = "HUD groups hide in fields, floors and battles";
        return -1;
    }
    if (!any_hidden()) {
        int pending = 0;
        hud_scene_root = word(FIELD_ROOT);
        hud_scene_battle = word(BATTLE_CONTEXT);
        hud_scene_sequence = sequence_state(&pending);
    }
    hide_owner[i][hide_count[i]++] = owner;
    publish_hidden();
    want_safe_point();
    return 0;
}

void p4_mod_hud_status_changed(void)
{
    hud_rows = p4_mod_status_count(P4_MOD_STATUS_HUD) != 0;
    draw_seam_update();
}

/* ---- lifetime */

void p4_mod_services_prepare(void) { prepare(); }

void p4_mod_services_release_owner(size_t owner)
{
    if (cam.active && cam.owner == owner) camera_end_lease("the mod shut down");
    release_hides(owner, 0);
    if (request.owner == owner && request.state == REQ_QUEUED) refuse("the mod shut down");
}
