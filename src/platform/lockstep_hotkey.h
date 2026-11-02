#ifndef P4PC_LOCKSTEP_HOTKEY_H
#define P4PC_LOCKSTEP_HOTKEY_H

/* Capture control only: no game-pad state or physical keyboard polling.
 * The pad event bridge supplies non-repeated key-down events. */
typedef struct {
    int scancode, pending;
} P4LockstepHotkey;

static inline void p4ls_hotkey_press(P4LockstepHotkey *key, int scancode)
{
    if (scancode == key->scancode) key->pending = 1;
}

static inline int p4ls_hotkey_take(P4LockstepHotkey *key)
{
    int pending = key->pending;
    key->pending = 0;
    return pending;
}
#endif
