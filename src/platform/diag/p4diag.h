/* Player diagnostics (docs/platform/diagnostics.md): what a bug report needs, written to the log of a player run.
 *
 *   - a header at startup: build, OS, CPU, memory, display, settings, mods, the disc check (no personal paths);
 *   - frame-time statistics tagged with the area (field:room, event id, battle): one line per ten seconds and per area change, plus a summary at exit;
 *   - audio underrun events and statistics from the mixer (p4snd_diag_poll), tagged with the area;
 *   - a context line for the crash report (p4_os_crash_note_set), and the clean-exit marker the next run's log rotation looks for (p4iso_rotate_log).
 *
 * Active for a player run (p4cfg_player_run); P4PC_DIAG=1 forces it on for another run, P4PC_DIAG=0 turns it off. Tests and gates never see it unless they ask.
 * Cost: one clock read and a few additions per flip, one area sample every 15 flips, no allocation, no output on a frame. */
#ifndef P4PC_DIAG_P4DIAG_H
#define P4PC_DIAG_P4DIAG_H

#include <stddef.h>
#include <stdint.h>

/* 1 when diagnostics are on for this run. */
int p4diag_active(void);

/* Prints the header, installs the frame hook and the exit handler. `executable` is the game executable's path (its size is logged, never the path), or NULL.
 * Call once, after the log is redirected and the settings are loaded; does nothing when diagnostics are off. */
void p4diag_startup(const char *executable);

/* Prints the summary and the clean-exit line, once. Runs from atexit; Android's exit wrapper calls it first, as the log thread stops draining soon after. */
void p4diag_shutdown(void);

/* ---- the parts the test drives ---- */

enum { P4DIAG_HIST = 256 }; /* 1 ms bins up to 255 ms; the last bin holds everything slower */

typedef struct P4DiagStats {
    uint32_t frames;
    uint32_t slow;       /* flips that took 42 ms or more: a 30 Hz game missed a tick */
    uint32_t hitch;      /* 100 ms or more */
    uint32_t cadence[4]; /* flips that took 1, 2, 3 and 4 or more 60 Hz ticks */
    uint64_t sum_ns, busy_ns, max_ns;
    uint32_t hist[P4DIAG_HIST];
} P4DiagStats;

/* Adds one flip: `frame_ns` from the last flip to this one, `busy_ns` of the game thread's CPU time in it (0 when unknown). */
void p4diag_stats_add(P4DiagStats *s, uint64_t frame_ns, uint64_t busy_ns);
/* The smallest millisecond value that `percent` (1..100) of the flips did not exceed, to the bin: 34 means "at most 34 ms". 0 for no flips. */
unsigned p4diag_stats_percentile_ms(const P4DiagStats *s, unsigned percent);

typedef struct P4DiagArea {
    int scene, event, battle;
    uint32_t field, room, event_id[3];
} P4DiagArea;

/* Reads the area from the running game's memory (the scene, the event task, the battle context). Never faults: every pointer is range-checked. */
void p4diag_area_read(P4DiagArea *area);
/* The same over any memory: `read` fills *out with the word at the guest address and returns 1, or returns 0 when it is not mapped. */
/* Test seam: the memory the running game's area is read from (default: the guest's). */
void p4diag_use_memory(int (*read)(uint32_t address, uint32_t *out, void *user), void *user);
void p4diag_area_read_with(P4DiagArea *area, int (*read)(uint32_t address, uint32_t *out, void *user), void *user);
int p4diag_area_equal(const P4DiagArea *a, const P4DiagArea *b);
/* "f12:r3 ev 176.1.0 battle", "title" (no scene yet). */
void p4diag_area_format(const P4DiagArea *area, char *out, size_t capacity);

/* Copies `text` with the user's profile folder and user name replaced by "~" and "<user>" (so a line cannot carry a personal path). */
void p4diag_redact(const char *text, char *out, size_t capacity);

#endif
