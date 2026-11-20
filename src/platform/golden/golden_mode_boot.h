#ifndef P4_GOLDEN_MODE_BOOT_H
#define P4_GOLDEN_MODE_BOOT_H
#include "p4pc/game_mode.h"
#include "p4pc/game_table_builder.h"
#include <stddef.h>

/* Trusted compiled adapters: conversion callbacks prepare their own checked
 * outputs in context; publish admits those outputs and declares their rows. */
typedef int (*P4GoldenModeConvert)(void *context, char *reason, size_t capacity);
typedef int (*P4GoldenModePublish)(void *context, P4GameTablesDefinitionBuilder *builder,
                                   char *reason, size_t capacity);
typedef struct P4GoldenModeAdapter {
    const char *name;
    P4GoldenModeConvert convert;
} P4GoldenModeAdapter;
typedef struct P4GoldenModeAdapters {
    const P4GoldenModeAdapter *items;
    size_t count;
    P4GoldenModePublish publish;
    void (*rollback)(void *context);
    void *context;
} P4GoldenModeAdapters;
/* Core registration before table preparation; adapters and context must
 * outlive the session. Unknown and duplicate names are refused. */
int p4_golden_mode_adapter_register(const char *name, P4GoldenModeConvert convert);
int p4_golden_mode_publisher_register(P4GoldenModePublish publish, void (*rollback)(void *), void *context);
const P4GoldenModeAdapters *p4_golden_mode_registered(void);
/* Reports every missing converter and returns -1 before invoking any adapter.
 * PS2 is a no-op. On failure rollback releases all prepared context; on success
 * publisher must transfer its resources to their real session owners. */
int p4_golden_mode_boot_prepare(P4GameMode mode, P4GameTablesDefinitionBuilder *builder,
                                 const P4GoldenModeAdapters *adapters, char *reason, size_t capacity);
#endif
