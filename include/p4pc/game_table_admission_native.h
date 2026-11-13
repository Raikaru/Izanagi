#ifndef P4PC_GAME_TABLE_ADMISSION_NATIVE_H
#define P4PC_GAME_TABLE_ADMISSION_NATIVE_H
#include <stddef.h>
#include <stdint.h>
typedef struct P4GameTableAdmissionReceipt P4GameTableAdmissionReceipt;
typedef void (*P4GameTablePackedObserver)(const uint8_t *descriptor, const char *path,
    const uint8_t *bytes, uint32_t size, P4GameTableAdmissionReceipt *receipt);
/* The receipt is borrowed only during this synchronous observer invocation.
 * It authenticates one exact input/descriptor/buffer tuple, and can be consumed
 * once. NULL means no selected input was checked; never retain or copy it. */
int p4_game_table_admission_consume(P4GameTableAdmissionReceipt *receipt,
    const uint8_t *descriptor, const char *path, const uint8_t *bytes, uint32_t size);
/* No staged ITEM input leaves the original load untouched. Otherwise verify
 * the actual completed request before the retail destination is written. When
 * a derived source composed the input, the verified loader buffer is then
 * overwritten with the composed bytes (same size) before the native copy. */
void p4_game_table_admission_item(const char *path, uint8_t *bytes, uint32_t size);
/* Validate before any GP slot or bank copy, then invoke exactly one observer.
 * A successful selected-input check hands its stack-local receipt to the
 * observer; unrelated/no-stage loads invoke it with NULL. A composed derived
 * input is written into the verified buffer before the observer runs. */
void p4_game_table_admission_packed(const uint8_t *descriptor, const char *path,
    uint8_t *bytes, uint32_t size, P4GameTablePackedObserver observer);
/* One block of the loader's twenty-slot descriptor: its GP pointer slot and the
 * ORIGINAL block size, exactly as 0x00193840 copies it. */
typedef struct P4GameTablePackedBlock { uint32_t slot, size; } P4GameTablePackedBlock;
#define P4_GAME_TABLE_PACKED_SLOTS 20u
/* Block count, or -1 for a malformed chain (no terminator within twenty slots,
 * a block past the buffer or an offset overflow). */
int p4_game_tables_packed_blocks(const uint8_t *descriptor, const uint8_t *bytes,
    uint32_t size, P4GameTablePackedBlock blocks[P4_GAME_TABLE_PACKED_SLOTS]);
/* The packed loader's single observer. It authenticates the selected input
 * once (the receipt, or the generic preparation without one) and then hands
 * the observed banks to every staged family. Nothing staged: no work. */
void p4_game_tables_observe_packed(const uint8_t *descriptor, const char *path,
    const uint8_t *bytes, uint32_t size, P4GameTableAdmissionReceipt *receipt);
#endif
