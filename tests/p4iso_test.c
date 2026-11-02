/* The player's disc (src/platform/config/p4iso.c): the settings file keeps the disc's path (p4cfg_set_str adds the key, replaces it, keeps the other sections and
 * keys), and, with P4PC_ISO naming the retail image, the executable is found on the disc. The dialog itself is not exercised. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/platform/config/p4cfg.h"
#include "../src/platform/config/p4iso.h"
#include "p4pc/os.h"

static int failures;

#define CHECK(c)                                                         \
    do {                                                                 \
        if (!(c)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                  \
        }                                                                \
    } while (0)

static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *t;
    long n;

    if (f == NULL) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    t = calloc((size_t)n + 1, 1);
    if (t != NULL && fread(t, 1, (size_t)n, f) != (size_t)n) {
        free(t);
        t = NULL;
    }
    fclose(f);
    return t;
}

int main(void)
{
    char path[512];
    FILE *f;
    char *t;
    const char *iso = getenv("P4PC_ISO");
    long long off = 0, size = 0;

    snprintf(path, sizeof path, "p4iso_test_%d.ini", (int)(size_t)&failures & 0xffff);
    f = fopen(path, "wb");
    CHECK(f != NULL);
    fputs("# mine\n[video]\nresolution = scale:2\n\n[audio]\nvolume = 50\n", f);
    fclose(f);
    p4_os_setenv("P4PC_CONFIG", path, 1);
    p4cfg_clear();
    p4cfg_load();
    CHECK(p4cfg_video()->scale == 2);

    /* a key in a section that is not there: the section is appended, the rest stays */
    CHECK(p4cfg_set_str("game.iso", "D:/games/Persona 4 (USA).iso") == 0);
    t = slurp(path);
    CHECK(t != NULL && strstr(t, "# mine") != NULL && strstr(t, "resolution = scale:2") != NULL && strstr(t, "volume = 50") != NULL);
    CHECK(t != NULL && strstr(t, "[game]\niso = D:/games/Persona 4 (USA).iso\n") != NULL);
    free(t);

    /* the same key again: replaced, not repeated */
    CHECK(p4cfg_set_str("game.iso", "E:/other.iso") == 0);
    t = slurp(path);
    CHECK(t != NULL && strstr(t, "E:/other.iso") != NULL && strstr(t, "D:/games") == NULL);
    CHECK(t != NULL && strstr(strstr(t, "iso = ") + 1, "iso = ") == NULL);
    free(t);

    /* a key in an existing section, between others */
    CHECK(p4cfg_set_str("audio.volume", "80") == 0);
    t = slurp(path);
    CHECK(t != NULL && strstr(t, "volume = 80") != NULL && strstr(t, "volume = 50") == NULL && strstr(t, "iso = E:/other.iso") != NULL);
    free(t);
    p4cfg_clear();
    p4cfg_load();
    CHECK(strcmp(p4cfg_str("game.iso", ""), "E:/other.iso") == 0 && p4cfg_int("audio.volume", 0) == 80);
    remove(path);

    /* One binding operation is one atomic write even with conflicts/reset.
     * Validate the whole batch before touching the original or memory table. */
    {
        const P4CfgChange changes[] = {{"video.resolution", "scale:3"}, {"audio.volume", "35"}, {"input.kb.cross", "K, none"}};
        f = fopen(path, "wb");
        CHECK(f != NULL);
        fputs("# keep\r\n[video]\r\nresolution = scale:2 ; inline\r\nunknown = value\r\n[audio]\r\nvolume = 80\r\n", f);
        fclose(f);
        CHECK(p4cfg_set_batch(changes, 3) == 0);
        CHECK(p4cfg_video()->scale == 3 && p4cfg_int("audio.volume", 0) == 35);
        t = slurp(path);
        CHECK(t && strstr(t, "# keep\r\n") && strstr(t, "resolution = scale:3 ; inline\r\n") && strstr(t, "unknown = value\r\n") &&
              strstr(t, "kb.cross = K, none"));
        const P4CfgChange invalid[] = {{"video.resolution", "scale:4"}, {"audio.volume", "bad\nvalue"}};
        CHECK(p4cfg_set_batch(invalid, 2) == -1);
        char *after = slurp(path);
        CHECK(t && after && !strcmp(t, after));
        CHECK(p4cfg_video()->scale == 3 && p4cfg_int("audio.volume", 0) == 35);
        free(after);
        free(t);
        remove(path);
    }

    /* a long path survives (it was cut at 120 bytes once) */
    {
        char longp[400];

        memset(longp, 'a', sizeof longp - 1);
        longp[sizeof longp - 1] = '\0';
        f = fopen(path, "wb");
        fclose(f);
        CHECK(p4cfg_set_str("game.iso", longp) == 0);
        p4cfg_clear();
        p4cfg_load();
        CHECK(strlen(p4cfg_str("game.iso", "")) == sizeof longp - 1);
        remove(path);
    }

    CHECK(p4iso_locate_elf("no such file.iso", &off, &size) == -1);
    if (iso != NULL && iso[0] != '\0') {
        CHECK(p4iso_locate_elf(iso, &off, &size) == 0 && size > 1000000 && off > 0);
        printf("p4iso_test: SLUS_217.82 at %lld, %lld bytes\n", off, size);
    }
    if (failures == 0) {
        printf("p4iso_test: ok\n");
    }
    return failures != 0;
}
