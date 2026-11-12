/* Host input for the libpad replacement (src/platform/pad/ps2_pad.c).
 *
 * The snapshot uses the DualShock 2 layout that libpad reports. Button bits are
 * active-high here: the high byte is the PS2 data[2] layout and the low byte is
 * data[3]. The caller inverts them into the active-low bytes of the pad packet. */
#ifndef P4PC_PAD_SDL_H
#define P4PC_PAD_SDL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Active-high button bits. */
#define P4PC_PAD_SELECT   0x0001u
#define P4PC_PAD_L3       0x0002u
#define P4PC_PAD_R3       0x0004u
#define P4PC_PAD_START    0x0008u
#define P4PC_PAD_UP       0x0010u
#define P4PC_PAD_RIGHT    0x0020u
#define P4PC_PAD_DOWN     0x0040u
#define P4PC_PAD_LEFT     0x0080u
#define P4PC_PAD_L2       0x0100u
#define P4PC_PAD_R2       0x0200u
#define P4PC_PAD_L1       0x0400u
#define P4PC_PAD_R1       0x0800u
#define P4PC_PAD_TRIANGLE 0x1000u
#define P4PC_PAD_CIRCLE   0x2000u
#define P4PC_PAD_CROSS    0x4000u
#define P4PC_PAD_SQUARE   0x8000u

/* Pressure order in the pad packet (data[8..19]): the twelve digital buttons. */
enum
{
    P4PC_PRESS_RIGHT,
    P4PC_PRESS_LEFT,
    P4PC_PRESS_UP,
    P4PC_PRESS_DOWN,
    P4PC_PRESS_TRIANGLE,
    P4PC_PRESS_CIRCLE,
    P4PC_PRESS_CROSS,
    P4PC_PRESS_SQUARE,
    P4PC_PRESS_L1,
    P4PC_PRESS_R1,
    P4PC_PRESS_L2,
    P4PC_PRESS_R2,
    P4PC_PRESS_COUNT
};

typedef struct P4PadSnapshot
{
    uint16_t buttons;               /* active-high, see above */
    uint8_t pressure[P4PC_PRESS_COUNT];
    uint8_t rx, ry, lx, ly;         /* PS2 stick bytes, centre 0x80 */
} P4PadSnapshot;

/* One scripted press of P4PC_AUTOPRESS (see pad_sdl.c): button bit, start time, hold time and
 * optional repeat period and repeat count (0 = forever), the times in seconds. */
typedef struct P4PadAutoPress
{
    uint16_t bit;
    double start, hold, every;
    int count;
} P4PadAutoPress;

/* Parses a P4PC_AUTOPRESS spec; returns the entry count, or -1 on a malformed entry. */
int p4pc_pad_autopress_parse(const char *spec, P4PadAutoPress *out, int max);

/* The frame counter P4PC_AUTOPRESS_FRAMES=1 scripts against (the kernel model's flip count). */
void p4pc_pad_autopress_set_clock(uint32_t (*frames)(void));
/* The scripted presses (P4PC_AUTOPRESS_FRAMES=1) run on the clock flips - flipAtT + T from here; before this is called and with
 * P4PC_AUTOPRESS_NEWGAME set, none run (src/platform/title_driver.c). */
void p4pc_pad_autopress_set_origin(unsigned flipAtT, double T);
void p4pc_pad_inject(unsigned bits, unsigned frames);   /* buttons down for that many flips (harness-driven presses) */
/* -2 until a walk is parsed; otherwise P4WALK_RUNNING/DONE/FAILED. */
int p4pc_pad_walk_status(void);
/* Optional source for the walk's battle driver once its battleturn: lines are spent (P4WalkHost.battle_turn); NULL by default.
 * The battle simulator's fork search sets it (src/platform/battle_sim/live.c). */
void p4pc_pad_set_battle_turn_hook(int (*hook)(unsigned n, char *line, size_t size));
/* The game's active-skill predicate for the walk driver's Skill/Item list model (P4WalkHost.skill_active), so it lists an added
 * skill as the native list does; NULL (the default) keeps the retail split. Kernel init sets p4_game_skill_active. */
void p4pc_pad_set_skill_active(int (*active)(uint32_t id));
/* The game's passive grant query for an enemy kind (P4WalkHost.passive_granted), so the battle driver's affinity model sees granted
 * passives; NULL (the default) sees none. Kernel init sets p4_game_passive_enemy_has. */
void p4pc_pad_set_passive_granted(int (*granted)(uint32_t enemy, uint32_t skill));

/* ORs every scripted press that is down at `now` seconds into the snapshot. */
void p4pc_pad_autopress_apply(const P4PadAutoPress *list, int count, double now, P4PadSnapshot *out);

/* SDL axes (-32768..32767) to PS2 bytes, matching PCSX2's default 1.33
 * per-axis sensitivity and half-axis quantization; optional round dead zone. */
void p4pc_pad_stick(int x, int y, int dead, uint8_t *ox, uint8_t *oy);

/* Initialise SDL video and gamepad, and create the window. Returns 0 on success. */
int p4pc_pad_sdl_init(void);

/* Pump host events and read the keyboard and the first open gamepad. */
void p4pc_pad_sdl_read(P4PadSnapshot *out);
/* P4PC_PAD_TRACE: per-game-frame raw SDL input, emitted packet and guest movement state. */
void p4pc_pad_trace_packet(const uint8_t *packet);

/* Keyboard part of the snapshot: ORs in the digital bits and pressures for every
 * keyboard-bound button whose scancode is set in keys[0..numkeys). Pure, so the host
 * test can drive it with a synthetic key array. p4pc_pad_sdl_read calls it. */
void p4pc_pad_keys_to_snapshot(const bool *keys, int numkeys, P4PadSnapshot *out);
/* A changed binding/reset cannot turn its still-held activation into a second
 * game/menu action. Player or explicit virtual-fixture input must release first. */
void p4pc_pad_settings_fence(void);
/* Explicit virtual-input fixture only; holds real SDL virtual buttons for
 * the requested game frames. Never attaches or reads a physical device. */
int p4pc_pad_virtual_press(unsigned bits, unsigned frames);
int p4pc_pad_virtual_stick(int x, int y, unsigned frames);

/* Builds the 32-byte DualShock 2 packet from a snapshot. analog selects the mode id
 * (0x79 analog, 0x41 digital). scePadRead calls it. */
void p4pc_pad_build_packet(const P4PadSnapshot *snap, int analog, uint8_t *out);

/* The SDL window created by p4pc_pad_sdl_init, or NULL before it. Rendering may share it. */
struct SDL_Window *p4pc_pad_sdl_window(void);

/* 1 once after SDL replaced the window's GL context (SDL_EVENT_RENDER_DEVICE_RESET: Android resumed with the old context lost), else 0. */
int p4pc_pad_sdl_take_device_reset(void);

#endif
