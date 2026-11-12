#include "p4pc/game_items.h"

extern int64_t datItemBankOffset(int16_t);
extern void fclMesWindowDraw(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, float, float, float);
void p4_game_tables_items_queue_description(int16_t id, int32_t color, int32_t font, int32_t mode,
    int32_t table, int32_t list_index, float x, float y, float depth)
{
    if (id >= (int32_t)P4_ITEM_RETAIL_COUNT) {
        uint32_t stream = p4_game_tables_items_native_description((uint32_t)id);
        /* Same real primitive-list owner as 002bc7f0, preserving its selected
         * list and draw order; the native glyph list owns queued text work. */
        uint32_t queue = 0x00793e80u + (uint32_t)list_index * 0x30u;
        p4_game_tables_text_queue(stream, x, y, depth, (uint32_t)color, (uint32_t)font,
            (uint32_t)mode, 1, 1, queue);
        return;
    }
    fclMesWindowDraw((int16_t)datItemBankOffset(id), color, font, mode, table, list_index, x, y, depth);
}
