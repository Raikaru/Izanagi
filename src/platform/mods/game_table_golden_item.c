/* Keep host allocations out of guest pointer slots and guest libc. */
#include "game_table_inputs_internal.h"
#include "p4pc/game_table_admission_native.h"
#include "p4pc/game_items_native.h"
#include <stdlib.h>
#include <string.h>

int p4_game_table_admission_golden_load_item(const char *path, void *destination, uint32_t capacity)
{
    uint8_t *bytes = NULL;
    uint32_t size = 0;
    int found = p4_game_table_admission_golden_copy(path, &bytes, &size);
    if (found != 1) return found;
    if (!destination || size > capacity || size > P4_ITEM_RETAIL_FILE_SIZE) {
        free(bytes);
        return -1;
    }
    p4_game_table_admission_item(path, bytes, size);
    int result = p4_game_tables_items_native_load(path, bytes, size);
    if (result) p4_game_tables_items_native_refuse(result);
    memcpy(destination, bytes, size);
    free(bytes);
    return 1;
}
