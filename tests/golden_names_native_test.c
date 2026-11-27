#include "p4pc/golden_names_native.h"
#include "p4pc/game_mode.h"
#include <assert.h>
#include <stdint.h>

static P4GameMode mode = P4_GAME_MODE_PS2;
static unsigned calls;
static uint32_t base = 0x100000u, rows = 4;
P4GameMode p4_game_mode(void) { return mode; }
int p4_game_table_extent_rows(uint32_t slot, uint32_t stride, uint32_t *out, uint32_t *count)
{
    ++calls;
    assert((slot == 0x00764538u && stride == 23u) || (slot == 0x00764530u && stride == 21u));
    *out = base; *count = rows;
    return 1;
}
int main(void)
{
    assert(!p4_golden_skill_name(0) && !p4_golden_persona_name(0) && !calls);
    mode = P4_GAME_MODE_GOLDEN;
    assert(p4_golden_skill_name(3) == base + 3u * 23u);
    assert(p4_golden_persona_name(3) == base + 3u * 21u);
    assert(!p4_golden_skill_name(4));
    base = UINT32_MAX - 10u;
    assert(!p4_golden_persona_name(1));
    base = 0;
    assert(!p4_golden_skill_name(0));
    return 0;
}
