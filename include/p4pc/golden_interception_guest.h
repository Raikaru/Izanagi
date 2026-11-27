/* Native persistent services used by the Golden guest result resolver. */
#ifndef P4PC_GOLDEN_INTERCEPTION_GUEST_H
#define P4PC_GOLDEN_INTERCEPTION_GUEST_H
#include "p4pc/golden_battle_rows_native.h"
extern u16 cmmCommunityForArcana(s64 arcanaValue);
extern u16 cmmCommunityRecordValue(s32 communityId);
extern u8 *btlPacketAlloc(s32 packetType, s32 workSize);
extern s32 sdkSndStartIndexedSource(s32 sourceAux, s32 channelIndex, s32 cueId);
extern void btlPanelStartValues(s32 task, s32 unit, s32 window, s32 record);
#endif
