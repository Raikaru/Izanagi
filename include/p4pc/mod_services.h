/* Game services behind p4.clock, p4.scene, p4.encounter, p4.camera and
 * p4.hud (API 1.4). Scalar state only. Requests are validated when made and
 * carried out by the game thread at the scheduler frame boundary
 * (P4_MOD_SAFE_SERVICES); nothing here runs, hooks or allocates until a mod
 * first calls one of these services. */
#ifndef P4PC_MOD_SERVICES_H
#define P4PC_MOD_SERVICES_H
#include <stddef.h>
#include <stdint.h>

/* Scene contexts reported by p4_mod_scene_state. */
enum P4ModSceneContext {
    P4_MOD_SCENE_NONE = 0,   /* no sequence (boot, between scenes) */
    P4_MOD_SCENE_TITLE,      /* title/boot sequences */
    P4_MOD_SCENE_FIELD,      /* fixed field (sequence 9) */
    P4_MOD_SCENE_DUNGEON,    /* generated dungeon floor (sequence 0xB) */
    P4_MOD_SCENE_BATTLE,     /* battle sequence or a field battle task */
    P4_MOD_SCENE_EVENT,      /* event sequence (0xA) */
    P4_MOD_SCENE_OTHER       /* any other sequence */
};
typedef struct P4ModSceneState {
    int context, sequence;
    int ready;          /* idle: ready field/floor, player in control, no event, menu, battle, fade or request */
    int control;        /* the player-control task ("rotate pc") exists */
    int event;          /* a message window or event holds the player's input */
    int menu;           /* the Camp root or native Config is open */
    int pending;        /* a sequence change or a service request is in flight */
    uint32_t field, room, entry, step;   /* step: dungeon step index, 0xffffffff when not a floor */
    const char *reason; /* why ready is 0; static text */
} P4ModSceneState;

void p4_mod_scene_state(P4ModSceneState *out);

/* Requests. Each returns 0 when queued, or -1 with reason (static text) when
 * refused before anything changed. Only one request is in flight at a time. */
int p4_mod_scene_warp(size_t owner, uint32_t field, uint32_t room, uint32_t entry, uint32_t direction, const char **reason);
/* 1 when the disc has field/pack/fFFF_RRR.p00 for this field and room. */
int p4_mod_scene_available(uint32_t field, uint32_t room);
uint32_t p4_mod_encounter_count(void);
/* Enemy IDs of one encounter record (five slots, zero is empty) and its formation. */
int p4_mod_encounter_info(uint32_t id, uint16_t enemies[5], uint16_t *field, uint16_t *variant);
int p4_mod_encounter_launch(size_t owner, uint32_t id, const char **reason);
/* Ends the current time slot the way an activity does: the calendar sequence
 * applies the next slot of the same day and its scheduler procedure runs.
 * Refused across midnight. Status "done" with detail "passed" once the game
 * is back in a ready field at that slot. */
int p4_mod_calendar_pass(size_t owner, const char **reason);

/* Status of the owner's last request: "idle", "requested", "battle",
 * "returning", "done" or "refused". detail is static text (refusal reason or
 * battle outcome name) and outcome the retail battle outcome (0..3, -1 none). */
const char *p4_mod_services_status(size_t owner, const char **detail, int *outcome);

/* Clock. updates counts completed kwlnRunFrame updates (not its reset
 * branch) since the first call; -1 when that seam is unavailable. */
int p4_mod_clock_updates(uint32_t *updates);
uint64_t p4_mod_clock_wall_ns(void);

/* Camera lease: render-only view override at the RenderWare CameraBeginUpdate
 * seam, restored at EndUpdate. acquire returns a nonzero generation or 0 with
 * reason; owner_name receives the current owner's lease name on refusal. */
uint32_t p4_mod_camera_acquire(size_t owner, const char *name, const char **reason, const char **owner_name);
int p4_mod_camera_set(size_t owner, uint32_t generation, const float pose[6]);
int p4_mod_camera_get(size_t owner, uint32_t generation, float pose[6]);
/* Lease state: 1 active, 0 released (by its owner or the scene). */
int p4_mod_camera_active(size_t owner, uint32_t generation, const char **why);
void p4_mod_camera_release(size_t owner, uint32_t generation);
const char *p4_mod_camera_owner(void);
/* Exclusive input: the game receives a neutral pad while set; after it is
 * cleared or the lease ends, neutral continues until every button is up. */
int p4_mod_camera_input(size_t owner, uint32_t generation, int exclusive);
/* Called by the renderer's camera seam (src/render/p4gl_camera.c) while
 * p4_mod_camera_override is set; begin before frame sync, end after drawing. */
extern int p4_mod_camera_override;
void p4_mod_camera_view(void *camera, int begin);

/* Render-only HUD groups: their ordering-table nodes still run at the flush;
 * the renderer drops their GL draws (p4gl_hud_hidden_set). */
int p4_mod_hud_group_valid(const char *group);
int p4_mod_hud_hide(size_t owner, const char *group, int hidden, const char **reason);
int p4_mod_hud_hidden(const char *group);
/* Status rows with the "hud" placement changed (lua_ui.c). */
void p4_mod_hud_status_changed(void);

/* Practice speed (include/p4pc/pace.h): one owner; quarters 1, 2, 4, 8 or 16;
 * 4 releases. The requested rate runs only in gameplay (held reason set
 * otherwise). Refused on a virtual clock. Changes wall time only. */
int p4_mod_pace_set(size_t owner, unsigned quarters, const char **reason);
void p4_mod_pace_state(unsigned *requested, unsigned *effective, const char **held);

/* Input fence for exclusive camera input (src/platform/pad/ps2_pad.c). */
extern int p4_mod_input_fence;
void p4_mod_input_fence_apply(uint8_t packet[32]);

/* Owner shutdown: releases every lease, hide and queued request. */
void p4_mod_services_release_owner(size_t owner);
/* Creates the native seams disabled; mod VM initialization (no dispatch). */
void p4_mod_services_prepare(void);

/* Native half in the game unit (services_native.c): retail calls only. */
int p4_mod_services_native_field(uint32_t field, uint32_t room, uint32_t entry, uint32_t direction);
int p4_mod_services_native_encounter(uint32_t id);
int p4_mod_services_native_resources_ready(void);
int p4_mod_services_native_battle_running(void);
void p4_mod_services_native_revive_party(void);
/* The current date index and slot, and the slot the next calendar step
 * applies: 1 when the game has staged it, 0 when computed by the step's own
 * rule from the current slot. */
void p4_mod_services_native_calendar_now(int16_t *date, uint8_t *slot);
int p4_mod_services_native_calendar_next(int16_t *date, uint8_t *slot);
/* Stages date/slot through the game's setters when nothing is staged, then
 * requests the calendar sequence (25), as K_Cmd_CALL_TV (00176320) does. */
void p4_mod_services_native_calendar_pass(int16_t date, uint8_t slot);
#endif
