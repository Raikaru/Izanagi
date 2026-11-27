#include "p4pc/game_mode.h"
void yListSlotFillNearLevel(s8 list, u16 *exclude, s8 requestedLevel)
{
    YListPersonaWork *nearLevelList;
    u8 *work;
    s16 lowerLevel;
    s32 level;
    u8 **slotp;
    s32 upperLevel;
    s16 i;
    s16 initialRow;
    s16 expandedRow;
    s32 found;
    u16 id;
    u8 *refreshedWork;
    s16 rowIndex;
    u32 clearKind;
    u32 initializeKind;
    u8 *clearDestination;
    u8 *initializeDestination;
    s16 expandedLowerLevel;
    s16 expandedUpperLevel;
    s16 outer;
    s32 lowLimit;
    s32 expandedHighLimit;
    s32 expandedLowLimit;
    slotp = &D_00882F70[list];
    if (*slotp == NULL) {
        return;
    }
    work = ((KwlnTask *)*slotp)->work;
    nearLevelList = (YListPersonaWork *)work;
    yListSlotClear(list);
    level = (s8)requestedLevel;
    lowerLevel = (s16)fclSubClamped(level, 10, 1, 0x63, 1);
    upperLevel = (s16)fclAddClamped(level, 1, 0x63, 1, 1);
    i = 0;
    lowLimit = (s16)lowerLevel;
    for (; i < (p4_game_mode() == P4_GAME_MODE_GOLDEN ? 256 : 0xC0); i++) {
        s32 signedIndex = i;
        s32 metadataOffset;
        u8 *metadata;
        u8 kind;
        metadata = iGpffffb3d4;
        metadataOffset = signedIndex * 14;
        metadata += metadataOffset;
        kind = metadata[2];
        if (kind < 2 || kind >= (p4_game_mode() == P4_GAME_MODE_GOLDEN ? 28 : 0x16)) {
            continue;
        }
        id = (u16)i;
        if (cmmCommunityIsListable(id) == 0) {
            continue;
        }
        if (cmmCommunityPassesFlagGate(id) == 0) {
            continue;
        }
        if (yListIsExcludedId(id, exclude) != 0) {
            continue;
        }
        if (((u8 **)(u32)slotp)[0] != NULL) {
            for (initialRow = 0; initialRow < ((YListPersonaWork *)((KwlnTask *)((u8 **)(u32)slotp)[0])->work)->count.value; initialRow++) {
                if (signedIndex == ((DatPersonaRecord *)yListSlotEntry(list, initialRow))->id) {
                    found = 1;
                    goto initialSearchDone;
                }
            }
        }
        found = 0;
initialSearchDone:
        if (found != 0) {
            continue;
        }
        if (cmmCommunityHasFlagGate(id) != 0) {
            continue;
        }
        metadata = iGpffffb3d4;
        metadata += metadataOffset;
        if (upperLevel < metadata[3] || lowLimit > metadata[3]) {
            continue;
        }
        rowIndex = ((YListPersonaWork *)work)->count.half;
        refreshedWork = ((KwlnTask *)*slotp)->work;
        clearKind = ((YListPersonaWork *)refreshedWork)->kind.unsignedValue;
        switch (clearKind) {
        case 0:
        case 2:
        case 7:
        case 8:
            clearDestination = refreshedWork + rowIndex * 0x30 + 0x14;
            break;
        case 1:
        case 5:
        case 6:
        case 10:
            clearDestination = refreshedWork + rowIndex * 0x30 + 0xA4;
            break;
        default:
            clearDestination = refreshedWork + rowIndex * 0x30 + 0x14;
            break;
        }
        memset(clearDestination, 0, 0x30);
        rowIndex = nearLevelList->count.half;
        refreshedWork = ((KwlnTask *)*slotp)->work;
        initializeKind = ((YListPersonaWork *)refreshedWork)->kind.unsignedValue;
        switch (initializeKind) {
        case 0:
        case 2:
        case 7:
        case 8:
            initializeDestination = refreshedWork + rowIndex * 0x30 + 0x14;
            break;
        case 1:
        case 5:
        case 6:
        case 10:
            initializeDestination = refreshedWork + rowIndex * 0x30 + 0xA4;
            break;
        default:
            initializeDestination = refreshedWork + rowIndex * 0x30 + 0x14;
            break;
        }
        datPersonaInitRecord(initializeDestination, id);
        nearLevelList->count.value = nearLevelList->count.value + 1;
    }
    if (level >= 0x3C && ((YListPersonaWork *)work)->count.value < 3) {
        yListSlotClear(list);
    }
    if (((YListPersonaWork *)work)->count.value != 0) {
        return;
    }
    outer = 0;
    do {
        s32 expansion = outer;
        expandedLowerLevel = (s16)fclSubClamped(level, expansion * 5 + 10, 1, 0x63, 1);
        expandedUpperLevel = (s16)fclAddClamped(level, expansion + 1, 0x63, 1, 1);
        yListSlotClear(list);
        i = 0;
        expandedHighLimit = (s16)expandedUpperLevel;
        expandedLowLimit = (s16)expandedLowerLevel;
        for (; i < (p4_game_mode() == P4_GAME_MODE_GOLDEN ? 256 : 0xC0); i++) {
            s32 signedIndex = i;
            s32 metadataOffset;
            u8 *metadata;
            u8 kind;
            metadata = iGpffffb3d4;
            metadataOffset = signedIndex * 14;
            metadata += metadataOffset;
            kind = metadata[2];
            if (kind < 2 || kind >= (p4_game_mode() == P4_GAME_MODE_GOLDEN ? 28 : 0x16)) {
                continue;
            }
            id = (u16)i;
            if (cmmCommunityIsListable(id) == 0) {
                continue;
            }
            if (cmmCommunityPassesFlagGate(id) == 0) {
                continue;
            }
            if (yListIsExcludedId(id, exclude) != 0) {
                continue;
            }
            if (*slotp != NULL) {
                for (expandedRow = 0; expandedRow < ((YListPersonaWork *)((KwlnTask *)*slotp)->work)->count.value; expandedRow++) {
                    if (signedIndex == ((DatPersonaRecord *)yListSlotEntry(list, expandedRow))->id) {
                        found = 1;
                        goto expandedSearchDone;
                    }
                }
            }
            found = 0;
expandedSearchDone:
            if (found != 0) {
                continue;
            }
            if (cmmCommunityHasFlagGate(id) != 0) {
                continue;
            }
            metadata = iGpffffb3d4;
            metadata += metadataOffset;
            if (expandedHighLimit < metadata[3] || expandedLowLimit > metadata[3]) {
                continue;
            }
            rowIndex = nearLevelList->count.half;
            refreshedWork = ((KwlnTask *)*slotp)->work;
            clearKind = ((YListPersonaWork *)refreshedWork)->kind.unsignedValue;
            switch (clearKind) {
            case 0:
            case 2:
            case 7:
            case 8:
                clearDestination = refreshedWork + rowIndex * 0x30 + 0x14;
                break;
            case 1:
            case 5:
            case 6:
            case 10:
                clearDestination = refreshedWork + rowIndex * 0x30 + 0xA4;
                break;
            default:
                clearDestination = refreshedWork + rowIndex * 0x30 + 0x14;
                break;
            }
            memset(clearDestination, 0, 0x30);
            rowIndex = nearLevelList->count.half;
            refreshedWork = ((KwlnTask *)*slotp)->work;
            initializeKind = ((YListPersonaWork *)refreshedWork)->kind.unsignedValue;
            switch (initializeKind) {
            case 0:
            case 2:
            case 7:
            case 8:
                initializeDestination = refreshedWork + rowIndex * 0x30 + 0x14;
                break;
            case 1:
            case 5:
            case 6:
            case 10:
                initializeDestination = refreshedWork + rowIndex * 0x30 + 0xA4;
                break;
            default:
                initializeDestination = refreshedWork + rowIndex * 0x30 + 0x14;
                break;
            }
            datPersonaInitRecord(initializeDestination, id);
            nearLevelList->count.value = nearLevelList->count.value + 1;
        }
        outer++;
    } while (nearLevelList->count.value <= 5);
}
