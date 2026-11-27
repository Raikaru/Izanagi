#include "p4pc/game_mode.h"
extern void kAssertFail(void *, s32);
extern char D_00642F30[];

void fclCombineFuseSourceRecords(u16 *resultRecord, u8 *firstSource, u8 *secondSource, u8 *thirdSource, u8 *fourthSource, u8 *fifthSource, u8 *sixthSource, s32 sourceCount, s8 listSlot, s8 entryIndex)
{
    u16 buf[6 * 24];
    s8 v;

    if (p4_game_mode() == P4_GAME_MODE_GOLDEN && sourceCount == 7) {
        u16 wide[12 * 24];
        s32 i;
        for (i = 0; i < 12; ++i) {
            u8 *source = (u8 *)yListSlotEntry(listSlot == 1 ? 0 : (s8)(entryIndex + 1), (s16)i);
            if (source == NULL) {
                kAssertFail(D_00642F30, 0x3AF);
                return;
            }
            memcpy(wide + i * 24, source, 0x30);
        }
        datPersonaInitRecord((u8 *)resultRecord, resultRecord[1]);
        v = (s8)fclFuseRollPersona(resultRecord, (u8 *)wide, 12);
        *(yListSlotWork(listSlot) + entryIndex + 0x2E4) = v;
        return;
    }
    switch ((s16)sourceCount) {
    case 0:
        break;
    case 6:
        memcpy(buf + 5 * 24, sixthSource, 0x30);
    case 5:
        memcpy(buf + 4 * 24, fifthSource, 0x30);
    case 4:
        memcpy(buf + 3 * 24, fourthSource, 0x30);
    case 3:
        memcpy(buf + 2 * 24, thirdSource, 0x30);
    case 2:
        memcpy(buf + 1 * 24, secondSource, 0x30);
    case 1:
        memcpy(buf, firstSource, 0x30);
        break;
    }
    datPersonaInitRecord((u8 *)resultRecord, resultRecord[1]);
    v = (s8)fclFuseRollPersona(resultRecord, (u8 *)buf, (s8)sourceCount);
    *(yListSlotWork(listSlot) + entryIndex + 0x2E4) = v;
}
