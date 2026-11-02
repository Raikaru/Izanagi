/* Menu harness: P4PC_START_MENU=<field>[:<room>] boots the engine as the real boot does and, once the boot chain has reached
 * gameSequence (sequence 27, as for the battle harness in src/platform/battle_harness.c), installs the new-game state and starts
 * the game's field sequence (9, FIELD RECORD: K_SeqFieldRecordStart -> FldRootCreate) on that field, the same entry the viewer's
 * `field:F:R` mode uses. The field's own camp trigger (FldCampTriggerTest, retail 0x00170B50: a Triangle edge, 0x10 in the game's
 * pad word) then opens the camp menu (cmpMainTaskCreate, 0x0012D000) under the field frame task, and AUTOPRESS drives it:
 *
 *   P4PC_START_MENU=7:2 P4PC_AUTOPRESS_FRAMES=1 P4PC_AUTOPRESS='triangle@900:6,down@1000:6,cross@1040:6'
 *
 * P4PC_MENU_SCRIPT drives the menus by what the game shows rather than by flip numbers (a field's load time varies, an AUTOPRESS press
 * before it is ready is lost): a comma list of `open` (Triangle every 60 flips until the camp task has left state 0), button names
 * (cross, circle, triangle, square, up, down, left, right, l1, r1, start, select: held 6 flips, then P4PC_MENU_STEP flips of settling,
 * default 45), `waitN` (N flips), `shot:name` (writes menu_<name>.png from the next presented frame) and `state` (logs the camp state words).
 * P4PC_MENU_EXIT=1 exits 0 when the script ends. Example:
 *   P4PC_MENU_SCRIPT='open,shot:top,down,down,cross,wait90,shot:skill,circle'
 *
 * P4PC_START_MENU_AT=<frames> delays the start after sequence 27 begins (default 10). The status line every 60 flips reports the
 * sequence, the field root task and the camp task's state words. P4PC_MENU_GATE=<flip> exits 0 at that flip when the camp task
 * exists and has left state 0, 1 otherwise (the ctest camp_menu). P4PC_START_MENU_SKIP_STATE=1 keeps the loaded game state
 * (datInitDefaultGameState is not run), for a state installed by the save loader. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "p4pc/os.h"
#include "p4pc/save_import.h"
#include "p4pc/gmem.h"
#include "p4pc/settings.h"
#include "p4pc/settings_native.h"
#ifdef P4_MOD_GAME_BINDINGS
#include "p4pc/mod_fidelity.h"
#endif
#include "config/p4cfg.h"
#include "pad/pad_sdl.h"
#include "../render/p4gl_readback.h"
#include "../render/p4gl.h"
#include "../render/p4gl_gl.h"
#include "../../tests/settings_pages_fixture.h"

#define iGpffffb1a8 P4_GPDATA(int32_t, 0xb1a8)       /* the sequence manager's task handle */
#define iGpffffb1ec P4_GPDATA(int32_t, 0xb1ec)       /* the camp task's work (cmpMainTaskCreate), 0x9740 bytes */
#define D_008C024E P4_GARRAY(uint16_t, 0x008C024Eu)     /* the pad edge word the game polls */
#define D_005E57A8 P4_GARRAY(char, 0x005E57A8u)         /* the boot logo task's name (logoDraw.c) */
extern void *kwlnTaskGetTaskByName(const void *name);
extern int32_t kwlnTaskDestroyWithHierarchy(void *task);
extern void datInitDefaultGameState(void);
extern void sequenceRequest(int32_t id, uint32_t data, int32_t size, int32_t delay);   /* decomp: the record's guest address */
extern double p4gl_context_capture_nonblack(void);
extern int p4gl_context_capture_written(void);
extern void p4gl_context_capture_next(const char *path);
extern void p4gl_context_draw_between(unsigned delay, unsigned count);
extern void p4pc_pad_inject(unsigned bits, unsigned frames);
extern uint8_t *datSaveBuildImage(int32_t version, int32_t *outSize, int32_t compact);   /* what the save screen writes (mcSaveState: (0, &size, 0)) */
/* The allocator's free: the guest code address in the retail slot 0x008873EC. */
#define P4_GAME_FREE(p) ((void (*)(void *))p4_fn_g2h_sig(P4_G32(0x008873ECu), P4_SIG(1, 0, 1, 0, 0, 0, 0, 0)))(p)
extern void datSetMoney(uint32_t money);
extern void datSetFlag(int32_t flag, int32_t value);
extern void datSetHeroLevel(int32_t id, int32_t level);
extern void datSetUnitHp(int32_t id, int32_t hp);
extern int32_t mcIconTaskCreate(void *parent, int32_t mode);   /* the memory card screen task (retail 0x002AA300): mode 1 saves, 0 loads */

enum { SEQ_GAME = 27, SEQ_FIELD_RECORD = 9 };

static int32_t sField = -2;   /* -2 not read yet, -1 off */
static int32_t sRoom;
static int sDelay = 10;
static int sSeen27;
static int sDone;

static int32_t currentSequence(void)
{
    uint8_t *task;
    uint8_t *work;

    if (iGpffffb1a8 == 0) {
        return -1;
    }
    task = (uint8_t *)P4_G2H((uint32_t)iGpffffb1a8);
    work = (uint8_t *)P4_G2H(*(uint32_t *)(task + 0x38));
    if (work == NULL) {
        return -1;
    }
    return *(int32_t *)(work + 4);
}

static void startField(void)
{
    static uint16_t *start;   /* the field record the sequence reads by guest address */
    void *logo = kwlnTaskGetTaskByName(D_005E57A8);

    if (start == NULL) {
        start = (uint16_t *)P4_G2H(p4_gport_alloc(16, 16));
    }
    if (logo != NULL) {   /* as in the battle harness: the logo task would fade the screen out and hold black */
        fprintf(stderr, "p4pc: menu harness: destroying the boot logo task %08x\n", (unsigned)P4_H2G(logo));
        kwlnTaskDestroyWithHierarchy(logo);
    }
    if (getenv("P4PC_START_MENU_SKIP_STATE") == NULL) {
        datInitDefaultGameState();
    }
    memset(start, 0, 16);
    start[0] = (uint16_t)sField;
    start[1] = (uint16_t)sRoom;
    fprintf(stderr, "p4pc: menu harness: entering sequence %d with field %d room %d\n", SEQ_FIELD_RECORD, (int)sField, (int)sRoom);
    sequenceRequest(SEQ_FIELD_RECORD, P4_H2G(start), 0, 0);
}

/* ---- the script engine */
#define SCRIPT_MAX 256
static char *sToks[SCRIPT_MAX];
static int sNumToks;
static int sIdx;
static unsigned sWaitUntil;
static int sScriptState;   /* 0 not read, 1 running, 2 done */
static int sCampSeen;      /* the camp task reached state 2 or later */
static int sShots, sShotsDark, sShotsFailed;   /* requested shots, dark readbacks and failed PNG writes */
static int sPendingShot;
static int sCheckUsed, sCheckOk;
static int sSettingsChecks, sSettingsFailed;
static int sSettingsImportBefore = -1;
static int sSettingsCursorBefore = -1;
#define KEEP_MAX 8
static struct { char name[16]; uint8_t *data; int32_t size; } sKeep[KEEP_MAX];
static int32_t sMcTask;   /* the memory card screen task made by `save` / `load` */

static unsigned buttonBit(const char *t)
{
    static const struct { const char *name; unsigned bit; } map[] = {
        {"select", 0x0001}, {"start", 0x0008}, {"up", 0x0010}, {"right", 0x0020}, {"down", 0x0040}, {"left", 0x0080}, {"l2", 0x0100},
        {"r2", 0x0200}, {"l1", 0x0400}, {"r1", 0x0800}, {"triangle", 0x1000}, {"circle", 0x2000}, {"cross", 0x4000}, {"square", 0x8000},
    };
    size_t i;

    for (i = 0; i < sizeof map / sizeof map[0]; i++) {
        if (strcmp(map[i].name, t) == 0) {
            return map[i].bit;
        }
    }
    return 0;
}
static void menuPress(unsigned bits, unsigned frames)
{
    if (getenv("P4PC_SETTINGS_VIRTUAL_TEST")) {
        if (p4pc_pad_virtual_press(bits, frames)) {
            fputs("menu harness: virtual settings pad unavailable\n", stderr);
            exit(1);
        }
    } else p4pc_pad_inject(bits, frames);
}

static void logCamp(unsigned frame, const char *what)
{
    const uint8_t *camp = (const uint8_t *)P4_G2H((uint32_t)iGpffffb1ec);

    fprintf(stderr, "p4pc: menu harness: frame %u %s camp %p", frame, what, (const void *)camp);
    if (camp != NULL) {
        fprintf(stderr, " state %u +4 %u +8 %u +0x28 %u flags %u", (unsigned)*(const uint32_t *)camp, (unsigned)*(const uint32_t *)(camp + 4),
                (unsigned)*(const uint32_t *)(camp + 8), (unsigned)*(const uint32_t *)(camp + 0x28), (unsigned)*(const uint32_t *)(camp + 0x10));
    }
    fprintf(stderr, "\n");
}

/* Settings witnesses use the same queued SDL events as a player, explicitly
 * tagged so script input never reads the user's keyboard or controllers. */
static void settingsKey(const char *name)
{
    SDL_Scancode scan = SDL_GetScancodeFromName(name);
    SDL_Event event;
    if (getenv("P4PC_SETTINGS_TEST") == NULL || scan == SDL_SCANCODE_UNKNOWN) {
        fprintf(stderr, "menu harness: invalid or non-opted-in settings key %s\n", name);
        exit(1);
    }
    SDL_zero(event);
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.scancode = scan;
    event.key.which = P4_SETTINGS_TEST_DEVICE;
    if (!SDL_PushEvent(&event)) exit(1);
    event.type = SDL_EVENT_KEY_UP;
    if (!SDL_PushEvent(&event)) exit(1);
}
static void settingsMouse(const char *coordinates)
{
    float x, y, rect[4];
    int width, height;
    SDL_Event event;
    if (!getenv("P4PC_SETTINGS_TEST") || sscanf(coordinates, "%f:%f", &x, &y) != 2 ||
        !p4gl_context_menu_bounds(rect) ||
        !SDL_GetWindowSize(p4pc_pad_sdl_window(), &width, &height)) exit(1);
    SDL_zero(event);
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.which = P4_SETTINGS_TEST_DEVICE;
    event.motion.windowID = SDL_GetWindowID(p4pc_pad_sdl_window());
    event.motion.x = (rect[0] + x / 640.0f * rect[2]) * width;
    event.motion.y = (rect[1] + y / 448.0f * rect[3]) * height;
    if (!SDL_PushEvent(&event)) exit(1);
}

static void settingsTouch(const char *coordinates)
{
    float x, y, rect[4];
    SDL_Event event;
    if (getenv("P4PC_SETTINGS_TEST") == NULL ||
        sscanf(coordinates, "%f:%f", &x, &y) != 2 ||
        !p4gl_context_menu_bounds(rect)) {
        fprintf(stderr, "menu harness: invalid or unavailable settings touch %s\n", coordinates);
        exit(1);
    }
    SDL_zero(event);
    event.type = SDL_EVENT_FINGER_DOWN;
    event.tfinger.touchID = P4_SETTINGS_TEST_DEVICE;
    event.tfinger.fingerID = 1;
    event.tfinger.windowID = SDL_GetWindowID(p4pc_pad_sdl_window());
    event.tfinger.x = rect[0] + x / 640.0f * rect[2];
    event.tfinger.y = rect[1] + y / 448.0f * rect[3];
    event.tfinger.pressure = 1;
    if (!SDL_PushEvent(&event)) exit(1);
    event.type = SDL_EVENT_FINGER_UP;
    event.tfinger.pressure = 0;
    if (!SDL_PushEvent(&event)) exit(1);
    /* SDL emits this compatibility event for a real finger as well. It must
     * never turn the first selection tap into a second activation. */
    int width, height;
    float normalizedX = event.tfinger.x, normalizedY = event.tfinger.y;
    if (!SDL_GetWindowSize(p4pc_pad_sdl_window(), &width, &height)) exit(1);
    SDL_zero(event);
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.which = SDL_TOUCH_MOUSEID;
    event.button.windowID = SDL_GetWindowID(p4pc_pad_sdl_window());
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = normalizedX * (float)width;
    event.button.y = normalizedY * (float)height;
    if (!SDL_PushEvent(&event)) exit(1);
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    if (!SDL_PushEvent(&event)) exit(1);
}

static int settingsEntry(const char *wanted)
{
    char label[96], value[96];
    for (int i = 0; i < p4_settings_entry_count(); ++i) {
        p4_settings_entry(i, label, sizeof label, value, sizeof value);
        if (!strcmp(label, wanted)) return i;
    }
    return -1;
}
static void settingsTouchRow(const char *wanted)
{
    float x, y;
    char coordinates[64];
    int model = settingsEntry(wanted);
    if (getenv("P4PC_SETTINGS_TEST") == NULL ||
        !p4_settings_native_row_position(!strcmp(wanted, "Controls") ? -2 : model, &x, &y)) {
        fprintf(stderr, "menu harness: row %s not visible for touch\n", wanted);
        exit(1);
    }
    snprintf(coordinates, sizeof coordinates, "%g:%g", (double)x, (double)y);
    settingsTouch(coordinates);
}
static void settingsMouseRow(const char *wanted)
{
    float x, y;
    char coordinates[64];
    int model = settingsEntry(wanted);
    if (!p4_settings_native_row_position(!strcmp(wanted, "Controls") ? -2 : model, &x, &y)) exit(1);
    snprintf(coordinates, sizeof coordinates, "%g:%g", (double)x, (double)y);
    settingsMouse(coordinates);
}
static void settingsMouseClickRow(const char *wanted)
{
    float x, y, rect[4];
    int width, height, model = settingsEntry(wanted);
    SDL_Event event;
    if (!getenv("P4PC_SETTINGS_TEST") ||
        !p4_settings_native_row_position(!strcmp(wanted, "Controls") ? -2 : model, &x, &y) ||
        !p4gl_context_menu_bounds(rect) ||
        !SDL_GetWindowSize(p4pc_pad_sdl_window(), &width, &height)) exit(1);
    x = 480.0f; /* the stock left arrow, not the centre activation target */
    SDL_zero(event);
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.which = P4_SETTINGS_TEST_DEVICE;
    event.button.windowID = SDL_GetWindowID(p4pc_pad_sdl_window());
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = (rect[0] + x / 640.0f * rect[2]) * width;
    event.button.y = (rect[1] + y / 448.0f * rect[3]) * height;
    if (!SDL_PushEvent(&event)) exit(1);
}
static int settingsSwipeBefore = -1;
static void settingsSwipe(const char *distance)
{
    float rect[4], dy = (float)atoi(distance);
    SDL_Event event;
    int view;
    settingsSwipeBefore = p4_settings_native_cursor(&view);
    if (getenv("P4PC_SETTINGS_TEST") == NULL || settingsSwipeBefore < 0 ||
        dy < -128 || dy > 128 || !p4gl_context_menu_bounds(rect)) exit(1);
    SDL_zero(event);
    event.type = SDL_EVENT_FINGER_DOWN;
    event.tfinger.touchID = P4_SETTINGS_TEST_DEVICE;
    event.tfinger.fingerID = 2;
    event.tfinger.windowID = SDL_GetWindowID(p4pc_pad_sdl_window());
    event.tfinger.x = rect[0] + 300.0f / 640.0f * rect[2];
    event.tfinger.y = rect[1] + 200.0f / 448.0f * rect[3];
    event.tfinger.pressure = 1;
    if (!SDL_PushEvent(&event)) exit(1);
    event.type = SDL_EVENT_FINGER_MOTION;
    event.tfinger.y = rect[1] + (200.0f + dy) / 448.0f * rect[3];
    if (!SDL_PushEvent(&event)) exit(1);
    event.type = SDL_EVENT_FINGER_UP;
    event.tfinger.pressure = 0;
    if (!SDL_PushEvent(&event)) exit(1);
}

typedef struct SettingsSaveCount { const char *root; int count; } SettingsSaveCount;
static void settingsCountSave(const char *name, void *userdata)
{
    SettingsSaveCount *count = userdata;
    char path[2048];
    if (strncmp(name, "BASLUS-21782Save", sizeof "BASLUS-21782Save" - 1)) return;
    int n = snprintf(path, sizeof path, "%s/%s", count->root, name);
    if (n > 0 && (size_t)n < sizeof path &&
        p4_os_extended_path(path, sizeof path) == 0 && p4_os_is_dir_nofollow(path)) ++count->count;
}
static int settingsSaveCount(void)
{
    const char *root = getenv("P4PC_SAVE_DIR");
    char card[2048];
    if (!root || !*root) return -1;
    int n = snprintf(card, sizeof card, "%s/mc0", root);
    if (n < 0 || (size_t)n >= sizeof card || p4_os_extended_path(card, sizeof card)) return -1;
    SettingsSaveCount count = {card, 0};
    if (p4_os_list_dir(card, settingsCountSave, &count) && errno != ENOENT) return -1;
    return count.count;
}
static void settingsImportPhase(const char *phase)
{
    if (getenv("P4PC_SETTINGS_TEST") == NULL) {
        fputs("menu harness: import witness requires scripted settings opt-in\n", stderr);
        exit(1);
    }
    if (!strcmp(phase, "cancel") && p4_settings_active()) {
        if (sSettingsImportBefore < 0) sSettingsImportBefore = settingsSaveCount();
        if (sSettingsImportBefore < 0) exit(1);
        const char *cancelled[] = {NULL};
        p4save_import_dialog_result(NULL, cancelled, 0);
    } else if (!strcmp(phase, "complete") && !p4_settings_active()) {
        const char *source = getenv("P4PC_IMPORT_SOURCE");
        if (!source || !*source || sSettingsImportBefore < 0) exit(1);
        const char *files[] = {source, NULL};
        p4save_import_dialog_result(NULL, files, 0);
        /* Exercise the real formatter's closed-menu guard too. */
        int entry = settingsEntry("Import save");
        char label[96], value[96];
        if (entry < 0 || p4_settings_entry(entry, label, sizeof label, value, sizeof value) < 0) exit(1);
    } else if (!strcmp(phase, "folder") && p4_settings_active()) {
#if defined(SDL_PLATFORM_ANDROID)
        p4pc_pad_inject(buttonBit("cross"), 6); /* actual selected native row */
#else
        fputs("menu harness: folder witness is Android-only; refusing a desktop dialog\n", stderr);
        exit(1);
#endif
    } else {
        fprintf(stderr, "menu harness: invalid import phase %s (active %d)\n", phase, p4_settings_active());
        exit(1);
    }
}

/* Resolve registrations by label, then move through the real SDK list with
 * ordinary pad packets. Never force a guest cursor or assume extension order. */
static int settingsNavigateRow(const char *wanted)
{
    int model = settingsEntry(wanted), view = -1;
    int current = p4_settings_native_cursor(&view);
    if (getenv("P4PC_SETTINGS_TEST") == NULL || current < 0) {
        fprintf(stderr, "menu harness: native Config unavailable for row %s\n", wanted);
        exit(1);
    }
    int target = p4_settings_native_find_row(!strcmp(wanted, "Controls") ? -2 : model);
    if (target < 0) {
        fprintf(stderr, "menu harness: row %s not in native view %d\n", wanted, view);
        exit(1);
    }
    if (current == target) return 1;
    menuPress(buttonBit(current < target ? "down" : "up"), 6);
    return 0;
}

static void settingsCheck(const char *name)
{
    int ok = 0;
    if (!strcmp(name, "cursor-visible")) {
        ok = p4_settings_active() && SDL_CursorVisible();
    } else if (!strcmp(name, "cursor-hidden")) {
        ok = p4_settings_active() && !SDL_CursorVisible();
    } else if (!strcmp(name, "cursor-restored")) {
        ok = !p4_settings_active() && !SDL_CursorVisible();
    } else if (!strcmp(name, "window-mode")) {
        const char *expected = getenv("P4PC_SETTINGS_EXPECT_FULLSCREEN");
        SDL_Window *window = p4pc_pad_sdl_window();
        ok = expected && window &&
            !!(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) == (expected[0] == '1');
    } else if (!strcmp(name, "cursor-repeat")) {
        int view, current = p4_settings_native_cursor(&view);
        ok = sSettingsCursorBefore >= 0 && current >= sSettingsCursorBefore + 2;
    } else if (!strncmp(name, "row:", 4)) {
        int view, current = p4_settings_native_cursor(&view);
        int model = settingsEntry(name + 4);
        int target = p4_settings_native_find_row(!strcmp(name + 4, "Controls") ? -2 : model);
        ok = current >= 0 && current == target;
    } else if (!strncmp(name, "active-page:", 12)) {
        const char *active = p4_settings_page_active();
        ok = active && !strcmp(active, name + 12);
    } else if (!strncmp(name, "page-", 5)) {
        ok = settingsPagesFixtureCheck(name);
    } else if (!strcmp(name, "retail")) {
        uint32_t work = p4_settings_native_retail_work();
        ok = work && !p4_settings_active() && (P4_G32(work + 0x24u) & 2u) &&
            P4_G32(work + 0x30u) < 7;
    } else if (!strcmp(name, "platform")) {
        int confirm = p4_settings_native_find_row(-3);
        ok = p4_settings_active() && confirm > p4_settings_native_find_row(-2);
        for (int i = 0; i < p4_settings_entry_count(); ++i)
            if (p4_settings_native_find_row(i) >= confirm) ok = 0;
#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
        const int hidden[] = {0, 1, 2, 5, 9};
        for (unsigned i = 0; i < sizeof hidden / sizeof hidden[0]; ++i)
            if (p4_settings_entry_available(hidden[i])) ok = 0;
        int touch = settingsEntry("Touch scrolling");
        ok = ok && touch >= 0 && p4_settings_entry_available(touch) &&
            p4_settings_entry_available(3) && p4_settings_entry_available(6) &&
            p4_settings_entry_available(41);
#else
        for (int i = 0; i < 9; ++i)
            if (!p4_settings_entry_available(i)) ok = 0;
        ok = ok && p4_settings_entry_available(9) && p4_settings_entry_available(41);
#endif
    } else if (!strcmp(name, "touch-on") || !strcmp(name, "touch-off")) {
        ok = p4_settings_active() &&
            p4cfg_int("input.touch_scroll", 1) == !strcmp(name, "touch-on");
    } else if (!strcmp(name, "swipe-blocked") || !strcmp(name, "swipe-moved")) {
        int view;
        int current = p4_settings_native_cursor(&view);
        ok = settingsSwipeBefore >= 2 && current ==
            settingsSwipeBefore - (!strcmp(name, "swipe-moved") ? 2 : 0);
    } else if (!strcmp(name, "open")) {
        ok = p4_settings_active();
    } else if (!strcmp(name, "draw-skipped")) {
        const char *turbo = getenv("P4PC_TURBO");
        ok = p4_settings_active() && turbo && !strcmp(turbo, "2") &&
            !p4gl_gl_draw_enabled();
    } else if (!strcmp(name, "closed")) {
        ok = !p4_settings_active() && iGpffffb1ec == 0;
    } else if (!strcmp(name, "title")) {
        ok = p4_settings_active() && iGpffffb1ec == 0;
    } else if (!strcmp(name, "persisted")) {
        p4cfg_clear();
        p4cfg_load();
        ok = !p4_settings_active() &&
            !strcmp(p4cfg_str("video.resolution", ""), "scale:2") &&
            !strcmp(p4cfg_str("video.aspect", ""), "4:3") &&
            !strcmp(p4cfg_str("input.kb.cross", ""), "K, none") &&
            !strcmp(p4cfg_str("input.kb.circle", "X"), "X");
    } else if (!strcmp(name, "scale1") || !strcmp(name, "scale2")) {
        uint32_t w = 0, h = 0;
        const unsigned scale = name[5] == '2' ? 2u : 1u;
        p4gl_readback_pixel_size(&w, &h);
        ok = !strcmp(p4cfg_str("video.resolution", ""), scale == 2 ? "scale:2" : "scale:1") &&
            w == 640u * scale && h == 448u * scale;
        fprintf(stderr, "menu harness: native resolution %s, target %ux%u\n",
            p4cfg_str("video.resolution", ""), (unsigned)w, (unsigned)h);
    } else if (!strcmp(name, "aspect169") || !strcmp(name, "aspectauto") ||
        !strcmp(name, "aspect43")) {
        const char *expected = !strcmp(name, "aspect169") ? "16:9"
            : !strcmp(name, "aspectauto") ? "auto" : "4:3";
        int mode = !strcmp(name, "aspect169") ? P4CFG_ASPECT_16_9
            : !strcmp(name, "aspectauto") ? P4CFG_ASPECT_AUTO : P4CFG_ASPECT_4_3;
        int entry = settingsEntry("Aspect ratio");
        char label[96], value[96];
        uint32_t w = 0, h = 0;
        p4gl_readback_pixel_size(&w, &h);
        ok = entry >= 0 && p4_settings_entry(entry, label, sizeof label, value, sizeof value) == 1 &&
            !strcmp(p4cfg_str("video.aspect", ""), expected) && p4cfg_video()->aspect == mode &&
            p4gl_aspect_width() == 1.0f && w == 1280 && h == 896;
    } else if (!strcmp(name, "movie-bars") || !strcmp(name, "movie-zoom") ||
        !strcmp(name, "movie-stretch")) {
        const char *expected = name + 6;
        const int mode = !strcmp(expected, "bars") ? P4CFG_MOVIE_BARS :
            !strcmp(expected, "zoom") ? P4CFG_MOVIE_ZOOM : P4CFG_MOVIE_STRETCH;
        const int entry = settingsEntry("Movie fit");
        char label[96], value[96];
        ok = p4_settings_active() && entry >= 0 &&
            p4_settings_entry(entry, label, sizeof label, value, sizeof value) == 0 &&
            p4_settings_entry_flags(entry) == 0 && p4_settings_entry_available(entry) &&
            p4_settings_native_find_row(entry) >= 0 && !strcmp(value, expected) &&
            !strcmp(p4cfg_str("video.movie_fit", ""), expected) && p4cfg_video()->movieFit == mode;
        /* Prove the live callback first, then independently reload its saved choice. */
        if (ok) {
            p4cfg_clear();
            p4cfg_load();
            ok = !strcmp(p4cfg_str("video.movie_fit", ""), expected) &&
                p4cfg_video()->movieFit == mode;
        }
    } else if (!strcmp(name, "movie-persisted")) {
        p4cfg_clear();
        p4cfg_load();
        ok = !p4_settings_active() &&
            !strcmp(p4cfg_str("video.movie_fit", ""), "stretch") &&
            p4cfg_video()->movieFit == P4CFG_MOVIE_STRETCH;
    } else if (!strcmp(name, "binding")) {
        bool keys[SDL_SCANCODE_COUNT] = {false};
        P4PadSnapshot pad;
        keys[SDL_SCANCODE_K] = true;
        memset(&pad, 0, sizeof pad);
        p4pc_pad_keys_to_snapshot(keys, SDL_SCANCODE_COUNT, &pad);
        ok = !strcmp(p4cfg_str("input.kb.cross", ""), "K, none") &&
            !strcmp(p4cfg_str("input.kb.circle", "X"), "X") &&
            pad.buttons == P4PC_PAD_CROSS;
    } else if (!strncmp(name, "save-count:", 11)) {
        int expected = atoi(name + 11);
        int actual = settingsSaveCount();
        ok = actual == expected;
        fprintf(stderr, "menu harness: save directories %d, expected total %d\n", actual, expected);
    } else if (!strncmp(name, "import-count:", 13)) {
        int expected = atoi(name + 13);
        int actual = settingsSaveCount();
        ok = sSettingsImportBefore >= 0 && p4_settings_active() &&
            actual == sSettingsImportBefore + expected;
        fprintf(stderr, "menu harness: import slots %d, baseline %d, expected addition %d\n",
            actual, sSettingsImportBefore, expected);
    } else if (!strcmp(name, "import-deferred")) {
        ok = sSettingsImportBefore >= 0 && !p4_settings_active() &&
            settingsSaveCount() == sSettingsImportBefore;
    } else {
        fprintf(stderr, "menu harness: unknown native settings check %s\n", name);
    }
    ++sSettingsChecks;
    if (!ok) ++sSettingsFailed;
    fprintf(stderr, "menu harness: native settings check %s: %s\n", name, ok ? "PASS" : "FAIL");
}


/* P4PC_TURBO=2 keeps only the warmup before the next screenshot. Use the
 * scheduler's real delays: tagged settings actions and pad presses wait
 * stepFlips, checks take one flip, and retrying gates have no known distance.
 * Re-plan after each token and whenever a retrying gate actually completes. */
static void hintShotDraws(unsigned frame, unsigned stepFlips)
{
    static int window = -1;
    unsigned r = sWaitUntil > frame ? sWaitUntil - frame : 1;
    int j;

    if (window < 0) {
        const char *w = getenv("P4PC_DRAW_WINDOW");

        window = w != NULL && *w != '\0' ? atoi(w) : 30;
    }
    for (j = sIdx; j < sNumToks; j++) {
        const char *next = sToks[j];
        if (strncmp(next, "shot:", 5) == 0) {
            const unsigned w = (unsigned)window;

            p4gl_context_draw_between(r > w ? r - w : 0, (r > w ? w : r) + 4u);
            return;
        }
        if (strncmp(next, "wait", 4) == 0) {
            r += (unsigned)atoi(next + 4);
        } else if (!strcmp(next, "open") || !strcmp(next, "settings-title-ready") ||
                   !strncmp(next, "settings-row:", 13)) {
            return;
        } else if (!strcmp(next, "save") || !strcmp(next, "load")) {
            r += 120u;
        } else if (buttonBit(next) != 0 ||
                   (!strncmp(next, "settings-", 9) &&
                    strncmp(next, "settings-check:", 15) && strncmp(next, "settings-block:", 15))) {
            r += stepFlips ? stepFlips : 1u;
        } else {
            r++;
        }
    }
}

static void scriptStep(unsigned frame)
{
    const uint8_t *camp = (const uint8_t *)P4_G2H((uint32_t)iGpffffb1ec);
    static int stepFlips = -1;
    const char *tok;

    if (sScriptState == 2 || frame < sWaitUntil) {
        return;
    }
    if (sScriptState == 0) {
        const char *e = getenv("P4PC_MENU_SCRIPT");
        const char *st = getenv("P4PC_MENU_STEP");
        char *copy, *save = NULL, *t;

        stepFlips = st != NULL && *st != '\0' ? atoi(st) : 45;
        if (e == NULL || *e == '\0') {
            sScriptState = 2;
            return;
        }
        #ifdef P4_MOD_GAME_BINDINGS
        p4_mod_fidelity_check_start();
        #endif
        settingsPagesFixtureInit();
        copy = strdup(e);
        for (t = strtok_r(copy, ",", &save); t != NULL && sNumToks < SCRIPT_MAX; t = strtok_r(NULL, ",", &save)) {
            sToks[sNumToks++] = t;
        }
        sScriptState = 1;
        sWaitUntil = frame + 450;   /* the field needs a while to load before the first step */
        return;
    }
    if (sPendingShot) {   /* the frame requested by the last `shot:` has been presented: was it drawn? */
        double lit = p4gl_context_capture_nonblack();
        int written = p4gl_context_capture_written();

        sPendingShot = 0;
        sShots++;
        if (lit < 0.3) {
            sShotsDark++;
        }
        if (!written) {
            sShotsFailed++;
        }
        fprintf(stderr, "p4pc: menu harness: frame %u shot lit %.3f, PNG %s\n", frame, lit, written ? "written" : "FAILED");
    }
    if (sIdx >= sNumToks) {
        int ok = (sCheckUsed ? sCheckOk : (sCampSeen || sSettingsChecks)) &&
            sSettingsFailed == 0 && sShotsDark == 0 && sShotsFailed == 0;

        sScriptState = 2;
        fprintf(stderr, "p4pc: menu harness: frame %u script finished: camp %s, %d shots, %d dark, %d failed writes, state check %s: %s\n", frame,
                sCampSeen ? "opened" : "never opened", sShots, sShotsDark, sShotsFailed, !sCheckUsed ? "unused" : sCheckOk ? "ok" : "FAILED", ok ? "PASS" : "FAIL");
        if (getenv("P4PC_MENU_EXIT") != NULL) {
            fflush(stderr);
            exit(ok ? 0 : 1);
        }
        return;
    }
    tok = sToks[sIdx];
    if (strcmp(tok, "open") == 0) {
        if (camp != NULL && *(const uint32_t *)camp >= 2) {
            logCamp(frame, "camp is open:");
            sCampSeen = 1;
            sIdx++;
            sWaitUntil = frame + 30;
            hintShotDraws(frame, (unsigned)stepFlips);
        } else {
            menuPress(0x1000, 8);
            sWaitUntil = frame + 60;   /* the token stays current until the camp task has left state 0 */
        }
        return;
    }
    if (!strcmp(tok, "settings-title-ready")) {
        if (getenv("P4PC_SETTINGS_TEST") == NULL) exit(1);
        if (p4_settings_native_title_ready()) {
            fprintf(stderr, "menu harness: real title selector is ready\n");
            ++sIdx;
            sWaitUntil = frame + (unsigned)stepFlips;
            hintShotDraws(frame, (unsigned)stepFlips);
        } else {
            /* Normal Start only while logos/movie precede the real selector;
             * never press it again once the three-entry title menu is ready. */
            menuPress(buttonBit("start"), 6);
            sWaitUntil = frame + 60;
        }
        return;
    }
    if (!strncmp(tok, "settings-row:", 13)) {
        int reached = settingsNavigateRow(tok + 13);
        if (reached) ++sIdx;
        sWaitUntil = frame + (unsigned)stepFlips;
        if (reached) hintShotDraws(frame, (unsigned)stepFlips);
        return;
    }
    sIdx++;
    if (strncmp(tok, "wait", 4) == 0) {
        sWaitUntil = frame + (unsigned)atoi(tok + 4);
    } else if (!strcmp(tok, "settings-hide-cursor")) {
        if (!getenv("P4PC_SETTINGS_TEST") || !SDL_HideCursor()) exit(1);
    } else if (!strcmp(tok, "settings-mark-cursor")) {
        int view;
        sSettingsCursorBefore = p4_settings_native_cursor(&view);
        if (sSettingsCursorBefore < 0) exit(1);
    } else if (!strcmp(tok, "settings-stick-down")) {
        if (p4pc_pad_virtual_stick(0, 32767, 60)) exit(1);
        sWaitUntil = frame + 60 + (unsigned)stepFlips;
    } else if (!strcmp(tok, "settings-open")) {
        settingsKey("F1");
        sWaitUntil = frame + (unsigned)stepFlips;
    } else if (!strncmp(tok, "settings-key:", 13)) {
        settingsKey(tok + 13);
        sWaitUntil = frame + (unsigned)stepFlips;
    } else if (!strncmp(tok, "settings-hold:", 14)) {
        char name[24];
        unsigned frames;
        if (sscanf(tok + 14, "%23[^:]:%u", name, &frames) != 2 ||
            !buttonBit(name) || !frames || frames > 300) exit(1);
        menuPress(buttonBit(name), frames);
        sWaitUntil = frame + frames + (unsigned)stepFlips;
    } else if (!strncmp(tok, "settings-mouse-row:", 19)) {
        settingsMouseRow(tok + 19);
        sWaitUntil = frame + (unsigned)stepFlips;
    } else if (!strncmp(tok, "settings-click-row:", 19)) {
        settingsMouseClickRow(tok + 19);
        sWaitUntil = frame + (unsigned)stepFlips;
    } else if (!strncmp(tok, "settings-touch-row:", 19)) {
        settingsTouchRow(tok + 19);
        sWaitUntil = frame + (unsigned)stepFlips;
    } else if (!strncmp(tok, "settings-swipe:", 15)) {
        settingsSwipe(tok + 15);
        sWaitUntil = frame + (unsigned)stepFlips;
    } else if (!strncmp(tok, "settings-mouse:", 15)) {
        settingsMouse(tok + 15);
        sWaitUntil = frame + (unsigned)stepFlips;
    } else if (!strncmp(tok, "settings-touch:", 15)) {
        settingsTouch(tok + 15);
        sWaitUntil = frame + (unsigned)stepFlips;
    } else if (!strncmp(tok, "settings-page:", 14)) {
        settingsPagesFixtureCommand(tok + 14);
        sWaitUntil = frame + (unsigned)stepFlips;
    } else if (!strncmp(tok, "settings-block:", 15)) {
        datSetFlag(0x37, atoi(tok + 15) != 0);
    } else if (!strncmp(tok, "settings-check:", 15)) {
        settingsCheck(tok + 15);
    } else if (!strncmp(tok, "settings-import:", 16)) {
        settingsImportPhase(tok + 16);
        sWaitUntil = frame + (unsigned)stepFlips;
    } else if (strncmp(tok, "shot:", 5) == 0) {
        char path[256];

        snprintf(path, sizeof path, "menu_%s.png", tok + 5);
        p4gl_context_capture_next(path);
        fprintf(stderr, "p4pc: menu harness: frame %u shot %s\n", frame, path);
        sPendingShot = 1;
        sWaitUntil = frame + 20;
    } else if (strcmp(tok, "save") == 0 || strcmp(tok, "load") == 0) {
        /* what the event commands at 0x0025D460 (save, mode 1) and 0x0025D4D0 (load, mode 0) do: the card screen as a root task */
        int32_t task = mcIconTaskCreate(NULL, strcmp(tok, "save") == 0 ? 1 : 0);

        sMcTask = task;
        fprintf(stderr, "p4pc: menu harness: frame %u %s: memory card task %08x\n", frame, tok, (unsigned)task);
        sWaitUntil = frame + 120;
    } else if (strncmp(tok, "hash", 4) == 0) {
        /* FNV-1a of the image the save screen would write: the game state as the save format sees it */
        int32_t size = 0;
        uint8_t *img = datSaveBuildImage(0, &size, 0);
        uint32_t h = 2166136261u;
        int32_t i;

        for (i = 0; img != NULL && i < size; i++) {
            h = (h ^ img[i]) * 16777619u;
        }
        fprintf(stderr, "p4pc: menu harness: frame %u state hash %s: image %d bytes, fnv1a %08x\n", frame, tok + 4, (int)size, (unsigned)h);
        if (img != NULL) {
            P4_GAME_FREE(img);
        }
    } else if (strncmp(tok, "keep:", 5) == 0) {
        /* keep the save image under a name, for `check` */
        int k;

        for (k = 0; k < KEEP_MAX && sKeep[k].data != NULL; k++) {
        }
        if (k < KEEP_MAX) {
            int32_t size = 0;
            uint8_t *img = datSaveBuildImage(0, &size, 0);

            snprintf(sKeep[k].name, sizeof sKeep[k].name, "%s", tok + 5);
            sKeep[k].data = (uint8_t *)malloc((size_t)size);
            memcpy(sKeep[k].data, img, (size_t)size);
            sKeep[k].size = size;
            P4_GAME_FREE(img);
            fprintf(stderr, "p4pc: menu harness: frame %u kept image %s (%d bytes)\n", frame, sKeep[k].name, (int)size);
        }
    } else if (strncmp(tok, "vsfile:", 7) == 0) {
        /* vsfile:<kept image>:<path>: every byte range where the kept image differs from the file (a save written by retail, to show what the
         * port's save format would change in it) */
        char name[16];
        const char *path = strchr(tok + 7, ':');
        int k, ik = -1;
        FILE *f = NULL;

        if (path != NULL && (size_t)(path - (tok + 7)) < sizeof name) {
            snprintf(name, sizeof name, "%.*s", (int)(path - (tok + 7)), tok + 7);
            for (k = 0; k < KEEP_MAX; k++) {
                if (sKeep[k].data != NULL && strcmp(sKeep[k].name, name) == 0) ik = k;
            }
            f = fopen(path + 1, "rb");
        }
        if (ik >= 0 && f != NULL) {
            int32_t n = sKeep[ik].size, i, start = -1, ranges = 0, total = 0;
            uint8_t *file = (uint8_t *)malloc((size_t)n);
            size_t got = file != NULL ? fread(file, 1, (size_t)n, f) : 0;

            fprintf(stderr, "p4pc: menu harness: frame %u vsfile %s: image %d bytes, file %d bytes\n", frame, tok + 7, (int)n, (int)got);
            for (i = 0; i <= n && got == (size_t)n; i++) {
                int d = i < n && sKeep[ik].data[i] != file[i];

                if (d && start < 0) start = i;
                if (!d && start >= 0) {
                    fprintf(stderr, "p4pc: menu harness: vsfile diff 0x%05x +%d: file %02x.. image %02x..\n", start, i - start, file[start], sKeep[ik].data[start]);
                    total += i - start;
                    ranges++;
                    start = -1;
                }
            }
            sCheckUsed = 1;
            sCheckOk = got == (size_t)n && total <= 16;      /* play-time counters and the checksums that cover them */
            fprintf(stderr, "p4pc: menu harness: vsfile %d differing bytes in %d ranges (16 or fewer: play-time counters): %s\n", total, ranges, sCheckOk ? "ok" : "FAILED");
            free(file);
        } else {
            fprintf(stderr, "p4pc: menu harness: frame %u vsfile %s: image or file missing\n", frame, tok + 7);
        }
        if (f != NULL) fclose(f);
    } else if (strncmp(tok, "check:", 6) == 0) {
        /* check:<saved>:<reset>:<loaded>: the loaded image equals the saved one in all but a few volatile bytes (play time counters), and the reset image
         * differs from the saved one (so the comparison is not vacuous) */
        char a[16], b[16], c[16];
        int ia = -1, ib = -1, ic = -1, k;

        if (sscanf(tok + 6, "%15[^:]:%15[^:]:%15s", a, b, c) == 3) {
            for (k = 0; k < KEEP_MAX; k++) {
                if (sKeep[k].data != NULL && strcmp(sKeep[k].name, a) == 0) ia = k;
                if (sKeep[k].data != NULL && strcmp(sKeep[k].name, b) == 0) ib = k;
                if (sKeep[k].data != NULL && strcmp(sKeep[k].name, c) == 0) ic = k;
            }
        }
        sCheckUsed = 1;
        if (ia >= 0 && ib >= 0 && ic >= 0 && sKeep[ia].size == sKeep[ic].size && sKeep[ia].size == sKeep[ib].size) {
            int32_t n = sKeep[ia].size, i, loadedDiff = 0, resetDiff = 0;

            for (i = 0; i < n; i++) {
                loadedDiff += sKeep[ia].data[i] != sKeep[ic].data[i];
                resetDiff += sKeep[ia].data[i] != sKeep[ib].data[i];
            }
            sCheckOk = loadedDiff <= 16 && resetDiff >= 8;
            fprintf(stderr, "p4pc: menu harness: frame %u check %s: loaded differs from saved in %d bytes (volatile counters only: 16 or fewer), reset differs in %d (8 or more): %s\n",
                    frame, tok + 6, loadedDiff, resetDiff, sCheckOk ? "ok" : "FAILED");
        } else {
            fprintf(stderr, "p4pc: menu harness: frame %u check %s: image missing\n", frame, tok + 6);
        }
    } else if (strncmp(tok, "dump:", 5) == 0) {
        /* the save image as a file (state_<name>.bin), to diff a saved game against a loaded one byte by byte */
        int32_t size = 0;
        uint8_t *img = datSaveBuildImage(0, &size, 0);
        char path[256];
        FILE *f;

        snprintf(path, sizeof path, "state_%s.bin", tok + 5);
        f = img != NULL ? fopen(path, "wb") : NULL;
        if (f != NULL) {
            fwrite(img, 1, (size_t)size, f);
            fclose(f);
        }
        fprintf(stderr, "p4pc: menu harness: frame %u dumped %s (%d bytes)\n", frame, path, (int)size);
        if (img != NULL) {
            P4_GAME_FREE(img);
        }
    } else if (strcmp(tok, "reset") == 0) {
        datInitDefaultGameState();
        fprintf(stderr, "p4pc: menu harness: frame %u state reset to the new-game state\n", frame);
    } else if (strcmp(tok, "totitle") == 0) {
        /* quit to the title: gameSequence (27) again; its logo, opening movie and title screen follow, as after a reset */
        fprintf(stderr, "p4pc: menu harness: frame %u back to sequence %d (title)\n", frame, SEQ_GAME);
        sequenceRequest(SEQ_GAME, 0, 0, 0);
    } else if (strcmp(tok, "mod") == 0) {
        /* a recognisable game state: money, the hero's level, a few story flags */
        datSetMoney(12345);
        datSetHeroLevel(1, 7);
        datSetUnitHp(1, 1);
        datSetFlag(0x100, 1);
        datSetFlag(0x101, 1);
        datSetFlag(0x102, 0);
        fprintf(stderr, "p4pc: menu harness: frame %u state modified (money 12345, hero level 7, hp 1, flags 0x100/0x101 set)\n", frame);
    } else if (strcmp(tok, "state") == 0) {
        logCamp(frame, "state:");
    } else if (buttonBit(tok) != 0) {
        menuPress(buttonBit(tok), 6);
        sWaitUntil = frame + (unsigned)stepFlips;
        fprintf(stderr, "p4pc: menu harness: frame %u press %s\n", frame, tok);
    } else {
        fprintf(stderr, "p4pc: menu harness: unknown script token '%s'\n", tok);
    }
    hintShotDraws(frame, (unsigned)stepFlips);
}

void p4pc_menu_harness_frame(unsigned frame)
{
    if (sField == -2) {
        const char *e = getenv("P4PC_START_MENU");
        const char *d = getenv("P4PC_START_MENU_AT");
        char *end = NULL;

        sField = -1;
        if (e != NULL && *e != '\0') {
            #ifdef P4_MOD_GAME_BINDINGS
            p4_mod_fidelity_check_start();
            #endif
            sField = (int32_t)strtol(e, &end, 0);
            if (end != NULL && *end == ':') {
                sRoom = (int32_t)strtol(end + 1, NULL, 0);
            }
            fprintf(stderr, "p4pc: menu harness: P4PC_START_MENU=%d:%d\n", (int)sField, (int)sRoom);
        }
        if (d != NULL && *d != '\0') {
            sDelay = atoi(d);
        }
    }
    if (sField < 0) {
        /* A package smoke observes natural ISO startup through the title. A
         * field override is optional, not a prerequisite for scripted input. */
        if (sScriptState != 2) scriptStep(frame);
        return;
    }
    if (!sDone) {
        if (currentSequence() == SEQ_GAME) {
            if (!sSeen27) {
                sSeen27 = frame ? (int)frame : 1;
            }
            /* Match the logo destructor's readiness requirement, not host disc speed. */
            uint8_t *logo = (uint8_t *)kwlnTaskGetTaskByName(D_005E57A8);
            uint8_t *work = logo != NULL ? (uint8_t *)P4_G2H(*(uint32_t *)(logo + 0x38)) : NULL;

            if (work == NULL || *(int32_t *)work < 4) {
                return;
            }
            if ((int)frame - sSeen27 >= sDelay) {
                sDone = 1;
                startField();
            }
        }
        return;
    }
    {
        static uint16_t lastEdge;
        if (D_008C024E[0] != lastEdge) {
            lastEdge = D_008C024E[0];
            fprintf(stderr, "p4pc: menu harness: frame %u pad edge word %04x\n", frame, lastEdge);
        }
    }
    scriptStep(frame);
    if (sMcTask != 0 && frame % 30 == 0) {   /* the card screen work (mc.c): state s16 +0, sub-state s16 +2, flags +4, step +0xC, slot +0x3AC */
        const uint8_t *tk = (const uint8_t *)P4_G2H((uint32_t)sMcTask);
        const uint8_t *w = (const uint8_t *)P4_G2H(*(const uint32_t *)(tk + 0x38));

        if (w != NULL) {
            fprintf(stderr, "p4pc: menu harness: frame %u mc screen state %d/%d flags %08x step %d slot %d\n", frame, (int)*(const int16_t *)w,
                    (int)*(const int16_t *)(w + 2), (unsigned)*(const uint32_t *)(w + 4), (int)*(const int32_t *)(w + 0xC), (int)*(const int32_t *)(w + 0x3AC));
        }
    }
    if (frame % 60 == 0) {
        const uint8_t *camp = (const uint8_t *)P4_G2H((uint32_t)iGpffffb1ec);
        uint32_t fieldHandle = *(uint32_t *)P4_G2H(0x007D2510u);   /* the field root task, as the viewer reads it */

        fprintf(stderr, "p4pc: menu harness: frame %u seq %d field task %08x camp %p", frame, (int)currentSequence(), (unsigned)fieldHandle,
                (const void *)camp);
        if (camp != NULL) {
            fprintf(stderr, " state %u +4 %u +8 %u +0xC %u +0x28 %u", (unsigned)*(const uint32_t *)camp, (unsigned)*(const uint32_t *)(camp + 4),
                    (unsigned)*(const uint32_t *)(camp + 8), (unsigned)*(const uint32_t *)(camp + 0xC), (unsigned)*(const uint32_t *)(camp + 0x28));
        }
        fprintf(stderr, "\n");
    }
    {
        const char *g = getenv("P4PC_MENU_GATE");

        if (g != NULL && *g != '\0' && frame >= (unsigned)atoi(g)) {
            const uint8_t *camp = (const uint8_t *)P4_G2H((uint32_t)iGpffffb1ec);
            double lit = p4gl_context_capture_nonblack();
            int ok = camp != NULL && *(const uint32_t *)camp != 0 && lit >= 0.3;

            fprintf(stderr, "p4pc: camp gate %s: camp task %p state %d, %.3f of the captured frame lit (need 0.3+)\n", ok ? "PASS" : "FAIL",
                    (const void *)camp, camp != NULL ? (int)*(const uint32_t *)camp : -1, lit);
            fflush(stderr);
            exit(ok ? 0 : 1);
        }
    }
}
