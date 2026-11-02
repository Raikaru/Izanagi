/* Host input for libpad: one SDL window, the keyboard, and the first gamepad.
 *
 * Keyboard (physical keys, DualShock 2 names):
 *   D-pad      arrows or W A S D
 *   Cross      Z        Circle  X        Triangle  C        Square  V
 *   Start      Enter    Select  Backspace
 *   L1 / R1    Q / E    L2 / R2 1 / 3    L3 / R3   Left / Right Shift
 *   Left stick is the D-pad when no gamepad stick is past the dead zone.
 *   F11 or Alt+Enter: fullscreen on and off (when the picture is drawn offscreen and fitted: p4cfg_video()->blit).
 * Every binding can be changed in the settings file (src/platform/config/p4cfg.c, [input]).
 *
 * Gamepad (SDL names; SDL's south/east/west/north are Cross/Circle/Square/Triangle, the DualShock layout by position on any pad):
 *   Face buttons, Start, Back (Select), shoulders, stick clicks, D-pad, both sticks (PCSX2 sensitivity, optional extra dead zone),
 *   and the triggers as L2/R2 with pressure. The first pad found is used; one plugged in later is picked up, and when it is
 *   unplugged another connected pad takes over.
 *
 * Closing the window ends the process. Nothing else in the port has a quit path. */
#include "pad_sdl.h"
#ifdef P4_MOD_GAME_BINDINGS
#include "p4pc/package_boot.h"
#endif

#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "p4pc/os.h"
#include "p4pc/guest.h"
#include "p4pc/hostmath.h"
#include "../config/p4cfg.h"
#include "p4pc/walk_script.h"
#include "../../render/p4gl_api.h"
#include "../../render/p4gl_win.h"
#include "p4pc/settings.h"
#ifdef P4_MOD_GAME_BINDINGS
#include "p4pc/mod_fidelity.h"
#endif

#define P4PC_STICK_DEAD_PERCENT 0 /* PCSX2's default; P4 already applies its own movement dead zone */

static SDL_Window *sWindow;
static SDL_Gamepad *sGamepad;
static void loadBindings(void);
static void virtualPadInit(void);
static void virtualPadUpdate(void);
static int sInitState; /* 0 not tried, 1 ok, -1 failed */
static int sInputMode; /* 0 player, 1 script (video only), 2 explicit virtual fixture only */
static void (*sKeyHook)(int);
static int sSettingsFence;
static int (*sSettingsEvent)(const SDL_Event *);
static int (*sSettingsActive)(void);
static int (*sSettingsMenuActive)(void);
static void (*sSettingsPad)(SDL_Gamepad *);

void p4pc_pad_set_settings_hooks(int (*event)(const SDL_Event *), int (*active)(void),
                                int (*menu_active)(void), void (*pad)(SDL_Gamepad *))
{
    sSettingsEvent = event;
    sSettingsActive = active;
    sSettingsMenuActive = menu_active;
    sSettingsPad = pad;
}

void p4pc_pad_settings_fence(void)
{
    sSettingsFence = 1;
}

void p4pc_pad_set_key_hook(void (*hook)(int))
{
    sKeyHook = hook;
}

static void openGamepad(SDL_JoystickID id)
{
    if (sGamepad != NULL) {
        return;
    }
    sGamepad = SDL_OpenGamepad(id);
    if (sGamepad != NULL) {
        fprintf(stderr, "p4pc pad: gamepad %s\n", SDL_GetGamepadName(sGamepad));
    }
}

/* Opens the first connected gamepad (at start, and after the one in use is unplugged). */
static void openAnyGamepad(void)
{
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    int i;

    for (i = 0; ids != NULL && i < count && sGamepad == NULL; i++) {
        openGamepad(ids[i]);
    }
    SDL_free(ids);
}

/* F11 or Alt+Enter: fullscreen on and off. Only where the picture is fitted to the window: drawn directly, the window is the frame. */
static void toggleFullscreen(void)
{
#if !defined(SDL_PLATFORM_ANDROID) && !defined(SDL_PLATFORM_IOS)
    if (sWindow != NULL && p4cfg_video()->blit) {
        const bool full = (SDL_GetWindowFlags(sWindow) & SDL_WINDOW_FULLSCREEN) != 0;

        SDL_SetWindowFullscreen(sWindow, !full);
        if (!sSettingsMenuActive || !sSettingsMenuActive()) {
            if (!full) SDL_HideCursor();
            else SDL_ShowCursor();
        }
    }
#endif
}

/* SDL_EVENT_RENDER_DEVICE_RESET: on Android SDL replaces the GL context itself when the old one did not survive a pause (src/render/p4gl_context.c). */
static volatile int sDeviceReset;

int p4pc_pad_sdl_take_device_reset(void)
{
    const int r = sDeviceReset;

    sDeviceReset = 0;
    return r;
}

static void pumpEvents(void)
{
    SDL_Event event;
    static uint64_t last;
    const uint64_t now = p4_os_now_ns();

    /* The window system is asked for events at most every 2 ms: an unattended run reads the pad 1500 times a second, and on Windows
     * each pump is a PeekMessage round trip (6% of a frame, measured). A frame at 60 Hz is 16 ms, so a player cannot tell. */
    if (last != 0 && now - last < 2000000u) {
        return;
    }
    last = now;
    SDL_PumpEvents();
    while (SDL_PollEvent(&event)) {
        int settingsEvent = sInputMode == 0;
        if (!settingsEvent && getenv("P4PC_SETTINGS_TEST") != NULL) {
            if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
                settingsEvent = event.key.which == P4_SETTINGS_TEST_DEVICE;
            else if (event.type == SDL_EVENT_FINGER_DOWN || event.type == SDL_EVENT_FINGER_UP ||
                     event.type == SDL_EVENT_FINGER_MOTION)
                settingsEvent = event.tfinger.touchID == P4_SETTINGS_TEST_DEVICE;
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                settingsEvent = event.button.which == P4_SETTINGS_TEST_DEVICE;
            else if (event.type == SDL_EVENT_MOUSE_MOTION)
                settingsEvent = event.motion.which == P4_SETTINGS_TEST_DEVICE;
            else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_GAMEPAD_BUTTON_UP ||
                     event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION)
                settingsEvent = sInputMode == 2;
        }
        SDL_JoystickID eventPad = event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_GAMEPAD_BUTTON_UP
            ? event.gbutton.which : event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION ? event.gaxis.which : 0;
        if (sSettingsEvent != NULL && settingsEvent &&
            (!eventPad || (sGamepad != NULL && eventPad == SDL_GetGamepadID(sGamepad))) &&
            sSettingsEvent(&event)) {
            /* Mouse/touch intent never enters a pad packet. Fencing it can
             * strand a held controller behind a mouse-motion event. */
            if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP ||
                event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_GAMEPAD_BUTTON_UP ||
                event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION) sSettingsFence = 1;
            if (event.type != SDL_EVENT_QUIT && event.type != SDL_EVENT_RENDER_DEVICE_RESET &&
                event.type != SDL_EVENT_GAMEPAD_REMOVED && event.type != SDL_EVENT_GAMEPAD_ADDED) continue;
        }
        switch (event.type) {
        case SDL_EVENT_QUIT:
            fprintf(stderr, "p4pc pad: window closed, exiting\n");
            fflush(stderr);
#ifdef P4_MOD_GAME_BINDINGS
            p4_package_boot_clean_exit();
#endif
            exit(0);
        case SDL_EVENT_RENDER_DEVICE_RESET:
            sDeviceReset = 1;
            break;
        case SDL_EVENT_GAMEPAD_ADDED:
            if (sInputMode == 0) openGamepad((SDL_JoystickID)event.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            if (sGamepad != NULL && SDL_GetGamepadID(sGamepad) == (SDL_JoystickID)event.gdevice.which) {
                SDL_CloseGamepad(sGamepad);
                sGamepad = NULL;
                fprintf(stderr, "p4pc pad: gamepad removed\n");
                if (sInputMode == 0) openAnyGamepad(); /* player-only takeover */
            }
            break;
        case SDL_EVENT_KEY_DOWN:
            /* Session controls use queued events; gameplay input keeps its mode isolation. */
            if (!event.key.repeat && sKeyHook != NULL) {
                sKeyHook((int)event.key.scancode);
            }
            if (sInputMode == 0 && !event.key.repeat && (event.key.scancode == SDL_SCANCODE_F11 ||
                                      (event.key.scancode == SDL_SCANCODE_RETURN && (event.key.mod & SDL_KMOD_ALT) != 0))) {
                toggleFullscreen();
            }
            break;
        default:
            break;
        }
    }
}

void p4_settings_pump(void)
{
    pumpEvents();
    if ((sInputMode == 0 || sInputMode == 2) && sSettingsPad != NULL) {
        if (sGamepad && SDL_GetGamepadButton(sGamepad, SDL_GAMEPAD_BUTTON_BACK) &&
            SDL_GetGamepadButton(sGamepad, SDL_GAMEPAD_BUTTON_START)) sSettingsFence = 1;
        sSettingsPad(sGamepad);
    }
}

extern int p4_battle_sim_renderless;

int p4pc_pad_sdl_init(void)
{
    const char *hidden;

    if (sInitState != 0) {
        return sInitState > 0 ? 0 : -1;
    }
    if (p4_battle_sim_renderless) {
        /* No video/gamepad subsystem, window or physical input in a CPU case.
         * Script injection and the ordinary pad packet/update owner stay real. */
        sInputMode = 1;
        if (!SDL_Init(SDL_INIT_EVENTS)) {
            fprintf(stderr, "p4pc pad: simulator event init failed: %s\n", SDL_GetError());
            sInitState = -1;
            return -1;
        }
        loadBindings();
        sInitState = 1;
        return 0;
    }
    {
        const char *mode = getenv("P4PC_INPUT");
        if (mode != NULL && strcmp(mode, "script") == 0) sInputMode = 1;
        else if (mode != NULL && strcmp(mode, "virtual") == 0) sInputMode = 2;
    }
    #ifdef P4_MOD_GAME_BINDINGS
    if (sInputMode != 0) p4_mod_fidelity_check_start();
    else if (p4_mod_witness_validate()) { sInitState = -1; return -1; }
    #endif
    if (!SDL_Init(SDL_INIT_VIDEO | (sInputMode == 1 ? 0 : SDL_INIT_GAMEPAD))) {
        fprintf(stderr, "p4pc pad: SDL_Init failed: %s\n", SDL_GetError());
        sInitState = -1;
        return -1;
    }
    /* P4PC_HIDDEN_WINDOW=1: create the window hidden, so a run never shows anything on the desktop.
     * The Windows build uses it for headless runs: SDL's offscreen driver needs an EGL library
     * there, and a hidden window still gives WGL a GL context. */
    hidden = getenv("P4PC_HIDDEN_WINDOW");
    /* The GL API (desktop 3.3 core or ES 3.0) decides the window's pixel format or EGL config, so its attributes are set before the
     * window exists; p4gl_context_open sets them again. */
    p4gl_api_set_attributes();
    /* On Android a fullscreen window is what makes SDL hide the status and navigation bars (immersive mode); the picture is fitted into the
     * whole screen by the present blit (src/render/p4gl_context.c). */
    {
        /* The size, fullscreen and resizing come from the settings (p4cfg_video): a window drawn into directly is the 640x448 frame and stays so; one the
         * picture is fitted to is resizable, fullscreen on request, and on a high-density display has its true pixels (the picture is blitted to them). */
        const P4CfgVideo *video = p4cfg_video();
        SDL_WindowFlags flags = P4GL_WIN_FLAG | (hidden != NULL && hidden[0] == '1' ? SDL_WINDOW_HIDDEN : 0);

        if (video->blit) {
            flags |= SDL_WINDOW_RESIZABLE;
#if !defined(SDL_PLATFORM_ANDROID)
            flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif
        }
        if (video->fullscreen) {
            flags |= SDL_WINDOW_FULLSCREEN;
        }
        sWindow = SDL_CreateWindow("Persona 4 PC", video->width, video->height, flags);
        if (sWindow != NULL && video->fullscreen && !(hidden != NULL && hidden[0] == '1')) {
            SDL_HideCursor();
        }
    }
    if (sWindow == NULL) {
        fprintf(stderr, "p4pc pad: SDL_CreateWindow failed: %s\n", SDL_GetError());
        sInitState = -1;
        return -1;
    }
    loadBindings();
    if (sInputMode == 2) virtualPadInit();
    if (sInputMode == 0) openAnyGamepad();
    sInitState = 1;
    return 0;
}

SDL_Window *p4pc_pad_sdl_window(void)
{
    return sWindow;
}

static uint8_t triggerByte(Sint16 axis)
{
    return (uint8_t)((int)axis * 255 / 32767);
}

/* Digital button bit with a keyboard key and a gamepad button. */
typedef struct KeyBinding
{
    uint16_t bit;
    SDL_Scancode key[2];
} KeyBinding;

static KeyBinding kKeys[] = {
    { P4PC_PAD_UP, { SDL_SCANCODE_UP, SDL_SCANCODE_W } },
    { P4PC_PAD_DOWN, { SDL_SCANCODE_DOWN, SDL_SCANCODE_S } },
    { P4PC_PAD_LEFT, { SDL_SCANCODE_LEFT, SDL_SCANCODE_A } },
    { P4PC_PAD_RIGHT, { SDL_SCANCODE_RIGHT, SDL_SCANCODE_D } },
    { P4PC_PAD_CROSS, { SDL_SCANCODE_Z, SDL_SCANCODE_UNKNOWN } },
    { P4PC_PAD_CIRCLE, { SDL_SCANCODE_X, SDL_SCANCODE_UNKNOWN } },
    { P4PC_PAD_TRIANGLE, { SDL_SCANCODE_C, SDL_SCANCODE_UNKNOWN } },
    { P4PC_PAD_SQUARE, { SDL_SCANCODE_V, SDL_SCANCODE_UNKNOWN } },
    { P4PC_PAD_START, { SDL_SCANCODE_RETURN, SDL_SCANCODE_UNKNOWN } },
    { P4PC_PAD_SELECT, { SDL_SCANCODE_BACKSPACE, SDL_SCANCODE_UNKNOWN } },
    { P4PC_PAD_L1, { SDL_SCANCODE_Q, SDL_SCANCODE_UNKNOWN } },
    { P4PC_PAD_R1, { SDL_SCANCODE_E, SDL_SCANCODE_UNKNOWN } },
    { P4PC_PAD_L2, { SDL_SCANCODE_1, SDL_SCANCODE_UNKNOWN } },
    { P4PC_PAD_R2, { SDL_SCANCODE_3, SDL_SCANCODE_UNKNOWN } },
    { P4PC_PAD_L3, { SDL_SCANCODE_LSHIFT, SDL_SCANCODE_UNKNOWN } },
    { P4PC_PAD_R3, { SDL_SCANCODE_RSHIFT, SDL_SCANCODE_UNKNOWN } },
};

/* Pressure index for each keyboard-bound digital button, or -1. */
static int pressureIndex(uint16_t bit)
{
    switch (bit) {
    case P4PC_PAD_RIGHT: return P4PC_PRESS_RIGHT;
    case P4PC_PAD_LEFT: return P4PC_PRESS_LEFT;
    case P4PC_PAD_UP: return P4PC_PRESS_UP;
    case P4PC_PAD_DOWN: return P4PC_PRESS_DOWN;
    case P4PC_PAD_TRIANGLE: return P4PC_PRESS_TRIANGLE;
    case P4PC_PAD_CIRCLE: return P4PC_PRESS_CIRCLE;
    case P4PC_PAD_CROSS: return P4PC_PRESS_CROSS;
    case P4PC_PAD_SQUARE: return P4PC_PRESS_SQUARE;
    case P4PC_PAD_L1: return P4PC_PRESS_L1;
    case P4PC_PAD_R1: return P4PC_PRESS_R1;
    case P4PC_PAD_L2: return P4PC_PRESS_L2;
    case P4PC_PAD_R2: return P4PC_PRESS_R2;
    default: return -1;
    }
}

static void setBit(P4PadSnapshot *out, uint16_t bit)
{
    int p = pressureIndex(bit);

    out->buttons |= bit;
    if (p >= 0) {
        out->pressure[p] = 0xFF;
    }
}

/* The gamepad button that gives each PS2 button (input.pad.*): SDL's south/east/west/north are the DualShock's Cross/Circle/Square/Triangle by position, so
 * a pad of any maker plays as the PS2 pad's layout. L2 and R2 are the analog triggers, and also a button when one is bound (none by default). */
typedef struct PadBinding
{
    uint16_t bit;
    SDL_GamepadButton button;
} PadBinding;

static PadBinding kPad[] = {
    { P4PC_PAD_CROSS, SDL_GAMEPAD_BUTTON_SOUTH },
    { P4PC_PAD_CIRCLE, SDL_GAMEPAD_BUTTON_EAST },
    { P4PC_PAD_SQUARE, SDL_GAMEPAD_BUTTON_WEST },
    { P4PC_PAD_TRIANGLE, SDL_GAMEPAD_BUTTON_NORTH },
    { P4PC_PAD_START, SDL_GAMEPAD_BUTTON_START },
    { P4PC_PAD_SELECT, SDL_GAMEPAD_BUTTON_BACK },
    { P4PC_PAD_L1, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER },
    { P4PC_PAD_R1, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER },
    { P4PC_PAD_L3, SDL_GAMEPAD_BUTTON_LEFT_STICK },
    { P4PC_PAD_R3, SDL_GAMEPAD_BUTTON_RIGHT_STICK },
    { P4PC_PAD_UP, SDL_GAMEPAD_BUTTON_DPAD_UP },
    { P4PC_PAD_DOWN, SDL_GAMEPAD_BUTTON_DPAD_DOWN },
    { P4PC_PAD_LEFT, SDL_GAMEPAD_BUTTON_DPAD_LEFT },
    { P4PC_PAD_RIGHT, SDL_GAMEPAD_BUTTON_DPAD_RIGHT },
    { P4PC_PAD_L2, SDL_GAMEPAD_BUTTON_INVALID },
    { P4PC_PAD_R2, SDL_GAMEPAD_BUTTON_INVALID },
};

static int sStickDead = P4PC_STICK_DEAD_PERCENT * 32767 / 100; /* raw axis units */

/* PCSX2 PadDualshock2::Set: default AxisScale 1.33, saturating each half-axis
 * independently, then MERGE around 127. Never renormalize the pair to a circle:
 * P4 discards small components before its 125/128 run threshold, so that loses
 * full travel at some headings. SDL already uses the PS2 Y convention. */
static uint8_t stickByte(int axis)
{
    float value = (float)axis / (axis < 0 ? -32768.0f : 32767.0f);
    value = value * 1.33f * 255.0f;
    unsigned half = (unsigned)(value > 255.0f ? 255.0f : value);
    return (uint8_t)(axis > 0 ? 127u + (half + 1u) / 2u : 127u - half / 2u);
}

void p4pc_pad_stick(int x, int y, int dead, uint8_t *ox, uint8_t *oy)
{
    *ox = *oy = 0x7F;
    if ((double)x * x + (double)y * y > (double)dead * dead) {
        const unsigned csr = p4pc_fpu_host_enter(); /* PCSX2 host mapping, not EE arithmetic */
        *ox = stickByte(x);
        *oy = stickByte(y);
        p4pc_fpu_host_leave(csr);
    }
}

static void readGamepad(P4PadSnapshot *out)
{
    Sint16 lt, rt;
    size_t i;
    uint8_t x, y;

    if (sGamepad == NULL) {
        return;
    }
    for (i = 0; i < sizeof(kPad) / sizeof(kPad[0]); i++) {
        if (kPad[i].button != SDL_GAMEPAD_BUTTON_INVALID && SDL_GetGamepadButton(sGamepad, kPad[i].button)) {
            setBit(out, kPad[i].bit);
        }
    }

    lt = SDL_GetGamepadAxis(sGamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    rt = SDL_GetGamepadAxis(sGamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
    if (lt > sStickDead) {
        out->buttons |= P4PC_PAD_L2;
        out->pressure[P4PC_PRESS_L2] = triggerByte(lt);
    }
    if (rt > sStickDead) {
        out->buttons |= P4PC_PAD_R2;
        out->pressure[P4PC_PRESS_R2] = triggerByte(rt);
    }

    p4pc_pad_stick(SDL_GetGamepadAxis(sGamepad, SDL_GAMEPAD_AXIS_LEFTX), SDL_GetGamepadAxis(sGamepad, SDL_GAMEPAD_AXIS_LEFTY), sStickDead, &x, &y);
    out->lx = x;
    out->ly = y;
    if (sSettingsMenuActive && sSettingsMenuActive()) {
        /* Config's stock list consumes digital directions; preserve the real
         * analog axes, and bridge only a deliberate left-stick movement. */
        int ax = SDL_GetGamepadAxis(sGamepad, SDL_GAMEPAD_AXIS_LEFTX);
        int ay = SDL_GetGamepadAxis(sGamepad, SDL_GAMEPAD_AXIS_LEFTY);
        int threshold = sStickDead > 16000 ? sStickDead : 16000;
        if (ax < -threshold) setBit(out, P4PC_PAD_LEFT);
        else if (ax > threshold) setBit(out, P4PC_PAD_RIGHT);
        if (ay < -threshold) setBit(out, P4PC_PAD_UP);
        else if (ay > threshold) setBit(out, P4PC_PAD_DOWN);
    }
    p4pc_pad_stick(SDL_GetGamepadAxis(sGamepad, SDL_GAMEPAD_AXIS_RIGHTX), SDL_GetGamepadAxis(sGamepad, SDL_GAMEPAD_AXIS_RIGHTY), sStickDead, &x, &y);
    out->rx = x;
    out->ry = y;
}

static uint16_t autoButtonBit(const char *name, size_t len);

/* The bindings of the settings file ([input]: kb.<button> = key names, pad.<button> = a gamepad button name or none; deadzone): over the defaults above. */
static void loadBindings(void)
{
    static const char *const kNames[] = { "up", "down", "left", "right", "cross", "circle", "triangle", "square", "start", "select", "l1", "r1", "l2", "r2", "l3", "r3" };
    size_t n;
    int pct;

    p4cfg_load();
    pct = p4cfg_int("input.deadzone", P4PC_STICK_DEAD_PERCENT);
    sStickDead = (pct < 0 ? 0 : (pct > 50 ? 50 : pct)) * 32767 / 100;
    for (n = 0; n < sizeof kNames / sizeof kNames[0]; n++) {
        const uint16_t bit = autoButtonBit(kNames[n], strlen(kNames[n]));
        char key[32];
        const char *v;
        size_t i;

        snprintf(key, sizeof key, "input.kb.%s", kNames[n]);
        v = p4cfg_get(key);
        for (i = 0; v != NULL && i < sizeof kKeys / sizeof kKeys[0]; i++) {
            if (kKeys[i].bit == bit) {
                SDL_Scancode keys[2] = { SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN };
                int count = 0;
                const char *p = v;

                while (*p != '\0' && count < 2) {
                    char tok[40];
                    const size_t len = strcspn(p, ",");
                    size_t from = 0, to = len < sizeof tok - 1 ? len : sizeof tok - 1;

                    while (from < to && (p[from] == ' ' || p[from] == '\t')) {
                        from++;
                    }
                    while (to > from && (p[to - 1] == ' ' || p[to - 1] == '\t')) {
                        to--;
                    }
                    memcpy(tok, p + from, to - from);
                    tok[to - from] = '\0';
                    p += len;
                    if (*p == ',') {
                        p++;
                    }
                    if (tok[0] == '\0' || SDL_strcasecmp(tok, "none") == 0) {
                        continue;
                    }
                    {
                        const SDL_Scancode sc = SDL_GetScancodeFromName(tok);

                        if (sc == SDL_SCANCODE_UNKNOWN) {
                            fprintf(stderr, "p4pc pad: %s: unknown key '%s'\n", key, tok);
                        } else {
                            keys[count++] = sc;
                        }
                    }
                }
                if (count > 0 || SDL_strcasecmp(v, "none") == 0) {
                    kKeys[i].key[0] = keys[0];
                    kKeys[i].key[1] = keys[1];
                }
            }
        }
        snprintf(key, sizeof key, "input.pad.%s", kNames[n]);
        v = p4cfg_get(key);
        for (i = 0; v != NULL && i < sizeof kPad / sizeof kPad[0]; i++) {
            if (kPad[i].bit == bit) {
                if (SDL_strcasecmp(v, "none") == 0) {
                    kPad[i].button = SDL_GAMEPAD_BUTTON_INVALID;
                } else {
                    const SDL_GamepadButton b = SDL_GetGamepadButtonFromString(v);

                    if (b == SDL_GAMEPAD_BUTTON_INVALID) {
                        fprintf(stderr, "p4pc pad: %s: unknown gamepad button '%s'\n", key, v);
                    } else {
                        kPad[i].button = b;
                    }
                }
            }
        }
    }
}

void p4pc_pad_reload_bindings(void)
{
    loadBindings();
}

/* P4PC_AUTOPRESS: scripted presses for unattended boots. A comma-separated list of
 * `button@start[:hold][/every[*count]]`, all in seconds since the first pad read (frame flips with
 * P4PC_AUTOPRESS_FRAMES=1): `button` is start,
 * select, cross, circle, triangle, square, l1, r1, l2, r2, l3, r3, up, down, left or right;
 * `hold` (default 0.3) is how long the button stays down; with `/every` the press repeats at
 * that period after `start` (`*count` limits it to that many presses). Example: P4PC_AUTOPRESS=start@12/4,cross@30:0.5. The held button
 * is ORed into the snapshot like a keyboard press (full pressure), so the game sees an edge
 * exactly as with a player. */
#define P4PC_AUTOPRESS_MAX 16

static P4PadAutoPress sAuto[P4PC_AUTOPRESS_MAX];
static int sAutoCount;
static int sAutoParsed;

static uint16_t autoButtonBit(const char *name, size_t len)
{
    static const struct { const char *name; uint16_t bit; } map[] = {
        { "start", P4PC_PAD_START }, { "select", P4PC_PAD_SELECT },
        { "cross", P4PC_PAD_CROSS }, { "circle", P4PC_PAD_CIRCLE },
        { "triangle", P4PC_PAD_TRIANGLE }, { "square", P4PC_PAD_SQUARE },
        { "l1", P4PC_PAD_L1 }, { "r1", P4PC_PAD_R1 }, { "l2", P4PC_PAD_L2 }, { "r2", P4PC_PAD_R2 },
        { "l3", P4PC_PAD_L3 }, { "r3", P4PC_PAD_R3 },
        { "up", P4PC_PAD_UP }, { "down", P4PC_PAD_DOWN },
        { "left", P4PC_PAD_LEFT }, { "right", P4PC_PAD_RIGHT },
    };
    size_t i;

    for (i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
        if (strlen(map[i].name) == len && strncmp(map[i].name, name, len) == 0) {
            return map[i].bit;
        }
    }
    return 0;
}

/* Parses a spec into `out`; returns the entry count, or -1 on a malformed entry (message on stderr). */
int p4pc_pad_autopress_parse(const char *spec, P4PadAutoPress *out, int max)
{
    int n = 0;
    const char *p = spec;

    while (p != NULL && *p != '\0') {
        const char *end = strchr(p, ',');
        size_t len = end != NULL ? (size_t)(end - p) : strlen(p);
        const char *at = memchr(p, '@', len);
        char *rest;
        P4PadAutoPress a;

        if (len == 0) {
            p = end != NULL ? end + 1 : p + len;
            continue;
        }
        if (at == NULL || n >= max) {
            fprintf(stderr, "p4pc: P4PC_AUTOPRESS: bad entry '%.*s'\n", (int)len, p);
            return -1;
        }
        a.bit = autoButtonBit(p, (size_t)(at - p));
        a.hold = 0.3;
        a.every = 0.0;
        a.count = 0;
        a.start = strtod(at + 1, &rest);
        if (a.bit == 0 || rest == at + 1) {
            fprintf(stderr, "p4pc: P4PC_AUTOPRESS: bad entry '%.*s'\n", (int)len, p);
            return -1;
        }
        if (*rest == ':') {
            a.hold = strtod(rest + 1, &rest);
        }
        if (*rest == '/') {
            a.every = strtod(rest + 1, &rest);
            if (*rest == '*') {
                a.count = (int)strtol(rest + 1, &rest, 10);
            }
        }
        if (rest > p + len || a.start < 0.0 || a.hold <= 0.0 || a.every < 0.0) {
            fprintf(stderr, "p4pc: P4PC_AUTOPRESS: bad entry '%.*s'\n", (int)len, p);
            return -1;
        }
        out[n++] = a;
        p = end != NULL ? end + 1 : p + len;
    }
    return n;
}

/* ORs every scripted press that is down at `now` seconds into the snapshot. */
void p4pc_pad_autopress_apply(const P4PadAutoPress *list, int count, double now, P4PadSnapshot *out)
{
    int i;

    for (i = 0; i < count; i++) {
        double t = now - list[i].start;

        if (t < 0.0) {
            continue;
        }
        if (list[i].every > 0.0) {
            if (list[i].count > 0 && t >= list[i].every * (double)list[i].count) {
                continue;
            }
            t = fmod(t, list[i].every);
        }
        if (t < list[i].hold) {
            setBit(out, list[i].bit);
        }
    }
}

static uint32_t (*sFrameClock)(void);

void p4pc_pad_autopress_set_clock(uint32_t (*frames)(void))
{
    sFrameClock = frames;
}

/* SDL-backed field-input fixture: START,X,Y,XJITTER,DPAD_UP,HZ.
 * Reports belong to the 30 Hz game flip, not a separately scheduled host thread. */
static SDL_Joystick *sVirtualPad;
static unsigned sVirtualStart;
static int sVirtualX, sVirtualY, sVirtualJitter, sVirtualDpad, sVirtualHz;
static uint32_t sVirtualFrame;
static int sVirtualHasFrame;
static uint16_t sVirtualBits;
static uint32_t sVirtualUntil;
static int sVirtualManualX, sVirtualManualY;

int p4pc_pad_virtual_press(unsigned bits, unsigned frames)
{
    if (sInputMode != 2 || !sVirtualPad || !sFrameClock ||
        !getenv("P4PC_SETTINGS_TEST") || sVirtualHz != 0) return -1;
    sVirtualBits = (uint16_t)bits;
    sVirtualManualX = sVirtualManualY = 0;
    sVirtualUntil = sFrameClock() + frames;
    return 0;
}
int p4pc_pad_virtual_stick(int x, int y, unsigned frames)
{
    if (x < -32768 || x > 32767 || y < -32768 || y > 32767 ||
        p4pc_pad_virtual_press(0, frames)) return -1;
    sVirtualManualX = x;
    sVirtualManualY = y;
    return 0;
}

static void virtualPadInit(void)
{
    const char *spec = getenv("P4PC_PAD_VIRTUAL");
    SDL_VirtualJoystickDesc desc;
    SDL_JoystickID id;
    if (spec == NULL) {
        if (sInputMode == 2) {
            fprintf(stderr, "p4pc: P4PC_INPUT=virtual requires P4PC_PAD_VIRTUAL\n");
            exit(2);
        }
        return;
    }
    if (sscanf(spec, "%u,%d,%d,%d,%d,%d", &sVirtualStart, &sVirtualX, &sVirtualY,
               &sVirtualJitter, &sVirtualDpad, &sVirtualHz) != 6 ||
        sVirtualX < -32768 || sVirtualX > 32767 || sVirtualY < -32768 || sVirtualY > 32767 ||
        sVirtualJitter < 0 || sVirtualJitter > 32767 || sVirtualHz < 0 || sVirtualHz > 1000) {
        fprintf(stderr, "p4pc: P4PC_PAD_VIRTUAL needs START,X,Y,XJITTER,DPAD_UP,HZ\n");
        exit(2);
    }
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.name = "P4PC input fixture";
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    id = SDL_AttachVirtualJoystick(&desc);
    sVirtualPad = id ? SDL_OpenJoystick(id) : NULL;
    if (sVirtualPad == NULL) {
        fprintf(stderr, "p4pc: virtual pad: %s\n", SDL_GetError());
        exit(2);
    }
    if (getenv("P4PC_SETTINGS_TEST")) {
        /* Raw virtual triggers idle at -32768, not half-pressed at zero. */
        SDL_SetJoystickVirtualAxis(sVirtualPad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, -32768);
        SDL_SetJoystickVirtualAxis(sVirtualPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, -32768);
    }
    openGamepad(id);
    /* HZ=0 leaves the SDL device under the adapter unit test's manual control. */
}

static void virtualPadUpdate(void)
{
    uint32_t frame;
    uint64_t begin = 0, end = 1;
    int active;
    if (sVirtualPad == NULL || sFrameClock == NULL ||
        (sVirtualHz == 0 && !getenv("P4PC_SETTINGS_TEST"))) return;
    frame = sFrameClock();
    if (sVirtualHasFrame && sVirtualFrame == frame) return;
    sVirtualFrame = frame;
    sVirtualHasFrame = 1;
    if (sVirtualHz == 0) {
        uint16_t bits = frame < sVirtualUntil ? sVirtualBits : 0;
        for (size_t i = 0; i < sizeof kPad / sizeof kPad[0]; ++i)
            if (kPad[i].button != SDL_GAMEPAD_BUTTON_INVALID)
                SDL_SetJoystickVirtualButton(sVirtualPad, kPad[i].button, (bits & kPad[i].bit) != 0);
        SDL_SetJoystickVirtualAxis(sVirtualPad, SDL_GAMEPAD_AXIS_LEFTX, frame < sVirtualUntil ? sVirtualManualX : 0);
        SDL_SetJoystickVirtualAxis(sVirtualPad, SDL_GAMEPAD_AXIS_LEFTY, frame < sVirtualUntil ? sVirtualManualY : 0);
        SDL_UpdateJoysticks();
        return;
    }
    active = frame >= sVirtualStart;
    if (active) {
        uint64_t elapsed = (uint64_t)frame - sVirtualStart;
        begin = elapsed * (unsigned)sVirtualHz / 30u;
        end = (elapsed + 1u) * (unsigned)sVirtualHz / 30u;
        if (end == begin) ++end; /* Activate at START even below one report/flip. */
    }
    for (uint64_t sample = begin; sample < end; ++sample) {
        int x = sVirtualX + ((int)(sample % 3u) - 1) * sVirtualJitter;
        x = x < -32768 ? -32768 : x > 32767 ? 32767 : x;
        SDL_SetJoystickVirtualAxis(sVirtualPad, SDL_GAMEPAD_AXIS_LEFTX, active ? (Sint16)x : 0);
        SDL_SetJoystickVirtualAxis(sVirtualPad, SDL_GAMEPAD_AXIS_LEFTY, active ? (Sint16)sVirtualY : 0);
        SDL_SetJoystickVirtualButton(sVirtualPad, SDL_GAMEPAD_BUTTON_DPAD_UP, active && sVirtualDpad);
        /* Publish through SDL before any game/settings reader, even when the
         * ordinary window event pump is inside its 2 ms throttle. */
        SDL_UpdateJoysticks();
    }
}

/* A press injected by a harness (src/platform/menu_harness.c): the buttons in `bits` (P4PC_PAD_* mask) are down for `frames` flips from now. */
static uint16_t sInjectBits;
static uint32_t sInjectUntil;

void p4pc_pad_inject(unsigned bits, unsigned frames)
{
    sInjectBits = (uint16_t)bits;
    sInjectUntil = (sFrameClock != NULL ? sFrameClock() : 0u) + frames;
}

static int sOriginSet;
/* 1 when no title driver (P4PC_AUTOPRESS_NEWGAME / P4PC_AUTOPRESS_LOAD) holds the scripts back until its menu choice is confirmed. */
static int titleDriverOff(void)
{
    return getenv("P4PC_AUTOPRESS_NEWGAME") == NULL && getenv("P4PC_AUTOPRESS_LOAD") == NULL;
}
static double sOriginShift;   /* script clock = flips + sOriginShift */

void p4pc_pad_autopress_set_origin(unsigned flipAtT, double T)
{
    sOriginShift = T - (double)flipAtT;
    sOriginSet = 1;
}

/* P4PC_WALK_LOG=1 prints the player's position and facing every 30 flips. P4PC_WALK_SCRIPT=FILE: a waypoint script (include/p4pc/walk_script.h) that steers the field player from guest memory, the same file and the
 * same semantics as retail's `p4hook --walk` (the PCSX2 hook). Stepped once per flip; its buttons are ORed into the snapshot. It starts with
 * the AUTOPRESS clock (after the NEW GAME confirm when P4PC_AUTOPRESS_NEWGAME is set). A failed script logs the position and exits with 3. */
static P4Walk *sWalk;
static int sWalkParsed;
static uint32_t sWalkFrame = 0xFFFFFFFFu, sWalkMask;

int p4pc_pad_walk_status(void)
{
    return sWalk ? p4walk_status(sWalk) : -2;
}

/* P4PC_WALK_LOG_EVERY=<flips>: how often P4PC_WALK_LOG prints (default 30). */
static unsigned walkLogEvery(void)
{
    static unsigned every;

    if (every == 0) {
        const char *e = getenv("P4PC_WALK_LOG_EVERY");

        every = e != NULL && atoi(e) > 0 ? (unsigned)atoi(e) : 30u;
    }
    return every;
}

/* P4PC_WALK_DIGEST=1: a rolling FNV-1a over the game's state as a walk script sees it, printed every 500 flips as `p4pc: walk digest flip N hhhhhhhh`:
 * every 10 flips the player's matrix when there is one (16 words: where it stands and faces) and the Mersenne Twister array of RpRandom (0x00885A90, 624 words). The same script gives the same digest on every target and build (tests/walk_digest_gate.sh): a float or library
 * difference in the game's arithmetic changes the matrix within a few flips (docs/platform/numeric_determinism.md). */
static void walkDigest(const P4WalkHost *host, uint32_t frame)
{
    static int on = -1;
    static uint32_t h = 2166136261u;
    static uint32_t words;

    if (on < 0) {
        on = getenv("P4PC_WALK_DIGEST") != NULL;
        #ifdef P4_MOD_GAME_BINDINGS
        if (on) p4_mod_fidelity_check_start();
        #endif
    }
    if (!on) {
        return;
    }
    if (frame % 10u == 0) {
        const uint32_t m = p4walk_player_matrix(host);
        uint32_t k;

        if (m != 0) {
            for (k = 0; k < 16; k++) {
                const uint32_t v = host->read32(host->user, m + k * 4u);
                int b;

                for (b = 0; b < 4; b++) {
                    h = (h ^ ((v >> (8 * b)) & 255u)) * 16777619u;
                }
            }
            words++;
        }
        for (k = 0; k < 0x270u; k++) {
            const uint32_t v = host->read32(host->user, 0x00885A90u + k * 4u);
            int b;

            for (b = 0; b < 4; b++) {
                h = (h ^ ((v >> (8 * b)) & 255u)) * 16777619u;
            }
        }
    }
    if (frame % 500u == 0) {
        fprintf(stderr, "p4pc: walk digest flip %u %08x (%u samples)\n", frame, h, words);
    }
}

/* The walker's collision tests (src/platform/nav_probe.c registers them at start-up; programs that link only the pad have none). */
static int (*sNavBlocked)(void *user, float x, float y, float z, float radius);
static int (*sNavGround)(void *user, float x, float z, float y_hi, float y_lo, float *y);
static int (*sNavReady)(void *user);

void p4pc_pad_set_nav(int (*blocked)(void *, float, float, float, float), int (*ground)(void *, float, float, float, float, float *), int (*ready)(void *))
{
    sNavBlocked = blocked;
    sNavGround = ground;
    sNavReady = ready;
}

static uint32_t walkRead32(void *user, uint32_t addr)
{
    (void)user;
    if (addr < 0x00100000u || addr > 0x01FFFFFCu) {
        return 0;
    }
    return *(volatile uint32_t *)P4_G2H(addr);
}

static void walkLog(void *user, const char *message)
{
    (void)user;
    fprintf(stderr, "p4pc: %s\n", message);
}

static uint16_t walkSnapshotBit(int b)
{
    static const uint16_t bits[P4W_BUTTON_COUNT] = {
        P4PC_PAD_UP, P4PC_PAD_DOWN, P4PC_PAD_LEFT, P4PC_PAD_RIGHT, P4PC_PAD_CROSS, P4PC_PAD_CIRCLE, P4PC_PAD_TRIANGLE, P4PC_PAD_SQUARE,
        P4PC_PAD_START, P4PC_PAD_SELECT, P4PC_PAD_L1, P4PC_PAD_R1, P4PC_PAD_L2, P4PC_PAD_R2, P4PC_PAD_L3, P4PC_PAD_R3,
    };
    return bits[b];
}

/* The battle driver (src/platform/battle_driver.c) registers a gate that is true while a plan owns the pad in a fight; the walk script's own presses are
 * held back then. A hook, so that the pad code links without the driver (tests/pad_keyboard_test). */
static int (*sWalkHold)(void);

void p4pc_pad_set_walk_hold(int (*hold)(void))
{
    sWalkHold = hold;
}

static void walkScriptRead(P4PadSnapshot *out)
{
    if (!sWalkParsed) {
        const char *path = getenv("P4PC_WALK_SCRIPT");

        sWalkParsed = 1;
        if (path != NULL && path[0] != '\0') {
            FILE *f = fopen(path, "rb");
            char *text;
            char err[200];
            size_t n;

            if (f == NULL) {
                fprintf(stderr, "p4pc: P4PC_WALK_SCRIPT: cannot open %s\n", path);
                exit(2);
            }
            text = (char *)malloc(1u << 20);
            n = fread(text, 1, (1u << 20) - 1, f);
            fclose(f);
            text[n] = '\0';
            sWalk = p4walk_parse(text, err, sizeof err);
            free(text);
            if (sWalk == NULL) {
                fprintf(stderr, "p4pc: P4PC_WALK_SCRIPT %s: %s\n", path, err);
                exit(2);
            }
        }
    }
    if (sFrameClock != NULL && getenv("P4PC_DUMP_CHECKS") != NULL) {
        static const P4WalkHost checksHost = { walkRead32, walkLog, NULL };
        static uint32_t checksFrame = 0xFFFFFFFFu;
        uint32_t frame = sFrameClock();

        if (frame != checksFrame) {
            checksFrame = frame;
            p4walk_checks_frame(&checksHost, frame);
        }
    }
    if (sWalk != NULL && sFrameClock != NULL && (sOriginSet || titleDriverOff())) {
        uint32_t frame = sFrameClock();

        if (frame != sWalkFrame) {
            const P4WalkHost host = { walkRead32, walkLog, NULL, sNavBlocked, sNavGround };

            sWalkFrame = frame;
            /* flips by default (the scene gates' schedules are calibrated in flips: under P4PC_TURBO=2 a flip runs two game frames); P4PC_WALK_GAME=1 counts the
             * game's own frame counter, the unit of p4hook --walk-game */
            static int gameFrames = -1;

            if (gameFrames < 0) {
                gameFrames = getenv("P4PC_WALK_GAME") != NULL;
            }
            /* A surviving rotate-pc task is not a live collision world. Pause
             * field input across unload/load, without caching absent geometry
             * as permanent NAV_VOID cells. Title/card input has no player and
             * must keep running before a scene exists. */
            if (sNavReady != NULL && !sNavReady(host.user) && p4walk_player_matrix(&host) != 0) {
                sWalkMask = 0;
            } else {
                sWalkMask = gameFrames ? p4walk_tick(sWalk, &host) : p4walk_frame(sWalk, &host, frame);
            }
            walkDigest(&host, frame);
            if (frame % walkLogEvery() == 0) {
                static int logPos = -1;

                if (logPos < 0) {
                    logPos = getenv("P4PC_WALK_LOG") != NULL;
                }
                if (logPos) {
                    float x, z, fx, fz;

                    if (p4walk_player(&host, &x, &z, &fx, &fz)) {
                        fprintf(stderr, "p4pc: walk pos flip %u x %.1f z %.1f face %.0f\n", frame, x, z, atan2((double)fx, (double)fz) * 57.29578);
                        if (getenv("P4PC_WALK_LOG_M") != NULL) {
                            uint32_t m = p4walk_player_matrix(&host);
                            int k;

                            fprintf(stderr, "p4pc: walk matrix %08x:", (unsigned)m);
                            for (k = 0; k < 16; k++) {
                                fprintf(stderr, " %08x", (unsigned)walkRead32(NULL, m + (uint32_t)k * 4u));
                            }
                            fprintf(stderr, "\n");
                        }
                    }
                }
                {
                    static const char *peek = (const char *)-1;   /* P4PC_WALK_PEEK=0xADDR,0xADDR...: guest words printed with the position */

                    if (peek == (const char *)-1) {
                        peek = getenv("P4PC_WALK_PEEK");
                    }
                    if (logPos && peek != NULL && *peek != '\0') {
                        const char *p = peek;
                        char *end;

                        fprintf(stderr, "p4pc: walk peek flip %u", frame);
                        while (*p != '\0') {
                            unsigned long a = strtoul(p, &end, 0);

                            if (end == p) {
                                break;
                            }
                            fprintf(stderr, " %08lx=%08x", a, (unsigned)walkRead32(NULL, (uint32_t)a));
                            p = *end == ',' ? end + 1 : end;
                        }
                        fprintf(stderr, "\n");
                    }
                }
            }
            if (p4walk_status(sWalk) == P4WALK_FAILED) {
                exit(3);
            }
        }
        {
            int b;

            for (b = 0; b < P4W_BUTTON_COUNT && !(sWalkHold != NULL && sWalkHold()); b++) {
                if (sWalkMask & (1u << b)) {
                    setBit(out, walkSnapshotBit(b));
                }
            }
        }
    }
}

static void autoPressRead(P4PadSnapshot *out)
{
    walkScriptRead(out);

    static uint64_t t0;

    if (sInjectBits != 0 && sFrameClock != NULL && sFrameClock() < sInjectUntil) {
        unsigned b;

        for (b = 0; b < 16; b++) {
            if (sInjectBits & (1u << b)) {
                setBit(out, (uint16_t)(1u << b));
            }
        }
    }

    if (!sAutoParsed) {
        const char *spec = getenv("P4PC_AUTOPRESS");
        int n = spec != NULL ? p4pc_pad_autopress_parse(spec, sAuto, P4PC_AUTOPRESS_MAX) : 0;

        sAutoCount = n > 0 ? n : 0;
        sAutoParsed = 1;
        t0 = SDL_GetTicks();
    }
    if (sAutoCount > 0 && (sOriginSet || titleDriverOff())) {
        /* P4PC_AUTOPRESS_FRAMES=1: the times in the spec are frame flips (p4pc_flip_count), not seconds, so a
         * script means the same on a fast or a slow host. */
        static int frames = -1;

        if (frames < 0) {
            const char *e = getenv("P4PC_AUTOPRESS_FRAMES");

            frames = e != NULL && e[0] == '1';
        }
        p4pc_pad_autopress_apply(sAuto, sAutoCount,
                                 frames && sFrameClock != NULL ? (double)sFrameClock() + sOriginShift : (double)(SDL_GetTicks() - t0) / 1000.0, out);
    }
}

void p4pc_pad_keys_to_snapshot(const bool *keys, int numkeys, P4PadSnapshot *out)
{
    size_t i;
    int k;

    for (i = 0; i < sizeof(kKeys) / sizeof(kKeys[0]); i++) {
        for (k = 0; k < 2; k++) {
            SDL_Scancode sc = kKeys[i].key[k];
            if (sc != SDL_SCANCODE_UNKNOWN && sc < numkeys && keys[sc]) {
                setBit(out, kKeys[i].bit);
                break;
            }
        }
    }
}

void p4pc_pad_sdl_read(P4PadSnapshot *out)
{
    const bool *keys;
    int numkeys = 0;

    memset(out, 0, sizeof(*out));
    out->lx = out->ly = out->rx = out->ry = 0x80;
    autoPressRead(out);
    if (sInitState <= 0) {
        return;
    }

    virtualPadUpdate();
    p4_settings_pump();
    if ((sSettingsActive && sSettingsActive()) || sSettingsFence) {
        int held = 0;
        if (sInputMode == 0) {
            keys = SDL_GetKeyboardState(&numkeys);
            for (int i = 0; i < numkeys; i++) held |= keys[i];
        }
        if ((sInputMode == 0 || sInputMode == 2) && sGamepad) {
            for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; i++)
                held |= SDL_GetGamepadButton(sGamepad, (SDL_GamepadButton)i);
            for (int i = 0; i < SDL_GAMEPAD_AXIS_COUNT; i++) {
                const int axis = SDL_GetGamepadAxis(sGamepad, (SDL_GamepadAxis)i);
                held |= i == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || i == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER
                    ? axis > 16000 : axis < -16000 || axis > 16000;
            }
        }
        if (!held && !(sSettingsActive && sSettingsActive())) sSettingsFence = 0;
        memset(out, 0, sizeof *out);
        out->lx = out->ly = out->rx = out->ry = 0x80;
        return;
    }

    if (sInputMode == 0) {
        keys = SDL_GetKeyboardState(&numkeys);
        p4pc_pad_keys_to_snapshot(keys, numkeys, out);
    }
    if (sInputMode != 1) readGamepad(out);

    /* Keyboard left stick: the D-pad, used only when the gamepad stick is idle. */
    if (out->lx == 0x80 || out->lx == 0x7F) {
        if (out->buttons & P4PC_PAD_LEFT) {
            out->lx = 0x00;
        } else if (out->buttons & P4PC_PAD_RIGHT) {
            out->lx = 0xFF;
        }
    }
    if (out->ly == 0x80 || out->ly == 0x7F) {
        if (out->buttons & P4PC_PAD_UP) {
            out->ly = 0x00;
        } else if (out->buttons & P4PC_PAD_DOWN) {
            out->ly = 0xFF;
        }
    }
}

/* The actual libpad packet and the game's previous movement update, once per game
 * frame (not per host pump). Raw axes/buttons come from the same polled SDL state. */
void p4pc_pad_trace_packet(const uint8_t *packet)
{
    static int checked;
    static FILE *file;
    static uint32_t last = 0xFFFFFFFFu;
    uint32_t frame, buttons = 0, matrix;
    float x = 0, z = 0, fx, fz, speed;
    const P4WalkHost host = { walkRead32, NULL, NULL };
    if (!checked) {
        const char *path = getenv("P4PC_PAD_TRACE");
        checked = 1;
        if (path != NULL) {
            file = fopen(path, "w");
            if (file == NULL) { perror("P4PC_PAD_TRACE"); exit(2); }
            fprintf(file, "flip,frame,ns,raw_lx,raw_ly,raw_rx,raw_ry,raw_lt,raw_rt,raw_buttons,mode,buttons,rx,ry,lx,ly,game_buttons,game_axes,speed,event,matrix,x,z\n");
        }
    }
    if (file == NULL || sFrameClock == NULL) return;
    frame = walkRead32(NULL, 0x00764290u);
    if (frame == last) return;
    last = frame;
    if (sGamepad != NULL) {
        for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; i++)
            if (SDL_GetGamepadButton(sGamepad, (SDL_GamepadButton)i)) buttons |= 1u << i;
    }
    matrix = p4walk_player_matrix(&host);
    p4walk_player(&host, &x, &z, &fx, &fz);
    { uint32_t word = walkRead32(NULL, 0x007643DCu); memcpy(&speed, &word, 4); }
    fprintf(file, "%u,%u,%llu", sFrameClock(), frame, (unsigned long long)p4_os_now_ns());
    for (int i = 0; i < SDL_GAMEPAD_AXIS_COUNT; i++)
        fprintf(file, ",%d", sGamepad ? SDL_GetGamepadAxis(sGamepad, (SDL_GamepadAxis)i) : 0);
    fprintf(file, ",%u,%u,%u,%u,%u,%u,%u,%u,%u,%.9g,%u,%u,%.9g,%.9g\n",
            buttons, packet[1], (unsigned)packet[2] | (unsigned)packet[3] << 8,
            packet[4], packet[5], packet[6], packet[7],
            walkRead32(NULL, 0x008C024Cu), walkRead32(NULL, 0x008C025Cu),
            speed, walkRead32(NULL, 0x00764604u), matrix, x, z);
    fflush(file);
}
