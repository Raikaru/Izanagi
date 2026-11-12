/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003dd290 func_003dd290 src/promoted/code1_003d.c - romwright draft, unreviewed */
typedef struct fn_003dd290_arg0 fn_003dd290_arg0;
struct fn_003dd290_arg0 {
    unsigned char pad_0x0[0x4];
    unsigned char field_0x4; /* +0x4 */
    unsigned char pad_0x5[0x3];
    undefined4 field_0x8; /* +0x8 */
    undefined4 field_0xc; /* +0xc */
    int field_0x10; /* +0x10 */
    int field_0x14; /* +0x14 */
    unsigned char pad_0x18[0x8];
    undefined4 field_0x20; /* +0x20 */
    undefined4 field_0x24; /* +0x24 */
};


long FUN_003dd290(fn_003dd290_arg0 *param_1,int param_2,unsigned long long param_3,unsigned long long param_4)

{
  int iVar1;
  long lVar2;
  int iVar3;
  
  lVar2 = FUN_003dd7d0();
  if (lVar2 == 0) {
    lVar2 = 2;
  }
  else {
    lVar2 = FUN_003dd990(param_3);
    if (lVar2 != 0) {
      lVar2 = 6;
    }
    else {
      (*DAT_008873ac)(param_1 + 2,param_3);
      *(int *)&param_1->field_0x4 = param_2;
      iVar3 = 0;
      param_1->field_0x10 = 0;
      if (0 < param_2) {
        do {
          iVar1 = (*(code *)param_1->field_0x14)(param_1,iVar3);
          iVar3 = iVar3 + 1;
          *(unsigned int *)(iVar1 + 0x38) = 1;
          *(int *)(iVar1 + 0x30) = param_1->field_0x10;
        } while (iVar3 < param_2);
      }
      param_1->field_0xc = 0;
      lVar2 = FUN_003dd620(param_1,param_4);
      if (lVar2 == 1) {
        param_1->field_0x8 = 2;
        param_1->field_0x20 = 0;
        param_1->field_0x24 = 0;
        lVar2 = 1;
      }
    }
  }
  return lVar2;
}
