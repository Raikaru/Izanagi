/* 001e1530: battle script command 0x205A, "does the living member with this
 * character id on my side meet HP condition N" (btlCond_MYHP, 001d9740:
 * hp * 100 <= maxHp * N). Retail finds the member with
 * btlEffectFindUnitByIdAndStatus over the caller's side, skipping the dead,
 * and answers 0 when nothing matches.
 *
 * The ally AI library (BTL/BATTLE/AICALC.TBL block 9) ranks the protagonist
 * by this command alone: GET_Player_Parameter asks it for character 1 at 40
 * and 60 percent and keeps the answer as his HP tier (local 18, 2/1/0). The
 * tier of everyone else (local 20) is computed only for the lowest-HP living
 * ally, and only when that ally is not the action the protagonist lookup
 * (0x207B kind 0, override 001e6d20) publishes. AI_pattern_Free, _Damage,
 * _SaveSP and _SaveHP heal on these tiers, and SET_Heal targets that
 * protagonist action when local 18 is at least local 20.
 *
 * A lineup projected without Yu (p4.battle.formation, override 001b1d70)
 * gives his leader role to its first fighter, and 001e6d20 answers the
 * protagonist lookup with that leader action. So once the leader is the
 * lowest-HP ally, local 20 is never computed, while this command finds no
 * character 1 and leaves local 18 at 0: with two or more fighters the leader's
 * own low HP never reaches a tier, and no heal is chosen for it.
 *
 * So the request for character 1 on the player side, when no action of that
 * side carries character 1 at all (any status, as 001e6d20 tests), asks
 * instead about the character holding the leader role: the battle's
 * selectedAction, which btlActionCreateWork sets to Yu's action and no other
 * retail code writes. That character goes through the same retail lookup, so
 * a dead or inactive leader still answers 0, and the same retail condition.
 * Where Yu is in the lineup, alive or dead, his own lookup decides as in
 * retail; other ids, the enemy side and a battle with no leader action keep
 * the retail answer. No mod state is read. */
u32 btlAICommandSideIdQueryMyHp(void)
{
    u8 *unit;
    u16 id;
    u32 sideMask;
    u8 *context;
    u8 *leader;
    u32 selector;
    u32 dispatchType;
    u32 payload;
    s32 result;
    s32 dispatchResult;

    unit = scrGetAiState();
    id = scrGetParamInt(0) & 0xFFFF;
    sideMask = 1 << (((BtlAction *)unit))->unit->genus & 0xffff;
    context = (u8 *)btlEffectFindUnitByIdAndStatus(id, sideMask, 0x80000);
    if (context == 0 && id == 1 && sideMask == 1
        && btlEffectFindUnitByIdAndStatus(1, 1, 0) == 0) {
        leader = ((BtlBattleState *)iGpffffb3ac)->selectedAction;
        if (leader != 0) {
            context = (u8 *)btlEffectFindUnitByIdAndStatus(((BtlAction *)leader)->unit->charId, sideMask, 0x80000);
        }
    }
    if (context != 0) {
        selector = (scrGetParamInt(1) & 0xffffff) | 0x01000000;
        dispatchType = (selector & 0xff000000) >> 24;
        payload = selector & 0xffffff;
        if (dispatchType == 0) {
            dispatchResult = 0;
        } else {
            dispatchResult = ((s32 (*)(u8 *, u32))D_00609850[dispatchType * 3])(context, payload);
        }
        result = dispatchResult;
    } else {
        result = 0;
    }
    scrSetReturnInt(result != 0);
    return 1;
}
