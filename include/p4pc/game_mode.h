/* Which game the port plays: the PS2 game from the player's disc (the default and primary mode) or Golden mode,
 * built at run time from the player's own Persona 4 Golden (Steam) install (docs/golden_mode/README.md).
 *
 * The mode is `[game] mode` in p4pc.ini (`ps2` or `golden`), overridden by P4PC_MODE. It is read once, on the
 * first query, and never changes for the process: everything Golden-specific asks this switch, and PS2 code
 * paths are not edited for Golden behaviour. An unknown value is refused, never read as PS2. */
#ifndef P4PC_GAME_MODE_H
#define P4PC_GAME_MODE_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum P4GameMode {
    P4_GAME_MODE_PS2 = 0,
    P4_GAME_MODE_GOLDEN = 1,
    P4_GAME_MODE_INVALID = -1
} P4GameMode;

/* Pure parse of a configured value: NULL or empty is PS2; "ps2" and "golden" (ASCII case-insensitive) select
 * their mode; anything else is P4_GAME_MODE_INVALID with reason set. */
P4GameMode p4_game_mode_parse(const char *value, char *reason, size_t capacity);

/* The process's mode, resolved and frozen on the first call from P4PC_MODE / `game.mode`. P4_GAME_MODE_INVALID
 * (with p4_game_mode_reason) when the configured value is unknown; callers refuse to start then. */
P4GameMode p4_game_mode(void);
const char *p4_game_mode_reason(void);
const char *p4_game_mode_name(P4GameMode mode);

#ifdef __cplusplus
}
#endif
#endif
