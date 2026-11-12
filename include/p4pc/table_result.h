#ifndef P4PC_TABLE_RESULT_H
#define P4PC_TABLE_RESULT_H
/* Shared by host registries and scalar-only native game-unit bridges. */
typedef enum P4TableResult {
    P4_TABLE_OK = 0, P4_TABLE_INVALID = -1, P4_TABLE_CONFLICT = -2,
    P4_TABLE_NOMEM = -3, P4_TABLE_FROZEN = -4, P4_TABLE_NOT_FOUND = -5,
    P4_TABLE_PREPARE_REQUIRED = -6, P4_TABLE_NOT_READY = -7,
    P4_TABLE_RENDER_ERROR = -8
} P4TableResult;
#endif
