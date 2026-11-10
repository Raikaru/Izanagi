/* See p4iso.h. */
#include "p4iso.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#endif
#ifndef _WIN32
#include <unistd.h>
#endif

#include "../files/iso9660.h"
#include "p4cfg.h"
#include "p4pc/os.h"

#define ELF_NAME "SLUS_217.82"

typedef struct {
    long long offset, size;
    int found;
} ElfFind;

static void visit(void *ctx, const char *path, long long offset, long long size)
{
    ElfFind *f = ctx;

    if (strcmp(path, ELF_NAME) == 0) {
        f->offset = offset;
        f->size = size;
        f->found = 1;
    }
}

int p4iso_locate_elf(const char *iso, long long *offset, long long *size)
{
    FILE *fp = fopen(iso, "rb");
    ElfFind f = {0, 0, 0};

    if (fp == NULL) {
        return -1;
    }
    if (iso_walk(fp, 0, visit, &f) != 0 || !f.found || f.size <= 0) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    *offset = f.offset;
    *size = f.size;
    return 0;
}

/* ---- the settings file ---------------------------------------------------------------------- */

static void settings_path(char *out, size_t cap)
{
    const char *p = p4cfg_path();

    if (p != NULL) {
        snprintf(out, cap, "%s", p);
        return;
    }
    {
        char *pref = SDL_GetPrefPath("p4pc", "Persona4");

        snprintf(out, cap, "%sp4pc.ini", pref != NULL ? pref : "");
        SDL_free(pref);
    }
}


/* ---- asking ----------------------------------------------------------------------------------- */

typedef struct {
    volatile int done;
    char path[1024];
} Pick;

static void SDLCALL picked(void *ud, const char *const *list, int filter)
{
    Pick *p = ud;

    (void)filter;
    if (list != NULL && list[0] != NULL) {
        snprintf(p->path, sizeof p->path, "%s", list[0]);
    }
    p->done = 1;
}

static void show_problem(const char *text)
{
    fprintf(stderr, "p4pc: %s\n", text);
    if (p4cfg_player_run()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Persona 4", text, NULL);
    }
}

/* The dialog, until a usable disc is chosen or the player cancels. 0 with the path in out. */
static int ask(char *out, size_t cap)
{
    static const SDL_DialogFileFilter filters[] = {{"Disc images (*.iso)", "iso;ISO"}, {"All files", "*"}};

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        show_problem("There is no disc image set and no dialog can be shown. Set the P4PC_ISO environment variable, or `iso` under [game] in p4pc.ini, to the path of your Persona 4 (USA) disc image.");
        return -1;
    }
    for (;;) {
        Pick pick;
        long long off, size;

        memset(&pick, 0, sizeof pick);
        SDL_ShowOpenFileDialog(picked, &pick, NULL, filters, 2, NULL, false);
        while (!pick.done) {
            SDL_PumpEvents();
            SDL_Delay(30);
        }
        if (pick.path[0] == '\0') {
            show_problem("No disc image was chosen. Start the game again and pick the .iso of your own Shin Megami Tensei: Persona 4 (USA) disc (SLUS-21782).");
            return -1;
        }
        if (p4iso_locate_elf(pick.path, &off, &size) == 0) {
            snprintf(out, cap, "%s", pick.path);
            return 0;
        }
        show_problem("That file is not a Persona 4 (USA) disc image: it has no SLUS_217.82 in its root. Pick the .iso of Shin Megami Tensei: Persona 4 (USA, SLUS-21782).");
    }
}

/* The disc is larger than 2 GB and the executable is near its end. */
static int seek64(FILE *f, long long off)
{
#ifdef _WIN32
    return _fseeki64(f, off, SEEK_SET);
#else
    return fseeko(f, (off_t)off, SEEK_SET);
#endif
}

static int extract_elf(const char *iso, char *elf, size_t cap)
{
    char dir[1024], *slash, *buf;
    long long off, size;
    FILE *in, *out;
    long cur = -1;

    if (p4iso_locate_elf(iso, &off, &size) != 0) {
        char msg[1200];

        snprintf(msg, sizeof msg, "The disc image %s has no SLUS_217.82 in its root: it is not a Persona 4 (USA, SLUS-21782) disc image.", iso);
        show_problem(msg);
        return -1;
    }
    settings_path(dir, sizeof dir);
    slash = strrchr(dir, '/');
    if (slash == NULL) {
        slash = strrchr(dir, '\\');
    }
    if (slash != NULL) {
        slash[1] = '\0';
    } else {
        dir[0] = '\0';
    }
    snprintf(elf, cap, "%s%s", dir, ELF_NAME);
    out = fopen(elf, "rb");
    if (out != NULL) {
        fseek(out, 0, SEEK_END);
        cur = ftell(out);
        fclose(out);
    }
    if (cur == (long)size) {
        return 0; /* the one from this disc, as before */
    }
    in = fopen(iso, "rb");
    buf = malloc((size_t)size);
    if (in == NULL || buf == NULL || seek64(in, off) != 0 || fread(buf, 1, (size_t)size, in) != (size_t)size) {
        if (in != NULL) {
            fclose(in);
        }
        free(buf);
        show_problem("The game's executable could not be read from the disc image.");
        return -1;
    }
    fclose(in);
    out = fopen(elf, "wb");
    if (out == NULL || fwrite(buf, 1, (size_t)size, out) != (size_t)size) {
        char msg[1200];

        if (out != NULL) {
            fclose(out);
        }
        free(buf);
        snprintf(msg, sizeof msg, "Cannot write the game's executable to %s.", elf);
        show_problem(msg);
        return -1;
    }
    fclose(out);
    free(buf);
    fprintf(stderr, "p4pc: read %s from the disc (%lld bytes) to %s\n", ELF_NAME, size, elf);
    return 0;
}

void p4iso_player_log(void)
{
    char path[1024], *slash;

    if (!p4cfg_player_run()) {
        return;
    }
    p4cfg_load();
    settings_path(path, sizeof path);
    slash = strrchr(path, '/');
    if (slash == NULL) {
        slash = strrchr(path, '\\');
    }
    if (slash == NULL) {
        return;
    }
    snprintf(slash + 1, sizeof path - (size_t)(slash + 1 - path), "p4pc.log");
#ifndef _WIN32
    p4_os_setenv("P4PC_LOG_FILE", path, 1);
#endif
    if (freopen(path, "w", stdout) == NULL) {
        return;
    }
    setvbuf(stdout, NULL, _IOLBF, 4096);
#ifdef _WIN32
    _dup2(_fileno(stdout), _fileno(stderr));
    setvbuf(stderr, NULL, _IONBF, 0);
    FreeConsole();
#else
    dup2(fileno(stdout), fileno(stderr));
    setvbuf(stderr, NULL, _IONBF, 0);
#endif
}

/* The memory card folder of a player: beside the settings (the game's own save screens write here); a test or a gate names its own (P4PC_SAVE_DIR). */
static void default_save_dir(void)
{
    char dir[1024], *slash;

    if (getenv("P4PC_SAVE_DIR") != NULL && getenv("P4PC_SAVE_DIR")[0] != '\0') {
        return;
    }
    settings_path(dir, sizeof dir);
    slash = strrchr(dir, '/');
    if (slash == NULL) {
        slash = strrchr(dir, '\\');
    }
    if (slash == NULL) {
        return;
    }
    snprintf(slash + 1, sizeof dir - (size_t)(slash + 1 - dir), "save");
    p4_os_setenv("P4PC_SAVE_DIR", dir, 0);
}

int p4iso_prepare(char *elf, size_t cap)
{
    const char *iso;
    char chosen[1024];

    p4cfg_load();
    iso = getenv("P4PC_ISO");
    if (iso == NULL || iso[0] == '\0') {
        iso = p4cfg_str("game.iso", NULL);
        if (iso != NULL) {
            long long off, size;

            if (p4iso_locate_elf(iso, &off, &size) != 0) {
                fprintf(stderr, "p4pc: the disc image %s from the settings file is gone or is not a Persona 4 disc image\n", iso);
                iso = NULL;
            }
        }
        if (iso == NULL) {
            if (!p4cfg_player_run()) {
                show_problem("No disc image: set P4PC_ISO, or `iso` under [game] in the settings file.");
                return -1;
            }
            if (ask(chosen, sizeof chosen) != 0) {
                return -1;
            }
            iso = chosen;
            if (p4cfg_set_str("game.iso", iso) != 0) {
                fprintf(stderr, "p4pc: could not save the disc image's path to the settings file\n");
            }
        }
        p4_os_setenv("P4PC_ISO", iso, 1);
    }
    if (p4cfg_player_run()) {
        default_save_dir();
    }
    return extract_elf(iso, elf, cap);
}
