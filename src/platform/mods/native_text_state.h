/* Native glyph operations are game-thread-only and restore caller text state. */
#ifndef P4PC_NATIVE_TEXT_STATE_H
#define P4PC_NATIVE_TEXT_STATE_H
#include "p4pc/guest.h"
typedef struct FontState { uint32_t horizontal_scale, mode, state; uint16_t flags; } FontState;
static FontState save_font(uint32_t multiline)
{
    FontState saved = {*(uint32_t *)P4_G2H(0x007645a0u), *(uint32_t *)P4_G2H(0x00764594u),
        *(uint32_t *)P4_G2H(0x007637f8u), *(uint16_t *)P4_G2H(0x00763810u)};
    *(uint16_t *)P4_G2H(0x00763810u) = (uint16_t)((saved.flags & ~4u) | (multiline ? 4u : 0u));
    return saved;
}
static void restore_font(FontState saved)
{
    *(uint32_t *)P4_G2H(0x007645a0u) = saved.horizontal_scale;
    *(uint32_t *)P4_G2H(0x00764594u) = saved.mode;
    *(uint32_t *)P4_G2H(0x007637f8u) = saved.state;
    *(uint16_t *)P4_G2H(0x00763810u) = saved.flags;
}
#endif
