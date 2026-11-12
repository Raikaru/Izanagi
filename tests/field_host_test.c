/* The field host (src/platform/mods/field_host.c) over stub native calls and the guest words it
 * reads: acquisition only on a settled field, checked again at the controller's slot (no live
 * script, the nm window free, no Camp, Config or other Persona panel, no sequence change pending
 * since the offer, the field root running, and still armed: a scene request accepted after the
 * offer withdraws it); the session's hold and release of the field, its panel, PAK and replace
 * task with the result screen's message ids; the outcome read back from the record (forgotten,
 * declined, no longer needed); the member whose Persona the panel shows; and a field closing under
 * the panel. The native panel itself is witnessed in game (tests/mod_actions_game_test.py). */
#include "guest_test.h"
#include "p4pc/mod_field_host.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define LIVE_SCRIPTS 0x00764604u
#define WINDOW_CURRENT 0x00882F60u
#define CAMP_WORK P4_GPADDR(0xb1ec)
#define FIELD_FRAME 0x007D2510u
#define SEQUENCE_TASK P4_GPADDR(0xb1a8)
#define BR_MSG_PATH P4_GPADDR(0xa5b0)
#define TASK_WORK 0x38u

static uint8_t *record, *controller, *supporter, *panel_task, *panel_work, *replace_task, *pak, *sprite, *sequence_work, *root_work;
static int settings, holds, supporter_active = -1, slots = -1, panels, panel_alive, pak_loaded, transitioning;
static int replaces, replace_stage, destroyed, finished, pak_released, window_resets, linked = 1;
static int sprite_requests, sprite_releases, defer_panel_destroy;
static int32_t category = -1, pending_skill, replace_ids[11];
static uint32_t replace_table;

static uint32_t g(const void *p) { return P4_H2G(p); }
static uint16_t *skills(void) { return (uint16_t *)(record + 0x0C); }
static void persona(const uint16_t *known, int count)
{
    memset(record, 0, 0x30);
    *(uint16_t *)(record + 2) = 0x11;
    for (int i = 0; i < count; ++i) skills()[i] = known[i];
}

int p4_settings_native_active(void) { return settings; }
void fclMiscStopAndClose(void) { ++holds; }
void fclMiscSetDoneFlag(void) {}
void FldFrameObjSetActive(uint8_t *task, int32_t active) { assert(task == supporter); supporter_active = active; }
void FldSlotBroadcastState(int32_t state) { slots = state; }
uint8_t *H_Cdvd_AcquireDirectory(uint8_t *path, int32_t flags)
{
    assert(!strcmp((const char *)path, "battle/result/br_msg.bin") && flags == 1);
    return pak;
}
uint32_t H_Cdvd_IsFileLoaded(void *cdvd) { assert(cdvd == pak); return (uint32_t)pak_loaded; }
uint8_t *H_Cdvd_GetArchiveEntry(uint8_t *request, int32_t entry, int32_t *size)
{
    assert(request == pak && entry == 0 && !size);
    return pak + 0x100;
}
uint32_t H_Cdvd_Destroy(void *cdvd) { assert(cdvd == pak); ++pak_released; return 1; }
uint8_t *sdkSpriteRequestLoad(const char *name)
{
    assert(g(name) == 0x005E4E00u);
    ++sprite_requests;
    *(uint16_t *)sprite = 0;
    return sprite;
}
void sdkSpriteRelease(uint8_t *node)
{
    assert(node == sprite && !panel_alive && sprite_requests == sprite_releases + 1);
    ++sprite_releases;
}
uint8_t *shdPersonaTaskCreate(int32_t parent, int32_t priority, int32_t state, int32_t open, int32_t close)
{
    assert((uint32_t)parent == g(controller) && priority == 15 && state == 4 && open == 5 && close == 0);
    ++panels;
    panel_alive = 1;
    pending_skill = 0;
    return panel_task;
}
void shdPersonaSetOrigin(uint8_t *task, float *position) { assert(task == panel_task && position[0] == 0.0f && position[1] == 19.0f); }
void shdPersonaStartSlideIn(uint8_t *task) { assert(task == panel_task); transitioning = 1; }
void shdPersonaSetPersonaId(uint8_t *task, int32_t id) { assert(task == panel_task && id == 174); }
void shdPersonaSetDisplayState(uint8_t *task, int32_t inputCategory, uint32_t persona, int8_t cursor)
{
    assert(task == panel_task && persona == g(record) && !cursor);
    category = inputCategory;
}
void shdPersonaSetPendingSkill(uint8_t *task, int32_t skill)
{
    assert(task == panel_task);
    assert(*(uint16_t *)sprite == 5 && P4_G32(g(panel_work) + 0x2C8) == g(sprite));
    pending_skill = skill;
}
int32_t shdPersonaIsTransitioning(uint8_t *task) { assert(task == panel_task); return transitioning; }
void shdPersonaFinishPhase(uint8_t *task) { assert(task == panel_task); ++finished; }
/* The replace task opens the nm current window in its first step (state 0). */
uint8_t *shdPersonaSkillReplaceCreate(int32_t parent, uint8_t *panel, int32_t table, uint8_t *ids)
{
    assert((uint32_t)parent == g(controller) && panel == panel_task);
    ++replaces;
    replace_table = (uint32_t)table;
    memcpy(replace_ids, ids, sizeof replace_ids);
    replace_stage = 0;
    P4_G32(WINDOW_CURRENT) = 4;
    return replace_task;
}
int32_t datBlockStageIsEight(uint8_t *task) { assert(task == replace_task); return replace_stage == 8; }
uint8_t kwlnTaskDestroyWithHierarchy(void *task)
{
    if (task == panel_task) { if (!defer_panel_destroy) panel_alive = 0; }
    else { assert(task == replace_task); ++destroyed; }
    return 0;
}
void *kwlnTaskGetTaskByName(const char *name) { (void)name; return panel_alive ? panel_task : NULL; }
int32_t kwlnTaskIsLinked(void *target) { return target == controller ? linked : target == panel_task && panel_alive; }
int32_t nmMesWindowResetCurrent(void) { ++window_resets; P4_G32(WINDOW_CURRENT) = 0xFFFFFFFFu; return 0; }
void *datPersonaGetRecord(uint16_t member) { assert(member >= 1 && member <= 8); return record; }
int32_t datPersonaCountSkills(void *persona)
{
    int32_t n = 0;
    for (int i = 0; i < 8; ++i) n += ((uint16_t *)((uint8_t *)persona + 0x0C))[i] != 0;
    return n;
}
int32_t datPersonaFindSkillSlot(void *persona, uint16_t skill)
{
    for (int i = 0; i < 8; ++i) if (((uint16_t *)((uint8_t *)persona + 0x0C))[i] == skill) return i;
    return -1;
}

/* The player's answer: the replace task writes the record (slot -1: the offered skill, kept out),
 * resets the window in state 7 and reaches state 8. */
static void answer(int slot)
{
    if (slot >= 0) skills()[slot] = (uint16_t)pending_skill;
    P4_G32(WINDOW_CURRENT) = 0xFFFFFFFFu;
    replace_stage = 8;
}
/* run() until the panel has opened and the replace task exists. */
static void open_session(uint32_t member, uint32_t skill)
{
    assert(!p4_mod_field_host_offer(member, skill) && p4_mod_field_host_enter(g(controller)));
    assert(p4_mod_field_host_busy() && p4_mod_field_host_member() == member);
    pak_loaded = 0;
    const int before = replaces;
    assert(p4_mod_field_host_run(g(controller)) == 1 && supporter_active == 1 && slots == 1 && category == (int32_t)member);
    pak_loaded = 1;
    assert(p4_mod_field_host_run(g(controller)) == 1 && replaces == before && !pending_skill);
    *(uint16_t *)sprite = 5;
    transitioning = 0;
    assert(p4_mod_field_host_run(g(controller)) == 1 && replaces == before + 1);
    assert(pending_skill == (int32_t)skill);
}

int main(void)
{
    if (p4t_guest_map(0x00763000u, 0x2000) || p4t_guest_map(0x007D2000u, 0x1000) || p4t_guest_map(0x00882000u, 0x1000))
        return 77;
    record = p4t_galloc(0x30, 16);
    controller = p4t_galloc(0x40, 16);
    supporter = p4t_galloc(0x40, 16);
    panel_task = p4t_galloc(0x40, 16);
    panel_work = p4t_galloc(0x574, 16);
    sprite = p4t_galloc(0x240, 16);
    P4_G32(g(panel_task) + TASK_WORK) = g(panel_work);
    replace_task = p4t_galloc(0x40, 16);
    pak = p4t_galloc(0x200, 64);
    uint8_t *sequence = p4t_galloc(0x40, 16), *root = p4t_galloc(0x40, 16);
    sequence_work = p4t_galloc(0x10, 16);
    root_work = p4t_galloc(0x10, 16);
    char *path = p4t_galloc(32, 16);
    assert(record && controller && supporter && panel_task && replace_task && pak && sequence && root && path);
    strcpy(path, "battle/result/br_msg.bin");
    P4_G32(BR_MSG_PATH) = g(path);
    P4_G32(SEQUENCE_TASK) = g(sequence);
    P4_G32(g(sequence) + TASK_WORK) = g(sequence_work);
    P4_G32(FIELD_FRAME) = g(root);
    P4_G32(g(root) + TASK_WORK) = g(root_work);
    P4_G32(g(root_work)) = 13;
    P4_G32(FIELD_FRAME + 0x1C) = g(supporter);
    P4_G32(WINDOW_CURRENT) = 0xFFFFFFFFu;
    const int32_t member_ids[11] = {27, 25, 6, 9, 10, 26, 13, 14, -1, -1, -1};
    const int32_t hero_ids[11] = {12, 0, 6, 9, 10, 11, 13, 14, 19, 20, 21};
    int learned;
    uint16_t forgot;
    const char *reason;

    /* Not armed: the controller's slot stays the retail one. */
    assert(!p4_mod_field_host_enter(g(controller)) && !p4_mod_field_host_run(g(controller)));
    assert(!p4_mod_field_host_busy() && !p4_mod_field_host_member());

    /* Armed at the frame boundary; everything that frame_start callbacks or the frame's tasks can
     * change since is checked again when the controller offers its slot. */
    persona((const uint16_t[]){1, 2, 3, 4, 5, 6, 7, 8}, 8);
    assert(!p4_mod_field_host_offer(2, 30));
    P4_G32(LIVE_SCRIPTS) = 1;
    assert(!p4_mod_field_host_enter(g(controller)));
    P4_G32(LIVE_SCRIPTS) = 0;
    P4_G32(WINDOW_CURRENT) = 3;
    assert(!p4_mod_field_host_enter(g(controller)));
    P4_G32(WINDOW_CURRENT) = 0xFFFFFFFFu;
    P4_G32(CAMP_WORK) = g(sequence_work);
    assert(!p4_mod_field_host_enter(g(controller)));
    P4_G32(CAMP_WORK) = 0;
    settings = 1;
    assert(!p4_mod_field_host_enter(g(controller)));
    settings = 0;
    panel_alive = 1;
    assert(!p4_mod_field_host_enter(g(controller)));
    panel_alive = 0;
    P4_G32(g(sequence_work)) = 1; /* a sequence change requested since the offer (a warp, the game's own) */
    assert(!p4_mod_field_host_enter(g(controller)));
    P4_G32(g(sequence_work)) = 0;
    P4_G32(g(root_work)) = 12; /* the field root stopped running */
    assert(!p4_mod_field_host_enter(g(controller)));
    P4_G32(g(root_work)) = 13;
    p4_mod_field_host_withdraw(); /* a scene request accepted after the offer goes first */
    assert(!p4_mod_field_host_enter(g(controller)) && !p4_mod_field_host_busy());
    assert(!holds && !panels && supporter_active == -1);

    /* A party member forgets skill 4: the field is held and released as FldFieldScriptTriggerRun
     * does, the panel and PAK are the result screen's, the outcome comes from the record. */
    open_session(2, 30);
    assert(holds == 1 && panels == 1 && p4_mod_field_host_offer(2, 31) == -1); /* one session at a time */
    assert(replace_table == g(pak + 0x100) && !memcmp(replace_ids, member_ids, sizeof member_ids));
    assert(p4_mod_field_host_run(g(controller)) == 1); /* the player chooses */
    answer(3);
    assert(p4_mod_field_host_run(g(controller)) == 1 && destroyed == 1 && pak_released == 1 && finished == 1);
    assert(p4_mod_field_host_run(g(controller)) == 1 && sprite_releases == 0);
    panel_alive = 0;
    assert(p4_mod_field_host_run(g(controller)) == 0 && sprite_releases == 1);
    assert(supporter_active == 0 && slots == 0 && !p4_mod_field_host_busy() && !p4_mod_field_host_member());
    assert(p4_mod_field_host_offer(2, 31) == -1); /* the outcome waits to be collected */
    assert(p4_mod_field_host_result(0, &learned, &forgot, &reason) == 1 && learned && forgot == 4 && !reason);
    assert(!p4_mod_field_host_result(0, &learned, &forgot, &reason));

    /* The protagonist's Persona keeps every skill: the hero's message ids; nothing written. */
    open_session(1, 31);
    assert(!memcmp(replace_ids, hero_ids, sizeof hero_ids));
    answer(-1);
    assert(p4_mod_field_host_run(g(controller)) == 1 && pak_released == 2 && finished == 2);
    panel_alive = 0;
    assert(p4_mod_field_host_run(g(controller)) == 0 && sprite_releases == 2);
    assert(p4_mod_field_host_result(0, &learned, &forgot, &reason) == 1 && !learned && !forgot && !strcmp(reason, "declined"));
    assert(datPersonaFindSkillSlot(record, 31) == -1 && skills()[3] == 30);

    /* No choice needed any more when the panel would open: the field is held and released in the
     * same frame and nothing opens. */
    assert(!p4_mod_field_host_offer(2, 30) && p4_mod_field_host_enter(g(controller)));
    assert(p4_mod_field_host_run(g(controller)) == 0 && holds == 3 && slots == 0 && panels == 2);
    assert(p4_mod_field_host_result(0, &learned, &forgot, &reason) == 1 && !learned);
    assert(!strcmp(reason, "the Persona already knows that skill"));

    /* The field closes under the panel: the window the replace task opened is reset before its PAK
     * is released; the panel and the task went with the controller. */
    open_session(2, 32);
    assert((int32_t)P4_G32(WINDOW_CURRENT) == 4);
    assert(!p4_mod_field_host_result(0, &learned, &forgot, &reason)); /* still linked: running */
    linked = 0;
    assert(p4_mod_field_host_result(0, &learned, &forgot, &reason) == 1 && !learned);
    assert(!strcmp(reason, "the field closed first") && window_resets == 1 && pak_released == 3 && sprite_releases == 3);
    assert(!p4_mod_field_host_busy() && !p4_mod_field_host_run(g(controller)));
    linked = 1;
    panel_alive = 0;

    /* No game any more: a running session ends at once. */
    open_session(2, 33);
    defer_panel_destroy = 1;
    assert(!p4_mod_field_host_result(1, &learned, &forgot, &reason));
    assert(sprite_releases == 3 && pak_released == 3);
    panel_alive = 0;
    assert(p4_mod_field_host_result(1, &learned, &forgot, &reason) == 1 && !learned);
    assert(!strcmp(reason, "no game is loaded any more") && window_resets == 2 && pak_released == 4 && sprite_releases == 4);
    puts("field_host: acquisition revalidation, hold and release, panel and PAK, outcomes and abandonment passed");
    return 0;
}
