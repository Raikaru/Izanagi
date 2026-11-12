/* 001e1800: battle script command 0x205D, "does the member with this
 * character id on my side have bad status N" (condition 11, btlCond_MYBAD,
 * 001d9b60: datCalcChkBadStatus). Retail finds the member with
 * btlEffectFindUnitByIdAndStatus over the caller's side, skipping the dead
 * unless N itself carries the KO bit 0x80000, and answers 0 when nothing
 * matches.
 *
 * The ally AI library (BTL/BATTLE/AICALC.TBL block 9) weighs the
 * protagonist's ailments by this command for character 1 alone:
 * GET_Point_BadStatus adds a bonus of 2 or 3 to a status's cure score when he
 * has it (3 for most statuses once at least two allies do); SET_Bad_Priority, whose
 * target the cure actions use, takes him first for the chosen status, ahead
 * of characters 4, 8, 2, 7, 3 and 6 (and 0x207B, override 001e6d20, answers
 * that pick with the leader action); ACT_EndBattle_GATK, ACT_1more_ATK and
 * ACT_1more_MGK attack only while he is free of status 1.
 *
 * A lineup projected without Yu (p4.battle.formation, override 001b1d70)
 * gives his leader role to its first fighter. This command finds no
 * character 1 and answers 0, so the leader's ailments lose that weight and
 * first place, and the attack gates ignore them.
 *
 * So the request for character 1 on the player side, when no action of that
 * side carries character 1 at all (any status, as 001e6d20 and 001e1530 test),
 * asks instead about the character holding the leader role: the battle's
 * selectedAction, which btlActionCreateWork sets to Yu's action and no other
 * retail code writes. That character goes through the same retail lookup with
 * the same status filter, and the same retail condition. Where Yu is in the
 * lineup, alive or dead, his own lookup decides as in retail; other ids
 * (including Cnv_UID_to_PID's own-unit identity, which uses 0x200E), the enemy
 * side and a battle with no leader action keep the retail answer. No mod state
 * is read. */
u32 btlAICommandSideIdQueryMyBad(void)
{
    u16 id;
    u8 *unit;
    u32 argument;
    u32 sideMask;
    u32 statusMask;
    u8 *context;
    u8 *leader;
    u32 dispatchType;
    u32 payload;
    s32 result;
    s32 dispatchResult;

    argument = scrGetParamInt(1);
    unit = scrGetAiState();
    sideMask = 1 << (((BtlAction *)unit))->unit->genus & 0xffff;
    if ((argument & 0x80000) != 0) {
        statusMask = 0;
    } else {
        statusMask = 0x80000;
    }
    id = scrGetParamInt(0);
    context = (u8 *)btlEffectFindUnitByIdAndStatus(id, sideMask, statusMask);
    if (context == 0 && id == 1 && sideMask == 1
        && btlEffectFindUnitByIdAndStatus(1, 1, 0) == 0) {
        leader = ((BtlBattleState *)iGpffffb3ac)->selectedAction;
        if (leader != 0) {
            context = (u8 *)btlEffectFindUnitByIdAndStatus(((BtlAction *)leader)->unit->charId, sideMask, statusMask);
        }
    }
    if (context != 0) {
        dispatchType = (((argument & 0xffffff) | 0x0b000000) & 0xff000000) >> 24;
        payload = (argument | 0x0b000000) & 0xffffff;
        if (dispatchType == 0)
            dispatchResult = 0;
        else
            dispatchResult = ((s32 (*)(u8 *, u32))D_00609850[dispatchType * 3])(context, payload);
        result = dispatchResult;
    } else {
        result = 0;
    }
    scrSetReturnInt(result != 0);
    return 1;
}
