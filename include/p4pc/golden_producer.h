/* Registers the p4pc:golden derived-table producer (docs/modding/golden.md).
 * Core boot only, before package admission; registration does no I/O. */
#ifndef P4PC_GOLDEN_PRODUCER_H
#define P4PC_GOLDEN_PRODUCER_H

#ifdef __cplusplus
extern "C" {
#endif

/* 0 registered, -1 refused (duplicate id). */
int p4_golden_producer_register(void);

#ifdef __cplusplus
}
#endif
#endif
