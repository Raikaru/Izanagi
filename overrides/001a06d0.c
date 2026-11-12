/* 001a06d0: turn start. Retail counts eligible ordinary turns of character 1;
 * 001ef7e0 uses that count to select the victory Shuffle Time chance. A
 * projected lineup without Yu instead has a selectedAction leader, so count
 * that action under the same turn-start gates. A present Yu retains retail's
 * identity rule; extra turns, blocked turns and enemies remain uncounted. */
void btlActionStartInit(BtlAction *action) {
    extern u8 *btlEffectFindUnitByIdAndStatus(s32, s32, s32);
    BtlAction *otherAction;
    s32 canAct;
    u32 threshold;
    s16 storedId;
    s32 needsDelay;

    canAct = 1;
    if (action->unit->flags3 & 0x10) {
        action->unk_430 = 2;
        btlActionSetState(action, 0x18);
        return;
    }
    otherAction = (BtlAction *)((BtlBattleState *)D_0076449C)->actions;
    while (otherAction != NULL) {
        if ((action != otherAction) && (otherAction->unk_1a & 1) &&
            (datCalcIsDead(otherAction->unit->datUnit, 0) == 0) &&
            (otherAction->unit->flags3 & 0x10)) {
            canAct = 0;
            break;
        }
        otherAction = otherAction->next;
    }
    btlUnitSelectRecordTable((u8 *)action);
    if (action->flags & 0x8000) {
        btlActionSetState(action, 0x21);
        return;
    }
    btlTargetClearWork(&action->targetBlock);
    if (btlStatusWindowShowSlot2((u8 *)action) != 0) {
        action->unk_440 = (void *)&btlTaskSlotLinkedSlot2;
        action->unk_43c = 2;
        btlActionSetState(action, 0x16);
        return;
    }
    if (action->unit->genus != 0) {
        needsDelay = 0;
    } else if ((((BtlBattleState *)iGpffffb3ac)->unkA70 != -1) &&
               (threshold = btlSideCountNotBad(2, 0x80000) & 0xFFFF,
                (u32)(((BtlBattleState *)iGpffffb3ac)->unkA72 >> 1) >= threshold) &&
               ((storedId = ((BtlBattleState *)iGpffffb3ac)->unkA70) != (s16)btlCameraSelectFormationIndex())) {
        needsDelay = 1;
    } else {
        needsDelay = 0;
    }
    if ((needsDelay != 0) &&
        (datCalcChkBadStatus(action->unit->datUnit, 0x180001) == 0)) {
        btlActionSetStateWithDelay((BtlAction *)((u8 *)action), (u16)(4), (u16)(1));
        return;
    }
    if (((BtlBattleState *)iGpffffb3ac)->flags & 0x80000) {
        canAct = 0;
    }
    if (btlTargetCanControlDamagePhase() == 1) {
        ((BtlBattleState *)iGpffffb3ac)->flags |= 0x2000;
    } else {
        ((BtlBattleState *)iGpffffb3ac)->flags &= ~0x2000;
        btlTargetEndDamagePhase();
    }
    if ((s32)action->unk_28 > 0) {
        if (!(((BtlBattleState *)iGpffffb3ac)->battleFlags & 0x20)) {
            btlDamageStart((u8 *)((BtlBattleState *)iGpffffb3ac)->uiTask, (u8 *)action);
            btlPacketEnqueue(func_001f99c0((u8 *)action, 0xE, 0, 0, 0), 1);
        }
    } else {
        action->flags |= 0x400;
        action->unk_1a &= 0xFFBF;
    }
    if ((action->unk_28 == 0) && (action->unk_29 == 0)) {
        action->unk_41c = datCalcGetSkillMask((u8 *)action->unit->datUnit);
    } else {
        action->unk_41c = 0;
    }
    if ((action->unk_41c != 0) ||
        (datCalcIsGuarding((u16 *)action->unit->datUnit) != 0)) {
        canAct = 0;
    }
    if (canAct != 0) {
        if ((action->unit->genus == 0) &&
            (action->unk_28 == 0) && (action->unk_29 == 0) &&
            (action->unit->charId == 1 ||
             ((u8 *)action == ((BtlBattleState *)iGpffffb3ac)->selectedAction &&
              btlEffectFindUnitByIdAndStatus(1, 1, 0) == NULL &&
              btlEffectFindUnitByIdAndStatus(action->unit->charId, 1, 0) == (u8 *)action))) {
            ((BtlBattleState *)iGpffffb3ac)->protagonistTurns += 1;
        }
        if ((s8)datCalcFindStrongBuff((u8 *)action->unit->datUnit) >= 0) {
            btlActionSetState(action, 0xA);
            return;
        }
        if (btlSoundRollStatusCure((u8 *)action) != 0) {
            btlActionSetState(action, 0xB);
            return;
        }
        btlActionSetState(action, action->state);
    }
}
