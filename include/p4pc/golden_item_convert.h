/* Golden ITEMTBL section projection. */
#ifndef P4PC_GOLDEN_ITEM_CONVERT_H
#define P4PC_GOLDEN_ITEM_CONVERT_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/golden_domains.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Validates both section headers and item/skill domains, then emits 68-byte
 * metadata and 24-byte names, indexed by item ID. Input and outputs must not
 * overlap. Empty names remain empty and are not valid declaration names. */
int p4_golden_items_convert(const P4GoldenDomainMap *map, const uint8_t *input, size_t size,
                            uint8_t *rows, size_t rows_capacity, uint8_t *names, size_t names_capacity,
                            char *reason, size_t reason_capacity);
#ifdef __cplusplus
}
#endif
#endif
