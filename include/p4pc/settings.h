/* Platform settings model presented by the game's own Config tasks. */
#ifndef P4PC_SETTINGS_H
#define P4PC_SETTINGS_H
#include <SDL3/SDL.h>
#include <stddef.h>
#include "p4pc/settings_pages.h"
/* Synthetic device id accepted only by an explicitly opted-in scripted UI test.
 * Real keyboard/touch/mouse events remain isolated from unattended runs. */
#define P4_SETTINGS_TEST_DEVICE 0x50345549u

/* No platform bits means both. Capability bits additionally require that
 * input device. TOGGLE formatters return "On"/"Off" for the retail widget. */
enum P4SettingsFlags {
    P4_SETTINGS_DESKTOP = 1u, P4_SETTINGS_MOBILE = 2u,
    P4_SETTINGS_KEYBOARD = 4u, P4_SETTINGS_TOUCH = 8u,
    P4_SETTINGS_TOGGLE = 16u, P4_SETTINGS_ACTION = 32u
};
typedef struct P4SettingsEntry {
    const char *label; /* static lifetime; rendered with the ISO's own font */
    void (*format)(char *value, size_t capacity, void *userdata);
    void (*adjust)(int direction, void *userdata); /* -1/+1 changes, 0 activates */
    void *userdata;
    int restart; /* display "applies on restart" */
    const char *key; /* optional builtin key to replace in place; NULL appends */
    unsigned flags; /* optional availability and widget metadata; zero is general */
    const char *description; /* optional static-lifetime, single-line native help */
} P4SettingsEntry;
/* Copies descriptor; an optional builtin key replaces that row without changing
 * binding indexes; zero flags inherit its availability. Other descriptors append
 * (P4_SETTINGS_EXTENSION_MAX slots; mobile reserves one for Touch scrolling).
 * Returns 0 on success, -1 for invalid/full registration. Register on the game
 * thread; additions while open preserve the selected row. Callbacks own
 * persistence, live application and errors. */
int p4_settings_register(const P4SettingsEntry *entry);
void p4_settings_message(const char *message);
/* Requests the native Config path; opening is asynchronous and respects the
 * game's menu gates. Before its menu system is ready the request is discarded. */
void p4_settings_open(void);
int p4_settings_active(void);
/* Toolkit-independent model introspection, also used by scripted UI witnesses.
 * entry returns restart flag, or -1 for an invalid index. */
int p4_settings_entry_count(void);
int p4_settings_entry(int index, char *label, size_t label_capacity, char *value, size_t value_capacity);
unsigned p4_settings_entry_flags(int index);
int p4_settings_entry_available(int index);
#if defined(SDL_PLATFORM_ANDROID)
/* OS device metadata only, watched while Config is open, never key sampling. */
void p4_settings_android_keyboard_watch(int enabled);
void p4_settings_android_keyboard_presence(int connected);
#endif
/* Input backend forwards only player events, or explicitly opted-in test
 * events. */
int p4_settings_event(const SDL_Event *event);
void p4_settings_pad(SDL_Gamepad *gamepad);
void p4_settings_pump(void);
/* Config owns temporary cursor/relative-mode state only while its task lives. */
void p4_settings_cursor_begin(void);
void p4_settings_cursor_end(void);
/* Input backend hooks. Its active callback fences binding capture; the native
 * Config task continues receiving ordinary navigation input. */
void p4pc_pad_set_settings_hooks(int (*event)(const SDL_Event *), int (*capturing)(void),
                               int (*menu_active)(void), void (*pad)(SDL_Gamepad *));
/* Video changes requested by a menu callback are applied at the present boundary. */
int p4gl_context_apply_settings(void);
void p4pc_pad_reload_bindings(void);
#endif
