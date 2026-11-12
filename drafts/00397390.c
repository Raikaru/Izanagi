/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00397390 func_00397390 src/promoted/code1_0039.c - romwright draft, unreviewed */
typedef struct fn_00397390_arg0 fn_00397390_arg0;
struct fn_00397390_arg0 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    unsigned char pad_0x8[0x8];
    int field_0x10; /* +0x10 */
    int field_0x14; /* +0x14 */
};


long FUN_00397390(fn_00397390_arg0 *param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int *piVar6;
  
  iVar1 = param_1->field_0x14;
  iVar5 = 0;
  if (0 < param_1->field_0x4) {
    iVar4 = 0;
    do {
      piVar6 = (int *)(param_1->field_0x10 + iVar4);
      iVar2 = *piVar6;
      iVar3 = FUN_00397470(iVar1);
      if (iVar2 == iVar3) {
        piVar6[3] = iVar1;
      }
      iVar5 = iVar5 + 1;
      iVar4 = iVar4 + 0x10;
    } while (iVar5 < param_1->field_0x4);
  }
  FUN_003e9680(iVar1);
  FUN_003e9af0(iVar1,0x396680,(long)(int)param_1);
  return (long)(int)param_1;
}
