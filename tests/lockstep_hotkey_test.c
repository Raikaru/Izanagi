/* Exercise the actual capture-control state with synthetic scancodes: no SDL,
 * window, keyboard, controller, game memory or alternate input mode. */
#include "../src/platform/lockstep_hotkey.h"

int main(void)
{
    enum { FIRST_KEY = 17, SECOND_KEY = 18, OTHER_KEY = 19 };
    P4LockstepHotkey key = {FIRST_KEY, 0};
    if (p4ls_hotkey_take(&key) != 0) return 1;
    p4ls_hotkey_press(&key, OTHER_KEY);
    if (p4ls_hotkey_take(&key) != 0) return 2;
    p4ls_hotkey_press(&key, FIRST_KEY);
    p4ls_hotkey_press(&key, FIRST_KEY);
    if (p4ls_hotkey_take(&key) != 1) return 3;
    if (p4ls_hotkey_take(&key) != 0) return 4;
    key.scancode = SECOND_KEY;
    p4ls_hotkey_press(&key, FIRST_KEY);
    if (p4ls_hotkey_take(&key) != 0) return 5;
    p4ls_hotkey_press(&key, SECOND_KEY);
    p4ls_hotkey_press(&key, OTHER_KEY);
    if (p4ls_hotkey_take(&key) != 1) return 6;
    if (p4ls_hotkey_take(&key) != 0) return 7;
    p4ls_hotkey_press(&key, SECOND_KEY);
    if (p4ls_hotkey_take(&key) != 1) return 8;
    return 0;
}
