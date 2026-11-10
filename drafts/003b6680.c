/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b6680 func_003b6680 src/promoted/code1_003b.c - romwright draft, unreviewed */
typedef struct fn_003b6680_arg2 fn_003b6680_arg2;
struct fn_003b6680_arg2 {
    unsigned char pad_0x0[0x58];
    int field_0x58; /* +0x58 */
    unsigned char pad_0x5c[0x1c];
    unsigned char field_0x78; /* +0x78 */
    unsigned char pad_0x79[0x37];
    float field_0xb0; /* +0xb0 */
    int field_0xb4; /* +0xb4 */
    int field_0xb8; /* +0xb8 */
    unsigned char pad_0xbc[0x4];
    float field_0xc0; /* +0xc0 */
    int field_0xc4; /* +0xc4 */
    int field_0xc8; /* +0xc8 */
};


void FUN_003b6680(int *param_1,int param_2,fn_003b6680_arg2 *param_3,int param_4,unsigned long long param_5)

{
  int iVar1;
  unsigned char bVar2;
  unsigned char bVar3;
  unsigned int uVar4;
  unsigned int uVar5;
  unsigned int uVar6;
  float *pfVar7;
  unsigned int uVar8;
  unsigned int *puVar9;
  float *pfVar10;
  float *pfVar11;
  int iVar12;
  int iVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  float fVar17;
  float fVar18;
  float fVar19;
  float fVar20;
  float fVar21;
  float fVar22;
  
  uVar6 = DAT_0070af48;
  uVar5 = DAT_0070af44;
  uVar4 = DAT_0070af40;
  pfVar10 = (float *)(param_1 + 0xc);
  iVar1 = *(int *)(param_2 + 0x18);
  pfVar11 = (float *)(param_1 + 0xf);
  if ((param_5 & 0x800000) == 0x800000) {
    iVar12 = *(int *)(iVar1 + 0x5c);
    iVar13 = **(int **)(param_2 + DAT_00764700);
    FUN_003c2290(iVar1,2);
    iVar12 = *(int *)(iVar12 + 0x14);
    if (param_4 < iVar13) {
      if (8 < iVar13 - param_4) {
        bVar2 = 0;
        bVar3 = 0;
        if ((param_4 <= iVar13) && (iVar13 < 0x7fffffff)) {
          bVar3 = 1;
        }
        if ((bVar3) && (param_4 < 0x7fffffff)) {
          bVar2 = 1;
        }
        if (bVar2) {
          puVar9 = (unsigned int *)(iVar12 + param_4 * 0x30);
          do {
            *puVar9 = uVar4;
            param_4 = param_4 + 8;
            puVar9[1] = uVar5;
            puVar9[2] = uVar6;
            puVar9[3] = uVar4;
            puVar9[4] = uVar5;
            puVar9[5] = uVar6;
            puVar9[6] = uVar4;
            puVar9[7] = uVar5;
            puVar9[8] = uVar6;
            puVar9[9] = uVar4;
            puVar9[10] = uVar5;
            puVar9[0xb] = uVar6;
            puVar9[0xc] = uVar4;
            puVar9[0xd] = uVar5;
            puVar9[0xe] = uVar6;
            puVar9[0xf] = uVar4;
            puVar9[0x10] = uVar5;
            puVar9[0x11] = uVar6;
            puVar9[0x12] = uVar4;
            puVar9[0x13] = uVar5;
            puVar9[0x14] = uVar6;
            puVar9[0x15] = uVar4;
            puVar9[0x16] = uVar5;
            puVar9[0x17] = uVar6;
            puVar9[0x18] = uVar4;
            puVar9[0x19] = uVar5;
            puVar9[0x1a] = uVar6;
            puVar9[0x1b] = uVar4;
            puVar9[0x1c] = uVar5;
            puVar9[0x1d] = uVar6;
            puVar9[0x1e] = uVar4;
            puVar9[0x1f] = uVar5;
            puVar9[0x20] = uVar6;
            puVar9[0x21] = uVar4;
            puVar9[0x22] = uVar5;
            puVar9[0x23] = uVar6;
            puVar9[0x24] = uVar4;
            puVar9[0x25] = uVar5;
            puVar9[0x26] = uVar6;
            puVar9[0x27] = uVar4;
            puVar9[0x28] = uVar5;
            puVar9[0x29] = uVar6;
            puVar9[0x2a] = uVar4;
            puVar9[0x2b] = uVar5;
            puVar9[0x2c] = uVar6;
            puVar9[0x2d] = uVar4;
            puVar9[0x2e] = uVar5;
            puVar9[0x2f] = uVar6;
            puVar9[0x30] = uVar4;
            puVar9[0x31] = uVar5;
            puVar9[0x32] = uVar6;
            puVar9[0x33] = uVar4;
            puVar9[0x34] = uVar5;
            puVar9[0x35] = uVar6;
            puVar9[0x36] = uVar4;
            puVar9[0x37] = uVar5;
            puVar9[0x38] = uVar6;
            puVar9[0x39] = uVar4;
            puVar9[0x3a] = uVar5;
            puVar9[0x3b] = uVar6;
            puVar9[0x3c] = uVar4;
            puVar9[0x3d] = uVar5;
            puVar9[0x3e] = uVar6;
            puVar9[0x3f] = uVar4;
            puVar9[0x40] = uVar5;
            puVar9[0x41] = uVar6;
            puVar9[0x42] = uVar4;
            puVar9[0x43] = uVar5;
            puVar9[0x44] = uVar6;
            puVar9[0x45] = uVar4;
            puVar9[0x46] = uVar5;
            puVar9[0x47] = uVar6;
            puVar9[0x48] = uVar4;
            puVar9[0x49] = uVar5;
            puVar9[0x4a] = uVar6;
            puVar9[0x4b] = uVar4;
            puVar9[0x4c] = uVar5;
            puVar9[0x4d] = uVar6;
            puVar9[0x4e] = uVar4;
            puVar9[0x4f] = uVar5;
            puVar9[0x50] = uVar6;
            puVar9[0x51] = uVar4;
            puVar9[0x52] = uVar5;
            puVar9[0x53] = uVar6;
            puVar9[0x54] = uVar4;
            puVar9[0x55] = uVar5;
            puVar9[0x56] = uVar6;
            puVar9[0x57] = uVar4;
            puVar9[0x58] = uVar5;
            puVar9[0x59] = uVar6;
            puVar9[0x5a] = uVar4;
            puVar9[0x5b] = uVar5;
            puVar9[0x5c] = uVar6;
            puVar9[0x5d] = uVar4;
            puVar9[0x5e] = uVar5;
            puVar9[0x5f] = uVar6;
            puVar9 = puVar9 + 0x60;
          } while (param_4 < iVar13 - 8);
        }
      }
      if (param_4 < iVar13) {
        puVar9 = (unsigned int *)(iVar12 + param_4 * 0x30);
        do {
          *puVar9 = uVar4;
          param_4 = param_4 + 1;
          puVar9[1] = uVar5;
          puVar9[2] = uVar6;
          puVar9[3] = uVar4;
          puVar9[4] = uVar5;
          puVar9[5] = uVar6;
          puVar9[6] = uVar4;
          puVar9[7] = uVar5;
          puVar9[8] = uVar6;
          puVar9[9] = uVar4;
          puVar9[10] = uVar5;
          puVar9[0xb] = uVar6;
          puVar9 = puVar9 + 0xc;
        } while (param_4 < iVar13);
      }
    }
  }
  pfVar7 = (float *)FUN_003e9700(*(unsigned int *)(DAT_008872e0 + 4));
  if ((param_3->field_0x58 & 0x8000U) != 0x8000) {
    if ((param_3->field_0x58 & 0x20000U) != 0x20000) {
      *pfVar10 = -*pfVar7;
      param_1[0xd] = (int)-pfVar7[1];
      param_1[0xe] = (int)-pfVar7[2];
      *pfVar11 = pfVar7[4];
      param_1[0x10] = (int)pfVar7[5];
      param_1[0x11] = (int)pfVar7[6];
    }
    else {
      fVar17 = *pfVar7;
      fVar14 = pfVar7[2];
      fVar22 = pfVar7[4];
      fVar20 = pfVar7[5];
      fVar21 = pfVar7[6];
      fVar19 = -pfVar7[1];
      fVar15 = (float)FUN_0044b7b0(*(unsigned int *)&param_3->field_0x78);
      fVar16 = (float)FUN_0044b610(*(unsigned int *)&param_3->field_0x78);
      fVar18 = -fVar15;
      *pfVar10 = fVar16 * -fVar17 + fVar18 * fVar22;
      param_1[0xd] = (int)(fVar16 * fVar19 + fVar18 * fVar20);
      param_1[0xe] = (int)(fVar16 * -fVar14 + fVar18 * fVar21);
      *pfVar11 = fVar15 * -fVar17 + fVar16 * fVar22;
      param_1[0x10] = (int)(fVar15 * fVar19 + fVar16 * fVar20);
      param_1[0x11] = (int)(fVar15 * -fVar14 + fVar16 * fVar21);
    }
  }
  else {
    iVar12 = param_3->field_0xb4;
    iVar13 = param_3->field_0xb8;
    *pfVar10 = param_3->field_0xb0;
    param_1[0xd] = iVar12;
    param_1[0xe] = iVar13;
    iVar12 = param_3->field_0xc4;
    iVar13 = param_3->field_0xc8;
    *pfVar11 = param_3->field_0xc0;
    param_1[0x10] = iVar12;
    param_1[0x11] = iVar13;
  }
  uVar8 = (unsigned int)((param_5 & 2) == 2);
  if (uVar8 == 0) {
    uVar8 = (unsigned int)((param_5 & 0x2000) == 0x2000);
  }
  if (uVar8 == 0) {
    uVar8 = (unsigned int)((param_5 & 0x40) == 0x40);
  }
  if (uVar8 == 0) {
    uVar8 = (unsigned int)((param_5 & 0x40000) == 0x40000);
  }
  param_1[2] = uVar8;
  uVar8 = (unsigned int)((param_5 & 0x80) == 0x80);
  if (uVar8 == 0) {
    uVar8 = (unsigned int)((param_5 & 0x100) == 0x100);
  }
  if (uVar8 == 0) {
    uVar8 = (unsigned int)((param_5 & 0x80000) == 0x80000);
  }
  if (uVar8 == 0) {
    uVar8 = (unsigned int)((param_5 & 0x100000) == 0x100000);
  }
  param_1[1] = uVar8;
  *param_1 = 1;
  param_1[3] = (unsigned int)((param_5 & 0x10) == 0x10);
  iVar12 = *(int *)(iVar1 + 0x5c);
  if (*param_1 != 0) {
    FUN_003c2290(iVar1,2);
    param_1[4] = *(int *)(iVar12 + 0x14);
    param_1[5] = 0xc;
  }
  if (param_1[2] != 0) {
    FUN_003c2290(iVar1,8);
    param_1[8] = *(int *)(iVar1 + 0x30);
    param_1[9] = 4;
  }
  if (param_1[1] != 0) {
    FUN_003c2290(iVar1,0x10);
    param_1[6] = *(int *)(iVar1 + 0x34);
    param_1[7] = 8;
  }
  if (param_1[3] != 0) {
    FUN_003c2290(iVar1,4);
    param_1[10] = *(int *)(iVar12 + 0x18);
    param_1[0xb] = 0xc;
  }
  return;
}
