/* What the Android test executables need and only the Android game executables define (cmake/sources/350-port64-player.cmake,
 * P4_ANDROID_PHONE_TESTS; the same stubs as src/platform/android/phone_main.c, which is itself an executable). A static library, so a
 * test that defines one of these itself keeps its own. */
#include <errno.h>

/* SDL3 on Android refuses to start when the process's main is not SDL's: the test executables are plain mains, as the headless game
 * runner (phone_main.c) is, and say so here before any test code runs. Weak, so an executable without SDL has nothing to resolve. */
extern void SDL_SetMainReady(void) __attribute__((weak));
__attribute__((constructor)) static void p4_phone_tests_main_ready(void) { if (SDL_SetMainReady) SDL_SetMainReady(); }

void p4_settings_android_keyboard_watch(int enabled) { (void)enabled; }
void p4_android_pad_layout_pump(void) { }
void p4_android_edit_pad_layout(void) { }
int p4_android_prepare_restart(void) { return -1; }

/* The Zig runtime's process spawning names fexecve, which bionic has from API 28 (the build targets 24). The tests never spawn. */
int fexecve(int fd, char *const argv[], char *const envp[])
{
    (void)fd; (void)argv; (void)envp;
    errno = ENOSYS;
    return -1;
}
