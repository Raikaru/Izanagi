#ifndef P4PC_GAME_ELEMENTS_AI_COMMANDS_H
#define P4PC_GAME_ELEMENTS_AI_COMMANDS_H
#include <stdint.h>
#include "p4pc/mod_memory.h"
/* Immutable executable contracts, not resource-provider identities.
 * Qualified D_00748530 groups in SLUS_217.82 sha1
 * 4eeec0360cf2715535d9f7e52eb69d786fb0158c; decomp/docs/dispatch_tables.md:47–575.
 * Tables0063D070/0063D440/0063D5E0/0063DE80 hold {function,paramCount}.
 * 255 marks NULL, not a supported command. Native prepare verifies the actual
 * used dispatch arities before publication; the interpreter never trusts a
 * guessed pop count or rereads this immutable contract per candidate. */
static const uint8_t p4_ai_command_arity_0[121] = {
    1u,0u,0u,1u,255u,255u,3u,2u,1u,1u,1u,1u,1u,1u,1u,0u,
    1u,1u,1u,1u,1u,1u,2u,0u,8u,2u,1u,1u,1u,1u,2u,0u,
    4u,1u,2u,2u,3u,3u,5u,5u,1u,1u,2u,255u,255u,255u,255u,255u,
    4u,4u,3u,0u,0u,0u,0u,2u,1u,1u,0u,1u,0u,3u,1u,2u,
    4u,2u,2u,2u,1u,3u,3u,2u,0u,0u,0u,0u,0u,0u,0u,0u,
    1u,0u,1u,2u,2u,3u,0u,0u,0u,0u,1u,1u,4u,1u,1u,1u,
    2u,1u,1u,2u,1u,0u,1u,0u,0u,0u,0u,1u,0u,2u,0u,1u,
    1u,1u,1u,1u,1u,1u,2u,0u,0u
};
static const uint8_t p4_ai_command_arity_1[51] = {
    0u,1u,0u,0u,1u,0u,1u,2u,5u,1u,2u,2u,2u,2u,2u,1u,
    2u,2u,2u,2u,0u,0u,2u,2u,2u,0u,0u,0u,0u,0u,0u,0u,
    2u,2u,0u,0u,0u,0u,0u,0u,0u,1u,2u,2u,0u,1u,3u,3u,
    1u,1u,0u
};
static const uint8_t p4_ai_command_arity_2[276] = {
    1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,
    1u,1u,0u,0u,0u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,
    1u,1u,1u,1u,1u,1u,1u,0u,0u,0u,1u,1u,0u,0u,0u,0u,
    1u,1u,1u,0u,0u,1u,1u,1u,1u,1u,1u,1u,1u,0u,1u,0u,
    0u,1u,0u,0u,0u,0u,0u,0u,0u,1u,1u,1u,1u,1u,1u,1u,
    1u,1u,1u,1u,1u,1u,1u,1u,1u,2u,2u,2u,2u,2u,2u,2u,
    2u,2u,2u,2u,2u,1u,1u,0u,1u,2u,1u,1u,1u,1u,1u,1u,
    1u,1u,1u,1u,1u,2u,2u,1u,2u,2u,1u,2u,0u,1u,1u,1u,
    1u,1u,1u,1u,1u,1u,3u,3u,3u,0u,1u,0u,0u,1u,1u,1u,
    0u,1u,0u,0u,0u,1u,1u,1u,1u,0u,0u,0u,2u,3u,2u,2u,
    2u,2u,2u,2u,2u,2u,2u,2u,2u,1u,1u,1u,1u,1u,2u,1u,
    1u,1u,1u,1u,1u,1u,1u,0u,0u,3u,2u,0u,0u,8u,7u,7u,
    1u,2u,0u,0u,0u,0u,1u,1u,2u,2u,1u,2u,1u,1u,0u,0u,
    1u,1u,1u,1u,2u,1u,2u,2u,2u,0u,1u,2u,1u,0u,0u,0u,
    0u,0u,0u,1u,2u,0u,1u,1u,0u,1u,1u,1u,1u,1u,1u,1u,
    1u,1u,1u,1u,1u,1u,1u,1u,2u,1u,1u,1u,0u,5u,5u,0u,
    1u,1u,0u,0u,4u,2u,0u,1u,0u,2u,0u,2u,2u,2u,2u,2u,
    2u,2u,0u,0u
};
static const uint8_t p4_ai_command_arity_3[61] = {
    2u,1u,1u,1u,1u,2u,1u,1u,0u,2u,0u,0u,2u,0u,1u,1u,
    1u,2u,2u,1u,1u,2u,1u,0u,0u,1u,0u,0u,2u,1u,1u,0u,
    1u,1u,1u,0u,1u,1u,0u,0u,2u,1u,1u,1u,3u,2u,1u,3u,
    2u,3u,0u,0u,0u,0u,0u,0u,0u,1u,1u,2u,0u
};
static inline uint8_t p4_ai_command_arity(uint32_t command)
{
    uint32_t index = command & 0xfffu;
    switch (command >> 12) {
    case 0: return index < 121u ? p4_ai_command_arity_0[index] : 255u;
    case 1: return index < 51u ? p4_ai_command_arity_1[index] : 255u;
    case 2: return index < 276u ? p4_ai_command_arity_2[index] : 255u;
    case 3: return index < 61u ? p4_ai_command_arity_3[index] : 255u;
    default: return 255u;
    }
}
typedef struct P4AiQueryBinding { uint32_t command, function, parameter; } P4AiQueryBinding;
/* Actual SLUS dispatch bindings. A numeric command/arity alone is insufficient
 * evidence for classifying a BF argument or an actor-basic guard. */
static const P4AiQueryBinding p4_ai_query_bindings[] = {
    {0x2015u, 0x001e0060u, 0u}, {0x2016u, 0x001e0110u, 0u},
    {0x2017u, 0x001e01c0u, 0u}, {0x2018u, 0x001e0270u, 0u},
    {0x2019u, 0x001e0320u, 0u}, {0x201au, 0x001e03d0u, 0u},
    {0x201bu, 0x001e0480u, 0u}, {0x201cu, 0x001e0530u, 0u},
    {0x201du, 0x001e05e0u, 0u}, {0x201eu, 0x001e0690u, 0u},
    {0x201fu, 0x001e0740u, 0u}, {0x2020u, 0x001e07f0u, 0u},
    {0x2051u, 0x001e0fb0u, 0u}, {0x2052u, 0x001e1060u, 0u},
    {0x2053u, 0x001e1110u, 0u}, {0x2054u, 0x001e11c0u, 0u},
    {0x2055u, 0x001e1270u, 0u}, {0x2056u, 0x001e1320u, 0u},
    {0x2057u, 0x001e13d0u, 0u}, {0x2058u, 0x001e1480u, 0u},
    {0x206du, 0x001e36c0u, 0u}, {0x206eu, 0x001e3770u, 0u},
    {0x206fu, 0x001e3820u, 0u}, {0x2071u, 0x001e3980u, 0u},
    {0x2072u, 0x001e3a30u, 0u}, {0x2073u, 0x001e3ae0u, 0u},
    {0x2074u, 0x001e3b90u, 0u}, {0x207du, 0x001e6ea0u, 0u},
    {0x207eu, 0x001e6fa0u, 0u}, {0x207fu, 0x001e7080u, 0u},
    {0x2080u, 0x001e8490u, 0u}, {0x2081u, 0x001e85c0u, 0u},
    {0x2082u, 0x001e86f0u, 0u}, {0x2083u, 0x001e8eb0u, 0u},
    {0x2084u, 0x001e8fe0u, 0u}, {0x2085u, 0x001e9110u, 0u},
    {0x208du, 0x001e7160u, 0u}, {0x208eu, 0x001e7240u, 0u},
    {0x208fu, 0x001e7320u, 0u}, {0x209cu, 0x001e6930u, 1u},
    {0x20d1u, 0x001e77f0u, 0u}, {0x20d2u, 0x001e7950u, 0u},
    {0x20d3u, 0x001e7ab0u, 0u}, {0x20dau, 0x001e7c20u, 0u},
    {0x20ebu, 0x001e4600u, 0u}, {0x20ecu, 0x001e46b0u, 0u},
    {0x20edu, 0x001e4760u, 0u}, {0x20f1u, 0x001e8090u, 0u},
    {0x20f2u, 0x001e8190u, 0u}, {0x20f3u, 0x001e8290u, 0u},
    {0x20f4u, 0x001e8390u, 0u}, {0x20f5u, 0x001e8820u, 0u},
    {0x20f6u, 0x001e8950u, 0u}, {0x20f7u, 0x001e8a80u, 0u},
    {0x2109u, 0x001e6b90u, 1u}
};
static inline const P4AiQueryBinding *p4_ai_query_binding(uint32_t command)
{
    uint32_t low = 0u, high = sizeof p4_ai_query_bindings / sizeof p4_ai_query_bindings[0];
    while (low < high) {
        uint32_t mid = low + (high - low) / 2u;
        if (p4_ai_query_bindings[mid].command < command) low = mid + 1u;
        else high = mid;
    }
    return low < sizeof p4_ai_query_bindings / sizeof p4_ai_query_bindings[0] &&
        p4_ai_query_bindings[low].command == command ? &p4_ai_query_bindings[low] : 0;
}
static inline uint32_t p4_ai_semantic_binding(uint32_t command)
{
    const P4AiQueryBinding *query = p4_ai_query_binding(command);
    if (query) return query->function;
    switch (command) {
    case 0x203du: return 0x001def60u;
    case 0x203eu: return 0x001defa0u;
    case 0x2042u: return 0x001eb1d0u;
    case 0x2068u: return 0x001df040u;
    case 0x2086u: return 0x001e8bb0u;
    case 0x2088u: return 0x001e8cd0u;
    case 0x2090u: return 0x001e8d80u;
    case 0x20d0u: return 0x001e64c0u;
    case 0x20fau: return 0x001e9950u;
    default: return 0u;
    }
}
/* Dispatch words are retail addresses on 64-bit, but patched native pointers
 * on i386. Compare the actual word against the same encoded function identity. */
static inline int p4_ai_semantic_binding_matches(uint32_t command, uint32_t word)
{
    uint32_t expected = p4_ai_semantic_binding(command), encoded;
    return !expected || (!p4_mod_memory_function_encode(expected, &encoded) && word == encoded);
}
static inline int p4_ai_command_uses_frames(uint32_t command)
{
    if ((command >= 0x2000u && command <= 0x2023u) ||
        (command >= 0x203du && command <= 0x2041u) ||
        (command >= 0x204du && command <= 0x2058u) ||
        (command >= 0x205fu && command <= 0x2063u) ||
        (command >= 0x206du && command <= 0x2074u) ||
        (command >= 0x207du && command <= 0x2090u) ||
        (command >= 0x20a3u && command <= 0x20a7u) ||
        (command >= 0x20d4u && command <= 0x20d9u)) return 1;
    switch (command) {
    case 0x2068u: case 0x2078u: case 0x2079u: case 0x207au: case 0x209cu:
    case 0x20d0u: case 0x20d1u: case 0x20d2u: case 0x20d3u: case 0x20dau:
    case 0x20fau: case 0x2104u: case 0x2105u: case 0x2109u: return 1;
    default: return 0;
    }
}
#endif
