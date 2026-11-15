/* 001e6d20: battle script command 0x207B, "publish the action of the character
 * with this id". Kind 0 searches the player side with no status filter, so a
 * dead or downed member still matches and the lookup fails only when no active
 * action carries that character id; kind 1 searches the enemy side, skipping
 * the dead. Retail then loads result->id with no test (lw 8($v0) at 001e6d94),
 * so a failed lookup, or any kind but 0 and 1, reads through NULL.
 *
 * The ally AI library (BTL/BATTLE/AICALC.TBL block 9) asks for character 1,
 * the protagonist, on the player side without checking he is present: in
 * GET_Player_Parameter, the first procedure main_AI_routine calls, and in
 * SET_Heal behind an HP-tier comparison only. SET_Heal hands the published id
 * to the GetHp and GetMaxHp commands (0x20E7, 0x20E6), which resolve it with
 * btlActionFindById and read through the result unguarded. Every other kind-0
 * request in that library (SET_Heal_Priority_FR, SET_Bad_Priority,
 * Cnv_UID_to_PID) first passes a presence check (0x205A, 0x205D or 0x200E),
 * each answering 0 for an absent member; the enemy library (block 10) never
 * runs this command. With Yu in every retail lineup none of this can fail, but
 * a lineup projected without him (p4.battle.formation, override 001b1d70)
 * reaches both NULL reads, and a "no unit" value alone would only move the
 * fault into GetHp.
 *
 * So the request for the protagonist's action, when no action carries his
 * character id, answers with the action that holds his leader role: the
 * battle's selectedAction, which btlActionCreateWork sets to Yu's action and
 * no other retail code writes, and which 001b1d70 hands to a projected
 * lineup's first fighter. In retail that lookup finds Yu's own action, which
 * is that same action, so this answers only where retail would read NULL; no
 * mod state is read. Any other unresolved request publishes -1, the value
 * retail's btlEffectScriptPickLowestHpNormalUnit publishes when nothing
 * matches. Any request whose character is present keeps the retail lookup and
 * the retail result. */
s32 btlEffectScriptPickUnitByKind(void) {
    s32 first;
    s32 second;
    s32 kind;
    u8 *result;

    first = scrGetParamIntS32(0) & 0xFFFF;
    second = scrGetParamIntS32(1) & 0xFFFF;
    result = 0;
    kind = first & 0xFFFF;
    switch (kind) {
    case 0:
        result = btlEffectFindUnitByIdAndStatus(second, 1, 0);
        break;
    case 1:
        result = btlEffectFindUnitByIdAndStatus(second, 2, 0x80000);
        break;
    }
    if (result == NULL) {
        if (kind == 0 && second == 1) {
            result = ((BtlBattleState *)iGpffffb3ac)->selectedAction;
        }
        if (result == NULL) {
            scrSetReturnInt(-1);
            return 1;
        }
    }
    scrSetReturnInt((s32)((BtlAction *)result)->id | 0x80000000);
    return 1;
}
