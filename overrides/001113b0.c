#include "p4pc/mod_save_native.h"
void datInitDefaultGameState(void)
{
    s32 j;
    s32 i;
    u8 *p;
    u8 *q;
    u16 datCalcMaxHpById(s16);
    u16 datCalcMaxSpById(s16);
    void datSetCategoryValueById(s16, s16, s16);
    void datCalcInitUnitById(s16);
    void func_00105690(s16, u8);
    void datSetUnitHp(s16, s16);
    void datSetUnitSp(s16, s16);
    void datSetHeroExp(s16, u32);
    void func_00105c90(s16, s16);
    void datCalcSetBadStatusById(s16, u32);
    void func_00105dc0(s16, s16, s16);
    void datSetMoney(u32);
    void datSetPartyId(s32, s16);
    void datSetDateIndex(s16);
    void datSetTimeSlot(u8);
    void datSetPendingDateIndex(s16);
    void datSetPendingTimeSlot(u8);
    void datSetPendingFlag(u32);
    void func_00106480(void);
    void datPersonaClearRosterMode(void);
    void func_001065c0(void);
    void func_00106d40(s16, s16, s16);
    void cmmCommunityClearTable(void);
    s32 datPersonaPlaceRosterMember(s32, u16);
    void datPersonaResetRosterMode(s16);
    void func_0010d490(u8 *, u8 *);
    void func_0010f390(void);
    void func_0010f4c0(void);
    void func_0010fc80(void);
    void MT_Scene_ResetFrameState(void);
    void FldStepResetTables(void);
    void FldStateTablesClear(void);
    extern u16 D_005E4630[];
    extern s16 D_005E4650[];

    if (p4_mod_save_new_game_begin())
        p4_mod_save_restore_failed();
    calSetStateImagePending(0);
    datSetPendingFlag(0);
    func_0010d490(iGpffff9bb8, iGpffff9bc0);
    datSetMoney(0);
    func_0010fc80();

    i = 1;
    goto loop_1_check;
loop_1:
    datCalcInitUnitById((s16)i);
    if (i == 1) {
        func_00105690((s16)i, 1);
        datSetHeroExp((s16)i, 0);
    }
    datSetUnitHp((s16)i, 1);
    datSetUnitSp((s16)i, 1);
    j = 0;
    goto loop_2_check;
loop_2:
    datSetCategoryValueById((s16)i, (s16)j, 0);
    j++;
loop_2_check:
    if (j < 5) {
        goto loop_2;
    }
    func_00105c90((s16)i, 0);
    datCalcSetBadStatusById((s16)i, 0);
    func_00105dc0((s16)i, 0, -1);
    func_00105dc0((s16)i, 1, -1);
    func_00105dc0((s16)i, 2, -1);
    datPersonaResetRosterMode((s16)i);
    q = (u8 *)D_005E4630 + i * 2;
    datPersonaPlaceRosterMember(i & 0xFFFF, *(u16 *)(q - 2));
    datSetUnitHp((s16)i, (s16)datCalcMaxHpById((s16)i));
    datSetUnitSp((s16)i, (s16)datCalcMaxSpById((s16)i));
    i++;
loop_1_check:
    if (i < 11) {
        goto loop_1;
    }

    func_00106480();
    func_001065c0();
    datPersonaClearRosterMode();
    datSetTimeSlot(5);
    datSetDateIndex(9);
    datSetPendingTimeSlot(5);
    datSetPendingDateIndex(9);
    i = 0;
    goto loop_3_check;
loop_3:
    datItemSetCount((s16)i, 0);
    i++;
loop_3_check:
    if (i < 1536) {
        goto loop_3;
    }

    i = 1;
    goto loop_4_check;
loop_4:
    p = (u8 *)D_005E4650 + i * 6;
    func_00106d40((s16)i, 0, *(s16 *)(p - 6));
    func_00106d40((s16)i, 1, *(s16 *)(p - 4));
    func_00106d40((s16)i, 2, *(s16 *)(p - 2));
    i++;
loop_4_check:
    if (i < 11) {
        goto loop_4;
    }

    datSetPartyId(0, 0);
    datSetPartyId(1, 0);
    datSetPartyId(2, 0);
    datSetPartyId(3, 0);
    MT_Scene_ResetFrameState();
    FldStateTablesClear();
    FldStepResetTables();
    func_0010f390();
    func_0010f4c0();
    cmmCommunityClearTable();

    i = 0;
    goto loop_5_check;
loop_5:
    datSetStateTableEntry(i, 0);
    i++;
loop_5_check:
    if (i < 1536) {
        goto loop_5;
    }
    iGpffffb1f0 = 0;
    iGpffffb19c = 0;
    datSetFlag(0x39, 1);
    datSetFlag(0x3A, 1);
    datSetFlag(0x3B, 0);
    datSetFlag(0x3C, 1);
    datSetFlag(0x3D, 1);
    datSetFlag(0x3E, 0);
    p4_mod_save_new_game_commit();
}
