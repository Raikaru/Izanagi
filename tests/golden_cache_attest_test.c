/* Synthetic input ownership metadata, never a game payload. */
#include "../src/platform/mods/game_table_golden_attest.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
const char *p4_game_table_input_label(P4GameTableInputKind kind)
{
    static const char *labels[] = {"init/itemtbl.bin", "battle/SKILL.TBL", "battle/UNIT.TBL", "battle/MSG.TBL",
        "battle/PERSONA.TBL", "battle/ENCOUNT.TBL", "battle/EFFECT.TBL", "battle/MODEL.TBL", "battle/AICALC.TBL"};
    return labels[kind];
}
int main(void)
{
    P4GameTableInput *inputs[P4_TABLE_INPUT_COUNT];
    char key[65]; memset(key, 'a', 64); key[64] = 0;
    for (unsigned i = 0; i < P4_TABLE_INPUT_COUNT; ++i) {
        inputs[i] = calloc(1, sizeof *inputs[i]); assert(inputs[i]);
        inputs[i]->kind = (P4GameTableInputKind)i; inputs[i]->sealed = 1;
        snprintf(inputs[i]->golden_origin, sizeof inputs[i]->golden_origin, "golden-cache/%s/%s", key,
                 p4_game_table_input_label(inputs[i]->kind));
    }
    assert(!p4_game_table_golden_inputs_attest(inputs, P4_TABLE_INPUT_COUNT, 0));
    assert(!p4_game_table_golden_inputs_attest(inputs, P4_TABLE_INPUT_COUNT - 1, 1));
    assert(p4_game_table_golden_inputs_attest(inputs, P4_TABLE_INPUT_COUNT, 1));
    inputs[1]->golden_origin[13] = 'b';
    assert(!p4_game_table_golden_inputs_attest(inputs, P4_TABLE_INPUT_COUNT, 1));
    inputs[1]->golden_origin[13] = 'a'; inputs[1]->kind = P4_TABLE_INPUT_ITEM;
    assert(!p4_game_table_golden_inputs_attest(inputs, P4_TABLE_INPUT_COUNT, 1));
    inputs[1]->kind = P4_TABLE_INPUT_SKILL; inputs[1]->sealed = 0;
    assert(!p4_game_table_golden_inputs_attest(inputs, P4_TABLE_INPUT_COUNT, 1));
    inputs[1]->sealed = 1; inputs[1]->golden_origin[77] = 'x';
    assert(!p4_game_table_golden_inputs_attest(inputs, P4_TABLE_INPUT_COUNT, 1));
    memset(inputs[1]->golden_origin, 'a', sizeof inputs[1]->golden_origin);
    assert(!p4_game_table_golden_inputs_attest(inputs, P4_TABLE_INPUT_COUNT, 1));
    for (unsigned i = 0; i < P4_TABLE_INPUT_COUNT; ++i) free(inputs[i]);
    puts("golden_cache_attest: ok"); return 0;
}
