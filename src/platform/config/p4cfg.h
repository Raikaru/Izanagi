/* The player's settings (docs/platform/settings.md).
 *
 * A plain INI file, `p4pc.ini`, in the platform's per-user data folder (SDL_GetPrefPath("p4pc", "Persona4"): %APPDATA% on Windows, ~/.local/share on
 * Linux, ~/Library/Application Support on macOS, the app's files directory on Android and iOS), or the file named by P4PC_CONFIG (P4PC_CONFIG=none: no file at all,
 * which is how the tests run). Every setting has a built-in default that keeps the renderer gates exact (a 640x448 window drawn directly, scale 1, no
 * file), and an environment variable that overrides the file, so a test or a launch script never depends on what a player saved.
 *
 * Keys are "section.name" (`[video]` + `resolution` is "video.resolution"); a key the table below names has its environment variable, the others (the
 * key and button bindings, "input.kb.cross") are file only. Values are text; p4cfg_int and p4cfg_float parse them, with the default for a
 * missing or malformed one.
 *
 * First run of a player (no test switch in the environment, p4cfg_player_run): the file is written with the defaults of a player (p4cfg_template), so that the
 * settings can be found and edited. */
#ifndef P4PC_PLATFORM_CONFIG_P4CFG_H
#define P4PC_PLATFORM_CONFIG_P4CFG_H

#include <stddef.h>

/* Finds and parses the file (once; later calls do nothing). Safe to call from any thread before the game starts. */
void p4cfg_load(void);
/* Startup-only, after owning the actual settings-directory lease and before
 * consumers retain values. Re-read the SAME existing file; no path reselection
 * or file creation. Invalidates borrowed values only on success.0/-1. */
int p4cfg_refresh_startup(void);

/* The value of `key`: its environment variable if set and not empty, else the file's, else NULL. */
const char *p4cfg_get(const char *key);
int p4cfg_int(const char *key, int def);
double p4cfg_float(const char *key, double def);
const char *p4cfg_str(const char *key, const char *def);

/* The file in use, or NULL (none found, or P4PC_CONFIG=none). */
const char *p4cfg_path(void);

/* 1 for a run a person plays (no test, harness, capture or headless switch in the environment); the first such run writes the file. */
int p4cfg_player_run(void);

/* The text of a new file: every key, with the player defaults and a comment each. */
const char *p4cfg_template(void);

/* Cached video policy (invalidated by a settings write). Resolution chooses exact
 * physical storage; aspect and game coordinates are independent. Native or WxH,
 * scale above1, a larger window or fullscreen select the blit path. With no
 * settings, desktop gates retain direct640x448 at scale:1. */
typedef struct {
    int fullscreen;
    int width, height; /* the window's size in pixels when not fullscreen */
    int vsync;         /* 0 off, 1 on, 2 auto (on unless the display runs below 59 Hz) */
    int scale;         /* scale:N parameter, 1 to 8; physical sizes need not be integer multiples */
    int resolution;    /* P4CFG_RESOLUTION_* */
    int renderWidth, renderHeight; /* physical target dimensions for WxH */
    int fit;           /* the picture in the window: P4CFG_FIT_* */
    int aspect;        /* P4CFG_ASPECT_*: projection and layout, independent of presentation fit */
    int movieFit;      /* P4CFG_MOVIE_*: movie-only fit, applied immediately */
    int blit;
} P4CfgVideo;

enum { P4CFG_FIT_STRETCH = 0, P4CFG_FIT_4_3 = 1, P4CFG_FIT_INTEGER = 2 };
enum { P4CFG_ASPECT_4_3 = 0, P4CFG_ASPECT_16_9 = 1, P4CFG_ASPECT_AUTO = 2 };
enum { P4CFG_RESOLUTION_SCALE = 0, P4CFG_RESOLUTION_NATIVE = 1, P4CFG_RESOLUTION_EXPLICIT = 2 };
enum { P4CFG_MOVIE_BARS = 0, P4CFG_MOVIE_ZOOM = 1, P4CFG_MOVIE_STRETCH = 2 };

const P4CfgVideo *p4cfg_video(void);
/* Resolves a candidate through the same video policy without changing the file,
 * table or cache. Environment overrides retain priority over the proposed value. */
void p4cfg_video_preview_resolution(const char *value, P4CfgVideo *video);

/* Test hooks: replaces the table with the keys of `text` (no file, no environment lookups are skipped), and empties it. */
void p4cfg_parse_text(const char *text);
void p4cfg_clear(void);

/* Atomically replaces a key on disk, preserving other keys and comments, then updates
 * the in-memory value and video cache. Environment overrides still win. 0 or -1.
 * P4PC_CONFIG=none refuses writes. */
int p4cfg_set_str(const char *key, const char *value);

typedef struct P4CfgChange { const char *key, *value; } P4CfgChange;
/* One atomic user operation, including conflict resolution/reset's multiple
 * binding keys. No in-memory values change unless the entire write commits. */
int p4cfg_set_batch(const P4CfgChange *changes, size_t count);

#endif
