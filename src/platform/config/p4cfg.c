/* See p4cfg.h. */
#include "p4cfg.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#define MAX_ENTRIES 96
#define KEY_LEN 40
#define VAL_LEN 600

typedef struct {
    char key[KEY_LEN];
    char val[VAL_LEN];
} Entry;

static Entry sEntries[MAX_ENTRIES];
static int sCount;
static int sLoaded;
static char sPath[1024];

/* The settings that have an environment variable. */
static const struct {
    const char *key;
    const char *env;
} kEnv[] = {
    {"video.fullscreen", "P4PC_FULLSCREEN"}, {"video.width", "P4PC_WINDOW_W"}, {"video.height", "P4PC_WINDOW_H"}, {"video.vsync", "P4PC_VSYNC"},
    {"video.resolution", "P4PC_RESOLUTION"}, {"video.fit", "P4PC_PRESENT_FIT"}, {"video.present", "P4PC_PRESENT"}, {"audio.volume", "P4PC_VOLUME"},
    {"input.deadzone", "P4PC_STICK_DEADZONE"},
    {"video.aspect", "P4PC_ASPECT"},
    {"video.movie_fit", "P4PC_MOVIE_FIT"},
    {"lockstep.directory", "P4PC_LOCKSTEP_DIR"}, {"lockstep.hotkey", "P4PC_LOCKSTEP_HOTKEY"}, {"lockstep.interval", "P4PC_LOCKSTEP_INTERVAL"},
};

const char *p4cfg_template(void)
{
    return "# Persona 4 PC settings. Delete a line to get its default; P4PC_* environment variables override this file (docs/platform/settings.md).\n"
           "\n"
           "[game]\n"
           "# The path of your own Persona 4 (USA) disc image (.iso). The game asks for it on the first run and writes it here; delete the line to be asked again.\n"
           "\n"
           "[video]\n"
           "# 1: borderless fullscreen on the desktop. F11 or Alt+Enter switch it while playing.\n"
           "fullscreen = 0\n"
           "# The window's size in pixels when not fullscreen (it can be resized).\n"
           "width = 1280\n"
           "height = 960\n"
           "# auto: wait for the display's vertical blank when it runs at 60 Hz, else pace by the clock; on: always wait for the display; off: never.\n"
           "vsync = auto\n"
           "# Internal rendering resolution: native (fitted output pixels), WxH, or scale:N (1 to 8 times the original).\n"
           "resolution = scale:1\n"
           "# 4:3: original picture; 16:9: wider horizontal view; auto: match the window without stretching the interface.\n"
           "aspect = 4:3\n"
           "# Movies only: bars keeps the original picture; zoom fills uniformly and crops centrally; stretch fills non-uniformly.\n"
           "movie_fit = bars\n"
           "# How the picture fills the window: 4:3 (bars), integer (4:3 at a whole multiple of 640x480 that fits, bars), stretch (the whole window).\n"
           "fit = 4:3\n"
           "# blit: draw offscreen and fit to the window (needed for scale, resizing, fit); direct: draw into the window as is; auto: blit when this file asks for it.\n"
           "present = blit\n"
           "\n"
           "[audio]\n"
           "# 0 to 100.\n"
           "volume = 100\n"
           "\n"
           "[input]\n"
           "# Optional extra stick dead zone (0 to 50 percent); the game already has its own.\n"
           "deadzone = 0\n"
           "# Keyboard: up to two keys per button (SDL key names: https://wiki.libsdl.org/SDL3/SDL_Scancode), comma separated. Defaults shown.\n"
           "kb.up = Up, W\n"
           "kb.down = Down, S\n"
           "kb.left = Left, A\n"
           "kb.right = Right, D\n"
           "kb.cross = Z\n"
           "kb.circle = X\n"
           "kb.triangle = C\n"
           "kb.square = V\n"
           "kb.start = Return\n"
           "kb.select = Backspace\n"
           "kb.l1 = Q\n"
           "kb.r1 = E\n"
           "kb.l2 = 1\n"
           "kb.r2 = 3\n"
           "kb.l3 = Left Shift\n"
           "kb.r3 = Right Shift\n"
           "# Gamepad: the SDL button for each PS2 button (south, east, west, north, back, start, leftshoulder, rightshoulder, leftstick, rightstick, dpup, dpdown,\n"
           "# dpleft, dpright, misc1, touchpad ...). L2 and R2 are the analog triggers (pad.l2 / pad.r2 = none keeps them there). Defaults shown.\n"
           "pad.cross = south\n"
           "pad.circle = east\n"
           "pad.square = west\n"
           "pad.triangle = north\n"
           "pad.start = start\n"
           "pad.select = back\n"
           "pad.l1 = leftshoulder\n"
           "pad.r1 = rightshoulder\n"
           "pad.l3 = leftstick\n"
           "pad.r3 = rightstick\n"
           "pad.up = dpup\n"
           "pad.down = dpdown\n"
           "pad.left = dpleft\n"
           "pad.right = dpright\n"
           "\n"
           "[mods]\n"
           "# Opt-in package ids, comma separated. Empty keeps retail behavior; P4PC_MODS=0 overrides this list.\n"
           "enabled =\n"
           "# Optional enabled mod order; later mods win. Changes take effect on restart.\n"
           "load_order =\n"
           "# Packages default to mods/ beside this file. See docs/modding/files.md.\n"
           "\n"
           "[packs]\n"
           "# Opt-in engine-pack ids, comma separated. P4PC_PACKS=0 disables this selector.\n"
           "enabled =\n"
           "# Packages default to packs/ beside this file; P4PC_PACK_DIR overrides packs.directory.\n"
           "\n"
           "[lockstep]\n"
           "# Sessions are started by the lockstep developer tooling. F9 marks the exact game frame for both sides.\n"
           "# directory is set by the launcher; leave unset for normal play without recording.\n"
           "hotkey = F9\n"
           "interval = 60\n";
}

static int sVideoDone;
static P4CfgVideo sVideo;

void p4cfg_clear(void)
{
    sVideoDone = 0;
    sCount = 0;
    sLoaded = 0;
    sPath[0] = '\0';
}

static char *trim(char *s)
{
    char *e;

    while (*s != '\0' && isspace((unsigned char)*s)) {
        s++;
    }
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) {
        *--e = '\0';
    }
    return s;
}
/* Match the writer's convention: only whitespace-separated ;/# starts an
 * inline comment. Embedded path/key characters remain literal. */
static const char *inlineComment(const char *value, const char *end)
{
    for (const char *c = value; c < end; ++c) {
        if ((*c == ';' || *c == '#') && c > value && isspace((unsigned char)c[-1])) {
            while (c > value && (c[-1] == ' ' || c[-1] == '\t')) --c;
            return c;
        }
    }
    return end;
}

static void put(const char *key, const char *val)
{
    int i;

    for (i = 0; i < sCount; i++) {
        if (strcmp(sEntries[i].key, key) == 0) {
            snprintf(sEntries[i].val, VAL_LEN, "%s", val);
            return;
        }
    }
    if (sCount < MAX_ENTRIES) {
        snprintf(sEntries[sCount].key, KEY_LEN, "%s", key);
        snprintf(sEntries[sCount].val, VAL_LEN, "%s", val);
        sCount++;
    }
}

void p4cfg_parse_text(const char *text)
{
    char section[KEY_LEN] = "";
    const char *p = text;

    sCount = 0;
    sLoaded = 1;
    while (*p != '\0') {
        char line[1200];
        size_t n = strcspn(p, "\n");
        char *s, *eq;

        snprintf(line, sizeof line, "%.*s", (int)(n < sizeof line - 1 ? n : sizeof line - 1), p);
        p += n;
        if (*p == '\n') {
            p++;
        }
        s = trim(line);
        if (*s == '\0' || *s == '#' || *s == ';') {
            continue;
        }
        if (*s == '[') {
            char *close = strchr(s, ']');

            if (close != NULL) {
                *close = '\0';
                snprintf(section, sizeof section, "%s", trim(s + 1));
                for (char *c = section; *c != '\0'; c++) {
                    *c = (char)tolower((unsigned char)*c);
                }
            }
            continue;
        }
        eq = strchr(s, '=');
        if (eq == NULL) {
            continue;
        }
        *eq = '\0';
        {
            char *k = trim(s);
            char *v = trim(eq + 1);
            char full[KEY_LEN];
            *(char *)inlineComment(v, v + strlen(v)) = '\0';

            for (char *c = k; *c != '\0'; c++) {
                *c = (char)tolower((unsigned char)*c);
            }
            snprintf(full, sizeof full, "%s%s%s", section, section[0] != '\0' ? "." : "", k);
            put(full, v);
        }
    }
}

int p4cfg_player_run(void)
{
    static const char *const kTest[] = {"P4PC_TURBO",      "P4PC_VIRTUAL_CLOCK", "P4PC_HIDDEN_WINDOW", "P4PC_AUTOPRESS", "P4PC_AUTOPRESS_NEWGAME",
                                        "P4PC_AUTOPRESS_LOAD", "P4PC_CAPTURE_FRAMES", "P4PC_CAPTURE_STATE", "P4PC_START_BATTLE", "P4PC_START_MENU", "P4PC_CONFIG",
                                        "P4PC_WALK_SCRIPT",   "P4PC_BATTLE_DRIVER",   "P4PC_TITLE_FRAME"};
    const char *drv = getenv("SDL_VIDEODRIVER");
    const char *audio = getenv("SDL_AUDIODRIVER"); /* every unattended run asks for the dummy driver */
    size_t i;

    if (audio != NULL && audio[0] != '\0') {
        return 0;
    }

    for (i = 0; i < sizeof kTest / sizeof kTest[0]; i++) {
        if (getenv(kTest[i]) != NULL) {
            return 0;
        }
    }
    return !(drv != NULL && (strcmp(drv, "offscreen") == 0 || strcmp(drv, "dummy") == 0));
}

void p4cfg_load(void)
{
    const char *over;
    FILE *f;

    if (sLoaded) {
        return;
    }
    sLoaded = 1;
    sCount = 0;
    over = getenv("P4PC_CONFIG");
    if (over != NULL && strcmp(over, "none") == 0) {
        return;
    }
    if (over != NULL && over[0] != '\0') {
        snprintf(sPath, sizeof sPath, "%s", over);
    } else {
        char *pref = SDL_GetPrefPath("p4pc", "Persona4");

        if (pref == NULL) {
            return;
        }
        snprintf(sPath, sizeof sPath, "%sp4pc.ini", pref);
        SDL_free(pref);
    }
    f = fopen(sPath, "rb");
    if (f != NULL) {
        char *text = (char *)malloc(1u << 16);
        size_t n = text != NULL ? fread(text, 1, (1u << 16) - 1, f) : 0;

        fclose(f);
        if (text != NULL) {
            text[n] = '\0';
            p4cfg_parse_text(text);
            free(text);
            sLoaded = 1;
        }
        return;
    }
    if (over == NULL && p4cfg_player_run()) {
        /* the first run of a player: write the file (and use it), so the settings can be found */
        f = fopen(sPath, "wb");
        if (f != NULL) {
            const char *t = p4cfg_template();

            fwrite(t, 1, strlen(t), f);
            fclose(f);
            fprintf(stderr, "p4pc: wrote the settings file %s\n", sPath);
            p4cfg_parse_text(t);
            return;
        }
    }
    sPath[0] = '\0';
}

int p4cfg_refresh_startup(void)
{
    if(!sLoaded||!sPath[0])return -1;
    FILE *file=fopen(sPath,"rb");
    if(!file)return -1;
    char *text=malloc(1u<<16);
    if(!text){fclose(file);return -1;}
    size_t size=fread(text,1,(1u<<16)-1,file);
    int extra=size==(1u<<16)-1?fgetc(file):EOF;
    int failed=ferror(file)||extra!=EOF;
    if(fclose(file))failed=1;
    if(failed){free(text);return -1;}
    text[size]=0;p4cfg_parse_text(text);sVideoDone=0;free(text);
    return 0;
}

const char *p4cfg_path(void)
{
    return sPath[0] != '\0' ? sPath : NULL;
}

const char *p4cfg_get(const char *key)
{
    size_t i;
    int k;

    for (i = 0; i < sizeof kEnv / sizeof kEnv[0]; i++) {
        if (strcmp(kEnv[i].key, key) == 0) {
            const char *e = getenv(kEnv[i].env);

            if (e != NULL && e[0] != '\0') {
                return e;
            }
        }
    }
    for (k = 0; k < sCount; k++) {
        if (strcmp(sEntries[k].key, key) == 0) {
            return sEntries[k].val;
        }
    }
    return NULL;
}

int p4cfg_int(const char *key, int def)
{
    const char *v = p4cfg_get(key);
    char *end;
    long n;

    if (v == NULL || v[0] == '\0') {
        return def;
    }
    n = strtol(v, &end, 10);
    return end != v ? (int)n : def;
}

double p4cfg_float(const char *key, double def)
{
    const char *v = p4cfg_get(key);
    char *end;
    double d;

    if (v == NULL || v[0] == '\0') {
        return def;
    }
    d = strtod(v, &end);
    return end != v ? d : def;
}

const char *p4cfg_str(const char *key, const char *def)
{
    const char *v = p4cfg_get(key);

    return v != NULL && v[0] != '\0' ? v : def;
}


static int clampInt(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static int boolValue(const char *v, int def)
{
    if (v == NULL || v[0] == '\0') {
        return def;
    }
    if (strcmp(v, "0") == 0 || SDL_strcasecmp(v, "off") == 0 || SDL_strcasecmp(v, "false") == 0 || SDL_strcasecmp(v, "no") == 0) {
        return 0;
    }
    if (strcmp(v, "1") == 0 || SDL_strcasecmp(v, "on") == 0 || SDL_strcasecmp(v, "true") == 0 || SDL_strcasecmp(v, "yes") == 0) {
        return 1;
    }
    return def;
}

/* The atomic INI writer preserves whitespace-separated inline comments.
 * Validate the whole mode token without interpreting that comment as junk. */
static int resolutionEnd(const char *end)
{
    if (*end == '\0') return 1;
    if (!isspace((unsigned char)*end)) return 0;
    do { end++; } while (isspace((unsigned char)*end));
    return *end == '\0' || *end == ';' || *end == '#';
}

static void videoResolution(P4CfgVideo *video, const char *v)
{
    char *end;
    unsigned long w, h;

    video->resolution = P4CFG_RESOLUTION_SCALE;
    video->scale = 1;
    video->renderWidth = video->renderHeight = 0;
    if (v == NULL || v[0] == '\0') return;
    if (strncmp(v, "native", 6) == 0 && resolutionEnd(v + 6)) {
        video->resolution = P4CFG_RESOLUTION_NATIVE;
        return;
    }
    if (strncmp(v, "scale:", 6) == 0) {
        w = strtoul(v + 6, &end, 10);
        if (v[6] >= '1' && v[6] <= '8' && end == v + 7 && resolutionEnd(end)) {
            video->scale = (int)w;
            return;
        }
    } else if (v[0] >= '1' && v[0] <= '9') {
        w = strtoul(v, &end, 10);
        if (*end == 'x' || *end == 'X') {
            const char *second = end + 1;
            h = strtoul(second, &end, 10);
            if (second[0] >= '1' && second[0] <= '9' && resolutionEnd(end) &&
                w >= 1 && w <= 16384 && h >= 1 && h <= 16384) {
                video->resolution = P4CFG_RESOLUTION_EXPLICIT;
                video->renderWidth = (int)w;
                video->renderHeight = (int)h;
                return;
            }
        }
    }
    fprintf(stderr, "p4pc: invalid video.resolution '%s': expected native, WxH (1..16384), or scale:1..8; using scale:1\n", v);
}

static void readVideo(P4CfgVideo *video, const char *resolution)
{
    const char *v;
    int device = 0;
    int want; /* -1 auto, 0 direct, 1 blit */

#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
    device = 1; /* a phone's window is the whole screen: the picture is fitted into it */
#endif
    video->fullscreen = device || boolValue(p4cfg_get("video.fullscreen"), 0);
    video->width = clampInt(p4cfg_int("video.width", 640), 160, 16384);
    video->height = clampInt(p4cfg_int("video.height", 448), 112, 16384);
    videoResolution(video, resolution);
    v = p4cfg_get("video.aspect");
    video->aspect = v != NULL && strcmp(v, "16:9") == 0 ? P4CFG_ASPECT_16_9 :
                    v != NULL && strcmp(v, "auto") == 0 ? P4CFG_ASPECT_AUTO : P4CFG_ASPECT_4_3;
    v = p4cfg_get("video.movie_fit");
    video->movieFit = v != NULL && strcmp(v, "zoom") == 0 ? P4CFG_MOVIE_ZOOM :
                      v != NULL && strcmp(v, "stretch") == 0 ? P4CFG_MOVIE_STRETCH : P4CFG_MOVIE_BARS;
    v = p4cfg_get("video.vsync");
    video->vsync = v == NULL || SDL_strcasecmp(v, "auto") == 0 ? 2 : boolValue(v, 2);
    v = p4cfg_get("video.fit");
    video->fit = device ? P4CFG_FIT_4_3 : P4CFG_FIT_STRETCH;
    if (v != NULL) {
        if (strcmp(v, "4:3") == 0) {
            video->fit = P4CFG_FIT_4_3;
        } else if (strcmp(v, "integer") == 0) {
            video->fit = P4CFG_FIT_INTEGER;
        } else if (strcmp(v, "stretch") == 0) {
            video->fit = P4CFG_FIT_STRETCH;
        }
    }
    v = p4cfg_get("video.present");
    want = v == NULL || strcmp(v, "auto") == 0 ? -1 : (strcmp(v, "blit") == 0);
    {
        const int needs = video->resolution != P4CFG_RESOLUTION_SCALE || video->scale > 1 || video->fullscreen ||
                          video->width != 640 || video->height != 448 || video->aspect != P4CFG_ASPECT_4_3;

        if (device) {
            /* A mobile drawable is presentation-only: resolution, fit and GS
             * feedback require the retained RGBA8 target even for direct. */
            video->blit = 1;
        } else if (want < 0) {
            video->blit = needs;
        } else {
            video->blit = want;
            if (!want && needs) {
                fprintf(stderr, "p4pc: video.present = direct cannot do scale %d / fullscreen / a %dx%d window: drawing offscreen and fitting instead\n", video->scale,
                        video->width, video->height);
                video->blit = 1;
            }
        }
    }
    if (!video->blit) {
        video->width = 640;
        video->height = 448;
        video->fullscreen = 0;
        video->scale = 1;
    }
}

const P4CfgVideo *p4cfg_video(void)
{
    if (!sVideoDone) {
        p4cfg_load();
        readVideo(&sVideo, p4cfg_get("video.resolution"));
        sVideoDone = 1;
    }
    return &sVideo;
}

void p4cfg_video_preview_resolution(const char *value, P4CfgVideo *video)
{
    const char *override = getenv("P4PC_RESOLUTION");
    p4cfg_load();
    readVideo(video, override && override[0] ? override : value);
}

static int writeBytes(SDL_IOStream *out, const char *text, size_t length) { return SDL_WriteIO(out, text, length) == length ? 0 : -1; }

static int writeKey(SDL_IOStream *out, const char *name, const char *value)
{
    return writeBytes(out, name, strlen(name)) || writeBytes(out, " = ", 3) || writeBytes(out, value, strlen(value)) ||
           writeBytes(out, "\n", 1);
}

int p4cfg_set_batch(const P4CfgChange *changes, size_t count)
{
    char sections[MAX_ENTRIES][KEY_LEN], current[KEY_LEN] = "", tmp[1100];
    const char *names[MAX_ENTRIES];
    size_t nameLengths[MAX_ENTRIES];
    unsigned char wrote[MAX_ENTRIES] = {0};
    size_t length = 0;
    int failed = 0;
    char *text;
    SDL_IOStream *out;

    p4cfg_load();
    if (!changes || count == 0 || count > MAX_ENTRIES || p4cfg_path() == NULL)
        return -1;
    for (size_t i = 0; i < count; i++) {
        const char *key = changes[i].key, *value = changes[i].value;
        if (!key || !value)
            return -1;
        const char *dot = strchr(key, '.');
        if (!dot || dot == key || !dot[1] || strlen(key) >= KEY_LEN || strlen(value) >= VAL_LEN || strpbrk(value, "\r\n"))
            return -1;
        snprintf(sections[i], KEY_LEN, "%.*s", (int)(dot - key), key);
        names[i] = dot + 1;
        nameLengths[i] = strlen(names[i]);
    }
    text = SDL_LoadFile(sPath, &length);
    if (!text) {
        SDL_PathInfo info;
        if (SDL_GetPathInfo(sPath, &info))
            return -1;
        text = SDL_strdup("");
    }
    if (!text)
        return -1;
    snprintf(tmp, sizeof tmp, "%s.tmp", sPath);
    out = SDL_IOFromFile(tmp, "wb");
    if (!out) {
        SDL_free(text);
        return -1;
    }
    for (const char *p = text; *p && !failed;) {
        size_t n = strcspn(p, "\n"), bytes = n + (p[n] == '\n');
        const char *s = p, *end = p + n, *eq;
        while (s < end && isspace((unsigned char)*s))
            s++;
        int replacement = -1;
        if (s < end && *s == '[') {
            const char *close = memchr(s, ']', (size_t)(end - s));
            if (close) {
                for (size_t i = 0; i < count; i++)
                    if (!wrote[i] && !SDL_strcasecmp(current, sections[i])) {
                        failed |= writeKey(out, names[i], changes[i].value);
                        wrote[i] = 1;
                    }
                snprintf(current, sizeof current, "%.*s", (int)(close - s - 1), s + 1);
                char *trimmed = trim(current);
                if (trimmed != current)
                    memmove(current, trimmed, strlen(trimmed) + 1);
            }
        } else if (s < end && *s != '#' && *s != ';' && (eq = memchr(s, '=', (size_t)(end - s))) != NULL) {
            const char *ke = eq;
            while (ke > s && isspace((unsigned char)ke[-1]))
                ke--;
            for (size_t i = 0; i < count; i++)
                if (!SDL_strcasecmp(current, sections[i]) && (size_t)(ke - s) == nameLengths[i] &&
                    !SDL_strncasecmp(s, names[i], nameLengths[i]))
                    replacement = (int)i;
            if (replacement >= 0) {
                const char *v = eq + 1, *comment = end;
                while (v < end && (*v == ' ' || *v == '\t'))
                    v++;
                comment = inlineComment(v, end);
                if (comment == end && end > p && end[-1] == '\r')
                    comment--;
                failed |= writeBytes(out, p, (size_t)(v - p));
                failed |= writeBytes(out, changes[replacement].value, strlen(changes[replacement].value));
                failed |= writeBytes(out, comment, (size_t)(p + bytes - comment));
                wrote[replacement] = 1;
            }
        }
        if (replacement < 0)
            failed |= writeBytes(out, p, bytes);
        p += bytes;
    }
    int separated = length == 0 || text[length - 1] == '\n';
    for (size_t i = 0; i < count && !failed; i++)
        if (!wrote[i]) {
            if (!separated) {
                failed |= writeBytes(out, "\n", 1);
                separated = 1;
            }
            if (SDL_strcasecmp(current, sections[i])) {
                failed |= writeBytes(out, "\n[", 2) || writeBytes(out, sections[i], strlen(sections[i])) || writeBytes(out, "]\n", 2);
                snprintf(current, sizeof current, "%s", sections[i]);
            }
            failed |= writeKey(out, names[i], changes[i].value);
        }
    failed |= !SDL_FlushIO(out);
    failed |= !SDL_CloseIO(out);
    SDL_free(text);
    if (failed || !SDL_RenamePath(tmp, sPath)) {
        SDL_RemovePath(tmp);
        return -1;
    }
    for (size_t i = 0; i < count; i++)
        put(changes[i].key, changes[i].value);
    sVideoDone = 0;
    return 0;
}

int p4cfg_set_str(const char *key, const char *value)
{
    const P4CfgChange change = {key, value};
    return p4cfg_set_batch(&change, 1);
}
