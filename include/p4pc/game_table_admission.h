#ifndef P4PC_GAME_TABLE_ADMISSION_H
#define P4PC_GAME_TABLE_ADMISSION_H
#include "p4pc/game_table_admission_native.h"
#include "p4pc/game_table_inputs.h"
#include "p4pc/table_result.h"
/* One exact CPU-sealed Root tuple, staged before the first raw load. Inputs
 * and their selected plans/providers outlive it. No contribution means no
 * stage, allocation, digest comparison or change to retail loading. */
int p4_game_table_admission_stage(P4GameTableInput *const *inputs, size_t count);
/* Exact early owner rollback only; never release another prepare callback. */
int p4_game_table_admission_unstage(P4GameTableInput *const *inputs, size_t count);
/* Pure content/descriptor check of actual live readable bytes. On failure,
 * *failed_provider is the exact retained selected package, or NULL for a known
 * native plan/unknown descriptor; never infer identity from the guest path. */
int p4_game_table_admission_check_item(const char *path, const uint8_t *bytes,
    size_t size, const P4PackageProvider **failed_provider);
int p4_game_table_admission_check_packed(const uint8_t *descriptor, const char *path,
    const uint8_t *bytes, size_t size, const P4PackageProvider **failed_provider);
#endif
