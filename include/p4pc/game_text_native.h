#ifndef P4PC_GAME_TEXT_NATIVE_H
#define P4PC_GAME_TEXT_NATIVE_H
#include <stdint.h>
/* MESSAGE recipe using current native font/style/spacing and horizontal scale,
 * no approximate character-width formula or legacy123/128 scale substitution.
 * Width is native itfMesListWidth in sixteenth-pixel units, unscaled.
 * multiline uses the active native line height. Font state is scoped/restored.
 * No registry allocation, text copy or language callback occurs. */
int32_t p4_game_tables_text_measure(uint32_t stream, uint32_t style, uint32_t multiline);
void p4_game_tables_text_draw(uint32_t stream, float x, float y, float depth,
    uint32_t color, uint32_t style, uint32_t effect, uint32_t flags, uint32_t multiline);
/* Build now; transfer glyph-list ownership to actual00279350/00279300 queue
 * recipe. Validated command-free streams are not retained by the list. Native
 * font/cache lifetime and queue quiescence remain the game's original owners. */
void p4_game_tables_text_queue(uint32_t stream, float x, float y, float depth,
    uint32_t color, uint32_t style, uint32_t effect, uint32_t flags, uint32_t multiline, uint32_t queue);
/* Expanded first entry plus the original banked second entry: same append,
 * layout and release recipe as00279470, including record-zero omission. */
void p4_game_tables_text_draw_pair(uint32_t stream, float x, float y, float depth,
    uint32_t color, uint32_t style, uint32_t effect, uint32_t flags, uint32_t multiline,
    int32_t second_bank, int32_t second_record);
/* depth above is native00274a20→raster Z, NEVER glyph-size scaling.
 * Explicit shrink/collapse uses the stock128-based glyph/kerning geometry
 * words7645a0/7645a4 during the SAME MESSAGE builder. Touched state is scoped
 * and restored, style1 is the native slot1 recipe, and depth stays independent.
 * Geometry words are0..128:128 normal,0 collapsed with no list allocation.
 * Native font/metrics rules and integer rounding apply, not an approximate
 * character-width model or a substitute renderer. Measure returns native
 * sixteenth-pixel width (-1 invalid geometry); draw returns0/-1. */
int32_t p4_game_tables_text_measure_geometry(uint32_t stream, uint32_t style,
    uint32_t multiline, uint32_t horizontal_128, uint32_t vertical_128);
int32_t p4_game_tables_text_draw_geometry(uint32_t stream, float x, float y, float depth,
    uint32_t color, uint32_t style, uint32_t effect, uint32_t flags, uint32_t multiline,
    uint32_t horizontal_128, uint32_t vertical_128);
#endif
