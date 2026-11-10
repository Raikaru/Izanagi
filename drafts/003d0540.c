/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d0540 func_003d0540 src/promoted/code1_003d.c - romwright draft, unreviewed */
typedef struct fn_003d0540_arg0 fn_003d0540_arg0;
struct fn_003d0540_arg0 {
    unsigned char pad_0x0[0x8];
    int field_0x8; /* +0x8 */
    undefined4 field_0xc; /* +0xc */
    undefined4 field_0x10; /* +0x10 */
    unsigned char pad_0x14[0x8];
    undefined4 field_0x1c; /* +0x1c */
    int field_0x20; /* +0x20 */
    unsigned char pad_0x24[0x6];
    undefined1 field_0x2a; /* +0x2a */
};


unsigned int FUN_003d0540(fn_003d0540_arg0 *param_1,unsigned int *param_2)

{
  int iVar1;
  float *pfVar2;
  unsigned long long uVar3;
  int *piVar4;
  float *pfVar5;
  unsigned char bVar6;
  unsigned int uVar7;
  int iVar8;
  
  iVar1 = param_1->field_0x8;
  iVar8 = *(int *)(iVar1 + 0x18);
  param_1->field_0xc = *(unsigned int *)(iVar8 + 0x54);
  if (*(int *)(iVar8 + 0x18) == 1) {
    piVar4 = (int *)(iVar8 + DAT_00764820);
    bVar6 = 0;
    if (((unsigned int *)*piVar4 != (unsigned int *)0x0) &&
       ((unsigned int)*(unsigned short *)(*(int *)(iVar8 + 0x54) + 4) == *(unsigned int *)*piVar4)) {
      bVar6 = 1;
    }
    if (bVar6) {
      iVar8 = *piVar4;
    }
    else {
      piVar4 = (int *)FUN_003ce050(piVar4,*(unsigned short *)(*(int *)(iVar8 + 0x54) + 4));
      iVar8 = *piVar4;
    }
    param_1->field_0x10 = iVar8;
  }
  else {
    piVar4 = (int *)(iVar1 + DAT_0076481c);
    bVar6 = 0;
    if (((unsigned int *)*piVar4 != (unsigned int *)0x0) &&
       ((unsigned int)*(unsigned short *)(*(int *)(iVar8 + 0x54) + 4) == *(unsigned int *)*piVar4)) {
      bVar6 = 1;
    }
    if (bVar6) {
      iVar8 = *piVar4;
    }
    else {
      piVar4 = (int *)FUN_003ce050(piVar4,*(unsigned short *)(*(int *)(iVar8 + 0x54) + 4));
      iVar8 = *piVar4;
    }
    param_1->field_0x10 = iVar8;
  }
  param_1->field_0x20 = *(int *)(*(int *)(iVar1 + 0x18) + 0x18);
  if (1 < param_1->field_0x20) {
    param_1->field_0x1c = *(float *)(iVar1 + 0x58) * *(float *)(iVar1 + 0x5c);
  }
  FUN_003cfff0(iVar1,param_1);
  *(unsigned short *)(*(int *)(iVar1 + 0x18) + 0xc) = 0;
  *(unsigned int *)(iVar1 + 0x4c) = *(unsigned int *)(iVar1 + 0x4c) & 0xfffffffe;
  iVar8 = DAT_008872e0 + 0x20;
  uVar3 = FUN_003e9700(*(unsigned int *)(iVar1 + 4));
  FUN_003e05f0(*param_2,uVar3,iVar8);
  uVar7 = 2;
  pfVar2 = (float *)FUN_003bfae0(iVar1);
  iVar8 = 5;
  pfVar5 = (float *)(DAT_008872e0 + DAT_00764a24 + 0x60);
  do {
    if (-pfVar2[3] < (pfVar2[2] * pfVar5[2] + *pfVar2 * *pfVar5 + pfVar2[1] * pfVar5[1]) - pfVar5[3]
       ) {
      uVar7 = 1;
      break;
    }
    pfVar5 = pfVar5 + 5;
    bVar6 = iVar8 != 0;
    iVar8 = iVar8 - 1;
  } while (bVar6);
  FUN_003d0140(param_1,uVar7);
  param_1->field_0x2a = (*(unsigned int *)(*(int *)(iVar1 + 0x18) + 8) & 0x40) != 0;
  FUN_003d0230(param_1);
  return 1;
}
