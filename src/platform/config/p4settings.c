#include "../pad/pad_sdl.h"
#include "../sound/p4snd.h"
#include "p4cfg.h"
#include "p4pc/settings.h"
#include "p4pc/settings_native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUILTINS P4_SETTINGS_MODEL_BUILTINS
static P4SettingsEntry extensions[P4_SETTINGS_EXTENSION_MAX], builtins[9] = {
    [0] = {.flags = P4_SETTINGS_DESKTOP | P4_SETTINGS_TOGGLE},
    [1] = {.flags = P4_SETTINGS_DESKTOP},
    [2] = {.flags = P4_SETTINGS_DESKTOP},
    [5] = {.flags = P4_SETTINGS_DESKTOP},
};
static int extensionCount, binding = -1, conflict = -1, bindingSlot;
static int bindingEntry = -1, actionIndex = -1, statusRow = -1, feedback = -1;
typedef struct VideoAdjustment { int index, direction; } VideoAdjustment;
static VideoAdjustment videoQueue[32];
static unsigned videoCount;
static unsigned char restartVideoRows[BUILTINS + P4_SETTINGS_EXTENSION_MAX];
static char pending[96], status[160];
static Uint64 statusUntil;
static SDL_JoystickID selectedPad;
#if defined(SDL_PLATFORM_ANDROID)
static SDL_AtomicInt hardwareKeyboard;
void p4_settings_android_keyboard_presence(int connected)
{
    SDL_SetAtomicInt(&hardwareKeyboard, connected != 0);
}
#endif
#if !defined(SDL_PLATFORM_ANDROID) && !defined(SDL_PLATFORM_IOS)
static int touchActive;
#endif
#if !defined(SDL_PLATFORM_ANDROID) && !defined(SDL_PLATFORM_IOS)
static int cursorOwned, cursorWasVisible, cursorWasRelative;
static void cursorInput(int mouse)
{
    if (!cursorOwned) return;
    if (mouse) SDL_ShowCursor();
    else SDL_HideCursor();
}
#endif
void p4_settings_cursor_begin(void)
{
#if !defined(SDL_PLATFORM_ANDROID) && !defined(SDL_PLATFORM_IOS)
    SDL_Window *window = p4pc_pad_sdl_window();
    if (!window || cursorOwned) return;
    cursorWasVisible = SDL_CursorVisible();
    cursorWasRelative = SDL_GetWindowRelativeMouseMode(window);
    cursorOwned = 1;
    if (cursorWasRelative) SDL_SetWindowRelativeMouseMode(window, false);
    SDL_ShowCursor();
#endif
}
void p4_settings_cursor_end(void)
{
#if !defined(SDL_PLATFORM_ANDROID) && !defined(SDL_PLATFORM_IOS)
    SDL_Window *window = p4pc_pad_sdl_window();
    if (!cursorOwned) return;
    cursorOwned = 0;
    if (window && cursorWasRelative) SDL_SetWindowRelativeMouseMode(window, true);
    if (cursorWasVisible) SDL_ShowCursor(); else SDL_HideCursor();
#endif
}
static const char *const buttons[] = {"up", "down", "left", "right", "cross", "circle", "triangle", "square",
    "start", "select", "l1", "r1", "l2", "r2", "l3", "r3"};
static const char *const defaultKeys[] = {"Up, W", "Down, S", "Left, A", "Right, D", "Z", "X", "C", "V",
    "Return", "Backspace", "Q", "E", "1", "3", "Left Shift", "Right Shift"};
static const char *const defaultPad[] = {"dpup", "dpdown", "dpleft", "dpright", "south", "east", "north", "west",
    "start", "back", "leftshoulder", "rightshoulder", "none", "none", "leftstick", "rightstick"};
static const char *const labels[] = {"Fullscreen", "Window width", "Window height", "Resolution", "Picture fit",
    "Presentation", "Vsync", "Volume", "Stick deadzone", "Reset all bindings"};
static const char *const keys[] = {"video.fullscreen", "video.width", "video.height", "video.resolution", "video.fit",
    "video.present", "video.vsync", "audio.volume", "input.deadzone"};
static const char *const fits[] = {"stretch", "4:3", "integer"};
static const char *const presents[] = {"auto", "blit", "direct"};
static const char *const vsyncs[] = {"auto", "on", "off"};
#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
static void touchScrollFormat(char *value, size_t capacity, void *unused);
static void touchScrollAdjust(int direction, void *unused);
static void initialiseMobileEntry(void)
{
    static int registered;
    if (!registered) {
        static const P4SettingsEntry entry = {
            .label = "Touch scrolling", .format = touchScrollFormat,
            .adjust = touchScrollAdjust, .key = "input.touch_scroll",
            .flags = P4_SETTINGS_MOBILE | P4_SETTINGS_TOUCH | P4_SETTINGS_TOGGLE,
            .description = "Swipe through menu rows ON/OFF."
        };
        registered = 1;
        extensions[extensionCount++] = entry;
    }
}
#endif
static int count(void)
{
#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
    initialiseMobileEntry();
#endif
    return BUILTINS + extensionCount;
}
static void bindingKey(int index, char *out, size_t cap)
{
    snprintf(out, cap, "input.%s.%s", index < 16 ? "kb" : "pad", buttons[index % 16]);
}
int p4_settings_register(const P4SettingsEntry *entry)
{
#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
    initialiseMobileEntry(); /* reserve its slot before admitting extensions */
#endif
    if (!entry || !entry->label || !entry->adjust ||
        !memchr(entry->label, '\0', P4_SETTINGS_PAGE_LABEL_MAX) ||
        (entry->description && !memchr(entry->description, '\0', P4_SETTINGS_PAGE_HELP_MAX)))
        return -1;
    if (entry->key) {
        for (unsigned i = 0; i < sizeof keys / sizeof keys[0]; ++i) {
            if (!strcmp(entry->key, keys[i])) {
                unsigned inherited = builtins[i].flags;
                builtins[i] = *entry;
                if (!entry->flags) builtins[i].flags = inherited;
                return 0;
            }
        }
    }
    if (extensionCount == P4_SETTINGS_EXTENSION_MAX) return -1;
    extensions[extensionCount++] = *entry;
    return 0;
}
void p4_settings_message(const char *message)
{
    snprintf(status, sizeof status, "%s", message);
    fprintf(stderr, "p4pc settings: %s\n", message);
    statusRow = actionIndex;
    statusUntil = SDL_GetTicks() + 4000;
}
int p4_settings_active(void) { return p4_settings_native_active(); }
void p4_settings_open(void) { p4_settings_native_request(); }
void p4_settings_model_reset(void)
{
    p4pc_pad_settings_fence();
#if defined(SDL_PLATFORM_ANDROID)
    p4_settings_android_keyboard_watch(p4_settings_active());
#endif
    binding = conflict = bindingEntry = -1;
    feedback = -1;
    pending[0] = status[0] = '\0';
}
int p4_settings_model_capturing(void) { return binding >= 0; }
int p4_settings_model_feedback(void)
{
    int result = feedback;
    feedback = -1;
    return result;
}
static int save(const char *key, const char *value)
{
    if (p4cfg_set_str(key, value) != 0) {
        p4_settings_message("Save failed. Nothing changed.");
        feedback = 8;
        return 0;
    }
    status[0] = '\0';
    feedback = 1;
    return 1;
}
#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
static void touchScrollFormat(char *value, size_t capacity, void *unused)
{
    (void)unused;
    snprintf(value, capacity, "%s", p4cfg_int("input.touch_scroll", 1) ? "On" : "Off");
}
static void touchScrollAdjust(int direction, void *unused)
{
    (void)direction;
    (void)unused;
    if (save("input.touch_scroll", p4cfg_int("input.touch_scroll", 1) ? "0" : "1"))
        p4_settings_native_value_changed(actionIndex);
}
#endif

static void keyboardSlots(int index, char slots[2][96])
{
    char key[40], copy[192], *comma;
    bindingKey(index, key, sizeof key);
    snprintf(copy, sizeof copy, "%s", p4cfg_str(key, defaultKeys[index]));
    comma = strchr(copy, ',');
    if (comma)
        *comma++ = '\0';
    snprintf(slots[0], 96, "%s", copy);
    while (comma && *comma == ' ')
        comma++;
    snprintf(slots[1], 96, "%s", comma ? comma : "none");
}

static int saveBindings(const P4CfgChange *changes, size_t count)
{
    if (p4cfg_set_batch(changes, count) != 0) {
        p4_settings_message("Save failed. Bindings unchanged.");
        feedback = 8;
        return 0;
    }
    status[0] = '\0';
    feedback = 1;
    p4pc_pad_reload_bindings();
    p4pc_pad_settings_fence();
    return 1;
}
static void commitBinding(void)
{
    char keys[32][40], values[32][192];
    P4CfgChange changes[32];
    size_t count = 0;
    if (conflict >= 0) {
        for (int i = binding < 16 ? 0 : 16; i < (binding < 16 ? 16 : 32); i++) {
            if (i == binding)
                continue;
            char key[40];
            bindingKey(i, key, sizeof key);
            int changed = 0;
            if (i < 16) {
                char slots[2][96];
                keyboardSlots(i, slots);
                for (int j = 0; j < 2; j++)
                    if (!SDL_strcasecmp(slots[j], pending)) {
                        snprintf(slots[j], 96, "none");
                        changed = 1;
                    }
                if (changed)
                    snprintf(values[count], sizeof values[count], "%s, %s", slots[0], slots[1]);
            } else if (!SDL_strcasecmp(p4cfg_str(key, defaultPad[i - 16]), pending)) {
                snprintf(values[count], sizeof values[count], "none");
                changed = 1;
            }
            if (changed) {
                snprintf(keys[count], sizeof keys[count], "%s", key);
                changes[count] = (P4CfgChange){keys[count], values[count]};
                count++;
            }
        }
    }
    bindingKey(binding, keys[count], sizeof keys[count]);
    if (binding < 16) {
        char slots[2][96];
        keyboardSlots(binding, slots);
        snprintf(slots[bindingSlot], 96, "%s", pending);
        snprintf(values[count], sizeof values[count], "%s, %s", slots[0], slots[1]);
    } else
        snprintf(values[count], sizeof values[count], "%s", pending);
    changes[count] = (P4CfgChange){keys[count], values[count]};
    if (saveBindings(changes, count + 1))
        binding = conflict = -1;
}
static void captureBinding(const char *value)
{
    char key[40];
    snprintf(pending, sizeof pending, "%s", value);
    conflict = -1;
    if (!SDL_strcasecmp(value, "none")) {
        commitBinding();
        return;
    }
    for (int i = binding < 16 ? 0 : 16; i < (binding < 16 ? 16 : 32); i++) {
        char copy[96];
        if (i == binding)
            continue;
        bindingKey(i, key, sizeof key);
        snprintf(copy, sizeof copy, "%s", p4cfg_str(key, i < 16 ? defaultKeys[i] : defaultPad[i - 16]));
        for (char *part = strtok(copy, ","); part; part = strtok(NULL, ",")) {
            while (*part == ' ')
                part++;
            size_t n = strlen(part);
            while (n && part[n - 1] == ' ')
                part[--n] = '\0';
            if (!SDL_strcasecmp(part, pending)) {
                conflict = i;
                break;
            }
        }
        if (conflict >= 0)
            break;
    }
    if (conflict >= 0) {
        snprintf(status, sizeof status, "Used by %s. Confirm replaces.", buttons[conflict % 16]);
        return;
    }
    commitBinding();
}

void p4_settings_model_cancel_capture(void)
{
    if (binding < 0) return;
    binding = conflict = bindingEntry = -1;
    status[0] = pending[0] = '\0';
    feedback = 2;
}
void p4_settings_model_confirm_capture(void)
{
    if (binding >= 0 && pending[0]) commitBinding();
}
void p4_settings_model_clear_capture(void)
{
    if (binding >= 0) captureBinding("none");
}
static int adjust(int selected, int direction)
{
    char value[96];
    if (selected < 0 || selected >= count()) return 0;
    if (selected < 9 && builtins[selected].adjust) {
        feedback = 1;
        builtins[selected].adjust(direction, builtins[selected].userdata);
        return 1;
    }
    if (selected == 3) {
        p4_settings_message("Resolution control needs an active renderer.");
        feedback = 8;
        return 0;
    }
    if (selected < 9) {
#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
        if (selected < 3) {
            p4_settings_message("Window size and fullscreen are controlled by the mobile OS.");
            feedback = 8;
            return 0;
        }
#endif
        int n = 0;
        const char *const *choices = NULL;
        if (!direction) direction = 1;
        if (selected >= 4 && selected <= 6) {
            choices = selected == 4 ? fits : selected == 5 ? presents : vsyncs;
            if (selected == 4) n = p4cfg_video()->fit;
            else if (selected == 6) n = p4cfg_video()->vsync == 2 ? 0 : p4cfg_video()->vsync == 1 ? 1 : 2;
            else {
                const char *v = p4cfg_str(keys[selected], choices[0]);
                for (int i = 0; i < 3; ++i) if (!strcmp(v, choices[i])) n = i;
            }
            snprintf(value, sizeof value, "%s", choices[(n + direction + 3) % 3]);
        } else {
            int old = selected == 0 ? p4cfg_video()->fullscreen
                : selected == 1 ? p4cfg_video()->width
                : selected == 2 ? p4cfg_video()->height
                : p4cfg_int(keys[selected], selected == 7 ? 100 : 0);
            const int lo = selected == 1 ? 160 : selected == 2 ? 112 : 0;
            const int hi = selected == 0 ? 1 : selected == 1 || selected == 2 ? 7680 : selected == 7 ? 100 : 50;
            const int step = selected == 1 || selected == 2 ? 16 : selected == 7 ? 5 : 1;
            n = selected == 0 ? !old : old + direction * step;
            if (n < lo) n = lo;
            if (n > hi) n = hi;
            if (n == old) return 0;
            snprintf(value, sizeof value, "%d", n);
        }
        if (!save(keys[selected], value)) return 0;
        if (selected < 7) {
            restartVideoRows[selected] = p4gl_context_apply_settings() != 0;
            if (restartVideoRows[selected])
                p4_settings_message("Saved; applies on restart. Live video change unavailable.");
        }
        if (selected == 8) p4pc_pad_reload_bindings();
        if (selected == 7) p4snd_set_user_volume((float)p4cfg_int("audio.volume", 100) / 100.0f);
        return 1;
    }
    if (selected < P4_SETTINGS_MODEL_BINDINGS_END) {
        binding = selected < 41 ? (selected - 9) / 2 : 16 + selected - 41;
        bindingSlot = selected < 41 ? (selected - 9) % 2 : 0;
        bindingEntry = selected;
        conflict = -1;
        pending[0] = '\0';
        if (direction < 0) {
            captureBinding("none");
        } else {
            p4_settings_message(binding < 16 ? "Press key. Back: cancel."
                : "Press button. Back: cancel.");
            feedback = 1;
        }
        return 1;
    }
    if (selected == P4_SETTINGS_MODEL_RESET_BINDINGS) {
        char names[32][40];
        P4CfgChange changes[32];
        for (int i = 0; i < 32; ++i) {
            bindingKey(i, names[i], sizeof names[i]);
            changes[i] = (P4CfgChange){names[i], i < 16 ? defaultKeys[i] : defaultPad[i - 16]};
        }
        if (!saveBindings(changes, 32)) return 0;
        p4_settings_message("Default bindings restored.");
        return 1;
    }
    feedback = 1;
    extensions[selected - BUILTINS].adjust(direction, extensions[selected - BUILTINS].userdata);
    return 1;
}
void p4_settings_model_adjust(int index, int direction)
{
    if (index >= 0 && index < 7) {
        /* Keep callback order and its transaction intact. No success, INI write
         * or target mutation is reported merely because an action was queued. */
        if (videoCount == sizeof videoQueue / sizeof videoQueue[0]) {
            fputs("native settings: video adjustment queue overflow\n", stderr);
            exit(1);
        }
        videoQueue[videoCount++] = (VideoAdjustment){index, direction};
        return;
    }
    actionIndex = index;
    adjust(index, direction);
}
void p4_settings_model_frame_end(void)
{
    /* Do not clear this queue on menu close or context recovery. Providers own
     * apply failure, persistence and retaining their last valid render target. */
    for (unsigned i = 0; i < videoCount; ++i) {
        const VideoAdjustment action = videoQueue[i];
        const int previous = action.index == 0 ? p4cfg_video()->fullscreen : 0;
        actionIndex = action.index;
        adjust(action.index, action.direction);
        if (action.index == 0 && previous != p4cfg_video()->fullscreen)
            p4_settings_native_value_changed(action.index);
    }
    videoCount = 0;
}
int p4_settings_entry_count(void) { return count(); }
unsigned p4_settings_entry_flags(int index)
{
    if (index < 0 || index >= count()) return 0;
    if (index < 9) return builtins[index].flags;
    if (index < 41) return P4_SETTINGS_KEYBOARD;
    return index >= BUILTINS ? extensions[index - BUILTINS].flags : 0;
}
int p4_settings_entry_available(int index)
{
    if (index < 0 || index >= count()) return 0;
    unsigned flags = p4_settings_entry_flags(index);
#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
    if ((flags & (P4_SETTINGS_DESKTOP | P4_SETTINGS_MOBILE)) == P4_SETTINGS_DESKTOP)
        return 0;
    if (flags & P4_SETTINGS_KEYBOARD) {
        const char *mode = getenv("P4PC_INPUT");
        /* Unattended cases never enumerate physical input devices. */
        if (mode && (!strcmp(mode, "script") || !strcmp(mode, "virtual"))) return 0;
#if defined(SDL_PLATFORM_ANDROID)
        if (!SDL_GetAtomicInt(&hardwareKeyboard)) return 0;
#else
        if (!SDL_HasKeyboard()) return 0;
#endif
    }
#else
    if ((flags & (P4_SETTINGS_DESKTOP | P4_SETTINGS_MOBILE)) == P4_SETTINGS_MOBILE)
        return 0;
#endif
    return !(flags & P4_SETTINGS_TOUCH) || p4_settings_touch_active();
}
int p4_settings_entry(int i, char *label, size_t labelCap, char *value, size_t valueCap)
{
    if (i < 0 || i >= count()) return -1;
    value[0] = '\0';
    if (i < 9) {
        snprintf(label, labelCap, "%s", builtins[i].label ? builtins[i].label : labels[i]);
        if (builtins[i].format) {
            int previousAction = actionIndex;
            actionIndex = i;
            builtins[i].format(value, valueCap, builtins[i].userdata);
            actionIndex = previousAction;
        }
        else if (i == 3) snprintf(value, valueCap, "%s", p4cfg_str(keys[i], "scale:1"));
        else if (i == 0) snprintf(value, valueCap, "%s", p4cfg_video()->fullscreen ? "On" : "Off");
        else if (i == 1 || i == 2) snprintf(value, valueCap, "%d", i == 1 ? p4cfg_video()->width : p4cfg_video()->height);
        else if (i == 7 || i == 8) snprintf(value, valueCap, "%d%%", p4cfg_int(keys[i], i == 7 ? 100 : 0));
        else if (i == 4) snprintf(value, valueCap, "%s", fits[p4cfg_video()->fit]);
        else if (i == 6) snprintf(value, valueCap, "%s", vsyncs[p4cfg_video()->vsync == 2 ? 0 : p4cfg_video()->vsync == 1 ? 1 : 2]);
        else snprintf(value, valueCap, "%s", p4cfg_str(keys[i], "auto"));
    } else if (i < P4_SETTINGS_MODEL_BINDINGS_END) {
        int b = i < 41 ? (i - 9) / 2 : 16 + i - 41;
        snprintf(label, labelCap, "%s %s%s", b < 16 ? "Key" : "Pad", buttons[b % 16],
            b < 16 ? ((i - 9) % 2 ? " (2)" : " (1)") : "");
        if (binding >= 0 && i == bindingEntry) {
            snprintf(value, valueCap, "%s", conflict >= 0 ? "Replace?" : binding < 16 ? "Press key" : "Press button");
        } else if (b < 16) {
            char slots[2][96];
            keyboardSlots(b, slots);
            snprintf(value, valueCap, "%s", slots[(i - 9) % 2]);
        } else {
            char key[40];
            bindingKey(b, key, sizeof key);
            const char *assigned = p4cfg_str(key, defaultPad[b - 16]);
            const char *display = !SDL_strcasecmp(assigned, "leftshoulder") ? "L1" :
                !SDL_strcasecmp(assigned, "rightshoulder") ? "R1" :
                !SDL_strcasecmp(assigned, "leftstick") ? "L3" :
                !SDL_strcasecmp(assigned, "rightstick") ? "R3" : assigned;
            snprintf(value, valueCap, "%s", display);
        }
    } else if (i == P4_SETTINGS_MODEL_RESET_BINDINGS) {
        snprintf(label, labelCap, "%s", labels[9]);
    } else {
        P4SettingsEntry *entry = &extensions[i - BUILTINS];
        snprintf(label, labelCap, "%s", entry->label);
        if (entry->format) {
            int previousAction = actionIndex;
            actionIndex = i;
            entry->format(value, valueCap, entry->userdata);
            actionIndex = previousAction;
        }
    }
    return restartVideoRows[i] || (i < 9 ? builtins[i].restart : i >= BUILTINS ? extensions[i - BUILTINS].restart : 0);
}
const char *p4_settings_model_notice(void)
{
    return status[0] && SDL_GetTicks() < statusUntil ? status : NULL;
}
const char *p4_settings_model_description(int index)
{
    static const char *const descriptions[] = {
        "Fullscreen ON/OFF.", "Window width.", "Window height.",
        "Choose render resolution.", "Choose picture fitting.",
        "Choose presentation method.", "Choose vertical sync.",
        "Master volume.", "Extra stick deadzone."};
    if (status[0] && statusRow == index &&
        (binding >= 0 || SDL_GetTicks() < statusUntil)) return status;
    if (index >= 0 && index < 9)
        return builtins[index].description ? builtins[index].description : descriptions[index];
    if (index < P4_SETTINGS_MODEL_BINDINGS_END) return "Confirm: bind. Left: clear.";
    if (index == P4_SETTINGS_MODEL_RESET_BINDINGS) return "Reset all control assignments.";
    if (index >= BUILTINS && index < count()) {
        static char description[128];
        const P4SettingsEntry *entry = &extensions[index - BUILTINS];
        if (entry->description) return entry->description;
        snprintf(description, sizeof description,
            entry->flags & P4_SETTINGS_ACTION ? "Confirm: %s." :
            entry->flags & P4_SETTINGS_TOGGLE ? "%s ON/OFF." : "Change %s.",
            entry->label);
        return description;
    }
    return "Left/Right: change.";
}


int p4_settings_touch_active(void)
{
    static int automated = -1;
    if (automated < 0) {
        const char *mode = getenv("P4PC_INPUT");
        automated = mode && (!strcmp(mode, "script") || !strcmp(mode, "virtual")) &&
                    getenv("P4PC_SETTINGS_TEST") == NULL;
    }
    if (automated) return 0;
#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
    return 1;
#else
    return touchActive;
#endif
}
static void touchMode(int active)
{
#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
    (void)active;
#else
    touchActive = active;
#endif
}
static int mappedPadButton(int index)
{
    char key[40];
    bindingKey(16 + index, key, sizeof key);
    return SDL_GetGamepadButtonFromString(p4cfg_str(key, defaultPad[index]));
}
static void captureKey(SDL_Scancode key)
{
    if (key == SDL_SCANCODE_ESCAPE || key == SDL_SCANCODE_F1) {
        p4_settings_model_cancel_capture();
    } else if (key == SDL_SCANCODE_DELETE) {
        p4_settings_model_clear_capture();
    } else if (conflict >= 0) {
        bool keys[SDL_SCANCODE_COUNT] = {false};
        P4PadSnapshot pad;
        memset(&pad, 0, sizeof pad);
        keys[key] = true;
        p4pc_pad_keys_to_snapshot(keys, SDL_SCANCODE_COUNT, &pad);
        if (key == SDL_SCANCODE_RETURN || (pad.buttons & P4PC_PAD_CROSS))
            p4_settings_model_confirm_capture();
        else if (pad.buttons & P4PC_PAD_CIRCLE)
            p4_settings_model_cancel_capture();
    } else if (binding < 16) {
        captureBinding(SDL_GetScancodeName(key));
    }
}
int p4_settings_event(const SDL_Event *event)
{
    static int tracking;
    static SDL_FingerID finger;
    static float startX, startY;
    int active = p4_settings_active();
    if (event->type == SDL_EVENT_KEY_DOWN) {
        SDL_Scancode key = event->key.scancode;
        touchMode(0);
        if (binding >= 0) {
            if (!event->key.repeat && key > SDL_SCANCODE_UNKNOWN && key < SDL_SCANCODE_COUNT)
                captureKey(key);
            return 1;
        }
        if (key == SDL_SCANCODE_ESCAPE || key == SDL_SCANCODE_F1) {
            if (!event->key.repeat) {
                if (active) p4_settings_native_back();
                else p4_settings_open();
            }
            return 1;
        }
        if (active && (key == SDL_SCANCODE_PAGEUP || key == SDL_SCANCODE_PAGEDOWN)) {
            p4_settings_native_touch_scroll(key == SDL_SCANCODE_PAGEUP ? -7 : 7);
            return 1;
        }
        if (active && (key == SDL_SCANCODE_HOME || key == SDL_SCANCODE_END)) {
            p4_settings_native_jump(key == SDL_SCANCODE_END);
            return 1;
        }
        return 0; /* Retail's pad/list code owns navigation, confirm and cancel. */
    }
    if (event->type == SDL_EVENT_FINGER_DOWN) {
        float x, y;
        touchMode(1);
        tracking = p4gl_context_menu_position(event->tfinger.x, event->tfinger.y, &x, &y);
        if (tracking) {
            finger = event->tfinger.fingerID;
            startX = x;
            startY = y;
        }
        return active || (tracking && x >= 544 && y < 48);
    }
    if (event->type == SDL_EVENT_FINGER_UP) {
        float x, y;
        if (tracking && finger == event->tfinger.fingerID &&
            p4gl_context_menu_position(event->tfinger.x, event->tfinger.y, &x, &y)) {
            float dy = y - startY;
            if (active && p4cfg_int("input.touch_scroll", 1) && (dy < -20 || dy > 20)) {
                int step = -(int)(dy / 32);
                p4_settings_native_touch_scroll(step ? step : dy < 0 ? 1 : -1);
            } else if (dy >= -20 && dy <= 20) {
                p4_settings_native_touch(startX, startY);
            }
        }
        tracking = 0;
        return active;
    }
    if (event->type == SDL_EVENT_MOUSE_MOTION && active && event->motion.which != SDL_TOUCH_MOUSEID) {
#if !defined(SDL_PLATFORM_ANDROID) && !defined(SDL_PLATFORM_IOS)
        int w = 0, h = 0;
        float x, y;
        cursorInput(1);
        touchMode(0);
        SDL_GetWindowSize(p4pc_pad_sdl_window(), &w, &h);
        if (w > 0 && h > 0 && p4gl_context_menu_position(event->motion.x / w, event->motion.y / h, &x, &y))
            p4_settings_native_hover(x, y);
#endif
        return 1;
    }
    if (event->type == SDL_EVENT_FINGER_MOTION) return active;
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event->button.which == SDL_TOUCH_MOUSEID) return active;
#if !defined(SDL_PLATFORM_ANDROID) && !defined(SDL_PLATFORM_IOS)
        if (active) cursorInput(1);
#endif
        touchMode(0);
        if (active) {
            int w = 0, h = 0;
            float x, y;
            SDL_GetWindowSize(p4pc_pad_sdl_window(), &w, &h);
            if (w > 0 && h > 0 && p4gl_context_menu_position(event->button.x / w, event->button.y / h, &x, &y))
                p4_settings_native_touch(x, y);
            return 1;
        }
    }
    if (event->type == SDL_EVENT_MOUSE_WHEEL && active) {
        touchMode(0);
        if (event->wheel.y) p4_settings_native_touch_scroll(event->wheel.y > 0 ? -1 : 1);
        return 1;
    }
    if (event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION && event->gaxis.which == selectedPad) {
#if !defined(SDL_PLATFORM_ANDROID) && !defined(SDL_PLATFORM_IOS)
        if (active && (event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX || event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY) &&
            (event->gaxis.value < -16000 || event->gaxis.value > 16000)) cursorInput(0);
#endif
        if (event->gaxis.value < -16000 || event->gaxis.value > 16000) touchMode(0);
        return binding >= 0;
    }
    if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        int b = event->gbutton.button;
#if !defined(SDL_PLATFORM_ANDROID) && !defined(SDL_PLATFORM_IOS)
        if (active) cursorInput(0);
#endif
        touchMode(0);
        if (binding < 0) return 0;
        if (conflict >= 0) {
            if (b == mappedPadButton(4)) p4_settings_model_confirm_capture();
            else if (b == mappedPadButton(5)) p4_settings_model_cancel_capture();
        } else if (binding >= 16) {
            captureBinding(SDL_GetGamepadStringForButton((SDL_GamepadButton)b));
        } else if (b == mappedPadButton(5)) {
            p4_settings_model_cancel_capture();
        }
        return 1;
    }
    return binding >= 0 && event->type == SDL_EVENT_GAMEPAD_BUTTON_UP;
}
void p4_settings_pad(SDL_Gamepad *pad)
{
    static Uint64 held;
    static int triggered;
    const Uint64 now = SDL_GetTicks();
    selectedPad = pad ? SDL_GetGamepadID(pad) : 0;
    if (pad && SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_BACK) &&
        SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_START)) {
        if (!held) held = now;
        if (!triggered && now - held >= 600) {
            if (binding >= 0) p4_settings_model_cancel_capture();
            else if (p4_settings_active()) p4_settings_native_back();
            else p4_settings_open();
            triggered = 1;
        }
    } else {
        held = 0;
        triggered = 0;
    }
}
