/* Mod status rows (p4.ui.status): bounded plain-text lines drawn by native
 * code. One table serves every placement. No SDL, renderer or provider types
 * cross into game units. */
#ifndef P4PC_MOD_STATUS_H
#define P4PC_MOD_STATUS_H
#include <stdint.h>

#define P4_MOD_STATUS_ROWS 4      /* total, across all mods and placements */
#define P4_MOD_STATUS_TEXT_MAX 48 /* bytes including NUL, native RAW glyph codes */

enum P4ModStatusPlacement {
    P4_MOD_STATUS_CAMP = 0, /* the native Camp root */
    P4_MOD_STATUS_HUD = 1   /* an always-visible layer (drawn by its own seam) */
};

/* Game thread only. Replaces all of the owner's rows with count lines (count 0
 * clears them). Every line is validated (RAW glyphs, 1..47 bytes) and copied
 * into port-owned guest storage here, never per frame. Rows are ordered by
 * owner (load order). Returns 0, or -1 with the owner's previous rows intact
 * when a line is invalid or the lines do not fit beside earlier-loaded owners'
 * rows: a later owner never displaces an earlier one. */
int p4_mod_status_set(uint32_t owner, int placement, const char *const *lines, int count);
/* Camp root draw: nothing at all (no allocation, no guest write) without Camp
 * rows. Never calls into Lua. */
void p4_mod_status_draw(void);
/* Rows with one placement (the HUD seam enables itself only while > 0). */
int p4_mod_status_count(int placement);
/* HUD layer, drawn by the mod services' mainDrawFrameUpdate seam into the
 * last ordering-table bucket, below this frame's Camp rows. Nothing at all
 * without HUD rows. Never calls into Lua. */
void p4_mod_status_draw_hud(void);
#endif
