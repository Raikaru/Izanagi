#include "p4pc/game_mode.h"
void yListSlotFillByLevel(s8 list, u16 *exclude, s8 level)
{
    YListPersonaWork *levelList;
    u8 *data;
    s32 limit;
    s16 i;

    if (D_00882F70[list] == NULL) {
        return;
    }
    data = ((KwlnTask *)D_00882F70[list])->work;
    levelList = (YListPersonaWork *)data;
    yListSlotClear(list);
    limit = (s16)fclAddClamped(level, 3, 99, 1, 1);
    for (i = 0; i < (p4_game_mode() == P4_GAME_MODE_GOLDEN ? 256 : 0xC0); i++) {
        if (iGpffffb3d4[i * 0xE + 2] == 1 && (*(u16 *)&iGpffffb3d4[i * 0xE] & 8) == 0) {
            if (cmmCommunityIsListable((u16)i) != 0 && cmmCommunityPassesFlagGate((u16)i) != 0 && yListExcluded(i, exclude) == 0 &&
                yListContains(list, i) == 0 && cmmCommunityHasFlagGate((u16)i) == 0 &&
                limit >= iGpffffb3d4[i * 0xE + 3]) {
                memset(yListEntry(list, ((YListPersonaWork *)data)->count.value), 0, 0x30);
                datPersonaInitRecord(yListEntry(list, levelList->count.value), i);
                levelList->count.value += 1;
            }
        }
    }
}
