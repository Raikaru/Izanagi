#include "golden_bf.h"
#include <string.h>

int p4_golden_bf_tables(P4GameMode mode, P4GoldenBfTables *out,
                        const P4GoldenBfNativeGroup original[4])
{
    static const uint32_t counts[4] = {121,51,276,61};
    static const uint32_t added[] = {148,8468,8469,8470,8471,8472,8473,8474,8479,8481,8484,8485};
    if (mode == P4_GAME_MODE_PS2) return 0;
    if (mode != P4_GAME_MODE_GOLDEN || !out || !original) return -1;
    for (unsigned group = 0; group < 4; ++group)
        if (!original[group].commands || original[group].count != counts[group]) return -1;
    memset(out,0,sizeof *out);
    P4GoldenBfCommand *destinations[] = {out->common,out->field,out->ai,out->battle};
    for (unsigned group = 0; group < 4; ++group) {
        for (unsigned i = 0; i < counts[group]; ++i) {
            const P4GoldenBfNativeCommand *entry = original[group].commands + i;
            destinations[group][i].native_function = entry->function;
            destinations[group][i].arity = entry->arity;
        }
    }
    for (unsigned i = 0; i < sizeof added / sizeof added[0]; ++i) {
        uint32_t command = added[i];
        P4GoldenBfCommand *entry = destinations[command >> 12] + (command & 0xfff);
        entry->native_function = 0;
        entry->arity = (uint32_t)p4_golden_ai_arity(mode,command);
        entry->golden = p4_golden_ai_execute;
    }
    return 1;
}
const P4GoldenBfCommand *p4_golden_bf_resolve(const P4GoldenBfTables *tables, uint32_t command)
{
    static const uint32_t counts[] = {192,51,294,61};
    uint32_t group = command >> 12, index = command & 0xfff;
    if (!tables || group >= 4 || index >= counts[group]) return NULL;
    const P4GoldenBfCommand *groups[] = {tables->common,tables->field,tables->ai,tables->battle};
    const P4GoldenBfCommand *entry = groups[group] + index;
    return entry->native_function || entry->golden ? entry : NULL;
}
