/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003a3670 func_003a3670 src/promoted/code1_003a.c - romwright draft, unreviewed */
typedef struct fn_003a3670_arg0 fn_003a3670_arg0;
struct fn_003a3670_arg0 {
    unsigned char pad_0x0[0x8];
    undefined4 field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x4];
    undefined4 field_0x10; /* +0x10 */
    unsigned char pad_0x14[0x4];
    undefined4 field_0x18; /* +0x18 */
    unsigned char pad_0x1c[0x30];
    int field_0x4c; /* +0x4c */
    uint field_0x50; /* +0x50 */
    uint field_0x54; /* +0x54 */
    int field_0x58; /* +0x58 */
};


int FUN_003a3670(fn_003a3670_arg0 *param_1,unsigned long long param_2,unsigned long long param_3,unsigned long long param_4)

{
  int iVar1;
  unsigned int uVar2;
  unsigned char uVar3;
  unsigned int uVar4;
  int iVar5;
  unsigned int uVar6;
  fn_003a3670_arg0 *pfVar7;
  int iVar8;
  unsigned int uVar9;
  unsigned int uVar10;
  unsigned int uVar11;
  unsigned int uVar12;
  unsigned int uVar13;
  unsigned int *puVar14;
  int iVar15;
  int iVar16;
  int iVar17;
  unsigned int uVar18;
  int iVar19;
  int iVar20;
  int aiStack_30 [4];
  unsigned int uStack_20;
  unsigned int uStack_1c;
  unsigned int uStack_18;
  unsigned int uStack_14;
  unsigned int uStack_10;
  
  iVar19 = 0;
  uVar2 = param_1->field_0x58;
  uVar13 = uVar2 & 4;
  uVar12 = uVar2 & 2;
  uVar11 = uVar2 & 0x40;
  uVar6 = uVar2 & 0x20;
  uVar10 = uVar2 & 0x80;
  uVar9 = uVar2 & 0x100;
  param_1->field_0x50 =
       (unsigned int)(uVar9 == 0x100) +
       (unsigned int)(uVar10 == 0x80) +
       (unsigned int)(uVar6 == 0x20) +
       (unsigned int)(uVar11 == 0x40) +
       (unsigned int)(uVar12 == 2) +
       (unsigned int)(uVar13 == 4) +
       (unsigned int)((uVar2 & 0x10) == 0x10) + (unsigned int)((uVar2 & 1) == 1) + (unsigned int)((uVar2 & 8) == 8);
  if (((param_4 & 4) == 0) && ((uVar2 & 0xeee1fe18) == 0)) {
    iVar19 = 1;
  }
  uVar18 = 2;
  if ((param_4 & 2) == 0) {
    uVar18 = 1;
  }
  if ((param_4 & 0x10) == 0) {
    if (iVar19 == 0) {
      if ((uVar2 & 0x20000000) != 0) {
        uVar4 = 0;
        if ((uVar2 & 1) != 0) {
          uVar4 = 0x10;
        }
        if ((uVar2 & 8) != 0) {
          uVar4 = uVar4 + DAT_00752d24;
        }
        if ((uVar2 & 0x10) != 0) {
          uVar4 = uVar4 + DAT_00752d28;
        }
        if (uVar13 != 0) {
          uVar4 = uVar4 + DAT_00752d2c;
        }
        if (uVar12 != 0) {
          uVar4 = uVar4 + DAT_00752d30;
        }
        if (uVar11 != 0) {
          uVar4 = uVar4 + DAT_00752d34;
        }
        if (uVar6 != 0) {
          uVar4 = uVar4 + DAT_00752d38;
        }
        if (uVar10 != 0) {
          uVar4 = uVar4 + DAT_00752d3c;
        }
        if (uVar9 != 0) {
          uVar4 = uVar4 + DAT_00752d40;
        }
        if ((uVar2 & 9) != 0) {
          uVar4 = (uVar4 + 0xf) & 0xfffffff0;
        }
        param_1->field_0x54 = uVar4;
        uVar2 = 0;
        pfVar7 = param_1;
        do {
          pfVar7->field_0x8 = uVar4;
          uVar2 = uVar2 + 3;
          pfVar7->field_0x10 = uVar4;
          pfVar7->field_0x18 = uVar4;
          pfVar7 = (fn_003a3670_arg0 *)&pfVar7->field_0x18;
        } while (uVar2 < 9);
        iVar1 = (int)param_2 * uVar4;
      }
      else {
        param_1->field_0x54 = 0;
        uVar6 = 0;
        puVar14 = (unsigned int *)0x707e70;
        pfVar7 = param_1;
        do {
          uVar6 = uVar6 + 3;
          pfVar7->field_0x8 = *puVar14;
          pfVar7->field_0x10 = puVar14[1];
          pfVar7->field_0x18 = puVar14[2];
          puVar14 = puVar14 + 3;
          pfVar7 = (fn_003a3670_arg0 *)&pfVar7->field_0x18;
        } while (uVar6 < 9);
        iVar1 = FUN_003a3500(param_2,uVar2);
      }
      if ((param_4 & 8) != 0) {
        iVar1 = (*DAT_008873e8)(400,0x3012f);
      }
      else {
        iVar1 = (*DAT_008873e8)(iVar1 + 400,0x3012f);
      }
      FUN_0043f9c8(iVar1,0,400);
    }
    else {
      param_1->field_0x54 = 0;
      uVar4 = 0;
      puVar14 = (unsigned int *)0x707e70;
      pfVar7 = param_1;
      do {
        uVar4 = uVar4 + 3;
        pfVar7->field_0x8 = *puVar14;
        pfVar7->field_0x10 = puVar14[1];
        pfVar7->field_0x18 = puVar14[2];
        puVar14 = puVar14 + 3;
        pfVar7 = (fn_003a3670_arg0 *)&pfVar7->field_0x18;
      } while (uVar4 < 9);
      uVar4 = (unsigned int)((uVar2 & 0x40040) != 0);
      if ((uVar2 & 0x100100) != 0) {
        uVar4 = uVar4 + 2;
      }
      if ((uVar2 & 0x20020) != 0) {
        uVar4 = uVar4 + 4;
      }
      aiStack_30[0] = 0;
      iVar16 = 0;
      iVar8 = uVar4 * 0xa0;
      iVar17 = iVar8 + 0x707ed0;
      iVar15 = 0;
      uStack_1c = uVar11 != 0 ^ 1;
      uStack_18 = uVar6 != 0 ^ 1;
      uVar2 = *(unsigned int *)(iVar8 + 0x707ee8);
      iVar20 = ((int)param_2 - 1) / *(int *)(iVar8 + 0x707ee0) + 1;
      uStack_14 = uVar10 != 0 ^ 1;
      uStack_10 = uVar9 != 0 ^ 1;
      aiStack_30[3] = uVar13 != 0 ^ 1;
      uStack_20 = uVar12 != 0 ^ 1;
      iVar5 = (uVar2 + 1) * 0x10;
      if (uVar13 != 0) {
        iVar15 = 0x40;
      }
      if (uVar12 != 0) {
        iVar15 = iVar15 + 0x40;
      }
      uVar6 = 0;
      iVar1 = iVar17;
      if (uVar2 != 0) {
        do {
          if (aiStack_30[*(int *)(iVar1 + 0x24)] != 0) {
            iVar15 = iVar15 + *(int *)(iVar1 + 0x2c) * 0x10;
          }
          else {
            iVar16 = iVar16 + *(int *)(iVar1 + 0x28) * 0x10;
          }
          uVar6 = uVar6 + 1;
          iVar1 = iVar1 + 0x18;
        } while (uVar6 < uVar2);
      }
      if ((param_4 & 8) != 0) {
        iVar1 = (*DAT_008873e8)(400,0x3012f);
      }
      else {
        iVar1 = (*DAT_008873e8)(uVar18 * (iVar15 + 0x40 + iVar20 * (iVar5 + iVar16)) + 400,0x3012f);
      }
      if (iVar1 == 0) {
        return 0;
      }
      FUN_0043f9c8(iVar1,0,400);
      *(int *)(iVar1 + 0xfc) = iVar17;
      *(int *)(iVar1 + 0x124) = iVar20;
      *(unsigned int *)(iVar1 + 0x130) = 0x40;
      *(int *)(iVar1 + 0x134) = iVar15;
      *(int *)(iVar1 + 0x128) = iVar5;
      *(int *)(iVar1 + 300) = iVar16;
      FUN_0043f810(iVar1 + 0x100,aiStack_30,0x24);
      *(unsigned int *)(iVar1 + 0xf8) =
           *(unsigned int *)((unsigned int)((param_4 & 0x20) != 0) * 4 + iVar17 + 0x1c);
      uVar2 = *(unsigned int *)(iVar8 + 0x707ee8);
      uVar6 = 1;
      iVar5 = iVar17;
      if (1 < uVar2) {
        do {
          if (aiStack_30[*(int *)(iVar5 + 0x3c)] != 0) {
            uVar3 = 0;
          }
          else {
            uVar3 = (unsigned char)*(unsigned int *)(*(int *)(iVar5 + 0x3c) * 4 + 0x707ea0);
          }
          *(unsigned char *)(iVar1 + uVar6 + 0xf3) = uVar3;
          uVar6 = uVar6 + 1;
          uVar2 = *(unsigned int *)(iVar8 + 0x707ee8);
          iVar5 = iVar5 + 0x18;
        } while (uVar6 < uVar2);
      }
      uVar6 = 0;
      iVar5 = iVar1;
      iVar15 = iVar1;
      if (uVar2 != 0) {
        do {
          if (aiStack_30[*(int *)(iVar17 + 0x24)] != 0) {
            *(unsigned int *)(iVar15 + 0x40) = *(unsigned int *)(iVar17 + 0x2c) | 0x30000000;
            *(unsigned int *)(iVar15 + 0x4c) = *(unsigned int *)(iVar17 + 0x34);
            *(unsigned long long *)(iVar5 + 0x90) = 0;
          }
          else {
            *(unsigned int *)(iVar15 + 0x40) = *(unsigned int *)(iVar17 + 0x28) | 0x30000000;
            *(unsigned int *)(iVar15 + 0x4c) = *(unsigned int *)(iVar17 + 0x30);
            *(unsigned long long *)(iVar5 + 0x90) = (unsigned long long)(unsigned int)(*(int *)(iVar17 + 0x28) << 4) << 0x20;
          }
          uVar6 = uVar6 + 1;
          *(unsigned int *)(iVar15 + 0x48) = *(unsigned int *)(iVar17 + 0x38);
          iVar17 = iVar17 + 0x18;
          iVar5 = iVar5 + 8;
          iVar15 = iVar15 + 0x10;
        } while (uVar6 < *(unsigned int *)(iVar8 + 0x707ee8));
      }
    }
    *(int *)(iVar1 + 0xd8) = iVar19;
    *(unsigned int *)(iVar1 + 0xe8) = uVar18 - 1;
    *(unsigned int *)(iVar1 + 0xe0) = uVar18 < 2 ^ 1;
    *(unsigned long long *)(iVar1 + 200) = 0xe;
    if ((param_4 & 1) != 0) {
      *(unsigned int *)(iVar1 + 0xdc) = 1;
    }
    param_1->field_0x4c = iVar1;
  }
  else {
    iVar1 = (*(code *)PTR_FUN_007084e0)(param_1,param_2,uVar2,param_4);
  }
  return iVar1;
}
