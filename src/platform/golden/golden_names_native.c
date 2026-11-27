#include "p4pc/golden_names_native.h"
#include "p4pc/game_mode.h"
#include "p4pc/game_table_extents.h"
#include <stdint.h>

static uint32_t name_at(uint32_t slot, uint32_t stride, uint32_t id)
{
    uint32_t base, rows;
    if (p4_game_mode() != P4_GAME_MODE_GOLDEN ||
        !p4_game_table_extent_rows(slot, stride, &base, &rows) ||
        !base || id >= rows || id > (UINT32_MAX - base) / stride) return 0;
    return base + id * stride;
}
uint32_t p4_golden_skill_name(uint32_t id)
{ return name_at(0x00764538u, P4_GOLDEN_SKILL_NAME_SIZE, id); }
uint32_t p4_golden_persona_name(uint32_t id)
{ return name_at(0x00764530u, P4_GOLDEN_PERSONA_NAME_SIZE, id); }
