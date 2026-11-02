/* SDL virtual fixture -> PS2 layout, PCSX2 stick quantization, trigger pressure,
 * and foreign-hotplug isolation. Never opens a physical controller or reads keys.
 * Needs a hidden GL window; exits 77 where that cannot be created. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "../src/platform/pad/pad_sdl.h"
#include "p4pc/os.h"

static int failures;
#define CHECK(c)                                                         \
    do {                                                                 \
        if (!(c)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                  \
        }                                                                \
    } while (0)

static uint32_t fixtureFrame;

static uint32_t frameClock(void) { return fixtureFrame; }

static void checkFrame(SDL_Joystick *pad, uint32_t frame)
{
    P4PadSnapshot snap;
    int active = frame >= 3000u;
    int x = 0;
    fixtureFrame = frame;
    if (active) {
        uint64_t report = ((uint64_t)frame - 3000u + 1u) * 1000u / 30u - 1u;
        x = ((int)(report % 3u) - 1) * 2000;
    }
    /* No sleeping/event-pump grace: the first read at START must be current,
     * and all additional reads of this frame must see the same SDL state. */
    for (int read = 0; read < 8; ++read) {
        p4pc_pad_sdl_read(&snap);
        CHECK(SDL_GetJoystickAxis(pad, SDL_GAMEPAD_AXIS_LEFTX) == x);
        CHECK(SDL_GetJoystickAxis(pad, SDL_GAMEPAD_AXIS_LEFTY) == (active ? -32768 : 0));
        CHECK(snap.buttons == 0 && snap.ly == (active ? 0 : 127));
    }
}

static SDL_Joystick *attach(void)
{
    SDL_VirtualJoystickDesc desc;
    SDL_JoystickID id;

    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    id = SDL_AttachVirtualJoystick(&desc);
    return id != 0 ? SDL_OpenJoystick(id) : NULL;
}

/* One read after the event pump has run (the pad layer asks the window system at most every 2 ms). */
static void readPad(P4PadSnapshot *snap)
{
    p4_os_usleep(5000);
    p4pc_pad_sdl_read(snap);
    p4_os_usleep(5000);
    p4pc_pad_sdl_read(snap);
}

int main(int argc, char **argv)
{
    P4PadSnapshot snap;
    SDL_Joystick *a, *b;
    int framed = argc > 1 && strcmp(argv[1], "--frame-clocked") == 0;

    p4_os_setenv("SDL_VIDEODRIVER", "offscreen", 0);
    p4_os_setenv("P4PC_HIDDEN_WINDOW", "1", 0);
    p4_os_setenv("P4PC_INPUT", "virtual", 1);
    p4_os_setenv("P4PC_PAD_VIRTUAL", framed ? "3000,0,-32768,2000,0,1000" : "0,0,0,0,0,0", 1);
    if (framed) p4pc_pad_autopress_set_clock(frameClock);
    /* The hidden window never has the keyboard focus, and SDL drops a gamepad's input then (what a player wants of a game in the background). */
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (p4pc_pad_sdl_init() != 0) {
        printf("pad_gamepad_test: no window (%s), skipped\n", SDL_GetError());
        return 77;
    }
    readPad(&snap);
    CHECK(snap.buttons == 0 && snap.lx == 0x7F && snap.ly == 0x7F);

    /* Only the ID created by the pad layer is eligible, even if hardware is connected. */
    a = NULL;
    {
        int count;
        SDL_JoystickID *ids = SDL_GetJoysticks(&count);
        for (int i = 0; ids != NULL && i < count; i++) {
            const char *name = SDL_GetJoystickNameForID(ids[i]);
            if (SDL_IsJoystickVirtual(ids[i]) && name && strcmp(name, "P4PC input fixture") == 0)
                a = SDL_OpenJoystick(ids[i]);
        }
        SDL_free(ids);
    }
    if (a == NULL) {
        printf("pad_gamepad_test: no virtual joystick (%s), skipped\n", SDL_GetError());
        return 77;
    }
    if (framed) {
        checkFrame(a, 2999);
        checkFrame(a, 3000);
        for (uint32_t frame = 3100; frame <= 4100; ++frame) checkFrame(a, frame);
        p4_os_usleep(10000);
        checkFrame(a, 4100); /* wall time cannot advance a held game frame */
        checkFrame(a, 2999);
        checkFrame(a, 3000);
        if (!failures) printf("pad_gamepad_frame_clock: exact START and stable per-frame SDL reports: PASS\n");
        return failures != 0;
    }
    readPad(&snap);
    CHECK(snap.buttons == 0);
    SDL_SetJoystickVirtualButton(a, SDL_GAMEPAD_BUTTON_SOUTH, true);
    readPad(&snap);
    CHECK(snap.buttons == P4PC_PAD_CROSS && snap.pressure[P4PC_PRESS_CROSS] == 0xFF);
    SDL_SetJoystickVirtualButton(a, SDL_GAMEPAD_BUTTON_SOUTH, false);
    SDL_SetJoystickVirtualButton(a, SDL_GAMEPAD_BUTTON_EAST, true);
    SDL_SetJoystickVirtualButton(a, SDL_GAMEPAD_BUTTON_WEST, true);
    SDL_SetJoystickVirtualButton(a, SDL_GAMEPAD_BUTTON_NORTH, true);
    SDL_SetJoystickVirtualButton(a, SDL_GAMEPAD_BUTTON_DPAD_UP, true);
    SDL_SetJoystickVirtualButton(a, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, true);
    SDL_SetJoystickVirtualButton(a, SDL_GAMEPAD_BUTTON_START, true);
    readPad(&snap);
    CHECK(snap.buttons == (P4PC_PAD_CIRCLE | P4PC_PAD_SQUARE | P4PC_PAD_TRIANGLE | P4PC_PAD_UP | P4PC_PAD_L1 | P4PC_PAD_START));
    for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; i++) {
        SDL_SetJoystickVirtualButton(a, i, false);
    }

    /* Small motion and saturated endpoints under PCSX2 sensitivity; trigger pressure. */
    SDL_SetJoystickVirtualAxis(a, SDL_GAMEPAD_AXIS_LEFTX, 3000);
    SDL_SetJoystickVirtualAxis(a, SDL_GAMEPAD_AXIS_LEFTY, -3000);
    readPad(&snap);
    CHECK(snap.lx > 0x7F && snap.ly < 0x7F); /* PCSX2 default: no host dead zone; P4 applies its own */
    SDL_SetJoystickVirtualAxis(a, SDL_GAMEPAD_AXIS_LEFTX, 32767);
    SDL_SetJoystickVirtualAxis(a, SDL_GAMEPAD_AXIS_LEFTY, 0);
    SDL_SetJoystickVirtualAxis(a, SDL_GAMEPAD_AXIS_RIGHTY, -32768);
    SDL_SetJoystickVirtualAxis(a, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 32767);
    readPad(&snap);
    CHECK(snap.lx == 0xFF && snap.ly == 0x7F && snap.ry == 0x00 && snap.rx == 0x7F);
    CHECK((snap.buttons & P4PC_PAD_R2) != 0 && snap.pressure[P4PC_PRESS_R2] == 0xFF && (snap.buttons & P4PC_PAD_L2) == 0);
    SDL_SetJoystickVirtualAxis(a, SDL_GAMEPAD_AXIS_LEFTX, 0);
    SDL_SetJoystickVirtualAxis(a, SDL_GAMEPAD_AXIS_RIGHTY, 0);
    SDL_SetJoystickVirtualAxis(a, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, -32768); /* a trigger at rest is the bottom of the raw range */

    /* A foreign pad is never selected, including after the owned fixture disconnects. */
    b = attach();
    CHECK(b != NULL);
    readPad(&snap);
    SDL_SetJoystickVirtualButton(b, SDL_GAMEPAD_BUTTON_NORTH, true);
    readPad(&snap);
    CHECK(snap.buttons == 0);
    SDL_DetachVirtualJoystick(SDL_GetJoystickID(a));
    SDL_CloseJoystick(a);
    readPad(&snap);
    CHECK(snap.buttons == 0);
    SDL_DetachVirtualJoystick(SDL_GetJoystickID(b));
    SDL_CloseJoystick(b);
    readPad(&snap);
    CHECK(snap.buttons == 0 && snap.lx == 0x80); /* no gamepad: keyboard snapshot's neutral */

    if (failures == 0) {
        printf("pad_gamepad_test: ok\n");
    }
    return failures != 0;
}
