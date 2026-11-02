/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003a8d20 func_003a8d20 src/promoted/code1_003a.c - romwright draft, unreviewed */
typedef struct fn_003a8d20_arg0 fn_003a8d20_arg0;
struct fn_003a8d20_arg0 {
    unsigned char pad_0x0[0x8];
    int field_0x8; /* +0x8 */
    int field_0xc; /* +0xc */
    int field_0x10; /* +0x10 */
    unsigned char field_0x14; /* +0x14 */
    unsigned char pad_0x15[0x3];
    unsigned char field_0x18; /* +0x18 */
    unsigned char pad_0x19[0xf];
    byte field_0x28; /* +0x28 */
    undefined1 field_0x29; /* +0x29 */
    undefined1 field_0x2a; /* +0x2a */
};


unsigned int FUN_003a8d20(fn_003a8d20_arg0 *param_1,unsigned int *param_2)

{
  int iVar1;
  unsigned int uVar2;
  unsigned int uVar3;
  unsigned int *puVar4;
  unsigned int uVar5;
  float *pfVar6;
  int iVar7;
  unsigned long long uVar8;
  int *piVar9;
  int iVar10;
  float *pfVar11;
  unsigned char bVar12;
  unsigned int *puVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  float fVar17;
  int aiStack_70 [4];
  unsigned char auStack_60 [32];
  float fStack_40;
  float fStack_3c;
  float fStack_38;
  float fStack_20;
  float fStack_1c;
  
  iVar1 = param_1->field_0x8;
  iVar10 = *(int *)(iVar1 + 0x18);
  puVar13 = (unsigned int *)(DAT_008872e0 + 0x20);
  param_1->field_0xc = *(int *)(iVar10 + 0x54);
  piVar9 = (int *)(iVar10 + DAT_00764820);
  bVar12 = 0;
  if (((unsigned int *)*piVar9 != (unsigned int *)0x0) &&
     ((unsigned int)*(unsigned short *)(*(int *)(iVar10 + 0x54) + 4) == *(unsigned int *)*piVar9)) {
    bVar12 = 1;
  }
  if (bVar12) {
    iVar10 = *piVar9;
  }
  else {
    piVar9 = (int *)FUN_003ce050(piVar9,*(unsigned short *)(*(int *)(iVar10 + 0x54) + 4));
    iVar10 = *piVar9;
  }
  param_1->field_0x10 = iVar10;
  iVar10 = *(int *)(iVar1 + 0x18);
  if ((*(unsigned int *)(iVar10 + 8) & 0x1000000) == 0) {
    *(unsigned int *)&param_1->field_0x18 =
         ((unsigned int)*(unsigned char *)(iVar10 + 0x18) << 0x18) |
         (unsigned int)*(unsigned short *)(param_1->field_0xc + 6) | ((*(unsigned int *)(iVar10 + 8) & 0x9f) << 0x10);
    *(unsigned int *)&param_1->field_0x14 = *(unsigned int *)&param_1->field_0x14 & 0xfffffffe;
    iVar7 = *(int *)(param_1->field_0x10 + 4);
    if (iVar7 != 0) {
      uVar3 = *(unsigned int *)(iVar7 + 0x28);
      uVar5 = *(unsigned int *)&param_1->field_0x18;
      if (uVar5 != uVar3) {
        if ((((uVar5 >> 0x10) & 0xff) == ((uVar3 >> 0x10) & 0xff)) &&
           (uVar5 >> 0x18 == uVar3 >> 0x18)) {
          *(unsigned int *)&param_1->field_0x14 = *(unsigned int *)&param_1->field_0x14 | 4;
        }
        else {
          *(unsigned int *)&param_1->field_0x14 = *(unsigned int *)&param_1->field_0x14 | 8;
        }
      }
      if (*(unsigned short *)(iVar10 + 0xc) != 0) {
        if ((*(unsigned short *)(iVar10 + 0xc) & 1) != 0) {
          *(unsigned int *)&param_1->field_0x14 = *(unsigned int *)&param_1->field_0x14 | 8;
        }
        else {
          *(unsigned int *)&param_1->field_0x14 = *(unsigned int *)&param_1->field_0x14 | 2;
          *(unsigned int *)&param_1->field_0x14 =
               *(unsigned int *)&param_1->field_0x14 | ((unsigned int)*(unsigned short *)(iVar10 + 0xc) << 3);
        }
      }
      if (((*(unsigned int *)(iVar1 + 0x4c) & 1) != 0) &&
         (((long)*(int *)(iVar7 + 0x40) != (long)*(short *)(iVar1 + 0x52) ||
          ((long)*(int *)(iVar7 + 0x3c) != (long)*(short *)(iVar1 + 0x50))))) {
        *(unsigned int *)&param_1->field_0x14 = *(unsigned int *)&param_1->field_0x14 | 4;
      }
    }
  }
  iVar10 = 8;
  *(unsigned short *)(*(int *)(iVar1 + 0x18) + 0xc) = 0;
  *(unsigned int *)(iVar1 + 0x4c) = *(unsigned int *)(iVar1 + 0x4c) & 0xfffffffe;
  param_2 = (unsigned int *)*param_2;
  do {
    iVar10 = iVar10 - 1;
    uVar2 = puVar13[1];
    *param_2 = *puVar13;
    puVar13 = puVar13 + 2;
    param_2[1] = uVar2;
    param_2 = param_2 + 2;
  } while (0 < iVar10);
  iVar7 = 2;
  pfVar6 = (float *)FUN_003bfae0(iVar1);
  iVar10 = 5;
  pfVar11 = (float *)(DAT_008872e0 + DAT_00764a24 + 0x60);
  do {
    if (-pfVar6[3] <
        (pfVar6[2] * pfVar11[2] + *pfVar6 * *pfVar11 + pfVar6[1] * pfVar11[1]) - pfVar11[3]) {
      iVar7 = 1;
      break;
    }
    pfVar11 = pfVar11 + 5;
    bVar12 = iVar10 != 0;
    iVar10 = iVar10 - 1;
  } while (bVar12);
  uVar3 = *(unsigned int *)param_1->field_0xc;
  if ((uVar3 & 1) != 1) {
    if (((uVar3 & 0xff) != 0) && ((uVar3 & 2) != 2)) {
      if ((uVar3 & 0x10) != 0x10) {
        if ((uVar3 & 8) == 8) {
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
  if (iVar7 != 2) {
    param_1->field_0x28 = param_1->field_0x28 | 2;
  }
  param_1->field_0x28 = param_1->field_0x28 | 2;
  param_1->field_0x2a = (*(unsigned int *)(*(int *)(iVar1 + 0x18) + 8) & 0x40) != 0;
  iVar1 = param_1->field_0x8;
  iVar10 = *(int *)(iVar1 + 0x18);
  if ((*(unsigned int *)(iVar10 + 8) & 0x20) != 0) {
    DAT_008872ea = DAT_008872ea + 1;
    uVar8 = FUN_003e9700(*(unsigned int *)(iVar1 + 4));
    FUN_003e0960(auStack_60,uVar8);
    if ((*(unsigned int *)((int)uVar8 + 0xc) & 3) == 3) {
      fStack_20 = 1.0f;
      fStack_1c = 1.0f;
    }
    else {
      fStack_20 = SQRT(fStack_38 * fStack_38 + fStack_40 * fStack_40 + fStack_3c * fStack_3c);
      fStack_1c = fStack_20;
      if (0.0f < fStack_20) {
        fStack_1c = 1.0f / fStack_20;
      }
    }
    iVar10 = *(int *)(iVar10 + 0x54);
    aiStack_70[0] = *(int *)(*(int *)(iVar10 + 0xc) + iVar10 + 0x18) + 0xc;
    FUN_003c96d0(0x3cffa0,aiStack_70);
    for (puVar13 = *(unsigned int **)(iVar1 + 100); puVar13 != (unsigned int *)(iVar1 + 100);
        puVar13 = (unsigned int *)*puVar13) {
      iVar10 = puVar13[2];
      for (puVar4 = *(unsigned int **)(iVar10 + 0x40); puVar4 != (unsigned int *)(iVar10 + 0x40);
          puVar4 = (unsigned int *)*puVar4) {
        iVar7 = puVar4[2];
        if (((iVar7 != 0) && (*(short *)(iVar7 + 0x3c) != DAT_008872ea)) &&
           ((*(unsigned char *)(iVar7 + 2) & 1) != 0)) {
          *(short *)(iVar7 + 0x3c) = DAT_008872ea;
          iVar7 = FUN_003e9700(*(unsigned int *)(puVar4[2] + 4));
          pfVar6 = (float *)FUN_003bfae0(iVar1);
          fVar16 = pfVar6[1] - *(float *)(iVar7 + 0x34);
          fVar17 = *pfVar6 - *(float *)(iVar7 + 0x30);
          fVar14 = pfVar6[2] - *(float *)(iVar7 + 0x38);
          fVar15 = pfVar6[3] + *(float *)(puVar4[2] + 0x14);
          if (fVar14 * fVar14 + fVar17 * fVar17 + fVar16 * fVar16 < fVar15 * fVar15) {
            FUN_003cffa0(puVar4[2],aiStack_70);
          }
        }
      }
    }
  }
  return 1;
}
