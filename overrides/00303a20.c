#include "p4pc/game_mode.h"

s32 fclCombineCheckIds(u8 *task)
{
    s8 *p = ((s8 *)((KwlnTask *)task)->work);
    FclCombineWork *menuState = (FclCombineWork *)p;
    u16 buf[7];
    if (p4_game_mode() == P4_GAME_MODE_GOLDEN && p[0x1A] == 7) {
        u16 wide[13];
        for (s32 i = 0; i < 12; ++i) wide[i] = ((DatPersonaRecord *)yListSlotEntry(0, i))->id;
        wide[12] = ((DatPersonaRecord *)yListSlotEntry(p[0x2F9], p[0x2FA]))->id;
        return fclCombineCheckRules(task, 12, wide);
    }
    switch (p[0x1A]) {
    case 2:
        buf[0] = ((DatPersonaRecord *)(yListSlotEntry(0, p[0x128])))->id;
        buf[1] = ((DatPersonaRecord *)(yListSlotEntry(0, ((FclCombineWork *)p)->f11E)))->id;
        buf[2] = ((DatPersonaRecord *)(yListSlotEntry(p[0x2F9], p[0x2FA])))->id;
        break;
    case 3:
        buf[0] = ((DatPersonaRecord *)(yListSlotEntry(0, p[0x128])))->id;
        buf[1] = ((DatPersonaRecord *)(yListSlotEntry(0, p[0x129])))->id;
        buf[2] = ((DatPersonaRecord *)(yListSlotEntry(0, ((FclCombineWork *)p)->f11E)))->id;
        buf[3] = ((DatPersonaRecord *)(yListSlotEntry(p[0x2F9], p[0x2FA])))->id;
        break;
    case 4:
        buf[0] = ((DatPersonaRecord *)(yListSlotEntry((s8)(((FclCombineWork *)p)->f11E + 1), 0)))->id;
        buf[1] = ((DatPersonaRecord *)(yListSlotEntry((s8)(menuState->f11E + 1), 1)))->id;
        buf[2] = ((DatPersonaRecord *)(yListSlotEntry((s8)(menuState->f11E + 1), 2)))->id;
        buf[3] = ((DatPersonaRecord *)(yListSlotEntry((s8)(menuState->f11E + 1), 3)))->id;
        buf[4] = ((DatPersonaRecord *)(yListSlotEntry(p[0x2F9], p[0x2FA])))->id;
        break;
    case 5:
        buf[0] = ((DatPersonaRecord *)(yListSlotEntry((s8)(((FclCombineWork *)p)->f11E + 1), 0)))->id;
        buf[1] = ((DatPersonaRecord *)(yListSlotEntry((s8)(menuState->f11E + 1), 1)))->id;
        buf[2] = ((DatPersonaRecord *)(yListSlotEntry((s8)(menuState->f11E + 1), 2)))->id;
        buf[3] = ((DatPersonaRecord *)(yListSlotEntry((s8)(menuState->f11E + 1), 3)))->id;
        buf[4] = ((DatPersonaRecord *)(yListSlotEntry((s8)(menuState->f11E + 1), 4)))->id;
        buf[5] = ((DatPersonaRecord *)(yListSlotEntry(p[0x2F9], p[0x2FA])))->id;
        break;
    case 6:
        buf[0] = ((DatPersonaRecord *)(yListSlotEntry((s8)(((FclCombineWork *)p)->f11E + 1), 0)))->id;
        buf[1] = ((DatPersonaRecord *)(yListSlotEntry((s8)(menuState->f11E + 1), 1)))->id;
        buf[2] = ((DatPersonaRecord *)(yListSlotEntry((s8)(menuState->f11E + 1), 2)))->id;
        buf[3] = ((DatPersonaRecord *)(yListSlotEntry((s8)(menuState->f11E + 1), 3)))->id;
        buf[4] = ((DatPersonaRecord *)(yListSlotEntry((s8)(menuState->f11E + 1), 4)))->id;
        buf[5] = ((DatPersonaRecord *)(yListSlotEntry((s8)(menuState->f11E + 1), 5)))->id;
        buf[6] = ((DatPersonaRecord *)(yListSlotEntry(p[0x2F9], p[0x2FA])))->id;
        break;
    }
    return fclCombineCheckRules(task, p[0x1A], buf);
}
