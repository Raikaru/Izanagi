#include "p4pc/game_mode.h"
s32 yListSlotRefresh(u8 *task) {
    DatPersonaRecord registered;
    DatPersonaRecord current;
    s16 slotIndex;
    u16 *selectedRecord;
    u8 *work;
    u32 kind;
    work = ((KwlnTask *)task)->work;
    if (*(s8 *)work == 1) {
        return 0;
    }
    ((YListPersonaWork *)work)->count.value = 0;
    kind = ((YListPersonaWork *)work)->kind.unsignedValue;
    switch (kind) {
    case 0:
        for (slotIndex = 1; slotIndex < 0xC; slotIndex++) {
            datPersonaInitRecord(work + ((slotIndex - 1) * 0x30) + 0x14, (u16)slotIndex);
            MEMBER(s32, work, YListPersonaWork, count.value) += 1;
        }
        break;
    case 1:
        for (slotIndex = 0; slotIndex < (datPersonaRosterCapacity() & 0xFFFF); slotIndex++) {
            if (datPersonaRosterSlotActive(slotIndex) == 1) {
                memset(work + (MEMBER(s32, work, YListPersonaWork, count.value) * 0x30) + 0xA4, 0, 0x30);
                memcpy(work + (MEMBER(s32, work, YListPersonaWork, count.value) * 0x30) + 0xA4, datPersonaRosterSlot(slotIndex), 0x30);
                MEMBER(s32, work, YListPersonaWork, count.value) += 1;
            }
        }
        break;
    case 2:
        for (slotIndex = 0; slotIndex < 0x100; slotIndex++) {
            if (datPersonaCachedRecord(slotIndex) != 0) {
                memset(work + (MEMBER(s32, work, YListPersonaWork, count.value) * 0x30) + 0x14, 0, 0x30);
                memcpy(work + (MEMBER(s32, work, YListPersonaWork, count.value) * 0x30) + 0x14, datPersonaCachedRecord(slotIndex), 0x30);
                MEMBER(s32, work, YListPersonaWork, count.value) += 1;
            }
        }
        break;
    case 5:
        for (slotIndex = 0; slotIndex < (datPersonaRosterCapacity() & 0xFFFF); slotIndex++) {
            if (datPersonaRosterSlotActive(slotIndex) == 1) {
                selectedRecord = (u16 *)datPersonaGetRecord(1);
                if (selectedRecord != (u16 *)datPersonaRosterSlot(slotIndex)) {
                    memset(work + (MEMBER(s32, work, YListPersonaWork, count.value) * 0x30) + 0xA4, 0, 0x30);
                    memcpy(work + (MEMBER(s32, work, YListPersonaWork, count.value) * 0x30) + 0xA4, datPersonaRosterSlot(slotIndex), 0x30);
                    MEMBER(s32, work, YListPersonaWork, count.value) += 1;
                }
            }
        }
        break;
    case 6:
        for (slotIndex = 0; slotIndex < (datPersonaRosterCapacity() & 0xFFFF); slotIndex++) {
            if (datPersonaRosterSlotActive(slotIndex) == 1) {
                memcpy(&current, datPersonaRosterSlot(slotIndex), 0x30);
                if (((YListPersonaMetadata *)iGpffffb3d4)[current.id].arcana < (p4_game_mode() == P4_GAME_MODE_GOLDEN ? 28 : 0x16)) {
                    memcpy(&registered, datPersonaCachedRecord(current.id), 0x30);
                    if (yListRecordsDiffer((u8 *)&current, (u8 *)&registered) == 1) {
                        memset(work + (MEMBER(s32, work, YListPersonaWork, count.value) * 0x30) + 0xA4, 0, 0x30);
                        memcpy(work + (MEMBER(s32, work, YListPersonaWork, count.value) * 0x30) + 0xA4, datPersonaRosterSlot(slotIndex), 0x30);
                        MEMBER(s32, work, YListPersonaWork, count.value) += 1;
                    }
                }
            }
        }
        break;
    case 7:
        for (slotIndex = 1; slotIndex < (p4_game_mode() == P4_GAME_MODE_GOLDEN ? 256 : 0xC0); slotIndex++) {
            datPersonaInitRecord(work + ((slotIndex - 1) * 0x30) + 0x14, (u16)slotIndex);
            MEMBER(s32, work, YListPersonaWork, count.value) += 1;
        }
        break;
    case 8:
        datPersonaInitAllRecords();
        for (slotIndex = 1; slotIndex < (p4_game_mode() == P4_GAME_MODE_GOLDEN ? 256 : 0xC0); slotIndex++) {
            datPersonaInitRecord(work + ((slotIndex - 1) * 0x30) + 0x14, (u16)slotIndex);
            MEMBER(s32, work, YListPersonaWork, count.value) += 1;
        }
        break;
    default:
        break;
    }
    *work = 1;
    return 0;
}
