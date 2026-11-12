/* Observe only the first scripted-field request; native polling is unchanged. */
#include "p4pc/battle_events.h"
#include "p4pc/battle_events_native.h"

s32 FldEvent_BeginEncounter(s32 task, s32 encounterId, s32 battleFlags)
{
    s32 *h;
    s32 *actor;
    s32 r;

    h = (s32 *)((SdkTask *)task)->work;
    r = 1;
    if (h[0x43] == 0) {
        if (p4_battle_observer_interest & P4_BATTLE_OBSERVE_REQUEST)
            p4_battle_emit_request(P4_BATTLE_FIELD_REQUEST_BEGIN, (u32)encounterId, (u32)task, (u32)battleFlags);
        h[0x45] = h[1];
        h[0x44] = h[4];
        h[1] = 3;
        actor = FldObjFindFree();
        actor[0x12] = datUnitCreateEncounterGroup(encounterId);
        actor[0x13] = D_00724504 + ((encounterId & 0xFFFF) * 24);
        ((FldEventActor *)actor)->partyId = 0;
        h[0x11] = (s32)actor;
        if (p4_battle_observer_interest & P4_BATTLE_OBSERVE_REQUEST)
            p4_battle_emit_request(P4_BATTLE_FIELD_GROUP_CREATED, (u32)encounterId, (u32)task, (u32)actor[0x12]);
        ((FldEventWork *)h)->battleFlags = battleFlags;
        h[0x43] = r;
        goto done;
    }
    actor = (s32 *)FldEvent_FieldToBattle((u8 *)task);
    if (actor != 0) {
        goto done;
    }
    if (D_007EF9F8[0].x == 0) {
        r = 0;
    } else {
        if (datUnitGroupIsDefeated((struct DatUnitGenusBase *)(D_007EF9F8[0].x)) == r) {
            r = (s32)actor | 0x80000000;
        } else {
            r = 0;
            if ((u16)((FldEventWork *)h)->targetHasUnits != 0) {
                r |= 0x40000000;
            }
        }
    }
    h[1] = h[0x45];
    h[4] = h[0x44];
    h[0x43] = 0;
done:
    return r;
}
