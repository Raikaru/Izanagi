#ifndef P4PC_GAME_TABLE_USE_H
#define P4PC_GAME_TABLE_USE_H
/* Stable semantic domains shared by the registry and its future source ledger. */
typedef enum P4TableSourceDomain {
    P4_TABLE_SOURCE_ELEMENT = 1, P4_TABLE_SOURCE_CHANNEL = 2,
    P4_TABLE_SOURCE_AFFINITY = 3, P4_TABLE_SOURCE_ACTION = 4,
    P4_TABLE_SOURCE_ALIAS = 5, P4_TABLE_SOURCE_EFFECT = 6,
    P4_TABLE_SOURCE_SKILL = 7, P4_TABLE_SOURCE_ITEM = 8,
    P4_TABLE_SOURCE_CATEGORY = 9, P4_TABLE_SOURCE_STRING = 10,
    P4_TABLE_SOURCE_ENEMY = 11, P4_TABLE_SOURCE_PERSONA = 12
} P4TableSourceDomain;
/* Native actual semantic consumption ONLY, never registration/CPU preparation,
 * sealing, byte verification or binding. Identity is the exact sealed source
 * span; context owns its actual retained source/provider. No active lookup or
 * provider-string reconstruction. Zero permits the already-proved native use;
 * an origin failure must enter the core's real controlled failure frontend
 * before native effects, not silently omit touch or execute an unknown source. */
typedef int (*P4GameTableSourceUseFn)(const void *sealed_span_identity,
    void *immutable_prepared_context);
#endif
