/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d3bd0 func_003d3bd0 src/promoted/code1_003d.c - romwright draft, unreviewed */
typedef struct fn_003d3bd0_arg0 fn_003d3bd0_arg0;
struct fn_003d3bd0_arg0 {
    unsigned char pad_0x0[0x8];
    int field_0x8; /* +0x8 */
    int field_0xc; /* +0xc */
    int field_0x10; /* +0x10 */
    int field_0x14; /* +0x14 */
    int field_0x18; /* +0x18 */
    unsigned char pad_0x1c[0xc];
    byte field_0x28; /* +0x28 */
    undefined1 field_0x29; /* +0x29 */
    undefined1 field_0x2a; /* +0x2a */
};


unsigned int FUN_003d3bd0(fn_003d3bd0_arg0 *param_1,int *param_2)

{
  int iVar1;
  unsigned int uVar2;
  unsigned char bVar3;
  int iVar4;
  int *piVar5;
  int iStack_4;
  
  iVar1 = param_1->field_0x8;
  param_1->field_0xc = *(int *)(iVar1 + 0x78);
  piVar5 = (int *)(iVar1 + DAT_00764818);
  bVar3 = 0;
  if (((unsigned int *)*piVar5 != (unsigned int *)0x0) &&
     ((unsigned int)*(unsigned short *)(param_1->field_0xc + 4) == *(unsigned int *)*piVar5)) {
    bVar3 = 1;
  }
  if (bVar3) {
    iVar4 = *piVar5;
  }
  else {
    piVar5 = (int *)FUN_003ce050(piVar5,*(unsigned short *)(param_1->field_0xc + 4));
    iVar4 = *piVar5;
  }
  param_1->field_0x10 = iVar4;
  if ((*(unsigned int *)(DAT_008872e4 + 8) & 0x1000000) == 0) {
    param_1->field_0x18 =
         (unsigned int)*(unsigned short *)(param_1->field_0xc + 6) | ((*(unsigned int *)(DAT_008872e4 + 8) & 0x9d) << 0x10);
    param_1->field_0x14 = param_1->field_0x14 & 0xfffffffe;
    iVar4 = *(int *)(param_1->field_0x10 + 4);
    if (iVar4 != 0) {
      uVar2 = *(unsigned int *)(iVar4 + 0x28);
      if (param_1->field_0x18 != uVar2) {
        if ((((unsigned int)param_1->field_0x18 >> 0x10) & 0xff) != ((uVar2 >> 0x10) & 0xff)) {
          param_1->field_0x14 = param_1->field_0x14 | 8;
        }
        else {
          param_1->field_0x14 = param_1->field_0x14 | 4;
        }
      }
    }
  }
  *param_2 = DAT_008872e0 + 0x20;
  FUN_003d3ae0(iVar1,&iStack_4);
  uVar2 = *(unsigned int *)param_1->field_0xc;
  if ((uVar2 & 1) != 1) {
    if (((uVar2 & 0xff) != 0) && ((uVar2 & 2) != 2)) {
      if ((uVar2 & 0x10) != 0x10) {
        if ((uVar2 & 8) == 8) {
          param_1->field_0x29 = 2;
          param_1->field_0x28 = DAT_00764a74 | 0x10;
        }
        else {
          param_1->field_0x29 = 1;
          param_1->field_0x28 = DAT_00764a74 | 0x14;
        }
      }
      else {
        param_1->field_0x29 = 4;
        param_1->field_0x28 = DAT_00764a74;
      }
    }
    else {
      param_1->field_0x29 = 3;
      param_1->field_0x28 = DAT_00764a74 | 4;
    }
  }
  else {
    param_1->field_0x29 = 4;
    param_1->field_0x28 = DAT_00764a74;
  }
  if (iStack_4 != 2) {
    param_1->field_0x28 = param_1->field_0x28 | 2;
  }
  param_1->field_0x2a = (*(unsigned int *)(DAT_008872e4 + 8) & 0x40) != 0;
  iVar1 = param_1->field_0x8;
  if ((*(unsigned int *)(DAT_008872e4 + 8) & 0x20) != 0) {
    FUN_003c96d0(0x3d3a90,0);
    FUN_003c9750(iVar1,0x3d3a90,0);
  }
  return 1;
}
