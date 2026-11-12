/* Native Config bridge. The overrides keep retail's 0x4b0-byte work layout;
 * this port-owned guest sidecar holds only the expanded list and input intent.
 * No SDL, renderer or provider-descriptor types cross into game translation units. */
#ifndef P4PC_SETTINGS_NATIVE_H
#define P4PC_SETTINGS_NATIVE_H
#include <stdint.h>

#define P4_SETTINGS_NATIVE_VISIBLE 7
#define P4_SETTINGS_MODEL_BINDINGS_FIRST 9
#define P4_SETTINGS_MODEL_BINDINGS_END 57
#define P4_SETTINGS_MODEL_RESET_BINDINGS 57
#define P4_SETTINGS_MODEL_BUILTINS 58

enum P4SettingsNativeKind {
    P4_SETTINGS_NATIVE_RETAIL,
    P4_SETTINGS_NATIVE_TOGGLE,
    P4_SETTINGS_NATIVE_VALUE,
    P4_SETTINGS_NATIVE_CONTROLS,
    P4_SETTINGS_NATIVE_BINDING,
    P4_SETTINGS_NATIVE_ACTION,
    P4_SETTINGS_NATIVE_CONFIRM,
    P4_SETTINGS_NATIVE_INFO,
    P4_SETTINGS_NATIVE_CHOICE,
    P4_SETTINGS_NATIVE_EMPTY
};
enum { P4_SETTINGS_NATIVE_ROOT, P4_SETTINGS_NATIVE_CONTROL_LIST };
enum { P4_SETTINGS_NATIVE_SHORTCUT = 1, P4_SETTINGS_NATIVE_HOSTED = 2, P4_SETTINGS_NATIVE_HOSTED_TOP = 4 };
enum { P4_SETTINGS_NATIVE_ROW_DISABLED = 1 };
enum { P4_SETTINGS_NATIVE_BACK = -1, P4_SETTINGS_NATIVE_RESTORE = -2 };
enum { P4_SETTINGS_NATIVE_SWITCH = 1, P4_SETTINGS_NATIVE_CLOSE = 2 };

typedef struct P4SettingsNativeRow {
    int32_t kind, index;
    uint32_t label, value, help, timer;
    int32_t boolean, restart;
    uint32_t flags;
} P4SettingsNativeRow;

typedef struct P4SettingsNativeState {
    int32_t count, scroll, view, nextView;
    int32_t cursor, navigation;
    int32_t touchRow, activate, direction, back, step;
    int32_t jump;
    uint32_t flags, scrollbar[3];
    float scrollOffset; /* stock System menu's three-frame scrollbar nudge */
    P4SettingsNativeRow rows[P4_SETTINGS_NATIVE_VISIBLE];
    uint32_t choiceArrows[2]; /* retail equipment stat-arrow art */
    int32_t helpPage, helpPages;
    uint32_t title;
} P4SettingsNativeState;

/* Entry requests live for one guest input frame. Only retail's own field/title
 * entry points consume them; rejected or unvisited requests expire. */
void p4_settings_native_request(void);
int p4_settings_native_field_request(uint32_t task, int allowed);
void p4_settings_native_camp_created(uint32_t parent, uint32_t work);
int p4_settings_native_camp_route(uint32_t work);
int p4_settings_native_system_route(uint32_t work, int allowed);
int p4_settings_native_title_request(void);
int p4_settings_native_title_ready(void); /* observed real selector, not forced */
void p4_settings_native_frame_end(void);

int p4_settings_native_active(void);
void p4_settings_native_begin(uint32_t work, int mode);
void p4_settings_native_end(uint32_t work);
/* A game parent other than System/title, running outside Camp/title with nLine
 * off, hosts one registered page flow in its own real cmpConfigCreate(task, 1)
 * child. Call with the returned task handle before that child's first update;
 * Camp mode 0 is refused (its close waits on Camp's nLine forever). The page is
 * the top view: Back, Esc, touch Back or page_back/close there close Config,
 * never revealing the Config root, Controls or CONFIRM; retail flags are not
 * synchronized. The parent reads no pad edges until cmpConfigIsStepDone, then
 * destroys the child; cmpConfigDestroy closes the page stack first and turns
 * the title-mode nLine off again. 0/-1. */
int p4_settings_native_host(uint32_t config_task, const char *key);
/* Returns a guest address, or zero for the retail-only qualification path.
 * refresh formats the seven visible rows using the settings model. */
uint32_t p4_settings_native_state(uint32_t work, int refresh);
uint32_t p4_settings_native_retail_work(void); /* qualification path only */
void p4_settings_native_view(uint32_t work, int view, int cursor);
/* Current logical cursor and view; -1 when native Config is not owned. */
int p4_settings_native_cursor(int *view);
/* Logical row in the current filtered view; -2 selects Controls, -1 is absent. */
int p4_settings_native_find_row(int modelIndex);
int p4_settings_native_row_position(int modelIndex, float *x, float *y);
/* Bounded PC labels/values use the game's metrics, palette and scale path. */
void p4_settings_native_draw_value(float x, float y, uint32_t color, int row);
void p4_settings_native_draw_label(float x, float y, uint32_t color, int row);
void p4_settings_native_draw_info_bar(void *sprite, float x, float y, uint32_t color);
void p4_settings_native_value_changed(int modelIndex);
void p4_settings_native_draw_title(float x, float y, uint32_t color);
void p4_settings_native_draw_help(float x, float y, uint32_t color, int row);
void p4_settings_native_help_step(int direction);
void p4_settings_native_jump(int last);
void p4_settings_native_activate(int slot, int direction);
int p4_settings_native_feedback(void);
void p4_settings_native_touch(float x, float y);
void p4_settings_native_hover(float x, float y);
void p4_settings_native_touch_scroll(int direction);
void p4_settings_native_back(void);
/* Queues the game's own rounded HUD frame and font; does nothing without touch. */
void p4_settings_native_corner(void);

/* Model-only actions shared by native Config and the SDL press-to-bind fence. */
void p4_settings_model_reset(void);
void p4_settings_model_adjust(int index, int direction);
int p4_settings_model_capturing(void);
void p4_settings_model_cancel_capture(void);
void p4_settings_model_confirm_capture(void);
void p4_settings_model_clear_capture(void);
int p4_settings_model_feedback(void); /* -1: none; otherwise stock COMSE id */
const char *p4_settings_model_description(int index);
const char *p4_settings_model_notice(void);
void p4_settings_model_frame_end(void);
int p4_settings_touch_active(void);
/* Normalized window coordinates to the centered native menu's 640x448 grid. */
int p4gl_context_menu_position(float x, float y, float *menuX, float *menuY);
int p4gl_context_menu_bounds(float rect[4]); /* normalized x, y, width, height */
#endif
