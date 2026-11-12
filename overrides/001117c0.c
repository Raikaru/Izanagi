/* 001117c0: authenticate before the retail copy; publish expansion later. */
#include "p4pc/game_table_admission_native.h"
#include "p4pc/game_items_native.h"
s32 initDataReadStep(u8 *task)
{
    InitDataReadWork *readWork;
    u8 *work;
    s32 state;
    u8 *file;

    work = ((KwlnTask *)task)->work;
    readWork = (InitDataReadWork *)work;
    state = ((InitDataReadWork *)work)->state;
    switch (state) {
    case 0:
        func_00440b68(&D_00762CB8, &D_00762CC0, 0x1FA);
        ((InitDataReadWork *)work)->file = H_Cdvd_AcquireDirectory(D_005E46A0, 0);
        ((InitDataReadWork *)work)->state += 1;
        break;
    case 1:
        if (H_Cdvd_IsFileLoaded((HCdvd *)((InitDataReadWork *)work)->file) == 1) {
            s32 result;
            file = readWork->file;
            p4_game_table_admission_item((const char *)D_005E46A0, ((HCdvd *)file)->fileMemory,
                MEMBER(s32, file, HCdvd, fileSize));
            result = p4_game_tables_items_native_load((const char *)D_005E46A0, ((HCdvd *)file)->fileMemory,
                MEMBER(s32, file, HCdvd, fileSize));
            if (result) p4_game_tables_items_native_refuse(result);
            memcpy(&D_0079BF00, ((HCdvd *)file)->fileMemory, MEMBER(s32, file, HCdvd, fileSize));
            H_Cdvd_Destroy((HCdvd *)readWork->file);
            readWork->file = NULL;
            datEntryTableUseBuiltin();
            p4_game_tables_items_native_use_builtin();
            return -1;
        }
        break;
    }
    return 0;
}
