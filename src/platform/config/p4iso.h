/* The player's disc: where the game finds its files when started with no arguments (docs/platform/playing.md).
 *
 * The disc image is the player's own, named by P4PC_ISO, else by `iso` in the `[game]` section of p4pc.ini, else asked for with the system's file dialog
 * (the answer is checked: it must be a disc with SLUS_217.82 in its root, and is saved to p4pc.ini). The game's executable is read from that disc
 * (SLUS_217.82, the PS2 boot file) and kept beside p4pc.ini, so the player never handles it. Started with an executable path (every test and gate does),
 * nothing here runs. */
#ifndef P4PC_PLATFORM_CONFIG_P4ISO_H
#define P4PC_PLATFORM_CONFIG_P4ISO_H

#include <stddef.h>
#include "p4cfg.h"

/* Makes P4PC_ISO point at the player's disc (asking when it is not known) and writes the executable's path into elf (cap bytes). Returns 0, or -1 when there
 * is no usable disc (the reason has been shown to the player, in a box when there is a desktop, and on stderr). */
int p4iso_prepare(char *elf, size_t cap);

/* Desktop player run: stdout/stderr go to p4pc.log beside the INI; the previous run's log is kept as p4pc.prev.log (and as p4pc.unclean.log when it
 * did not end with the clean-exit line below), so a crash followed by a restart does not lose the crash's log. Windows also closes its console.
 * Linux names that path for native crash reports. Automated runs leave their runner's streams untouched. Call before anything prints. */
void p4iso_player_log(void);

/* The line the player writes last when the run ends by itself (an atexit handler in src/platform/diag/p4diag.c); a log without it is an unclean end. */
#define P4_LOG_CLEAN_EXIT "p4diag: clean exit"

/* Moves the log at `path` ( .../p4pc.log, .../boot.log ) aside before a new run writes it: to .../p4pc.prev.log, replacing the one there, and, when the
 * old log lacks P4_LOG_CLEAN_EXIT, also to .../p4pc.unclean.log. No file, or an empty one, is not an error. Returns 0, or -1 when the old log could not
 * be moved (the caller then truncates it as before). */
int p4iso_rotate_log(const char *path);

/* The pure part, for the test: the executable's place on the disc image at `iso` (an ISO 9660 volume with SLUS_217.82 in its root). Returns 0 with its byte
 * offset and size in the image, or -1. */
int p4iso_locate_elf(const char *iso, long long *offset, long long *size);


#endif
