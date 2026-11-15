/* The PS2 / Golden mode switch (src/platform/config/game_mode.c): parsing, the environment over the file, the
 * PS2 default and the freeze on first query. No game data. */
#include <stdio.h>
#include <string.h>

#include "p4pc/game_mode.h"
#include "p4pc/os.h"

static int failures;
#define CHECK(c)                                                         \
    do {                                                                 \
        if (!(c)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                  \
        }                                                                \
    } while (0)

int main(int argc, char **argv)
{
    char reason[128];

    CHECK(p4_game_mode_parse(NULL, reason, sizeof reason) == P4_GAME_MODE_PS2 && !reason[0]);
    CHECK(p4_game_mode_parse("", reason, sizeof reason) == P4_GAME_MODE_PS2);
    CHECK(p4_game_mode_parse("ps2", reason, sizeof reason) == P4_GAME_MODE_PS2);
    CHECK(p4_game_mode_parse("PS2", reason, sizeof reason) == P4_GAME_MODE_PS2);
    CHECK(p4_game_mode_parse("golden", reason, sizeof reason) == P4_GAME_MODE_GOLDEN);
    CHECK(p4_game_mode_parse("Golden", reason, sizeof reason) == P4_GAME_MODE_GOLDEN);
    CHECK(p4_game_mode_parse("gold", reason, sizeof reason) == P4_GAME_MODE_INVALID && strstr(reason, "gold"));
    CHECK(p4_game_mode_parse("golden ", NULL, 0) == P4_GAME_MODE_INVALID);
    CHECK(!strcmp(p4_game_mode_name(P4_GAME_MODE_PS2), "ps2") && !strcmp(p4_game_mode_name(P4_GAME_MODE_GOLDEN), "golden"));

    /* The process: P4PC_CONFIG=none (no file), so only P4PC_MODE can select; the first query freezes. */
    p4_os_setenv("P4PC_CONFIG", "none", 1);
    if (argc > 1 && !strcmp(argv[1], "golden")) {
        p4_os_setenv("P4PC_MODE", "golden", 1);
        CHECK(p4_game_mode() == P4_GAME_MODE_GOLDEN);
        p4_os_setenv("P4PC_MODE", "ps2", 1);
        CHECK(p4_game_mode() == P4_GAME_MODE_GOLDEN);
    } else if (argc > 1 && !strcmp(argv[1], "invalid")) {
        p4_os_setenv("P4PC_MODE", "neither", 1);
        CHECK(p4_game_mode() == P4_GAME_MODE_INVALID && strstr(p4_game_mode_reason(), "neither"));
    } else {
        p4_os_setenv("P4PC_MODE", "", 1);
        CHECK(p4_game_mode() == P4_GAME_MODE_PS2 && !p4_game_mode_reason()[0]);
        p4_os_setenv("P4PC_MODE", "golden", 1);
        CHECK(p4_game_mode() == P4_GAME_MODE_PS2);
    }
    if (failures) fprintf(stderr, "%d failure(s)\n", failures);
    else printf("game_mode: ok\n");
    return failures != 0;
}
