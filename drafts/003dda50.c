/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003dda50 func_003dda50 src/promoted/code1_003d.c - romwright draft, unreviewed */
typedef struct fn_003dda50_arg0 fn_003dda50_arg0;
struct fn_003dda50_arg0 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    unsigned char pad_0x8[0x8];
    int field_0x10; /* +0x10 */
    int field_0x14; /* +0x14 */
    unsigned char pad_0x18[0x10];
    int field_0x28; /* +0x28 */
};


long FUN_003dda50(fn_003dda50_arg0 *param_1,unsigned long long param_2,unsigned long long param_3,unsigned int param_4,
                 unsigned int param_5)

{
  int iVar1;
  long lVar2;
  long lVar3;
  
  lVar3 = 0;
  if (param_1 != (fn_003dda50_arg0 *)0x0) {
    FUN_00421820(DAT_00764844);
    iVar1 = 0;
    if (0 < param_1->field_0x4) {
      do {
        lVar2 = (*(code *)param_1->field_0x14)(param_1,iVar1);
        if (*(int *)((int)lVar2 + 0x38) == 1) {
          *(unsigned int *)((int)lVar2 + 0x38) = 5;
          lVar3 = lVar2;
          break;
        }
        iVar1 = iVar1 + 1;
      } while (iVar1 < param_1->field_0x4);
    }
    FUN_00421800(DAT_00764844);
    if (lVar3 != 0) {
      iVar1 = (int)lVar3;
      if (((param_3 & 0x10) != 0) || (*(int *)(iVar1 + 0x30) != 0)) {
        *(unsigned int *)(iVar1 + 0x40) = 2;
        *(unsigned int *)(iVar1 + 0x48) = param_4;
        *(unsigned int *)(iVar1 + 0x4c) = param_5;
      }
      if (param_1->field_0x10 != 0) {
        param_3 = param_3 | 0x10;
        *(unsigned int *)(iVar1 + 0x40) = 2;
        *(unsigned int *)(iVar1 + 0x48) = param_4;
        *(unsigned int *)(iVar1 + 0x4c) = param_5;
      }
      lVar2 = (*(code *)param_1->field_0x28)(param_1,lVar3,param_2,param_3);
      if (lVar2 != 1) {
        *(unsigned int *)(iVar1 + 0x38) = 1;
        lVar3 = 0;
      }
      if (lVar3 != 0) {
        *(unsigned int *)((int)lVar3 + 0x34) = 2;
      }
    }
    else {
      iVar1 = FUN_003ddc10();
      *(unsigned int *)(iVar1 + 0x10) = 8;
      if (DAT_00887194 != (code *)0x0) {
        (*DAT_00887194)(8);
      }
      lVar3 = 0;
    }
  }
  else {
    if (DAT_00887194 != (code *)0x0) {
      (*DAT_00887194)(4);
    }
    lVar3 = 0;
  }
  return lVar3;
}
