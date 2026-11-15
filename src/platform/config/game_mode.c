/* The PS2 / Golden mode switch (include/p4pc/game_mode.h). */
#include "p4pc/game_mode.h"
#include "p4cfg.h"
#include <stdio.h>
#include <string.h>

static int ascii_equal(const char *a, const char *b)
{
    for (; *a && *b; ++a, ++b) {
        char x = *a >= 'A' && *a <= 'Z' ? (char)(*a + 32) : *a, y = *b >= 'A' && *b <= 'Z' ? (char)(*b + 32) : *b;
        if (x != y) return 0;
    }
    return !*a && !*b;
}

P4GameMode p4_game_mode_parse(const char *value, char *reason, size_t capacity)
{
    if (reason && capacity) reason[0] = 0;
    if (!value || !value[0] || ascii_equal(value, "ps2")) return P4_GAME_MODE_PS2;
    if (ascii_equal(value, "golden")) return P4_GAME_MODE_GOLDEN;
    if (reason && capacity)
        snprintf(reason, capacity, "unknown game mode \"%.40s\" (game.mode / P4PC_MODE is ps2 or golden)", value);
    return P4_GAME_MODE_INVALID;
}

static int resolved;
static P4GameMode mode;
static char reason[96];

P4GameMode p4_game_mode(void)
{
    if (!resolved) {
        p4cfg_load();
        mode = p4_game_mode_parse(p4cfg_get("game.mode"), reason, sizeof reason);
        resolved = 1;
    }
    return mode;
}

const char *p4_game_mode_reason(void)
{
    return p4_game_mode() == P4_GAME_MODE_INVALID ? reason : "";
}

const char *p4_game_mode_name(P4GameMode value)
{
    return value == P4_GAME_MODE_PS2 ? "ps2" : value == P4_GAME_MODE_GOLDEN ? "golden" : "invalid";
}
