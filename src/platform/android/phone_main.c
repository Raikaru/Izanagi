/* A separate adb-shell game runner: no Activity, application data or physical input. */
#define SDL_MAIN_HANDLED 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

int p4pc_main(int argc, char **argv);

/* The regular Android implementation also suppresses keyboard watching for
 * script input. This executable never creates an Activity. */
void p4_settings_android_keyboard_watch(int enabled)
{
    (void)enabled;
}

/* There is no Activity to hand a restart to; the normal error path handles it. */
int p4_android_prepare_restart(void)
{
    return -1;
}

int main(int argc, char **argv)
{
    const char *input = getenv("P4PC_INPUT");
    if (!input || strcmp(input, "script") != 0) {
        fputs("p4pc phone: script input is required\n", stderr);
        return 1;
    }
    if (setenv("SDL_VIDEODRIVER", "offscreen", 1) != 0 ||
        setenv("SDL_AUDIODRIVER", "dummy", 1) != 0 ||
        setpriority(PRIO_PROCESS, 0, 15) != 0 || getpriority(PRIO_PROCESS, 0) != 15) {
        perror("p4pc phone: cannot configure headless low-priority execution");
        return 1;
    }
    SDL_SetMainReady();
    return p4pc_main(argc, argv);
}
