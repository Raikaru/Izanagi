#include "p4pc/lockstep_align_guest.h"
#include "p4pc/mod_save_native.h"
void func_0010e710(s32 arg0, u8 *arg1, s32 arg2)
{
    u32 v1;
    u32 v2;
    u8 *p = arg1 + 0x34;
    /* Decode/validation and the player's missing-mod acknowledgement happened
     * before the full MC read succeeded. Prove the exact image before GP writes. */
    if (p4_mod_save_restore_begin(arg0,arg1,arg2))
        p4_mod_save_restore_failed();
    while (1)
    {
        memcpy(&v1, p, 4);
        func_00440b68(D_005E45C0, v1);
        if (v1 == -1 || v1 == 0x2000)
            break;
        memcpy(&v2, p + 4, 4);
        p = p + 8;
        datSaveRestoreSection(arg0, v1, v2, p);
        p = p + v2;
    }
    iGpffffb19c = D_0079B67C[0];
    p4pc_align_restore_applied();
    memset(DAT_00796de0, 0, 0x12);
    memset(DAT_00796e00, 0, 0x12);
    memset(DAT_00796e20, 0, 0x24);
    memcpy(DAT_00796de0, DAT_007973a0, 0x12);
    memcpy(DAT_00796e00, DAT_007973b2, 0x12);
    sprintf(DAT_00796e20, &iGpffff9bb0, DAT_00796e00, DAT_00796de0);
    p4_mod_save_restore_commit();
}
