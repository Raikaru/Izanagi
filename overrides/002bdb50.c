/* 002bdb50: original controller setup and exact retail sentinel ranges;
 * append registered rows using kind-based native stock policy. */
#include "p4pc/game_items_native.h"
s32 fclShopCreate(s32 parentTask, s8 mode)
{
    FclShopWork *work;
    s32 taskHandle;
    s16 itemId;

    chkmemAllocSiteTag(D_0063F3C8, 0xB0);
    work = D_008873F4[0](1, 0x1840, 0x40000);
    taskHandle = (s32)kwlnTaskCreateChild((void *)(parentTask), (const void *)(D_0063F3E0), 0xF, 0, 0, fclShopMainUpdate, fclShopMainDestroy, (u8 *)(work));
    work->state = 1;
    work->mode = 0;
    if (mode == 1) {
        work->state = 0x19;
        work->state = 3;
        work->mode = 1;
    }
    work->drawTask = fclShopDrawCreate(parentTask);
    work->unk18 = 0;
    work->unk24 = 0;
    work->unk26 = 0;
    work->unk2A = 0;
    work->unk10 = 0x41000000;
    work->unk14 = 0x41200000;
    work->unk18 = 0;
    work->unk1C = 0x41000000;
    work->unk20 = 0x41200000;
    work->unk24 = 0;
    sdkSndFadeOutChannelFrames(3, 0, 0x1E);
    for (itemId = 0; itemId < 0x2FF; itemId++) {
        if (((datItemMoonGateFirst(itemId) & 0xFFF00) >> 8) == 0 &&
            ((datItemMoonGateSecond(itemId) & 0xFFF00) >> 8) == 0) {
            datSetStateTableEntry(itemId, clndGetMoonPhase(itemId) | 4);
        } else if (!(datItemMoonGateFirst(itemId) & 0xFF) && !(datItemMoonGateSecond(itemId) & 0xFF)) {
            datSetStateTableEntry(itemId, clndGetMoonPhase(itemId) | 4);
        }
    }
    for (itemId = 0x300; itemId < 0x3FF; itemId++) {
        if ((fclCalendarMonthOrder((u8)datItemAvailableMonth(itemId)) & 0xFF) <
            (fclCalendarMonthOrder(4) & 0xFF)) {
            datSetStateTableEntry(itemId, clndGetMoonPhase(itemId) | 4);
        } else if ((fclCalendarMonthOrder((u8)datItemAvailableMonth(itemId)) & 0xFF) ==
                   (fclCalendarMonthOrder(4) & 0xFF) && (s8)datItemAvailableDay(itemId) < 2) {
            datSetStateTableEntry(itemId, clndGetMoonPhase(itemId) | 4);
        }
    }
    p4_game_tables_items_native_stock_defaults();
    return taskHandle;
}
