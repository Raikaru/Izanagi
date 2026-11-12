/* CPU BF role proofs only. Native bank/source publication and gameplay parity
 * remain the composed cohort's gates, not claims of this synthetic fixture. */
#include "../src/platform/mods/game_elements_ai_state.h"
#include <assert.h>
#include <string.h>

typedef struct Fixture {
    P4AiScriptProduct script;
    P4AiCfgNode nodes[64];
    P4AiCfgRegion regions[2];
    uint8_t bytes[256];
    uint32_t count, query, argument, command_write;
} Fixture;
static uint32_t emit(Fixture *f, uint16_t opcode, int16_t operand)
{
    uint32_t pc = f->count++;
    assert(pc < 64u);
    f->nodes[pc].opcode = opcode;
    f->nodes[pc].operand = operand;
    f->nodes[pc].command_arity = opcode == 8u ? 1u : 0u;
    return pc;
}
static void make(Fixture *f, int id_query, int wide_buff)
{
    memset(f, 0, sizeof *f);
    f->script.nodes = f->nodes; f->script.regions = f->regions;
    f->script.bytes = f->bytes; f->script.local_int_count = 3;
    emit(f, 29, 0); emit(f, 30, 0); emit(f, 23, 0);
    uint32_t consumer = emit(f, 28, 0);
    emit(f, 30, 2); emit(f, 8, 0x203e); emit(f, 9, 0);
    f->nodes[consumer].first_region = 0; f->nodes[consumer].region_count = 1;
    f->regions[0] = (P4AiCfgRegion){4, 6};
    emit(f, 7, 1); emit(f, 29, 1);
    f->argument = emit(f, wide_buff ? 0 : 29, 0);
    if (wide_buff) {
        uint32_t raw = emit(f, P4_AI_UNUSED_OPCODE, 0);
        f->bytes[raw * 4u + 1u] = 0x80; /* Actual u32 literal8000, not a role. */
    }
    if (id_query) emit(f, 30, 1);
    f->query = emit(f, 8, wide_buff ? 0x20cc : id_query ? 0x209c : 0x2017);
    f->nodes[f->query].command_arity = id_query ? 2u : 1u;
    emit(f, 4, 0); emit(f, 22, 0);
    uint32_t branch = emit(f, 28, 0), first = f->count;
    emit(f, 29, 1); emit(f, 32, 0);
    emit(f, 29, 0); f->command_write = emit(f, 32, 2); emit(f, 9, 0);
    emit(f, 29, 0); emit(f, 32, 0); emit(f, 9, 0);
    f->nodes[branch].first_region = 1; f->nodes[branch].region_count = 1;
    f->regions[1] = (P4AiCfgRegion){first, f->count - 1u};
    f->script.code_count = f->count;
    for (uint32_t pc = 0; pc < f->count; ++pc)
        f->nodes[pc].successor[0] = f->nodes[pc].opcode == 9u ? P4_AI_EXIT : pc + 1u;
}
static void basic_break_or_chain(void)
{
    Fixture f = {0};
    f.script.nodes = f.nodes; f.script.regions = f.regions;
    f.script.bytes = f.bytes; f.script.local_int_count = 3;
    emit(&f, 29, 0); emit(&f, 30, 0); emit(&f, 23, 0);
    uint32_t consumer = emit(&f, 28, 0);
    emit(&f, 30, 2); emit(&f, 8, 0x203e); emit(&f, 9, 0); emit(&f, 7, 0);
    f.nodes[consumer].first_region = 0; f.nodes[consumer].region_count = 1;
    f.regions[0] = (P4AiCfgRegion){4, 6};
    const uint16_t commands[] = {0x20eb, 0x2017, 0x2016, 0x2015};
    uint32_t queries[4];
    for (unsigned i = 0; i < 4; ++i) {
        emit(&f, 29, 1); emit(&f, 29, 0);
        queries[i] = emit(&f, 8, commands[i]); emit(&f, 4, 0); emit(&f, 22, 0);
        if (i) emit(&f, 20, 0);
    }
    uint32_t branch = emit(&f, 28, 0), first = f.count;
    emit(&f, 29, 1); emit(&f, 32, 0); emit(&f, 29, 0); emit(&f, 32, 2); emit(&f, 9, 0);
    emit(&f, 29, 0); emit(&f, 32, 0); emit(&f, 9, 0);
    f.nodes[branch].first_region = 1; f.nodes[branch].region_count = 1;
    f.regions[1] = (P4AiCfgRegion){first, f.count - 1u};
    f.script.code_count = f.count;
    for (uint32_t pc = 0; pc < f.count; ++pc)
        f.nodes[pc].successor[0] = f.nodes[pc].opcode == 9u ? P4_AI_EXIT : pc + 1u;
    assert(!p4_ai_script_basic_roles(&f.script));
    for (unsigned i = 0; i < 4; ++i) assert(f.nodes[queries[i]].query_role == P4_GAME_AI_QUERY_BASIC);
}
int main(void)
{
    basic_break_or_chain();
    Fixture f;
    make(&f, 0, 0);
    assert(!p4_ai_script_basic_roles(&f.script));
    assert(f.nodes[f.query].query_role == P4_GAME_AI_QUERY_BASIC);
    make(&f, 1, 0);
    assert(!p4_ai_script_basic_roles(&f.script));
    assert(f.nodes[f.query].query_role == P4_GAME_AI_QUERY_BASIC);
    make(&f, 0, 0);
    f.nodes[f.query].query_role = P4_GAME_AI_QUERY_AVAILABLE;
    assert(p4_ai_script_basic_roles(&f.script) == P4_TABLE_CONFLICT);
    make(&f, 0, 0);
    f.nodes[5].operand = 0x20cc; /* No actual command-result consumer. */
    assert(p4_ai_script_basic_roles(&f.script) == P4_TABLE_INVALID);
    make(&f, 0, 0);
    f.nodes[5].operand = 0x20cc;
    f.nodes[f.query].query_role = P4_GAME_AI_QUERY_AVAILABLE; /* Explicit qualified site. */
    assert(!p4_ai_script_basic_roles(&f.script));
    assert(f.nodes[f.query].query_role == P4_GAME_AI_QUERY_AVAILABLE);
    make(&f, 0, 0);
    f.nodes[f.argument].operand = 1; /* Literal Fire is a profile query, not Phys0. */
    assert(!p4_ai_script_basic_roles(&f.script));
    assert(!f.nodes[f.query].query_role);
    make(&f, 0, 1);
    assert(!p4_ai_script_basic_roles(&f.script));
    assert(!f.nodes[f.query].query_role); /* Buff8000 does not become an attack. */
    make(&f, 0, 0);
    f.nodes[5].operand = 0x20cc;
    f.nodes[f.command_write - 1u].opcode = 7;
    f.nodes[f.command_write].opcode = 8;
    f.nodes[f.command_write].operand = 0x203d;
    f.nodes[f.command_write].command_arity = 0;
    assert(!p4_ai_script_basic_roles(&f.script));
    assert(f.nodes[f.query].query_role == P4_GAME_AI_QUERY_BASIC);
    return 0;
}
