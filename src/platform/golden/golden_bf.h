#ifndef P4_GOLDEN_BF_H
#define P4_GOLDEN_BF_H
#include "golden_ai.h"
#include <stddef.h>
#include <stdint.h>

/* A native handler word is a guest function identity, never a host pointer.
 * The Golden interpreter selects either that original native entry or the
 * wide Golden handler. It never rewrites the PS2 tables at 0x748530. */
typedef struct P4GoldenBfNativeCommand {
    uint32_t function, arity;
} P4GoldenBfNativeCommand;
typedef struct P4GoldenBfNativeGroup {
    const P4GoldenBfNativeCommand *commands;
    uint32_t count;
} P4GoldenBfNativeGroup;
typedef int (*P4GoldenBfHandler)(P4GameMode, P4GoldenAi *, uint32_t,
                                const int64_t *, P4GoldenAiResult *);
typedef struct P4GoldenBfCommand {
    uint32_t native_function, arity;
    P4GoldenBfHandler golden;
} P4GoldenBfCommand;
typedef struct P4GoldenBfTables {
    P4GoldenBfCommand common[192], field[51], ai[294], battle[61];
} P4GoldenBfTables;
/* PS2: 0, with no reads/writes. Golden: 1 on construction, -1 on invalid
 * original table shapes. Caller owns a separate table for the session. */
int p4_golden_bf_tables(P4GameMode mode, P4GoldenBfTables *out,
                        const P4GoldenBfNativeGroup original[4]);
const P4GoldenBfCommand *p4_golden_bf_resolve(const P4GoldenBfTables *tables, uint32_t command);
#endif
