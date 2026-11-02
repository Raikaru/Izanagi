/* Script mode never initializes gamepads. Virtual mode opens only its own fixture.
 * Even a second virtual pad masquerading as a physical hotplug must be ignored. */
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include "../src/platform/pad/pad_sdl.h"
#include "p4pc/os.h"
#include "p4pc/lockstep.h"

static uint32_t clockFrame(void) { return 1; }
static unsigned markKeys;
static void captureControl(int scancode)
{
    if (scancode == SDL_SCANCODE_F9) ++markKeys;
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "script";
    int virtualMode = strcmp(mode, "virtual") == 0;
    SDL_Joystick *foreign[2];
    SDL_VirtualJoystickDesc desc;
    P4PadSnapshot snap;
    SDL_WindowFlags before;
    SDL_Event event;
    p4_os_setenv("P4PC_INPUT", mode, 1);
    p4_os_setenv("P4PC_PAD_VIRTUAL", "10000,0,0,0,0,1000", 1);
    p4_os_setenv("P4PC_HIDDEN_WINDOW", "1", 1);
    p4_os_setenv("P4PC_RESOLUTION", "scale:2", 1); /* fullscreen hotkey would be enabled in player mode */
    p4_os_setenv("SDL_VIDEODRIVER", "offscreen", 0);
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    p4pc_pad_autopress_set_clock(clockFrame);
    if (p4pc_pad_sdl_init() != 0) return 77;
    if (!virtualMode && (SDL_WasInit(SDL_INIT_GAMEPAD) & SDL_INIT_GAMEPAD)) {
        fprintf(stderr, "script mode initialized the gamepad subsystem\n");
        return 1;
    }
    /* A test producer may initialize it now; script mode must still ignore its events. */
    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) return 1;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    for (int n = 0; n < 2; n++) {
        SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
        foreign[n] = id ? SDL_OpenJoystick(id) : NULL;
        if (!foreign[n]) return 1;
        for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++)
            SDL_SetJoystickVirtualButton(foreign[n], b, true);
        SDL_SetJoystickVirtualAxis(foreign[n], SDL_GAMEPAD_AXIS_LEFTX, 32767);
        SDL_SetJoystickVirtualAxis(foreign[n], SDL_GAMEPAD_AXIS_LEFTY, -32768);
    }
    before = SDL_GetWindowFlags(p4pc_pad_sdl_window());
    p4pc_pad_set_key_hook(captureControl);
    SDL_zero(event);
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.scancode = SDL_SCANCODE_F11;
    event.key.down = true;
    SDL_PushEvent(&event);
    event.key.scancode = SDL_SCANCODE_F9;
    SDL_PushEvent(&event);
    event.key.repeat = true;
    SDL_PushEvent(&event);
    for (int i = 0; i < 20; i++) {
        p4_os_usleep(5000);
        p4pc_pad_sdl_read(&snap);
        unsigned neutral = virtualMode ? 127 : 128;
        if (snap.buttons || snap.lx != neutral || snap.ly != neutral ||
            SDL_GetWindowFlags(p4pc_pad_sdl_window()) != before) {
            fprintf(stderr, "%s consumed foreign input: buttons=%04x lx=%u ly=%u\n", mode, snap.buttons, snap.lx, snap.ly);
            return 1;
        }
    }
    if (markKeys != 1) {
        fprintf(stderr, "%s capture control callbacks: %u, expected one non-repeat F9\n", mode, markKeys);
        return 1;
    }
    p4pc_pad_set_key_hook(NULL);
    event.key.repeat = false;
    SDL_PushEvent(&event);
    p4_os_usleep(5000);
    p4pc_pad_sdl_read(&snap);
    if (markKeys != 1) return 1;
    /* Scripted presses must still reach both the digital packet and analog synthesis. */
    p4pc_pad_inject(P4PC_PAD_UP, 10);
    p4pc_pad_sdl_read(&snap);
    if (snap.buttons != P4PC_PAD_UP || snap.ly != 0) return 1;
    for (int n = 0; n < 2; n++) {
        SDL_DetachVirtualJoystick(SDL_GetJoystickID(foreign[n]));
        SDL_CloseJoystick(foreign[n]);
    }
    printf("pad_isolation_test: %s ignores foreign game input; capture controls and scripted input work: PASS\n", mode);
    return 0;
}
