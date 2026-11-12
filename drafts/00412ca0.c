/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00412ca0 func_00412ca0 src/promoted/code1_0041.c - romwright draft, unreviewed */
typedef struct fn_00412ca0_arg0 fn_00412ca0_arg0;
struct fn_00412ca0_arg0 {
    int field_0x0; /* +0x0 */
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x14];
    int field_0x20; /* +0x20 */
    int field_0x24; /* +0x24 */
};


unsigned long long FUN_00412ca0(fn_00412ca0_arg0 *param_1)

{
  int iVar1;
  code *pcVar2;
  int iVar3;
  int iVar4;
  long lVar5;
  long lVar6;
  int iVar7;
  unsigned int uVar8;
  int uStack_8;
  unsigned int uStack_4;
  
  lVar6 = (long)(int)param_1;
  if (param_1->field_0x0 != 0) {
    return lVar6;
  }
  uVar8 = *(int *)(DAT_00764aa8 + 0x887318) * 0xb4;
  if (param_1->field_0x8 != 0) {
    if (((unsigned int)param_1->field_0x24 < uVar8) && (lVar5 = FUN_004118c0(lVar6,uVar8), lVar5 == 0)) {
      return 0;
    }
    lVar5 = FUN_00411a70(lVar6,lVar6);
    if (lVar5 == 0) {
      return 0;
    }
  }
  else {
    iVar7 = (*DAT_008873e8)(uVar8,0x1030409);
    param_1->field_0x20 = iVar7;
    if (param_1->field_0x20 == 0) {
      uStack_8 = 1;
      uStack_4 = FUN_003df590(0xffffffff80000013,uVar8);
      FUN_003df4d0(&uStack_8);
      return 0;
    }
    param_1->field_0x24 = uVar8;
    param_1->field_0x8 = param_1->field_0x20;
  }
  param_1->field_0x0 = 1;
  if ((param_1->field_0x8 != 0) && (uVar8 = 0, param_1->field_0x4 != 0)) {
    iVar7 = 0;
    do {
      iVar1 = *(int *)(param_1->field_0x8 + iVar7);
      pcVar2 = *(code **)(iVar1 + 0x14);
      if (pcVar2 != (code *)0x0) {
        (*pcVar2)();
        iVar3 = param_1->field_0x8;
      }
      else {
        iVar3 = param_1->field_0x8;
      }
      iVar4 = *(int *)(*(int *)(iVar3 + iVar7) + 0x3c) - 1;
      *(int *)(*(int *)(iVar3 + iVar7) + 0x3c) = iVar4;
      if ((iVar4 == 0) && (pcVar2 = *(code **)(iVar1 + 0xc), pcVar2 != (code *)0x0)) {
        (*pcVar2)(*(unsigned int *)(param_1->field_0x8 + iVar7));
      }
      uVar8 = uVar8 + 1;
      *(unsigned int *)(param_1->field_0x8 + iVar7 + 0xc) = 0;
      iVar7 = iVar7 + 0x28;
    } while (uVar8 < (unsigned int)param_1->field_0x4);
  }
  return lVar6;
}
