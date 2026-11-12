/* The retail script request owns the parameter read and all construction.
 * Observation is synchronous and inactive unless a native broker subscribes. */
#include "p4pc/battle_events.h"
#include "p4pc/battle_events_native.h"

s32 btlScriptStartEncounter(void)
{
    BtlEncounterSetup setup;
    s32 partyIndex;
    u16 partyCount;
    u16 encounterId;
    s32 slotIndex;
    u16 partyId;
    u16 formation;
    BtlEncounterRecord *encounter;

    if (p4_battle_observer_interest & P4_BATTLE_OBSERVE_REQUEST)
        p4_battle_emit_request(P4_BATTLE_SCRIPT_REQUEST_BEGIN, 0, 0, 0);
    hFadeTaskStart(1);
    encounterId = scrGetParamInt(0) & 0xFFFF;
    if (p4_battle_observer_interest & P4_BATTLE_OBSERVE_REQUEST)
        p4_battle_emit_request(P4_BATTLE_SCRIPT_REQUEST_IDENTIFIED, encounterId, 0, 0);
    encounter = &((BtlEncounterRecord *)iGpffffb414)[encounterId];
    memset(&setup, 0, 0x3C);
    setup.partyUnits[0] = (s32)datUnitCreatePc(1);
    partyCount = 1;
    partyIndex = 0;
    goto loop_5_check;
loop_5_body:
    partyId = datGetPartyId(slotIndex) & 0xFFFF;
    if ((partyId != 0) && (partyCount < 4)) {
        setup.partyUnits[partyCount++] = (s32)datUnitCreatePc(partyId);
    }
    partyIndex = (partyIndex + 1) & 0xFFFF;
loop_5_check:
    slotIndex = partyIndex & 0xFFFF;
    if (slotIndex < 4) {
        goto loop_5_body;
    }
    setup.enemyUnits[0] = (s32)datUnitCreateEncounterGroup(encounterId);
    if (p4_battle_observer_interest & P4_BATTLE_OBSERVE_REQUEST)
        p4_battle_emit_request(P4_BATTLE_SCRIPT_GROUP_CREATED, encounterId, (u32)setup.enemyUnits[0], 0);
    formation = encounter->formation;
    if ((formation == 0) && (encounter->formationVariant == 0)) {
        setup.formation = 0xF0;
        setup.formationVariant = 1;
    } else {
        setup.formation = formation;
        setup.formationVariant = encounter->formationVariant;
    }
    sequenceRequest(6, &setup, 0x3C, 4);
    return 1;
}
