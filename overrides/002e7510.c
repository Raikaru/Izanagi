/* 002e7510: preserve the field controller/backdrop setup and retail ranges;
 * both native shop entry owners append the same expanded stock policy. */
#include "p4pc/game_items_native.h"
s32 shopSystemCreate(s32 parent)
{
    s32 result;
    u8 *work;
    s32 i;

    chkmemAllocSiteTag(D_0063FC80, 0x11A);
    work = D_008873F4[0](1, 0x18, 0x40000);
    result = (s32)kwlnTaskCreateChild((void *)((void *)(parent)), (const void *)((char *)D_0063FC90), 0xF, 0, 0, shopSystemUpdate, shopSystemDestroy, (u8 *)(work));
    iGpffffb590 = NULL;
    iGpffffb590 = (u8 *)fclSpriteSpawnChild(result, fclMakeVec2(0, 0));
    fclSpriteSetPositionBounds(iGpffffb590, fclMakeVec2(0, 0), fclMakeBoundsPacket(640.0f, 448.0f));
    fclSpriteSetCornerColors(iGpffffb590, fclMakeDrawColor(0, 0, 0, 0xFF));
    ((ShopSystemWork *)work)->state = 1;
    sdkSndFadeOutChannelFrames(3, 0, 0x1E);

    for (i = 0; i < 0x2FF; i++) {
        if (((datItemMoonGateFirst((s16)i) & 0xFFF00) >> 8) == 0 &&
            ((datItemMoonGateSecond((s16)i) & 0xFFF00) >> 8) == 0) {
            datSetStateTableEntry(i, (clndGetMoonPhase(i) & 0xFF) | 4);
        } else if ((datItemMoonGateFirst((s16)i) & 0xFF) == 0 &&
                   (datItemMoonGateSecond((s16)i) & 0xFF) == 0) {
            datSetStateTableEntry(i, (clndGetMoonPhase(i) & 0xFF) | 4);
        }
    }
    for (i = 0x300; i < 0x3FF; i++) {
        if ((fclCalendarMonthOrder((u8)datItemAvailableMonth((s16)i)) & 0xFF) <
            (fclCalendarMonthOrder(4) & 0xFF)) {
            datSetStateTableEntry(i, (clndGetMoonPhase(i) & 0xFF) | 4);
        } else if ((fclCalendarMonthOrder((u8)datItemAvailableMonth((s16)i)) & 0xFF) ==
                   (fclCalendarMonthOrder(4) & 0xFF) &&
                   (s8)datItemAvailableDay((s16)i) < 2) {
            datSetStateTableEntry(i, (clndGetMoonPhase(i) & 0xFF) | 4);
        }
    }
    p4_game_tables_items_native_stock_defaults();
    return result;
}
