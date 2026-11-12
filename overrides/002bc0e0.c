/* 002bc0e0: new-ID single/paired descriptions bypass retail bank indexing;
 * the complete retail switch (including kind2's quantity-derived ID) remains. */
#include "p4pc/game_items_native.h"
 s32 fclMesWindowDrawItemEntry(f32 x, f32 y, f32 depth, s32 color, s32 font, s32 mode, s32 flags, s32 kind, s32 item)
{
    extern s32 datItemCount(s16 itemId);
    extern u32 datEntryGetFlags(s16 itemId);
    extern s32 itfMesWindowRecordCount(s32 handle);
    extern s32 itfMesWindowDrawEntry(f32 x, f32 y, f32 scale, s32 color, s32 slotIndex, s32 effect, s32 flags, s32 handle, s32 recordIndex);
    extern s32 itfMesWindowDrawPair(f32 x, f32 y, f32 scale, s32 color, s32 slotIndex, s32 effect, s32 flags, s32 handle, s32 recordIndex, s32 secondHandle, s32 secondRecordIndex);
    extern s32 D_0063F2F0[];
    extern s32 D_0063F310[];
    extern s32 D_0063F2C0[];
    s32 itemTables[6];
    s32 entryTables[9];
    s32 *src;
    s32 *dst;
    u32 recordCount;
    s32 *src2;
    s32 *dst2;
    s32 loop;
    s32 loop2;
    s32 firstWord;
    s32 secondWord;
    s32 category;
    s32 tableIndex;
    s32 messageTable;
    s32 itemLevel;
    u32 itemId;
    s32 entryIndex;

    if (kind == 0 && p4_game_tables_items_native_id_is_new((u32)item)) {
        p4_game_tables_text_draw(p4_game_tables_items_native_description((u32)item),
            x, y, depth, (u32)color, (u32)font, (u32)mode, (u32)flags, 1);
        return 0;
    }
    if (kind == 1 && p4_game_tables_items_native_id_is_new((u32)(item & 0xffff))) {
        itemId = item & 0xffff;
        itemLevel = item >> 16;
        if ((u32)itfMesWindowRecordCount(D_0063F2C0[0]) < (u32)itemLevel) return 0;
        p4_game_tables_text_draw_pair(p4_game_tables_items_native_description(itemId),
            x, y, depth, (u32)color, (u32)font, (u32)mode, 1, 1, D_0063F2C0[0], itemLevel);
        return 0;
    }
    src = D_0063F2F0;
    dst = itemTables;
    loop = 3;
    do {
        firstWord = src[0];
        secondWord = src[1];
        src += 2;
        loop -= 1;
        dst[0] = firstWord;
        dst[1] = secondWord;
        dst += 2;
    } while (loop > 0);
    src2 = D_0063F310;
    dst2 = entryTables;
    loop2 = 9;
    do {
        firstWord = *src2;
        src2 += 1;
        loop2 -= 1;
        *dst2 = firstWord;
        dst2 += 1;
    } while (loop2 > 0);
    switch (kind) {
    case 0:
        if (item >= 0x400) {
            category = 5;
        } else {
            category = (s32)datEntryGetFlags((s16)item);
        }
        tableIndex = itemTables[category];
        item = (s16)datItemBankOffset((s16)item);
        goto block_23;
    case 1:
        itemId = item & 0xFFFF;
        tableIndex = itemTables[datEntryGetFlags((s16)itemId)];
        messageTable = D_0063F2B0[tableIndex];
        entryIndex = (s16)datItemBankOffset((s16)itemId);
        if ((u32)itfMesWindowRecordCount(messageTable) < (u32)entryIndex) {
            return 0;
        }
        itemLevel = item >> 16;
        if ((u32)itfMesWindowRecordCount(D_0063F2C0[0]) < (u32)itemLevel) {
            return 0;
        }
        itfMesWindowDrawPair(x, y, depth,
                      color, font, mode, 1, messageTable, entryIndex,
                      D_0063F2C0[0], itemLevel);
        return 0;
    case 2:
        itemId = datItemCount((s16)item) & 0xFF;
        tableIndex = itemTables[datEntryGetFlags((s16)itemId)];
        messageTable = D_0063F2B0[tableIndex];
        entryIndex = (s16)datItemBankOffset((s16)itemId);
        if ((u32)itfMesWindowRecordCount(messageTable) < (u32)entryIndex) {
            return 0;
        }
        recordCount = itfMesWindowRecordCount(D_0063F2C0[0]);
        if (recordCount < (u32)(datItemCount((s16)item) & 0xFF)) {
            return 0;
        }
        itfMesWindowDrawPair(x, y, depth,
                      color, font, mode, 1, messageTable, entryIndex,
                      D_0063F2C0[0], datItemCount((s16)item) & 0xFF);
        return 0;
    default:
        tableIndex = entryTables[kind];
        goto block_23;
    }
block_23:
    messageTable = D_0063F2B0[tableIndex];
    if ((u32)itfMesWindowRecordCount(messageTable) < (u32)item) {
        return 0;
    }
    itfMesWindowDrawEntry(x, y, depth,
                  color, font, mode, flags, messageTable, item);
    return 0;
}
