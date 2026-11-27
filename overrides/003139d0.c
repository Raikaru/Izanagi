#include "p4pc/game_mode.h"
s32 fclCombineMaterialsHeld(s8 materialCount, s8 slot)
{
    s32 i;
    s32 start;
    s64 limit;
    s32 result;
    if (p4_game_mode() == P4_GAME_MODE_GOLDEN) {
        s32 n = materialCount == 7 ? 12 : materialCount;
        if (n < 0 || n > 12) return 0;
        for (s32 a = 0; a < n; ++a) {
            FclCombinePersonaHeader *entry = (FclCombinePersonaHeader *)yListSlotEntry((s8)(slot + 1), a);
            if (!entry || !datPersonaFindRosterRecordById(entry->personaId)) return 0;
            for (s32 b = 0; b < a; ++b) {
                FclCombinePersonaHeader *previous = (FclCombinePersonaHeader *)yListSlotEntry((s8)(slot + 1), b);
                if (previous->personaId == entry->personaId) return 0;
            }
        }
        return 1;
    }
    i = 0;
    start = slot + 1;
    limit = materialCount;
    goto loop_test;
loop_body:
    if (datPersonaFindRosterRecordById(((FclCombinePersonaHeader *)yListSlotEntry((s8)start, i))->personaId) == 0) {
        result = 0;
        goto done;
    }
    i = (s16)(i + 1);
loop_test:
    if ((s16)i < limit) {
        goto loop_body;
    }
    result = 1;
done:
    return result;
}
