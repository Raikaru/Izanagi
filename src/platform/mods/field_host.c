/* The port's field host: include/p4pc/mod_field_host.h. Always compiled with
 * the game (like authorize.c): overrides/001747d0.c calls it in every build;
 * only game_party.c (the Luau services) ever arms it. */
#include "p4pc/mod_field_host.h"
#include "p4pc/gmem.h"
#include "p4pc/settings_native.h"
#include <string.h>

extern void fclMiscStopAndClose(void);
extern void fclMiscSetDoneFlag(void);
extern void FldFrameObjSetActive(uint8_t *task, int32_t active);
extern void FldSlotBroadcastState(int32_t state);
extern uint8_t *H_Cdvd_AcquireDirectory(uint8_t *path, int32_t flags);
extern uint32_t H_Cdvd_IsFileLoaded(void *cdvd);
extern uint8_t *H_Cdvd_GetArchiveEntry(uint8_t *request, int32_t entryIndex, int32_t *size);
extern uint32_t H_Cdvd_Destroy(void *cdvd);
extern uint8_t *sdkSpriteRequestLoad(const char *name);
extern void sdkSpriteRelease(uint8_t *node);
extern uint8_t *shdPersonaTaskCreate(int32_t parentTask, int32_t priority, int32_t state, int32_t openDuration,
                                     int32_t closeDuration);
extern void shdPersonaSetOrigin(uint8_t *task, float *position);
extern void shdPersonaStartSlideIn(uint8_t *task);
extern void shdPersonaSetPersonaId(uint8_t *task, int32_t personaId);
extern void shdPersonaSetDisplayState(uint8_t *task, int32_t category, uint32_t persona, int8_t cursor);
extern void shdPersonaSetPendingSkill(uint8_t *task, int32_t skill);
extern int32_t shdPersonaIsTransitioning(uint8_t *task);
extern void shdPersonaFinishPhase(uint8_t *task);
extern uint8_t *shdPersonaSkillReplaceCreate(int32_t parentTask, uint8_t *personaTask, int32_t messageResource,
                                             uint8_t *messages);
extern int32_t datBlockStageIsEight(uint8_t *task);
extern uint8_t kwlnTaskDestroyWithHierarchy(void *task);
extern void *kwlnTaskGetTaskByName(const char *name);
extern int32_t kwlnTaskIsLinked(void *target);
extern int32_t nmMesWindowResetCurrent(void);
extern void *datPersonaGetRecord(uint16_t partyId);
extern int32_t datPersonaCountSkills(void *persona);
extern int32_t datPersonaFindSkillSlot(void *persona, uint16_t skillId);

#define LIVE_SCRIPTS 0x00764604u      /* s32 scrGetWorkCount: live field/event scripts (gp-0x4AEC) */
#define WINDOW_CURRENT 0x00882F60u    /* s32 nmMesWindowCurrent handle: -1 when free */
#define CAMP_WORK P4_GPADDR(0xb1ec)   /* cmpMain work: 0 while Camp is closed */
#define FIELD_FRAME 0x007D2510u       /* FldFrame (MT_Scene_GetFrameState); +0 the field root task */
#define FRAME_SUPPORTER 0x1Cu         /* FldFrame.supporterTask (FldFieldScriptTriggerRun's target) */
#define SEQUENCE_TASK P4_GPADDR(0xb1a8) /* sequence manager task: work +0 bit 0 = a sequence change pending */
#define TASK_WORK 0x38u               /* KwlnTask.work */
#define ROOT_RUNNING 13               /* the field root's work state while the field runs (game_services.c) */
#define PANEL_NAME 0x005E4810u        /* D_005E4810 "shdPersonaStatusDraw": the Persona panel task name */
#define BR_MSG_PATH P4_GPADDR(0xa5b0) /* word: "battle/result/br_msg.bin", btlResultSimple's PAK */
#define ACQUISITION_SPRITE 0x005E4E00u /* state-3 panel's facility/bonus_exp01.spr */
#define PANEL_BACKDROP 0x2C8u        /* shdPersonaSetState's acquisition sprite */
#define RECORD_SKILLS 0x0Cu           /* DatPersonaRecord.skills[8] */
#define SKILLS 8u
#define PANEL_PRIORITY 15             /* shdPersonaTaskCreate(0, 15, 4, 5, 0): the result screen's panel */
#define PANEL_STATE 4
#define PANEL_OPEN 5
#define PANEL_CLOSE 0
#define PANEL_QUEUE 174               /* shdPersonaSetPersonaId's draw-queue slot, as the result screen sets */
#define LOAD_FRAMES 600u              /* the PAK is a few KB: ten seconds means it cannot load */

/* The replace task reads ids[0] (Yes/No), [4] (intro), [5] ("forget X?"), [6] (not learned) and
 * [7] (learned, X forgotten): the result screen's tables for the protagonist's Persona
 * (btlResultOverlayTick 0x002232A0) and for a party member's (btlResultFriendPsUpdate 0x002239A0). */
static const int32_t hero_ids[11] = {12, 0, 6, 9, 10, 11, 13, 14, 19, 20, 21};
static const int32_t member_ids[11] = {27, 25, 6, 9, 10, 26, 13, 14, -1, -1, -1};

/* IDLE -offer-> ARMED -enter-> SUSPEND -run-> OPEN -> CHOOSE -> ENDED -result-> IDLE (run may end any state). */
enum { HOST_IDLE, HOST_ARMED, HOST_SUSPEND, HOST_OPEN, HOST_CHOOSE, HOST_CLOSING, HOST_ENDED };
static struct {
    int state;
    uint32_t member, skill;
    uint32_t task, record, panel, pak, replace, sprite; /* guest addresses */
    uint32_t waited;
    int detached;
    uint16_t before[SKILLS];
    int learned;
    uint16_t forgot;
    const char *reason;
} host;

static void field_hold(int held)
{
    const uint32_t supporter = P4_G32(FIELD_FRAME + FRAME_SUPPORTER);
    if (held) {
        fclMiscStopAndClose();
        fclMiscSetDoneFlag();
    }
    FldFrameObjSetActive(supporter ? P4_GPTR(uint8_t, supporter) : NULL, held);
    FldSlotBroadcastState(held);
}

/* Acquisition revalidates the transitions an offer made at the last frame boundary cannot see:
 * a sequence change requested since (by the game or a service) and a field root that stopped
 * running. A task holds its work at +0x38; the sequence manager's bit 0 and the root's state
 * are the ones the scene services read. */
static int field_settled(void)
{
    const uint32_t sequence = P4_G32(SEQUENCE_TASK);
    const uint32_t sequence_work = sequence ? P4_G32(sequence + TASK_WORK) : 0;
    const uint32_t root = P4_G32(FIELD_FRAME);
    const uint32_t root_work = root ? P4_G32(root + TASK_WORK) : 0;
    return sequence_work && !(P4_G32(sequence_work) & 1u) && root_work && (int32_t)P4_G32(root_work) == ROOT_RUNNING;
}

static void session_end(int learned, uint16_t forgot, const char *reason)
{
    host.state = HOST_ENDED;
    host.learned = learned;
    host.forgot = forgot;
    host.reason = learned ? NULL : reason;
}

/* The outcome from the record: the replace task writes it itself and reports nothing. Learned
 * puts the offered skill in the forgotten skill's slot (datPersonaReplaceSkill). */
static void end_from_record(const char *unlearned)
{
    uint8_t *record = P4_GPTR(uint8_t, host.record);
    if (datPersonaFindSkillSlot(record, (uint16_t)host.skill) == -1) {
        session_end(0, 0, unlearned);
        return;
    }
    uint16_t forgot = 0;
    for (uint32_t i = 0; i < SKILLS; ++i) {
        uint16_t now;
        memcpy(&now, record + RECORD_SKILLS + 2 * i, sizeof now);
        if (now == host.skill && host.before[i] != host.skill) forgot = host.before[i];
    }
    session_end(1, forgot, NULL);
}

int p4_mod_field_host_offer(uint32_t member, uint32_t skill)
{
    if (host.state != HOST_IDLE && host.state != HOST_ARMED) return -1;
    host.state = HOST_ARMED;
    host.member = member;
    host.skill = skill;
    return 0;
}

void p4_mod_field_host_withdraw(void)
{
    if (host.state == HOST_ARMED) host.state = HOST_IDLE;
}

int p4_mod_field_host_busy(void)
{
    return host.state == HOST_SUSPEND || host.state == HOST_OPEN || host.state == HOST_CHOOSE ||
           host.state == HOST_CLOSING;
}

uint32_t p4_mod_field_host_member(void)
{
    return p4_mod_field_host_busy() ? host.member : 0;
}

int p4_mod_field_host_enter(uint32_t task)
{
    if (host.state != HOST_ARMED) return 0;
    /* The predicates every retail opener of this panel and window relies on: no script
     * holds the field, the nm current window is free (the replace task opens it without
     * checking), no other panel lives (Camp's and fusion's use the same task name), and
     * neither Camp nor the native Config is up; and the field is still settled. */
    if ((int32_t)P4_G32(LIVE_SCRIPTS) > 0 || (int32_t)P4_G32(WINDOW_CURRENT) >= 0 || P4_G32(CAMP_WORK) != 0 ||
        p4_settings_native_active() || !field_settled() ||
        kwlnTaskGetTaskByName(P4_GARRAY(char, PANEL_NAME)) != NULL)
        return 0;
    host.state = HOST_SUSPEND;
    host.task = task;
    host.record = host.panel = host.pak = host.replace = host.sprite = 0;
    host.waited = 0;
    host.detached = 0;
    return 1;
}

/* The first frame of a session: hold the field as FldFieldScriptTriggerRun does, then open the
 * panel and start loading the message PAK, or end at once when no choice is needed any more. */
static void open_panel(void)
{
    field_hold(1);
    uint8_t *record = datPersonaGetRecord((uint16_t)host.member);
    if (!record) {
        session_end(0, 0, "that member has no Persona");
        return;
    }
    host.record = P4_H2G(record);
    if (datPersonaFindSkillSlot(record, (uint16_t)host.skill) != -1) {
        session_end(0, 0, "the Persona already knows that skill");
        return;
    }
    if (datPersonaCountSkills(record) < (int32_t)SKILLS) {
        session_end(0, 0, "the Persona's skills changed before the choice opened");
        return;
    }
    memcpy(host.before, record + RECORD_SKILLS, sizeof host.before);
    host.pak = P4_H2G(H_Cdvd_AcquireDirectory(P4_GPTR(uint8_t, P4_G32(BR_MSG_PATH)), 1));
    if (!host.pak) {
        session_end(0, 0, "the game's skill-choice messages could not be requested");
        return;
    }
    host.sprite = P4_H2G(sdkSpriteRequestLoad(P4_GPTR(char, ACQUISITION_SPRITE)));
    if (!host.sprite) {
        H_Cdvd_Destroy(P4_GPTR(void, host.pak));
        host.pak = 0;
        session_end(0, 0, "the game's skill-choice sprite could not be requested");
        return;
    }
    uint8_t *panel = shdPersonaTaskCreate((int32_t)host.task, PANEL_PRIORITY, PANEL_STATE, PANEL_OPEN, PANEL_CLOSE);
    float origin[2] = {0.0f, 19.0f};
    shdPersonaSetOrigin(panel, origin);
    shdPersonaStartSlideIn(panel);
    shdPersonaSetPersonaId(panel, PANEL_QUEUE);
    shdPersonaSetDisplayState(panel, (int32_t)host.member, host.record, 0);
    host.panel = P4_H2G(panel);
    host.state = HOST_OPEN;
}

static void close_panel(void)
{
    if (host.pak) H_Cdvd_Destroy(P4_GPTR(void, host.pak));
    host.pak = 0;
    if (host.panel) shdPersonaFinishPhase(P4_GPTR(uint8_t, host.panel));
    host.state = HOST_CLOSING;
}

int p4_mod_field_host_run(uint32_t task)
{
    (void)task; /* the field has one controller: the session's own (host.task) */
    if (!p4_mod_field_host_busy()) return 0;
    switch (host.state) {
    case HOST_SUSPEND:
        open_panel();
        if (host.state == HOST_ENDED) break;
        return 1;
    case HOST_OPEN: {
        uint8_t *panel = P4_GPTR(uint8_t, host.panel);
        uint8_t *pak = P4_GPTR(uint8_t, host.pak);
        if (!H_Cdvd_IsFileLoaded(pak) || P4_G16(host.sprite) != 5) {
            if (++host.waited < LOAD_FRAMES) return 1;
            session_end(0, 0, "the game's skill-choice resources did not load");
            close_panel();
            return 1;
        }
        if (shdPersonaIsTransitioning(panel) || (int32_t)P4_G32(WINDOW_CURRENT) >= 0) return 1;
        /* State 3 initializes this same work field from the sprite registry.
         * The result-style panel uses state 4, so give it our own loaded node.
         * It remains owned until the panel has actually left the task tree. */
        P4_G32(P4_G32(host.panel + TASK_WORK) + PANEL_BACKDROP) = host.sprite;
        shdPersonaSetPendingSkill(panel, (int32_t)host.skill);
        int32_t ids[11];
        memcpy(ids, host.member == 1 ? hero_ids : member_ids, sizeof ids);
        const uint32_t table = P4_H2G(H_Cdvd_GetArchiveEntry(pak, 0, NULL)); /* sysmsg0_br.bmd */
        host.replace = P4_H2G(shdPersonaSkillReplaceCreate((int32_t)host.task, panel, (int32_t)table, (uint8_t *)ids));
        host.state = HOST_CHOOSE;
        return 1;
    }
    case HOST_CHOOSE:
        if (!datBlockStageIsEight(P4_GPTR(uint8_t, host.replace))) return 1;
        kwlnTaskDestroyWithHierarchy(P4_GPTR(void, host.replace)); /* state 7 already reset the window */
        host.replace = 0;
        end_from_record("declined");
        close_panel();
        return 1;
    case HOST_CLOSING:
        if (host.detached) return 1; /* result polling owns teardown after abandonment */
        if (host.panel && kwlnTaskIsLinked(P4_GPTR(void, host.panel))) return 1;
        host.panel = 0;
        if (host.sprite) sdkSpriteRelease(P4_GPTR(uint8_t, host.sprite));
        host.sprite = 0;
        host.state = HOST_ENDED;
        break;
    }
    field_hold(0);
    return 0;
}

int p4_mod_field_host_result(int abandon, int *learned, uint16_t *forgot, const char **reason)
{
    if (p4_mod_field_host_busy() && !host.detached &&
        (abandon || !kwlnTaskIsLinked(P4_GPTR(void, host.task)))) {
        /* Children can still be on the dying list after the controller disappears.
         * Request native panel teardown, but retain the sprite until it unlinks. */
        if (host.replace && (int32_t)P4_G32(WINDOW_CURRENT) >= 0) nmMesWindowResetCurrent();
        if (host.replace && kwlnTaskIsLinked(P4_GPTR(void, host.replace)))
            kwlnTaskDestroyWithHierarchy(P4_GPTR(void, host.replace));
        if (host.panel && kwlnTaskIsLinked(P4_GPTR(void, host.panel)))
            kwlnTaskDestroyWithHierarchy(P4_GPTR(void, host.panel));
        if (abandon || !host.record) session_end(0, 0, abandon ? "no game is loaded any more" : "the field closed first");
        else end_from_record("the field closed first");
        host.detached = 1;
        host.state = HOST_CLOSING;
    }
    if (host.detached && host.state == HOST_CLOSING) {
        if (host.replace && kwlnTaskIsLinked(P4_GPTR(void, host.replace))) return 0;
        if (host.panel && kwlnTaskIsLinked(P4_GPTR(void, host.panel))) return 0;
        host.panel = 0;
        host.replace = 0;
        if (host.pak) H_Cdvd_Destroy(P4_GPTR(void, host.pak));
        host.pak = 0;
        if (host.sprite) sdkSpriteRelease(P4_GPTR(uint8_t, host.sprite));
        host.sprite = 0;
        host.state = HOST_ENDED;
    }
    if (host.state != HOST_ENDED) return 0;
    *learned = host.learned;
    *forgot = host.forgot;
    *reason = host.reason;
    host.state = HOST_IDLE;
    return 1;
}
